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
 * @file nrf24_driver.h
 * 
 * @brief main interface for the NRF24L01 driver. Enumerations and type
 * definitions used in function calls.
 */

#ifndef NRF24L01_H
#define NRF24L01_H

#include "error_manager.h"
#include "device_config.h"

#include "hardware/spi.h"


// descriptive typedef for number of bytes written over SPI to NRF24L01
typedef enum number_of_bytes_e 
{ 
  ZERO_BYTES, ONE_BYTE, TWO_BYTES, THREE_BYTES, FOUR_BYTES, 
  FIVE_BYTES, SIX_BYTES, SEVEN_BYTES, EIGHT_BYTES, MAX_BYTES = 32 
} number_of_bytes_t;


// descriptive typedef for a particular data pipe
typedef enum data_pipe_e
{ 
  DATA_PIPE_0, DATA_PIPE_1, DATA_PIPE_2,
  DATA_PIPE_3, DATA_PIPE_4, DATA_PIPE_5,
  ALL_DATA_PIPES
} data_pipe_t;


// descriptive typedef for a bitwise operations testing individual bits
typedef enum bit_set_e { UNSET_BIT, SET_BIT } bit_set_t;

// GPIO information for pin_manager utility functions
typedef struct pin_manager_s
{
  uint8_t copi;
  uint8_t cipo;
  uint8_t sck;
  uint8_t csn;
  uint8_t ce;
} pin_manager_t;

// SPI information for spi_manager utility functions
typedef struct spi_manager_s
{ 
  uint32_t baudrate;
  spi_inst_t *instance; 
} spi_manager_t;

// NRF24L01 config information for nrf_driver utility functions
typedef struct nrf_manager_s 
{
  address_width_t address_width;
  retr_delay_t retr_delay;
  retr_count_t retr_count;
  rf_data_rate_t data_rate;
  rf_power_t power;
  uint8_t channel;
} nrf_manager_t;

// provides access to nrf_driver public functions
typedef struct nrf_client_s
{
  // configure user pins and the SPI interface
  fn_status_t (*configure)(pin_manager_t* user_pins, uint32_t baudrate_hz);

  // initialise the NRF24L01. A NULL argument will use default configuration.
  fn_status_t (*initialise)(nrf_manager_t* user_config);

  // set an address for a data pipe, a packet from another NRF24L01 will transmit to, for this NRF24L01
  fn_status_t (*rx_destination)(data_pipe_t data_pipe, const uint8_t *buffer);

  // set an address a packet from this NRF24L01 will transmit to, for a recipient NRF24L01
  fn_status_t (*tx_destination)(const uint8_t *buffer);

  // set payload size for received packets for one specific data pipe or for all
  fn_status_t (*payload_size)(data_pipe_t data_pipe, size_t size);

  // set packet auto-retransmission delay and count configurations
  fn_status_t (*auto_retransmission)(retr_delay_t delay, retr_count_t count);

  // set RF channel
  fn_status_t (*rf_channel)(uint8_t channel);

  // set the RF data rate
  fn_status_t (*rf_data_rate)(rf_data_rate_t data_rate);

  // set the RF power level, whilst in TX mode
  fn_status_t (*rf_power)(rf_power_t rf_power);

  // send a packet
  fn_status_t (*send_packet)(const void *tx_packet, size_t size);

  // read a received packet
  fn_status_t (*read_packet)(void *rx_packet, size_t size);

  // indicates if a packet has been received and is ready to read
  fn_status_t (*is_packet)(uint8_t *rx_p_no);

  // switch NRF24L01 to TX Mode
  fn_status_t (*tx_mode)(void);

  // switch NRF24L01 to RX Mode
  fn_status_t (*rx_mode)(void);
} nrf_client_t;


/**
 * Takes the nrf_client_t argument and sets the function pointers
 * to the appropriate nrf_driver_[function_pointer_name] function.
 * 
 * @param client nrf_client_t struct
 * 
 * @return NRF_MNGR_OK (3), ERROR (0)
 */
fn_status_t nrf_driver_create_client(nrf_client_t *client);


/**
 * Returns value of the specified register. 
 * 
 * @param reg register address
 * 
 * @return PASS (1), FAIL (0)
 */
uint8_t debug_address(register_map_t reg);


/**
 * Reads a multiple byte value of the specified 
 * register into the buffer.
 * 
 *@param reg register address
 *@param buffer buffer for register value
 *@param buffer_size size of buffer
 * 
 * @return PASS (1), FAIL (0)
 */
void debug_address_bytes(register_map_t reg, uint8_t *buffer, size_t buffer_size);


#endif // NRF24L01_H