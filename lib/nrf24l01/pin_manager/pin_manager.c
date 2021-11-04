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

// pico_pins_t struct holds GPIO pin values
static pico_pins_t pico_pins = { .cipo_pin = 100, .copi_pin = 100, .csn_pin = 100, .sck_pin = 100 };

void csn_put_high(void) {

  if (pico_pins.csn_pin != 100)
  {
    gpio_put(pico_pins.csn_pin, HIGH);
  }

  return;
}

void csn_put_low(void) {

  if (pico_pins.csn_pin != 100)
  {
    gpio_put(pico_pins.csn_pin, LOW);
  }

  return;
}

void ce_put_high(void) {

  if (pico_pins.ce_pin != 100)
  {
    gpio_put(pico_pins.ce_pin, HIGH);
  }

  return;
}

void ce_put_low(void) {

  if (pico_pins.ce_pin != 100)
  {
    gpio_put(pico_pins.ce_pin, LOW);
  }

  return;
}

pico_pins_t* pin_manager_init_pins(uint8_t cipo_pin, uint8_t copi_pin, uint8_t csn_pin, uint8_t sck_pin, uint8_t ce_pin) {

  // set pico_pins GPIO values
  pico_pins = (pico_pins_t){ 
    .cipo_pin = cipo_pin, 
    .copi_pin = copi_pin, 
    .csn_pin = csn_pin, 
    .sck_pin = sck_pin, 
    .ce_pin = ce_pin 
  };

  // Set GPIO function as SPI for SCK, COPI & CIPO
  gpio_set_function(pico_pins.sck_pin, GPIO_FUNC_SPI);
  gpio_set_function(pico_pins.copi_pin, GPIO_FUNC_SPI);
  gpio_set_function(pico_pins.cipo_pin, GPIO_FUNC_SPI);

  // Initialise CE & CSN
  gpio_init(pico_pins.ce_pin);
  gpio_init(pico_pins.csn_pin);


  // Set direction for CE & CSN
  gpio_set_dir(pico_pins.ce_pin, GPIO_OUT);
  gpio_set_dir(pico_pins.csn_pin, GPIO_OUT);
  
  return &pico_pins;
};