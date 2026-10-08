#include "external_flash.h"
#include "assets_data.h"
#include "main.h"
#include "gpio.h"
#include "spi.h"
#include "stm32f4xx_hal.h"
#include <stdio.h>
#include <string.h>

#define EXTERNAL_FLASH_CS_PORT FLAS_CS_PIN_GPIO_Port
#define EXTERNAL_FLASH_CS_PIN  FLAS_CS_PIN_Pin
#define W25Q16_EXPECTED_ID     0xEF4015UL
#define W25Q16_READ_DATA       0x03U
#define W25Q16_FAST_READ       0x0BU
#define W25Q16_READ_ID         0x9FU
#define W25Q16_RELEASE_POWERDOWN 0xABU
#define W25Q16_WRITE_ENABLE    0x06U
#define W25Q16_READ_STATUS_1   0x05U
#define W25Q16_PAGE_PROGRAM    0x02U
#define W25Q16_SECTOR_ERASE_4K 0x20U
#define W25Q16_BUSY_MASK       0x01U
#define EXTERNAL_FLASH_TIMEOUT_MS 500U

static bool externalFlashReady = false;

static GPIO_TypeDef *flash_cs_port = FLAS_CS_PIN_GPIO_Port;
static uint16_t flash_cs_pin = FLAS_CS_PIN_Pin;

/**
 * SELECT FLASH CS PIN
 */
static void flash_select(void) {
  HAL_GPIO_WritePin(flash_cs_port, flash_cs_pin, GPIO_PIN_RESET);
}

/**
 * DESELECT FLASH CS PIN
 */
static void flash_deselect(void) {
  HAL_GPIO_WritePin(flash_cs_port, flash_cs_pin, GPIO_PIN_SET);
}

/**
 * @brief FLASH SPI TRANSFER
 */
static bool flash_transfer(const uint8_t *tx, uint8_t *rx, uint32_t length) {
  if (length == 0U) return true;
  if (tx != NULL && rx != NULL) {
    return (HAL_SPI_TransmitReceive(&hspi1, (uint8_t *)tx, rx, (uint16_t)length, EXTERNAL_FLASH_TIMEOUT_MS) == HAL_OK);
  } else if (tx != NULL) {
    return (HAL_SPI_Transmit(&hspi1, (uint8_t *)tx, (uint16_t)length, EXTERNAL_FLASH_TIMEOUT_MS) == HAL_OK);
  } else if (rx != NULL) {
    return (HAL_SPI_Receive(&hspi1, rx, (uint16_t)length, EXTERNAL_FLASH_TIMEOUT_MS) == HAL_OK);
  }
  return false;
}


static bool flash_write_enable(void) {
  uint8_t command = W25Q16_WRITE_ENABLE;
  flash_select();
  bool status = flash_transfer(&command, NULL, 1U);
  flash_deselect();
  return status;
}

/**
 * FLASH WAIT READY
 */
static bool flash_wait_ready(void) {
  uint8_t command = W25Q16_READ_STATUS_1;
  uint8_t status = 0U;
  uint32_t start = HAL_GetTick();
  do {
    flash_select();
    if (!flash_transfer(&command, NULL, 1U)) {
      flash_deselect();
      return false;
    }
    if (!flash_transfer(NULL, &status, 1U)) {
      flash_deselect();
      return false;
    }
    flash_deselect();
    if ((status & W25Q16_BUSY_MASK) == 0U) return true;
  } while ((HAL_GetTick() - start) < EXTERNAL_FLASH_TIMEOUT_MS);
  return false;
}


