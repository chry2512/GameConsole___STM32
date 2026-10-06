#ifndef ASSETS_DATA_H
#define ASSETS_DATA_H

#include <stdint.h>

#define ASSET_CUBENIRO_SIZE 153600U
#define ASSET_SNAKE_SIZE 153600U
#define ASSET_SNAKE_SIZE    153600U

/* ENABLE INITIAL LOAD OF INTERNAL ASSETS DATA
 * ON --> 1
 * OFF --> 0
 */
#define ENABLE_INTERNAL_ASSETS_DATA 1

#if ENABLE_INTERNAL_ASSETS_DATA
extern const uint8_t asset_cubeniro_rgb565[153600U];
extern const uint8_t asset_snake_rgb565[153600U];
#endif

#endif /* ASSETS_DATA_H */
