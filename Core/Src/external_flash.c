#include "external_flash.h"
#include "main.h"
#include "gpio.h"
#include "stm32f4xx_hal.h"

#define EXTERNAL_FLASH_CS_PORT FLAS_CS_PIN_GPIO_Port
#define EXTERNAL_FLASH_CS_PIN FLAS_CS_PIN_Pin
#define W25Q16_EXPECTED_ID 0xEF4015UL
#define W25Q16_READ_DATA 0x03U
#define W25Q16_READ_ID 0x9FU
#define W25Q16_WRITE_ENABLE 0x06U
#define W25Q16_READ_STATUS_1 0x05U
#define W25Q16_PAGE_PROGRAM 0x02U
#define W25Q16_SECTOR_ERASE_4K 0x20U
#define W25Q16_BUSY_MASK 0x01U
#define EXTERNAL_FLASH_TIMEOUT_MS 1000U

static bool externalFlashReady = false;

static void flash_select(void) {
  HAL_GPIO_WritePin(EXTERNAL_FLASH_CS_PORT, EXTERNAL_FLASH_CS_PIN, GPIO_PIN_RESET);
}

static void flash_deselect(void) {
  HAL_GPIO_WritePin(EXTERNAL_FLASH_CS_PORT, EXTERNAL_FLASH_CS_PIN, GPIO_PIN_SET);
}

static bool spi2_transfer_byte(uint8_t transmitted, uint8_t *received) {
  uint32_t started = HAL_GetTick();
  while ((SPI2->SR & SPI_SR_TXE) == 0U) {
    if ((HAL_GetTick() - started) >= EXTERNAL_FLASH_TIMEOUT_MS) return false;
  }
  *(__IO uint8_t *)&SPI2->DR = transmitted;
  started = HAL_GetTick();
  while ((SPI2->SR & SPI_SR_RXNE) == 0U) {
    if ((HAL_GetTick() - started) >= EXTERNAL_FLASH_TIMEOUT_MS) return false;
  }
  *received = *(__IO uint8_t *)&SPI2->DR;
  return true;
}

static bool flash_transfer(const uint8_t *tx, uint8_t *rx, uint32_t length) {
  for (uint32_t index = 0U; index < length; index++) {
    uint8_t received = 0U;
    if (!spi2_transfer_byte(tx == NULL ? 0xFFU : tx[index], &received)) return false;
    if (rx != NULL) rx[index] = received;
  }
  return true;
}

static bool flash_write_enable(void) {
  uint8_t command = W25Q16_WRITE_ENABLE;
  flash_select();
  bool status = flash_transfer(&command, NULL, 1U);
  flash_deselect();
  return status;
}

static bool flash_wait_ready(void) {
  uint8_t command[2] = {W25Q16_READ_STATUS_1, 0U};
  uint8_t response[2] = {0U, 0U};
  uint32_t started = HAL_GetTick();
  do {
    flash_select();
    bool transferred = flash_transfer(command, response, sizeof(command));
    flash_deselect();
    if (!transferred) return false;
    if ((response[1] & W25Q16_BUSY_MASK) == 0U) return true;
  } while ((HAL_GetTick() - started) < EXTERNAL_FLASH_TIMEOUT_MS);
  return false;
}

/* FLASH MEMORY ESTERNA: programmazione pagine W25Q16 per il provisioning degli asset. */
static bool flash_program_page(uint32_t address, const uint8_t *data, uint16_t length) {
  uint8_t command[4] = {
    W25Q16_PAGE_PROGRAM,
    (uint8_t)(address >> 16),
    (uint8_t)(address >> 8),
    (uint8_t)address
  };
  if (!flash_write_enable()) return false;
  flash_select();
  bool transferred = flash_transfer(command, NULL, sizeof(command)) &&
                     flash_transfer(data, NULL, length);
  flash_deselect();
  return transferred && flash_wait_ready();
}

/* FLASH MEMORY ESTERNA: inizializzazione SPI2 e verifica JEDEC W25Q16. */
bool ExternalFlash_Init(void) {
  __HAL_RCC_SPI2_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  GPIO_InitTypeDef pins = {0};
  pins.Pin = GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
  pins.Mode = GPIO_MODE_AF_PP;
  pins.Pull = GPIO_NOPULL;
  pins.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  pins.Alternate = GPIO_AF5_SPI2;
  HAL_GPIO_Init(GPIOB, &pins);

  SPI2->CR1 = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI | SPI_CR1_BR_1;
  SPI2->CR2 = 0U;
  SPI2->CR1 |= SPI_CR1_SPE;

  uint8_t command = W25Q16_READ_ID;
  uint8_t id[3] = {0U, 0U, 0U};
  flash_select();
  bool transferred = flash_transfer(&command, NULL, 1U) &&
                     flash_transfer(NULL, id, sizeof(id));
  flash_deselect();
  uint32_t jedecId = ((uint32_t)id[0] << 16) | ((uint32_t)id[1] << 8) | id[2];
  externalFlashReady = transferred && jedecId == W25Q16_EXPECTED_ID;
  return externalFlashReady;
}

bool ExternalFlash_IsReady(void) {
  return externalFlashReady;
}

bool ExternalFlash_Read(uint32_t address, void *data, uint32_t length) {
  if (!externalFlashReady || data == NULL || length == 0U) return false;
  uint8_t command[4] = {
    W25Q16_READ_DATA,
    (uint8_t)(address >> 16),
    (uint8_t)(address >> 8),
    (uint8_t)address
  };
  flash_select();
  bool transferred = flash_transfer(command, NULL, sizeof(command)) &&
                     flash_transfer(NULL, data, length);
  flash_deselect();
  return transferred;
}

bool ExternalFlash_EraseSector(uint32_t address) {
  if (!externalFlashReady) return false;
  uint8_t command[4] = {
    W25Q16_SECTOR_ERASE_4K,
    (uint8_t)(address >> 16),
    (uint8_t)(address >> 8),
    (uint8_t)address
  };
  if (!flash_write_enable()) return false;
  flash_select();
  bool transferred = flash_transfer(command, NULL, sizeof(command));
  flash_deselect();
  return transferred && flash_wait_ready();
}

bool ExternalFlash_Program(uint32_t address, const void *data, uint32_t length) {
  if (!externalFlashReady || data == NULL || length == 0U) return false;
  const uint8_t *bytes = (const uint8_t *)data;
  while (length > 0U) {
    uint16_t pageOffset = (uint16_t)(address & 0xFFU);
    uint16_t chunk = (uint16_t)(256U - pageOffset);
    if (chunk > length) chunk = (uint16_t)length;
    if (!flash_program_page(address, bytes, chunk)) return false;
    address += chunk;
    bytes += chunk;
    length -= chunk;
  }
  return true;
}

bool ExternalFlash_ReadImagePixel(uint32_t imageAddress, uint16_t x, uint16_t y,
                                  uint16_t *color) {
  if (color == NULL || x >= EXTERNAL_FLASH_IMAGE_WIDTH ||
      y >= EXTERNAL_FLASH_IMAGE_HEIGHT) return false;
  uint32_t address = imageAddress + ((uint32_t)y * EXTERNAL_FLASH_IMAGE_WIDTH + x) * 2U;
  uint8_t pixel[2] = {0U, 0U};
  if (!ExternalFlash_Read(address, pixel, sizeof(pixel))) return false;
  *color = ((uint16_t)pixel[0] << 8) | pixel[1];
  return true;
}
