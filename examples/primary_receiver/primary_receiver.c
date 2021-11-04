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
 * NRF24L01 driver.
 */

#include <stdio.h>

#include "nrf24_driver.h"
#include "pico/stdlib.h"

const uint8_t SCK_PIN = 2;
const uint8_t COPI_PIN = 3;
const uint8_t CIPO_PIN = 4;
const uint8_t CSN_PIN = 5;
const uint8_t CE_PIN = 6;

const uint32_t BAUDRATE_HZ = 6000000;
const uint8_t RF_CHANNEL = 110;

/**
 * A setup function to carry out preliminary steps before
 * Pico and NRF24L01 are ready to use.
 * 
 * 1. Initialise GPIO pins & spi (init_pico)
 * 2. Initialise the NRF24L01 (init_nrf24)
 * 3. Set the payload size (set_payload_size)
 * 4. Set the address to receive on data pipe 0 (set_rx_address)
 * 5. Set the address to receive on data pipe 1 (set_rx_address)
 * 6. Set NRF24L01 to RX Mode (set_rx_mode) 
 * 6. Exit setup
 */
external_status_t setup(void) {
  
  // external_status_t PASS (1) or FAIL (0) is returned by all PUBLIC API functions
  external_status_t status = PASS;

  // initial setup states
  typedef enum states_e
  {
    INIT_PICO, 
    INIT_NRF24, 
    SET_PAYLOAD_SIZE,
    SET_RX_ADDRESS_ONE, 
    SET_RX_ADDRESS_TWO,
    SET_RX_MODE, 
    EXIT
  } states_t;

  // pointer to array of states
  uint8_t *setup_states = (uint8_t []){ 
    INIT_PICO, 
    INIT_NRF24, 
    SET_PAYLOAD_SIZE, 
    SET_RX_ADDRESS_ONE, 
    SET_RX_ADDRESS_TWO, 
    SET_RX_MODE, 
    EXIT 
  };

  // initial state is INIT_PICO
  uint8_t next_state = INIT_PICO;

  // while next_state is not EXIT
  while (next_state != EXIT)
  {
    // test current value then increment pointer
    switch (*(setup_states++)) 
    {
      case INIT_PICO:
        status = init_pico(CIPO_PIN, COPI_PIN, CSN_PIN, SCK_PIN, CE_PIN, BAUDRATE_HZ);
        next_state = (status == PASS) ? *setup_states : EXIT;
      break;

      case INIT_NRF24:
        status = init_nrf24(RF_CHANNEL);
        next_state = (status == PASS) ? *setup_states : EXIT;
      break;

      case SET_PAYLOAD_SIZE:
        status = set_payload_size(ALL_DATA_PIPES, ONE_BYTE);
        next_state = (status == PASS) ? *setup_states : EXIT;
      break;

      case SET_RX_ADDRESS_ONE: // matches primary_transmitter TX_ADDR value
        status = set_rx_address(DATA_PIPE_0, (uint8_t[]){0x37, 0x37, 0x37, 0x37, 0x37});
        next_state = (status == PASS) ? *setup_states : EXIT;
      break;

      case SET_RX_ADDRESS_TWO:
        status = set_rx_address(DATA_PIPE_1, (uint8_t[]){0xC7, 0xC7, 0xC7, 0xC7, 0xC7});
        next_state = (status == PASS) ? *setup_states : EXIT;
      break;

      case SET_RX_MODE:
        status = set_rx_mode();
        next_state = (status == PASS) ? *setup_states : EXIT;
      break;

      default:
        status = FAIL;
        next_state = EXIT;
      break;
    }
  }

  // indicate final state reached.
  printf("Reached setup state %d of 6, before EXIT.\n\n", *setup_states);

  // if any state transition failed, status == FAIL
  return status;
}

int main(void) {
  // initialize all present standard stdio types
  stdio_init_all();

  // descriptive enum for all static & transitional states
  typedef enum prx_states_e { WAIT_FOR_PACKET, READ_PACKET, PRINT_PACKET, ERROR } prx_states_t;

  /**
   * pointer to array of the static & transitional states.
   * state enum value is the same as its array index.
   */
  uint8_t *prx_states = (uint8_t[]){ WAIT_FOR_PACKET, READ_PACKET, PRINT_PACKET, ERROR };

  // used to set/reset ptx_states position
  uint8_t *initial_state = prx_states;

  sleep_ms(10000); // gives time to open/restart PuTTy

  // attempt setup of NRF24L01
  if (setup() != PASS)
  {
    printf("NRF24L01 setup failed.\n");
    // hold in perminent loop
    while (1) { tight_loop_contents(); }
  }

 /*  for debugging
  uint8_t value = debug_address(CONFIG);
  printf("CONFIG: 0x%x\n", value);

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
  
  debug_address_bytes(RX_ADDR_P1, buffer, FIVE_BYTES);
  printf("RX_ADDR_P1: %X %X %X %X %X\n\n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4]); */

  sleep_ms(1000);

  // initial state is WAIT_FOR_PACKET
  uint8_t next_state = WAIT_FOR_PACKET;

  // packet buffer to receive
  uint8_t packet = 0;

  while (next_state != ERROR)
  {
    switch (*prx_states)
    {
      case WAIT_FOR_PACKET:
        // if packet in RX FIFO, transition to RX_PACKET state
        next_state = is_rx_packet() ? READ_PACKET : WAIT_FOR_PACKET;
        // &(initial_state[RX_PACKET])
        prx_states = &(initial_state[next_state]); // prx_states = initial_state + next_state			
      break;

      case READ_PACKET:
        next_state = rx_packet(&packet, ONE_BYTE) ? PRINT_PACKET : ERROR;
        // &(initial_state[PRINT_PACKET]) or &(initial_state[ERROR])
        prx_states = &(initial_state[next_state]); // prx_states = initial_state + next_state
      break;

      case PRINT_PACKET:
        printf("Packet receipt success:- Payload: %d\n", packet);
        next_state = WAIT_FOR_PACKET;
        // &(initial_state[WAIT_FOR_PACKET])
        prx_states = &(initial_state[next_state]); // prx_states = initial_state + next_state
      break;

      default:
        next_state = ERROR;
        prx_states = &(initial_state[next_state]);
      break;
    }
  }

  if (next_state == ERROR)
  {
    printf("Packet receipt failed.\n");
    while (1) { tight_loop_contents(); }
  }
}