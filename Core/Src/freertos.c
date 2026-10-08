/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
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

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "adc.h"
#include "console.h"
#include "gpio.h"
#include "lcd.h"
#include "touch.h"
#include "external_flash.h"
#include "assets_data.h"
#include "bme280.h"
#include <stdio.h>
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
static Game_t game;
static Console_t console;

// Mutex protecting game and console data across tasks
osMutexId_t gameMutexHandle;
const osMutexAttr_t gameMutex_attributes = {
  .name = "gameMutex"
};

// Power management task and active low-power state flag
static volatile uint8_t stopModeActive = 0U;
osThreadId_t PowerManagerTaskHandle;
const osThreadAttr_t PowerManagerTask_attributes = {
  .name = "PowerManagerTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityAboveNormal,
};
/* USER CODE END Variables */
/* Definitions for inputTask */
osThreadId_t inputTaskHandle;
const osThreadAttr_t inputTask_attributes = {
  .name = "inputTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for GameLogicTask */
osThreadId_t GameLogicTaskHandle;
const osThreadAttr_t GameLogicTask_attributes = {
  .name = "GameLogicTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityAboveNormal,
};
/* Definitions for RenderTask */
osThreadId_t RenderTaskHandle;
const osThreadAttr_t RenderTask_attributes = {
  .name = "RenderTask",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for SensorTask */
osThreadId_t SensorTaskHandle;
const osThreadAttr_t SensorTask_attributes = {
  .name = "SensorTask",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for joystickQueue */
osMessageQueueId_t joystickQueueHandle;
const osMessageQueueAttr_t joystickQueue_attributes = {
  .name = "joystickQueue"
};
/* Definitions for wakeupSemaphoreHandler */
osSemaphoreId_t wakeupSemaphoreHandlerHandle;
const osSemaphoreAttr_t wakeupSemaphoreHandler_attributes = {
  .name = "wakeupSemaphoreHandler"
};
/* Definitions for gameEvents */
osEventFlagsId_t gameEventsHandle;
const osEventFlagsAttr_t gameEvents_attributes = {
  .name = "gameEvents"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void StartPowerManagerTask(void *argument);
static void handle_joystick_direction(const JoystickData_t *input);
static void handle_joystick_button(void);
/* USER CODE END FunctionPrototypes */

void StartInputTask(void *argument);
void StartGameLogicTask(void *argument);
void StartRenderTask(void *argument);
void StartSensorTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  Console_Init(&console, &game);
  gameMutexHandle = osMutexNew(&gameMutex_attributes);

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* Create the semaphores(s) */
  /* creation of wakeupSemaphoreHandler */
  wakeupSemaphoreHandlerHandle = osSemaphoreNew(1, 0, &wakeupSemaphoreHandler_attributes);

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of joystickQueue */
  joystickQueueHandle = osMessageQueueNew (4, sizeof(JoystickData_t), &joystickQueue_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of inputTask */
  inputTaskHandle = osThreadNew(StartInputTask, NULL, &inputTask_attributes);

  /* creation of GameLogicTask */
  GameLogicTaskHandle = osThreadNew(StartGameLogicTask, NULL, &GameLogicTask_attributes);

  /* creation of RenderTask */
  RenderTaskHandle = osThreadNew(StartRenderTask, NULL, &RenderTask_attributes);

  /* creation of SensorTask */
  SensorTaskHandle = osThreadNew(StartSensorTask, NULL, &SensorTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  PowerManagerTaskHandle = osThreadNew(StartPowerManagerTask, NULL, &PowerManagerTask_attributes);
  /* USER CODE END RTOS_THREADS */

  /* Create the event(s) */
  /* creation of gameEvents */
  gameEventsHandle = osEventFlagsNew(&gameEvents_attributes);

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartInputTask */
/**
  * @brief  Function implementing the inputTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartInputTask */
void StartInputTask(void *argument)
{
  /* USER CODE BEGIN StartInputTask */
  (void) argument;
  uint8_t previousButton = 0U;
  /* Infinite loop */
  while (1)
  {
    JoystickData_t input = {0};
    ADC_ReadJoystick(&input.x, &input.y);
    input.buttonPressed = GPIO_ReadJoystickButton() == JOYSTICK_BUTTON_ACTIVE ? 1U : 0U;
    uint8_t buttonState = input.buttonPressed;
    input.buttonPressed = (buttonState != 0U && previousButton == 0U) ? 1U : 0U;
    previousButton = buttonState;
    osMessageQueuePut(joystickQueueHandle, &input, 0U, 0U);
    osDelay(JOYSTICK_SAMPLE_PERIOD_MS);
  }
  /* USER CODE END StartInputTask */
}

/* USER CODE BEGIN Header_StartGameLogicTask */
/**
* @brief Function implementing the GameLogicTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartGameLogicTask */
void StartGameLogicTask(void *argument)
{
  /* USER CODE BEGIN StartGameLogicTask */
  (void) argument;
  uint32_t lastUpdate = osKernelGetTickCount();
  const uint32_t period = LOGIC_TASK_PERIOD;
  /* Infinite loop */
  while (1)
  {
    if (osMutexAcquire(gameMutexHandle, osWaitForever) == osOK)
    {
      // 1. JOYSTICK INPUT
      JoystickData_t input = {0};
      while (osMessageQueueGet(joystickQueueHandle, &input, NULL, 0U) == osOK) {
        if (input.buttonPressed != 0U) {
          handle_joystick_button();
        } else {
          handle_joystick_direction(&input);
        }
      }

      // 1.1 TOUCH SCREEN INPUT (Tocca qualsiasi punto in OFF o START per avanzare allo step successivo)
      static bool touch_was_pressed = false;
      bool touch_now = Touch_IsPressed();

      // Debug periodico (ogni 3 secondi in stato OFF o START) per monitorare lo stato del pin PC5
      static uint32_t last_touch_debug_tick = 0U;
      uint32_t current_tick = osKernelGetTickCount();
      ConsoleState_t cState = Console_GetState(&console);
      if ((cState == CONSOLE_STATE_OFF || cState == CONSOLE_STATE_START) && (current_tick - last_touch_debug_tick >= 3000U)) {
        printf("[TOUCH MONITOR] PC5 (PENIRQ) = %d | Touch_IsPressed = %d\r\n", 
               (int)HAL_GPIO_ReadPin(TOUCH_INPUT_PIN_GPIO_Port, TOUCH_INPUT_PIN_Pin), (int)touch_now);
        last_touch_debug_tick = current_tick;
      }

      if (touch_now && !touch_was_pressed) {
        printf("[TOUCH EVENT] Rilevata pressione dello schermo (PC5 LOW)!\r\n");
        if (cState == CONSOLE_STATE_OFF) {
          printf("[TOUCH] Schermo toccato in stato OFF -> Passaggio a LOAD\r\n");
          Console_HandleEvent(&console, &game, CONSOLE_EVENT_POWER, '\0');
        } else if (cState == CONSOLE_STATE_START) {
          printf("[TOUCH] Schermo toccato in stato START -> Passaggio a NAME\r\n");
          Console_HandleEvent(&console, &game, CONSOLE_EVENT_X, '\0');
        }
      }
      touch_was_pressed = touch_now;

      // 2. CONSOLE STATE MACHINE

      static ConsoleState_t last_logged_console_state = (ConsoleState_t)-1;
      ConsoleState_t currentState = Console_GetState(&console);
      if (currentState != last_logged_console_state) {
        const char *stateNames[] = {"OFF", "LOAD", "START", "NAME", "DIFFICULTY", "GAME", "LEADERBOARD"};
        printf("[CONSOLE] State changed to: %s\r\n", 
               (currentState <= CONSOLE_STATE_LEADERBOARD) ? stateNames[currentState] : "UNKNOWN");
        last_logged_console_state = currentState;
      }

     
      static ConsoleState_t prev_check_state = CONSOLE_STATE_OFF;
      static uint32_t load_start_tick = 0U;

      if (currentState == CONSOLE_STATE_LOAD) {
        if (prev_check_state != CONSOLE_STATE_LOAD) {
          load_start_tick = osKernelGetTickCount();
        }

#if ENABLE_INTERNAL_ASSETS_DATA
        // In modalita' asset interni: passa a START solo se completata la verifica Flash al 100%
        // E sono trascorsi almeno 3 secondi per mostrare il logo Cubeniro
        if (ExternalFlash_AreAssetsLoaded() && ((osKernelGetTickCount() - load_start_tick) >= 3000U)) {
          printf("[LOAD] Caricamento Flash esterna verificato al 100%% -> passaggio a START\r\n");
          Console_HandleEvent(&console, &game, CONSOLE_EVENT_BUTTON_SELECT, '\0');
        }
#else
        // In modalita' Flash esterna gia' programmata: timeout standard di 5.0s
        else if ((osKernelGetTickCount() - load_start_tick) >= 5000U) {
          printf("[LOAD] Timeout 5.0s completato -> passaggio automatico a START\r\n");
          Console_HandleEvent(&console, &game, CONSOLE_EVENT_BUTTON_SELECT, '\0');
        }
#endif
      }
      prev_check_state = currentState;
      uint32_t now = osKernelGetTickCount();
      static uint32_t last_logged_score = 0U;
      static uint16_t last_logged_stage = 0U;
      if (Console_GetState(&console) == CONSOLE_STATE_GAME && game.state == GAME_STATE_RUNNING) {
        uint16_t moveInterval = Snake_GetMoveIntervalMs(&game);
        if (now - lastUpdate >= moveInterval) {
          Snake_Update(&game);
          lastUpdate = now;
        }
        if (game.score != last_logged_score || game.stage != last_logged_stage) {
          printf("[SNAKE] Score: %lu | Stage: %u | Length: %u\r\n", 
                 (unsigned long)game.score, (unsigned int)game.stage, (unsigned int)game.snake.length);
          last_logged_score = game.score;
          last_logged_stage = game.stage;
        }
      } else {
        lastUpdate = now;
      }

      if (game.state == GAME_STATE_LEVEL_TRANSITION) {
        Snake_AdvanceTransition(&game, (uint16_t)period);
      }

      osMutexRelease(gameMutexHandle);
    }

    osDelay(period);
  }
  /* USER CODE END StartGameLogicTask */
}

/* USER CODE BEGIN Header_StartRenderTask */
/**
* @brief Function implementing the RenderTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartRenderTask */
void StartRenderTask(void *argument)
{
  /* USER CODE BEGIN StartRenderTask */
  (void) argument;
  

  LCD_Init();
  Touch_Init();
  ExternalFlash_Init();
  ExternalFlash_EnsureAssetsProgrammed();

  /* Infinite loop */
  while (1)
  {
    if (osMutexAcquire(gameMutexHandle, osWaitForever) == osOK) {
      LCD_Render(&game, &console);  
      osMutexRelease(gameMutexHandle);
    }

    osDelay(RENDER_TASK_PERIOD); 
  }
  /* USER CODE END StartRenderTask */
}

/* USER CODE BEGIN Header_StartSensorTask */
/**
* @brief Function implementing the SensorTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartSensorTask */
void StartSensorTask(void *argument)
{
  /* USER CODE BEGIN StartSensorTask */
  (void) argument;


  osDelay(150U);

  bool sensor_ready = false;

  /* Infinite loop */
  while (1)
  {
    // Leggiamo e stampiamo i dati del sensore SOLO quando la console e' in stato CONSOLE_STATE_OFF
    ConsoleState_t currState = Console_GetState(&console);

    if (currState == CONSOLE_STATE_OFF) {
      if (!sensor_ready) {
        sensor_ready = BME280_Init();
        if (sensor_ready) {
          BME280_Data_t initData;
          BME280_GetLatestData(&initData);
          if (initData.type == SENSOR_TYPE_BME280) {
            printf("[SENSOR] [CONNESSO] Sensore BME280 riconosciuto e attivo (T, P, RH)!\r\n");
          } else {
            printf("[SENSOR] [CONNESSO] Sensore BMP280 riconosciuto e attivo (T, P)!\r\n");
          }
        }
      } else {
        BME280_Data_t data;
        if (BME280_Read(&data)) {
          int32_t t_int = (int32_t)data.temperature;
          int32_t t_dec = (int32_t)((data.temperature - (float)t_int) * 10.0f);
          if (t_dec < 0) t_dec = -t_dec;

          int32_t p_int = (int32_t)data.pressure;
          int32_t p_dec = (int32_t)((data.pressure - (float)p_int) * 10.0f);
          if (p_dec < 0) p_dec = -p_dec;

          if (data.type == SENSOR_TYPE_BME280) {
            int32_t h_int = (int32_t)data.humidity;
            int32_t h_dec = (int32_t)((data.humidity - (float)h_int) * 10.0f);
            if (h_dec < 0) h_dec = -h_dec;

            printf("[SENSOR] BME280 -> Temp: %ld.%ld C | Press: %ld.%ld hPa | Umidita': %ld.%ld %%\r\n",
                   (long)t_int, (long)t_dec, (long)p_int, (long)p_dec, (long)h_int, (long)h_dec);
          } else {
            printf("[SENSOR] BMP280 -> Temp: %ld.%ld C | Press: %ld.%ld hPa\r\n",
                   (long)t_int, (long)t_dec, (long)p_int, (long)p_dec);
          }
        } else {
          printf("[SENSOR] [ERRORE] Comunicazione I2C persa, tentero' la riconnessione...\r\n");
          sensor_ready = false;
        }
      }
    }

    osDelay(2000U); 
  }
  /* USER CODE END StartSensorTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == BTN_ON_OFF_Pin && stopModeActive == 0U)
  {
    osSemaphoreRelease(wakeupSemaphoreHandlerHandle);
  }
}

//ON-OFF BUTTON TASK

void StartPowerManagerTask(void *argument)
{
  (void) argument;

  while (1)
  {
    osSemaphoreAcquire(wakeupSemaphoreHandlerHandle, osWaitForever);
    __HAL_GPIO_EXTI_CLEAR_IT(BTN_ON_OFF_Pin);
    NVIC_ClearPendingIRQ(BTN_ON_OFF_EXTI_IRQn);
    stopModeActive = 1U;

    // Safely wait for any in-flight frame rendering or SPI flash read to finish
    osMutexAcquire(gameMutexHandle, osWaitForever);
    vTaskSuspendAll();
    Enter_StopMode_Process();
    xTaskResumeAll();
    osMutexRelease(gameMutexHandle);

    stopModeActive = 0U;
  }
}

//HANDLE JOYSTICK INPUTS

static void handle_joystick_button(void)
{
  const char *stateNames[] = {"OFF", "LOAD", "START", "NAME", "DIFFICULTY", "GAME", "LEADERBOARD"};
  ConsoleState_t st = Console_GetState(&console);
  const char *stName = (st <= CONSOLE_STATE_LEADERBOARD) ? stateNames[st] : "UNKNOWN";
  printf("[INPUT] JOYSTICK BTN X premuto (Stato: %s)\r\n", stName);

  switch (st) {
    case CONSOLE_STATE_OFF:
      Console_HandleEvent(&console, &game, CONSOLE_EVENT_POWER, '\0');
      break;
    case CONSOLE_STATE_LOAD:
      Console_HandleEvent(&console, &game, CONSOLE_EVENT_BUTTON_SELECT, '\0');
      break;
    case CONSOLE_STATE_START:
      Console_HandleEvent(&console, &game, CONSOLE_EVENT_X, '\0');
      break;
    case CONSOLE_STATE_NAME:
      Console_HandleEvent(&console, &game, CONSOLE_EVENT_BUTTON_SELECT, '\0');
      break;
    case CONSOLE_STATE_DIFFICULTY:
      Console_HandleEvent(&console, &game, CONSOLE_EVENT_BUTTON_SELECT, '\0');
      break;
    case CONSOLE_STATE_GAME:
      Console_HandleEvent(&console, &game, CONSOLE_EVENT_BUTTON_SELECT, '\0');
      break;
    case CONSOLE_STATE_LEADERBOARD:
      Console_HandleEvent(&console, &game, CONSOLE_EVENT_BUTTON_SELECT, '\0');
      break;
    default:
      break;
  }
}

static void handle_joystick_direction(const JoystickData_t *input)
{
  if (input == NULL) return;
  uint16_t x = input->x;
  uint16_t y = input->y;

 
  int32_t dx = (int32_t)x - 2048;
  int32_t dy = (int32_t)y - 2048;

  int32_t abs_dx = (dx >= 0) ? dx : -dx;
  int32_t abs_dy = (dy >= 0) ? dy : -dy;

  static uint8_t menu_held = 0U;
  static int8_t last_logged_dir = -1; 
  ConsoleState_t cState = Console_GetState(&console);

  #define JOY_SECTOR_DEADZONE 350
  if (abs_dx < JOY_SECTOR_DEADZONE && abs_dy < JOY_SECTOR_DEADZONE) {

    menu_held = 0U;
    last_logged_dir = -1;
    return;
  }


  uint8_t left_deflected  = 0U;
  uint8_t right_deflected = 0U;
  uint8_t up_deflected    = 0U;
  uint8_t down_deflected  = 0U;
  int8_t current_dir = -1;

  if (abs_dx >= abs_dy) {
    //Left o Right
    if (dx > 0) {
      left_deflected = 1U;
      current_dir = 2; // LEFT
    } else {
      right_deflected = 1U;
      current_dir = 3; // RIGHT
    }
  } else {
    // Up o Down
    if (dy > 0) {
      down_deflected = 1U;
      current_dir = 1; // DOWN
    } else {
      up_deflected = 1U;
      current_dir = 0; // UP
    }
  }

  // LOG
  if (current_dir != last_logged_dir) {
    const char *dirNames[] = {"UP", "DOWN", "LEFT", "RIGHT"};
    if (current_dir >= 0 && current_dir < 4) {
      printf("[INPUT] JOYSTICK DIR: %s\r\n", dirNames[current_dir]);
    }
    last_logged_dir = current_dir;
  }

  if (cState == CONSOLE_STATE_NAME || 
      cState == CONSOLE_STATE_DIFFICULTY || 
      cState == CONSOLE_STATE_LEADERBOARD ||
      (cState == CONSOLE_STATE_GAME && (game.state == GAME_STATE_PAUSED || game.state == GAME_STATE_GAMEOVER))) {

    if (menu_held == 0U) {
      if (up_deflected != 0U) {
        Console_HandleEvent(&console, &game, CONSOLE_EVENT_UP, '\0');
        menu_held = 1U;
      } else if (down_deflected != 0U) {
        Console_HandleEvent(&console, &game, CONSOLE_EVENT_DOWN, '\0');
        menu_held = 1U;
      } else if (left_deflected != 0U) {
        Console_HandleEvent(&console, &game, CONSOLE_EVENT_LEFT, '\0');
        menu_held = 1U;
      } else if (right_deflected != 0U) {
        Console_HandleEvent(&console, &game, CONSOLE_EVENT_RIGHT, '\0');
        menu_held = 1U;
      }
    }
    return;
  }

  if (cState != CONSOLE_STATE_GAME) return;

 
  if (up_deflected != 0U) Console_HandleEvent(&console, &game, CONSOLE_EVENT_UP, '\0');
  else if (down_deflected != 0U) Console_HandleEvent(&console, &game, CONSOLE_EVENT_DOWN, '\0');
  else if (left_deflected != 0U) Console_HandleEvent(&console, &game, CONSOLE_EVENT_LEFT, '\0');
  else if (right_deflected != 0U) Console_HandleEvent(&console, &game, CONSOLE_EVENT_RIGHT, '\0');
}
/* USER CODE END Application */

