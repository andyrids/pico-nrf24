# NRF24L01 driver for Raspberry Pi Pico and pico-sdk

A work in progress for an NRF24L01 driver for the Raspberry Pi Pico.  

**TODO**:  

- [ ] Further testing & debugging
- [ ] Implement dynamic payloads
- [ ] Implement auto-acknowledgement with payloads
- [ ] Design optional IRQ pin with ISR implementation
- [ ] Design optional SPI data transfer using DMA (could be more performant)
- [ ] Explore Pico deep sleep implementation for power saving

## Introduction

The context for this project was learning C and the pico-sdk and how to work with the NRF24L01. Over time, I aim to investigate other possibilities, such as the implementation of data transfer over SPI using Pico DMA, Pico multicore and PIO features.  

## Connect The Pico to NRF24L01

This table shows available connections between the NRF24L01 VCC, GND, CE and CSN pins and the Pico. Any spare pin on the Pico can be 
connected to the NRF24L01 CE and CSN pins.

| NRF24L01 | Pico      |
| -------- | --------- | 
| VCC      | 3V3 (OUT) |
| GND      | Any GND   | 
| CE       | Any GP    |
| CSN      | Any GP    |

This table shows the available pins for connecting the NRF24L01 SPI pins to each of the Pico's two SPI interfaces (SPI 0 & SPI 1).

| NRF24L01 | Pico SPI 0           | Pico SPI 1       |
| -------- | -------------------- | ---------------- |
| SCK      | GP2, GP6, GP18, GP22 | GP10, GP14, GP26 |
| COPI     | GP3, GP7, GP19, GP23 | GP11, GP15, GP27 |
| CIPO     | GP0, GP4, GP16, GP20 | GP8, GP12, GP24  |

## Structure

```
project_folder
├ examples <- examples using driver
├ lib
│ ├ CMakeLists.txt <- adds subdirectory for folders in lib
│ └ nrf24l01
|   ├ error_manager
|   ├ pin_manager
|   ├ spi_manager
|   ├ CMakeLists.txt <- driver CMakeLists.txt
|   ├ device_config.h
|   ├ nrf24_driver.h <- driver interface header
|   └ nrf24_driver.c     
├ test
└ CMakeLists.txt <- main project CMakeLists.txt
```

The `nrf24_driver.h` file provides the main interface, which uses utility functions from other components (error_manager, pin_manager and spi_manager) to interact with the NRF24L01. `device_config.h` contains the full register map for the nRF24L01 and defines specific
register bit mnemonics that are useful for interfacing with the NRF24L01P over SPI.

## Configuration

The NRF24L01 will be setup with the following configuration, by default:

- **Air data rate:** 1Mbps
- **Power Amplifier:** 0dBm
- **Enhanced ShockBurst:** enabled
- **CRC:** enabled, 2 byte encoding scheme
- **Address width:** 5 bytes
- **Auto Retransmit Delay:** 500μS
- **Auto Retransmit Count:** 10
- **Dynamic payload:** disabled
- **Acknowledgment payload:** disabled 

All public functions in `nrf24_driver.h` return an `external_status_t` value of either PASS (1) or FAIL (0), indicating the success of the operation.  

```C
// available through nrf24_driver.h
typedef enum external_status_e  
{ 
  FAIL, 
  PASS 
} external_status_t;
```  

### Initial Setup 

1. The `init_pico` function will initialise all GPIO pins (pin_manager/pin_manager.c) and set the correct SPI interface, baudrate and format for reading/writing to the NRF24L01 (spi_manager/spi_manager.c). This function will return FAIL (0), if any of the pin numbers are incorrect. For example, the function will check to make sure that you haven't used a CIPO pin on SPI 0 interface and a COPI pin on the SPI 1 interface (spi_manager/spi_manager.c). **NOTE:** The highest baudrate I can get working correctly is 7000000Hz. I'm not Not sure why.

