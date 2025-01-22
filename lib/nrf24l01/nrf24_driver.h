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

#include "error_manager/error_manager.h"
#include "hardware/spi.h"


// SETUP_AW register address width (AW) settings
typedef enum address_width_e
{
  AW_3_BYTES = 1, // 3 Byte address width
  AW_4_BYTES = 2, // 4 Byte address width
  AW_5_BYTES = 3 // 5 Byte address width
} address_width_t;


// DYNPD register dynamic payloads settings
typedef enum dyn_payloads_t
{
  DYNPD_DISABLE = 0x00,
  DYNPD_ENABLE = 0x3F
} dyn_payloads_t;


// SETUP_RETR register Automatic Retransmission Delay (ARD) settings
typedef enum retr_delay_e
{
  // Automatic retransmission delay of 250μS
  ARD_250US = (0x00 << 4), 

  // Automatic retransmission delay of 500μS
  ARD_500US = (0x01 << 4), 

  // Automatic retransmission delay of 750μS
  ARD_750US = (0x02 << 4), 

  // Automatic retransmission delay of 1000μS
  ARD_1000US = (0x03 << 4) 
} retr_delay_t;


// SETUP_RETR register Automatic Retransmission Count (ARC) settings
typedef enum retr_count_e 
{
  ARC_NONE = (0x00 << 0), // ARC disabled
  ARC_1RT = (0x01 << 0), // ARC of 1
  ARC_2RT = (0x02 << 0), // ARC of 2
  ARC_3RT = (0x03 << 0), // ARC of 3
  ARC_4RT = (0x04 << 0), // ARC of 4
  ARC_5RT = (0x05 << 0), // ARC of 5
  ARC_6RT = (0x06 << 0), // ARC of 6
  ARC_7RT = (0x07 << 0), // ARC of 7
  ARC_8RT = (0x08 << 0), // ARC of 8
  ARC_9RT = (0x09 << 0), // ARC of 9
  ARC_10RT = (0x0A << 0), // ARC of 10
  ARC_11RT = (0x0B << 0), // ARC of 11
  ARC_12RT = (0x0C << 0), // ARC of 12
  ARC_13RT = (0x0D << 0), // ARC of 13
  ARC_14RT = (0x0E << 0), // ARC of 14
  ARC_15RT = (0x0F << 0) // ARC of 15
} retr_count_t;


// RF_SETUP register Data Rate (DR) settings
typedef enum rf_data_rate_e
{
  RF_DR_1MBPS   = ((0x00 << 5) | (0x00 << 3)), // 1 Mbps
  RF_DR_2MBPS   = ((0x00 << 5) | (0x01 << 3)), // 2 Mbps
  RF_DR_250KBPS = ((0x01 << 5) | (0x00 << 3)), // 250 kbps
} rf_data_rate_t;


// RF_SETUP register RF Power (RF_PWR) settings
typedef enum rf_power_e
{
  // -18dBm Tx output power
  RF_PWR_NEG_18DBM = (0x00 << 1),

  // -12dBm Tx output power
  RF_PWR_NEG_12DBM = (0x01 << 1),

  // -6dBm Tx output power
  RF_PWR_NEG_6DBM  = (0x02 << 1),

  // 0dBm Tx output power
  RF_PWR_0DBM  = (0x03 << 1)
} rf_power_t;


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
  dyn_payloads_t dyn_payloads;
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

  // enables dynamic payloads
  fn_status_t (*dyn_payloads_enable)(void);

  // disables dynamic payloads
  fn_status_t (*dyn_payloads_disable)(void);

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
  fn_status_t (*standby_mode)(void);

  // switch NRF24L01 to RX Mode
  fn_status_t (*receiver_mode)(void);
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


#endif // NRF24L01_H
