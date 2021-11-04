# Adds a library target called nrf24_driver for the NRF24L01 driver
# STATIC libraries are archives of object files for use when linking other targets
# INTERFACE libraries do not compile sources and don't produce a library artifact on disk
add_library(nrf24_driver INTERFACE)

# PRIVATE means these sources should only be added to nrf24_driver
# PUBLIC wuould mean these sources should be added to nrf24_driver and to any 
# target that links to nrf24_driver library. 
target_sources(nrf24_driver
    INTERFACE 
      ${CMAKE_CURRENT_LIST_DIR}/nrf24_driver.c
)

# ${CMAKE_CURRENT_LIST_DIR} (nrf24_driver.h)
# ${CMAKE_CURRENT_LIST_DIR}/error_manager (error_manager.h)
# ${CMAKE_CURRENT_LIST_DIR}/pin_manager (pin_manager.h)
# ${CMAKE_CURRENT_LIST_DIR}/spi_manager (spi_manager.h)
target_include_directories(nrf24_driver 
    INTERFACE 
      ${CMAKE_CURRENT_LIST_DIR}
      ${CMAKE_CURRENT_LIST_DIR}/error_manager
      ${CMAKE_CURRENT_LIST_DIR}/pin_manager
      ${CMAKE_CURRENT_LIST_DIR}/spi_manager
)

# Link nrf24_driver against pico-sdk;
# pico_stdlib, hardware_spi & hardware_gpio libraries
target_link_libraries(nrf24_driver 
    INTERFACE
      pico_stdlib
      hardware_spi 
      hardware_gpio
)

# Each subdirectory added, has Further target sources for 
# the nrf24_driver library.
add_subdirectory(spi_manager)
add_subdirectory(pin_manager)
add_subdirectory(error_manager)