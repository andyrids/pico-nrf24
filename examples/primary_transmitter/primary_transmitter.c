/**
 * Copyright (C) 2021, A. Ridyard.
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License v2.0 as
 * published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
 * GNU General Public License for more details.
 * 
 * @file primary_transmitter.c
 * 
 * @brief example of NRF24L01 setup as a primary transmitter using the 
 * NRF24L01 driver. Different data structures are send to different 
 * receiver data pipes as an example.
 */

#include <stdio.h>

#include "nrf24_driver.h"
#include "pico/stdlib.h"

#include <tusb.h> // TinyUSB tud_cdc_connected()

int main(void)
{
  // initialize all present standard stdio types
  stdio_init_all();

  // wait until the CDC ACM (serial port emulation) is connected
  while (!tud_cdc_connected()) 
  {
    sleep_ms(10);
  }

  // GPIO pin numbers
  pin_manager_t my_pins = { 
    .sck = 2,
    .copi = 3, 
    .cipo = 4, 
    .csn = 5, 
    .ce = 6 };

  /**
   * nrf_manager_t can be passed to the nrf_client_t
   * initialise function, to specify the NRF24L01 
   * configuration. If NULL is passed to the initialise 
   * function, then the default configuration will be used.
   */
  nrf_manager_t my_config = {
    // RF Channel 
    .channel = 110,

    // AW_3_BYTES, AW_4_BYTES, AW_5_BYTES
    .address_width = AW_5_BYTES,

    // RF_DR_250KBPS, RF_DR_1MBPS, RF_DR_2MBPS
    .data_rate = RF_DR_1MBPS,

    // RF_PWR_NEG_18DBM, RF_PWR_NEG_12DBM, RF_PWR_NEG_6DBM, RF_PWR_0DBM
    .power = RF_PWR_0DBM,

    // ARC_NONE...ARC_15RT
    .retr_count = ARC_10RT,

    // ARD_250US, ARD_500US, ARD_750US, ARD_1000US
    .retr_delay = ARD_250US 
  };

  // SPI baudrate
  uint32_t my_baudrate = 1000000;

  nrf_client_t my_nrf;

  printf("Create client: %s\n", (nrf_driver_create_client(&my_nrf)) ? "PASS" : "FAIL" );

  printf("configure: %s\n", (my_nrf.configure(&my_pins, my_baudrate)) ? "PASS" : "FAIL");

  printf("initialise: %s\n", (my_nrf.initialise(NULL)) ? "PASS" : "FAIL");

  printf("payload_size: %s\n", (my_nrf.payload_size(ALL_DATA_PIPES, ONE_BYTE)) ? "PASS" : "FAIL");

  printf("tx_destination: %s\n", (my_nrf.tx_destination((uint8_t[]){0xC9,0xC7,0xC7,0xC7,0xC7})) ? "PASS" : "FAIL");

  printf("TX Mode: %s\n", (my_nrf.tx_mode()) ? "PASS" : "FAIL");

  printf("\nhttps://www.rapidtables.com/convert/number/ascii-hex-bin-dec-converter.html\n");

  uint8_t value = debug_address(CONFIG);
  printf("\nCONFIG: 0x%X\n", value);

  value = debug_address(EN_AA);
  printf("EN_AA: 0x%X\n", value);

  value = debug_address(EN_RXADDR);
  printf("EN_RXADDR: 0x%X\n", value);

  value = debug_address(SETUP_AW);
  printf("SETUP_AW: 0x%X\n", value);

  value = debug_address(SETUP_RETR);
  printf("SETUP_RETR: 0x%X\n", value);

  value = debug_address(RF_SETUP);
  printf("RF_SETUP: 0x%X\n", value);

  uint8_t buffer[5];
  debug_address_bytes(RX_ADDR_P0, buffer, FIVE_BYTES);
  printf("RX_ADDR_P0: %X %X %X %X %X\n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4]);

  debug_address_bytes(TX_ADDR, buffer, FIVE_BYTES);
  printf("TX_ADDR: %X %X %X %X %X\n\n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4]);



  // uint64_t pause_end = delayed_by_ms(get_absolute_time(), 1000); // time to remain in PAUSE state

  uint8_t payload_zero = 123;

  uint8_t payload_one[5] = "Hello";

  typedef struct payload_two_s { uint8_t one; uint8_t two; } payload_two_t;

  payload_two_t payload_two = { .one = 123, .two = 213 };

  fn_status_t success = 0;

  uint64_t time_sent = 0; // time packet was sent
  uint64_t time_response = 0; // response time after packet sent

  while (1) {
    // send to receiver's DATA_PIPE_0 address
    my_nrf.tx_destination((uint8_t[]){0x37,0x37,0x37,0x37,0x37});

    time_sent = to_us_since_boot(get_absolute_time()); // time sent

    success = my_nrf.send_packet(&payload_zero, sizeof(payload_zero));

    time_response = to_us_since_boot(get_absolute_time()); // response time

    printf("payload_zero transmission success:- Response time: %lluμS | Payload: %d\n", time_response - time_sent, payload_zero);

    sleep_ms(5000);

    // send to receiver's DATA_PIPE_1 address
    my_nrf.tx_destination((uint8_t[]){0xC7,0xC7,0xC7,0xC7,0xC7});

    time_sent = to_us_since_boot(get_absolute_time()); // time sent

    success = my_nrf.send_packet(payload_one, sizeof(payload_one));
    
    time_response = to_us_since_boot(get_absolute_time()); // response time

    printf("payload_one transmission success:- Response time: %lluμS | Payload: %s\n", time_response - time_sent, payload_one);

    sleep_ms(5000);

    // send to receiver's DATA_PIPE_2 address
    my_nrf.tx_destination((uint8_t[]){0xC8,0xC7,0xC7,0xC7,0xC7});

    time_sent = to_us_since_boot(get_absolute_time()); // time sent

    success = my_nrf.send_packet(&payload_two, sizeof(payload_two));
    
    time_response = to_us_since_boot(get_absolute_time()); // response time

    printf("payload_two transmission success:- Response time: %lluμS | Payload: %d & %d\n", time_response - time_sent, payload_two.one, payload_two.two);

    sleep_ms(5000);
  }
  
}