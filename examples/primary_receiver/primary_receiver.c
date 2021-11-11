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
 * @file primary_receiver.c
 * 
 * @brief example of NRF24L01 setup as a primary receiver using the 
 * NRF24L01 driver. Different payload widths are set for the data pipes,
 * in order to receive different data structures from the transmitter.
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
    .ce = 6 
  };

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

  // provides access to driver functions
  nrf_client_t my_nrf;

  // initialise my_nrf
  printf("Create client: %s\n", (nrf_driver_create_client(&my_nrf)) ? "Y" : "N");

  // configure GPIO pins and SPI
  printf("configure: %s\n", (my_nrf.configure(&my_pins, my_baudrate)) ? "Y" : "N");

  // not using my_config, but instead using default configuration (NULL) 
  printf("initialise: %s\n", (my_nrf.initialise(NULL)) ? "Y" : "N");

  // set data pipe 0 to a one byte payload width
  printf("payload_size DATA_PIPE_0: %s\n", (my_nrf.payload_size(DATA_PIPE_0, ONE_BYTE)) ? "Y" : "N");

  // set data pipe 1 to a five byte payload width
  printf("payload_size DATA_PIPE_1: %s\n", (my_nrf.payload_size(DATA_PIPE_1, FIVE_BYTES)) ? "Y" : "N");

  // set data pipe 2 to a two byte payload width
  printf("payload_size DATA_PIPE_2: %s\n", (my_nrf.payload_size(DATA_PIPE_2, TWO_BYTES)) ? "Y" : "N");

  /**
   * set addresses for DATA_PIPE_0 - DATA_PIPE_3.
   * These are addresses the transmitter will send its packets to.
   */
  my_nrf.rx_destination(DATA_PIPE_0, (uint8_t[]){0x37,0x37,0x37,0x37,0x37});
  my_nrf.rx_destination(DATA_PIPE_1, (uint8_t[]){0xC7,0xC7,0xC7,0xC7,0xC7});
  my_nrf.rx_destination(DATA_PIPE_2, (uint8_t[]){0xC8,0xC7,0xC7,0xC7,0xC7});
  my_nrf.rx_destination(DATA_PIPE_3, (uint8_t[]){0xC9,0xC7,0xC7,0xC7,0xC7});

  // set to RX Mode
  printf("RX Mode: %s\n", (my_nrf.rx_mode()) ? "Y" : "N");

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

  value = debug_address(RX_PW_P0);
  printf("RX_PW_P0: 0x%X\n", value);

  value = debug_address(RX_PW_P1);
  printf("RX_PW_P1: 0x%X\n", value);

  value = debug_address(RX_PW_P2);
  printf("RX_PW_P2: 0x%X\n", value);

  uint8_t buffer[5];
  debug_address_bytes(RX_ADDR_P0, buffer, FIVE_BYTES);
  printf("RX_ADDR_P0: %X %X %X %X %X\n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4]);
  
  debug_address_bytes(RX_ADDR_P1, buffer, FIVE_BYTES);
  printf("RX_ADDR_P1: %X %X %X %X %X\n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4]);

  debug_address_bytes(RX_ADDR_P2, buffer, ONE_BYTE);
  printf("RX_ADDR_P2: %X\n", buffer[0]);

  debug_address_bytes(RX_ADDR_P3, buffer, ONE_BYTE);
  printf("RX_ADDR_P3: %X\n\n", buffer[0]);

  // data pipe number a packet was received on
  uint8_t pipe_number = 0;

  // holds payload_zero sent by the transmitter
  uint8_t payload_zero = 0;

  // holds payload_one sent by the transmitter
  uint8_t payload_one[5];

  // two byte struct sent by transmitter
  typedef struct payload_two_s { uint8_t one; uint8_t two; } payload_two_t;

  // holds payload_two struct sent by the transmitter
  payload_two_t payload_two;

  while (1)
  {
    if (my_nrf.is_packet(&pipe_number))
    {
      switch (pipe_number)
      {
        case DATA_PIPE_0:
          // read payload
          my_nrf.read_packet(&payload_zero, sizeof(payload_zero));

          // receiving a one byte uint8_t payload on DATA_PIPE_0
          printf("\nPacket received:- Payload (%d) on data pipe (%d)\n", payload_zero, pipe_number);
        break;
        
        case DATA_PIPE_1:
          // read payload
          my_nrf.read_packet(payload_one, sizeof(payload_one));

          // receiving a five byte string payload on DATA_PIPE_1
          printf("\nPacket received:- Payload (%s) on data pipe (%d)\n", payload_one, pipe_number);
        break;
        
        case DATA_PIPE_2:
          // read payload
          my_nrf.read_packet(&payload_two, sizeof(payload_two));

          // receiving a two byte struct payload on DATA_PIPE_2
          printf("\nPacket received:- Payload (1: %d, 2: %d) on data pipe (%d)\n", payload_two.one, payload_two.two, pipe_number);
        break;
        
        case DATA_PIPE_3:
        break;
        
        case DATA_PIPE_4:
        break;
        
        case DATA_PIPE_5:
        break;
        
        default:
        break;
      }
    }
  }
  
}