bool ExternalFlash_Init(void) {
  flash_deselect();
  HAL_Delay(5U);


  uint8_t releaseCmd = W25Q16_RELEASE_POWERDOWN;
  flash_select();
  flash_transfer(&releaseCmd, NULL, 1U);
  flash_deselect();
  HAL_Delay(5U);


  uint8_t command = W25Q16_READ_ID;
  uint8_t id[3] = {0U, 0U, 0U};
  flash_select();
  bool ok = flash_transfer(&command, NULL, 1U) && flash_transfer(NULL, id, sizeof(id));
  flash_deselect();

  uint32_t jedec = ((uint32_t)id[0] << 16) | ((uint32_t)id[1] << 8) | id[2];
  if (ok && id[0] != 0x00U && id[0] != 0xFFU) {
    externalFlashReady = true;
    printf("[FLASH] Memoria Flash esterna rilevata: ManufID=0x%02X, DevID=0x%04X (JEDEC: 0x%06lX) su CS=PB0\r\n",
           id[0], ((uint16_t)id[1] << 8) | id[2], (unsigned long)jedec);
  } else {
    externalFlashReady = false;
    printf("[FLASH] [ERRORE] Nessuna risposta dalla memoria Flash esterna su PB0 (JEDEC: 0x%06lX)\r\n",
           (unsigned long)jedec);
  }

  return externalFlashReady;
}


bool ExternalFlash_IsReady(void) {
  return externalFlashReady;
}


bool ExternalFlash_Read(uint32_t address, void *data, uint32_t length) {
  if (data == NULL || length == 0U) return false;
  uint8_t command[4] = {
    W25Q16_READ_DATA,
    (uint8_t)(address >> 16),
    (uint8_t)(address >> 8),
    (uint8_t)address
  };
  flash_select();
  bool transferred = flash_transfer(command, NULL, sizeof(command)) &&
                     flash_transfer(NULL, (uint8_t *)data, length);
  flash_deselect();
  return transferred;
}

/**
 * RE-WRITE FLASH.
 */
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


