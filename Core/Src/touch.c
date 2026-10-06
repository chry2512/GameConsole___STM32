#include "touch.h"
#include "main.h"
#include "spi.h"

#define TOUCH_CMD_READ_X 0x90U // Canale X, 12-bit, Differential
#define TOUCH_CMD_READ_Y 0xD0U // Canale Y, 12-bit, Differential

static void touch_select(void) {
  HAL_GPIO_WritePin(TOUCH_CS_PIN_GPIO_Port, TOUCH_CS_PIN_Pin, GPIO_PIN_RESET);
}

static void touch_deselect(void) {
  HAL_GPIO_WritePin(TOUCH_CS_PIN_GPIO_Port, TOUCH_CS_PIN_Pin, GPIO_PIN_SET);
}

void Touch_Init(void) {
  touch_deselect();
}

bool Touch_IsPressed(void) {
  // Il pin TOUCH_INPUT_PIN (PD6) e' attivo basso (PENIRQ) quando il touch resistivo viene premuto
  return (HAL_GPIO_ReadPin(TOUCH_INPUT_PIN_GPIO_Port, TOUCH_INPUT_PIN_Pin) == GPIO_PIN_RESET);
}

static void touch_spi_slow_down(void) {
  // Imposta prescaler a 32 (~1.3 MHz su bus APB2 42MHz) per non superare il limite di 2 MHz dell'XPT2046
  hspi1.Instance->CR1 &= ~SPI_CR1_SPE;
  hspi1.Instance->CR1 = (hspi1.Instance->CR1 & ~SPI_CR1_BR_Msk) | SPI_BAUDRATEPRESCALER_32;
  hspi1.Instance->CR1 |= SPI_CR1_SPE;
}

static void touch_spi_restore_fast(void) {
  // Ripristina prescaler a 4 (~10.5 MHz / 21 MHz) per la Flash SPI esterna
  hspi1.Instance->CR1 &= ~SPI_CR1_SPE;
  hspi1.Instance->CR1 = (hspi1.Instance->CR1 & ~SPI_CR1_BR_Msk) | SPI_BAUDRATEPRESCALER_4;
  hspi1.Instance->CR1 |= SPI_CR1_SPE;
}

static uint16_t touch_read_channel(uint8_t command) {
  uint8_t tx[3] = {command, 0x00U, 0x00U};
  uint8_t rx[3] = {0U, 0U, 0U};

  touch_spi_slow_down();
  touch_select();
  HAL_SPI_TransmitReceive(&hspi1, tx, rx, 3U, 10U);
  touch_deselect();
  touch_spi_restore_fast();

  // I 12 bit sono compresi tra il secondo e il terzo byte ricevuto
  uint16_t raw = (((uint16_t)rx[1] << 8) | rx[2]) >> 3;
  return raw & 0x0FFFU;
}

bool Touch_GetCoordinates(uint16_t *x, uint16_t *y) {
  if (x == NULL || y == NULL) return false;
  if (!Touch_IsPressed()) return false;

  // Campiona con una media su 4 letture
  uint32_t rawX = 0, rawY = 0;
  for (uint8_t i = 0; i < 4U; i++) {
    rawX += touch_read_channel(TOUCH_CMD_READ_X);
    rawY += touch_read_channel(TOUCH_CMD_READ_Y);
  }
  rawX /= 4U;
  rawY /= 4U;

  if (rawX < 150U || rawX > 3900U || rawY < 150U || rawY > 3900U) {
    return false; // Lettura fuori range o rumore
  }

  // Mappatura calibrata XPT2046 a risoluzione display 320x240 Landscape
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
  // Se il pin PENIRQ (PD6) e' premuto:
  if (Touch_IsPressed()) {
    uint16_t tx = 0, ty = 0;
    if (Touch_GetCoordinates(&tx, &ty)) {
      // Se le coordinate sono campionate con successo, controlliamo il centro
      if (tx >= 40U && tx <= 280U && ty >= 30U && ty <= 210U) {
        return true;
      }
    } else {
      // Se PENIRQ e' fisicamente premuto ma il rumore sporca le coordinate esatte,
      // per la conferma a centro schermo negli stati menu accettiamo la pressione
      return true;
    }
  }
  return false;
}

