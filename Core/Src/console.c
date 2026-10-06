#include "console.h"
#include <stddef.h>

/*/
 * CONSOLE HANDLER
 */

void Console_Init(Console_t *console, Game_t *game) {
    if (console == NULL) return;
    console->state = CONSOLE_STATE_OFF;
    console->selectedLevel = LEVEL_EASY;
    console->kbdRow = 0U;
    console->kbdCol = 0U;
    console->pauseBtn = PAUSE_BTN_RESUME;
    console->gameOverBtn = GAMEOVER_BTN_RESTART;
    console->leaderboardBtn = LEADERBOARD_BTN_RESTART;
    if (game != NULL) {
        Snake_SetPlayerName(game, "Serpente");
        game->state = GAME_STATE_IDLE; // NOT READY
    }
}

void Console_HandleEvent(Console_t *console, Game_t *game, ConsoleEvent_t event, char character) {
    if (console == NULL || game == NULL) return;

    if (event == CONSOLE_EVENT_POWER || event == CONSOLE_EVENT_X) {
        if (console->state == CONSOLE_STATE_OFF) {
            console->state = CONSOLE_STATE_LOAD;
            game->state = GAME_STATE_IDLE;
            return;
        } else if (event == CONSOLE_EVENT_POWER) {
            console->state = CONSOLE_STATE_OFF;
            game->state = GAME_STATE_IDLE;
            return;
        }
    }

    if (console->state == CONSOLE_STATE_OFF) return;

    switch (console->state) {

        // console load
        case CONSOLE_STATE_LOAD:
            if (event == CONSOLE_EVENT_X || event == CONSOLE_EVENT_BUTTON_SELECT) {
                console->state = CONSOLE_STATE_START;
            }
            break;


        // snake home page
        case CONSOLE_STATE_START:
            if (event == CONSOLE_EVENT_X || event == CONSOLE_EVENT_BUTTON_SELECT) {
                Snake_SetPlayerName(game, "");
                console->kbdRow = 0U;
                console->kbdCol = 0U;
                console->state = CONSOLE_STATE_NAME;
            }
            break;

        // snake name input
        case CONSOLE_STATE_NAME:
            if (event == CONSOLE_EVENT_NAME_CHAR) {
                Snake_AppendPlayerNameChar(game, character);
            } else if (event == CONSOLE_EVENT_NAME_BACKSPACE) {
                Snake_BackspacePlayerName(game);
            } else if (event == CONSOLE_EVENT_NAME_CONFIRM) {
                if (game->playerName[0] == '\0') {
                    Snake_SetPlayerName(game, "PLAYER1");
                }
                console->state = CONSOLE_STATE_DIFFICULTY;
            } else if (event == CONSOLE_EVENT_BACK) {
                console->state = CONSOLE_STATE_START;
            } else if (event == CONSOLE_EVENT_UP) {
                if (console->kbdRow > 0U) console->kbdRow--;
                else console->kbdRow = 3U;
            } else if (event == CONSOLE_EVENT_DOWN) {
                if (console->kbdRow < 3U) console->kbdRow++;
                else console->kbdRow = 0U;
            } else if (event == CONSOLE_EVENT_LEFT) {
                if (console->kbdCol > 0U) console->kbdCol--;
                else console->kbdCol = 6U;
            } else if (event == CONSOLE_EVENT_RIGHT) {
                if (console->kbdCol < 6U) console->kbdCol++;
                else console->kbdCol = 0U;
            } else if (event == CONSOLE_EVENT_BUTTON_SELECT) {
               
                if (console->kbdRow < 3U) {
                    char c = (char)('A' + (console->kbdRow * 7U + console->kbdCol));
                    Snake_AppendPlayerNameChar(game, c);
                } else {
                    if (console->kbdCol < 5U) {
                        char c = (char)('V' + console->kbdCol);
                        Snake_AppendPlayerNameChar(game, c);
                    } else if (console->kbdCol == 5U) {
                        // BACKSPACE
                        Snake_BackspacePlayerName(game);
                    } else {
                        // CONFIRM
                        if (game->playerName[0] == '\0') {
                            Snake_SetPlayerName(game, "PLAYER1");
                        }
                        console->state = CONSOLE_STATE_DIFFICULTY;
                    }
                }
            }
            break;

        // snake difficulty selection
        case CONSOLE_STATE_DIFFICULTY:
            if (event == CONSOLE_EVENT_BACK) {
                console->state = CONSOLE_STATE_START;
            } else if (event == CONSOLE_EVENT_UP || event == CONSOLE_EVENT_LEFT) {
                if (console->selectedLevel == LEVEL_EASY) console->selectedLevel = LEVEL_HARD;
                else console->selectedLevel--;
            } else if (event == CONSOLE_EVENT_DOWN || event == CONSOLE_EVENT_RIGHT) {
                if (console->selectedLevel == LEVEL_HARD) console->selectedLevel = LEVEL_EASY;
                else console->selectedLevel++;
            } else if (event == CONSOLE_EVENT_LEVEL_EASY || event == CONSOLE_EVENT_BUTTON_SELECT) {
                Snake_InitLevel(game, console->selectedLevel);
                console->state = CONSOLE_STATE_GAME;
                console->pauseBtn = PAUSE_BTN_RESUME;
                console->gameOverBtn = GAMEOVER_BTN_RESTART;
            } else if (event == CONSOLE_EVENT_LEVEL_MEDIUM) {
                Snake_InitLevel(game, console->selectedLevel);
                console->state = CONSOLE_STATE_GAME;
                console->pauseBtn = PAUSE_BTN_RESUME;
                console->gameOverBtn = GAMEOVER_BTN_RESTART;
            } else if (event == CONSOLE_EVENT_LEVEL_HARD) {
                Snake_InitLevel(game, console->selectedLevel);
                console->state = CONSOLE_STATE_GAME;
                console->pauseBtn = PAUSE_BTN_RESUME;
                console->gameOverBtn = GAMEOVER_BTN_RESTART;
            }
            break;

        // GAME STATE: RUNNING / PAUSED / GAMEOVER
        case CONSOLE_STATE_GAME:
            if (game->state == GAME_STATE_PAUSED) {
                if (event == CONSOLE_EVENT_UP || event == CONSOLE_EVENT_LEFT) {
                    if (console->pauseBtn > 0) console->pauseBtn--;
                    else console->pauseBtn = PAUSE_BTN_COUNT - 1;
                } else if (event == CONSOLE_EVENT_DOWN || event == CONSOLE_EVENT_RIGHT) {
                    if (console->pauseBtn + 1 < PAUSE_BTN_COUNT) console->pauseBtn++;
                    else console->pauseBtn = 0;
                } else if (event == CONSOLE_EVENT_BUTTON_SELECT || event == CONSOLE_EVENT_PAUSE) {
                    if (console->pauseBtn == PAUSE_BTN_RESUME) {
                        Snake_TogglePause(game);
                    } else if (console->pauseBtn == PAUSE_BTN_RETURN_START) {
                        game->state = GAME_STATE_IDLE;
                        console->state = CONSOLE_STATE_START;
                    }
                }
            } else if (game->state == GAME_STATE_GAMEOVER) {
                if (event == CONSOLE_EVENT_UP || event == CONSOLE_EVENT_LEFT) {
                    if (console->gameOverBtn > 0) console->gameOverBtn--;
                    else console->gameOverBtn = GAMEOVER_BTN_COUNT - 1;
                } else if (event == CONSOLE_EVENT_DOWN || event == CONSOLE_EVENT_RIGHT) {
                    if (console->gameOverBtn + 1 < GAMEOVER_BTN_COUNT) console->gameOverBtn++;
                    else console->gameOverBtn = 0;
                } else if (event == CONSOLE_EVENT_BUTTON_SELECT || event == CONSOLE_EVENT_RESTART) {
                    if (console->gameOverBtn == GAMEOVER_BTN_RESTART) {
                        Snake_InitLevel(game, console->selectedLevel);
                    } else if (console->gameOverBtn == GAMEOVER_BTN_VIEW_SCORE) {
                        console->leaderboardBtn = LEADERBOARD_BTN_RESTART;
                        console->state = CONSOLE_STATE_LEADERBOARD;
                    } else if (console->gameOverBtn == GAMEOVER_BTN_RETURN_START) {
                        game->state = GAME_STATE_IDLE;
                        console->state = CONSOLE_STATE_START;
                    }
                }
            } else {

                if (event == CONSOLE_EVENT_RESTART) {
                    Snake_InitLevel(game, console->selectedLevel);
                } else if (event == CONSOLE_EVENT_BACK) {
                    game->state = GAME_STATE_IDLE;
                    console->state = CONSOLE_STATE_START;
                } else if (event == CONSOLE_EVENT_UP) Snake_SetDirection(game, DIR_DOWN);
                else if (event == CONSOLE_EVENT_DOWN) Snake_SetDirection(game, DIR_UP);
                else if (event == CONSOLE_EVENT_LEFT) Snake_SetDirection(game, DIR_LEFT);
                else if (event == CONSOLE_EVENT_RIGHT) Snake_SetDirection(game, DIR_RIGHT);
                else if (event == CONSOLE_EVENT_PAUSE || event == CONSOLE_EVENT_BUTTON_SELECT) {
                    console->pauseBtn = PAUSE_BTN_RESUME;
                    Snake_TogglePause(game);
                }
            }
            break;

        // LEADERBOARD STATE CONSOLE
        case CONSOLE_STATE_LEADERBOARD:
            if (event == CONSOLE_EVENT_UP || event == CONSOLE_EVENT_LEFT) {
                if (console->leaderboardBtn > 0) console->leaderboardBtn--;
                else console->leaderboardBtn = LEADERBOARD_BTN_COUNT - 1;
            } else if (event == CONSOLE_EVENT_DOWN || event == CONSOLE_EVENT_RIGHT) {
                if (console->leaderboardBtn + 1 < LEADERBOARD_BTN_COUNT) console->leaderboardBtn++;
                else console->leaderboardBtn = 0;
            } else if (event == CONSOLE_EVENT_BUTTON_SELECT) {
                if (console->leaderboardBtn == LEADERBOARD_BTN_RESTART) {
                    Snake_InitLevel(game, console->selectedLevel);
                    console->state = CONSOLE_STATE_GAME;
                } else if (console->leaderboardBtn == LEADERBOARD_BTN_RETURN_HOME) {
                    game->state = GAME_STATE_IDLE;
                    console->state = CONSOLE_STATE_START;
                }
            } else if (event == CONSOLE_EVENT_BACK) {
                console->state = CONSOLE_STATE_START;
            }
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