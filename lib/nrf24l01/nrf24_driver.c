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
 * @file nrf24_driver.c
 * 
 * @brief function declarations for the main NRF24L01 driver interface.
 */
#include <string.h>
#include "pin_manager.h"
#include "spi_manager.h"
#include "device_config.h"
#include "nrf24_driver.h"

typedef enum device_mode_e
{
  STANDBY_I,
  STANDBY_II,
  TX_MODE,
  RX_MODE
} device_mode_t;

/**
 * A global struct, which encapsulates pin_manager_t and spi_manager_t 
 * objects, which hold data relevant to the pin_manager, spi_manager 
 * utility functions. The nrf_manager_t holds register configuration
 * settings for the NRF24L01.
 */
typedef struct nrf_driver_s
{
  // GPIO pin numbers
  pin_manager_t user_pins;

  // SPI interface and baudrate
  spi_manager_t user_spi;

  // NRF register configuration
  nrf_manager_t user_config;

  // address width as number of bytes
  uint8_t address_width_bytes;

  // STANDBY_I, STANDBY_II, TX_MODE, RX_MODE
  device_mode_t mode;

  // RX_ADDR_P0 register cache flag
  bool is_rx_addr_p0;

  // RX_ADDR_P0 register value cache
  uint8_t rx_addr_p0[5];

} nrf_driver_t;


/**
 * global nrf_driver_t struct, initialised
 * with default values for user_spi and
 * user_config structs and for mode value
 */
nrf_driver_t nrf_driver = { 
  .user_spi.baudrate = 7000000,
  .user_spi.instance = spi0,
  .user_config.address_width = AW_5_BYTES,
  .user_config.dyn_payloads = DYNPD_DISABLE,
  .user_config.retr_delay = ARD_500US,
  .user_config.retr_count = ARC_10RT,
  .user_config.data_rate = RF_DR_1MBPS,
  .user_config.power = RF_PWR_0DBM,
  .user_config.channel = 110,
  .address_width_bytes = FIVE_BYTES,
  .is_rx_addr_p0 = false,
  .rx_addr_p0 = { 0x00, 0x00, 0x00, 0x00, 0x00 },
  .mode = STANDBY_I
};


/**
 * forward declaration of static utility functions, 
 * to permit a more readable function order
 */
static fn_status_t validate_config(nrf_manager_t *user_config);

static fn_status_t w_register(register_map_t reg, const void *buffer, size_t buffer_size);

static uint8_t r_register_byte(register_map_t reg);

static fn_status_irq_t check_status_irq(uint8_t *rx_p_no);

static void flush_tx_fifo(void);

static void flush_rx_fifo(void);


/***********************************
 *     Public Driver Functions     *
 ***********************************/


/**
 * Validate and configure GPIO pins and ascertain the 
 * correct SPI instance, if the pins are valid. Store 
 * GPIO and SPI details in the pin_manager_t user_pins
 * and spi_manager user_spi structs, within the global 
 * nrf_driver_t nrf_driver struct.
 * 
 * @note The function returns ERROR, if the pins were 
 * not valid, possibly due to one SPI pin using SPI 0 
 * interface and another using SPI 1.
 * 
 * @param user_pins pin_manager_t struct of pin numbers
 * @param baudrate_hz baudrate in Hz
 * 
 * @return PI_MNGR_OK (1), ERROR (0)
 */
fn_status_t nrf_driver_configure(pin_manager_t *user_pins, uint32_t baudrate_hz) {

  // validate GPIO pins: PIN_MNGR_OK (1) or ERROR (0)
  fn_status_t status = pin_manager_configure(
    user_pins->copi, 
    user_pins->cipo, 
    user_pins->sck, 
    user_pins->csn, 
    user_pins->ce
  );

  if (status == PIN_MNGR_OK)
  {
    pin_manager_t *pins = &(nrf_driver.user_pins);

    // store user_pins in global nrf_driver struct
    *pins = *user_pins;

    spi_instance_t instance_pattern[8] = { SPI_0, SPI_0, SPI_1, SPI_1, SPI_0, SPI_0, SPI_1, SPI_1 };

    spi_instance_t spi_instances[3] = {
     instance_pattern[(pins->cipo - CIPO_MIN) / 4],
     instance_pattern[(pins->copi - COPI_MIN) / 4],
     instance_pattern[(pins->sck - SCK_MIN) / 4],
    };

    spi_instance_t count[3] = { 0, 0 }; // SPI_0 (0), SPI_1 (1)

    // count occurrence algorithm for SPI instance
    for (uint8_t i = 0; i < ALL_PINS; i++)
    {
      count[spi_instances[i]]++;
    }

    // validate SPI instance count
    status = ((count[SPI_0] == 3) || (count[SPI_1] == 3)) ? PIN_MNGR_OK : ERROR;

    if (status == PIN_MNGR_OK)
    {
      spi_manager_t *spi = &(nrf_driver.user_spi);

      // store baudrate & SPI instance in global nrf_driver
      spi->baudrate = (baudrate_hz > 7500000) ? 7500000 : baudrate_hz;
      spi->instance = (count[SPI_0] == 3) ? spi0 : spi1;
    }
  }

  return status;
}


/**
 * Initialise NRF24L01 registers, leaving the device in 
 * Standby Mode.
 * 
 * WiFi uses most of the lower channels and so, the 
 * highest 25 channels (100 - 124) are recommended 
 * for nRF24L01 projects.
 * 
 * Default configuration:
 * 
 * RF Channel: 110
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
 * @param user_config channel
 * 
 * @return SPI_MNGR_OK (2), ERROR (0)
 */
