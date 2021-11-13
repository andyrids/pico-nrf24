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
 * @file device_config.h
 * 
 * @brief enumerations, type definitions for the full nRF24L01 
 * register map and specific register bit mnemonics that are 
 * useful for interfacing with the NRF24L01P over SPI.
 */

#ifndef DEVICE_CONFIG_H
#define DEVICE_CONFIG_H    

// SPI commands [8.3.1 in Datasheet]
typedef enum register_commands_e
{
  // Read register [000] + [5 bit register address]
  R_REGISTER = 0x00, 

  // Write register [001] + [5 bit register address]
  W_REGISTER = 0x20, 

  // No Operation
  NOP = 0xFF 
} register_commands_t;


// SPI commands [8.3.1 in Datasheet]
typedef enum payload_commands_e
{
  // Read RX payload width for top payload in RX FIFO
  R_RX_PL_WID = 0x60, 

  // Read Rx payload (0b01100001)
  R_RX_PAYLOAD = 0x61, 

  // Write Tx payload (0b10100000)
  W_TX_PAYLOAD = 0xA0, 

  // Write Payload for Tx with ACK packet on pipe 0 
  W_ACK_PAYLOAD_P0 = 0xA8,

  // Write Payload for Tx with ACK packet on pipe 1
  W_ACK_PAYLOAD_P1 = 0xA9,

  // Write Payload for Tx with ACK packet on pipe 2
  W_ACK_PAYLOAD_P2 = 0xAA,

  // Write Payload for Tx with ACK packet on pipe 3 
  W_ACK_PAYLOAD_P3 = 0xAB,

  // Write Payload for Tx with ACK packet on pipe 4
  W_ACK_PAYLOAD_P4 = 0xAC,

  // Write Payload for Tx with ACK packet on pipe 5
  W_ACK_PAYLOAD_P5 = 0xAD,

  // Disables AUTOACK on this specific packet
  W_TX_PAYLOAD_NOACK = 0xB0, 

  // Reuse last transmitted payload
  REUSE_TX_PL = 0xE3, 
} payload_commands_t;


// SPI commands [8.3.1 in Datasheet]
typedef enum fifo_commands_e
{
  FLUSH_TX = 0xE1, // Flush TX FIFO (TX Mode)
  FLUSH_RX = 0xE2 // Flush RX FIFO (RX Mode)
} fifo_commands_t;


// Register map [9 in Datasheet]
typedef enum register_map_e
{
  CONFIG = 0x00, // Configuration register
  EN_AA = 0x01, // Enhanced ShockBurst
  EN_RXADDR = 0x02, // Enabled Rx addresses
  SETUP_AW = 0x03, // Data pipe address width
  SETUP_RETR = 0x04, // Setup automatic retransmission
  RF_CH = 0x05, // RF Channel
  RF_SETUP = 0x06, // RF Setup register
  STATUS = 0x07, // Status register
  OBSERVE_TX = 0x08, // Tx observe register
  RPD = 0x09, // Received Power Detector (NRF24L01+)
  RX_ADDR_P0 = 0x0A, // Rx address data pipe 0
  RX_ADDR_P1 = 0x0B, // Rx address data pipe 1
  RX_ADDR_P2 = 0x0C, // Rx address data pipe 2
  RX_ADDR_P3 = 0x0D, // Rx address data pipe 3
  RX_ADDR_P4 = 0x0E, // Rx address data pipe 4
  RX_ADDR_P5 = 0x0F, // Rx address data pipe 5
  TX_ADDR = 0x10, // Tx address (for PTX)
  RX_PW_P0 = 0x11, // Rx payload bytes in data pipe 0
  RX_PW_P1 = 0x12, // Rx payload bytes in data pipe 1
  RX_PW_P2 = 0x13, // Rx payload bytes in data pipe 2
  RX_PW_P3 = 0x14, // Rx payload bytes in data pipe 3
  RX_PW_P4 = 0x15, // Rx payload bytes in data pipe 4
  RX_PW_P5 = 0x16, // Rx payload bytes in data pipe 5
  FIFO_STATUS = 0x17, // FIFO Status register
  DYNPD = 0x1C, // Enable dynamic payload length
  FEATURE = 0x1D // Feature register
} register_map_t;


