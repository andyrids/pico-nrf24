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

typedef enum fn_status_e
{
  ERROR,
  PIN_MNGR_OK,
  SPI_MNGR_OK,
  NRF_MNGR_OK
} fn_status_t;

// return values for STATUS register functions
typedef enum fn_status_irq_e
{
  NONE_ASSERTED, // no IRQ bits asserted
  RX_DR_ASSERTED, // RX_DR bit asserted
  TX_DS_ASSERTED, // TX_DS bit asserted
  MAX_RT_ASSERTED // MAX_RT bit asserted
} fn_status_irq_t;

#endif // ERROR_MANAGER_H