fn_status_t nrf_driver_initialise(nrf_manager_t *user_config) {

  spi_manager_t *spi = &(nrf_driver.user_spi);

  // initialise SPI for function duration
  spi_manager_init_spi(spi->instance, spi->baudrate); 

  /** with a VDD of 1.9V or higher, nRF24L01+ enters the Power on reset state **/

  sleep_ms(100); // nRF24L01+ enters Power Down mode after 100ms

  /** NRF24L01 is now in Power Down mode. PWR_UP bit in the CONFIG register is unset (0) **/

  // CE to LOW in preperation for entering Standby-I mode
  ce_put_low(nrf_driver.user_pins.ce); 

  sleep_ms(1);

  // CSN high in preperation for writing to registers
  csn_put_high(nrf_driver.user_pins.csn); 

  nrf_manager_t *config = &(nrf_driver.user_config);

  fn_status_t status = ERROR;

  // if nrf_manager_t user_config !== NULL
  if (user_config != NULL)
  {
    status = validate_config(user_config);

    if (status == NRF_MNGR_OK)
    {
      // store user_config in global nrf_driver_t object
      *config = *user_config;

      nrf_driver.address_width_bytes = ((config->address_width + 2) <= FIVE_BYTES) ? config->address_width + 2 : FIVE_BYTES;
    }
  }

  if (status == NRF_MNGR_OK)
  {
    // register address and value to write
    typedef struct w_register_s { register_map_t reg; uint8_t buf[1]; } w_register_t;

    // array of register addresses and values
    w_register_t register_list[9] = {
      (w_register_t){ 
        .reg = CONFIG,
        .buf = { 0x0E } // set PWR_UP bit
      },
      (w_register_t){ 
        .reg = EN_AA, // enable auto-acknowledge
        .buf = { ENAA_ALL } // on all data pipes
      },
      (w_register_t){ 
        .reg = SETUP_AW, // set address width
        .buf = { config->address_width } 
      },
      (w_register_t){ 
        .reg = SETUP_RETR, // retransmission settings
        .buf = { config->retr_count | config->retr_delay } 
      },
      (w_register_t){ 
        .reg = RF_CH, // set RF channel
        .buf = { config->channel} 
      },
      (w_register_t){ 
        .reg = RF_SETUP, // RF data rate & TX power level  
        .buf = { config->data_rate | config->power } 
      },
      (w_register_t){ 
        .reg = FEATURE, // enable dynamic payloads in FEATURE register
        .buf = { SET_BIT << FEATURE_EN_DPL | SET_BIT << FEATURE_EN_DYN_ACK } 
      },
      (w_register_t){ 
        .reg = DYNPD, // dynamic payloads register
        .buf = { config->dyn_payloads } // DYNPD_ENABLE, DYNPD_DISABLE
      },
      (w_register_t){ 
        .reg = STATUS, 
        .buf = { STATUS_INTERRUPT_MASK } // clear STATUS interrupt bits
      },
    };
    
    // write the buffer to each register address
    for (size_t i = 0; i < 8; i++)
    {
      status = w_register(register_list[i].reg, register_list[i].buf, ONE_BYTE);

      // Crystal oscillator start up delay (Power Down to Standby-I state)
      if (register_list[i].reg == CONFIG) { sleep_ms(5); }

      if (status == ERROR) { break; } // break on error
    }

    // flush RX and TX FIFOs
    flush_tx_fifo();
    flush_rx_fifo();
  }

  // deinitialise SPI at function end
  spi_manager_deinit_spi(spi->instance); 

  return status;
}


/**
 * Set the destination address for a packet transmission, into the
 * TX_ADDR register.
 * 
 * The TX_ADDR register is used by a primary transmitter (PTX) for 
 * transmission of data packets and its value must be the same as 
 * an address in primary receiver (PRX) RX_ADDR_P0 - RX_ADDR_P5 
 * registers, for the PTX and PRX to communicate.
 * 
 * NOTE: For the auto-acknowledgement feature, the PTX must have its 
 * RX_ADDR_P0 register set with the same address as the TX_ADDR register. 
 * This driver uses auto-acknowledgement by default and this function 
 * writes the address to the the RX_ADDR_P0 by design.
 * 
 * @param address (uint8_t[]){0x37, 0x37, 0x37, 0x37, 0x37} etc.
 * 
 * @return SPI_MNGR_OK (2), ERROR (0)
 */
fn_status_t nrf_driver_tx_destination(const uint8_t *buffer) {

  spi_manager_t *spi = &(nrf_driver.user_spi);

  // initialise SPI for function duration
  spi_manager_init_spi(spi->instance, spi->baudrate);

  register_map_t registers[2] = { RX_ADDR_P0, TX_ADDR };

  // size_t buffer_size = ;

  fn_status_t status;

  for (size_t i = 0; i < 2; i++)
  {
    status = w_register(registers[i], buffer, nrf_driver.address_width_bytes);

    if (status == ERROR) { break; }
  }

  // deinitialise SPI at function end
  spi_manager_deinit_spi(spi->instance); 

  return status;
}


/**
 * This function sets the address in buffer to the specified data pipe. 
 * Addresses for DATA_PIPE_0 (0), DATA_PIPE_1 (1) and the remaining data 
 * pipes can be set with multiple set_rx_address calls. Data pipes 2 - 5 
 * use the 4 MSB of data pipe 1 address and should be set with 1 byte. 
 * 
 * NOTE: If you do use a 5 byte buffer for data pipes 2 - 5, then the 
 * function will only write one byte (buffer[0]).
 * 
 * @param address (uint8_t[]){0x37, 0x37, 0x37, 0x37, 0x37} etc.
 * 
 * @return SPI_MNGR_OK (2), ERROR (0)
 */
