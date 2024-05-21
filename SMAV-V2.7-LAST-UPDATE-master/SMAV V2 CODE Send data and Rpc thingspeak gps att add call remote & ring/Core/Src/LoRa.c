// Copyright (c) Konstantin Belyalov. All rights reserved.
// Licensed under the MIT license.
#include <stdio.h>
#include "LoRa.h"

// sx1276 registers
// registers
#define REG_FIFO                 		0x00
#define REG_OP_MODE              		0x01
#define REG_FRF_MSB              		0x06
#define REG_FRF_MID              		0x07
#define REG_FRF_LSB              		0x08
#define REG_PA_CONFIG            		0x09
#define REG_OCP                  		0x0b
#define REG_LNA                  		0x0c
#define REG_FIFO_ADDR_PTR        		0x0d
#define REG_FIFO_TX_BASE_ADDR    		0x0e
#define REG_FIFO_RX_BASE_ADDR    		0x0f
#define REG_FIFO_RX_CURRENT_ADDR 		0x10
#define REG_IRQ_FLAGS            		0x12
#define REG_RX_NB_BYTES          		0x13
#define REG_PKT_SNR_VALUE        		0x19
#define REG_PKT_RSSI_VALUE       		0x1a
#define REG_RSSI_VALUE           		0x1b
#define REG_MODEM_CONFIG_1       		0x1d
#define REG_MODEM_CONFIG_2       		0x1e
#define REG_SYMB_TIMEOUT_LSB     		0x1f
#define REG_PREAMBLE_MSB         		0x20
#define REG_PREAMBLE_LSB         		0x21
#define REG_PAYLOAD_LENGTH       		0x22
#define REG_MODEM_CONFIG_3       		0x26
#define REG_FREQ_ERROR_MSB       		0x28
#define REG_FREQ_ERROR_MID       		0x29
#define REG_FREQ_ERROR_LSB       		0x2a
#define REG_RSSI_WIDEBAND        		0x2c
#define REG_DETECTION_OPTIMIZE   		0x31
#define REG_INVERTIQ             		0x33
#define REG_DETECTION_THRESHOLD  		0x37
#define REG_SYNC_WORD            		0x39
#define REG_INVERTIQ2            		0x3b
#define REG_DIO_MAPPING_1        		0x40
#define REG_VERSION              		0x42
#define REG_PA_DAC               		0x4d

// modes
#define MODE_LONG_RANGE_MODE     		0x80
#define MODE_SLEEP               		0x00
#define MODE_STDBY               		0x01
#define MODE_TX                  		0x03
#define MODE_RX_CONTINUOUS       		0x05
#define MODE_RX_SINGLE           		0x06
#define MODE_CAD                 		0x07

// PA config
#define PA_BOOST                 		0x80

// IRQ masks
#define IRQ_FLAGS_RX_TIMEOUT        0x80
#define IRQ_RX_DONE_MASK           	0x40
#define IRQ_PAYLOAD_CRC_ERROR_MASK 	0x20
#define IRQ_FLAGS_VALID_HEADER      0x10

#define IRQ_TX_DONE_MASK           	0x08
#define IRQ_CAD_DONE_MASK          	0x04
#define IRQ_FLAGS_FHSSCHANGECHANNEL 0x02
#define IRQ_CAD_DETECTED_MASK      	0x01

#define RF_MID_BAND_THRESHOLD    		525E6
#define RSSI_OFFSET_HF_PORT      		157
#define RSSI_OFFSET_LF_PORT      		164

// Power Amplifier (PA_DAC) settings
#define PA_DAC_HIGH_POWER        		0x87
#define PA_DAC_HALF_POWER        		0x84

// Over Current Protection (OCP) config
#define OCP_ON                      (1 << 5)

#define MC3_AGCAUTO                 (1 << 2)
#define MC3_MOBILE_NODE             (1 << 3)

#define MAX_PKT_LENGTH           		255
#define IRQ_FLAGS_RX_ALL            0xf0

