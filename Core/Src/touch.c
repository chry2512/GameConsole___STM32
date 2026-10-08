#include "touch.h"
#include "main.h"
#include "spi.h"
#include <stdio.h>

#define TOUCH_CMD_READ_X 0x90U 
#define TOUCH_CMD_READ_Y 0xD0U 

static void touch_select(void) {
  HAL_GPIO_WritePin(TOUCH_CS_PIN_GPIO_Port, TOUCH_CS_PIN_Pin, GPIO_PIN_RESET);
}

static void touch_deselect(void) {
  HAL_GPIO_WritePin(TOUCH_CS_PIN_GPIO_Port, TOUCH_CS_PIN_Pin, GPIO_PIN_SET);
}

static uint16_t touch_read_channel(uint8_t command);

void Touch_Init(void) {
  touch_deselect();
  printf("\r\n==================================================\r\n");
  printf("[TOUCH DIAGNOSTIC] Avvio test Touch XPT2046 su SPI2\r\n");
  printf("  CS:     PB12\r\n");
  printf("  SCK:    PB13\r\n");
  printf("  MISO:   PB14\r\n");
  printf("  MOSI:   PB15\r\n");
  printf("  PENIRQ: PC5\r\n");

  GPIO_PinState penirq_state = HAL_GPIO_ReadPin(TOUCH_INPUT_PIN_GPIO_Port, TOUCH_INPUT_PIN_Pin);
  printf("[TOUCH DIAGNOSTIC] Stato attuale pin PC5 (PENIRQ): %s (%d)\r\n", 
         (penirq_state == GPIO_PIN_RESET) ? "LOW (Premuto / Rilevato)" : "HIGH (Rilasciato / Idle)", (int)penirq_state);

 
  uint16_t test_x = touch_read_channel(TOUCH_CMD_READ_X);
  uint16_t test_y = touch_read_channel(TOUCH_CMD_READ_Y);
  printf("[TOUCH DIAGNOSTIC] SPI2 Test Read -> Raw X: %u, Raw Y: %u\r\n", test_x, test_y);
  printf("==================================================\r\n\r\n");
}

bool Touch_IsPressed(void) {
  
  return (HAL_GPIO_ReadPin(TOUCH_INPUT_PIN_GPIO_Port, TOUCH_INPUT_PIN_Pin) == GPIO_PIN_RESET);
}

static uint16_t touch_read_channel(uint8_t command) {
  uint8_t tx[3] = {command, 0x00U, 0x00U};
  uint8_t rx[3] = {0U, 0U, 0U};

  touch_select();
  HAL_StatusTypeDef status = HAL_SPI_TransmitReceive(&hspi2, tx, rx, 3U, 10U);
  touch_deselect();

  if (status != HAL_OK) {
    return 0xFFFFU;
  }

  
  uint16_t raw = (((uint16_t)rx[1] << 8) | rx[2]) >> 3;
  return raw & 0x0FFFU;
}

bool Touch_GetCoordinates(uint16_t *x, uint16_t *y) {
  if (x == NULL || y == NULL) return false;
  if (!Touch_IsPressed()) return false;

  
  uint32_t rawX = 0, rawY = 0;
  for (uint8_t i = 0; i < 4U; i++) {
    uint16_t rx = touch_read_channel(TOUCH_CMD_READ_X);
    uint16_t ry = touch_read_channel(TOUCH_CMD_READ_Y);
    if (rx == 0xFFFFU || ry == 0xFFFFU) return false;
    rawX += rx;
    rawY += ry;
  }
  rawX /= 4U;
  rawY /= 4U;

  if (rawX < 100U || rawX > 4000U || rawY < 100U || rawY > 4000U) {
    return false; 
  }

  
  int32_t mappedX = ((int32_t)(rawX - 200) * 320) / 3600;
  int32_t mappedY = ((int32_t)(rawY - 200) * 240) / 3600;

  if (mappedX < 0) mappedX = 0;
  if (mappedX >= 320) mappedX = 319;
  if (mappedY < 0) mappedY = 0;
  if (mappedY >= 240) mappedY = 239;

  *x = (uint16_t)mappedX;
  *y = (uint16_t)mappedY;
  return true;
}

bool Touch_IsCenterTapped(void) {
 
  if (Touch_IsPressed()) {
    uint16_t tx = 0, ty = 0;
    if (Touch_GetCoordinates(&tx, &ty)) {
     
      if (tx >= 40U && tx <= 280U && ty >= 30U && ty <= 210U) {
        return true;
      }
    } else {
      return true;
    }
  }
  return false;
}

