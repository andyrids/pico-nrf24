#include "nrf24_driver.h"
#include "pin_manager.h"
#include "spi_manager.h"

/**
 * pointers to structs:
 * 
 * 1. pin_manager pico_pins_t struct, which stores GPIO pins
 * 
 * 2. pi_manager pico_spi_t struct, which stores SPI instance & baudrate
 * 
 * 3. nrf24_driver struct, which stores address and payload width, and an
 * array to cache the address set to RX_ADDR_P0 register. is_rx_addr_p0 is
 * used as a flag to determine whether the address has been cached. 
 * The device_mode struct member reflects the NRF24L01 operational modes; 
 * Power Down ('P'), Standby ('S'), RX Mode ('R') and TX Mode ('T').
 */
static pico_pins_t *pico_pins = NULL;
static pico_spi_t *pico_spi = NULL;

static device_status_t nrf_status = { 
  .mode = 'S', // 'S' standby, 'T' TX Mode, 'R' RX Mode
  .address_width = FIVE_BYTES, // address width (bytes)
  .payload_width = 0, // payload width (bytes)
  .is_rx_addr_p0 = false, // has RX_ADDR_P0 address been cached?
  .rx_addr_p0 = { 0x00, 0x00, 0x00, 0x00, 0x00 } // for cached RX_ADDR_P0 address
};



external_status_t init_pico(uint8_t cipo_pin, uint8_t copi_pin, uint8_t csn_pin, uint8_t sck_pin, uint8_t ce_pin, uint32_t baudrate_hz) {

  // check SPI pins and indicate SPI instance SPI_0 & SPI_1 or INSTANCE_ERROR
  spi_instance_t spi_instance = spi_manager_check_pins(cipo_pin, copi_pin, sck_pin);

  // if SPI pins are incorrect, reflect PARAMETER_FAIL in internal_status
  internal_status_t internal_status = (spi_instance != INSTANCE_ERROR) ? OK : PARAMETER_FAIL;

  if (internal_status == OK)
  {
    // initialise SPI settings, store SPI instance and baudrate and return pointer to pico_spi_t struct
    pico_spi = spi_manager_set_spi(spi_instance, baudrate_hz);

    // initialise GPIO pin function, store GPIO pin numbers & return pointer to pico_pins_t struct
    pico_pins = pin_manager_init_pins(cipo_pin, copi_pin, csn_pin, sck_pin, ce_pin);
  }

  // public function returns external_status_t
  external_status_t external_status = (internal_status == OK) ? PASS : FAIL;

  // return PASS (1) or FAIL (0)
  return external_status;
}


/**
 * writes the buffer value to the specified register.
 * 
 * @param reg register to write the buffer to
 * @param buffer value to be held in the register
 * @param buffer_size size of the buffer
 * 
 * @return OK (0) || REGISTER_W_FAIL (3)
 */
static internal_status_t w_register(register_map_t reg, const void *buffer, size_t buffer_size) {

  // will hold OK (0) or REGISTER_W_FAIL (3)
  internal_status_t internal_status = OK; 

  uint8_t *buffer_ptr = (uint8_t *)buffer;

  // buffer_size + 1 (register address)
  size_t total_size = buffer_size + 1;

  /**
   * RX_ADDR_P2 - RX_ADDR_P5 registers have one byte written. If buffer 
   * argument is a five byte array, like that used when writing to 
   * RX_ADDR_P0 and RX_ADDR_P1 registers, the loop guarantees only two 
   * bytes are written:- register address and one byte from the buffer.
   */
  for (uint8_t i = RX_ADDR_P2; i <= RX_ADDR_P5; i++)
  { 
    if (reg == i) 
    {
      total_size = TWO_BYTES;
      break;
    } 
  }

  // ensure 3 MSB are [001] (write to the register)
  reg = (REGISTER_MASK & reg | W_REGISTER);

  uint8_t tx_buffer[total_size]; // SPI Tx buffer
  uint8_t rx_buffer[total_size]; // SPI Rx buffer

  *(tx_buffer + 0) = reg; // store register in tx_buffer[0]

  // store byte(s) in buffer (buffer_ptr) into tx_buffer[total_size]
  for (size_t i = 1; i < total_size; i++) { *(tx_buffer + i) = *(buffer_ptr++); }
  
  csn_put_low(); // drive CSN pin LOW
  internal_status = spi_manager_transfer(tx_buffer, rx_buffer, total_size);
  csn_put_high(); // drive CSN pin HIGH

  return internal_status; // return error flag value
}

