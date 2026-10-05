#include "console.h"
#include <stddef.h>

void Console_Init(Console_t *console, Game_t *game) {
    if (console == NULL) return;
    console->state = CONSOLE_STATE_OFF;
    console->selectedLevel = LEVEL_EASY;
    if (game != NULL) {
        Snake_SetPlayerName(game, "Serpente");
        game->state = GAME_STATE_IDLE;
    }
}

void Console_HandleEvent(Console_t *console, Game_t *game, ConsoleEvent_t event, char character) {
    if (console == NULL || game == NULL) return;

    if (event == CONSOLE_EVENT_POWER) {
        if (console->state == CONSOLE_STATE_OFF) {
            console->state = CONSOLE_STATE_START;
            game->state = GAME_STATE_IDLE;
        } else {
            console->state = CONSOLE_STATE_OFF;
            game->state = GAME_STATE_IDLE;
        }
        return;
    }

    if (console->state == CONSOLE_STATE_OFF) return;

    switch (console->state) {
        case CONSOLE_STATE_START:
            if (event == CONSOLE_EVENT_X) {
                Snake_SetPlayerName(game, "");
                console->state = CONSOLE_STATE_NAME;
            }
            break;

        case CONSOLE_STATE_NAME:
            if (event == CONSOLE_EVENT_NAME_CHAR) {
                Snake_AppendPlayerNameChar(game, character);
            } else if (event == CONSOLE_EVENT_NAME_BACKSPACE) {
                Snake_BackspacePlayerName(game);
            } else if (event == CONSOLE_EVENT_NAME_CONFIRM) {
                console->state = CONSOLE_STATE_DIFFICULTY;
            } else if (event == CONSOLE_EVENT_BACK) {
                console->state = CONSOLE_STATE_START;
            }
            break;

        case CONSOLE_STATE_DIFFICULTY:
            if (event == CONSOLE_EVENT_BACK) {
                console->state = CONSOLE_STATE_START;
            } else if (event == CONSOLE_EVENT_UP) {
                if (console->selectedLevel == LEVEL_EASY) console->selectedLevel = LEVEL_HARD;
                else console->selectedLevel--;
            } else if (event == CONSOLE_EVENT_DOWN) {
                if (console->selectedLevel == LEVEL_HARD) console->selectedLevel = LEVEL_EASY;
                else console->selectedLevel++;
            } else if (event == CONSOLE_EVENT_LEVEL_EASY) {
                Snake_InitLevel(game, console->selectedLevel);
                console->state = CONSOLE_STATE_GAME;
            } else if (event == CONSOLE_EVENT_LEVEL_MEDIUM) {
                Snake_InitLevel(game, console->selectedLevel);
                console->state = CONSOLE_STATE_GAME;
            } else if (event == CONSOLE_EVENT_LEVEL_HARD) {
                Snake_InitLevel(game, console->selectedLevel);
                console->state = CONSOLE_STATE_GAME;
            }
            break;

        case CONSOLE_STATE_GAME:
            if (event == CONSOLE_EVENT_RESTART) {
                Snake_InitLevel(game, console->selectedLevel);
            } else if (event == CONSOLE_EVENT_BACK) {
                game->state = GAME_STATE_IDLE;
                console->state = CONSOLE_STATE_START;
            } else if (event == CONSOLE_EVENT_UP) Snake_SetDirection(game, DIR_UP);
            else if (event == CONSOLE_EVENT_DOWN) Snake_SetDirection(game, DIR_DOWN);
            else if (event == CONSOLE_EVENT_LEFT) Snake_SetDirection(game, DIR_LEFT);
            else if (event == CONSOLE_EVENT_RIGHT) Snake_SetDirection(game, DIR_RIGHT);
            else if (event == CONSOLE_EVENT_PAUSE) Snake_TogglePause(game);
            break;

        default:
            break;
    }
}

ConsoleState_t Console_GetState(const Console_t *console) {
    return console == NULL ? CONSOLE_STATE_OFF : console->state;
}

Level_t Console_GetSelectedLevel(const Console_t *console) {
    return console == NULL ? LEVEL_EASY : console->selectedLevel;
}