#ifndef LCD_H
#define LCD_H

#include "console.h"

/* LCD POWER SAVE MODE CONFIGURATION:
 * 1 -> DEMO MODE 
 * 0 -> HARDWARE SLEEP MODE 
 */
#define LCD_POWER_SAVE_DEMO_MODE 1

void LCD_Init(void);
void LCD_Sleep(void); // OFF / Standby
void LCD_Wakeup(void); // ON / Resume
void LCD_Render(const Game_t *game, const Console_t *console);
void LCD_ShowError(const char *msg);

#endif
