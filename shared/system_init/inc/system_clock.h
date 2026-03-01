#pragma once

#include <stdint.h>

// Keep these in sync with shared/system_init/src/system_init.c.
// Current policy: HSI selected as SYSCLK with AHB/APB1/APB2 prescalers at /1.
#define SYSTEM_SYSCLK_HZ 16000000U
#define SYSTEM_HCLK_HZ   SYSTEM_SYSCLK_HZ
#define SYSTEM_PCLK1_HZ  SYSTEM_HCLK_HZ
#define SYSTEM_PCLK2_HZ  SYSTEM_HCLK_HZ

