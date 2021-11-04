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

#include "pico/types.h"


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


// cache of NRF24L01 settings
typedef struct device_status_s
{
  // Power Down ('P'), Standby ('S'), RX Mode ('R'), TX Mode ('T')
  uint8_t mode;

  // data pipe a packet was received on
  uint8_t rx_pipe_no;

  // current payload width
  uint8_t payload_width;

  // data pipe address width
  uint8_t address_width; 

  // RX_ADDR_P0 address cache flag
  bool is_rx_addr_p0; 

  // cached RX_ADDR_P0 address
  uint8_t rx_addr_p0[5]; 
} device_status_t;


/**
 * 1. Initialise I/O for USB serial and all present stdio types.
 * 
 * 2. Ascertain correct SPI interface (spi0 or spi1), based on 
 * CIPO, COPI, CSN and SCK pins.
 * 
 * 3. Initialise SPI interface at specified baudrate.
 * 
 * 4. Call init_pico_pins function to initialise GPIO pins 
 * and set functions for each pin.
 * 
 * @param cipo_pin CIPO GPIO number
 * @param copi_pin COPI GPIO number
 * @param csn_pin CSN GPIO number
 * @param sck_pin SCK GPIO number
 * @param ce_pin CE GPIO number
 * @param baudrate_hz SPI baudrate in Hz
 * 
 * @return PASS (1), FAIL (0)
 */
external_status_t init_pico(uint8_t cipo_pin, uint8_t copi_pin, uint8_t csn_pin, uint8_t sck_pin, uint8_t ce_pin, uint32_t baudrate_hz);



/**
 * Initialise NRF24L01 registers, leaving the device in 
 * Standby Mode. Also sets the specified RF channel.
 * 
 * WiFi uses most of the lower channels and so, the 
 * highest 25 channels (100 - 124) are recommended 
 * for nRF24L01 projects.
 * 
 * Initial configuration:
 * 
 * Air data rate: 1Mbps
 * Power Amplifier: 0dBm
 * Enhanced ShockBurst: enabled
 * CRC: enabled, 2 byte encoding scheme
 * Address width: 5 bytes
 * Auto Retransmit Delay: 500μS
 * Auto Retransmit Count: 10
 * Dynamic payload: disabled
 * Acknowledgment payload: disabled
 * 
 * @param rf_channel channel
 * 
 * @return PASS (1), FAIL (0)
 */
external_status_t init_nrf24(uint8_t rf_channel);


/**
 * Set number of bytes in Tx payload for an individual data pipe or
 * for all data pipes, in the RX_PW_P0 - RX_PW_P5 registers.
 * 
 * data_pipe_t values:
 * 
 * DATA_PIPE_0 - DATA_PIPE_5, ALL_DATA_PIPES (6)
 * 
 * @param data_pipe DATA_PIPE_0 - DATA_PIPE_5 or ALL_DATA_PIPES
 * @param payload_width bytes in payload
 * 
 * @return PASS (1), FAIL (0)
 */
external_status_t set_payload_size(data_pipe_t data_pipe, uint8_t payload_width);


/**
 * Set Auto Retransmit Delay (ARD) and Auto Retransmit Count (ARC) 
 * in the SETUP_RETR register. The datasheet states that the delay
 * is defined from end of the transmission to start of the next 
 * transmission.
 * 
 * NOTE: ARD is the time the PTX is waiting for an ACK packet before 
 * a retransmit is made. The PTX is in RX mode for 250μS (500μS in 
 * 250kbps mode) to wait for address match. If the address match is 
 * detected, it stays in RX mode to the end of the packet, unless 
 * ARD elapses. Then it goes to standby-II mode for the rest of the
 * specified ARD. After the ARD, it goes to TX mode and retransmits 
 * the packet.
 * 
 * @param retr_delay ARD_250US, ARD_500US, ARD_750US, ARD_1000US
 * @param retr_count ARC_NONE, ARC_1RT...ARC_15RT
 * 
 * @return PASS (1), FAIL (0)
 */
external_status_t set_auto_retransmission(retr_delay_t retr_delay, retr_count_t retr_count);


/**
 * Set the RF data rates in the RF_SETUP register
 * through the RF_DR_LOW and RF_DR_HIGH bits.
 * 
 * @param data_rate RF_DR_1MBPS, RF_DR_2MBPS, RF_DR_250KBPS
 * 
 * @return PASS (1), FAIL (0)
 */
external_status_t set_rf_data_rate(rf_data_rate_t data_rate);


/**
 * Set TX Mode power level in RF_SETUP register through
 * the RF_PWR bits.
 * 
 * @param rf_pwr RF_PWR_NEG_18DBM, RF_PWR_NEG_12DBM, RF_PWR_NEG_6DBM, RF_PWR_0DBM
 * 
 * @return PASS (1), FAIL (0)
 */
external_status_t set_rf_power(rf_power_t rf_pwr);