// Debugging support
// To enable debug information add
#define LORA_DEBUG

uint8_t debug_sx_buffer[100];
uint8_t packetIndex = 0;

// SPI helpers //
/*
HAL_StatusTypeDef SPI_Transfer(lora_sx1276 *lora, uint8_t ch, uint8_t *data_out)
{
	uint8_t data = 0;
	HAL_StatusTypeDef status = HAL_SPI_TransmitReceive(lora->spi, (uint8_t *)&ch, (uint8_t *)&data, 1, lora->spi_timeout);
	*data_out = data;
	return status;
}
*/

// Reads single register
static uint8_t read_register(lora_sx1276 *lora, uint8_t address)
{
  uint8_t value = 0;

  // 7bit controls read/write mode
  CLEAR_BIT(address, BIT_7);

  // Start SPI transaction
  HAL_GPIO_WritePin(lora->nss_port, lora->nss_pin, GPIO_PIN_RESET);
  // Transmit reg address, then receive it value

  uint32_t res1 = HAL_SPI_Transmit(lora->spi, &address, 1, lora->spi_timeout);
  uint32_t res2 = HAL_SPI_Receive(lora->spi, &value, 1, lora->spi_timeout);
  // End SPI transaction
  HAL_GPIO_WritePin(lora->nss_port, lora->nss_pin, GPIO_PIN_SET);
	
  if (res1 != HAL_OK || res2 != HAL_OK) {
	#ifdef LORA_DEBUG
		snprintf((char*)debug_sx_buffer,sizeof(debug_sx_buffer),"SPI transmit/receive failed (%d %d)", res1, res2);
    debugPrintln((char*)debug_sx_buffer);
	#endif
  }
	
return value;
}

// Writes single register
static void write_register(lora_sx1276 *lora, uint8_t address, uint8_t value)
{
  // 7bit controls read/write mode
  SET_BIT(address, BIT_7);

  // Reg address + its new value
  uint16_t payload = (value << 8) | address;

  // Start SPI transaction, send address + value
  HAL_GPIO_WritePin(lora->nss_port, lora->nss_pin, GPIO_PIN_RESET);
  uint32_t res = HAL_SPI_Transmit(lora->spi, (uint8_t*)&payload, 2, lora->spi_timeout);
  // End SPI transaction
  HAL_GPIO_WritePin(lora->nss_port, lora->nss_pin, GPIO_PIN_SET);
	
  if (res != HAL_OK) {
	#ifdef LORA_DEBUG
		snprintf((char*)debug_sx_buffer,sizeof(debug_sx_buffer),"SPI transmit failed: %d", res);
    debugPrintln((char*)debug_sx_buffer);
	#endif
  }
}

// Copies bytes from buffer into radio FIFO given len length
static void write_fifo(lora_sx1276 *lora, uint8_t *buffer, uint8_t len, uint8_t mode)
{
  uint8_t address = REG_FIFO | BIT_7;

  // Start SPI transaction, send address
  HAL_GPIO_WritePin(lora->nss_port, lora->nss_pin, GPIO_PIN_RESET);
  uint32_t res1 = HAL_SPI_Transmit(lora->spi, &address, 1, lora->spi_timeout);
  if (mode == TRANSFER_MODE_DMA) {
    HAL_SPI_Transmit_DMA(lora->spi, buffer, len);
    // Intentionally leave SPI active - let DMA finish transfer
    return;
  }
  uint32_t res2 = HAL_SPI_Transmit(lora->spi, buffer, len, lora->spi_timeout);
  // End SPI transaction
  HAL_GPIO_WritePin(lora->nss_port, lora->nss_pin, GPIO_PIN_SET);

  if (res1 != HAL_OK || res2 != HAL_OK) {
	#ifdef LORA_DEBUG
		snprintf((char*)debug_sx_buffer,sizeof(debug_sx_buffer),"SPI transmit failed");
    debugPrintln((char*)debug_sx_buffer);
	#endif
  }
}

