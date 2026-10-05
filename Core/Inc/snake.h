#ifndef SNAKE_H
#define SNAKE_H

#include <stdint.h>
#include <stdbool.h>


#define GRID_WIDTH   32
#define GRID_HEIGHT  24
#define MAX_SNAKE_LENGTH (GRID_WIDTH * GRID_HEIGHT)
#define MAX_GRID_CELLS (GRID_WIDTH * GRID_HEIGHT)
#define MAX_AI_SNAKES 4U
#define MAX_FOODS 5U
#define FOODS_PER_STAGE 5U
#define MAX_GAME_STAGE 100U
#define SNAKE_NAME_MAX_LENGTH 12U
#define SNAKE_LEADERBOARD_SIZE 10U

typedef enum {
    DIR_UP = 0,
    DIR_DOWN,
    DIR_LEFT,
    DIR_RIGHT
} Direction_t;

typedef enum {
    LEVEL_EASY = 0,
    LEVEL_MEDIUM,
    LEVEL_HARD
} Level_t;


typedef struct {
    int16_t x;
    int16_t y;
} Point_t;


typedef enum {
    GAME_STATE_IDLE = 0,
    GAME_STATE_RUNNING,
    GAME_STATE_PAUSED,
    GAME_STATE_LEVEL_TRANSITION,
    GAME_STATE_GAMEOVER,
    GAME_STATE_VICTORY
} GameState_t;


typedef struct {
    Point_t body[MAX_SNAKE_LENGTH];
    uint16_t length;
    Direction_t dir;
    Direction_t nextDir;
} Snake_t;

typedef struct {
    uint8_t cells[MAX_GRID_CELLS];
    uint16_t length;
    Direction_t dir;
} Maze_t;


typedef struct {
    Snake_t snake;
    Point_t foods[MAX_FOODS];
    uint8_t foodCount;
    GameState_t state;
    uint32_t score;
    char playerName[SNAKE_NAME_MAX_LENGTH + 1U];
    Level_t level;
    Level_t initialLevel;
    uint16_t stage;
    uint16_t foodsEaten;
    uint16_t moveIntervalMs;
    uint16_t transitionRemainingMs;
    bool easterEggUnlocked;
    Maze_t maze;
    Snake_t enemies[MAX_AI_SNAKES];
    uint8_t enemyCount;
} Game_t;

typedef struct {
    char playerName[SNAKE_NAME_MAX_LENGTH + 1U];
    uint32_t score;
} LeaderboardEntry_t;


void Snake_Init(Game_t *game);
void Snake_InitLevel(Game_t *game, Level_t level);
void Snake_SetLevel(Game_t *game, Level_t level);
void Snake_SetDirection(Game_t *game, Direction_t newDir);
void Snake_SetPlayerName(Game_t *game, const char *name);
bool Snake_AppendPlayerNameChar(Game_t *game, char character);
void Snake_BackspacePlayerName(Game_t *game);
const char *Snake_GetPlayerName(const Game_t *game);
void Snake_TogglePause(Game_t *game);
bool Snake_Update(Game_t *game);
void Snake_AdvanceTransition(Game_t *game, uint16_t elapsedMs);
void Snake_RespawnFood(Game_t *game);
bool Snake_IsObstacle(const Game_t *game, Point_t point);
uint16_t Snake_GetMoveIntervalMs(const Game_t *game);
uint16_t Snake_GetStage(const Game_t *game);
void Snake_Leaderboard_Load(void);
uint8_t Snake_Leaderboard_Count(void);
const LeaderboardEntry_t *Snake_Leaderboard_Get(uint8_t position);

#endif 