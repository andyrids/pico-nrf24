# NRF24L01 driver for Raspberry Pi Pico and pico-sdk

A work in progress for an NRF24L01 driver for the Raspberry Pi Pico. The driver has now been rewritten, in an attempt to utilise a hardware proxy pattern, with a struct to encapsulate all access to the NRF24L01.  

**TODO**:  

- [X] Implement dynamic payloads
- [ ] Implement auto-acknowledgement with payloads
- [ ] Design optional IRQ pin with ISR implementation
- [ ] Design optional SPI data transfer using DMA (could be more performant)
- [ ] Explore Pico deep sleep implementation for power saving

## Introduction

The context for this project was learning C, the pico-sdk and how to work with the NRF24L01. Over time, I aim to investigate other possibilities, such as the implementation of data transfer over SPI using Pico DMA, Pico multicore and PIO features. Questions, issues and suggestions are welcome.  

## Connect The Pico to an NRF24L01

This table shows available connections between the NRF24L01 VCC, GND, CE and CSN pins and the Pico. Any spare pin on the Pico can be connected to the NRF24L01 CE and CSN pins.

| NRF24L01 | Pico      |
| -------- | --------- | 
| VCC      | 3V3 (OUT) |
| GND      | Any GND   | 
| CE       | Any GP    |
| CSN      | Any GP    |

This table shows the available pins for connecting the NRF24L01 SPI pins to each of the Pico's two SPI interfaces (SPI 0 & SPI 1).

| NRF24L01 | Pico SPI 0           | Pico SPI 1             |
| -------- | -------------------- | ---------------------- |
| SCK      | GP2, GP6, GP18, GP22 | GP10, GP14, GP26       |
| COPI     | GP3, GP7, GP19, GP23 | GP11, GP15, GP27       |
| CIPO     | GP0, GP4, GP16, GP20 | GP8, GP12, GP24, GP28  |  

## Structure

```
project_folder
├ examples <- examples using driver
├ lib
│ ├ CMakeLists.txt
│ └ nrf24l01 <- main driver folder
|   ├ error_manager
|   ├ pin_manager
|   ├ spi_manager
|   ├ CMakeLists.txt <- driver CMakeLists.txt
|   ├ device_config.h
|   ├ nrf24_driver.h <- driver interface header
|   └ nrf24_driver.c     
└ CMakeLists.txt <- main project CMakeLists.txt
```  

The `nrf24_driver.h` file provides the main interface, which combines functionality provided by static utility functions in other components (error_manager, pin_manager and spi_manager) to interact with the NRF24L01. `error_manager` is used for error handling, `pin_manager` provides utility functions to validate, initialise and drive GPIO pins high or low. `spi_manager` provides utility functions to initialise, format and deinitialise the Pico SPI interface. It also provides functions to serialize data to be sent over SPI to the NRF24L01. `device_config.h` contains the full register map for the NRF24L01 and defines specific register bit mnemonics that are useful for interfacing with the device over SPI.

## Configuration

The NRF24L01 is setup with the following default configuration:

- **RF channel:** 110
- **Air data rate:** 1Mbps
- **Power Amplifier:** 0dBm
- **Enhanced ShockBurst:** enabled
- **CRC:** enabled, 2 byte encoding scheme
- **Address width:** 5 bytes
- **Auto Retransmit Delay:** 250μS
- **Auto Retransmit Count:** 10
- **Dynamic payload:** disabled
- **Acknowledgment payload:** disabled 

## Background & Initial Setup 

All functions return a `fn_status_t` 'truthy' value of PIN_MNGR_OK (1), SPI_MNGR_OK (2), NRF_MNGR_OK (3) or a 'falsy' value of ERROR (0), to indicate the success of a particular operation. 

```C
// available through nrf24_driver.h
typedef enum fn_status_e
{
  ERROR,
  PIN_MNGR_OK,
  SPI_MNGR_OK,
  NRF_MNGR_OK
} fn_status_t;
```  

The driver interface file `nrf_driver.h` provides access to a number of structs, such as the `pin_manager_t`, `nrf_manager_t` and `nrf_client_t` structs. 

1- `pin_manager_t` stores CIPO, COPI, SCK, CSN and CE pin numbers:  

```C
// available through nrf24_driver.h
typedef struct pin_manager_s
{
  uint8_t copi;
  uint8_t cipo;
  uint8_t sck;
  uint8_t csn;
  uint8_t ce;
} pin_manager_t;
```