// Reads data "len" size from FIFO into buffer
static void read_fifo(lora_sx1276 *lora, uint8_t *buffer, uint8_t len, uint8_t mode)
{
  uint8_t address = REG_FIFO;

  // Start SPI transaction, send address
  HAL_GPIO_WritePin(lora->nss_port, lora->nss_pin, GPIO_PIN_RESET);
  uint32_t res1 = HAL_SPI_Transmit(lora->spi, &address, 1, lora->spi_timeout);
  uint32_t res2;
  if (mode == TRANSFER_MODE_DMA) {
    res2 = HAL_SPI_Receive_DMA(lora->spi, buffer, len);
    // Do not end SPI here - must be done externally when DMA done
  } else {
    res2 = HAL_SPI_Receive(lora->spi, buffer, len, lora->spi_timeout);
    // End SPI transaction
    HAL_GPIO_WritePin(lora->nss_port, lora->nss_pin, GPIO_PIN_SET);
  }
	
  if (res1 != HAL_OK || res2 != HAL_OK) {
	#ifdef LORA_DEBUG
		snprintf((char*)debug_sx_buffer,sizeof(debug_sx_buffer),"SPI receive/transmit failed");
    debugPrintln((char*)debug_sx_buffer);
	#endif
  }		
}

static void set_mode(lora_sx1276 *lora, uint8_t mode)
{	
	write_register(lora, REG_OP_MODE, MODE_LONG_RANGE_MODE | mode);
}

// Set Overload Current Protection
static void set_OCP(lora_sx1276 *lora, uint8_t imax)
{
  uint8_t value;

  // Minimum available current is 45mA, maximum 240mA
  // As per page 80 of datasheet
  if (imax < 45) {
    imax = 45;
  }
  if (imax > 240) {
    imax = 240;
  }

  if (imax < 130) {
    value = (imax - 45) / 5;
  } else {
    value = (imax + 30) / 10;
  }

  write_register(lora, REG_OCP, OCP_ON | value);
}

static void set_low_data_rate_optimization(lora_sx1276 *lora)
{
  // Read current signal bandwidth
  uint64_t bandwidth = read_register(lora, REG_MODEM_CONFIG_1) >> 4;
  // Read current spreading factor
  uint8_t  sf = read_register(lora, REG_MODEM_CONFIG_2) >> 4;

  uint8_t  mc3 = MC3_AGCAUTO;

  if (sf >= 11 && bandwidth == 0x07) { // LORA_BANDWIDTH_125_KHZ
    mc3 |= MC3_MOBILE_NODE;
  }

  write_register(lora, REG_MODEM_CONFIG_3, mc3);
}

void lora_mode_sleep(lora_sx1276 *lora)
{
  set_mode(lora, MODE_SLEEP);	
}

void lora_mode_receive_continuous(lora_sx1276 *lora)
{
  // Update base FIFO address for incoming packets
  write_register(lora, REG_FIFO_RX_BASE_ADDR, lora->rx_base_addr);
  // Clear all RX related IRQs
  write_register(lora, REG_IRQ_FLAGS, IRQ_FLAGS_RX_ALL);

  set_mode(lora, MODE_RX_CONTINUOUS);
}

void lora_mode_receive_single(lora_sx1276 *lora)
{
  // Update base FIFO address for incoming packets
  write_register(lora, REG_FIFO_RX_BASE_ADDR, lora->rx_base_addr);
  // Clear all RX related IRQs
  write_register(lora, REG_IRQ_FLAGS, IRQ_FLAGS_RX_ALL);

  set_mode(lora, MODE_RX_SINGLE);
}

void lora_mode_standby(lora_sx1276 *lora)
{
  set_mode(lora, MODE_STDBY);
}

void lora_set_implicit_header_mode(lora_sx1276 *lora)
{
  uint8_t mc1 = read_register(lora, REG_MODEM_CONFIG_1);
  mc1 |= MC1_IMPLICIT_HEADER_MODE;
  write_register(lora, REG_MODEM_CONFIG_1, mc1);
}

