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


// corresponding SPI instance (SPI_0, SPI_1) when checking GPIO pins
typedef enum spi_instance_e { SPI_0, SPI_1 } spi_instance_t;


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
 * @param instance SPI instance pointer
 * @param tx_buffer write buffer
 * @param rx_buffer read buffer
 * @param num_bytes bytes in buffers
 * 
 * @return SPI_MNGR_OK (2), ERROR (0)
 */
fn_status_t spi_manager_transfer(spi_inst_t *instance, const uint8_t *tx_buffer, uint8_t *rx_buffer, size_t len);

#endif // SPI_MANAGER_H