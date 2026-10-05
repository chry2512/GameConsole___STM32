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
osMutexId_t gameMutexHandle;
const osMutexAttr_t gameMutex_attributes = {
  .name = "gameMutex"
};
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
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for SensorTask */
osThreadId_t SensorTaskHandle;
const osThreadAttr_t SensorTask_attributes = {
  .name = "SensorTask",
  .stack_size = 128 * 4,
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

static void handle_joystick_button(void)
{
  switch (Console_GetState(&console)) {
    case CONSOLE_STATE_OFF:
      Console_HandleEvent(&console, &game, CONSOLE_EVENT_POWER, '\0');
      break;
    case CONSOLE_STATE_START:
      Console_HandleEvent(&console, &game, CONSOLE_EVENT_X, '\0');
      break;
    case CONSOLE_STATE_NAME:
      Console_HandleEvent(&console, &game, CONSOLE_EVENT_NAME_CONFIRM, '\0');
      break;
    case CONSOLE_STATE_DIFFICULTY:
      Console_HandleEvent(&console, &game, CONSOLE_EVENT_LEVEL_EASY, '\0');
      break;
    case CONSOLE_STATE_GAME:
      if (game.state == GAME_STATE_GAMEOVER) Console_HandleEvent(&console, &game, CONSOLE_EVENT_RESTART, '\0');
      else Console_HandleEvent(&console, &game, CONSOLE_EVENT_PAUSE, '\0');
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
  if (Console_GetState(&console) == CONSOLE_STATE_DIFFICULTY) {
    if (x > JOYSTICK_CENTER_MAX + JOYSTICK_DEADZONE) Console_HandleEvent(&console, &game, CONSOLE_EVENT_DOWN, '\0');
    else if (x + JOYSTICK_DEADZONE < JOYSTICK_CENTER_MIN) Console_HandleEvent(&console, &game, CONSOLE_EVENT_UP, '\0');
    return;
  }
  if (Console_GetState(&console) != CONSOLE_STATE_GAME) return;
  if (x > JOYSTICK_CENTER_MAX + JOYSTICK_DEADZONE) Console_HandleEvent(&console, &game, CONSOLE_EVENT_RIGHT, '\0');
  else if (x + JOYSTICK_DEADZONE < JOYSTICK_CENTER_MIN) Console_HandleEvent(&console, &game, CONSOLE_EVENT_LEFT, '\0');
  else if (y > JOYSTICK_CENTER_MAX + JOYSTICK_DEADZONE) Console_HandleEvent(&console, &game, CONSOLE_EVENT_DOWN, '\0');
  else if (y + JOYSTICK_DEADZONE < JOYSTICK_CENTER_MIN) Console_HandleEvent(&console, &game, CONSOLE_EVENT_UP, '\0');
}

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
  joystickQueueHandle = osMessageQueueNew (1, sizeof(JoystickData_t), &joystickQueue_attributes);

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
    osMessageQueuePut(joystickQueueHandle, &input, 0U, osWaitForever);
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
  /* Infinite loop */
  while (1)
  {
    JoystickData_t input = {0};
    while (osMessageQueueGet(joystickQueueHandle, &input, NULL, 0U) == osOK) {
      if (osMutexAcquire(gameMutexHandle, osWaitForever) == osOK) {
        if (input.buttonPressed != 0U) handle_joystick_button();
        else handle_joystick_direction(&input);
        osMutexRelease(gameMutexHandle);
      }
    }
    uint32_t now = osKernelGetTickCount();
    if (now - lastUpdate >= Snake_GetMoveIntervalMs(&game)) {
      if (osMutexAcquire(gameMutexHandle, osWaitForever) == osOK) {
        Snake_Update(&game);
        lastUpdate = now;
        osMutexRelease(gameMutexHandle);
      }
    }
    if (osMutexAcquire(gameMutexHandle, osWaitForever) == osOK) {
      if (game.state == GAME_STATE_LEVEL_TRANSITION) Snake_AdvanceTransition(&game, LOGIC_TASK_PERIOD);
      osMutexRelease(gameMutexHandle);
    }
    osDelay(LOGIC_TASK_PERIOD);
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
/* USER CODE BEGIN StartRenderTask */
void StartRenderTask(void *argument)
{
  (void) argument;
  
  // 1. Inizializzazione Hardware Display
  LCD_Init();

  /* Infinite loop */
  while (1)
  {
    // Proteggiamo l'accesso alle risorse condivise con il Mutex
    if (osMutexAcquire(gameMutexHandle, pdMS_TO_TICKS(20)) == osOK) {
      LCD_Render(&game, &console);  
      osMutexRelease(gameMutexHandle);
    }

    osDelay(33); // Circa 30 FPS stabili
  }
}
/* USER CODE END StartRenderTask */

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
  /* Infinite loop */
  while (1)
  {
    osDelay(1);
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

void StartPowerManagerTask(void *argument)
{
  (void) argument;

  while (1)
  {
    osSemaphoreAcquire(wakeupSemaphoreHandlerHandle, osWaitForever);
    __HAL_GPIO_EXTI_CLEAR_IT(BTN_ON_OFF_Pin);
    NVIC_ClearPendingIRQ(BTN_ON_OFF_EXTI_IRQn);
    stopModeActive = 1U;
    vTaskSuspendAll();
    Enter_StopMode_Process();
    xTaskResumeAll();
    stopModeActive = 0U;
  }
}

/* USER CODE END Application */

