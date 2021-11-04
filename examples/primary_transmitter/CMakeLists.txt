add_executable(primary_transmitter primary_transmitter.c)

target_link_libraries(primary_transmitter 
    PRIVATE 
      nrf24_driver 
      pico_stdlib
)

pico_enable_stdio_usb(primary_transmitter 1)
pico_enable_stdio_uart(primary_transmitter 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(primary_transmitter)