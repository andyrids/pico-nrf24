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
 * @file pin_manager.c
 * 
 * @brief utility functions to initialise GPIO pins, set their function 
 * and drive pins HIGH or LOW.
 */

#include "pin_manager.h"
#include "hardware/gpio.h"

// represents SPI pins and SPI pin count (ALL_PINS)
typedef enum spi_pins_e { CIPO, COPI, SCK, ALL_PINS } spi_pins_t;

typedef enum spi_min_range_e { CIPO_MIN = 0, CSN_MIN, SCK_MIN, COPI_MIN } spi_min_range_t; // lowest available SPI GPIO numbers
typedef enum spi_max_range_e { SCK_MAX = 26, COPI_MAX, CIPO_MAX, CSN_MAX } spi_max_range_t; // highest available SPI GPIO numbers



void csn_put_high(uint8_t csn) {

  gpio_put(csn, HIGH);

  return;
}

void csn_put_low(uint8_t csn) {

  gpio_put(csn, LOW);

  return;
}

void ce_put_high(uint8_t ce) {

  gpio_put(ce, HIGH);

  return;
}

void ce_put_low(uint8_t ce) {

  gpio_put(ce, LOW);

  return;
}



static fn_status_t pin_manager_validate(uint8_t copi, uint8_t cipo, uint8_t sck) {

  typedef enum pin_min_e { CIPO_MIN, SCK_MIN = 2, COPI_MIN } pin_min_t;
  typedef enum pin_max_e { SCK_MAX = 26, COPI_MAX, CIPO_MAX } pin_max_t;

  typedef struct validate_pin_e
  {
    uint8_t spi_pin;
    pin_min_t min;
    pin_max_t max;
  } validate_pin_t;

  validate_pin_t spi_pins[3] = {
    (validate_pin_t){ .spi_pin = cipo, .min = CIPO_MIN, .max = CIPO_MAX },
    (validate_pin_t){ .spi_pin = copi, .min = COPI_MIN, .max = COPI_MAX },
    (validate_pin_t){ .spi_pin = sck,  .min = SCK_MIN,  .max = SCK_MAX },
  };

  uint8_t valid_pins = 0;

  for (size_t i = 0; i < ALL_PINS; i++)
  {
    for (size_t pin = spi_pins[i].min; pin <= spi_pins[i].max; pin += 4)
    {
      if (spi_pins[i].spi_pin == pin) { valid_pins++; }
    }
  }

  fn_status_t status = (valid_pins == 3) ? PIN_MNGR_OK : ERROR;
  
  return status;
}


fn_status_t pin_manager_configure(uint8_t copi, uint8_t cipo, uint8_t sck, uint8_t csn, uint8_t ce) {

  fn_status_t status = pin_manager_validate(copi, cipo, sck);

  if (status)
  {
    // set GPIO function as SPI for SCK, COPI & CIPO
    gpio_set_function(sck, GPIO_FUNC_SPI);
    gpio_set_function(copi, GPIO_FUNC_SPI);
    gpio_set_function(cipo, GPIO_FUNC_SPI);

    // initialise CE & CSN
    gpio_init(ce);
    gpio_init(csn);

    // set direction for CE & CSN
    gpio_set_dir(ce, GPIO_OUT);
    gpio_set_dir(csn, GPIO_OUT);
  }
  
  return status;
};