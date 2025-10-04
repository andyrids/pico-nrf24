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


/**
 * Validates the SPI GPIO pin numbers provided.
 * 
 * @param copi COPI pin number
 * @param cipo CIPO pin number
 * @param sck SCK pin number
 * 
 * @return PIN_MNGR_OK (1), ERROR (0)
 */
static fn_status_t pin_manager_validate(uint8_t copi, uint8_t cipo, uint8_t sck) {

  // Can test the pins are valid by masking and comparing
  fn_status_t status;
  copi &= 3;
  cipo &= 3;
  sck  &= 3;

  if ((copi == 3) && (cipo == 0) && (sck == 2))
    status =  PIN_MNGR_OK;
  else
    status =  ERROR;

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