void lora_set_explicit_header_mode(lora_sx1276 *lora)
{
  uint8_t mc1 = read_register(lora, REG_MODEM_CONFIG_1);
  mc1 &= ~MC1_IMPLICIT_HEADER_MODE;
  write_register(lora, REG_MODEM_CONFIG_1, mc1);
}

void lora_set_tx_power(lora_sx1276 *lora, uint8_t level)
{
  if (lora->pa_mode == PA_OUTPUT_RFO_PIN) {
    // RFO pin
    if (level > 15) {
      level = 15;
    }
    // 7 bit -> PaSelect: 0 for RFO    --- = 0x70
    // 6-4 bits -> MaxPower (select all) --^
    // 3-0 bits -> Output power, dB (max 15)
    write_register(lora, REG_PA_CONFIG, 0x70 | level);
  } else {
    // PA BOOST pin, from datasheet (Power Amplifier):
    //   Pout=17-(15-OutputPower)
    if (level > 20) {
      level = 20;
    }
    if (level < 2) {
      level = 2;
    }
    // Module power consumption from datasheet:
    // RFOP = +20 dBm, on PA_BOOST -> 120mA
    // RFOP = +17 dBm, on PA_BOOST -> 87mA
    if (level > 17) {
      // PA_DAC_HIGH_POWER operation changes last 3 OutputPower modes to:
      // 13 -> 18dB, 14 -> 19dB, 15 -> 20dB
      // So subtract 3 from level
      level -= 3;
      // Enable High Power mode
      write_register(lora, REG_PA_DAC, PA_DAC_HIGH_POWER);
      // Limit maximum current to 140mA (+20mA to datasheet value to be sure)
      set_OCP(lora, 140);
    } else {
      // Enable half power mode (default)
      write_register(lora, REG_PA_DAC, PA_DAC_HALF_POWER);
      // Limit maximum current to 97mA (+10mA to datasheet value to be sure)
      set_OCP(lora, 97);
    }
    // Minimum power level is 2 which is 0 for chip
    level -= 2;
    // 7 bit -> PaSelect: 1 for PA_BOOST
    write_register(lora, REG_PA_CONFIG, BIT_7 | level);
  }
}

void lora_set_frequency(lora_sx1276 *lora, uint64_t freq)
{	
  // From datasheet: FREQ = (FRF * 32 Mhz) / (2 ^ 19)
  uint64_t frf = (freq << 19) / (32 * MHZ);

  write_register(lora, REG_FRF_MSB, frf >> 16);
  write_register(lora, REG_FRF_MID, (frf & 0xff00) >> 8);
  write_register(lora, REG_FRF_LSB, frf & 0xff);
}

int8_t lora_packet_rssi(lora_sx1276 *lora)
{
  uint8_t rssi = read_register(lora, REG_PKT_RSSI_VALUE);
  return lora->frequency < (868 * MHZ) ? rssi - 164 : rssi - 157;
}

uint8_t lora_packet_snr(lora_sx1276 *lora)
{
  uint8_t snr = read_register(lora, REG_PKT_SNR_VALUE);
  return snr / 5;
}

void lora_set_signal_bandwidth(lora_sx1276 *lora, uint64_t bw)
{
  // REG_MODEM_CONFIG_1 has 2 more parameters:
  // Coding rate / Header mode, so read them before set bandwidth
  uint8_t mc1 = read_register(lora, REG_MODEM_CONFIG_1);
  // Signal bandwidth uses 4-7 bits of config
  mc1 = (mc1 & 0x0F) | bw << 4;
  write_register(lora, REG_MODEM_CONFIG_1, mc1);

  set_low_data_rate_optimization(lora);
}

