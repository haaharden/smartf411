/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdio.h"
#include "lvgl.h"
#include "data.h"
#include "gui.h"
#include "rtc.h"
#include "blood.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
osMutexId_t I2C1_MutexHandle;
const osMutexAttr_t I2C1_Mutex_attributes = {
    .name = "I2C1_Mutex"
};
/* USER CODE END Variables */
/* Definitions for guiTask */
osThreadId_t guiTaskHandle;
const osThreadAttr_t guiTask_attributes = {
  .name = "guiTask",
  .stack_size = 1024 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for rtcTask */
osThreadId_t rtcTaskHandle;
const osThreadAttr_t rtcTask_attributes = {
  .name = "rtcTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityBelowNormal,
};
/* Definitions for MAX30102Task */
osThreadId_t MAX30102TaskHandle;
const osThreadAttr_t MAX30102Task_attributes = {
  .name = "MAX30102Task",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityBelowNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartGUITask(void *argument);
void StartRTCTask(void *argument);
void StartMAX30102Task(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* Hook prototypes */
void vApplicationTickHook(void);

/* USER CODE BEGIN 3 */
void vApplicationTickHook( void )
{
   /* This function will be called by each tick interrupt if
   configUSE_TICK_HOOK is set to 1 in FreeRTOSConfig.h. User code can be
   added here, but the tick hook is called from an interrupt context, so
   code must not attempt to block, and only the interrupt safe FreeRTOS API
   functions can be used (those that end in FromISR()). */
	lv_tick_inc(1);  
}
/* USER CODE END 3 */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */
  /* 创建 I2C1 互斥锁 */
  I2C1_MutexHandle = osMutexNew(&I2C1_Mutex_attributes);
  if (I2C1_MutexHandle == NULL) {
      Error_Handler();     // 建议直接报错，说明系统有问题
  }
  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of guiTask */
  guiTaskHandle = osThreadNew(StartGUITask, NULL, &guiTask_attributes);

  /* creation of rtcTask */
  rtcTaskHandle = osThreadNew(StartRTCTask, NULL, &rtcTask_attributes);

  /* creation of MAX30102Task */
  MAX30102TaskHandle = osThreadNew(StartMAX30102Task, NULL, &MAX30102Task_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartGUITask */
/**
  * @brief  Function implementing the guiTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartGUITask */
void StartGUITask(void *argument)
{
  /* USER CODE BEGIN StartGUITask */
	uint32_t tick = 0;
  /* Infinite loop */
  for(;;)
  {
      lv_timer_handler();   // 处理 LVGL 内部定时器 / 动画 / 输入
      osDelay(5);           // 每 5ms 跑一次
			tick += 5;
        if (tick >= 200) {    // 每 ~200ms 更新一次 UI
            tick = 0;

            // 1) 刷新时间
            char buf_time[16];
            lv_snprintf(buf_time, sizeof(buf_time), "%02d:%02d:%02d",
                        g_clock_time.hour,
                        g_clock_time.min,
                        g_clock_time.sec);
            lv_label_set_text(label_time, buf_time);

            // 2) 刷新血氧
            char buf_spo2[16];
            lv_snprintf(buf_spo2, sizeof(buf_spo2), "SpO2: %d%%", g_spo2_data.spo2);
            lv_label_set_text(label_spo2, buf_spo2);

            // 3) 刷新心率
            char buf_hr[16];
            lv_snprintf(buf_hr, sizeof(buf_hr), "HR: %d", g_spo2_data.heart_rate);
            lv_label_set_text(label_hr, buf_hr);
        }
    }
  /* USER CODE END StartGUITask */
}

/* USER CODE BEGIN Header_StartRTCTask */
/**
* @brief Function implementing the rtcTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartRTCTask */
void StartRTCTask(void *argument)
{
  /* USER CODE BEGIN StartRTCTask */
	RTC_TimeTypeDef Time_Struct;	     
  RTC_DateTypeDef Date_Struct;		
  /* Infinite loop */
  for(;;)
  {
        // 先读时间
        HAL_RTC_GetTime(&hrtc, &Time_Struct, RTC_FORMAT_BIN);
        // 再读日期（必须读一次才能解锁寄存器，虽然你现在用不到日期）
        HAL_RTC_GetDate(&hrtc, &Date_Struct, RTC_FORMAT_BIN);

        // 写入全局时间结构体
        g_clock_time.hour = Time_Struct.Hours;
        g_clock_time.min  = Time_Struct.Minutes;
        g_clock_time.sec  = Time_Struct.Seconds;

        // 每 200ms 更新一次就够了
        osDelay(200);
  }
  /* USER CODE END StartRTCTask */
}

/* USER CODE BEGIN Header_StartMAX30102Task */
/**
* @brief Function implementing the MAX30102Task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartMAX30102Task */
void StartMAX30102Task(void *argument)
{
  /* USER CODE BEGIN StartMAX30102Task */
    osDelay(200);  // 等 I2C、FreeRTOS、互斥锁都起来

    if(!MAX30102_Init()) {
        printf("MAX init failed, SpO2 task exit\r\n");
        vTaskDelete(NULL);
    }
  /* Infinite loop */
  for(;;)
  {
        /* 1) 调用你原来的测量流程：采集 + 计算 */
        blood_Loop();   // 里面已经做了 blood_data_update + blood_data_translate

        /* 2) 把结果写到 GUI 用的结构体里 */

        // 先处理 SpO2（float -> uint8_t）
        float spo2_val = SpO2;

        if (spo2_val < 0.0f)      spo2_val = 0.0f;
        if (spo2_val > 99.0f)     spo2_val = 99.0f;   // 你自己也在代码里限制到 99.99

        g_spo2_data.spo2 = (uint8_t)(spo2_val + 0.5f);  // 四舍五入取整

        // 再处理 heart（int -> uint8_t）
        int hr_val = heart;
        if (hr_val < 0)   hr_val = 0;
        if (hr_val > 250) hr_val = 250;   // 给个合理上限

        g_spo2_data.heart_rate = (uint8_t)hr_val;

        /* 3) 稍微休息一下再测下一组
         *    具体间隔你可以自己调，500~1000ms 都可以
         */
        osDelay(1000);
  }
  /* USER CODE END StartMAX30102Task */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

