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
 * @file error_manager.h
 * 
 * @brief contains enumerations and type definitions for static 
 * utility functions and public driver interface functions' return 
 * values.
 */

#ifndef ERROR_MANAGER_H
#define ERROR_MANAGER_H

// return values for static (private) functions
typedef enum internal_status_e
{
  OK, // no error
  PARAMETER_FAIL, // paramater error
  REGISTER_R_FAIL, // read register error
  REGISTER_W_FAIL, // write register error
  TX_MODE_FAIL, // TX Mode error
  RX_MODE_FAIL, // RX Mode error
  RX_FIFO_EMPTY, // RX FIFO empty
  SPI_FAIL // SPI transfer error
} internal_status_t;

// return values for STATUS register functions
typedef enum internal_status_irq_e
{
  NONE_ASSERTED, // no IRQ bits asserted
  RX_DR_ASSERTED, // RX_DR bit asserted
  TX_DS_ASSERTED, // TX_DS bit asserted
  MAX_RT_ASSERTED // MAX_RT bit asserted
} internal_status_irq_t;

// return values for public driver interface functions
typedef enum external_status_e
{
  FAIL, // operation failed
  PASS // operation success
} external_status_t;

#endif // ERROR_MANAGER_H