fn_status_t nrf_driver_rx_destination(data_pipe_t data_pipe, const uint8_t *buffer) {

  spi_manager_t *spi = &(nrf_driver.user_spi);

  spi_manager_init_spi(spi->instance, spi->baudrate);

  uint8_t registers[6] = {
    RX_ADDR_P0, RX_ADDR_P1, RX_ADDR_P2,
    RX_ADDR_P3, RX_ADDR_P4, RX_ADDR_P5
  };

  if (data_pipe == DATA_PIPE_0)
  {
    // set RX_ADDR_P0 cache flag
    nrf_driver.is_rx_addr_p0 = true;

    // cache RX_ADDR_P0 address
    memcpy(nrf_driver.rx_addr_p0, buffer, nrf_driver.address_width_bytes);
  }

  // will hold OK (0) or REGISTER_W_FAIL (3)
  fn_status_t status = ERROR;

  // if DATA_PIPE_0 - DATA_PIPE_5
  if (data_pipe < ALL_DATA_PIPES)
  {
    if (data_pipe < DATA_PIPE_2) // DATA_PIPE_0 & DATA_PIPE_1 hold full address width
    {
      status = w_register(registers[data_pipe], buffer, nrf_driver.address_width_bytes);

    } else {  // DATA_PIPE_2 - DATA_PIPE_5 hold 1 byte + 4 MSB of DATA_PIPE_1

      status = w_register(registers[data_pipe], buffer, ONE_BYTE);
    }

    // read value of EN_RXADDR register
    uint8_t en_rxaddr = r_register_byte(EN_RXADDR);

    // enable data pipe in EN_RXADDR register, if necessary
    if ((en_rxaddr >> data_pipe & SET_BIT) != SET_BIT)
    {
      en_rxaddr |= SET_BIT << data_pipe;
      status = w_register(EN_RXADDR, &en_rxaddr, ONE_BYTE);
    }
  }

  spi_manager_deinit_spi(spi->instance);

  return status;
}


/**
 * Set number of bytes in Tx payload for an individual data pipe or
 * for all data pipes, in the RX_PW_P0 - RX_PW_P5 registers.
 * 
 * data_pipe_t values:
 * 
 * DATA_PIPE_0...DATA_PIPE_5, ALL_DATA_PIPES (6)
 * 
 * @param data_pipe DATA_PIPE_0...DATA_PIPE_5 or ALL_DATA_PIPES
 * @param size bytes in payload
 * 
 * @return SPI_MNGR_OK (2), ERROR (0)
 */
fn_status_t nrf_driver_payload_size(data_pipe_t data_pipe, size_t size) {

  spi_manager_t *spi = &(nrf_driver.user_spi);

  // initialise SPI for function duration
  spi_manager_init_spi(spi->instance, spi->baudrate); 

  fn_status_t status = ((size > ZERO_BYTES) && (size <= MAX_BYTES)) ? NRF_MNGR_OK : ERROR;

  if (status == NRF_MNGR_OK)
  {
    // cache payload_width value
    // nrf_driver.payload_width = size;

    register_map_t rx_pw_registers[6] = { 
      RX_PW_P0, RX_PW_P1, RX_PW_P2, 
      RX_PW_P3, RX_PW_P4, RX_PW_P5 
    };

    if (data_pipe == ALL_DATA_PIPES)
    {
      for (size_t i = 0; i < ALL_DATA_PIPES; i++)
      {
        status = w_register(rx_pw_registers[i], &size, ONE_BYTE);
        if (!status) { break; }
      }
    } 
    else 
    {
      status = w_register(rx_pw_registers[data_pipe], &size, ONE_BYTE);
    }
  }

  // deinitialise SPI at function end
  spi_manager_deinit_spi(spi->instance); 

  return status;
}


/**
 * Enables dynamic payloads, if not already disabled.
 * 
 * @return SPI_MNGR_OK (2), ERROR (0)
 */
fn_status_t nrf_driver_dyn_payloads_enable(void) {

  spi_manager_t *spi = &(nrf_driver.user_spi);

  spi_manager_init_spi(spi->instance, spi->baudrate);
  
  nrf_manager_t *config = &(nrf_driver.user_config);

  fn_status_t status = SPI_MNGR_OK;

  if (config->dyn_payloads == DYNPD_DISABLE)
  {
    uint8_t feature = r_register_byte(FEATURE);

    feature |= SET_BIT << FEATURE_EN_DPL;

    status = w_register(FEATURE, &feature, ONE_BYTE);

    config->dyn_payloads = (status) ? DYNPD_ENABLE : DYNPD_DISABLE;

    if (status == SPI_MNGR_OK)
    {
      status = w_register(DYNPD, (uint8_t[]){DYNPD_ENABLE}, ONE_BYTE);
    }
  }

  spi_manager_deinit_spi(spi->instance);

  return status;
}


/**
 * Disables dynamic payloads, if not already disabled.
 * 
 * @return SPI_MNGR_OK (2), NRF_MNGR_OK (3), ERROR (0)
 */
fn_status_t nrf_driver_dyn_payloads_disable(void) {

  spi_manager_t *spi = &(nrf_driver.user_spi);

  // initialise SPI for function duration
  spi_manager_init_spi(spi->instance, spi->baudrate);
  
  nrf_manager_t *config = &(nrf_driver.user_config);

  fn_status_t status = NRF_MNGR_OK;

  if (config->dyn_payloads == DYNPD_ENABLE)
  {
    uint8_t feature = r_register_byte(FEATURE);

    // clear EN_DPL (bit 2) in FEATURE register
    feature &= ~(SET_BIT << FEATURE_EN_DPL);

    status = w_register(FEATURE, &feature, ONE_BYTE);

    config->dyn_payloads = (status) ? DYNPD_DISABLE : DYNPD_ENABLE;

    if (status == SPI_MNGR_OK)
    {
      status = w_register(DYNPD, (uint8_t[]){DYNPD_DISABLE}, ONE_BYTE);
    }
  }

  // deinitialise SPI at function end
  spi_manager_deinit_spi(spi->instance);

  return status;
}