/**
 * Puts the NRF24L01 into TX Mode.
 * 
 * 1. Read CONFIG register value and check PRIM_RX bit. If 
 * PRIM_RX is already set, leave and drive CE pin HIGH. If 
 * PRIM_RX is not set, set the PRIM_RX bit and write to the
 * CONFIG register and drive CE pin HIGH.
 * 
 * NOTE: State diagram in the datasheet (6.1.1) highlights
 * conditions for entering RX and TX operating modes. One 
 * condition is the value of PRIM_RX (bit 0) in the CONFIG
 * register. PRIM_RX = 1 for RX Mode or 0 for TX Mode. CE
 * pin is driven HIGH for Rx mode and is only driven high 
 * in TX Mode to facilitate the Tx of data (10us+).
 * 
 * @return PASS (1), FAIL (0)
 */
external_status_t set_tx_mode(void);

/**
 * Puts the NRF24L01 into RX Mode.
 * 
 * 1. Read CONFIG register value and the value of PRIM_RX bit. 
 * If PRIM_RX is already set, leave and drive CE pin HIGH. If 
 * not already set, set the PRIM_RX bit and write to CONFIG
 * register and drive CE pin HIGH.
 * 
 * NOTE: State diagram in the datasheet (6.1.1) highlights
 * conditions for entering Rx and Tx operating modes. One 
 * condition is the value of PRIM_RX (bit 0) in the CONFIG
 * register. PRIM_RX = 1 for Rx mode or 0 for Tx mode. CE
 * pin is driven HIGH for Rx mode and is only driven high 
 * in Tx mode to facilitate the Tx of data (10us+).
 * 
 * @return PASS (1), FAIL (0)
 */
external_status_t set_rx_mode(void);


/**
 * Set the destination address for a packet transmission, into the
 * TX_ADDR register.
 * 
 * The TX_ADDR register is used by a primary transmitter (PTX) for 
 * transmission of data packets and it's value must be the same as 
 * an address in primary receiver (PRX) RX_ADDR_P0 - RX_ADDR_P5 
 * registers, for the PTX and PRX to communicate.
 * 
 * NOTE: For the auto-acknowledgement feature, the PTX must have its 
 * RX_ADDR_P0 register set with the same address as the TX_ADDR register. T
 * his driver enables auto-acknowledgement by default and the set_tx_address 
 * function writes the address argument to the the RX_ADDR_P0 by design.
 * 
 * @param address (uint8_t){0x37, 0x37, 0x37, 0x37, 0x37} etc.
 * 
 * @return PASS (1), FAIL (0)
 */
external_status_t set_tx_address(const uint8_t *buffer);


/**
 * This function sets the address in buffer to the specified data pipe. 
 * Addresses for DATA_PIPE_0 (0), DATA_PIPE_1 (1) and the remaining data 
 * pipes can be set with multiple set_rx_address calls. Data pipes 2 - 5 
 * use the 4 MSB of data pipe 1 address and should be set with 1 byte. 
 * 
 * NOTE: If you do use a 5 byte buffer for data pipes 2 - 5, then the 
 * function will only write one byte (buffer[0]).
 * 
 * @param address (uint8_t){0x37, 0x37, 0x37, 0x37, 0x37} etc.
 * 
 * @return PASS (1), FAIL (0)
 */
external_status_t set_rx_address(data_pipe_t data_pipe, const uint8_t *buffer);


/**
 * Transmits a payload to a recipient NRF24L01 and will return PASS (1) if 
 * the transmission was successful and an auto-acknowledgement was received 
 * from the recipient NRF24L01. A return value of FAIL (0) indicates that 
 * either, the packet transmission failed, or no auto-acknowledgement was 
 * received before max retransmissions count was reached.
 * 
 * @param tx_packet packet for transmission
 * @param packet_size size of tx_packet
 * 
 * @return PASS (1), FAIL (0)
 */
external_status_t tx_packet(const void *tx_packet, size_t packet_size);


/**
 * Read an available packet from the RX FIFO into the 
 * buffer (rx_packet).
 * 
 * @param rx_packet packet buffer for receipt
 * @param packet_size size of rx_packet
 * 
 * @return PASS (1), FAIL (0)
 */
external_status_t rx_packet(void *rx_packet, size_t packet_size);


/**
 * Polls the STATUS register to ascertain if there is a packet 
 * in the RX FIFO. The function will return PASS (1) if there 
 * is a packet available to read, or FAIL (0) if not. 
 * 
 * @return PASS (1), FAIL (0)
 */
external_status_t is_rx_packet(void);


/**
 * Polls the STATUS register to ascertain if there is a packet 
 * in the RX FIFO. The data pipe number the packet was received
 * on will be stored in the buffer argument. The function will 
 * return PASS (1) if there is a packet available to read, or 
 * FAIL (0) if not. 
 * 
 * @param buffer data pipe number
 * 
 * @return PASS (1), FAIL (0)
 */
external_status_t is_rx_packet_pipe(uint8_t *buffer);


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