void lora_set_spreading_factor(lora_sx1276 *lora, uint8_t sf)
{
  if (sf < 6) {
    sf = 6;
  } else if (sf > 12) {
    sf = 12;
  }

  if (sf == 6) {
    write_register(lora, REG_DETECTION_OPTIMIZE, 0xc5);
    write_register(lora, REG_DETECTION_THRESHOLD, 0x0c);
  } else {
    write_register(lora, REG_DETECTION_OPTIMIZE, 0xc3);
    write_register(lora, REG_DETECTION_THRESHOLD, 0x0a);
  }
  // Set new spread factor
  uint8_t mc2 = read_register(lora, REG_MODEM_CONFIG_2);
  mc2 = (mc2 & 0x0F) | ((sf << 4) & 0xf0);
  // uint8_t new_config = (current_config & 0x0f) | ((sf << 4) & 0xf0);
  write_register(lora, REG_MODEM_CONFIG_2, mc2);

  set_low_data_rate_optimization(lora);
}

void lora_set_crc(lora_sx1276 *lora, uint8_t enable)
{
  uint8_t mc2 = read_register(lora, REG_MODEM_CONFIG_2);

  if (enable) {
    mc2 |= MC2_CRC_ON;
  } else {
    mc2 &= ~MC2_CRC_ON;
  }

  write_register(lora, REG_MODEM_CONFIG_2, mc2);
}

void lora_set_coding_rate(lora_sx1276 *lora, uint8_t rate)
{
	int cr = 0;
  uint8_t mc1 = read_register(lora, REG_MODEM_CONFIG_1);
	
  if (rate < 5) {
    rate = 5;
  } else if (rate > 8) {
    rate = 8;
  }

  cr = rate - 4;
	
  // coding rate bits are 1-3 in modem config 1 register
	mc1 = (mc1 & 0xf1);
  mc1 |= cr << 1;
  write_register(lora, REG_MODEM_CONFIG_1, mc1);
}

void lora_set_preamble_length(lora_sx1276 *lora, uint16_t len)
{
  write_register(lora, REG_PREAMBLE_MSB, len >> 8);
  write_register(lora, REG_PREAMBLE_LSB, len & 0xf);
}

uint8_t lora_version(lora_sx1276 *lora)
{
  return read_register(lora, REG_VERSION);
}

uint8_t lora_is_transmitting(lora_sx1276 *lora)
{
  uint8_t opmode = read_register(lora, REG_OP_MODE);
	uint8_t mode = (opmode & MODE_TX) == MODE_TX ? LORA_BUSY : LORA_OK;
	if(mode == LORA_BUSY) {
		if (read_register(lora, REG_IRQ_FLAGS) & IRQ_TX_DONE_MASK) {
			// clear IRQ's
			write_register(lora, REG_IRQ_FLAGS, IRQ_TX_DONE_MASK);
		}		
	}
  return mode;
}

static uint8_t lora_send_packet_base(lora_sx1276 *lora, uint8_t *data, uint8_t data_len, uint8_t mode)
{
  if (lora_is_transmitting(lora)) {
    return LORA_BUSY;
  }

  // Wakeup radio because of FIFO is only available in STANDBY mode
  set_mode(lora, MODE_STDBY);

  // Clear TX IRQ flag, to be sure
  lora_clear_interrupt_tx_done(lora);

  // Set FIFO pointer to the beginning of the buffer
  write_register(lora, REG_FIFO_ADDR_PTR, lora->tx_base_addr);
  write_register(lora, REG_FIFO_TX_BASE_ADDR, lora->tx_base_addr);
  write_register(lora, REG_PAYLOAD_LENGTH, data_len);

  // Copy packet into radio FIFO
  write_fifo(lora, data, data_len, mode);
  if (mode == TRANSFER_MODE_DMA) {
    return LORA_OK;
  }

  // Put radio in TX mode - packet will be transmitted ASAP
  set_mode(lora, MODE_TX);
  return LORA_OK;
}

uint8_t lora_send_packet(lora_sx1276 *lora, uint8_t *data, uint8_t data_len)
{
  return lora_send_packet_base(lora, data, data_len, TRANSFER_MODE_BLOCKING);
}

