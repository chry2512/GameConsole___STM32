#ifndef TOUCH_H
#define TOUCH_H

#include <stdbool.h>
#include <stdint.h>


void Touch_Init(void);


bool Touch_IsPressed(void);


bool Touch_GetCoordinates(uint16_t *x, uint16_t *y);


bool Touch_IsCenterTapped(void);

#endif /* TOUCH_H */

