# PRIVATE means these sources should only be added to nrf24_driver
target_sources(nrf24_driver
  	INTERFACE 
      ${CMAKE_CURRENT_LIST_DIR}/spi_manager.c
)