/**
 * Set the RF channel. Each device must be on the same 
 * channel in order to communicate.
 * 
 * @param channel RF channel 1..125
 * 
 * @return SPI_MNGR_OK (2), ERROR (0)
 */
fn_status_t nrf_driver_rf_channel(uint8_t channel) {

  fn_status_t status = ((channel >= 2) && (channel <= 125)) ? NRF_MNGR_OK : ERROR;

  if (status == NRF_MNGR_OK)
  {
    // write channel number to RF_CH register
    status = w_register(RF_CH, &channel, ONE_BYTE);

    // allows less verbose access to nrf_driver.user_config.channel
    nrf_manager_t *user_config = &(nrf_driver.user_config);

    // store channel configuration in global nrf_driver_t
    user_config->channel = (status == SPI_MNGR_OK) ? channel : user_config->channel;
  }
  
  return status;
}


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
 * @param delay ARD_250US, ARD_500US, ARD_750US, ARD_1000US
 * @param count ARC_NONE, ARC_1RT...ARC_15RT
 * 
 * @return SPI_MNGR_OK (2), ERROR (0)
 */
fn_status_t nrf_driver_auto_retransmission(retr_delay_t delay, retr_count_t count) {

  spi_manager_t *spi = &(nrf_driver.user_spi);

  spi_manager_init_spi(spi->instance, spi->baudrate); // initialise SPI for function duration

  uint8_t valid_params = 0;

  // validate retransmission count
  if (count <= ARC_15RT)
  {
    valid_params++;
  }

  // validate retransmission delay
  for (size_t i = 0; i < 48; i += 16)
  {
    if (delay == i) { valid_params++; }
  }

  fn_status_t status = (valid_params == 2) ? NRF_MNGR_OK : ERROR;

  if (status == NRF_MNGR_OK)
  {
    // write specified ARD and ARC settings to SETUP_RETR register
    status  = w_register(SETUP_RETR, (uint8_t*)(delay | count), ONE_BYTE);
  }

  spi_manager_deinit_spi(spi->instance); // deinitialise SPI at function end

  return status;
}


/**
 * Set the RF data rates in the RF_SETUP register
 * through the RF_DR_LOW and RF_DR_HIGH bits.
 * 
 * @param data_rate RF_DR_1MBPS, RF_DR_2MBPS, RF_DR_250KBPS
 * 
 * @return SPI_MNGR_OK (2), ERROR (0)
 */
fn_status_t nrf_driver_rf_data_rate(rf_data_rate_t data_rate) {

  spi_manager_t *spi = &(nrf_driver.user_spi);

  // initialise SPI for function duration
  spi_manager_init_spi(spi->instance, spi->baudrate);

  fn_status_t status = NRF_MNGR_OK;

  // validate RF data rate
  switch (data_rate)
  {
    case RF_DR_1MBPS:
    break;

    case RF_DR_2MBPS:
    break;

    case RF_DR_250KBPS:
    break;
    
    default:
      status = ERROR;
    break;
  }

  if (status == NRF_MNGR_OK)
  {
    // Value of RF_SETUP register
    uint8_t rf_setup = r_register_byte(RF_SETUP);

    rf_setup = (rf_setup & RF_SETUP_RF_PWR_MASK) | (data_rate & RF_SETUP_RF_DR_MASK);

    status = w_register(RF_SETUP, &rf_setup, ONE_BYTE);

    // deinitialise SPI at function end
    spi_manager_deinit_spi(spi->instance); 
  }

  return status;
}


/**
 * Set TX Mode power level in RF_SETUP register through
 * the RF_PWR bits.
 * 
 * @param rf_pwr RF_PWR_NEG_18DBM, RF_PWR_NEG_12DBM, RF_PWR_NEG_6DBM, RF_PWR_0DBM
 * 
 * @return SPI_MNGR_OK (2), ERROR (0)
 */
fn_status_t nrf_driver_rf_power(rf_power_t rf_pwr) {

  spi_manager_t *spi = &(nrf_driver.user_spi);

  // initialise SPI for function duration
  spi_manager_init_spi(spi->instance, spi->baudrate);

  fn_status_t status = ERROR;

  // validate RF power setting
  for (size_t i = 0; i < 6; i += 2)
  {
    if (rf_pwr == i) { status = NRF_MNGR_OK; break; }
  }

  if (status == NRF_MNGR_OK)
  {
    // Read RF_SETUP register value
    uint8_t rf_setup = r_register_byte(RF_SETUP);

    rf_setup = (rf_setup & RF_SETUP_RF_DR_MASK) | (rf_pwr & RF_SETUP_RF_PWR_MASK);

    // holds OK (0) or REGISTER_W_FAIL (3)
    status = w_register(RF_SETUP, &rf_setup, ONE_BYTE);
  }

  // deinitialise SPI at function end
  spi_manager_deinit_spi(spi->instance); 

  return status;
}


/**
 * Puts the NRF24L01 into Standby-I Mode. Resets the CONFIG 
 * register PRIM_RX bit value, in preparation for entering
 * TX Mode and drives CE pin LOW.
 * 
 * NOTE: State diagram in the datasheet (6.1.1) highlights
 * conditions for entering RX and TX operating modes. One 
 * condition is the value of PRIM_RX (bit 0) in the CONFIG
 * register. PRIM_RX = 1 for RX Mode or 0 for TX Mode. CE
 * pin is driven HIGH for Rx mode and is only driven high 
 * in TX Mode to facilitate the Tx of data (10us+).
 * 
 * @return SPI_MNGR_OK (2), ERROR (0)
 */
