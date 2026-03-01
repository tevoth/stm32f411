# Session Summary — cmsis_max6675_sdcard New Project

## What we did

Created `cmsis_max6675_sdcard/` — a new bare-metal sub-project that reads temperature from a MAX6675 K-type thermocouple sensor over SPI1 and logs it to a raw microSD card over SPI2.

---

## PR #96 — `feat/max6675-sdcard`

New project based on `cmsis_adxl_sdcard`, with the ADXL345 driver replaced by the MAX6675 driver from `cmsis_max6675`.

### What changed vs `cmsis_adxl_sdcard`

| Area | Change |
|---|---|
| SPI1 mode | CPOL=0 / CPHA=0 / MISO\_PULLUP=1 (MAX6675 mode 0, vs ADXL mode 3) |
| Sensor driver | `max6675.c/.h` (copied from cmsis\_max6675); `adxl345` + `spi1_adxl` library removed |
| `CMakeLists.txt` | `spi1_adxl` source/include dropped; SPI1 defines updated |
| `main.c` | Reads temp every 250 ms; logs all four status cases (OK, open, bus\_invalid, timeout) to UART + SD; flushes every 16 samples |
| `uart.c` | Uses `uart_wait_set_limit()` from shared `uart_helpers.h` (matches PR #93 pattern; adxl\_sdcard still has the older local helper) |
| `adc.c` | Not included (was unused in adxl\_sdcard too) |

### Validation
Built clean with `-Werror`. Flash: 13 128 B, SRAM: 672 B.

---

# Session Summary — Code Review & Bug Fix Sweep

## What we did

Reviewed the full codebase for bugs, then fixed them across a series of PRs.

---

## Bugs found and fixed

### PR #91 — `fix/code-review-bugs`
Four bugs fixed:

1. **FatFs file handle leak** (`cmsis_adxl_sdcard_fatfs/src/fatfs_log.c`)
   - `f_close` was missing on two error paths inside `fatfs_log_init()` after a successful `f_open`.
   - Fix: added `f_close(&file)` before each early return.

2. **OVR on multi-byte SPI transmit** (`shared/spi1_core/src/spi.c` — `spi1_transmit`)
   - Full-duplex SPI shifts one RX byte per TX byte. The RX FIFO was never drained during transmit, causing OVR flag on every multi-byte transfer.
   - Fix: added per-iteration RXNE wait + DR read inside the transmit loop.

3. **GPIO MODER transient glitch** (`shared/spi1_core/src/spi.c` — `spi_gpio_init`, PA4/CS)
   - MODER field was OR'd (set bits) before clearing, leaving the pin briefly in an undefined/analog mode.
   - Fix: `GPIOA->MODER &= ~(3U << (4*2))` before `|= (1U << (4*2))`.

4. **`compute_uart_bd` duplicate** (`cmsis_adxl_sdcard/src/uart.c`)
   - Private copy of baud-rate formula instead of shared `uart_compute_bd()`.
   - Fix: added `#include "uart_helpers.h"`, deleted local copy; CMakeLists updated to include `shared/uart_common/inc`.

---

### PR #92 — `fix/fatfs-uart-helper-dupe`
Missed instance from PR #91:

- **Same `compute_uart_bd` duplicate** in `cmsis_adxl_sdcard_fatfs/src/uart.c`.
- Fix: same pattern — include shared helper, remove local copy, update CMakeLists.

---

### PR #93 — `fix/uart-helper-remaining`
Completed the uart helper sweep + one more SPI GPIO fix:

5. **Remaining `compute_uart_bd` duplicates** (3 files still had full private copies):
   - `cmsis_spi/src/uart.c`
   - `cmsis_max6675/src/uart.c`
   - `cmsis_dac/src/uart.c` (USART1/APB2 variant)
   - Each also had a private unbounded `uart_wait_set()` spin-loop; replaced with `uart_wait_set_limit()`.
   - CMakeLists for each updated to add `SHARED_UART_COMMON_DIR` and its `inc/` path.

6. **Pointless forwarding wrappers** in `cmsis_uart1/src/uart.c` and `cmsis_uart2/src/uart.c`:
   - Both had `compute_uart_bd() { return uart_compute_bd(...); }` — a wrapper doing nothing.
   - Fix: deleted wrappers; `uart_set_baudrate` calls `uart_compute_bd` directly.

7. **OTYPER missing OT6 (MISO/PA6)** (`shared/spi1_core/src/spi.c` — `spi_gpio_init`):
   - OTYPER clear only masked OT5 and OT7; OT6 (PA6/MISO) was omitted.
   - On STM32F4, OTYPER resets to 0 (push-pull), but an explicit clear is needed for correctness on warm-reset paths where the register might have been modified.
   - Fix: `GPIOA->OTYPER &= ~(GPIO_OTYPER_OT5 | GPIO_OTYPER_OT6 | GPIO_OTYPER_OT7)`.

---

## Files changed across the sweep

| File | Change |
|---|---|
| `cmsis_adxl_sdcard_fatfs/src/fatfs_log.c` | f_close on error paths |
| `shared/spi1_core/src/spi.c` | OVR fix, MODER clear-before-set, OTYPER OT6 |
| `cmsis_adxl_sdcard/src/uart.c` | → shared helper |
| `cmsis_adxl_sdcard/CMakeLists.txt` | uart_common include |
| `cmsis_adxl_sdcard_fatfs/src/uart.c` | → shared helper |
| `cmsis_adxl_sdcard_fatfs/CMakeLists.txt` | uart_common include |
| `cmsis_spi/src/uart.c` | → shared helper (full migration) |
| `cmsis_spi/CMakeLists.txt` | uart_common include |
| `cmsis_max6675/src/uart.c` | → shared helper (full migration) |
| `cmsis_max6675/CMakeLists.txt` | uart_common include |
| `cmsis_dac/src/uart.c` | → shared helper (full migration, USART1) |
| `cmsis_dac/CMakeLists.txt` | uart_common include |
| `cmsis_uart1/src/uart.c` | remove forwarding wrapper |
| `cmsis_uart2/src/uart.c` | remove forwarding wrapper |

---

## Workflow rules established this session
- Always test-build all affected sub-projects before pushing a PR branch.
- Do not add `Co-Authored-By` lines to commits.
- PR format: `## Summary`, `## Why`, `## Files changed`, `## Validation`.

---

# Session Summary — SD init fix + clock constant centralization

## What we did

Fixed `sdcard_spi_init()` failing in `cmsis_max6675_sdcard` with `R1=0x01` during ACMD41, then centralized clock-frequency constants so shared and app-local modules no longer duplicate `16000000`.

---

## PR #98 — `fix/sdcard-acmd41-and-clock-constants`

### Functional fix

1. **SD card init stuck in IDLE** (`shared/sdcard_spi/src/sdcard_spi.c`)
   - Root cause: `ACMD41` was sent with `arg=0x00000000` even after successful `CMD8`.
   - Fix: send `ACMD41` with HCS set (`0x40000000`) for SD v2 cards.

### Clock constant centralization

2. **Single source of truth for clock frequencies**
   - Added `shared/system_init/inc/system_clock.h`:
     - `SYSTEM_SYSCLK_HZ`
     - `SYSTEM_HCLK_HZ`
     - `SYSTEM_PCLK1_HZ`
     - `SYSTEM_PCLK2_HZ`
   - Updated shared modules to use it:
     - `shared/sdcard_spi/src/sdcard_spi.c`
     - `shared/systick/src/systick_msec_delay.c`
     - `shared/uart_common/src/uart.c`
   - Updated remaining app-local hardcoded sites:
     - `cmsis_uart1/src/uart.c` (APB2 baud clock)
     - `cmsis_dac/src/uart.c` (APB2 baud clock)
     - `cmsis_blinky/src/systick_msec_delay.c` (SysTick reload)
     - `cmsis_blinky/CMakeLists.txt` (add include path for `system_clock.h`)

### Validation

- `cmake --build cmsis_max6675_sdcard/build`
- `cmake -S cmsis_uart1 -B cmsis_uart1/build && cmake --build cmsis_uart1/build`
- `cmake -S cmsis_dac -B cmsis_dac/build && cmake --build cmsis_dac/build`
- `cmake -S cmsis_blinky -B cmsis_blinky/build && cmake --build cmsis_blinky/build`

---

# Session Summary — New project `cmsis_max6675_sdcard_fatfs`

## What we did

Created `cmsis_max6675_sdcard_fatfs` by taking `cmsis_max6675_sdcard` and adding FatFs integration using the same structure as `cmsis_adxl_sdcard_fatfs`.

---

## PR — `feat/max6675-sdcard-fatfs`

### Added project

- New project directory `cmsis_max6675_sdcard_fatfs/` with:
  - MAX6675 driver/app files (`src/main.c`, `src/max6675.c`, `inc/max6675.h`)
  - FatFs core and disk I/O port (`src/fatfs/ff.c`, `src/fatfs/diskio_port.c`, `inc/fatfs/*`)
  - FatFs logger module (`src/fatfs_log.c`, `inc/fatfs_log.h`)
  - Project `CMakeLists.txt`
  - Project `README.md`

### Functional behavior

- Firmware reads MAX6675 over SPI1, prints status/temperature to UART, and appends CSV lines to a FAT32 file on microSD via SPI2.
- CSV file path uses 8.3 name (`0:max6675.csv`) to match current FatFs config (`FF_USE_LFN=0`).
- CSV header: `sample,status,temp_c_x100,raw_hex`.

### Validation

- `cmake -S cmsis_max6675_sdcard_fatfs -B cmsis_max6675_sdcard_fatfs/build`
- `cmake --build cmsis_max6675_sdcard_fatfs/build`