/**
 * Reads one byte from the specified register.
 *  
 * @param reg register address
 * 
 * @return register value 
 */
uint8_t r_register_byte(register_map_t reg) {

  /**
   * NRF24L01 returns the STATUS register value,
   * hence why tx_buffer and rx_buffer are two
   * bytes. The first index of rx_buffer will
   * hold the STATUS register value. The second
   * index will hold the value of the specified
   * register.
   */
  uint8_t tx_buffer[TWO_BYTES] = { reg, NOP };
  uint8_t rx_buffer[TWO_BYTES];

  csn_put_low(); // drive CSN pin LOW
  spi_manager_transfer(tx_buffer, rx_buffer, TWO_BYTES);
  csn_put_high(); // drive CSN pin HIGH

  // *(rx_buffer + 0) holds STATUS register value
  return *(rx_buffer + 1);
}

/**
 * Reads a number of bytes from the specified register.
 * 
 * NOTE: Works the same as r_register_byte function. 
 * Some registers hold more than one byte, such as
 * RX_ADDR_P0 - RX_ADDR_P5 registers (5 bytes). An
 * array buffer is used to store the register value.
 * 
 * @param reg register address
 * @param buffer buffer to store register value
 * @param buffer_size size of buffer
 * 
 * @return OK (0) || REGISTER_R_FAIL (2)
 */
static internal_status_t r_register_bytes(register_map_t reg, uint8_t *buffer, size_t buffer_size) {

  // will hold OK (0) or REGISTER_R_FAIL (2)
  internal_status_t internal_status = OK;

  /**
   * NRF24L01 returns the STATUS register value,
   * hence why tx_buffer and rx_buffer are the
   * size of buffer_size + 1. The first index 
   * of rx_buffer will hold the STATUS register 
   * value. The remainder will hold the value
   * of the specified register.
   */
  size_t total_size = buffer_size + 1;

  uint8_t tx_buffer[total_size];
  uint8_t rx_buffer[total_size];

  // store register address in tx_buffer[0]
  *(tx_buffer + 0) = reg;

  // fill rest of tx_buffer with NOP
  for (size_t i = 1; i < total_size; i++) { *(tx_buffer + i) = NOP; }

  csn_put_low(); // drive CSN pin LOW
  size_t bytes_written = spi_manager_transfer(tx_buffer, rx_buffer, total_size);
  csn_put_high(); // drive CSN pin HIGH
  

  // skip rx_buffer[0] (STATUS value) and transfer remaining values to buffer
  for (uint8_t i = 1; i < total_size; i++) { *(buffer++) = *(rx_buffer + i); }

  // set internal_status to OK || REGISTER_R_FAIL
  internal_status = (bytes_written == total_size) ? OK : REGISTER_R_FAIL;

  return internal_status;
}

/**
 * Set Auto Retransmit Delay (ARD) and Auto Retransmit Count (ARC) 
 * in the SETUP_RETR register. The datasheet states that the delay
 * is defined from end of the transmission to start of the next 
 * transmission.
 * 
 * NOTE: ARD is the time the PTX is waiting for an ACK packet before 
 * a retransmit is made. The PTX is in RX mode for 250μS (500μS in 
 * 250kbps mode) to wait for address match. If the address match is 
 * detected, it stays in RX mode to the end of the packet, unless 
 * ARD elapses. Then it goes to standby-II mode for the rest of the
 * specified ARD. After the ARD, it goes to TX mode and retransmits 
 * the packet.
 * 
 * @param retr_delay Auto Retransmit Delay (ARD_250US, ARD_500US, ARD_750US, ARD_1000US)
 * @param retr_count Auto Retransmit Count (ARC_NONE, ARC_1RT - ARC_15RT)
 * 
 * @return PASS (0) || FAIL (1)
 */
