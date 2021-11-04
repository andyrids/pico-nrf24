#ifndef PIN_MANAGER_H
#define PIN_MANAGER_H

#include "pico/types.h"

// Represents GPIO LOW or HIGH (0 or 1)
typedef enum pin_direction_e { LOW, HIGH } pin_direction_t;

// stores SPI instance, baudrate and GPIO pin values
typedef struct pico_pins_e
{
  uint8_t cipo_pin; // CIPO GPIO pin
  uint8_t copi_pin; // COPI GPIO pin
  uint8_t csn_pin; // CSN GPIO pin
  uint8_t sck_pin; // SCK GPIO pin
  uint8_t ce_pin; // CE GPIO pin
} pico_pins_t;


/**
 * ...
 * 
 * @return  
 */
pico_pins_t* pin_manager_init_pins(uint8_t cipo_pin, uint8_t copi_pin, uint8_t csn_pin, uint8_t sck_pin, uint8_t ce_pin);

/**
 * Drive CSN pin HIGH.
 * pin_manager.h
 * 
 * @param pin pico_pins_t * pin
 */
void csn_put_high(void);

/**
 * Drive CSN pin LOW.
 * pin_manager.h
 * 
 * @param pin pico_pins_t * pin
 */
void csn_put_low(void);

/**
 * Drive CE pin HIGH.
 * pin_manager.h
 * 
 * @param pin pico_pins_t * pin
 */
void ce_put_low(void);

/**
 * Drive CE pin LOW.
 * pin_manager.h
 * 
 * @param pin pico_pins_t * pin
 */
void ce_put_high(void);

#endif // PIN_MANAGER_H