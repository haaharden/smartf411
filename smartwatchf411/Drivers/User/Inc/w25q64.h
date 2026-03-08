#ifndef _W25Q64_H
#define _W25Q64_H

#include "main.h"
#include "spi.h"

// 定义片选引脚操作 (PB12)
#define W25Q_CS_LOW()  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET)
#define W25Q_CS_HIGH() HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET)

uint32_t W25Q_ReadID(void);
void W25Q_WriteEnable(void);
void W25Q_WaitForWriteEnd(void);
void W25Q_EraseSector(uint32_t addr);
void W25Q_PageProgram(uint32_t addr, const uint8_t *data, uint16_t size);
void W25Q_WriteData_Smart(uint32_t start_addr, const uint8_t *data, uint32_t size);
void W25Q_ReadData(uint32_t addr, uint8_t *data, uint32_t size);
	
#endif