fn_status_t nrf_driver_standby_mode(void) {

  // initialise SPI at function start
  spi_manager_t *spi = &(nrf_driver.user_spi);

  spi_manager_init_spi(spi->instance, spi->baudrate);

  fn_status_t status = NRF_MNGR_OK;

  if (nrf_driver.mode == RX_MODE)
  {
    // read CONFIG register value
    uint8_t config = r_register_byte(CONFIG);

    config &= ~(SET_BIT << CONFIG_PRIM_RX);

    w_register(CONFIG, &config, ONE_BYTE);

    // Drive CE LOW
    ce_put_low(nrf_driver.user_pins.ce);

    // NRF24L01+ enters Standby-I mode after 130μS
    sleep_us(130);

    nrf_driver.mode = STANDBY_I;
  }

  // deinitialise SPI at function end
  spi_manager_deinit_spi(spi->instance);

  return status;
}


/**
 * Transmits a payload to a recipient NRF24L01 and will return 
 * SPI_MNGR_OK (2) if the transmission was successful and an 
 * auto-acknowledgement was received from the recipient NRF24L01. 
 * A return value of ERROR (0) indicates that either, the packet 
 * transmission failed, or no auto-acknowledgement was received 
 * before max retransmissions count was reached.
 * 
 * @param tx_packet packet for transmission
 * @param size size of tx_packet
 * 
 * @return SPI_MNGR_OK (2), ERROR (0)
 */
fn_status_t nrf_driver_send_packet(const void *tx_packet, size_t size) {

  if (nrf_driver.mode == RX_MODE) { 
    nrf_driver_standby_mode(); 
    nrf_driver.mode = STANDBY_I;
  }

  spi_manager_t *spi = &(nrf_driver.user_spi);

  spi_manager_init_spi(spi->instance, spi->baudrate);
  
  // fn_status_t status = (size <= nrf_driver.payload_width) ? NRF_MNGR_OK : ERROR;

  // cast void *tx_packet to uint8_t pointer
  const uint8_t *tx_packet_ptr = (uint8_t *)tx_packet;

  // W_TX_PAYLOAD command + packet_size
  size_t total_size = size + 1;

  uint8_t tx_buffer[total_size]; // SPI transfer TX buffer
  uint8_t rx_buffer[total_size]; // SPI transfer RX buffer

  // put W_TX_PAYLOAD command into first index of tx_buffer
  tx_buffer[0] = W_TX_PAYLOAD;

  // store byte(s) in tx_packet (tx_packet_ptr) into tx_buffer[]
  for (size_t i = 1; i < total_size; i++)
  { 
    tx_buffer[i] = tx_packet_ptr[i - 1]; 
  }

  ce_put_high(nrf_driver.user_pins.ce);

  csn_put_low(nrf_driver.user_pins.csn); // drive CSN pin LOW
  fn_status_t status = spi_manager_transfer(spi->instance, tx_buffer, rx_buffer, total_size);
  csn_put_high(nrf_driver.user_pins.csn); // drive CSN pin HIGH

  nrf_driver.mode = TX_MODE;

  // pulse CE high for 10us to transmit
  sleep_us(15); 

  ce_put_low(nrf_driver.user_pins.ce);

  nrf_driver.mode = STANDBY_I;

  fn_status_irq_t status_irq = check_status_irq(NULL);

  /**
   * if spi_manager_transfer returns SPI_MNGR_OK, then poll STATUS register, checking 
   * TX_DS (auto-acknowledgement received) and MAX_RT (max retransmissions) bits in 
   * the STATUS register. If neither bits are set (NONE_ASSERTED), keep polling.
   */
  while ((status == SPI_MNGR_OK) && (status_irq == NONE_ASSERTED))
  {
     status_irq = check_status_irq(NULL);
  }
  
  spi_manager_deinit_spi(spi->instance);

  status = (status_irq == TX_DS_ASSERTED) ? NRF_MNGR_OK : ERROR;

  /**
   * returns NRF_MNGR_OK (3) if spi_manager_transfer returned SPI_MNGR_OK (2) and 
   * check_status_irq returned TX_DS_ASSERTED (2). Else ERROR (0), which indicates 
   * either an error in the SPI transfer of the packet or that auto-acknowledgment 
   * was not received after the packet was retransmitted the max number of times, 
   * indicated by the STATUS register MAX_RT bit being asserted (1).
   */
  return status;
}


/**
 * Read an available packet from the RX FIFO into the 
 * buffer (rx_packet).
 * 
 * @param rx_packet packet buffer for receipt
 * @param size size of buffer
 * 
 * @return SPI_MNGR_OK (2), ERROR (0)
 */
