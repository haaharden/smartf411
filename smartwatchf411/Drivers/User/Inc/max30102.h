#ifndef __MAX30102_H
#define __MAX30102_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"
#include <stdint.h>

/* MAX30102 7bit 地址 = 0x57，HAL 里传 0xAE */
#define MAX30102_I2C_ADDR      (0x57U << 1)

/* MAX30102 寄存器地址 */
#define REG_INTR_STATUS_1      0x00
#define REG_INTR_STATUS_2      0x01
#define REG_INTR_ENABLE_1      0x02
#define REG_INTR_ENABLE_2      0x03
#define REG_FIFO_WR_PTR        0x04
#define REG_OVF_COUNTER        0x05
#define REG_FIFO_RD_PTR        0x06
#define REG_FIFO_DATA          0x07
#define REG_FIFO_CONFIG        0x08
#define REG_MODE_CONFIG        0x09
#define REG_SPO2_CONFIG        0x0A
#define REG_LED1_PA            0x0C
#define REG_LED2_PA            0x0D
#define REG_PILOT_PA           0x10
#define REG_MULTI_LED_CTRL1    0x11
#define REG_MULTI_LED_CTRL2    0x12
#define REG_TEMP_INTR          0x1F
#define REG_TEMP_FRAC          0x20
#define REG_TEMP_CONFIG        0x21
#define REG_PROX_INT_THRESH    0x30
#define REG_REV_ID             0xFE
#define REG_PART_ID            0xFF

/* API：
 * 所有函数返回 0 = OK，非 0 = 错误
 */

uint8_t max30102_init(void);
uint8_t max30102_reset(void);

uint8_t max30102_write_reg(uint8_t reg, uint8_t value);
uint8_t max30102_read_reg(uint8_t reg, uint8_t *value);

/* 读 FIFO，一次拿一对 RED/IR 样本（24bit 有效，其中高 6bit 丢弃） */
uint8_t max30102_read_fifo(uint32_t *red, uint32_t *ir);

#ifdef __cplusplus
}
#endif

#endif /* __MAX30102_H */
