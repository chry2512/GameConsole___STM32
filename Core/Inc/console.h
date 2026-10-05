#ifndef CONSOLE_H
#define CONSOLE_H

#include "snake.h"

typedef enum {
    CONSOLE_STATE_OFF = 0,
    CONSOLE_STATE_START,
    CONSOLE_STATE_NAME,
    CONSOLE_STATE_DIFFICULTY,
    CONSOLE_STATE_GAME
} ConsoleState_t;

typedef enum {
    CONSOLE_EVENT_POWER = 0,
    CONSOLE_EVENT_X,
    CONSOLE_EVENT_PAUSE,
    CONSOLE_EVENT_UP,
    CONSOLE_EVENT_DOWN,
    CONSOLE_EVENT_LEFT,
    CONSOLE_EVENT_RIGHT,
    CONSOLE_EVENT_NAME_CHAR,
    CONSOLE_EVENT_NAME_BACKSPACE,
    CONSOLE_EVENT_NAME_CONFIRM,
    CONSOLE_EVENT_BACK,
    CONSOLE_EVENT_LEVEL_EASY,
    CONSOLE_EVENT_LEVEL_MEDIUM,
    CONSOLE_EVENT_LEVEL_HARD,
    CONSOLE_EVENT_RESTART
} ConsoleEvent_t;

typedef struct {
    ConsoleState_t state;
    Level_t selectedLevel;
} Console_t;

void Console_Init(Console_t *console, Game_t *game);
void Console_HandleEvent(Console_t *console, Game_t *game, ConsoleEvent_t event, char character);
ConsoleState_t Console_GetState(const Console_t *console);
Level_t Console_GetSelectedLevel(const Console_t *console);

#endif