fn_status_t nrf_driver_read_packet(void *rx_packet, size_t size) {

  spi_manager_t *spi = &(nrf_driver.user_spi);

  // initialise SPI at function start
  spi_manager_init_spi(spi->instance, spi->baudrate);

  fn_status_t status = SPI_MNGR_OK;

  /**
   * if dynamic payloads are enabled, read the payload width
   * via the R_RX_PL_WID command and check it is not greater
   * than 32 (max payload size). If it is, the packet is 
   * corrupted and RX FIFO is flushed. 
   */
  if (nrf_driver.user_config.dyn_payloads)
  {
    uint8_t tx_buffer[2] = { R_RX_PL_WID, NOP };
    uint8_t rx_buffer[2];

    csn_put_low(nrf_driver.user_pins.csn); // drive CSN pin LOW
    status = spi_manager_transfer(spi->instance, tx_buffer, rx_buffer, TWO_BYTES);
    csn_put_high(nrf_driver.user_pins.csn); // drive CSN pin HIGH

    // rx_buffer[0] holds STatus REGISTER value
    if (rx_buffer[1] > MAX_BYTES) 
    {
      flush_rx_fifo();
      status = ERROR;
    }
  }

  if (status)
  {
    // cast void *rx_packet to uint8_t pointer
    uint8_t *rx_packet_ptr = (uint8_t *)rx_packet;

    // W_TX_PAYLOAD command + packet_size
    size_t total_size = size + 1;

    uint8_t tx_buffer[total_size]; // SPI transfer TX buffer
    uint8_t rx_buffer[total_size]; // SPI transfer RX buffer

    // put R_RX_PAYLOAD command into first index of tx_buffer
    tx_buffer[0] = R_RX_PAYLOAD;

    // store byte(s) in tx_packet (tx_packet_ptr) into tx_buffer[]
    for (size_t i = 1; i < total_size; i++) { *(tx_buffer + i) = NOP; }
    
    csn_put_low(nrf_driver.user_pins.csn); // drive CSN pin LOW
    status = spi_manager_transfer(spi->instance, tx_buffer, rx_buffer, total_size);
    csn_put_high(nrf_driver.user_pins.csn); // drive CSN pin HIGH
    
    // skip rx_buffer[0] (STATUS value) and transfer remaining values to buffer
    for (size_t i = 0; i < size; i++)
    { 
      *(rx_packet_ptr + i) = rx_buffer[i + 1]; 
    }
  }

  // deinitialise SPI at function end
  spi_manager_deinit_spi(spi->instance);

  return status;
}


/**
 * Polls the STATUS register to ascertain if there is a packet 
 * in the RX FIFO. The function will return PASS (1) if there 
 * is a packet available to read, or FAIL (0) if not. 
 * 
 * @return NRF_MNGR_OK (3), ERROR (0)
 */
fn_status_t nrf_driver_is_packet(uint8_t *rx_p_no) {

  spi_manager_t *spi = &(nrf_driver.user_spi);

  spi_manager_init_spi(spi->instance, spi->baudrate);

  /**
   * check_status_irq function checks if uint8_t *rx_p_no
   * argument is NULL. If not, the data pipe number the 
   * packet was received on will be passed to rx_p_no
   */

  // NONE_ASSERTED (0), RX_DR_ASSERTED (1), TX_DS_ASSERTED (2), MAX_RT_ASSERTED (3)
  fn_status_t status = (check_status_irq(rx_p_no) == RX_DR_ASSERTED) ? NRF_MNGR_OK : ERROR;

  spi_manager_deinit_spi(spi->instance);

  return status;
}


/**
 * Puts the NRF24L01 into RX Mode. Sets the CONFIG register 
 * PRIM_RX bit value and drive CE pin HIGH.
 * 
 * NOTE: State diagram in the datasheet (6.1.1) highlights
 * conditions for entering Rx and Tx operating modes. One 
 * condition is the value of PRIM_RX (bit 0) in the CONFIG
 * register. PRIM_RX = 1 for Rx mode or 0 for Tx mode. CE
 * pin is driven HIGH for Rx mode and is only driven high 
 * in Tx mode to facilitate the Tx of data (10us+).
 * 
 * @return SPI_MNGR_OK (2), ERROR (0)
 */
fn_status_t nrf_driver_receiver_mode(void) {
  
  spi_manager_t *spi = &(nrf_driver.user_spi);

  spi_manager_init_spi(spi->instance, spi->baudrate);

  // read CONFIG register value
  uint8_t config = r_register_byte(CONFIG);

  // is CONFIG register PRIM_RX bit set?
  uint8_t prim_rx = (config >> CONFIG_PRIM_RX) & 1;

  fn_status_t status = NRF_MNGR_OK;

  // PRIM_RX bit should be set for RX Mode
  if (prim_rx != SET_BIT)
  {
    // set PRIM_RX bit in CONFIG register
    config |= (SET_BIT << CONFIG_PRIM_RX);

    // write set bit to CONFIG register
    status = w_register(CONFIG, &config, ONE_BYTE);
  }

  // restore the RX_ADDR_P0 address, if exists
  if (nrf_driver.is_rx_addr_p0)
  {
    w_register(RX_ADDR_P0, nrf_driver.rx_addr_p0, nrf_driver.address_width_bytes);
  }

  // Drive CE HIGH
  ce_put_high(nrf_driver.user_pins.ce);

  // NRF24L01+ enters RX Mode after 130μS
  sleep_us(130);

  nrf_driver.mode = RX_MODE; // reflect RX Mode in nrf_status

  spi_manager_deinit_spi(spi->instance);

  return status;
}


/**
 * Assign public nrf_driver function addresses to
 * the coressponding nrf_client_t function pointers,
 * providing access to these functions through the 
 * nrf_client_t object.
 * 
 * @param client nrf_client_t struct
 * 
 * @return NRF_MNGR_OK (3), ERROR (0)
 */
fn_status_t nrf_driver_create_client(nrf_client_t *client) {

  client->configure = nrf_driver_configure;
  client->initialise = nrf_driver_initialise;

  client->rx_destination = nrf_driver_rx_destination;
  client->tx_destination = nrf_driver_tx_destination;

  client->payload_size = nrf_driver_payload_size;
  client->dyn_payloads_enable = nrf_driver_dyn_payloads_enable;
  client->dyn_payloads_disable = nrf_driver_dyn_payloads_disable;

  client->auto_retransmission = nrf_driver_auto_retransmission;

  client->rf_channel = nrf_driver_rf_channel;
  client->rf_data_rate = nrf_driver_rf_data_rate;
  client->rf_power = nrf_driver_rf_power;

  client->send_packet = nrf_driver_send_packet;
  client->read_packet = nrf_driver_read_packet;
  client->is_packet = nrf_driver_is_packet;

  client->standby_mode = nrf_driver_standby_mode;
  client->receiver_mode = nrf_driver_receiver_mode;

  return NRF_MNGR_OK;
}