// Bitwise masks [REGISTER][MNEMONIC/DESCRIPTION]['MASK']
typedef enum bitwise_masks_e
{
  FIFO_STATUS_FIFO_MASK = 0x01, // 0b00000001
  RF_SETUP_RF_PWR_MASK = 0x06, // 0b00000110
  STATUS_RX_P_NO_MASK = 0x07, // 0b00000111
  REGISTER_MASK = 0x1F, // 0b00011111
  RF_SETUP_RF_DR_MASK = 0x28, // 0b00101000
  STATUS_INTERRUPT_MASK = 0x70 // 0b01110000
} bitwise_masks_t;  


/**
 * CONFIG register (0x00):
 * 
 * Mnemonic    | Bit | Set | Comment
 * (reserved)     7     0    Only '0' allowed
 * MASK_RX_DR     6     0    Mask interrupt caused by RX_DR
 * MASK_TX_DS     5     0    Mask interrupt caused by TX_DS
 * MASK_MAX_RT    4     0    Mask interrupt caused by MAX_RT
 * EN_CRC         3     1    Enable CRC
 * CRCO           2     1    CRC encoding scheme. 0: 1 byte, 1: 2 bytes
 * PWR_UP         1     1    1: Power up, 0: Power down
 * PRIM_RX        0     0    Rx & Tx control. 1: PRX, 0: PTX 
 * 
 **/
typedef enum config_bit_e
{
  CONFIG_PRIM_RX, // Bit 0
  CONFIG_PWR_UP, // Bit 1
  CONFIG_CRCO, // Bit 2
  CONFIG_EN_CRC, // Bit 3
  CONFIG_MASK_MAX_RT, // Bit 4
  CONFIG_MASK_TX_DS, // Bit 5
  CONFIG_MASK_RX_DR, // Bit 6
  CONFIG_RESERVED_0, // Bit 7 - Only '0' allowed
} config_bit_t;


/**
 * SETUP_AW register (0x03):
 * 
 * Setup Address Widths for all data pipes
 * 01: 3 bytes, 10: 4 bytes, 11: 5 bytes
 * 
 * Mnemonic    | Bit |  Set  | Comment
 * (reserved)   2:7   000000   Only '000000' allowed
 * AW           0:1    11      Rx or Tx Address field width 
 **/
typedef enum setup_aw_bit_e
{
  SETUP_AW_AW, // Bit 0:1
  SETUP_AW_RESERVED_1 = 2, // Bit 2
  SETUP_AW_RESERVED_2, // Bit 3
  SETUP_AW_RESERVED_3, // Bit 4
  SETUP_AW_RESERVED_4, // Bit 5
  SETUP_AW_RESERVED_5, // Bit 6
  SETUP_AW_RESERVED_6 // Bit 7
} setup_aw_bit_t;


/**
 * RF_CH register (0x05):
 * 
 * Set the nRF24L01+ frequency channel
 * 
 * Mnemonic    | Bit |   Set   | Comment
 * (reserved)     7      0       Only '0' allowed
 * RF_CH        0:6   1001100    Channel 2 - 127
 * 
 * Value written to RF_CH register: 0b01001100 (76)
 **/
typedef enum rf_ch_bit_e
{
  RF_CH_RF_CH, // Bit 0:6
  RF_CH_RESERVED = 7 // Bit 7
} rf_ch_bit_t;


/**
 * SETUP_RETR register (0x04):
 * 
 * Setup of Automatic Retransmission.
 * 
 * ARD- 0000: Wait 250µS, 0001: Wait 500µS, 0010: Wait 750µS... etc.
 * ARC- 0000: Disabled, 0001: 1 retransmit ... 1111: 15 retransmit
 * 
 * Mnemonic    | Bit |  Set  | Comment
 * ARD          4:7    0100    Auto Retransmit Delay
 * ARC          0:3    1010    Auto Retransmit Count
 **/
typedef enum setup_retr_bit_e
{
  SETUP_RETR_ARC, // Bit 0
  SETUP_RETR_ARD = 4, // Bit 4
} setup_retr_bit_t;


