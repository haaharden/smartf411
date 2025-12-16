/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    i2c.c
  * @brief   This file provides code for the configuration
  *          of the I2C instances.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "i2c.h"

/* USER CODE BEGIN 0 */
#include "gpio.h"
#include "main.h"
/* USER CODE END 0 */

I2C_HandleTypeDef hi2c1;
I2C_HandleTypeDef hi2c2;

/* I2C1 init function */
void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}
/* I2C2 init function */
void MX_I2C2_Init(void)
{

  /* USER CODE BEGIN I2C2_Init 0 */

  /* USER CODE END I2C2_Init 0 */

  /* USER CODE BEGIN I2C2_Init 1 */

  /* USER CODE END I2C2_Init 1 */
  hi2c2.Instance = I2C2;
  hi2c2.Init.ClockSpeed = 100000;
  hi2c2.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c2.Init.OwnAddress1 = 0;
  hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c2.Init.OwnAddress2 = 0;
  hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C2_Init 2 */

  /* USER CODE END I2C2_Init 2 */

}

void HAL_I2C_MspInit(I2C_HandleTypeDef* i2cHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(i2cHandle->Instance==I2C1)
  {
  /* USER CODE BEGIN I2C1_MspInit 0 */

  /* USER CODE END I2C1_MspInit 0 */

    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**I2C1 GPIO Configuration
    PB6     ------> I2C1_SCL
    PB7     ------> I2C1_SDA
    */
    GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* I2C1 clock enable */
    __HAL_RCC_I2C1_CLK_ENABLE();
  /* USER CODE BEGIN I2C1_MspInit 1 */

  /* USER CODE END I2C1_MspInit 1 */
  }
  else if(i2cHandle->Instance==I2C2)
  {
  /* USER CODE BEGIN I2C2_MspInit 0 */

  /* USER CODE END I2C2_MspInit 0 */

    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**I2C2 GPIO Configuration
    PB10     ------> I2C2_SCL
    PB3     ------> I2C2_SDA
    */
    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C2;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF9_I2C2;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* I2C2 clock enable */
    __HAL_RCC_I2C2_CLK_ENABLE();
  /* USER CODE BEGIN I2C2_MspInit 1 */

  /* USER CODE END I2C2_MspInit 1 */
  }
}

void HAL_I2C_MspDeInit(I2C_HandleTypeDef* i2cHandle)
{

  if(i2cHandle->Instance==I2C1)
  {
  /* USER CODE BEGIN I2C1_MspDeInit 0 */

  /* USER CODE END I2C1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_I2C1_CLK_DISABLE();

    /**I2C1 GPIO Configuration
    PB6     ------> I2C1_SCL
    PB7     ------> I2C1_SDA
    */
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_6);

    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_7);

  /* USER CODE BEGIN I2C1_MspDeInit 1 */

  /* USER CODE END I2C1_MspDeInit 1 */
  }
  else if(i2cHandle->Instance==I2C2)
  {
  /* USER CODE BEGIN I2C2_MspDeInit 0 */

  /* USER CODE END I2C2_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_I2C2_CLK_DISABLE();

    /**I2C2 GPIO Configuration
    PB10     ------> I2C2_SCL
    PB3     ------> I2C2_SDA
    */
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_10);

    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_3);

  /* USER CODE BEGIN I2C2_MspDeInit 1 */

  /* USER CODE END I2C2_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */
void I2C_BusRecover(I2C_HandleTypeDef *hi2c)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_TypeDef *SCL_Port = GPIOB;
    GPIO_TypeDef *SDA_Port = GPIOB;
    uint16_t SCL_Pin;
    uint16_t SDA_Pin;
    uint8_t  AF_Config; // ���ù���ӳ��ֵ

    /* 1. ���ݾ��ʶ���������� */
    if (hi2c->Instance == I2C1)
    {
        // === I2C1 (PB6 / PB7) ===
        SCL_Port = GPIOB; SCL_Pin = GPIO_PIN_6;
        SDA_Port = GPIOB; SDA_Pin = GPIO_PIN_7;
        AF_Config = GPIO_AF4_I2C1;

        __HAL_RCC_I2C1_CLK_DISABLE(); // �� I2C1 ��ʱ��
        __HAL_RCC_GPIOB_CLK_ENABLE(); // �� GPIO ʱ��
    }
    else if (hi2c->Instance == I2C2)
    {
        // === I2C2 (PB10 / PB3) ===
        // ע�⣺PB9 �������ж�ռ�ˣ���������Ĭ���� PB3
        SCL_Port = GPIOB; SCL_Pin = GPIO_PIN_10;
        SDA_Port = GPIOB; SDA_Pin = GPIO_PIN_3;
        AF_Config = GPIO_AF4_I2C2;

        __HAL_RCC_I2C2_CLK_DISABLE(); // �� I2C2 ʱ��
        __HAL_RCC_GPIOB_CLK_ENABLE(); // �� GPIO ʱ��
    }
    else
    {
        return; // δ֪�� I2C��ֱ�ӷ���
    }

    /* 2. ����Ϊ��ͨ��©��� (�ֶ�����) */
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull  = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;

    GPIO_InitStruct.Pin = SCL_Pin;
    HAL_GPIO_Init(SCL_Port, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = SDA_Pin;
    HAL_GPIO_Init(SDA_Port, &GPIO_InitStruct);

    /* 3. ��ʼģ��ʱ��9 ��������������Ĵӻ� */
    
    // �Ȱ�������
    HAL_GPIO_WritePin(SCL_Port, SCL_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(SDA_Port, SDA_Pin, GPIO_PIN_SET);
    HAL_Delay(1);

    // ���� 9 ��ʱ������
    for (int i = 0; i < 9; i++)
    {
        // ��� SDA �Ѿ����ͷţ���ߣ���˵���ӻ������ˣ�������ǰ����
        if (HAL_GPIO_ReadPin(SDA_Port, SDA_Pin) == GPIO_PIN_SET)
        {
            break;
        }

        // SCL ����
        HAL_GPIO_WritePin(SCL_Port, SCL_Pin, GPIO_PIN_RESET);
        HAL_Delay(1);

        // SCL ����
        HAL_GPIO_WritePin(SCL_Port, SCL_Pin, GPIO_PIN_SET);
        HAL_Delay(1);
    }

    /* 4. ���� STOP �ź� */
    // STOP ����SCL �ߵ�ƽʱ��SDA �ӵͱ��
    HAL_GPIO_WritePin(SDA_Port, SDA_Pin, GPIO_PIN_RESET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(SCL_Port, SCL_Pin, GPIO_PIN_SET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(SDA_Port, SDA_Pin, GPIO_PIN_SET);
    HAL_Delay(1);

    /* 5. �ָ�����Ϊ���ù��� (AF) */
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull      = GPIO_PULLUP;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = AF_Config; // �Զ���Ӧ AF4

    GPIO_InitStruct.Pin = SCL_Pin;
    HAL_GPIO_Init(SCL_Port, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = SDA_Pin;
    HAL_GPIO_Init(SDA_Port, &GPIO_InitStruct);

    /* 6. ���³�ʼ�� I2C ���� */
    if (hi2c->Instance == I2C1)
    {
        __HAL_RCC_I2C1_CLK_ENABLE(); // ���¿�ʱ��
    }
    else if (hi2c->Instance == I2C2)
    {
        __HAL_RCC_I2C2_CLK_ENABLE(); // ���¿�ʱ��
    }

    HAL_I2C_DeInit(hi2c);
    HAL_I2C_Init(hi2c);
}
/* USER CODE END 1 */
