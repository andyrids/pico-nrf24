cmake_minimum_required(VERSION 3.13)

# Pull in SDK (must be before project)
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(pico_nrf24 
  VERSION 1.0 
  DESCRIPTION "Raspberry Pi Pico NRF24L01P driver" 
  LANGUAGES C CXX
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the SDK
pico_sdk_init()

add_compile_options(-Wall -Wno-unused-function)

add_subdirectory(lib)
add_subdirectory(examples)