uint8_t lora_send_packet_dma_start(lora_sx1276 *lora, uint8_t *data, uint8_t data_len)
{
  return lora_send_packet_base(lora, data, data_len, TRANSFER_MODE_DMA);
}

// Finish packet send initiated by lora_send_packet_dma_start()
void  lora_send_packet_dma_complete(lora_sx1276 *lora)
{
  // End transfer
  HAL_GPIO_WritePin(lora->nss_port, lora->nss_pin, GPIO_PIN_SET);
  // Send packet
  set_mode(lora, MODE_TX);
}

uint8_t lora_send_packet_blocking(lora_sx1276 *lora, uint8_t *data, uint8_t data_len, uint32_t timeout)
{
  uint8_t res = lora_send_packet(lora, data, data_len);

  if (res == LORA_OK) {
    // Wait until packet gets transmitted
    uint32_t elapsed = 0;
    while (elapsed < timeout) {
			// wait for TX done
      uint8_t state = read_register(lora, REG_IRQ_FLAGS);
      if (state & IRQ_TX_DONE_MASK) {
        // Packet sent
				// clear IRQ's
        write_register(lora, REG_IRQ_FLAGS, IRQ_TX_DONE_MASK);
        return LORA_OK;
      }
      HAL_Delay(1);
      elapsed++;
    }
  }

  return LORA_TIMEOUT;
}

void lora_set_rx_symbol_timeout(lora_sx1276 *lora, uint16_t symbols)
{
  if (symbols < 4) {
    symbols = 4;
  }
  if (symbols > 1023) {
    symbols = 1024;
  }

  write_register(lora, REG_SYMB_TIMEOUT_LSB, symbols & 0xf);
  if (symbols > 255) {
    // MSB (2 first bits of config2)
    uint8_t mc2 = read_register(lora, REG_MODEM_CONFIG_2);
    mc2 |= symbols >> 8;
    write_register(lora, REG_MODEM_CONFIG_2, mc2);
  }
}

uint8_t lora_is_packet_available(lora_sx1276 *lora)
{
  uint8_t irqs = read_register(lora, REG_IRQ_FLAGS);
  // In case of Single receive mode RX_TIMEOUT will be issued
  return  irqs & (IRQ_RX_DONE_MASK | IRQ_FLAGS_RX_TIMEOUT);
}

uint8_t lora_pending_packet_length(lora_sx1276 *lora)
{
  uint8_t len;

  // Query for current header mode - implicit / explicit
  uint8_t implicit = read_register(lora, REG_MODEM_CONFIG_1) & MC1_IMPLICIT_HEADER_MODE;
  if (implicit) {
    len = read_register(lora, REG_PAYLOAD_LENGTH);
  } else {
    len = read_register(lora, REG_RX_NB_BYTES);
  }

  return len;
}

static uint8_t lora_receive_packet_base(lora_sx1276 *lora, uint8_t *buffer, uint8_t buffer_len, uint8_t *error, uint8_t mode)
{
  uint8_t res = LORA_EMPTY;
  uint8_t len = 0;

  // Read/Reset IRQs
  uint8_t state = read_register(lora, REG_IRQ_FLAGS);
  write_register(lora, REG_IRQ_FLAGS, IRQ_FLAGS_RX_ALL);

  if (state & IRQ_FLAGS_RX_TIMEOUT) {
		#ifdef LORA_DEBUG
			snprintf((char*)debug_sx_buffer,sizeof(debug_sx_buffer),"timeout");
			debugPrintln((char*)debug_sx_buffer);
		#endif		
    res = LORA_TIMEOUT;
    goto done;
  }

  if (state & IRQ_RX_DONE_MASK) {
    if (!(state & IRQ_FLAGS_VALID_HEADER)) {
		#ifdef LORA_DEBUG
			snprintf((char*)debug_sx_buffer,sizeof(debug_sx_buffer),"invalid header");
			debugPrintln((char*)debug_sx_buffer);
		#endif			
      res = LORA_INVALID_HEADER;
      goto done;
    }
    // Packet has been received
    if (state & IRQ_PAYLOAD_CRC_ERROR_MASK) {
		#ifdef LORA_DEBUG
			snprintf((char*)debug_sx_buffer,sizeof(debug_sx_buffer),"CRC error");
			debugPrintln((char*)debug_sx_buffer);
		#endif			
      res = LORA_CRC_ERROR;
      goto done;
    }
    // Query for current header mode - implicit / explicit
    len = lora_pending_packet_length(lora);
    // Set FIFO to beginning of the packet
    uint8_t offset = read_register(lora, REG_FIFO_RX_CURRENT_ADDR);
    write_register(lora, REG_FIFO_ADDR_PTR, offset);
    // Read payload
    read_fifo(lora, buffer, len, mode);
    res = LORA_OK;
  }

done:
  if (error) {
    *error = res;
  }

  return len;
}