external_status_t set_auto_retransmission(retr_delay_t retr_delay, retr_count_t retr_count) {
  spi_manager_init_spi(); // initialise SPI for function duration

  // holds OK (0) || REGISTER_W_FAIL (3)
  internal_status_t internal_status = OK;

  uint8_t retr_settings = (retr_delay | retr_count);
  // write specified ARD and ARC settings to SETUP_RETR register
  internal_status = w_register(SETUP_RETR, &retr_settings, ONE_BYTE);

  // success of writing settings to SETUP_RETR register
  external_status_t external_status = (internal_status == OK) ? PASS : FAIL;

  spi_manager_deinit_spi(); // deinitialise SPI at function end

  return external_status;
}

/**
 * Set the specified address width in the SETUP_AW register. By default,
 * the driver uses an address width of 5 bytes.
 * 
 * @param address_width address width (AW_3_BYTES, AW_4_BYTES, AW_5_BYTES)
 */
static internal_status_t set_address_width(address_width_t address_width) {

  switch (address_width)
  {
    case AW_3_BYTES:
      nrf_status.address_width = THREE_BYTES;
    break;
    case AW_4_BYTES:
      nrf_status.address_width = FOUR_BYTES;
    break;
    case AW_5_BYTES:
      nrf_status.address_width = FIVE_BYTES;
    break;
  }

  // holds OK (0) || REGISTER_W_FAIL (3)
  internal_status_t internal_status = w_register(SETUP_AW, &address_width, ONE_BYTE);

  return internal_status;
}

// set RF power level
external_status_t set_rf_power(rf_power_t rf_pwr) {

  spi_manager_init_spi(); // initialise SPI for function duration // initialise SPI for function duration

  // Read RF_SETUP register value
  uint8_t rf_setup = r_register_byte(RF_SETUP);

  rf_setup = (rf_setup & RF_SETUP_RF_DR_MASK) | (rf_pwr & RF_SETUP_RF_PWR_MASK);

  internal_status_t internal_status = w_register(RF_SETUP, &rf_setup, ONE_BYTE);

  external_status_t external_status = (internal_status == OK) ? PASS : FAIL;

  spi_manager_deinit_spi(); // deinitialise SPI at function end

  return external_status;
}

// set RF data rate
external_status_t set_rf_data_rate(rf_data_rate_t data_rate) {

  spi_manager_init_spi(); // initialise SPI for function duration

  // Value of RF_SETUP register
  uint8_t rf_setup = r_register_byte(RF_SETUP);

  rf_setup = (rf_setup & RF_SETUP_RF_PWR_MASK) | (data_rate & RF_SETUP_RF_DR_MASK);

  internal_status_t internal_status = w_register(RF_SETUP, &rf_setup, ONE_BYTE);

  external_status_t external_status = (internal_status == OK) ? PASS : FAIL;

  spi_manager_deinit_spi(); // deinitialise SPI at function end

  return external_status;
}

/**
 * Enables Enhanced ShockBurst™ auto acknowledgment (AA) 
 * function on the specified data pipe(s).
 * 
 * @param setting (ENAA_NONE, ENAA_P0, ENAA_P1, ENAA_P2, ENAA_P3, ENAA_P4, ENAA_P5, ENAA_ALL)
 * 
 * @return OK (0) || REGISTER_W_FAIL (3)
 */
static internal_status_t enable_auto_acknowledge(en_auto_ack_t setting) {

  internal_status_t internal_status = w_register(EN_AA, &setting, ONE_BYTE);

  return internal_status;
}


