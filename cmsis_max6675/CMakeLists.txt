cmake_minimum_required(VERSION 3.22)

# Bare-metal STM32, startup is in C (no CMSIS/HAL/other libs assumed).
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# --- Toolchain (arm-none-eabi) ---
set(TOOLCHAIN_PREFIX arm-none-eabi-)
set(CMAKE_C_COMPILER   ${TOOLCHAIN_PREFIX}gcc)
set(CMAKE_OBJCOPY      ${TOOLCHAIN_PREFIX}objcopy)
set(CMAKE_OBJDUMP      ${TOOLCHAIN_PREFIX}objdump)
set(CMAKE_SIZE         ${TOOLCHAIN_PREFIX}size)

project(stm32_baremetal C)

# --- Target config (EDIT THESE) ---
set(TARGET_NAME    firmware)
set(LINKER_SCRIPT  ${CMAKE_SOURCE_DIR}/ld/stm32_flash.ld)  # <-- your .ld file
set(SHARED_SPI1_CORE_DIR ${CMAKE_SOURCE_DIR}/../shared/spi1_core)
set(SHARED_SPI_COMMON_DIR ${CMAKE_SOURCE_DIR}/../shared/spi_common)

# CPU/ABI flags (pick the right core/FPU for your STM32)
set(MCPU_FLAGS   
  -mcpu=cortex-m4
)
set(MFLOAT_FLAGS 
  -mthumb 
  -mfloat-abi=hard 
  -mfpu=fpv4-sp-d16
)  # remove/adjust for no-FPU parts


# Default build type
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# --- Sources ---
# Expect:
#   - src/startup.c defines vector table + Reset_Handler (or your entry)
#   - src/*.c contains the rest of your firmware (including main.c)
file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS
  ${CMAKE_SOURCE_DIR}/src/*.c
)

add_executable(${TARGET_NAME}.elf ${APP_SOURCES})
target_sources(${TARGET_NAME}.elf PRIVATE
  ${SHARED_SPI1_CORE_DIR}/src/spi.c
)

target_include_directories(${TARGET_NAME}.elf PUBLIC
  ${CMAKE_SOURCE_DIR}/inc
  ${SHARED_SPI1_CORE_DIR}/inc
  ${SHARED_SPI_COMMON_DIR}/inc
  ${CMAKE_SOURCE_DIR}/../CMSIS/Include
  ${CMAKE_SOURCE_DIR}/../CMSIS/Device/ST/STM32F4xx/Include
)

# --- Compile options ---
target_compile_options(${TARGET_NAME}.elf PRIVATE
  ${MCPU_FLAGS}
  ${MFLOAT_FLAGS}
  -ffunction-sections
  -fdata-sections
  -fno-common
  -Wall -Wextra -Wundef -Wshadow -Wdouble-promotion -Werror
  -Wno-unused-parameter
  $<$<CONFIG:Debug>:-Og -g3>
  $<$<CONFIG:Release>:-O3 -g0>
)

# turn on/off STM32F411xE compile (will fail if off!)
set(STM32F411xE ON CACHE BOOL "turn on F411xE")
target_compile_definitions(${TARGET_NAME}.elf PRIVATE
  SPI1_CFG_CPOL=0
  SPI1_CFG_CPHA=0
  SPI1_CFG_MISO_PULLUP=1
  $<$<BOOL:${STM32F411xE}>:STM32F411xE=1>
)

# --- Link options (use your linker script; no external libs assumed) ---
# -nostdlib/-nostartfiles means:
#   you provide Reset_Handler + vector table + data/bss init + call main().
target_link_options(${TARGET_NAME}.elf PRIVATE
  ${MCPU_FLAGS}
  ${MFLOAT_FLAGS}
  -T${LINKER_SCRIPT}
  -Wl,-Map=${CMAKE_BINARY_DIR}/${TARGET_NAME}.map
  -Wl,--gc-sections
  -Wl,--print-memory-usage
  --specs=nosys.specs
  --specs=nano.specs
)

# Relink when the .ld changes
set_property(TARGET ${TARGET_NAME}.elf APPEND PROPERTY LINK_DEPENDS ${LINKER_SCRIPT})

# --- Produce .bin/.hex and show size ---
add_custom_command(TARGET ${TARGET_NAME}.elf POST_BUILD
  COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${TARGET_NAME}.elf>
  COMMAND ${CMAKE_OBJCOPY} -O ihex   $<TARGET_FILE:${TARGET_NAME}.elf> ${CMAKE_BINARY_DIR}/${TARGET_NAME}.hex
  COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${TARGET_NAME}.elf> ${CMAKE_BINARY_DIR}/${TARGET_NAME}.bin
  COMMENT "Generating .hex/.bin and printing size"
)

# Optional: disassembly listing target
add_custom_target(disasm
  COMMAND ${CMAKE_OBJDUMP} -d -S $<TARGET_FILE:${TARGET_NAME}.elf> > ${CMAKE_BINARY_DIR}/${TARGET_NAME}.lst
  DEPENDS ${TARGET_NAME}.elf
  COMMENT "Generating disassembly listing"
)