uint8_t lora_receive_packet(lora_sx1276 *lora, uint8_t *buffer, uint8_t buffer_len, uint8_t *error)
{
  return lora_receive_packet_base(lora, buffer, buffer_len, error, TRANSFER_MODE_BLOCKING);
}

uint8_t lora_receive_packet_dma_start(lora_sx1276 *lora, uint8_t *buffer, uint8_t buffer_len, uint8_t *error)
{
  return lora_receive_packet_base(lora, buffer, buffer_len, error, TRANSFER_MODE_DMA);
}

void lora_receive_packet_dma_complete(lora_sx1276 *lora)
{
  // Nothing to do expect - just end SPI transaction
  HAL_GPIO_WritePin(lora->nss_port, lora->nss_pin, GPIO_PIN_SET);
}

uint8_t lora_receive_packet_blocking(lora_sx1276 *lora, uint8_t *buffer, uint8_t buffer_len,
                   uint32_t timeout, uint8_t *error)
{
  uint32_t elapsed = 0;

  // Wait up to timeout for packet
  while (elapsed < timeout) {
    if (lora_is_packet_available(lora)) {
      break;
    }
    HAL_Delay(1);
    elapsed++;
  }
  return lora_receive_packet(lora, buffer, buffer_len, error);
}

void lora_enable_interrupt_rx_done(lora_sx1276 *lora)
{
  // Table 63 DIO Mapping LoRaTM Mode:
  // 00 -> (DIO0 rx_done)
  // DIO0 uses 6-7 bits of DIO_MAPPING_1
  write_register(lora, REG_DIO_MAPPING_1, 0x00);
}

void lora_enable_interrupt_tx_done(lora_sx1276 *lora)
{
  // Table 63 DIO Mapping LoRaTM Mode:
  // 01 -> (DIO0 tx_done)
  // DIO0 uses 6-7 bits of DIO_MAPPING_1
  write_register(lora, REG_DIO_MAPPING_1, 0x40);
}

void lora_clear_interrupt_tx_done(lora_sx1276 *lora)
{
  write_register(lora, REG_IRQ_FLAGS, IRQ_TX_DONE_MASK);
}

void lora_clear_interrupt_rx_all(lora_sx1276 *lora)
{
  write_register(lora, REG_IRQ_FLAGS, IRQ_FLAGS_RX_ALL);
}