external_status_t set_payload_size(data_pipe_t data_pipe, uint8_t payload_width) {
  spi_manager_init_spi(); // initialise SPI for function duration

  // OK (0) || PARAMETER_FAIL (1) || REGISTER_W_FAIL (3)
  internal_status_t internal_status = OK;

  internal_status = ((payload_width > ZERO_BYTES) && (payload_width <= MAX_BYTES)) ? OK : PARAMETER_FAIL;

  if (internal_status == OK)
  {
    // cache payload_width value
    nrf_status.payload_width = payload_width;

    uint8_t rx_pw_registers[6] = { 
      RX_PW_P0, RX_PW_P1, RX_PW_P2, 
      RX_PW_P3, RX_PW_P4, RX_PW_P5 
    };

    for (uint8_t rx_pw_register = RX_PW_P0; rx_pw_register <= RX_PW_P5; rx_pw_register++)
    {
      if (data_pipe == ALL_DATA_PIPES)
      {
        internal_status = w_register(rx_pw_register, &payload_width, ONE_BYTE);
        if (internal_status != OK) { break; } // break on error
      }

      if (rx_pw_registers[data_pipe] == rx_pw_register)
      {
        internal_status = w_register(rx_pw_register, &payload_width, ONE_BYTE);
        break;
      }
    }
  }
  
  // determine success of operation
  external_status_t external_status = (internal_status == OK) ? PASS : FAIL;

  spi_manager_deinit_spi(); // deinitialise SPI at function end

  return external_status;
}


external_status_t set_tx_address(const uint8_t *address) {
  spi_manager_init_spi(); // initialise SPI for function duration

  // OK (0) || REGISTER_W_FAIL (3)
  internal_status_t internal_status = OK;

  /**
   * as auto-acknowledge is enabled, the RX_ADDR_P0 (data pipe 0)
   * address is set to the same address as TX_ADDR. If RX_ADDR_P0
   * address was previously set (set_rx_address function), it will 
   * have been cached and will be restored when necessary.
   */
  internal_status = w_register(RX_ADDR_P0, address, FIVE_BYTES);
  internal_status = w_register(TX_ADDR, address, FIVE_BYTES);

  external_status_t external_status = (internal_status == OK) ? PASS : FAIL;

  spi_manager_deinit_spi(); // deinitialise SPI at function end

  return external_status;
}


/**
 * sets a 5 byte address to the specified data pipe
 * address register.
 * 
 * A primary receiver (PRX) can receive data from
 * six primary transmitters (PTX), one per data pipe.
 * Each data pipe has a unique address.
 * 
 * 1. RX_ADDR_P0 and RX_ADDR_P1 registers can have a 
 * maximum of a five byte address set. The size of the 
 * address is set using the SETUP_AW register (0x03). 
 * This driver uses a five byte address width.
 * 
 * 2. RX_ADDR_P2 - RX_ADDR_P5 automatically share bits
 * 8 - 39 MSB of RX_ADDR_P1 and are simply set with a 
 * unique 1 byte value (LSB), which would act as bits 
 * 0 - 7 of the full 40 bit address.
 * 
 * 3. Data pipes are enabled in the EN_RXADDR register.
 * If the data pipe to set the address for is not already
 * enabled, it will be enabled in the EN_RXADDR register.
 * By default, NRF24L01 has data pipes 0 and 1 enabled.
 * 
 * Example buffer values:
 * 
 * (uint8_t[]){0x37, 0x37, 0x37, 0x37, 0x37} or (uint8_t[]){0x37}
 * 
 * data_pipe_t values:
 * 
 * DATA_PIPE_0 (0), DATA_PIPE_1 (1), DATA_PIPE_2 (2), 
 * DATA_PIPE_3 (3), DATA_PIPE_4 (4), DATA_PIPE_5 (5)
 * 
 * @param data_pipe data pipe number (0 - 5)
 * @param buffer address for the data pipe
 * 
 * @return PASS (1) || FAIL (0)
**/
external_status_t set_rx_address(data_pipe_t data_pipe, const uint8_t *buffer) {

  spi_manager_init_spi(); // initialise SPI for function duration

  // read value of EN_RXADDR register
  uint8_t en_rxaddr = r_register_byte(EN_RXADDR);

  uint8_t registers[6] = {
    RX_ADDR_P0, RX_ADDR_P1, RX_ADDR_P2,
    RX_ADDR_P3, RX_ADDR_P4, RX_ADDR_P5
  };

  if (data_pipe == DATA_PIPE_0)
  {
    nrf_status.is_rx_addr_p0 = true;

    for (size_t i = 0; i < nrf_status.address_width; i++)
    { 
      nrf_status.rx_addr_p0[i] = *(buffer++);
    }
  }

  // will hold OK (0) || REGISTER_W_FAIL (3)
  internal_status_t internal_status = REGISTER_W_FAIL;

  // if DATA_PIPE_0 - DATA_PIPE_5
  if (data_pipe < DATA_PIPE_5)
  {
    if (data_pipe < DATA_PIPE_2) // DATA_PIPE_0 & DATA_PIPE_1 hold 5 byte address
    {
      internal_status = w_register(registers[data_pipe], buffer, FIVE_BYTES);
    } else {  // DATA_PIPE_2 - DATA_PIPE_5 hold 1 byte + 4 bytes of DATA_PIPE_1

      internal_status = w_register(registers[data_pipe], buffer, ONE_BYTE);
    }

    // enable data pipe in EN_RXADDR register, if necessary
    if ((en_rxaddr >> data_pipe & SET_BIT) != SET_BIT)
    {
      en_rxaddr |= SET_BIT << data_pipe;
      internal_status = w_register(EN_RXADDR, &en_rxaddr, ONE_BYTE);
    }
  }

  external_status_t external_status = (internal_status == OK) ? PASS : FAIL;

  spi_manager_deinit_spi(); // deinitialise SPI at function end

  return external_status;
}


