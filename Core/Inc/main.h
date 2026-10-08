/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
typedef struct {
  uint16_t x;
  uint16_t y;
  uint8_t buttonPressed;
} JoystickData_t;

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */
#define JOYSTICK_SAMPLE_PERIOD_MS INPUT_TASK_PERIOD
/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */
void Enter_StopMode_Process(void);
/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define UART_TX_TIMEOUT 100
#define JOYSTICK_CENTER_MIN 1800
#define BMP280_I2C_ADDRESS 0x76
#define INPUT_TASK_PERIOD 50
#define UART_BAUD_RATE 115200
#define TFT_HEIGHT 240
#define EVENT_FLAG_GAMEOVER (1U << 1)
#define SENSOR_POLL_INTERVAL_MS 1000
#define JOYSTICK_CENTER_MAX 2200
#define TFT_WIDTH 320
#define RENDER_TASK_PERIOD 33
#define EVENT_FLAG_PAUSE (1U << 0)
#define LOGIC_TASK_PERIOD 20
#define BTN_ON_OFF_Pin GPIO_PIN_3
#define BTN_ON_OFF_GPIO_Port GPIOE
#define BTN_ON_OFF_EXTI_IRQn EXTI3_IRQn
#define ON_LED_Pin GPIO_PIN_6
#define ON_LED_GPIO_Port GPIOA
#define ERROR_LED_Pin GPIO_PIN_7
#define ERROR_LED_GPIO_Port GPIOA
#define FLAS_CS_PIN_Pin GPIO_PIN_0
#define FLAS_CS_PIN_GPIO_Port GPIOB
#define TOUCH_CS_PIN_Pin GPIO_PIN_12
#define TOUCH_CS_PIN_GPIO_Port GPIOB
#define TOUCH_INPUT_PIN_Pin GPIO_PIN_5
#define TOUCH_INPUT_PIN_GPIO_Port GPIOC
#define TOUCH_INPUT_PIN_EXTI_IRQn EXTI9_5_IRQn
#define JOY_BTN_PIN_Pin GPIO_PIN_1
#define JOY_BTN_PIN_GPIO_Port GPIOE

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
