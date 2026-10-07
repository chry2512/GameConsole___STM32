#ifndef LCD_H
#define LCD_H

#include "console.h"

/* LCD POWER SAVE MODE CONFIGURATION:
 * 1 -> DEMO MODE (Software Blackout):
 *      Fills screen with pure black, ideal for demos where LCD backlight is hardwired to 3.3V.
 * 0 -> HARDWARE SLEEP MODE (Native ILI9341 commands):
 *      Sends 0x28 (Display OFF) and 0x10 (Sleep In) for true hardware power saving.
 */
#define LCD_POWER_SAVE_DEMO_MODE 1

void LCD_Init(void);
void LCD_Sleep(void); // OFF / Standby
void LCD_Wakeup(void); // ON / Resume
void LCD_Render(const Game_t *game, const Console_t *console);
void LCD_ShowError(const char *msg);

#endif