/**
 * Writes FLUSH_TX instruction over SPI to NRF24L01,
 * which will flush the TX FIFO.
 * 
 * @return true || false depending on success of operation
 */
static void flush_tx_fifo(void) {
  
  csn_put_low();
  spi_write_blocking(pico_spi->spi_instance, (uint8_t []){FLUSH_TX}, ONE_BYTE);
  csn_put_high();

  return;
}

/**
 * Writes FLUSH_RX instruction over SPI to NRF24L01,
 * which will flush the RX FIFO.
 * 
 * @return true || false depending on success of operation
 */
static void flush_rx_fifo(void) {
  
  csn_put_low();
  spi_write_blocking(pico_spi->spi_instance, (uint8_t []){FLUSH_RX}, ONE_BYTE);
  csn_put_high();

  return;
}


external_status_t init_nrf24(uint8_t rf_channel) {

  // static struct holding NRF24L01 details


  // pass address to static global pointer
  // nrf_status = &device_status;

  spi_manager_init_spi(); // initialise SPI for function duration

  /** With a VDD of 1.9V or higher, nRF24L01+ enters the Power on reset state **/

  sleep_ms(100); // nRF24L01+ enters Power Down mode after 100ms

  /** NRF24L01+ is now in Power Down mode. PWR_UP bit in the CONFIG register is low **/
  
  ce_put_low(); // CE to low in preperation for entering Standby-I mode

  sleep_ms(1);

  csn_put_high(); // CSN high in preperation for writing to registers

   // PWR_UP bit is now high
  internal_status_t internal_status = w_register(CONFIG, (uint8_t []){0b00001110}, ONE_BYTE);

  sleep_ms(2); // Crystal oscillator start up delay

  /** NRF24L01+ now in Standby-I mode. PWR_UP bit in CONFIG is high & CE pin is low **/
  nrf_status.mode = 'S';

  // enable auto-acknowledge on all data pipes
  internal_status = enable_auto_acknowledge(ENAA_ALL);

  // set address width of 5 bytes on all data pipes
  internal_status = set_address_width(AW_5_BYTES);

  // set Auto Retransmit Delay of 500us & Auto Retransmit Count of 10
  internal_status = w_register(SETUP_RETR, (uint8_t []){(ARD_500US | ARC_10RT)}, ONE_BYTE);

  // set RF channel to default value of 76 in the RF_CH register
  internal_status = w_register(RF_CH, (uint8_t []){76}, ONE_BYTE);

  // set RF Data Rate to 1Mbps & set RF power to 0dBm 
  internal_status = w_register(RF_SETUP, (uint8_t []){(RF_DR_1MBPS | RF_PWR_0DBM)}, ONE_BYTE);

  // reset RX_DR, TX_DS & MAX_RT interrupt bits in STATUS register
  internal_status = w_register(STATUS, (uint8_t []){STATUS_INTERRUPT_MASK}, ONE_BYTE); 

  flush_tx_fifo(); // flush TX FIFO
  flush_rx_fifo(); // flush RX FIFO

  external_status_t external_status = (internal_status == OK) ? PASS : FAIL;

  spi_manager_deinit_spi(); // deinitialise SPI at function end

  return external_status;
}

