add_executable(primary_receiver primary_receiver.c)

target_link_libraries(primary_receiver
    PRIVATE
      nrf24_driver 
      pico_stdlib
)

pico_enable_stdio_usb(primary_receiver 1)
pico_enable_stdio_uart(primary_receiver 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(primary_receiver)