bool ExternalFlash_Program(uint32_t address, const void *data, uint32_t length) {
  if (data == NULL || length == 0U) return false;
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

#if ENABLE_INTERNAL_ASSETS_DATA
static bool externalAssetsLoaded = false;
#endif

bool ExternalFlash_AreAssetsLoaded(void) {
#if ENABLE_INTERNAL_ASSETS_DATA
  return externalAssetsLoaded;
#else
  return true;
#endif
}

#if ENABLE_INTERNAL_ASSETS_DATA
static bool flash_verify_block(uint32_t flashAddr, const uint8_t *expectedData, uint32_t length, const char *name) {
  uint8_t readBuffer[256];
  uint32_t verified = 0U;

  while (verified < length) {
    uint32_t toRead = length - verified;
    if (toRead > sizeof(readBuffer)) toRead = sizeof(readBuffer);

    if (!ExternalFlash_Read(flashAddr + verified, readBuffer, toRead)) {
      printf("[FLASH] [ERRORE] Lettura SPI fallita durante verifica %s ad offset %lu!\r\n", 
             name, (unsigned long)verified);
      return false;
    }

    if (memcmp(readBuffer, expectedData + verified, toRead) != 0) {
      printf("[FLASH] [ERRORE] Discrepanza dati rilevata in %s ad offset %lu!\r\n", 
             name, (unsigned long)verified);
      return false;
    }

    verified += toRead;
  }
  return true;
}
#endif

/**
 *  IF ENABLE_INTERNAL_ASSETS_DATA is set to 1, this function checks if the assets are already programmed 
 */
bool ExternalFlash_EnsureAssetsProgrammed(void) {
  if (!ExternalFlash_IsReady()) {
    if (!ExternalFlash_Init()) {
      printf("[FLASH] [ERRORE] Memoria non pronta!\r\n");
      return false;
    }
  }

#if ENABLE_INTERNAL_ASSETS_DATA
  externalAssetsLoaded = false;

  printf("[FLASH] Controllo integrita' dati residenti su Flash esterna...\r\n");
  bool cubeniro_ok = flash_verify_block(EXTERNAL_FLASH_CUBENIRO_ADDRESS, asset_cubeniro_rgb565, ASSET_CUBENIRO_SIZE, "CUBENIRO");
  bool snake_ok = flash_verify_block(EXTERNAL_FLASH_SNAKE_ADDRESS, asset_snake_rgb565, ASSET_SNAKE_SIZE, "SNAKE");

  if (cubeniro_ok && snake_ok) {
    externalAssetsLoaded = true;
    printf("[FLASH] Dati gia' presenti e verificati al 100%% sulla Flash esterna (OK)\r\n");
    return true;
  }

  printf("[FLASH] Flash non allineata o corrotta. Avvio riscrittura completa...\r\n");

  // 1. Scrittura immagine CUBENIRO
  printf("[FLASH] [1/2] Cancellazione settori CUBENIRO (153.600 byte)...\r\n");
  uint32_t addr = EXTERNAL_FLASH_CUBENIRO_ADDRESS;
  bool erase_ok = true;
  for (uint32_t s = 0; s < ASSET_CUBENIRO_SIZE; s += 4096U) {
    if (!ExternalFlash_EraseSector(addr + s)) {
      erase_ok = false;
      break;
    }
  }
  if (!erase_ok) {
    printf("[FLASH] [ERRORE] Cancellazione settori CUBENIRO fallita!\r\n");
    return false;
  }

  printf("[FLASH] [1/2] Scrittura CUBENIRO in corso...\r\n");
  if (!ExternalFlash_Program(addr, asset_cubeniro_rgb565, ASSET_CUBENIRO_SIZE)) {
    printf("[FLASH] [ERRORE] Scrittura pagine CUBENIRO fallita!\r\n");
    return false;
  }

  printf("[FLASH] [1/2] Verifica byte-a-byte CUBENIRO (153.600 byte)...\r\n");
  if (!flash_verify_block(addr, asset_cubeniro_rgb565, ASSET_CUBENIRO_SIZE, "CUBENIRO")) {
    printf("[FLASH] [ERRORE] Verifica CUBENIRO fallita dopo scrittura!\r\n");
    return false;
  }
  printf("[FLASH] [1/2] CUBENIRO scritto e verificato al 100%% con successo!\r\n");

  // 2. Scrittura immagine SNAKE
  printf("[FLASH] [2/2] Cancellazione settori SNAKE (153.600 byte)...\r\n");
  addr = EXTERNAL_FLASH_SNAKE_ADDRESS;
  erase_ok = true;
  for (uint32_t s = 0; s < ASSET_SNAKE_SIZE; s += 4096U) {
    if (!ExternalFlash_EraseSector(addr + s)) {
      erase_ok = false;
      break;
    }
  }
  if (!erase_ok) {
    printf("[FLASH] [ERRORE] Cancellazione settori SNAKE fallita!\r\n");
    return false;
  }

  printf("[FLASH] [2/2] Scrittura SNAKE in corso...\r\n");
  if (!ExternalFlash_Program(addr, asset_snake_rgb565, ASSET_SNAKE_SIZE)) {
    printf("[FLASH] [ERRORE] Scrittura pagine SNAKE fallita!\r\n");
    return false;
  }

  printf("[FLASH] [2/2] Verifica byte-a-byte SNAKE (153.600 byte)...\r\n");
  if (!flash_verify_block(addr, asset_snake_rgb565, ASSET_SNAKE_SIZE, "SNAKE")) {
    printf("[FLASH] [ERRORE] Verifica SNAKE fallita dopo scrittura!\r\n");
    return false;
  }
  printf("[FLASH] [2/2] SNAKE scritto e verificato al 100%% con successo!\r\n");

  externalAssetsLoaded = true;
  printf("[FLASH] SUCCESSO TOTALE: 307.200 byte caricati e verificati al 100%% sulla Flash esterna!\r\n");
  return true;
#else
  printf("[FLASH] Dati residenti su memoria Flash SPI esterna (Asset interni disattivati per risparmio RAM/ROM)\r\n");
  return true;
#endif
}