/**
 * Check STATUS register RX_DR, TX_DS and MAX_RT 
 * bits and return a internal_status_irq_t value,
 * indicating which bit is asserted.
 * 
 * @return NONE_ASSERTED (0), RX_DR_ASSERTED (1), 
 * TX_DS_ASSERTED (2), MAX_RT_ASSERTED (3)
 */
static internal_status_irq_t check_status_irq(void) {
  // value of STATUS register
  uint8_t status = r_register_byte(STATUS);

  // test which interrupt was asserted
  uint8_t rx_dr = (status >> STATUS_RX_DR) & SET_BIT; // Asserted when packet received
  uint8_t tx_ds = (status >> STATUS_TX_DS) & SET_BIT; // Asserted when auto-acknowledge received
  uint8_t max_rt = (status >> STATUS_MAX_RT) & SET_BIT; // Asserted when max retries reached

  // NONE_ASSERTED (0), RX_DR_ASSERTED (1), TX_DS_ASSERTED (2), MAX_RT_ASSERTED (3)
  internal_status_irq_t asserted_bit = NONE_ASSERTED;

  // used to write 1 to asserted bit to reset
  uint8_t reset_bit = 0; 

  if (rx_dr)
  { 
    reset_bit = (SET_BIT << STATUS_RX_DR);
    // reset RX_DR (bit 6) in STATUS register by writing 1
    w_register(STATUS, &reset_bit, ONE_BYTE);
    // indicate RX_DR bit asserted in STATUS register
    asserted_bit = RX_DR_ASSERTED;
  }

  if (tx_ds)
  {
    reset_bit = (SET_BIT << STATUS_TX_DS);
    // reset TX_DS (bit 5) in STATUS register by writing 1
    w_register(STATUS, &reset_bit, ONE_BYTE);
    // indicate TX_DS bit asserted in STATUS register
    asserted_bit = TX_DS_ASSERTED; 
  }

  if (max_rt)
  {
    reset_bit = (SET_BIT << STATUS_MAX_RT);
    // reset MAX_RT (bit 4) in STATUS register by writing 1
    w_register(STATUS, &reset_bit, ONE_BYTE);
    // indicate MAX_RT bit asserted in STATUS register
    asserted_bit = MAX_RT_ASSERTED; 
  }

  return asserted_bit;
}

