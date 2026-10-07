#ifndef EXTERNAL_FLASH_H
#define EXTERNAL_FLASH_H

#include <stdbool.h>
#include <stdint.h>

#define EXTERNAL_FLASH_IMAGE_WIDTH 320U
#define EXTERNAL_FLASH_IMAGE_HEIGHT 240U
#define EXTERNAL_FLASH_IMAGE_BYTES (EXTERNAL_FLASH_IMAGE_WIDTH * EXTERNAL_FLASH_IMAGE_HEIGHT * 2U)
#define EXTERNAL_FLASH_CUBENIRO_ADDRESS 0x000000UL
#define EXTERNAL_FLASH_SNAKE_ADDRESS    0x030000UL 


bool ExternalFlash_Init(void);
bool ExternalFlash_IsReady(void);
bool ExternalFlash_Read(uint32_t address, void *data, uint32_t length);
bool ExternalFlash_EraseSector(uint32_t address); //RESET- RELOAD
bool ExternalFlash_Program(uint32_t address, const void *data, uint32_t length);
bool ExternalFlash_EnsureAssetsProgrammed(void); // PROGRAMM ASSET OK
bool ExternalFlash_AreAssetsLoaded(void);// LOAD ASSET OK

#endif
