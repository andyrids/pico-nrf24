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
 * @file pin_manager.h
 * 
 * @brief contains enumerations, type definitions and utility functions
 * to initialise GPIO pins, set their function and drive pins HIGH or LOW.
 */

#ifndef PIN_MANAGER_H
#define PIN_MANAGER_H

#include "error_manager.h"
#include "pico/types.h"

// represents SPI pins and SPI pin count (ALL_PINS)
typedef enum spi_pins_e { CIPO, COPI, SCK, ALL_PINS } spi_pins_t;

typedef enum pin_min_e { CIPO_MIN, SCK_MIN = 2, COPI_MIN } pin_min_t;

typedef enum pin_max_e { SCK_MAX = 26, COPI_MAX, CIPO_MAX } pin_max_t;

// Represents GPIO LOW or HIGH (0 or 1)
typedef enum pin_direction_e { LOW, HIGH } pin_direction_t;


/**
 * Initializes GPIO pins if they validate.
 * 
 * @param copi COPI pin number
 * @param cipo CIPO pin number
 * @param sck SCK pin number
 * @param csn CSN pin number
 * @param ce CE pin number
 * 
 * @return PIN_MNGR_OK (1), ERROR (0)
 */
fn_status_t pin_manager_configure(uint8_t copi, uint8_t cipo, uint8_t sck, uint8_t csn, uint8_t ce);


/**
 * Drive CSN pin HIGH.
 * 
 * @param csn CSN pin number
 */
void csn_put_high(uint8_t csn);

/**
 * Drive CSN pin LOW.
 * 
 * @param csn CSN pin number
 */
void csn_put_low(uint8_t csn);

/**
 * Drive CE pin HIGH.
 * 
 * @param ce CE pin number
 */
void ce_put_low(uint8_t ce);

/**
 * Drive CE pin LOW.
 * 
 * @param ce CE pin number
 */
void ce_put_high(uint8_t ce);

#endif // PIN_MANAGER_H