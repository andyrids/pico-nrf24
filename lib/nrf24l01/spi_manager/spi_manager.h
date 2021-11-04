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
 */
typedef enum spi_min_range_e { CIPO_MIN = 0, CSN_MIN, SCK_MIN, COPI_MIN } spi_min_range_t; // lowest available SPI GPIO numbers
typedef enum spi_max_range_e { SCK_MAX = 26, COPI_MAX, CIPO_MAX, CSN_MAX } spi_max_range_t; // highest available SPI GPIO numbers

typedef struct pico_spi_s
{
  spi_inst_t *spi_instance; // SPI instance
  uint32_t spi_baudrate; // SPI baudrate in Hz
} pico_spi_t;

/**
 * Checks the CIPO, COPI and SCK pins are correct and returns
 * SPI_0, SPI_1 or INSTANCE_ERROR, indicating if all pins are 
 * part of SPI 0, SPI 1 interfaces or not (INSTANCE_ERROR) .
 * 
 * @param pin CIPO GPIO pin
 * @param pin COPI GPIO pin
 * @param pin SCK GPIO pin
 * @return spi_instance_t SPI_0, SPI_1 or INSTANCE_ERROR
 */
spi_instance_t spi_manager_check_pins(uint8_t cipo_pin, uint8_t copi_pin, uint8_t sck_pin);


/**
 * C...
 * 
 * @param spi_instance spi_instance_t (SPI_0 or SPI_1)
 * @param baudrate_hz baudrate in hz
 * @return pointer to pico_spi_t struct variable
 */
pico_spi_t* spi_manager_set_spi(spi_instance_t spi_instance, uint32_t baudrate_hz);


void spi_manager_init_spi(void);
void spi_manager_deinit_spi(void);


/**
 * C...
 * 
 * @param tx_buffer 
 * @param rx_buffer
 * @param num_bytes
 * @return internal_status_t
 */
internal_status_t spi_manager_transfer(const uint8_t *tx_buffer, uint8_t *rx_buffer, size_t num_bytes);

#endif // SPI_MANAGER_H