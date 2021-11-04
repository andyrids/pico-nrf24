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
 * @brief example of NRF24L01 setup as a primary receiver using the 
 * NRF24L01 driver.
 */

#include <stdio.h>

#include "nrf24_driver.h"
#include "pico/stdlib.h"

const uint8_t SCK_PIN = 2;
const uint8_t MOSI_PIN = 3;
const uint8_t MISO_PIN = 4;
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
 * 4. Set the address to transmit to (set_tx_address)
 * 5. Set NRF24L01 to TX Mode (set_tx_mode)
 * 6. Exit setup
 */
external_status_t setup(void) {
  
  // external_status_t PASS (1) or FAIL (0) is returned by all PUBLIC API functions
  external_status_t status = PASS;

  typedef enum states_e	{	INIT_PICO, INIT_NRF24, SET_PAYLOAD_SIZE, SET_TX_ADDRESS, SET_TX_MODE, EXIT } states_t;

  // pointer to array of states
  uint8_t *setup_states = (uint8_t []){ INIT_PICO, INIT_NRF24, SET_PAYLOAD_SIZE, SET_TX_ADDRESS, SET_TX_MODE, EXIT };

  // initial state is INIT_PICO
  uint8_t next_state = INIT_PICO;

  // while next_state is not EXIT
  while (next_state != EXIT)
  {

    // test current value then increment pointer
    switch (*(setup_states++)) 
    {
      case INIT_PICO:
        status = init_pico(MISO_PIN, MOSI_PIN, CSN_PIN, SCK_PIN, CE_PIN, BAUDRATE_HZ);
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

      case SET_TX_ADDRESS: // same address as in primary_receiver example data pipe 0
        status = set_tx_address((uint8_t[]){0x37, 0x37, 0x37, 0x37, 0x37});
        next_state = (status == PASS) ? *setup_states : EXIT;
      break;

      case SET_TX_MODE:
        status = set_tx_mode();
        next_state = (status == PASS) ? *setup_states : EXIT;
      break;

      default:
        status = FAIL;
        next_state = EXIT;
      break;
    }
  }
  // indicate final state reached.
  printf("Reached setup state %d of 5, before EXIT.\n\n", *(setup_states));

  // if any state transition failed, status == FAIL
  return status;
}

int main(void) {
  // initialize all present standard stdio types
  stdio_init_all();

  // descriptive enum for all static & transitional states
  typedef enum ptx_states_e { TX_PACKET, AUTO_ACK_RECEIVED, PAUSE, ERROR } ptx_states_t;

  /**
   * pointer to array of the static & transitional states.
   * state enum value is the same as its array index.
   */
  uint8_t *ptx_states = (uint8_t []){ TX_PACKET, AUTO_ACK_RECEIVED, PAUSE, ERROR };

  // used to set/reset ptx_states position
  uint8_t *initial_state = ptx_states;

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
  printf("CONFIG: 0x%X\n", value);

  value = debug_address(EN_AA);
  printf("EN_AA: 0x%X\n", value);

  value = debug_address(EN_RXADDR);
  printf("EN_RXADDR: 0x%X\n", value);

  value = debug_address(SETUP_AW);
  printf("SETUP_AW: 0x%X\n", value);

  value = debug_address(SETUP_RETR);
  printf("SETUP_RETR: 0x%X\n", value);

  value = debug_address(RF_SETUP);
  printf("RF_SETUP: 0x%X\n\n", value);

  uint8_t buffer[5];
  debug_address_bytes(RX_ADDR_P0, buffer, FIVE_BYTES);
  printf("RX_ADDR_P0: %X %X %X %X %X\n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4]);

  debug_address_bytes(TX_ADDR, buffer, FIVE_BYTES);
  printf("TX_ADDR: %X %X %X %X %X\n\n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4]); */

  sleep_ms(1000);

  // initial state is TX_PACKET
  uint8_t next_state = TX_PACKET;

  // packet to transmit
  uint8_t packet = 1;

  uint64_t time_sent = 0; // time packet was sent
  uint64_t time_response = 0; // response time after packet sent
  uint64_t pause_end = 0; // time to remain in PAUSE state

  // break loop on MAX_RETRIES_REACHED, indicating PRX is not reachable
  while (next_state != ERROR)
  {
    switch (*ptx_states) // initial value is TX_PACKET
    {
      case TX_PACKET:
          time_sent = to_us_since_boot(get_absolute_time()); // time sent
          next_state = tx_packet(&packet, sizeof(packet)) ? AUTO_ACK_RECEIVED : ERROR;
          time_response = to_us_since_boot(get_absolute_time()); // response time

          // &(initial_state[AUTO_ACK_RECEIVED]) or &(initial_state[ERROR])
          ptx_states = &(initial_state[next_state]); // ptx_states = initial_state + next_state
      break;

      case AUTO_ACK_RECEIVED:
        printf("Transmission success:- Response time: %lluμS | Payload: %d\n", time_response - time_sent, packet);

        next_state = PAUSE;

        // &(initial_state[PAUSE])
        ptx_states = &(initial_state[next_state]); // ptx_states = initial_state + next_state

        pause_end = delayed_by_ms(get_absolute_time(), 5000); // 5000ms from current time

        // increment value each time a packet is transmitted
        packet++;
      break;

      case PAUSE:
        // time difference in μs between current time and pause_end
        if (!absolute_time_diff_us(get_absolute_time(), pause_end))
        {
          // transitioning back to TX_PACKET
          next_state = TX_PACKET;

          // &(initial_state[TX_PACKET])
          ptx_states = &(initial_state[next_state]); // ptx_states = initial_state + next_state
        }
      break;

      default:
        next_state = ERROR;

        // &(initial_state[ERROR])
        ptx_states = &(initial_state[next_state]); // ptx_states = initial_state + next_state
      break;
    }
  }

  if (next_state == ERROR)
  {
    printf("Packet transmission failed, or no auto-acknowledgement received before max retransmissions reached.");
    while (1) { tight_loop_contents(); }
  }
}