```C
external_status_t init_pico(
  uint8_t cipo_pin, 
  uint8_t copi_pin, 
  uint8_t csn_pin, 
  uint8_t sck_pin, 
  uint8_t ce_pin, 
  uint32_t baudrate_hz
);

// example use
init_pico(0, 3, 4, 2, 5, 6000000)
```  

2. The `init_nrf24` function will initialise the NRF24L01, using the default configuration listed above and will also set the RF channel. RF channel values must match among your different NRF24L01 devices.  

```C
external_status_t init_nrf24(uint8_t rf_channel);

// example use
init_nrf24(76);
```  

3. The `set_payload_size` function sets the payload size in bytes for the specified data pipe (0 - 5) or for all data pipes, if the value 6 is used. The `data_pipe_t` values are available after including the `nrf24_driver.h` header file.  

```C
// available through nrf24_driver.h
typedef enum data_pipe_e
{ 
  DATA_PIPE_0, DATA_PIPE_1, DATA_PIPE_2, DATA_PIPE_3, DATA_PIPE_4, DATA_PIPE_5, ALL_DATA_PIPES
} data_pipe_t;

external_status_t set_payload_size(data_pipe_t data_pipe, uint8_t payload_width);

// example use
uint8_t payload = 100;
set_payload_size(ALL_DATA_PIPES, sizeof(payload));
```  

4. If you plan to alternate an NRF24L01 device between RX Mode and TX Mode, rather than have dedicated primary transmitter and primary receiver setup - then the `set_rx_address` function should be used next, before setting the TX address (set_tx_address). This function sets the 5 byte address to the specified data pipe. Addresses for `DATA_PIPE_0` (0), `DATA_PIPE_1` (1) and the remaining data pipes can be set with multiple `set_rx_address` calls. Data pipes 2 - 5 use the 4 MSB of data pipe 1 address and should be set with a 1 byte address. If you want to use a 5 byte buffer for data pipes 2 - 5, then the function will automatically set one byte (buffer[0]). This might be useful to visually keep track of the full addresses set to each data pipe when coding. The NRF24L01 has data pipes 0 and 1 enabled by default, but if you set an address for data pipes 2 - 5, they will be enabled automatically.

```C
external_status_t set_rx_address(data_pipe_t data_pipe, const uint8_t *buffer);

// example use
set_rx_address(DATA_PIPE_0, (uint8_t []){0x37, 0x37, 0x37, 0x37, 0x37});
set_rx_address(DATA_PIPE_1, (uint8_t []){0xC7, 0xC7, 0xC7, 0xC7, 0xC7});
set_rx_address(DATA_PIPE_2, (uint8_t []){0xC8}); // 0xC8 + 0xC7, 0xC7, 0xC7, 0xC7
set_rx_address(DATA_PIPE_3, (uint8_t []){0xC9}); // 0xC9 + 0xC7, 0xC7, 0xC7, 0xC7

// only writes buffer[0] value to RX_ADD_P3 register as DATA_PIPE_3 is set with 1 byte
set_rx_address(DATA_PIPE_3, (uint8_t []){0xC9, 0xC9, 0xC9, 0xC9, 0xC9});
```  

5. The `set_tx_address` function will set the destination data pipe address for a packet transmission, into the TX_ADDR register. This address must match one of the addresses a recipient NRF24L01 has set for each of its data pipes (set_rx_address).  

```C
external_status_t set_tx_address(const uint8_t *buffer);

// example use
set_tx_address((uint8_t []){0xC7, 0xC7, 0xC7, 0xC7, 0xC7}); // matches DATA_PIPE_1 address in example above
set_tx_address((uint8_t []){0xC8, 0xC7, 0xC7, 0xC7, 0xC7}); // matches DATA_PIPE_2 address in example above (0xC8 + 4 MSB from DATA_PIPE_1)
```  

6. The `set_tx_mode` and `set_rx_mode` functions switch the NRF24L01 between TX Mode and RX Mode.  