/************************************
 *     Static Utility Functions     *
 ************************************/


/**
 * Validates user nrf_manager_t configuration.
 * 
 * @param reg register to write the buffer to
 * @param buffer value to be held in the register
 * @param size size of the buffer
 * 
 * @return NRF_MNGR_OK (3), ERROR (0)
 */
static fn_status_t validate_config(nrf_manager_t *user_config) {

  uint8_t valid_members = 0;

  // validate address width
  if ((user_config->address_width) >= AW_3_BYTES && (user_config->address_width <= AW_5_BYTES))
  {
    valid_members++;
  }

  // validate RF Channel
  if ((user_config->channel >= 2) && (user_config->channel <= 125))
  {
    valid_members++;
  }

  // validate RF data rate
  switch (user_config->data_rate)
  {
    case RF_DR_1MBPS:
      valid_members++;
    break;

    case RF_DR_2MBPS:
      valid_members++;
    break;

    case RF_DR_250KBPS:
      valid_members++;
    break;
    
    default:
    break;
  }

  // validate dynamic payloads setting
  if ((user_config->dyn_payloads == DYNPD_ENABLE) || (user_config->dyn_payloads == DYNPD_DISABLE))
  {
    valid_members++;
  }

  // validate RF power setting
  // validate RF power setting
  for (size_t rf_pow = RF_PWR_NEG_18DBM; rf_pow <= RF_PWR_0DBM; rf_pow += 2)
  {
    if (user_config->power == rf_pow) { valid_members++; }
  }

  // validate retransmission count
  if (user_config->retr_count <= ARC_15RT)
  {
    valid_members++;
  }

  // validate retransmission delay
  for (size_t delay = ARD_250US; delay <= ARD_1000US; delay += 16)
  {
    if (user_config->retr_delay == delay) { valid_members++; }
  }

  fn_status_t status = (valid_members == 7) ? NRF_MNGR_OK : ERROR;

  return status;
} 



/**
 * Writes the buffer value to the specified register.
 * 
 * @param reg register to write the buffer to
 * @param buffer value to be held in the register
 * @param size size of the buffer
 * 
 * @return SPI_MNGR_OK (2), ERROR (0)
 */
static fn_status_t w_register(register_map_t reg, const void *buffer, size_t size) {

  uint8_t *buffer_ptr = (uint8_t *)buffer;

  // buffer_size + 1 (register address)
  size_t total_size = size + 1;

  uint8_t tx_buffer[total_size]; // SPI Tx buffer
  uint8_t rx_buffer[total_size]; // SPI Rx buffer

  // ensure 3 MSB are [001] (write to the register)
  reg = ((REGISTER_MASK & reg) | W_REGISTER);

  tx_buffer[0] = reg; // store register in tx_buffer[0]

  // store byte(s) in buffer (buffer_ptr) into tx_buffer[total_size]
  for (size_t i = 0; i < size; i++) { tx_buffer[i + 1] = *(buffer_ptr + i); }

  spi_manager_t *spi = &(nrf_driver.user_spi);
  
  csn_put_low(nrf_driver.user_pins.csn); // drive CSN pin LOW
  fn_status_t status = spi_manager_transfer(spi->instance, tx_buffer, rx_buffer, total_size);
  csn_put_high(nrf_driver.user_pins.csn); // drive CSN pin HIGH

  return status; // return error flag value
}


/**
 * Reads one byte from the specified register.
 *  
 * @param reg register address
 * 
 * @return register value 
 */
static uint8_t r_register_byte(register_map_t reg) {

  /**
   * NRF24L01 returns the STATUS register value,
   * hence why tx_buffer and rx_buffer are two
   * bytes. The first index of rx_buffer will
   * hold the STATUS register value. The second
   * index will hold the value of the specified
   * register.
   */
  uint8_t tx_buffer[TWO_BYTES] = { reg, NOP };
  uint8_t rx_buffer[TWO_BYTES] = { 0, 0 };

  spi_manager_t *spi = &(nrf_driver.user_spi);

  csn_put_low(nrf_driver.user_pins.csn); // drive CSN pin LOW
  spi_manager_transfer(spi->instance, tx_buffer, rx_buffer, TWO_BYTES);
  csn_put_high(nrf_driver.user_pins.csn); // drive CSN pin HIGH

  // *(rx_buffer + 0) holds STATUS register value
  return *(rx_buffer + 1);
}


/**
 * Reads a number of bytes from the specified register.
 * 
 * NOTE: Works the same as r_register_byte function. 
 * Some registers hold more than one byte, such as
 * RX_ADDR_P0 - RX_ADDR_P5 registers (5 bytes). An
 * array buffer is used to store the register value.
 * 
 * @param reg register address
 * @param buffer buffer for register value
 * @param buffer_size size of buffer
 * 
 * @return SPI_MNGR_OK (2), ERROR (0)
 */
static fn_status_t r_register_bytes(register_map_t reg, uint8_t *buffer, size_t buffer_size) {

  /**
   * NRF24L01 returns the STATUS register value, hence why tx_buffer 
   * and rx_buffer are the size of buffer_size + 1. The first index 
   * of rx_buffer will hold the STATUS register  value. The remainder 
   * will hold the value of the specified register.
   */
  size_t total_size = buffer_size + 1;

  uint8_t tx_buffer[total_size];
  uint8_t rx_buffer[total_size];

  // store register address in tx_buffer[0]
  *(tx_buffer + 0) = reg;

  // fill rest of tx_buffer with NOP
  for (size_t i = 1; i < total_size; i++) { *(tx_buffer + i) = NOP; }

  spi_manager_t *spi = &(nrf_driver.user_spi);

  csn_put_low(nrf_driver.user_pins.csn); // drive CSN pin LOW
  fn_status_t status = spi_manager_transfer(spi->instance, tx_buffer, rx_buffer, total_size);
  csn_put_high(nrf_driver.user_pins.csn); // drive CSN pin HIGH
  
  // skip rx_buffer[0] (STATUS value) and transfer remaining values to buffer
  for (size_t i = 1; i < total_size; i++) { *(buffer++) = *(rx_buffer + i); }

  return status;
}