2- `nrf_manager_t` stores NRF24L01 register configuration settings, which can be used to initialise the device with different settings from the default configuration. Individual settings can also be changed through the driver interface functions (see [Optional Configurations](https://github.com/AndyRids/pico-nrf24#optional-configurations) section).  

```C
// available through nrf24_driver.h
typedef struct nrf_manager_s 
{
  /**
   * Address width in the SETUP_AW register
   * AW_3_BYTES, AW_4_BYTES, AW_5_BYTES
   */
  address_width_t address_width;

  /**
   * Dynamic payloads feature
   * DYNPD_ENABLE, DYNPD_DISABLE
   */
  dyn_payloads_t dyn_payloads;

  /**
   * Auto retransmission delay time
   * ARD_250US, ARD_500US, ARD_750US, ARD_1000US
   */
  retr_delay_t retr_delay;

  /**
   * Auto retransmission count
   * ARC_NONE, ARC_1RT...ARC_15RT
   */
  retr_count_t retr_count;

  /**
   * RF data rates
   * RF_DR_1MBPS, RF_DR_2MBPS, RF_DR_250KBPS
   */
  rf_data_rate_t data_rate;

  /**
   * TX Mode power level
   * RF_PWR_NEG_18DBM, RF_PWR_NEG_12DBM, RF_PWR_NEG_6DBM, RF_PWR_0DBM
   */
  rf_power_t power;

  // RF channel
  uint8_t channel;
} nrf_manager_t;
```

3- `nrf_client_t` provides access to all functions used to interact with the NRF24L01 and is initialized using the `nrf_driver_create_client` function. These functions will be covered in detail:

```C
// available through nrf24_driver.h
typedef struct nrf_client_s
{
  // configure user pins and the SPI interface
  fn_status_t (*configure)(pin_manager_t* user_pins, uint32_t baudrate_hz);

  // initialise the NRF24L01. A NULL argument will use default configuration.
  fn_status_t (*initialise)(nrf_manager_t* user_config);

  // set an address for a data pipe, a packet from another NRF24L01 will transmit to, for this NRF24L01
  fn_status_t (*rx_destination)(data_pipe_t data_pipe, const uint8_t *buffer);

  // set an address a packet from this NRF24L01 will transmit to, for a recipient NRF24L01
  fn_status_t (*tx_destination)(const uint8_t *buffer);

  // set payload size for received packets for one specific data pipe or for all
  fn_status_t (*payload_size)(data_pipe_t data_pipe, size_t size);

  // enables dynamic payloads
  fn_status_t (*dyn_payloads_enable)(void);

  // disables dynamic payloads
  fn_status_t (*dyn_payloads_disable)(void);

  // set packet auto-retransmission delay and count configurations
  fn_status_t (*auto_retransmission)(retr_delay_t delay, retr_count_t count);

  // set RF channel
  fn_status_t (*rf_channel)(uint8_t channel);

  // set the RF data rate
  fn_status_t (*rf_data_rate)(rf_data_rate_t data_rate);

  // set the RF power level, whilst in TX mode
  fn_status_t (*rf_power)(rf_power_t rf_power);

  // send a packet
  fn_status_t (*send_packet)(const void *tx_packet, size_t size);

  // read a received packet
  fn_status_t (*read_packet)(void *rx_packet, size_t size);

  // indicates if a packet has been received and is ready to read
  fn_status_t (*is_packet)(uint8_t *rx_p_no);

  // switch NRF24L01 to TX Mode
  fn_status_t (*standby_mode)(void);

  // switch NRF24L01 to RX Mode
  fn_status_t (*receiver_mode)(void);
} nrf_client_t;

/**
 * Takes the nrf_client_t argument and sets the function pointers
 * to the appropriate nrf_driver_[function_pointer_name] function.
 * 
 * available through nrf24_driver.h
 * 
 * @param client nrf_client_t struct
 * 
 * @return NRF_MNGR_OK (3), ERROR (0)
 */
fn_status_t nrf_driver_create_client(nrf_client_t *client);
```

### Initial Setup

1- An `nrf_client_t` should be declared and initialized though the `nrf_driver_create_client` function:

```C
#include "nrf24_driver.h"
#include "pico/stdlib.h"

nrf_client_t my_nrf;

nrf_driver_create_client(&my_nrf);
```

2- A `pin_manager_t` struct should be used, to store the CIPO, COPI, SCK, CSN and CE pin numbers and passed to the `configure` function along with an SPI baudrate in Hz. The `configure` function should be called first after the `nrf_client_t` is initialized:

```C
/**
 * Validate and configure GPIO pins and ascertain the 
 * correct SPI instance, if the pins are valid. Store 
 * GPIO and SPI details in the pin_manager_t user_pins
 * and spi_manager user_spi structs, within the global 
 * nrf_driver_t nrf_driver struct.
 * 
 * @note The function returns ERROR, if the pins were 
 * not valid, possibly due to one SPI pin using SPI 0 
 * interface and another using SPI 1.
 * 
 * @param user_pins pin_manager_t struct of pin numbers
 * @param baudrate_hz baudrate in Hz
 * 
 * @return PIN_MNGR_OK (1), ERROR (0)
 */
fn_status_t nrf_driver_configure(pin_manager_t *user_pins, uint32_t baudrate_hz);

// nrf_client_t access to nrf_driver_configure:

// set pin values
pin_manager_t my_pins = { .sck = 2, .copi = 3, .cipo = 4, .csn = 5, .ce = 6 };

// SPI baudrate in Hz
uint32_t my_baudrate = 1000000;

// configure GPIO pins and SPI
my_nrf.configure(&my_pins, my_baudrate);
```

3- The `initialise` function should be called next and only once, passing NULL to use the default NRF24L01 configuration or passing an `nrf_manager_t` struct to configure the device to your preferred settings.

```C
/**
 * Initialise NRF24L01 registers, leaving the device in 
 * Standby Mode. WiFi uses most of the lower channels and 
 * so, the highest 25 channels (100 - 124) are recommended 
 * for nRF24L01 projects.
 * 
 * Default configuration:
 * 
 * RF Channel: 110
 * Air data rate: 1Mbps
 * Power Amplifier: 0dBm
 * Enhanced ShockBurst: enabled
 * CRC: enabled, 2 byte encoding scheme
 * Address width: 5 bytes
 * Auto Retransmit Delay: 250μS
 * Auto Retransmit Count: 10
 * Dynamic payload: disabled
 * Acknowledgment payload: disabled
 * 
 * @param user_config nrf_manager_t config or NULL
 * 
 * @return SPI_MNGR_OK (2), ERROR (0)
 */
fn_status_t nrf_driver_initialise(nrf_manager_t *user_config);

// nrf_client_t access to nrf_driver_initialise:

// use default configuration
my_nrf.initialise(NULL);

// setup a specific configuration
nrf_manager_t my_config = {
  // RF Channel 
  .channel = 110,

  // AW_3_BYTES, AW_4_BYTES, AW_5_BYTES
  .address_width = AW_5_BYTES,

  // DYNPD_DISABLE, DYNPD_ENABLE
  .dyn_payloads = DYNPD_DISABLE,

  // RF_DR_250KBPS, RF_DR_1MBPS, RF_DR_2MBPS
  .data_rate = RF_DR_2MBPS,

  // RF_PWR_NEG_18DBM, RF_PWR_NEG_12DBM, RF_PWR_NEG_6DBM, RF_PWR_0DBM
  .power = RF_PWR_NEG_6DBM,

  // ARC_NONE...ARC_15RT
  .retr_count = ARC_13RT,

  // ARD_250US, ARD_500US, ARD_750US, ARD_1000US
  .retr_delay = ARD_750US 
};

// use specified configuration
my_nrf.initialise(&my_config);
```

4- The payload size for received packets can be set through the `payload_size` function for a specific data pipe or for all data pipes. The dynamic payload feature can be used instead of setting a static payload size. The `dyn_payloads_enable` function will enable dynamic payloads for all data pipes and `dyn_payloads_disable` will disable this feature: 

```C
// available through nrf24_driver.h
typedef enum data_pipe_e
{ 
  DATA_PIPE_0, DATA_PIPE_1, DATA_PIPE_2,
  DATA_PIPE_3, DATA_PIPE_4, DATA_PIPE_5,
  ALL_DATA_PIPES
} data_pipe_t;

/**
 * Set number of bytes in Tx payload for an individual data pipe or
 * for all data pipes, in the RX_PW_P0 - RX_PW_P5 registers.
 * 
 * data_pipe_t values:
 * 
 * DATA_PIPE_0...DATA_PIPE_5, ALL_DATA_PIPES (6)
 * 
 * @param data_pipe DATA_PIPE_0...DATA_PIPE_5 or ALL_DATA_PIPES
 * @param size bytes in payload
 * 
 * @return SPI_MNGR_OK (2), ERROR (0)
 */
fn_status_t nrf_driver_payload_size(data_pipe_t data_pipe, size_t size);

// nrf_client_t access to nrf_driver_payload_size:

// set the payload size for all data pipes to 5 bytes
my_nrf.payload_size(ALL_DATA_PIPES, FIVE_BYTES);



/**
 * Enables dynamic payloads, if not already.
 * 
 * @return SPI_MNGR_OK (2), NRF_MNGR_OK (3), ERROR (0)
 */
fn_status_t nrf_driver_dyn_payloads_enable(void);


/**
 * Disables dynamic payloads, if not already.
 * 
 * @return SPI_MNGR_OK (2), NRF_MNGR_OK (3), ERROR (0)
 */
fn_status_t nrf_driver_dyn_payloads_disable(void);

// nrf_client_t access to dyn_payloads_enable & dyn_payloads_disable:

my_nrf.dyn_payloads_enable(); // enabled

my_nrf.dyn_payloads_disable(); // disabled
```

5- If you alternate between RX Mode and TX Mode without a dedicated primary transmitter (PTX) and primary receiver (PRX) setup - then the `rx_destination` function should be used before setting the TX address through `tx_destination`. This allows the `rx_destination` function to cache the address for data pipe 0, which would be overwritten when using `tx_destination`. `rx_destination` sets an address to the specified data pipe.

The width of this address is determined by the address width setting. By default an address width of 5 bytes is used. Addresses for the data pipes are set with multiple `rx_destination` calls. Data pipes 2 - 5 use the remaining MSB (address width - 1 byte) of the data pipe 1 address and are set with a 1 byte address. 

If you want to use a 5 byte buffer for data pipes 2 - 5, then the function will automatically set one byte (buffer[0]). This might be useful to visually keep track of the full addresses set to each data pipe when coding. The NRF24L01 has data pipes 0 and 1 enabled by default, but if you set an address for data pipes 2 - 5, they will be enabled automatically by the function.

```C
/**
 * This function sets the address in buffer to the specified data pipe. 
 * Addresses for DATA_PIPE_0 (0), DATA_PIPE_1 (1) and the remaining data 
 * pipes can be set with multiple set_rx_address calls. Data pipes 2 - 5 
 * use the 4 MSB of data pipe 1 address and should be set with 1 byte. 
 * 
 * NOTE: If you do use a 5 byte buffer for data pipes 2 - 5, then the 
 * function will only write one byte (buffer[0]).
 * 
 * @param address (uint8_t[]){0x37, 0x37, 0x37, 0x37, 0x37} etc.
 * 
 * @return SPI_MNGR_OK (2), ERROR (0)
 */
fn_status_t nrf_driver_rx_destination(data_pipe_t data_pipe, const uint8_t *buffer);

// nrf_client_t access to nrf_driver_rx_destination:

// set DATA_PIPE_0 address
my_nrf.rx_destination(DATA_PIPE_0, (uint8_t[]){0x37, 0x37, 0x37, 0x37, 0x37});

// set DATA_PIPE_1 address
my_nrf.rx_destination(DATA_PIPE_1, (uint8_t[]){0xC7, 0xC7, 0xC7, 0xC7, 0xC7});

// full address: [0xC8] + DATA_PIPE_1 MSB [0xC7, 0xC7, 0xC7, 0xC7]
my_nrf.rx_destination(DATA_PIPE_2, (uint8_t[]){0xC8});

// full address: [0xC9] + DATA_PIPE_1 MSB [0xC7, 0xC7, 0xC7, 0xC7]
my_nrf.rx_destination(DATA_PIPE_3, (uint8_t[]){0xC9});

// only uses buffer[0] value (0xCA), but visually shows the full address
my_nrf.rx_destination(DATA_PIPE_4, (uint8_t[]){0xCA, 0xC7, 0xC7, 0xC7, 0xC7});
```  

6- The `tx_destination` function will set the destination data pipe address for a packet transmission, into the TX_ADDR register. This address must match one of the addresses a recipient NRF24L01 has set for each of its data pipes, through `rx_destination`.  

```C
/**
 * Set the destination address for a packet transmission, into the
 * TX_ADDR register.
 * 
 * The TX_ADDR register is used by a primary transmitter (PTX) for 
 * transmission of data packets and its value must be the same as 
 * an address in primary receiver (PRX) RX_ADDR_P0 - RX_ADDR_P5 
 * registers, for the PTX and PRX to communicate.
 * 
 * NOTE: For the auto-acknowledgement feature, the PTX must have its 
 * RX_ADDR_P0 register set with the same address as the TX_ADDR register. 
 * This driver uses auto-acknowledgement by default and this function 
 * writes the address to the the RX_ADDR_P0 by design.
 * 
 * @param address (uint8_t[]){0x37, 0x37, 0x37, 0x37, 0x37} etc.
 * 
 * @return SPI_MNGR_OK (2), ERROR (0)
 */
fn_status_t nrf_driver_tx_destination(const uint8_t *buffer);

// nrf_client_t access to nrf_driver_tx_destination:

// matches DATA_PIPE_1 address in example above
my_nrf.tx_destination((uint8_t []){0xC7, 0xC7, 0xC7, 0xC7, 0xC7});

/**
 * matches DATA_PIPE_4 address in example above, because its 
 * DATA_PIPE_1 address was set to {0xC7, 0xC7, 0xC7, 0xC7, 0xC7} 
 * and its DATA_PIPE_4 was set with one byte, to {0xCA}. Therefore,
 * its full DATA_PIPE_4 address is {0xCA, 0xC7, 0xC7, 0xC7, 0xC7}.
 */
my_nrf.tx_destination((uint8_t[]){0xCA, 0xC7, 0xC7, 0xC7, 0xC7});
```  

7- The `standby_mode` and `receiver_mode` functions switch the NRF24L01 between Standby-I mode and RX Mode. Regarding TX Mode, technically the device is only in this mode when the CONFIG register PRIM_RX bit is unset (0) **AND** the CE pin is HIGH for more than 10μs **AND** the TX_FIFO is not empty **AND** then after 130μs have passed. `standby_mode` function will also reset the PRIM_RX bit in preparation for TX Mode. `send_packet` function will transfer a packet to the TX FIFO (TX FIFO not empty) and will drive the CE pin HIGH, transitioning to TX Mode state and resulting in packet transmission. After packet transmission, the CE pin will be LOW with one packet having been transmitted, reverting back to Standby-I mode.

```C
/**
 * Puts the NRF24L01 into Standby-I Mode. Resets the CONFIG 
 * register PRIM_RX bit value, in preparation for entering
 * TX Mode and drives CE pin LOW.
 * 
 * NOTE: State diagram in the datasheet (6.1.1) highlights
 * conditions for entering RX and TX operating modes. One 
 * condition is the value of PRIM_RX (bit 0) in the CONFIG
 * register. PRIM_RX = 1 for RX Mode or 0 for TX Mode. CE
 * pin is driven HIGH for Rx mode and is only driven high 
 * in TX Mode to facilitate the Tx of data (10us+).
 * 
 * @return SPI_MNGR_OK (2), ERROR (0)
 */
fn_status_t nrf_driver_standby_mode(void);

/**
 * Puts the NRF24L01 into RX Mode. Sets the CONFIG register 
 * PRIM_RX bit value and drive CE pin HIGH.
 * 
 * NOTE: State diagram in the datasheet (6.1.1) highlights
 * conditions for entering Rx and Tx operating modes. One 
 * condition is the value of PRIM_RX (bit 0) in the CONFIG
 * register. PRIM_RX = 1 for Rx mode or 0 for Tx mode. CE
 * pin is driven HIGH for Rx mode and is only driven high 
 * in Tx mode to facilitate the Tx of data (10us+).
 * 
 * @return SPI_MNGR_OK (2), ERROR (0)
 */
fn_status_t nrf_driver_receiver_mode(void);

// nrf_client_t access to nrf_driver_standby_mode & nrf_driver_receiver_mode:

// now in Standby-I Mode
my_nrf.standby_mode(); 

// now in RX Mode
my_nrf.receiver_mode(); 
```  

### Transmitting A Packet

1- Make sure that:

   - Correct TX destination address has been set - `tx_destination`.
   - Dynamic payloads enabled (if enabled on receiver) - `dyn_payloads_enable`.
   - Device is in Standby-I mode - `standby_mode`.  

```C
// matches DATA_PIPE_1 address of recipient in previous example
my_nrf.tx_destination((uint8_t[]){0xC7, 0xC7, 0xC7, 0xC7, 0xC7});

// if receiver has dynamic payloads enabled, then the transmitter must also
my_nrf.dyn_payloads_enable();

// set to Standby-I mode, in preparation for packet transmission
my_nrf.standby_mode();
```

2- The `send_packet` function transmits a payload to a recipient NRF24L01 and will return SPI_MNGR_OK (2) if the transmission was successful and an auto-acknowledgement was received from the recipient NRF24L01. A return value of ERROR (0) indicates that either, the packet transmission failed, or no auto-acknowledgement was received before max retransmissions count was reached. If the receiver has enabled dynamic payloads then the transmitting device must also, through the `dyn_payloads_enable` function or through an initial nrf_manager_t config passed  to the `initialise` function. If dynamic payloads are not being used, then the receiver need only set the correct static size of the transmitted payloads through the `payload_size` function.

```C
/**
 * Transmits a payload to a recipient NRF24L01 and will return 
 * SPI_MNGR_OK (2) if the transmission was successful and an 
 * auto-acknowledgement was received from the recipient NRF24L01. 
 * A return value of ERROR (0) indicates that either, the packet 
 * transmission failed, or no auto-acknowledgement was received 
 * before max retransmissions count was reached.
 * 
 * @param tx_packet packet for transmission
 * @param size size of tx_packet
 * 
 * @return SPI_MNGR_OK (2), ERROR (0)
 */
fn_status_t nrf_driver_send_packet(const void *tx_packet, size_t size);

// nrf_client_t access to nrf_driver_send_packet:

// unsigned char packet
uint8_t my_packet = 123;

// transmit packet (one byte)
uint8_t result = my_nrf.send_packet(&my_packet, sizeof(my_packet));

if (result)
{
  // transmitted successfully, with auto-acknowledgement from receiver
}

// string packet 
uint8_t my_packet[] = "Hello";

// transmit packet (five bytes)
if (my_nrf.send_packet(my_packet, sizeof(my_packet)))
{
  // transmitted successfully, with auto-acknowledgement from receiver
}
```  

### Receiving A Packet

1- Make sure that:

   - Correct RX destination address has been set - `rx_destination`.
   - Correct payload size for its data pipes - `payload_size`
   - Device is in RX Mode - `receiver_mode`.   

```C
// set DATA_PIPE_0 address
my_nrf.rx_destination(DATA_PIPE_0, (uint8_t[]){0x37, 0x37, 0x37, 0x37, 0x37});

// set DATA_PIPE_1 address
my_nrf.rx_destination(DATA_PIPE_1, (uint8_t[]){0xC7, 0xC7, 0xC7, 0xC7, 0xC7});

// set payload size for ALL_DATA_PIPES (6), to FIVE_BYTES (5)
my_nrf.payload_size(ALL_DATA_PIPES, FIVE_BYTES);

// set to RX Mode
my_nrf.receiver_mode();
```

2- The `is_packet` function polls the STATUS register to ascertain if there is a packet in the RX FIFO. The function will return NRF_MNGR_OK (3) if there is a packet available to read, or ERROR (0) if not. If you want to store the data pipe number the packet was addressed to, call the function with the address of a uint8_t variable. If this detail is not relevant to your program, call `is_packet` with a NULL argument.

```C
/**
 * Polls the STATUS register to ascertain if there is a packet 
 * in the RX FIFO. The function will return PASS (1) if there 
 * is a packet available to read, or FAIL (0) if not. Calling
 * the function with NULL as the rx_p_no argument, will allow
 * you to disregard received pipe number information.
 * 
 * @param rx_p_no pipe number packet received on or NULL
 * 
 * @return PASS (1), FAIL (0)
 */
fn_status_t nrf_driver_is_packet(uint8_t *rx_p_no);

// nrf_client_t access to nrf_driver_is_packet:

// don't need pipe number
if (my_nrf.is_packet(NULL))
{
  // ....
}

// holds data pipe packet was addressed to
uint8_t pipe_no = 0;

// work with pipe number if packet received
if (my_nrf.is_packet(&pipe_no))
{
  // ....
}
```  

3- The `read_packet` function is used to read an available packet from the RX FIFO into the provided buffer.  

```C
/**
 * Read an available packet from the RX FIFO into the 
 * buffer (rx_packet).
 * 
 * @param rx_packet packet buffer for receipt
 * @param size size of buffer
 * 
 * @return SPI_MNGR_OK (2), ERROR (0)
 */
fn_status_t nrf_driver_read_packet(void *rx_packet, size_t size);

// nrf_client_t access to nrf_driver_read_packet:

// holds data pipe packet was addressed to
uint8_t pipe_no = 0;

// hold received packet data
uint8_t payload = 0;

// work with pipe number if packet received
if (my_nrf.is_packet(&pipe_no))
{
  // read payload value into payload variable
  my_nrf.read_packet(&payload, sizeof(payload));

  // print payload value and pipe number
  printf("Packet received:- Payload (%d) on data pipe (%d)\n", payload, pipe_no);
}
```  

### Optional Configurations

Aside from using a `nrf_manager` struct to initialise the NRF24L01 with different configuration settings through the `initialise` function, individual settings can be changed within a program through the following functions:

1- the `rf_channel` function sets the RF channel for the device. Devices must be using the same channel in order to communicate with one another. 

According to Nordic support:  

> The nRF24L01+ can only change frequency in standby mode. It takes 130μS from standby to active mode (RX or TX) regardless of whether you change frequency or not...

Therefore, if in RX Mode, switch to Standby-I mode through the `standby_mode` function before calling `rf_channel`.

```C
/**
 * Set the RF channel. Each device must be on the same 
 * channel in order to communicate.
 * 
 * @param channel RF channel 1..125
 * 
 * @return SPI_MNGR_OK (2), ERROR (0)
 */
fn_status_t nrf_driver_rf_channel(uint8_t channel);

// nrf_client_t access to nrf_driver_rf_channel:

// set RF channel to 120
my_nrf.rf_channel(120);

```

2- The `auto_retransmission` function will enable you to set the automatic retransmission delay (ARD) and automatic retransmission count (ARC) settings. Enhanced ShockBurst™ automatically retransmits the original data packet after a programmable delay (the ARD), a max number of times (the ARC), if an auto-acknowledgement is not received from the intended recipient of the packet.  

```C
// available through nrf24_driver.h
typedef enum retr_delay_e
{
  // ARD of 250μS
  ARD_250US = (0x00 << SETUP_RETR_ARD), 

  // ARD of 500μS
  ARD_500US = (0x01 << SETUP_RETR_ARD), 

  // ARD of 750μS
  ARD_750US = (0x02 << SETUP_RETR_ARD),

  // ARD 1000μS 
  ARD_1000US = (0x03 << SETUP_RETR_ARD) 
} retr_delay_t;

// available through nrf24_driver.h
typedef enum retr_count_e 
{
  // ARC disabled
  ARC_NONE = (0x00 << SETUP_RETR_ARC), 

  // ARC of 1
  ARC_1RT = (0x01 << SETUP_RETR_ARC),

  // ARC of 2
  ARC_2RT = (0x02 << SETUP_RETR_ARC),

  // ARC of 3
  ARC_3RT = (0x03 << SETUP_RETR_ARC),

  // etc... up to ARC_15RT 
  ARC_15RT = (0x0F << SETUP_RETR_ARC)
} retr_count_t;

/**
 * Set Auto Retransmit Delay (ARD) and Auto Retransmit Count (ARC) 
 * in the SETUP_RETR register. The datasheet states that the delay
 * is defined from end of the transmission to start of the next 
 * transmission.
 * 
 * NOTE: ARD is the time the PTX is waiting for an ACK packet before 
 * a retransmit is made. The PTX is in RX Mode for 250μS (500μS in 
 * 250kbps mode) to wait for address match. If the address match is 
 * detected, it stays in RX mode to the end of the packet, unless 
 * ARD elapses. Then it goes to standby-II mode for the rest of the
 * specified ARD. After the ARD, it goes to TX mode and retransmits 
 * the packet.
 * 
 * @param delay ARD_250US, ARD_500US, ARD_750US, ARD_1000US
 * @param count ARC_NONE, ARC_1RT...ARC_15RT
 * 
 * @return SPI_MNGR_OK (2), ERROR (0)
 */
fn_status_t nrf_driver_auto_retransmission(retr_delay_t delay, retr_count_t count);

// nrf_client_t access to nrf_driver_auto_retransmission:

// ARD 500μS & ARC 10
my_nrf.auto_retransmission(ARD_500US, ARC_10RT);

// ARD 750μS & ARC 13
my_nrf.auto_retransmission(ARD_750US, ARC_13RT);
```  

3- The `rf_data_rate` function sets the air data rate for the NRF24L01, to 1 Mbps, 2 Mbps or 250 kbps.  

```C
// available through nrf24_driver.h
typedef enum rf_data_rate_e 
{
  // 1 Mbps
  RF_DR_1MBPS = ((0x00 << RF_SETUP_RF_DR_LOW) | (0x00 << RF_SETUP_RF_DR_HIGH)),

  // 2 Mbps
  RF_DR_2MBPS = ((0x00 << RF_SETUP_RF_DR_LOW) | (0x01 << RF_SETUP_RF_DR_HIGH)),

  // 250 kbps
  RF_DR_250KBPS = ((0x01 << RF_SETUP_RF_DR_LOW) | (0x00 << RF_SETUP_RF_DR_HIGH)),
} rf_data_rate_t; 

/**
 * Set the RF data rates in the RF_SETUP register
 * through the RF_DR_LOW and RF_DR_HIGH bits.
 * 
 * @param data_rate RF_DR_1MBPS, RF_DR_2MBPS, RF_DR_250KBPS
 * 
 * @return SPI_MNGR_OK (2), ERROR (0)
 */
fn_status_t nrf_driver_rf_data_rate(rf_data_rate_t data_rate)

// nrf_client_t access to nrf_driver_rf_data_rate:

// 1 Mbps
my_nrf.rf_data_rate(RF_DR_1MBPS); 

// 250 kbps
my_nrf.rf_data_rate(RF_DR_250KBPS);
```  

4- The `rf_power` function sets the TX Mode power level. If your devices are close together, such as when testing, a lower power level is best. The default setting is `RF_PWR_0DBM`.  

```C
// available through nrf24_driver.h
typedef enum rf_power_e
{
  // -18dBm Tx output power
  RF_PWR_NEG_18DBM = (0x00 << RF_SETUP_RF_PWR), 

  // -12dBm Tx output power
  RF_PWR_NEG_12DBM = (0x01 << RF_SETUP_RF_PWR), 

  // -6dBm Tx output power
  RF_PWR_NEG_6DBM = (0x02 << RF_SETUP_RF_PWR), 

  // 0dBm Tx output power
  RF_PWR_0DBM = (0x03 << RF_SETUP_RF_PWR) 
} rf_power_t;  

/**
 * Set TX Mode power level in RF_SETUP register through
 * the RF_PWR bits.
 * 
 * @param rf_pwr RF_PWR_NEG_18DBM, RF_PWR_NEG_12DBM, RF_PWR_NEG_6DBM, RF_PWR_0DBM
 * 
 * @return SPI_MNGR_OK (2), ERROR (0)
 */
fn_status_t nrf_driver_rf_power(rf_power_t rf_pwr);

// nrf_client_t access to nrf_driver_rf_power:

// 0dBm Tx output power
my_nrf.rf_power(RF_PWR_0DBM);

// -18dBm Tx output power
my_nrf.rf_power(RF_PWR_NEG_18DBM); 
```  

## Acknowledgments

When I first started learning C and the pico-sdk, I couldn't find many examples of using the NRF24L01 and the Pico. Videos from [@guser210](https://github.com/guser210) on his [YouTube](https://youtu.be/V4ziwen24Ps) channel and the code shared in his [guser210/NRFDemo](https://github.com/guser210/NRFDemo) repository were instrumental in getting started on the right path.

The excellent multi-platform library [nRF24/RF24](https://github.com/nRF24/RF24) now supports the Pico and the pico-sdk. I learnt a lot from looking through this library.

GitHub repositories from [@TMRh20](https://github.com/TMRh20) and his [Project Blog](https://tmrh20.blogspot.com/2014/03/igh-speed-data-transfers-and-wireless.html), which details using the full potential of NRF24L01 radio modules.

This [Enhanced source file handling with target_sources()](https://crascit.com/2016/01/31/enhanced-source-file-handling-with-target_sources/) article, written by Craig Scott (a CMake co-maintainer) was very helpful in learning different ways of setting up a CMake project.

## Equipment Used

- [Raspberry Pi Pico](https://www.raspberrypi.com/products/raspberry-pi-pico/)
- [Cytron Maker Pi Pico](https://www.cytron.io/p-maker-pi-pico)
- [WaveShare NRF24L01 RF Board (B)](https://www.waveshare.com/nrf24l01-rf-board-b.htm)