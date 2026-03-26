cmake_minimum_required(VERSION 3.15)

# ch32_freertos.cmake — Build FreeRTOS for CH32V20x as a static library.
#
# Provides:
#   ch32_freertos (STATIC library — compiled FreeRTOS kernel)
#
# Required variables:
#   REPO_ROOT        — absolute path to monorepo root
#   CH32_CHIP_SERIES — e.g. "CH32V20x" (lowercased for directory lookup)
#
# Required targets (must already exist):
#   ch32_hal — chip HAL (for FreeRTOSConfig.h, system headers)

set(FREERTOS_PORT A_CUSTOM_PORT CACHE STRING "")

# ---- FreeRTOS Port for CH32V2xx/CH32V3xx ----

add_library(freertos_kernel_port OBJECT
    "port.c"
    "extras.c"
    "portASM.S"
    "picolibc_locks.c"
)


add_library(freertos_kernel_port_headers INTERFACE)

target_include_directories(freertos_kernel_port_headers INTERFACE
    "include"
    "chip_specific_extensions/RV32I_PFIC_no_extensions"
)

target_compile_definitions(freertos_kernel_port PUBLIC
    # Required by arduino_core_ch32 startup assembly (startup_ch32v20x_D6.S)
    # to enable the FreeRTOS interrupt vector table setup.
    __PIO_BUILD_FREERTOS__
)

target_link_libraries(freertos_kernel_port PUBLIC
    freertos_kernel_include
    freertos_kernel_port_headers
    ch32_hal
)
