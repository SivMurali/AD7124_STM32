/*
 * ad7124-driver.h
 *
 *  Created on: Feb 6, 2025
 *      Author: user
 */

#ifndef INC_AD7124_DRIVER_H_
#define INC_AD7124_DRIVER_H_

#include "ad7124-defs.h"
#include "stdbool.h"
extern SPI_HandleTypeDef hspi4;

// Inside ad7124-driver.h
#define AD7124_SPI_HANDLE    hspi4
#define AD7124_CS_PORT       SPI4_CS_GPIO_Port
#define AD7124_CS_PIN        SPI4_CS_Pin

#define AD7124_SPI_TIMEOUT 10  // Timeout in milliseconds
#define AD7124_DEVICE_ID_REG 0x05
#define AD7124_WRITE_CMD(regAddr) ((regAddr) & 0x3F)  // Write command format for AD7124
#define AD7124_READ_CMD(regAddr) (0x40 | ((regAddr) & 0x3F))  // Read command format for AD7124

#define AD7124_REG_NO 58

/* AD7124 Register struct */
typedef struct {
    uint8_t addr;
    int32_t value;
    uint8_t size;
    uint8_t rw; // 0 for read-only, 1 for read-write
} Ad7124Register;

/**
* @brief Reads data from SPI.
*
* @param reg - reg represents the register address of ad7124
* @param data - Data represents the write buffer as an input parameter and the
*               read buffer as an output parameter.
* @param size - Number of bytes to read.
*
* @return status of SPI operation success/fail.
*/
int AD7124_ReadRegister(uint8_t reg, uint8_t *data, uint16_t size);

/**
* @brief Writes data to SPI.
*
* @param reg - reg represents the register address of ad7124
* @param data - Data represents the write buffer.
* @param size - Number of bytes to write.
*
* @return status of SPI operation success/fail.
*/
int AD7124_WriteRegister(const Ad7124Register *r);

/**
 * Reset the ad7124-4 and update the registers to it's default
 */
void AD7124_Reset();


/**  Checks the SPI device is available or not
*
* @param timeout - time delay for the operation
*
*/
int waitForSpiReady (uint32_t timeout);

/**  Checks the SPI device is properly powered ON or not
*
* @param timeout - time delay for the operation
*
*/
int waitToPowerOn (uint32_t timeout);

/**  Monitors ready flag to ensure ADC conversion done or not
*
* @param timeout - wait time for the operation
*
*/
int waitForConvReady (uint32_t timeout);



#endif /* INC_AD7124_DRIVER_H_ */