```C
external_status_t set_tx_mode(void);

external_status_t set_rx_mode(void);

// example use
set_tx_mode(); // now in TX Mode
set_rx_mode(); // now in RX Mode
```  

### Transmitting A Packet

1. The `tx_packet` function transmits a payload to a recipient NRF24L01 and will return PASS (1) if the transmission was successful and an auto-acknowledgement was received from the recipient NRF24L01. A return value of FAIL (0) indicates that either, the packet transmission failed, or no auto-acknowledgement was received before max retransmissions count was reached.  

```C
external_status_t tx_packet(const void *tx_packet, size_t packet_size);
```  

2. Make sure the correct TX address has been set (set_tx_address), the device is in TX Mode (set_tx_mode) and the appropriate payload size has been set (set_payload_size) during the initialization of the recipient NRF24L01. The `examples` folder contains an example of an NRF24L01 configured as a primary transmitter (examples/primary_transmitter/).  

```C
set_tx_address((uint8_t []){0xC7, 0xC7, 0xC7, 0xC7, 0xC7}); // matches DATA_PIPE_1 address of recipient

set_tx_mode(); // now in TX Mode

uint8_t payload = 100; // data to transmit

// response = PASS (1) || FAIL (0)
uint8_t response = tx_packet(&payload, sizeof(payload));
```  

### Receiving A Packet

1. The `is_rx_packet` function polls the STATUS register to ascertain if there is a packet in the RX FIFO. The function will return PASS (1) if there is a packet available to read, or FAIL (0) if not.  

```C
external_status_t is_rx_packet(void);

// example use
if (is_rx_packet())
{
  // ....
}
```  

2. The `rx_packet` function is used to read an available packet from the RX FIFO into the provided buffer (rx_packet).  

```C
external_status_t rx_packet(void *rx_packet, size_t packet_size);

// example use
uint8_t message = 0; // buffer to receive payload value

rx_packet(&message, sizeof(message)); // read payload value from RX FIFO into buffer

printf("Message received:- Payload: %d\n", message);
```  

3. Make sure a valid destination address is set for the transmitting NRF24L01. The transmitting NRF24L01 TX address should match one of the addresses set through the `set_rx_address` function, on this recipient device. Set the device to RX Mode (set_rx_mode) and use `is_rx_packet` to check if a packet has been successfully received. If so, read the packet through the `rx_packet` function. The `examples` folder contains an example of an NRF24L01 configured as a primary receiver (examples/primary_receiver/).

```C
// example
set_rx_address(DATA_PIPE_0, (uint8_t []){0x37, 0x37, 0x37, 0x37, 0x37}); // Transmitting NRF24L01 TX address should match this address...
set_rx_address(DATA_PIPE_1, (uint8_t []){0xC7, 0xC7, 0xC7, 0xC7, 0xC7}); // ...or this address

set_rx_mode(); // now in RX Mode

uint8_t message = 0; // buffer to receive payload value

// permanent loop
while (1) { 
  // check for packet in RX FIFO
  if (is_rx_packet()) {

    // read payload value into buffer
    rx_packet(&message, sizeof(message));

    // print message
    printf("Message received:- Payload: %d\n", message);
  }
}
```  

### Optional Configurations

1. The `set_auto_retransmission` function will enable you to set the automatic retransmission delay (ARD) and automatic retransmission count (ARC) settings. Enhanced ShockBurst™ automatically retransmits the original data packet after a programmable delay (the ARD), a max number of times (the ARC), if an auto-acknowledgement is not received from the intended recipient of the packet.  

