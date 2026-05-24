/*
 * ad7124-driver.c
 *
 *  Created on: Feb 6, 2025
 *      Author: user
 */


#include "ad7124-driver.h"
#include <stdbool.h>
#include <integer.h>

bool useCRC = 1;
volatile int spi_ss = 0;


Ad7124Register reg[Reg_No];

static void setss()
{
			HAL_GPIO_WritePin(AD7124_CS_PORT, AD7124_CS_PIN, GPIO_PIN_RESET);
}

static void clearSS()
{
    // Deselect the AD7124 (pull CS high)
			HAL_GPIO_WritePin(AD7124_CS_PORT, AD7124_CS_PIN, GPIO_PIN_SET);
}


uint8_t computeCRC8(uint8_t *data, uint8_t bufSize)
{
    uint8_t crc = 0;

    for (uint8_t i = 0; i < bufSize; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x80)
                crc = (crc << 1) ^ 0x07;
            else
                crc <<= 1;
        }
    }

    return crc;
}

int AD7124_ReadRegister(uint8_t reg, uint8_t *data, uint16_t size) {
    int status;
    uint8_t i;
    uint8_t check8 = 0;
    uint8_t msgBuf[10] = {0};
    uint8_t cmd = AD7124_COMM_REG_WEN | AD7124_COMM_REG_RD | AD7124_COMM_REG_RA(reg);

    setss();

    // Send the read command
    status = HAL_SPI_Transmit(&hspi4, &cmd, 1, AD7124_SPI_TIMEOUT);
    if (status != HAL_OK) {
        clearSS();
        return status;
    }

    // Receive the register data
    for (i = 0; i < ((useCRC != AD7124_DISABLE_CRC) ? size + 1 : size); i++) {
        status = HAL_SPI_Receive(&hspi4, &data[i], 1, AD7124_SPI_TIMEOUT);
        if (status != HAL_OK) {
            clearSS();
            return status;
        }
    }

    clearSS();

    if (useCRC == AD7124_USE_CRC) {
        msgBuf[0] = cmd;
        for (i = 1; i < size + 2; ++i) {
            msgBuf[i] = data[i - 1];
        }

        check8 = computeCRC8(msgBuf, size + 2);
        if (check8 != 0) {
            return AD7124_COMM_ERR; // CRC check failed
        }
    }

    return 1;
}

int AD7124_WriteRegister(const Ad7124Register *r) {
    int32_t regValue = r->value;
    uint8_t wrt_buf[10] = {0};
    uint8_t i = 0;
    uint8_t crc8 = 0;
    int status;

    wrt_buf[0] = AD7124_COMM_REG_WEN | AD7124_COMM_REG_WR | AD7124_WRITE_CMD(r->addr);

    for (i = 0; i < r->size; i++) {
        wrt_buf[r->size - i] = regValue & 0xFF;
        regValue >>= 8;
    }

    if (useCRC != AD7124_DISABLE_CRC) {
        crc8 = computeCRC8(wrt_buf, r->size + 1);
        wrt_buf[r->size + 1] = crc8;
    }

    setss();

    for (i = 0; i < ((useCRC != AD7124_DISABLE_CRC) ? r->size + 2 : r->size + 1); i++) {
        status = HAL_SPI_Transmit(&hspi4, &wrt_buf[i], 1, HAL_MAX_DELAY);
        if (status != HAL_OK) {
            clearSS();
            return status;
        }
    }

    clearSS();

    return 1;
}

void AD7124_Reset()
{
	uint8_t reset_bytes[8] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

		    setss();
			HAL_SPI_Transmit(&hspi4, reset_bytes, 8, 100);
		    clearSS();
		    HAL_Delay(2);
}

int waitForSpiReady (uint32_t timeout)
{
  int ret;
  bool ready = false;
  bool timeout_en;
  uint8_t status_buf[3] = {0};
  timeout_en = (timeout > 0);
  do {
    /* Read the value of the Error Register (3 bytes) */
    ret = AD7124_ReadRegister(Error, status_buf, 3);
    if (ret < 0) {

      return ret;
    }

    /* Reconstruct 24-bit error value and check the SPI IGNORE Error bit */
    uint32_t err_val = ((uint32_t)status_buf[0] << 16) | ((uint32_t)status_buf[1] << 8) | status_buf[2];
    ready = ((err_val & AD7124_ERR_REG_SPI_IGNORE_ERR) == 0);

    if (timeout) {

      HAL_Delay (1);
      timeout--;
    }
  }
  while (!ready && timeout);

  return timeout_en && (timeout == 0) ? AD7124_TIMEOUT : ready;

}

int waitToPowerOn (uint32_t timeout) {
  int ret;
  bool powered_on = false;
  bool timeout_en;
  uint8_t status_buf[1] = {0};

  timeout /= 10;
  timeout_en = (timeout > 0);

  do {

    ret = AD7124_ReadRegister(Status, status_buf, 1);
    if (ret < 0) {

      return ret;
    }

    /* Check the POR_FLAG bit in the Status Register */
    powered_on = ((status_buf[0] &
                  AD7124_STATUS_REG_POR_FLAG) == 0);
    if (timeout) {

      HAL_Delay(10);
      timeout--;
    }
  }
  while (!powered_on && timeout);

  return timeout_en && (timeout == 0) ? AD7124_TIMEOUT : powered_on;
}

int waitForConvReady (uint32_t timeout) {
  int ret;
  bool ready = false;
  bool timeout_en;
  uint8_t status_buf[1] = {0};
  timeout_en = (timeout > 0);

  do {

    /* Read the value of the Status Register */
    ret = AD7124_ReadRegister(Status, status_buf, 1);
    if (ret < 0) {

      return ret;
    }

    /* Check the RDY bit in the Status Register */
    ready = ((status_buf[0] &
             AD7124_STATUS_REG_RDY) == 0);

    if (timeout) {

      for(UINT i=0; i<1200;i++);
      timeout--;
    }
  }
  while (!ready && timeout);

  return timeout_en && (timeout == 0) ? AD7124_TIMEOUT : ready;
}


