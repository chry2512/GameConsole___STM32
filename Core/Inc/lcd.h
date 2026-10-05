#ifndef LCD_H
#define LCD_H

#include "console.h"

void LCD_Init(void);
void LCD_Render(const Game_t *game, const Console_t *console);

#endif