```C
// available through nrf24_driver.h
typedef enum retr_delay_e
{
  // Automatic retransmission delay of 250μS
  ARD_250US = (0x00 << SETUP_RETR_ARD), 

  // Automatic retransmission delay of 500μS
  ARD_500US = (0x01 << SETUP_RETR_ARD), 

  // Automatic retransmission delay of 750μS
  ARD_750US = (0x02 << SETUP_RETR_ARD),

  // Automatic retransmission delay of 1000μS 
  ARD_1000US = (0x03 << SETUP_RETR_ARD) 
} retr_delay_t;

typedef enum retr_count_e // available through nrf24_driver.h
{
  ARC_NONE = (0x00 << SETUP_RETR_ARC), // ARC disabled
  ARC_1RT = (0x01 << SETUP_RETR_ARC), // ARC of 1
  ARC_2RT = (0x02 << SETUP_RETR_ARC), // ARC of 2
  ARC_3RT = (0x03 << SETUP_RETR_ARC), // ARC of 3
  // etc... up to ARC_15RT 
  ARC_15RT = (0x0F << SETUP_RETR_ARC) // ARC of 15
} retr_count_t;

external_status_t set_auto_retransmission(retr_delay_t retr_delay, retr_count_t retr_count);

// example use
set_auto_retransmission(ARD_500US, ARC_10RT); // ARD 500μS & ARC 10
set_auto_retransmission(ARD_750US, ARC_13RT); // ARD 750μS & ARC 13
```  

2. The `set_rf_data_rate` function sets the air data rate for the NRF24L01, to 1 Mbps, 2 Mbps or 250 kbps.  

```C
// available through nrf24_driver.h
typedef enum rf_data_rate_e 
{
   // 1 Mbps
  RF_DR_1MBPS   = ((0x00 << RF_SETUP_RF_DR_LOW) | (0x00 << RF_SETUP_RF_DR_HIGH)),

   // 2 Mbps
  RF_DR_2MBPS   = ((0x00 << RF_SETUP_RF_DR_LOW) | (0x01 << RF_SETUP_RF_DR_HIGH)),

  // 250 kbps
  RF_DR_250KBPS = ((0x01 << RF_SETUP_RF_DR_LOW) | (0x00 << RF_SETUP_RF_DR_HIGH)),
} rf_data_rate_t; 

external_status_t set_rf_data_rate(rf_data_rate_t data_rate);

// example use
set_rf_data_rate(RF_DR_1MBPS); // 1 Mbps
set_rf_data_rate(RF_DR_250KBPS); // 250 kbps
```  

3. The `set_rf_power` function sets the TX Mode power level. If your devices are close together, such as when testing, a lower power level is best. The default setting is `RF_PWR_0DBM`.  

```C
// available through nrf24_driver.h
typedef enum rf_power_e
{
  // -18dBm Tx output power
  RF_PWR_NEG_18DBM = (0x00 << RF_SETUP_RF_PWR), 

  // -12dBm Tx output power
  RF_PWR_NEG_12DBM = (0x01 << RF_SETUP_RF_PWR), 

  // -6dBm Tx output power
  RF_PWR_NEG_6DBM  = (0x02 << RF_SETUP_RF_PWR), 

  // 0dBm Tx output power
  RF_PWR_0DBM  = (0x03 << RF_SETUP_RF_PWR) 
} rf_power_t;  

external_status_t set_rf_power(rf_power_t rf_pwr);  

// example use
set_rf_power(RF_PWR_0DBM); // 0dBm Tx output power
set_rf_power(RF_PWR_NEG_18DBM); // -18dBm Tx output power
```  

## Acknowledgments

When I first started learning C and the pico-sdk, I couldn't find many examples of using the NRF24L01 and the Pico. Videos from [@guser210](https://github.com/guser210) on his [YouTube](https://youtu.be/V4ziwen24Ps) channel and the code shared in his [guser210/NRFDemo](https://github.com/guser210/NRFDemo) repository were instrumental in getting started on the right path.

The excellent multi-platform library [nRF24/RF24](https://github.com/nRF24/RF24) now supports the Pico and the pico-sdk. I learnt a lot from looking through this library.

This [Enhanced source file handling with target_sources()](https://crascit.com/2016/01/31/enhanced-source-file-handling-with-target_sources/) article, written by Craig Scott (a CMake co-maintainer) was very helpful in learning different ways of setting up a CMake project.