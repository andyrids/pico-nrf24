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

  pico_pins = (pico_pins_t){ 
    .cipo_pin = cipo_pin, 
    .copi_pin = copi_pin, 
    .csn_pin = csn_pin, 
    .sck_pin = sck_pin, 
    .ce_pin = ce_pin 
  };

  // Set GPIO function as SPI for SCK, COPI & CIPO
  gpio_set_function(pico_pins.sck_pin, GPIO_FUNC_SPI); // SCK_GP2
  gpio_set_function(pico_pins.copi_pin, GPIO_FUNC_SPI); // COPI_GP3
  gpio_set_function(pico_pins.cipo_pin, GPIO_FUNC_SPI); // CIPO_GP4

  // Initialise CE, CSN & IRQ GPIO
  gpio_init(pico_pins.ce_pin); // 6
  gpio_init(pico_pins.csn_pin); // CSN_GP5
  // gpio_init(pins.irq_pin); // 7

  // Set direction for CE, CSN & IRQ GPIO
  gpio_set_dir(pico_pins.ce_pin, GPIO_OUT);
  gpio_set_dir(pico_pins.csn_pin, GPIO_OUT);
  // gpio_set_dir(ins.irq_pin, GPIO_IN);
  
  return &pico_pins;
};