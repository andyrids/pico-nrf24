#ifndef NRF24L01_H
#define NRF24L01_H

#include "error_manager.h"
#include "device_config.h"

#include "pico/types.h"

/**** Enumerations & Type definitions ****/

// individual PTX ID, which can correspond to the relevant PRX data pipe
typedef enum ptx_id_e { PTX_0, PTX_1, PTX_2, PTX_3, PTX_4, PTX_5 } ptx_id_t;

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
  uint8_t mode; // Power Down ('P'), Standby ('S'), RX Mode ('R'), TX Mode ('T')
  uint8_t payload_width; // current payload width
  uint8_t address_width; // data pipe address width
  bool is_rx_addr_p0; // RX_ADDR_P0 address cache flag
  uint8_t rx_addr_p0[5]; // cached RX_ADDR_P0 address

} device_status_t;

typedef struct is_reponse_s
{
  bool is_acknowledge;
  bool is_timeout;
  bool is_waiting;
} is_response_t;

/**** PUBLIC Driver Functions ****/

/**
 * 1. Initialise I/O for USB serial and all present stdio types.
 * 
 * 2. Ascertain correct SPI interface (spi0 or spi1), based on 
 * CIPO, COPI, CSN and SCK pins.
 * 
 * 3. Initialise SPI interface at 6Mhz.
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
 * @return PASS (1) || FAIL (0) depending on success
 */
external_status_t init_pico(uint8_t cipo_pin, uint8_t copi_pin, uint8_t csn_pin, uint8_t sck_pin, uint8_t ce_pin, uint32_t baudrate_hz);

// initial config when device first powered
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
 * @param payload_width number of bytes in payload
 * 
 * @return PASS (1) || FAIL (0) depending on success
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
 * @param retr_delay Auto Retransmit Delay (ARD_250US, ARD_500US, ARD_750US, ARD_1000US)
 * @param retr_count Auto Retransmit Count (ARC_NONE, ARC_1RT...ARC_15RT)
 * 
 * @return PASS (1) || FAIL (0) depending on success
 */
external_status_t set_auto_retransmission(retr_delay_t retr_delay, retr_count_t retr_count);

/**
 * Set the RF data rates in the RF_SETUP register
 * through the RF_DR_LOW and RF_DR_HIGH bits.
 * 
 * rf_setup_dr_t values are accessible through nrf24l01.h
 * 
 * @param data_rate (RF_DR_1MBPS, RF_DR_2MBPS, RF_DR_250KBPS)
 * 
 * @return PASS (1) || FAIL (0) depending on success
 */
external_status_t set_rf_data_rate(rf_data_rate_t data_rate);

/**
 * Set Tx mode power level in RF_SETUP register through
 * the RF_PWR bits (1:2).
 * 
 * @param rf_pwr RF power (RF_PWR_NEG_18DBM, RF_PWR_NEG_12DBM, RF_PWR_NEG_6DBM, RF_PWR_0DBM)
 * 
 * @return PASS (1) || FAIL (0) depending on success
 */
external_status_t set_rf_power(rf_power_t rf_pwr);

/**
 * 1. Read CONFIG register value and check PRIM_RX bit. If 
 * PRIM_RX is already set, leave and drive CE pin HIGH. If 
 * PRIM_RX is not set, set the PRIM_RX bit and write to the
 * CONFIG register and drive CE pin HIGH.
 * 
 * NOTE: State diagram in the datasheet (6.1.1) highlights
 * conditions for entering Rx and Tx operating modes. One 
 * condition is the value of PRIM_RX (bit 0) in the CONFIG
 * register. PRIM_RX = 1 for Rx mode or 0 for Tx mode. CE
 * pin is driven HIGH for Rx mode and is only driven high 
 * in Tx mode to facilitate the Tx of data (10us+).
 * 
 * @return PASS (1) || FAIL (0) depending on success
 */
external_status_t set_tx_mode(void);

/**
 * Puts the NRF24L01+ into RX Mode.
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
 * @return PASS (1) || FAIL (0) depending on success
 */
external_status_t set_rx_mode(void);


/**
 * Setup TX_ADDR register, which is used by a primary transmitter (PTX).
 * The TX_ADDR must be the same as an address in primary receiver (PRX) 
 * RX_ADDR_P0 - RX_ADDR_P5 registers, for the PTX and PRX to communicate.
 * 
 * NOTE: If using auto-acknowledge feature, the PTX must have its RX_ADDR_P0 
 * register set with the same address as the TX_ADDR register. This driver
 * uses auto-acknowledge by default and the set_tx_address function writes 
 * the address argument to the the RX_ADDR_P0 by design.
 * 
 * @param address 5 byte address, (uint8_t){0x37, 0x37, 0x37, 0x37, 0x37}
 * 
 * @return PASS (1) || FAIL (0) depending on success
 */
external_status_t set_tx_address(const uint8_t *address);

external_status_t set_rx_address(data_pipe_t data_pipe, const uint8_t *buffer);

external_status_t tx_packet(const void *tx_packet, size_t packet_size);

external_status_t rx_packet(void *rx_packet, size_t packet_size);

external_status_t is_rx_packet(void);

uint8_t debug_address(register_map_t reg);

void debug_address_bytes(register_map_t reg, uint8_t *buffer, uint8_t buffer_size);

#endif // NRF24L01_H