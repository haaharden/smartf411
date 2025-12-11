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
#include "i2c.h"
#include "stdio.h"
#include "lvgl.h"
#include "rtc.h"
#include "gui.h"
#include "touch.h"
#include "blood.h"
#include "algorithm.h"
#include "MAX30102.h"
#include "mpu6500.h"
#include "ui.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
extern volatile TouchEvent g_last_event;//��ҪΪ��������
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
/* Definitions for MPU6500Task */
osThreadId_t MPU6500TaskHandle;
const osThreadAttr_t MPU6500Task_attributes = {
  .name = "MPU6500Task",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityBelowNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartGUITask(void *argument);
void StartRTCTask(void *argument);
void StartMAX30102Task(void *argument);
void StartMPU6500Task(void *argument);

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
  /* ���� I2C1 ������ */
  I2C1_MutexHandle = osMutexNew(&I2C1_Mutex_attributes);
  if (I2C1_MutexHandle == NULL) {
      Error_Handler();     // ����ֱ�ӱ���˵��ϵͳ������
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

  /* creation of MPU6500Task */
  MPU6500TaskHandle = osThreadNew(StartMPU6500Task, NULL, &MPU6500Task_attributes);

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
	ui_init();
	uint32_t tick = 0;
  /* Infinite loop */
  for(;;)
  {
      lv_timer_handler();   // 更新lvgl
      osDelay(5);           // ÿ 5ms ��һ��
			tick += 5;
        if (tick >= 200) {    // ÿ ~200ms ����һ�� UI
            tick = 0;

            // 1) ˢ��ʱ��
            char buf_time[16];
            lv_snprintf(buf_time, sizeof(buf_time), "%02d:%02d:%02d",
                        g_clock_time.hour,
                        g_clock_time.min,
                        g_clock_time.sec);
            lv_label_set_text(ui_labelclock, buf_time);

            // 2) ˢ��Ѫ��
            char buf_spo2[16];
            lv_snprintf(buf_spo2, sizeof(buf_spo2), "SpO2: %d%%", g_spo2_data.spo2);
            lv_label_set_text(ui_Labelspo2, buf_spo2);

            // 3) ˢ������
            char buf_hr[16];
            lv_snprintf(buf_hr, sizeof(buf_hr), "HR: %d", g_spo2_data.heart_rate);
            lv_label_set_text(ui_Labelheart, buf_hr);
        }
				/*TouchEvent ev = g_last_event;
        if (ev != EVENT_NONE) {
            g_last_event = EVENT_NONE;

            switch (ev) {
                case EVENT_SLIDE_LEFT:
                    // �е���һҳ
                    // lv_scr_load_anim(next_screen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 200, 0, false);
                    break;

                case EVENT_SLIDE_RIGHT:
                    // ������һҳ
                    break;

                case EVENT_SLIDE_UP:
                    // ���������˵�
                    break;

                case EVENT_SLIDE_DOWN:
                    // ����״̬��
                    break;

                case EVENT_SINGLE_CLICK:
                    // �����߼���������м䰴ť�͵�����Ļ��
                    break;
                default:
                    break;
            }*/
						//ImuActivity_t act = IMU_GetActivity();
						//printf("act = %d\r\n", act);
						/*switch (act) {
                case IMU_ACTIVITY_REST:
                    // 比如可以认为“用户静止 / 可能在休息”
                    // 可以将来用来做：自动熄屏 / 睡眠检测的一个条件
                    break;

                case IMU_ACTIVITY_LIGHT:
                    // 轻微活动
                    break;

                case IMU_ACTIVITY_WALK:
                    // 走路：可以将来计步
                    break;

                case IMU_ACTIVITY_RUN:
                    // 跑步：运动模式
                    break;

                default:
                    break;
            }*/
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
        // �ȶ�ʱ��
        HAL_RTC_GetTime(&hrtc, &Time_Struct, RTC_FORMAT_BIN);
        // �ٶ����ڣ������һ�β��ܽ����Ĵ�������Ȼ�������ò������ڣ�
        HAL_RTC_GetDate(&hrtc, &Date_Struct, RTC_FORMAT_BIN);

        // д��ȫ��ʱ��ṹ��
        g_clock_time.hour = Time_Struct.Hours+12;
        g_clock_time.min  = Time_Struct.Minutes;
        g_clock_time.sec  = Time_Struct.Seconds;

        // ÿ 200ms ����һ�ξ͹���
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
		static uint32_t ir_buf[BUFFER_SIZE];
    static uint32_t red_buf[BUFFER_SIZE];
    int buf_idx = 0;

    int32_t spo2;
    int8_t  spo2_valid;
    int32_t heart_rate;
    int8_t  hr_valid;

    if (max30102_init() != 0) {
        // ���Դ� log����Ҫ��ѭ��
    }
  /* Infinite loop */
for (;;)
    {
        int samples_this_loop = 0;

        while (samples_this_loop < 4)
        {
            uint8_t intr1;
            if (max30102_read_reg(REG_INTR_STATUS_1, &intr1) != 0) {
                break;
            }

            if (intr1 & 0x40) {
                uint32_t red, ir;
                if (max30102_read_fifo(&red, &ir) == 0)
                {
                    ir_buf[buf_idx]  = ir;
                    red_buf[buf_idx] = red;
                    buf_idx++;
                    samples_this_loop++;

                    if (buf_idx >= BUFFER_SIZE)
                    {
                        maxim_heart_rate_and_oxygen_saturation(
                            ir_buf,
                            red_buf,
                            BUFFER_SIZE,
                            &spo2,
                            &spo2_valid,
                            &heart_rate,
                            &hr_valid);

                        if (spo2_valid) {
                            g_spo2_data.spo2       = spo2;
                            g_spo2_data.spo2_valid = 1;
                        } else {
                            g_spo2_data.spo2_valid = 0;
                        }

                        if (hr_valid) {
                            g_spo2_data.heart_rate = heart_rate;
                            g_spo2_data.hr_valid   = 1;
                        } else {
                            g_spo2_data.hr_valid   = 0;
                        }

                        int keep = BUFFER_SIZE / 2;
                        for (int i = 0; i < keep; i++) {
                            ir_buf[i]  = ir_buf[BUFFER_SIZE - keep + i];
                            red_buf[i] = red_buf[BUFFER_SIZE - keep + i];
                        }
                        buf_idx = keep;
                    }
                }
                else {
                    break;
                }
            }
            else {
                break;
            }
        }

        osDelay(5);
    }
  /* USER CODE END StartMAX30102Task */
}

/* USER CODE BEGIN Header_StartMPU6500Task */
/**
* @brief Function implementing the MPU6500Task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartMPU6500Task */
void StartMPU6500Task(void *argument)
{
  /* USER CODE BEGIN StartMPU6500Task */
    (void)argument;

    /* 确保 I2C 已经初始化，这里调用一次 Init
     * 如果你已经在 main() 里调用过 MPU6500_Init()，这里可以省略或只在失败时重试。
     */
    MPU6500_Init();

    for (;;)
    {
        MPU6500_Update();  // 读取一次数据并更新 g_imu_data
				IMU_Activity_Update();
				/*printf("ACC[g]: ax=%.2f ay=%.2f az=%.2f, "
               "GYRO[dps]: gx=%.2f gy=%.2f gz=%.2f, "
               "T=%.2fC\r\n",
               g_imu_data.ax, g_imu_data.ay, g_imu_data.az,
               g_imu_data.gx, g_imu_data.gy, g_imu_data.gz,
               g_imu_data.temperature);*/
        /* 控制采样频率，这里是 100Hz（10ms 一次），你可以根据需求调整 */
        osDelay(10);
    }
		
  /* USER CODE END StartMPU6500Task */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