// transmit packet - see nrf24_driver.h
external_status_t tx_packet(const void *tx_packet, size_t size) {

  spi_manager_init_spi(); // initialise SPI for function duration
  
  internal_status_t internal_status = (size <= nrf_status.payload_width) ? OK : PARAMETER_FAIL;

  // cast void *tx_packet to uint8_t pointer
  const uint8_t *tx_packet_ptr = (uint8_t *)tx_packet;

  // W_TX_PAYLOAD command + packet_size
  size_t total_size = size + 1;

  uint8_t tx_buffer[total_size]; // SPI transfer TX buffer
  uint8_t rx_buffer[total_size]; // SPI transfer RX buffer

  // put W_TX_PAYLOAD command into first index of tx_buffer
  *(tx_buffer + 0) = W_TX_PAYLOAD;

  // store byte(s) in tx_packet (tx_packet_ptr) into tx_buffer[]
  for (size_t i = 1; i < total_size; i++) { *(tx_buffer + i) = *(tx_packet_ptr++); }
  
  csn_put_low(); // drive CSN pin LOW
  internal_status = spi_manager_transfer(tx_buffer, rx_buffer, total_size);
  csn_put_high(); // drive CSN pin HIGH
  

  ce_put_high();
  sleep_us(100);
  ce_put_low();

  external_status_t external_status = (internal_status == OK) ? PASS : FAIL;

  internal_status_irq_t status_irq = NONE_ASSERTED;

  /**
   * if spi_manager_transfer returned OK (internal_status), then poll STATUS register, 
   * checking TX_DS (auto-acknowledgement received) and MAX_RT (max retransmissions)
   * bits in the STATUS register. If neither bits are set (NONE_ASSERTED), keep polling. 
   * No point in polling the STATUS register if SPI packet transfer was not successful,
   * hence (internal_status == OK) && (status_irq = NONE_ASSERTED).
   */
  while ((internal_status == OK) && (status_irq == NONE_ASSERTED))
  {
    status_irq = check_status_irq();
    external_status = (status_irq == TX_DS_ASSERTED) ? PASS : FAIL;
  }

  spi_manager_deinit_spi(); // deinitialise SPI at function end
    
  /**
   * only returns PASS (1) if spi_manager_transfer was successful and if check_status_irq
   * returned TX_DS_ASSERTED. Else FAIL (0), which indicates either an error in the SPI 
   * transfer of the packet or that auto-acknowledgment was not received after the packet 
   * was retransmitted the max number of times, indicated by the STATUS register MAX_RT 
   * bit being asserted (1).
   */
  return external_status;
}

external_status_t rx_packet(void *rx_packet, size_t packet_size) {

  spi_manager_init_spi(); // initialise SPI for function duration

  internal_status_t internal_status = OK;

  internal_status = (packet_size <= nrf_status.payload_width) ? OK : PARAMETER_FAIL;

  // cast void *tx_packet to uint8_t pointer
  uint8_t *rx_packet_ptr = (uint8_t *)rx_packet;

    // W_TX_PAYLOAD command + packet_size
  size_t total_size = packet_size + 1;

  uint8_t tx_buffer[total_size]; // SPI transfer TX buffer
  uint8_t rx_buffer[total_size]; // SPI transfer RX buffer

  // put R_RX_PAYLOAD command into first index of tx_buffer
  *(tx_buffer + 0) = R_RX_PAYLOAD;

  // store byte(s) in tx_packet (tx_packet_ptr) into tx_buffer[]
  for (size_t i = 1; i < total_size; i++) { *(tx_buffer + i) = NOP; }
  
  csn_put_low(); // drive CSN pin LOW
  internal_status = spi_manager_transfer(tx_buffer, rx_buffer, total_size);
  csn_put_high(); // drive CSN pin HIGH
  
  // skip rx_buffer[0] (STATUS value) and transfer remaining values to buffer
  for (size_t i = 1; i < total_size; i++) { *(rx_packet_ptr++) = *(rx_buffer + i); }

  external_status_t external_status = (internal_status == OK) ? PASS : FAIL;

  spi_manager_deinit_spi(); // deinitialise SPI at function end

  return external_status;
}


external_status_t is_rx_packet(void) {

  spi_manager_init_spi(); // initialise SPI for function duration

  // NONE_ASSERTED (0), RX_DR_ASSERTED (1), TX_DS_ASSERTED (2), MAX_RT_ASSERTED (3)
  internal_status_irq_t status_irq = check_status_irq();

  // STATUS register RX_DR bit asserted when new data in RX FIFO
  external_status_t external_status = (status_irq == RX_DR_ASSERTED) ? PASS : FAIL;

  spi_manager_deinit_spi(); // deinitialise SPI at function end

  return external_status;
}


