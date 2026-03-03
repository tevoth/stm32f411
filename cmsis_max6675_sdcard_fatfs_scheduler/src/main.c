#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "fatfs_log.h"
#include "led.h"
#include "max6675.h"
#include "spi.h"
#include "stm32f4xx.h"
#include "system_clock.h"
#include "system_init.h"
#include "uart.h"

#define LOG_LINE_BUF_SZ 96U
#define ARRAY_LEN(x) (sizeof(x) / sizeof((x)[0]))

typedef struct {
  uint32_t timestamp_ms;
  uint16_t raw;
  int32_t temp_c_x100;
  max6675_status_t status;
  uint32_t seq;
} sample_t;

typedef struct {
  uint32_t dispatch_count;
  uint32_t late_count;
  uint32_t skipped_slots;
  uint32_t max_lateness_ms;
  uint32_t max_runtime_ms;
} task_stats_t;

typedef struct {
  const char *name;
  uint32_t period_ms;
  uint32_t next_run_ms;
  bool enabled;
  task_stats_t stats;
  void (*run)(uint32_t now_ms);
} task_t;

static volatile uint32_t g_millis = 0U;
static sample_t g_latest_sample = {0};
static fatfs_log_file_t g_log_file = {0};
static bool g_sd_logging_enabled = false;

static void systick_init_1khz(void);
static uint32_t millis(void);
static bool time_reached(uint32_t now, uint32_t target);
static void task_sample_sensor(uint32_t now_ms);
static void task_uart_report(uint32_t now_ms);
static void task_sd_log(uint32_t now_ms);
static void task_uart_stats(uint32_t now_ms);

static task_t g_tasks[] = {
  {"sample", 250U, 0U, true, {0}, task_sample_sensor},
  {"uart", 500U, 0U, true, {0}, task_uart_report},
  {"sd", 1000U, 0U, true, {0}, task_sd_log},
  {"stats", 5000U, 0U, true, {0}, task_uart_stats},
};

void SysTick_Handler(void) {
  g_millis++;
}

static void systick_init_1khz(void) {
  const uint32_t reload = (SYSTEM_HCLK_HZ / 1000U) - 1U;

  SysTick->LOAD = reload;
  SysTick->VAL = 0U;
  SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |
                  SysTick_CTRL_TICKINT_Msk |
                  SysTick_CTRL_ENABLE_Msk;
}

static uint32_t millis(void) {
  uint32_t now;
  __disable_irq();
  now = g_millis;
  __enable_irq();
  return now;
}

static bool time_reached(uint32_t now, uint32_t target) {
  return (int32_t)(now - target) >= 0;
}

static void task_sample_sensor(uint32_t now_ms) {
  uint16_t raw = 0U;
  const max6675_status_t status = max6675_read_status(&raw);

  g_latest_sample.timestamp_ms = now_ms;
  g_latest_sample.raw = raw;
  g_latest_sample.status = status;
  g_latest_sample.temp_c_x100 = (status == MAX6675_STATUS_OK) ? max6675_temp_c_x100(raw) : 0;
  g_latest_sample.seq++;

  led_toggle();
}

static void task_uart_report(uint32_t now_ms) {
  (void)now_ms;

  static uint32_t last_reported_seq = 0U;
  if (g_latest_sample.seq == 0U || g_latest_sample.seq == last_reported_seq) {
    return;
  }

  char uart_line[LOG_LINE_BUF_SZ];
  int n = 0;

  switch (g_latest_sample.status) {
    case MAX6675_STATUS_OK:
      n = snprintf(uart_line, sizeof(uart_line),
        "t=%" PRIu32 "ms, temp=%" PRId32 ".%02" PRId32 " C (raw=0x%04X)\n",
        g_latest_sample.timestamp_ms,
        g_latest_sample.temp_c_x100 / 100,
        g_latest_sample.temp_c_x100 % 100,
        (unsigned)g_latest_sample.raw);
      break;

    case MAX6675_STATUS_THERMOCOUPLE_OPEN:
      n = snprintf(uart_line, sizeof(uart_line),
        "t=%" PRIu32 "ms, fault=open (raw=0x%04X)\n",
        g_latest_sample.timestamp_ms,
        (unsigned)g_latest_sample.raw);
      break;

    case MAX6675_STATUS_BUS_INVALID:
      n = snprintf(uart_line, sizeof(uart_line),
        "t=%" PRIu32 "ms, fault=bus_invalid\n",
        g_latest_sample.timestamp_ms);
      break;

    case MAX6675_STATUS_TIMEOUT:
    default:
      n = snprintf(uart_line, sizeof(uart_line),
        "t=%" PRIu32 "ms, fault=timeout\n",
        g_latest_sample.timestamp_ms);
      break;
  }

  if (n > 0) {
    printf("%s", uart_line);
  }

  last_reported_seq = g_latest_sample.seq;
}

