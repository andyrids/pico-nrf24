#include "spi_manager.h"
#include "hardware/spi.h"

static pico_spi_t pico_spi = { .spi_instance = NULL, .spi_baudrate = 0 };

// static pico_spi_t *pico_spi_ptr = NULL;

/**
 * Checks the CIPO pin is correct and returns
 * SPI_0, SPI_1 or SPI_ERROR.
 * 
 * @param pin CIPO GPIO pin
 * @return spi_instance_t SPI_0, SPI_1 or SPI_ERROR
 */
static spi_instance_t check_cipo_pin(uint8_t pin) {
  // can be SPI_0 (0), SPI_1 (1) or SPI_ERROR (2)
  uint8_t spi_instance = INSTANCE_ERROR;

  /**
   * Valid CIPO pin: CIPO_GP0 CIPO_GP4 CIPO_GP8 CIPO_GP12 CIPO_GP16 CIPO_GP20 CIPO_GP24 CIPO_GP28
   * SPI instance:    SPI_0    SPI_0    SPI_1     SPI_1     SPI_0     SPI_0     SPI_1     SPI_1
   */

  spi_instance_t instance_pattern[8] = { SPI_0, SPI_0, SPI_1, SPI_1, SPI_0, SPI_0, SPI_1, SPI_1 };

  for (uint8_t valid_pin = CIPO_MIN; valid_pin <= CIPO_MAX; valid_pin += 4)
  {
    if (pin == valid_pin)
    { // valid_pin / 4 = corresponding index in instance_pattern[7]
      spi_instance = instance_pattern[valid_pin / 4];
      break; // break, if pin is valid
    }
  }
  // return SPI instance
  return spi_instance;
}



/**
 * Checks the COPI pin is correct and returns
 * SPI_0, SPI_1 or SPI_ERROR.
 * 
 * @param pin COPI GPIO pin
 * @return spi_instance_t SPI_0, SPI_1 or SPI_ERROR
 */
static spi_instance_t check_copi_pin(uint8_t pin) {
  // can be SPI_0 (0), SPI_1 (1) or SPI_ERROR (2)
  uint8_t spi_instance = INSTANCE_ERROR;

  /**
   * Valid COPI pin: COPI_GP3 COPI_GP7 COPI_GP11 COPI_GP15 COPI_GP19 COPI_GP23 COPI_GP27
   * SPI instance:    SPI_0    SPI_0    SPI_1     SPI_1     SPI_0     SPI_0     SPI_1
   */

  spi_instance_t instance_pattern[7] = { SPI_0, SPI_0, SPI_1, SPI_1, SPI_0, SPI_0, SPI_1 };

  for (uint8_t valid_pin = COPI_MIN; valid_pin <= COPI_MAX; valid_pin += 4)
  {
    if (pin == valid_pin)
    { // valid_pin / 4 = corresponding index in instance_pattern[7]
      spi_instance = instance_pattern[valid_pin / 4];
      break; // break, if pin is valid
    }
  }
  // return SPI instance
  return spi_instance;
}


/**
 * Checks the SCK pin is correct and returns
 * SPI_0, SPI_1 or SPI_ERROR.
 * 
 * @param pin SCK GPIO pin
 * @return spi_instance_t SPI_0, SPI_1 or SPI_ERROR
 */
static spi_instance_t check_sck_pin(uint8_t pin) {
  // can be SPI_0 (0), SPI_1 (1) or SPI_ERROR (2)
  uint8_t spi_instance = INSTANCE_ERROR;

  /**
   * Valid SCK pin: SCK_GP2 SCK_GP6 SCK_GP10 SCK_GP14 SCK_GP18 SCK_GP22 SCK_GP26
   * SPI instance:   SPI_0   SPI_0   SPI_1    SPI_1    SPI_0    SPI_0    SPI_1
   */

  spi_instance_t instance_pattern[7] = { SPI_0, SPI_0, SPI_1, SPI_1, SPI_0, SPI_0, SPI_1 };

  for (uint8_t valid_pin = SCK_MIN; valid_pin <= SCK_MAX; valid_pin += 4)
  {
    if (pin == valid_pin)
    { // valid_pin / 4 = corresponding index in instance_pattern[7]
      spi_instance = instance_pattern[valid_pin / 4];
      break; // break, if pin is valid
    }
  }
  // return SPI instance
  return spi_instance;
}

spi_instance_t spi_manager_check_pins(uint8_t cipo_pin, uint8_t copi_pin, uint8_t sck_pin) {
  // determined SPI instance for CIPO, COPI, CSN & SCK pins
  spi_instance_t spi_instances[3];

  // final suitable SPI instance
  spi_instance_t spi_instance = INSTANCE_ERROR;

  // spi_instance[i] = SPI_0 (0) || SPI_1 (1) || SPI_ERROR (2)
  spi_instances[CIPO] = check_cipo_pin(cipo_pin); 
  spi_instances[COPI] = check_copi_pin(copi_pin);
  spi_instances[SCK] = check_sck_pin(sck_pin);

  // count of SPI_0 (0), SPI_1 (1) & SPI_ERROR (2)
  spi_instance_t instance_count[3] = { 0, 0, 0 };

  // count occurrence algorithm for SPI instance
  for (uint8_t i = 0; i < ALL_PINS; i++)
  {
    instance_count[spi_instances[i]]++;
    
    if (spi_instances[i] == INSTANCE_ERROR)
    { 
      spi_instance = INSTANCE_ERROR; // set error flag
      break; // break on error
    }
  }

  if (instance_count[INSTANCE_ERROR] == 0)
  {
    spi_instance = (instance_count[SPI_0] == ALL_PINS) ? SPI_0 : SPI_1;
  }

  return spi_instance;
}

void spi_manager_init_spi(void) {

  // initialise SPI instance at the specified baudrate (Hz)
  spi_init(pico_spi.spi_instance, pico_spi.spi_baudrate);

  /**
   * spi_set_format is also called within the pico-sdk spi_init function, with 
   * the same arguments as below. Function called here to illustrate settings.
   */
  spi_set_format(pico_spi.spi_instance, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

  return;
} 

void spi_manager_deinit_spi(void) {

  // deinitialise SPI instance
  spi_deinit(pico_spi.spi_instance);

  return;
} 


internal_status_t spi_manager_transfer(const uint8_t *tx_buffer, uint8_t *rx_buffer, size_t num_bytes) {

  sleep_us(2);
  uint8_t bytes_written = spi_write_read_blocking(pico_spi.spi_instance, tx_buffer, rx_buffer, num_bytes);
  sleep_us(2);

  internal_status_t internal_status = (bytes_written == num_bytes) ? OK : SPI_FAIL;

  return internal_status;
}


pico_spi_t* spi_manager_set_spi(spi_instance_t spi_instance, uint32_t baudrate_hz) {

  pico_spi.spi_instance = (spi_instance == SPI_0) ? spi0 : spi1;
  
  pico_spi.spi_baudrate = baudrate_hz;

  // initialise the SPI
  
  // also called within the pico-sdk spi_init() function, with the same arguments
  spi_set_format(pico_spi.spi_instance, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

  return &pico_spi;
}