const is_response_t * const is_response(void) {

  static is_response_t response = { 
    .is_waiting = true,
    .is_acknowledge = false,
    .is_timeout = false
  };

  const is_response_t * const response_ptr = &response;

  // NONE_ASSERTED (0), RX_DR_ASSERTED (1), TX_DS_ASSERTED (2), MAX_RT_ASSERTED (3)
  internal_status_irq_t status_irq = check_status_irq();

  switch (status_irq)
  {
    case NONE_ASSERTED:
      response = (is_response_t){ .is_waiting = true, .is_acknowledge = false, .is_timeout = false };
    break;

    case TX_DS_ASSERTED:
      response = (is_response_t){ .is_waiting = false, .is_acknowledge = true, .is_timeout = false };
    break;

    case MAX_RT_ASSERTED:
      response = (is_response_t){ .is_waiting = false, .is_acknowledge = false, .is_timeout = true };
    break;

    default:

    break;
  }

  return response_ptr;
}


external_status_t set_tx_mode(void) {

  spi_manager_init_spi(); // initialise SPI for function duration

  // read CONFIG register value
  uint8_t config = r_register_byte(CONFIG);

  // is CONFIG register PRIM_RX bit set?
  uint8_t prim_rx = (config >> CONFIG_PRIM_RX) & SET_BIT;

  internal_status_t internal_status = OK;

  // PRIM_RX bit should be unset for TX Mode
  if (prim_rx == SET_BIT)
  {
    config &= ~(SET_BIT << CONFIG_PRIM_RX); // unset PRIM_RX bit

    internal_status = w_register(CONFIG, &config, ONE_BYTE);

    // NRF24L01+ enters TX Mode after 130us
    sleep_us(130);

    nrf_status.mode = 'T'; // reflect TX Mode in nrf_status
  }

  internal_status = (internal_status == OK) ? OK : TX_MODE_FAIL;

  ce_put_low(); // Drive CE LOW

  external_status_t external_status = (internal_status == OK) ? PASS : FAIL;

  spi_manager_deinit_spi(); // deinitialise SPI at function end

  return external_status;
}


external_status_t set_rx_mode(void) {

  spi_manager_init_spi(); // initialise SPI for function duration

  // read CONFIG register value
  uint8_t config = r_register_byte(CONFIG);

  // is CONFIG register PRIM_RX bit set?
  uint8_t prim_rx = (config >> CONFIG_PRIM_RX) & 1;

  internal_status_t internal_status = OK;

  // PRIM_RX bit should be set for RX Mode
  if (prim_rx != SET_BIT)
  {
    // set PRIM_RX bit in CONFIG register
    config |= (SET_BIT << CONFIG_PRIM_RX);

    // write set bit to CONFIG register
    internal_status = w_register(CONFIG, &config, ONE_BYTE);
  }

  // restore the RX_ADDR_P0 address, if exists
  if (nrf_status.is_rx_addr_p0)
  {
    w_register(RX_ADDR_P0, nrf_status.rx_addr_p0, FIVE_BYTES);
  }

  internal_status = (internal_status == OK) ? OK : RX_MODE_FAIL;

  // Drive CE HIGH
  ce_put_high();

  // NRF24L01+ enters RX Mode after 130us
  sleep_us(130);

  nrf_status.mode = 'R'; // reflect RX Mode in nrf_status

  external_status_t external_status = (internal_status == OK) ? PASS : FAIL;

  spi_manager_deinit_spi(); // deinitialise SPI at function end

  return external_status;
}

uint8_t debug_address(register_map_t reg) {

  spi_manager_init_spi(); // initialise SPI for function duration

  uint8_t value = r_register_byte(reg);

  spi_manager_deinit_spi(); // deinitialise SPI at function end
  return value;
}

void debug_address_bytes(register_map_t reg, uint8_t *buffer, uint8_t buffer_size) {

  spi_manager_init_spi(); // initialise SPI for function duration

  r_register_bytes(reg, buffer, buffer_size);

  spi_manager_deinit_spi(); // deinitialise SPI at function end

  return;
}