static void task_sd_log(uint32_t now_ms) {
  (void)now_ms;

  static uint32_t last_logged_seq = 0U;
  if (!g_sd_logging_enabled || g_latest_sample.seq == 0U || g_latest_sample.seq == last_logged_seq) {
    return;
  }

  char csv_line[LOG_LINE_BUF_SZ];
  int n = 0;

  if (g_latest_sample.status == MAX6675_STATUS_OK) {
    n = snprintf(csv_line, sizeof(csv_line),
      "%" PRIu32 ",%" PRId32 "\r\n",
      g_latest_sample.timestamp_ms,
      g_latest_sample.temp_c_x100);
  } else {
    n = snprintf(csv_line, sizeof(csv_line),
      "%" PRIu32 ",\r\n",
      g_latest_sample.timestamp_ms);
  }

  if ((n <= 0) || !fatfs_fprintf(&g_log_file, csv_line)) {
    printf("microSD FAT32 append failed; disabling SD logging\n");
    g_sd_logging_enabled = false;
    return;
  }

  last_logged_seq = g_latest_sample.seq;
}

static void task_uart_stats(uint32_t now_ms) {
  (void)now_ms;

  printf("sched_stats: name run late skip maxLate maxRun\n");
  for (size_t i = 0; i < ARRAY_LEN(g_tasks); i++) {
    if (g_tasks[i].run == task_uart_stats) {
      continue;
    }

    printf("sched_stats: %s %" PRIu32 " %" PRIu32 " %" PRIu32 " %" PRIu32 "ms %" PRIu32 "ms\n",
      g_tasks[i].name,
      g_tasks[i].stats.dispatch_count,
      g_tasks[i].stats.late_count,
      g_tasks[i].stats.skipped_slots,
      g_tasks[i].stats.max_lateness_ms,
      g_tasks[i].stats.max_runtime_ms);
  }
}

int main(void) {
  if (!system_init()) {
    while (1) {
      printf("system_init failed\n");
    }
  }

  led_init();
  uart_init();
  spi_gpio_init();
  spi1_config();
  systick_init_1khz();

  printf("\n=== MAX6675 + FAT32 microSD scheduler demo ===\n");
  printf("SPI1: MAX6675, USART2: console, SPI2: microSD(FAT32)\n");
  printf("task periods: sample=250ms uart=500ms sd=1000ms stats=5000ms\n");

  g_sd_logging_enabled = fatfs_fopen(&g_log_file, 0);
  if (g_sd_logging_enabled) {
    printf("microSD FAT32 log init OK\n");
  } else {
    printf("microSD FAT32 init FAILED, continuing with UART only\n");
  }

  const uint32_t start_ms = millis();
  for (size_t i = 0; i < ARRAY_LEN(g_tasks); i++) {
    g_tasks[i].next_run_ms = start_ms + g_tasks[i].period_ms;
  }

  while (1) {
    const uint32_t now_ms = millis();

    for (size_t i = 0; i < ARRAY_LEN(g_tasks); i++) {
      task_t *const task = &g_tasks[i];
      if (!task->enabled) {
        continue;
      }

      if (time_reached(now_ms, task->next_run_ms)) {
        const uint32_t lateness_ms = now_ms - task->next_run_ms;
        if (lateness_ms > 0U) {
          task->stats.late_count++;
          if (lateness_ms > task->stats.max_lateness_ms) {
            task->stats.max_lateness_ms = lateness_ms;
          }

          task->stats.skipped_slots += (lateness_ms / task->period_ms);
        }

        const uint32_t run_start_ms = now_ms;
        task->run(now_ms);
        const uint32_t run_time_ms = millis() - run_start_ms;
        if (run_time_ms > task->stats.max_runtime_ms) {
          task->stats.max_runtime_ms = run_time_ms;
        }

        task->stats.dispatch_count++;
        task->next_run_ms += task->period_ms;
      }
    }

    __WFI();
  }

  return 0;
}
