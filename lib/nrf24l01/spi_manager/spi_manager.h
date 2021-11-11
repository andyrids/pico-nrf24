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
 * @file spi_manager.h
 * 
 * @brief enumerations, type definitions and function declarations for
 * communicating with the NRF24L01 over SPI.
 */

#ifndef SPI_MANAGER_H
#define SPI_MANAGER_H   

#include "error_manager.h"
#include "hardware/spi.h"

// corresponding SPI instance (SPI_0, SPI_1) or error (SPI_ERROR), when checking GPIO pins
typedef enum spi_instance_e { SPI_0, SPI_1, INSTANCE_ERROR } spi_instance_t;

// Available GPIO pins for CIPO, CSN, SCK & COPI on Pico SPI0 SPI interface
typedef enum spi0_pins_e {
  CIPO_GP0, CSN_GP1, SCK_GP2, COPI_GP3, CIPO_GP4, CSN_GP5, SCK_GP6, COPI_GP7, CIPO_GP16 = 16,
  CSN_GP17, SCK_GP18, COPI_GP19, CIPO_GP20, CSN_GP21, SCK_GP22, COPI_GP23 
} spi0_pins_t;

// Available GPIO pins for CIPO, CSN, SCK & COPI on Pico SPI1 SPI interface
typedef enum spi1_pins_e {
  CIPO_GP8 = 8, CSN_GP9, SCK_GP10, COPI_GP11, CIPO_GP12, CSN_GP13, 
  SCK_GP14, COPI_GP15, CIPO_GP24 = 24, CSN_GP25, SCK_GP26, COPI_GP27
} spi1_pins_t;

// represents SPI pins and SPI pin count (ALL_PINS)
typedef enum spi_pins_e { CIPO, COPI, SCK, ALL_PINS } spi_pins_t;

/**
 * For SPI GPIO pins, whether for spi0 or spi1 SPI interface, the valid pins
 * are offset by 4, from the lowest valid GPIO pin to the highest GPIO pin.
 * 
 * i.e. CIPO_GP0, CIPO_GP4, CIPO_GP8, CIPO_GP12, CIPO_GP16, CIPO_GP20 & CIPO_GP24
 * 
 * by starting at the lowest available pin value for a specific SPI pin and incrementing
 * by 4, stoping at the highest valid pin value, it is possible to ascertain if the pin
 * is even a valid SPI pin of the specified type, before checking which SPI instance it 
 * is associated with.
 */
typedef enum spi_min_range_e { CIPO_MIN = 0, CSN_MIN, SCK_MIN, COPI_MIN } spi_min_range_t; // lowest available SPI GPIO numbers
typedef enum spi_max_range_e { SCK_MAX = 26, COPI_MAX, CIPO_MAX, CSN_MAX } spi_max_range_t; // highest available SPI GPIO numbers


/**
 * Initialise the SPI interface for read/write operations
 * to the NRF24L01 device over SPI.
 */
void spi_manager_init_spi(spi_inst_t *instance, uint32_t baudrate);


/**
 * Deinitialise the SPI interface for read/write operations
 * to the NRF24L01 device over SPI.
 */
void spi_manager_deinit_spi(spi_inst_t *instance);


/**
 * Performs a simultaneous red/write to the NRF24L01 over
 * SPI.
 * 
 * @param tx_buffer write buffer
 * @param rx_buffer read buffer
 * @param num_bytes bytes in buffers
 * 
 * @return OK (0), SPI_FAIL (7)
 */
fn_status_t spi_manager_transfer(spi_inst_t *instance, const uint8_t *tx_buffer, uint8_t *rx_buffer, size_t len);

#endif // SPI_MANAGER_H