/**
 * Set the specified address width in the SETUP_AW register. By default,
 * the driver uses an address width of 5 bytes.
 * 
 * @param address_width AW_3_BYTES, AW_4_BYTES, AW_5_BYTES
 */
/*
static fn_status_t set_address_width(address_width_t address_width) {

  spi_manager_t *spi = &(nrf_driver.user_spi);

  // initialise SPI for function duration
  spi_manager_init_spi(spi->instance, spi->baudrate); 

  // holds OK (0) or REGISTER_W_FAIL (3)
  fn_status_t status = w_register(SETUP_AW, &address_width, ONE_BYTE);

  nrf_manager_t *config = &(nrf_driver.user_config);

  if (status == NRF_MNGR_OK)
  {
    config->address_width = address_width;
  }

  // deinitialise SPI at function end
  spi_manager_deinit_spi(spi->instance); 

  return status;
}
*/

/**
 * Enables Enhanced ShockBurst™ auto acknowledgment (AA) 
 * function on the specified data pipe(s).
 * 
 * @param setting ENAA_NONE, ENAA_P0, ENAA_P1, ENAA_P2, ENAA_P3, ENAA_P4, ENAA_P5, ENAA_ALL
 * 
 * @return SPI_MNGR_OK (2), ERROR (0)
 */
/*
static fn_status_t enable_auto_acknowledge(en_auto_ack_t setting) {

  spi_manager_t *spi = &(nrf_driver.user_spi);

  // initialise SPI for function duration
  spi_manager_init_spi(spi->instance, spi->baudrate); 

  fn_status_t status = w_register(EN_AA, &setting, ONE_BYTE);

  // deinitialise SPI at function end
  spi_manager_deinit_spi(spi->instance); 

  return status;
}
*/


/**
 * Writes FLUSH_TX instruction over SPI to NRF24L01,
 * which will flush the TX FIFO.
 */
static void flush_tx_fifo(void) {
  
  csn_put_low(nrf_driver.user_pins.csn);
  sleep_us(2);
  spi_write_blocking(nrf_driver.user_spi.instance, (uint8_t[]){FLUSH_TX}, ONE_BYTE);
  sleep_us(2);
  csn_put_high(nrf_driver.user_pins.csn);

  return;
}


/**
 * Writes FLUSH_RX instruction over SPI to NRF24L01,
 * which will flush the RX FIFO.
 */
static void flush_rx_fifo(void) {
  
  csn_put_low(nrf_driver.user_pins.csn);
  sleep_us(2);
  spi_write_blocking(nrf_driver.user_spi.instance, (uint8_t[]){FLUSH_RX}, ONE_BYTE);
  sleep_us(2);
  csn_put_high(nrf_driver.user_pins.csn);

  return;
}


/**
 * Check STATUS register RX_DR, TX_DS and MAX_RT bits and 
 * return a internal_status_irq_t value, indicating which 
 * bit is asserted.
 * 
 * @return NONE_ASSERTED (0), RX_DR_ASSERTED (1), 
 * TX_DS_ASSERTED (2), MAX_RT_ASSERTED (3)
 */
static fn_status_irq_t check_status_irq(uint8_t *rx_p_no) {

  // value of STATUS register
  uint8_t status = r_register_byte(STATUS);

  // test which interrupt was asserted
  uint8_t rx_dr = (status >> STATUS_RX_DR) & SET_BIT; // Asserted when packet received
  uint8_t tx_ds = (status >> STATUS_TX_DS) & SET_BIT; // Asserted when auto-acknowledge received
  uint8_t max_rt = (status >> STATUS_MAX_RT) & SET_BIT; // Asserted when max retries reached

  // NONE_ASSERTED (0), RX_DR_ASSERTED (1), TX_DS_ASSERTED (2), MAX_RT_ASSERTED (3)
  fn_status_irq_t asserted_bit = NONE_ASSERTED;

  // used to write 1 to asserted bit to reset
  uint8_t reset_bit = 0; 

  if (rx_dr)
  { 
    if (rx_p_no != NULL) { *rx_p_no = (status >> STATUS_RX_P_NO) & STATUS_RX_P_NO_MASK; }

    reset_bit = (SET_BIT << STATUS_RX_DR);
    // reset RX_DR (bit 6) in STATUS register by writing 1
    w_register(STATUS, &reset_bit, ONE_BYTE);
    // indicate RX_DR bit asserted in STATUS register
    asserted_bit = RX_DR_ASSERTED;
  }

  if (tx_ds)
  {
    reset_bit = (SET_BIT << STATUS_TX_DS);
    // reset TX_DS (bit 5) in STATUS register by writing 1
    w_register(STATUS, &reset_bit, ONE_BYTE);
    // indicate TX_DS bit asserted in STATUS register
    asserted_bit = TX_DS_ASSERTED; 
  }

  if (max_rt)
  {
    reset_bit = (SET_BIT << STATUS_MAX_RT);
    // reset MAX_RT (bit 4) in STATUS register by writing 1
    w_register(STATUS, &reset_bit, ONE_BYTE);
    // indicate MAX_RT bit asserted in STATUS register
    asserted_bit = MAX_RT_ASSERTED;

    flush_tx_fifo();
  }

  return asserted_bit;
}
