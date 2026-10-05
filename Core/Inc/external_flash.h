#ifndef EXTERNAL_FLASH_H
#define EXTERNAL_FLASH_H

#include <stdbool.h>
#include <stdint.h>

#define EXTERNAL_FLASH_IMAGE_WIDTH 128U
#define EXTERNAL_FLASH_IMAGE_HEIGHT 160U
#define EXTERNAL_FLASH_IMAGE_BYTES (EXTERNAL_FLASH_IMAGE_WIDTH * EXTERNAL_FLASH_IMAGE_HEIGHT * 2U)
#define EXTERNAL_FLASH_CUBENIRO_ADDRESS 0x000000UL
#define EXTERNAL_FLASH_SNAKE_ADDRESS 0x00A000UL

/* FLASH MEMORY ESTERNA: W25Q16 collegata a SPI2. */
bool ExternalFlash_Init(void);
bool ExternalFlash_IsReady(void);
bool ExternalFlash_Read(uint32_t address, void *data, uint32_t length);
bool ExternalFlash_EraseSector(uint32_t address);
bool ExternalFlash_Program(uint32_t address, const void *data, uint32_t length);
bool ExternalFlash_ReadImagePixel(uint32_t imageAddress, uint16_t x, uint16_t y, uint16_t *color);

#endif