/**
 * RF_SETUP register (0x06):
 * 
 * RF_PWR:- 00: -18dBm, 01: -12dBm, 10: -6dBm, 11: 0dBm
 * 
 * Mnemonic    | Bit | Set | Comment
 * CONT_WAVE      7     0    Enable continuous carrier transmit
 * (reserved)     6     0    Only '0' allowed
 * RF_DR_LOW      5     0    Set RF Data Rate to 250kbps
 * PLL_LOCK       4     0    Force PLL lock signal
 * RF_DR_HIGH     3     0    Select between the high speed data rates
 * RF_PWR        1:2   11    Set RF output power in TX mode
 * (obsolete)     0     0
 * 
 **/
typedef enum rf_setup_bit_e
{
  RF_SETUP_OBSOLETE, // Bit 0
  RF_SETUP_RF_PWR, // Bit 1
  RF_SETUP_RF_DR_HIGH = 3, // Bit 3
  RF_SETUP_PLL_LOCK, // Bit 4
  RF_SETUP_RF_DR_LOW, // Bit 5
  RF_SETUP_RESERVED, // Bit 6
  RF_SETUP_CONT_WAVE, // Bit 7
} rf_setup_bit_t;


/**
 * STATUS register (0x07):
 * 
 * RX_DR, TX_DS, MAX_RT:- Write 1 to clear bit
 * 
 * TX_FULL:- 1: Tx FIFO full, 0: Tx FIFO not full
 * 
 * RX_P_NO:- 000-101: Data Pipe, 111: Rx FIFO Empty
 * 
 * Mnemonic    | Bit | Set | Comment
 * (reserved)     7     0    Only '0' allowed
 * RX_DR          6     1    Data Ready RX FIFO interrupt
 * TX_DS          5     1    Data Sent TX FIFO interrupt
 * MAX_RT         4     1    Maximum Tx retransmits interrupt
 * RX_P_NO       1:3   111   Data pipe number for available payload 
 * TX_FULL        0     0    Tx FIFO full flag
 **/
typedef enum status_bit_e
{
  STATUS_TX_FULL, // Bit 0 - TX FIFO full flag
  STATUS_RX_P_NO, // Bit 1:3 - Data pipe number for available payload from RX_FIFO
  STATUS_MAX_RT = 4, // Bit 4 - Maximum number of TX retransmits interrupt
  STATUS_TX_DS, // Bit 5 - Data Sent TX FIFO interrupt
  STATUS_RX_DR, // Bit 6 - Data Ready RX FIFO interrupt
  STATUS_RESERVED_0, // Bit 7 - Only '0' allowed
} status_bit_t;


// FIFO_STATUS register bit mnemonics
typedef enum fifo_status_bit_e
{
  FIFO_STATUS_RX_EMPTY,
  FIFO_STATUS_RX_FULL,
  FIFO_STATUS_RESERVED_0,
  FIFO_STATUS_TX_EMPTY,
  FIFO_STATUS_TX_FULL,
  FIFO_STATUS_TX_REUSE,
  FIFO_STATUS_RESERVED_1
} fifo_status_bit_t;


typedef enum feature_bit_e
{
  // Enables the W_TX_PAYLOAD_NOACK command
  FEATURE_EN_DYN_ACK,
  // Enables Payload with ACK
  FEATURE_EN_ACK_PAY,
  // Enables Dynamic Payload Length
  FEATURE_EN_DPL,
  // reserved
  FEATURE_RESERVED,
} feature_bit_t;


// EN_AA register enable AA settings
typedef enum en_auto_ack_e
{
  ENAA_NONE = 0x00,
  ENAA_P0 = (0x01 << 0),
  ENAA_P1 = (0x01 << 1),
  ENAA_P2 = (0x01 << 2),
  ENAA_P3 = (0x01 << 3),
  ENAA_P4 = (0x01 << 4),
  ENAA_P5 = (0x01 << 5),
  ENAA_ALL = 0x3F
} en_auto_ack_t;

#endif // DEVICE_CONFIG_h