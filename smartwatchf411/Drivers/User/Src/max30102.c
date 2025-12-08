/**
 * ************************************************************************
 * 
 * @file MAX30102.c
 * @author zxr
 * @brief 
 * 
 * ************************************************************************
 * @copyright Copyright (c) 2024 zxr 
 * ************************************************************************
 */
#include "MAX30102.h"
#include "cmsis_os.h"
#include "stdio.h"

extern osMutexId_t I2C1_MutexHandle;

/*SCL<->PB6
	SDA<->PB7
	VCC<->3.3V
	GND<->GND*/
uint16_t fifo_red;  //定义FIFO中的红光数据
uint16_t fifo_ir;   //定义FIFO中的红外光数据

/**
 * ************************************************************************
 * @brief 向MAX30102寄存器写入一个值
 * 
 * @param[in] addr  寄存器地址
 * @param[in] data  传输数据
 * 
 * @return 
 * ************************************************************************
 */
uint8_t max30102_write_reg(uint8_t reg, uint8_t data)
{
    if (osMutexAcquire(I2C1_MutexHandle, osWaitForever) != osOK) {
        return 0;   // 拿不到锁，直接失败
    }

    HAL_StatusTypeDef status = HAL_I2C_Mem_Write(&hi2c1,
                                                 MAX30102_Device_address,
                                                 reg,
                                                 I2C_MEMADD_SIZE_8BIT,
                                                 &data,
                                                 1,
                                                 100);

    osMutexRelease(I2C1_MutexHandle);

    return (status == HAL_OK) ? 1 : 0;
}


/**
 * ************************************************************************
 * @brief 读取MAX30102寄存器的一个值
 * 
 * @param[in] addr  寄存器地址
 * 
 * @return 
 * ************************************************************************
 */
uint8_t max30102_read_reg(uint8_t reg)
{
    uint8_t data = 0;

    if (osMutexAcquire(I2C1_MutexHandle, osWaitForever) != osOK) {
        return 0;
    }

    HAL_StatusTypeDef status = HAL_I2C_Mem_Read(&hi2c1,
                                                MAX30102_Device_address,
                                                reg,
                                                I2C_MEMADD_SIZE_8BIT,
                                                &data,
                                                1,
                                                100);

    osMutexRelease(I2C1_MutexHandle);

    if (status != HAL_OK) {
        return 0;   // 这里 0 既是失败标志，也可能是真 0，调试阶段够用
    }

    return data;
}


uint8_t max30102_get_part_id(void)
{
    uint8_t id = max30102_read_reg(REG_PART_ID);
    return id;
}

/**
 * ************************************************************************
 * @brief MAX30102传感器初始化
 * 
 * 
 * @return 
 * ************************************************************************
 */
uint8_t MAX30102_Init(void)
{
    // 1) 先读一下 PART ID，确认芯片在不在
    uint8_t part_id = max30102_read_reg(REG_PART_ID);
    printf("MAX30102 PART ID = 0x%02X\r\n", part_id);

    if(part_id != 0x15) {   // datasheet 里 PART ID 一般是 0x15
        printf("MAX30102 not detected!\r\n");
        return 0;
    }

    // 2) 软复位
    if(!max30102_write_reg(REG_MODE_CONFIG, 0x40)) {
        printf("MAX30102 reset write failed!\r\n");
        return 0;
    }
    HAL_Delay(10);

    // 3) 一条条配寄存器，顺便检查有没有写失败
    uint8_t ok = 1;

    ok &= max30102_write_reg(REG_INTR_ENABLE_1,0xc0);
    ok &= max30102_write_reg(REG_INTR_ENABLE_2,0x00);
    ok &= max30102_write_reg(REG_FIFO_WR_PTR,0x00);
    ok &= max30102_write_reg(REG_OVF_COUNTER,0x00);
    ok &= max30102_write_reg(REG_FIFO_RD_PTR,0x00);

    ok &= max30102_write_reg(REG_FIFO_CONFIG,0x0f);
    ok &= max30102_write_reg(REG_MODE_CONFIG,0x03);
    ok &= max30102_write_reg(REG_SPO2_CONFIG,0x27);
    ok &= max30102_write_reg(REG_LED1_PA,0x32);
    ok &= max30102_write_reg(REG_LED2_PA,0x32);
    ok &= max30102_write_reg(REG_PILOT_PA,0x7f);

    if(!ok) {
        printf("MAX30102 config I2C write failed!\r\n");
        return 0;
    }

    printf("MAX30102 init OK\r\n");
    return 1;
}

/**
 * ************************************************************************
 * @brief 读取FIFO寄存器的数据
 * 
 * 
 * ************************************************************************
 */
void max30102_read_fifo(void)
{
  uint16_t un_temp;
  fifo_red=0;
  fifo_ir=0;
  uint8_t ach_i2c_data[6];
  
  //read and clear status register
  max30102_read_reg(REG_INTR_STATUS_1);
  max30102_read_reg(REG_INTR_STATUS_2);
  
  ach_i2c_data[0]=REG_FIFO_DATA;
	
	HAL_I2C_Mem_Read(&hi2c1,MAX30102_Device_address,REG_FIFO_DATA,1,ach_i2c_data,6,HAL_MAX_DELAY);
	
  un_temp=ach_i2c_data[0];
  un_temp<<=14;
  fifo_red+=un_temp;
  un_temp=ach_i2c_data[1];
  un_temp<<=6;
  fifo_red+=un_temp;
  un_temp=ach_i2c_data[2];
	un_temp>>=2;
  fifo_red+=un_temp;
  
  un_temp=ach_i2c_data[3];
  un_temp<<=14;
  fifo_ir+=un_temp;
  un_temp=ach_i2c_data[4];
  un_temp<<=6;
  fifo_ir+=un_temp;
  un_temp=ach_i2c_data[5];
	un_temp>>=2;
  fifo_ir+=un_temp;
	
	if(fifo_ir<=10000)
	{
		fifo_ir=0;
	}
	if(fifo_red<=10000)
	{
		fifo_red=0;
	}
}