uint8_t lora_init(lora_sx1276 *lora, SPI_HandleTypeDef *spi, GPIO_TypeDef *nss_port, GPIO_TypeDef *reset_port, uint16_t nss_pin, uint16_t reset_pin, uint64_t freq)
{
  // Init params with default values
  lora->spi = spi;
  lora->nss_port = nss_port;
  lora->nss_pin = nss_pin;
	lora->reset_port = reset_port;
  lora->reset_pin = reset_pin;
  lora->frequency = freq;
  lora->pa_mode = PA_OUTPUT_PA_BOOST_PIN;
  lora->tx_base_addr = REG_FIFO_TX_BASE_ADDR;
  lora->rx_base_addr = REG_FIFO_RX_BASE_ADDR;
  lora->spi_timeout = LORA_DEFAULT_SPI_TIMEOUT;
	lora_reset(lora);
  // Check version
  uint8_t ver = lora_version(lora);
  if (ver != LORA_COMPATIBLE_VERSION) {
		#ifdef LORA_DEBUG
			snprintf((char*)debug_sx_buffer,sizeof(debug_sx_buffer),"Got wrong radio version 0x%x, expected 0x12", ver);
			debugPrintln((char*)debug_sx_buffer);
		#endif
    return LORA_ERROR;
  }

  // Modem parameters (freq, mode, etc) must be done in SLEEP mode.
  lora_mode_sleep(lora);
  // Enable LoRa mode (since it can be switched on only in sleep)
  //lora_mode_sleep(lora);

  // Set frequency
  lora_set_frequency(lora, freq);
	
	  // set base addresses
  write_register(lora, REG_FIFO_TX_BASE_ADDR, 0);
  write_register(lora, REG_FIFO_RX_BASE_ADDR, 0);
	
  // Set LNA boost
  uint8_t current_lna = read_register(lora, REG_LNA);
  write_register(lora, REG_LNA,  current_lna | 0x03);
  // Set auto AGC
  write_register(lora, REG_MODEM_CONFIG_3, 0x04);
  // Set default output power
  lora_set_tx_power(lora, LORA_DEFAULT_TX_POWER);
	
	// setting according to rak2
	lora_set_spreading_factor(lora,12);
	lora_set_signal_bandwidth(lora,7);
	lora_set_coding_rate(lora,0);
	lora_set_preamble_length(lora,8);
	
  // Set default mode
  lora_mode_standby(lora);

  return LORA_OK;
}

////////////////////////////////////////////////////////////////////
void lora_reset(lora_sx1276 *lora)
{
  // perform reset
	HAL_GPIO_WritePin(lora->reset_port, lora->reset_pin, GPIO_PIN_RESET);
  HAL_Delay(10);
  HAL_GPIO_WritePin(lora->reset_port, lora->reset_pin, GPIO_PIN_SET);
  HAL_Delay(10);	
}

uint8_t lora_parsepacket(lora_sx1276 *lora)
{
	int16_t packetLength = 0;	
  uint8_t irqFlags = read_register(lora, REG_IRQ_FLAGS);
	lora_set_explicit_header_mode(lora);
	// clear IRQ's
  write_register(lora, REG_IRQ_FLAGS, irqFlags);
	
  if ((irqFlags & IRQ_RX_DONE_MASK) && (irqFlags & IRQ_PAYLOAD_CRC_ERROR_MASK) == 0) {
    // received a packet
		packetIndex = 0;
    // read packet length
		packetLength = lora_pending_packet_length(lora);
    // set FIFO address to current RX address
    write_register(lora,REG_FIFO_ADDR_PTR, read_register(lora,REG_FIFO_RX_CURRENT_ADDR));
    // put in standby mode
		lora_mode_standby(lora);
  } else if (read_register(lora, REG_OP_MODE) != (MODE_LONG_RANGE_MODE | MODE_RX_SINGLE)) {
    // not currently in RX mode
    // reset FIFO address
    write_register(lora, REG_FIFO_ADDR_PTR, 0);
    // put in single RX mode
    write_register(lora, REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_RX_SINGLE);
  }	
  return packetLength;
}

uint8_t lora_available(lora_sx1276 *lora)
{
  return (read_register(lora,REG_RX_NB_BYTES) - packetIndex);
}

uint8_t lora_read(lora_sx1276 *lora)
{
  if (!lora_available(lora)) {
    return 0;
  }
  packetIndex++;
  return read_register(lora,REG_FIFO);
}

/////////////////////////////////////////////////////////////////////
