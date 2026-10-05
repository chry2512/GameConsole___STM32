#include "snake.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#if defined(STM32F407xx)
#include "stm32f4xx_hal.h"
#else
#include <stdio.h>
#endif

#define INITIAL_MOVE_INTERVAL_MS 250U
#define MIN_MOVE_INTERVAL_MS 140U
#define MOVE_ACCELERATION_MS 2U
#define FOODS_PER_SNAKE_GROWTH 2U
#define FOOD_SCORE_POINTS 10U
#define LEVEL_TRANSITION_MS 1200U
#define FOOD_MAZE_GAP_STAGE 95U
#define LEADERBOARD_MAGIC 0x534E4B31UL
#define LEADERBOARD_VERSION 1U
#if defined(STM32F407xx)
#define LEADERBOARD_FLASH_ADDRESS 0x08060000UL
#define LEADERBOARD_FLASH_SECTOR FLASH_SECTOR_7
#endif

typedef struct {
    uint32_t magic;
    uint32_t version;
    LeaderboardEntry_t entries[SNAKE_LEADERBOARD_SIZE];
    uint32_t checksum;
} LeaderboardStorage_t;

static LeaderboardEntry_t leaderboard[SNAKE_LEADERBOARD_SIZE];
static bool leaderboardLoaded = false;

static uint32_t leaderboard_checksum(const LeaderboardStorage_t *storage) {
    const uint8_t *bytes = (const uint8_t *)storage;
    uint32_t checksum = 2166136261UL;
    size_t length = offsetof(LeaderboardStorage_t, checksum);
    for (size_t index = 0U; index < length; index++) {
        checksum ^= bytes[index];
        checksum *= 16777619UL;
    }
    return checksum;
}

static bool leaderboard_storage_valid(const LeaderboardStorage_t *storage) {
    return storage->magic == LEADERBOARD_MAGIC &&
           storage->version == LEADERBOARD_VERSION &&
           storage->checksum == leaderboard_checksum(storage);
}

static void leaderboard_save(void) {
    LeaderboardStorage_t storage = {0};
    storage.magic = LEADERBOARD_MAGIC;
    storage.version = LEADERBOARD_VERSION;
    memcpy(storage.entries, leaderboard, sizeof(leaderboard));
    storage.checksum = leaderboard_checksum(&storage);
#if defined(STM32F407xx)
    FLASH_EraseInitTypeDef erase = {0};
    uint32_t sectorError = 0U;
    erase.TypeErase = FLASH_TYPEERASE_SECTORS;
    erase.Sector = LEADERBOARD_FLASH_SECTOR;
    erase.NbSectors = 1U;
    erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;

    // Disabilita data cache prima di scrivere/cancellare la Flash
    __HAL_FLASH_DATA_CACHE_DISABLE();
    __HAL_FLASH_INSTRUCTION_CACHE_DISABLE();

    HAL_FLASH_Unlock();
    // Pulisce i flag di errore pendenti
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | 
                           FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);

    if (HAL_FLASHEx_Erase(&erase, &sectorError) == HAL_OK) {
        const uint32_t *words = (const uint32_t *)&storage;
        for (size_t index = 0U; index < sizeof(storage) / sizeof(uint32_t); index++) {
            if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,
                                  LEADERBOARD_FLASH_ADDRESS + index * sizeof(uint32_t),
                                  words[index]) != HAL_OK) {
                break;
            }
        }
    }
    HAL_FLASH_Lock();

    // Resetta e riabilita le cache
    __HAL_FLASH_INSTRUCTION_CACHE_RESET();
    __HAL_FLASH_DATA_CACHE_RESET();
    __HAL_FLASH_INSTRUCTION_CACHE_ENABLE();
    __HAL_FLASH_DATA_CACHE_ENABLE();
#else
    FILE *file = fopen("snake_leaderboard.dat", "wb");
    if (file != NULL) {
        (void)fwrite(&storage, sizeof(storage), 1U, file);
        fclose(file);
    }
#endif
}

void Snake_Leaderboard_Load(void) {
    if (leaderboardLoaded) return;
    memset(leaderboard, 0, sizeof(leaderboard));
    LeaderboardStorage_t storage = {0};
#if defined(STM32F407xx)
    memcpy(&storage, (const void *)LEADERBOARD_FLASH_ADDRESS, sizeof(storage));
#else
    FILE *file = fopen("snake_leaderboard.dat", "rb");
    if (file != NULL) {
        (void)fread(&storage, sizeof(storage), 1U, file);
        fclose(file);
    }
#endif
    if (leaderboard_storage_valid(&storage)) {
        memcpy(leaderboard, storage.entries, sizeof(leaderboard));
    } else {
        // Se la memoria non e' formattata o e' vergine (0xFF), inizializzala pulita
        memset(leaderboard, 0, sizeof(leaderboard));
        leaderboard_save();
    }
    leaderboardLoaded = true;
}

static void leaderboard_submit(const Game_t *game) {
    if (game == NULL) return;
    Snake_Leaderboard_Load();

    const char *pName = (game->playerName[0] != '\0') ? game->playerName : "PLAYER1";

    uint8_t position = SNAKE_LEADERBOARD_SIZE;
    for (uint8_t index = 0U; index < SNAKE_LEADERBOARD_SIZE; index++) {
        // Se slot libero oppure punteggio migliore o uguale
        if (leaderboard[index].playerName[0] == '\0') {
            position = index;
            break;
        }
        if (game->score >= leaderboard[index].score) {
            position = index;
            break;
        }
    }
    if (position == SNAKE_LEADERBOARD_SIZE) return;

    for (uint8_t index = SNAKE_LEADERBOARD_SIZE - 1U; index > position; index--) {
        leaderboard[index] = leaderboard[index - 1U];
    }
    memset(&leaderboard[position], 0, sizeof(leaderboard[position]));
    strncpy(leaderboard[position].playerName, pName, SNAKE_NAME_MAX_LENGTH);
    leaderboard[position].score = game->score;
    leaderboard_save();
}

uint8_t Snake_Leaderboard_Count(void) {
    Snake_Leaderboard_Load();
    for (uint8_t index = 0U; index < SNAKE_LEADERBOARD_SIZE; index++) {
        if (leaderboard[index].playerName[0] == '\0') return index;
    }
    return SNAKE_LEADERBOARD_SIZE;
}

const LeaderboardEntry_t *Snake_Leaderboard_Get(uint8_t position) {
    Snake_Leaderboard_Load();
    return position < SNAKE_LEADERBOARD_SIZE ? &leaderboard[position] : NULL;
}

static void set_game_over(Game_t *game) {
    if (game->state != GAME_STATE_GAMEOVER) {
        game->state = GAME_STATE_GAMEOVER;
        leaderboard_submit(game);
    }
}

static void set_victory(Game_t *game) {
    if (game->state != GAME_STATE_VICTORY) {
        game->state = GAME_STATE_VICTORY;
        leaderboard_submit(game);
    }
}

static bool same_point(Point_t first, Point_t second) {
    return first.x == second.x && first.y == second.y;
}

static bool is_chry(const Game_t *game) {
    return game != NULL && strcmp(game->playerName, "CHRY") == 0;
}

static bool is_right_corner(Point_t point) {
    return point.x == GRID_WIDTH - 1 &&
           (point.y == 0 || point.y == GRID_HEIGHT - 1);
}

static bool valid_point(Point_t point) {
    return point.x >= 0 && point.x < GRID_WIDTH && point.y >= 0 && point.y < GRID_HEIGHT;
}

static Point_t next_point(Point_t point, Direction_t direction) {
    switch (direction) {
        case DIR_UP: point.y--; break;
        case DIR_DOWN: point.y++; break;
        case DIR_LEFT: point.x--; break;
        case DIR_RIGHT: point.x++; break;
    }
    return point;
}

static uint16_t point_index(Point_t point) {
    return (uint16_t)(point.y * GRID_WIDTH + point.x);
}

static bool food_between_walls(const Game_t *game, Point_t point) {
    bool betweenHorizontalWalls = point.x > 0 && point.x < GRID_WIDTH - 1 &&
        game->maze.cells[point_index((Point_t){point.x - 1, point.y})] != 0U &&
        game->maze.cells[point_index((Point_t){point.x + 1, point.y})] != 0U;
    bool betweenVerticalWalls = point.y > 0 && point.y < GRID_HEIGHT - 1 &&
        game->maze.cells[point_index((Point_t){point.x, point.y - 1})] != 0U &&
        game->maze.cells[point_index((Point_t){point.x, point.y + 1})] != 0U;
    return betweenHorizontalWalls || betweenVerticalWalls;
}

static uint8_t food_count_for_stage(uint16_t stage) {
    uint16_t count = 1U + stage / 20U;
    return (uint8_t)(count > MAX_FOODS ? MAX_FOODS : count);
}

static bool snake_contains(const Snake_t *snake, Point_t point, uint16_t length) {
    for (uint16_t i = 0; i < length; i++) {
        if (same_point(snake->body[i], point)) return true;
    }
    return false;
}

static void add_wall(Game_t *game, int16_t x, int16_t y) {
    Point_t point = {x, y};
    if (valid_point(point)) game->maze.cells[point_index(point)] = 1U;
}

static void add_horizontal_wall(Game_t *game, int16_t y, int16_t gapStart, int16_t gapLength) {
    for (int16_t x = 2; x < GRID_WIDTH - 2; x++) {
        if (x < gapStart || x >= gapStart + gapLength) add_wall(game, x, y);
    }
}

static void build_maze(Game_t *game) {
    for (uint16_t i = 0; i < MAX_GRID_CELLS; i++) game->maze.cells[i] = 0U;
    if (game->level == LEVEL_EASY) return;

    uint16_t pattern = (uint16_t)((game->stage - 1U) % 4U);
    if (pattern == 0U) {
        int16_t firstWall = 6 + (int16_t)((game->stage - 1U) % 4U);
        int16_t secondWall = 23 - (int16_t)((game->stage - 1U) % 3U);
        int16_t firstGap = 4 + (int16_t)((game->stage * 3U) % (GRID_HEIGHT - 8));
        int16_t secondGap = 5 + (int16_t)((game->stage * 5U) % (GRID_HEIGHT - 8));
        for (int16_t y = 2; y < GRID_HEIGHT - 2; y++) {
            if (y != firstGap) add_wall(game, firstWall, y);
            if (y != secondGap) add_wall(game, secondWall, y);
        }
    } else if (pattern == 1U) {
        int16_t firstRow = 5 + (int16_t)(game->stage % 5U);
        int16_t secondRow = 17 - (int16_t)(game->stage % 4U);
        add_horizontal_wall(game, firstRow, 5 + (int16_t)(game->stage % 8U), 4);
        add_horizontal_wall(game, secondRow, 19 - (int16_t)(game->stage % 7U), 4);
    } else if (pattern == 2U) {
        int16_t verticalWall = 8 + (int16_t)(game->stage % 5U);
        int16_t horizontalRow = 7 + (int16_t)(game->stage % 9U);
        for (int16_t y = 2; y < GRID_HEIGHT - 2; y++) {
            if (y < horizontalRow - 2 || y > horizontalRow + 2) add_wall(game, verticalWall, y);
        }
        add_horizontal_wall(game, horizontalRow, 14, 5);
    } else {
        int16_t topRow = 5 + (int16_t)(game->stage % 4U);
        int16_t bottomRow = 16 - (int16_t)(game->stage % 3U);
        add_horizontal_wall(game, topRow, 5, 5);
        add_horizontal_wall(game, topRow, 22, 5);
        add_horizontal_wall(game, bottomRow, 12, 5);
    }
    if (game->level == LEVEL_HARD && pattern != 1U) {
        int16_t row = 5 + (int16_t)((game->stage * 2U) % 12U);
        add_horizontal_wall(game, row, 14, 4);
    }

    for (uint16_t i = 0; i < game->snake.length; i++) {
        game->maze.cells[point_index(game->snake.body[i])] = 0U;
    }
    for (uint8_t enemyIndex = 0U; enemyIndex < game->enemyCount; enemyIndex++) {
        for (uint16_t i = 0; i < game->enemies[enemyIndex].length; i++) {
            game->maze.cells[point_index(game->enemies[enemyIndex].body[i])] = 0U;
        }
    }
}

bool Snake_IsObstacle(const Game_t *game, Point_t point) {
    if (game == NULL || !valid_point(point)) return true;
    return game->maze.cells[point_index(point)] != 0U;
}

static uint8_t enemy_count_for_stage(const Game_t *game) {
    if (game == NULL || game->level != LEVEL_HARD) return 0U;
    uint16_t count = 1U + game->stage / 15U;
    return (uint8_t)(count > MAX_AI_SNAKES ? MAX_AI_SNAKES : count);
}

static void init_enemies(Game_t *game) {
    static const Point_t starts[MAX_AI_SNAKES] = {
        {GRID_WIDTH - 5, GRID_HEIGHT - 3},
        {4, 2},
        {GRID_WIDTH - 5, 2},
        {4, GRID_HEIGHT - 3}
    };
    game->enemyCount = enemy_count_for_stage(game);
    for (uint8_t enemyIndex = 0U; enemyIndex < MAX_AI_SNAKES; enemyIndex++) {
        Snake_t *enemy = &game->enemies[enemyIndex];
        enemy->length = 0U;
        enemy->dir = DIR_LEFT;
        enemy->nextDir = DIR_LEFT;
        if (enemyIndex >= game->enemyCount) continue;
        enemy->length = 3U;
        enemy->body[0] = starts[enemyIndex];
        enemy->body[1] = (Point_t){starts[enemyIndex].x + 1, starts[enemyIndex].y};
        enemy->body[2] = (Point_t){starts[enemyIndex].x + 2, starts[enemyIndex].y};
    }
}

static bool occupied_by_enemy(const Game_t *game, Point_t point) {
    for (uint8_t enemyIndex = 0U; enemyIndex < game->enemyCount; enemyIndex++) {
        if (snake_contains(&game->enemies[enemyIndex], point, game->enemies[enemyIndex].length)) return true;
    }
    return false;
}

static bool occupied_by_other_enemy(const Game_t *game, Point_t point, uint8_t excludedIndex) {
    for (uint8_t enemyIndex = 0U; enemyIndex < game->enemyCount; enemyIndex++) {
        if (enemyIndex != excludedIndex &&
            snake_contains(&game->enemies[enemyIndex], point, game->enemies[enemyIndex].length)) return true;
    }
    return false;
}

static int8_t enemy_at(const Game_t *game, Point_t point) {
    for (uint8_t enemyIndex = 0U; enemyIndex < game->enemyCount; enemyIndex++) {
        if (game->enemies[enemyIndex].length > 0U &&
            snake_contains(&game->enemies[enemyIndex], point, game->enemies[enemyIndex].length)) {
            return (int8_t)enemyIndex;
        }
    }
    return -1;
}

static void update_level_for_stage(Game_t *game) {
    if (game->initialLevel == LEVEL_EASY) {
        if (game->stage >= 21U) game->level = LEVEL_HARD;
        else if (game->stage >= 11U) game->level = LEVEL_MEDIUM;
    } else if (game->initialLevel == LEVEL_MEDIUM && game->stage >= 11U) {
        game->level = LEVEL_HARD;
    }
}

void Snake_Init(Game_t *game) {
    Snake_InitLevel(game, LEVEL_EASY);
}

void Snake_InitLevel(Game_t *game, Level_t level) {
    if (game == NULL) return;
    game->level = level;
    game->initialLevel = level;
    game->snake.length = 3;
    game->snake.dir = DIR_RIGHT;
    game->snake.nextDir = DIR_RIGHT;
    game->snake.body[0] = (Point_t){GRID_WIDTH / 2, GRID_HEIGHT / 2};
    game->snake.body[1] = (Point_t){GRID_WIDTH / 2 - 1, GRID_HEIGHT / 2};
    game->snake.body[2] = (Point_t){GRID_WIDTH / 2 - 2, GRID_HEIGHT / 2};
    char playerName[SNAKE_NAME_MAX_LENGTH + 1U];
    memcpy(playerName, game->playerName, sizeof(playerName));
    game->score = 0;
    memcpy(game->playerName, playerName, sizeof(game->playerName));
    game->stage = 1U;
    game->foodsEaten = 0U;
    game->moveIntervalMs = INITIAL_MOVE_INTERVAL_MS;
    game->transitionRemainingMs = 0U;
    game->easterEggUnlocked = false;
    game->foodCount = food_count_for_stage(game->stage);
    game->state = GAME_STATE_RUNNING;
    init_enemies(game);
    build_maze(game);
    Snake_RespawnFood(game);
}

void Snake_SetPlayerName(Game_t *game, const char *name) {
    if (game == NULL) return;
    if (name == NULL) name = "Serpente";
    strncpy(game->playerName, name, SNAKE_NAME_MAX_LENGTH);
    game->playerName[SNAKE_NAME_MAX_LENGTH] = '\0';
}

bool Snake_AppendPlayerNameChar(Game_t *game, char character) {
    if (game == NULL || character < ' ' || character > '~') return false;
    size_t length = strlen(game->playerName);
    if (length >= SNAKE_NAME_MAX_LENGTH) return false;
    game->playerName[length] = character;
    game->playerName[length + 1U] = '\0';
    return true;
}

void Snake_BackspacePlayerName(Game_t *game) {
    if (game == NULL) return;
    size_t length = strlen(game->playerName);
    if (length > 0U) game->playerName[length - 1U] = '\0';
}

const char *Snake_GetPlayerName(const Game_t *game) {
    return game == NULL ? "" : game->playerName;
}

void Snake_SetLevel(Game_t *game, Level_t level) {
    Snake_InitLevel(game, level);
}

void Snake_SetDirection(Game_t *game, Direction_t newDir) {
    if (game == NULL || newDir > DIR_RIGHT) return;
    Direction_t currentDir = game->snake.nextDir;
    if ((newDir == DIR_UP && currentDir != DIR_DOWN) ||
        (newDir == DIR_DOWN && currentDir != DIR_UP) ||
        (newDir == DIR_LEFT && currentDir != DIR_RIGHT) ||
        (newDir == DIR_RIGHT && currentDir != DIR_LEFT)) {
        game->snake.nextDir = newDir;
    }
}

void Snake_TogglePause(Game_t *game) {
    if (game == NULL) return;
    if (game->state == GAME_STATE_RUNNING) game->state = GAME_STATE_PAUSED;
    else if (game->state == GAME_STATE_PAUSED) game->state = GAME_STATE_RUNNING;
}

void Snake_AdvanceTransition(Game_t *game, uint16_t elapsedMs) {
    if (game == NULL || game->state != GAME_STATE_LEVEL_TRANSITION) return;
    if (elapsedMs >= game->transitionRemainingMs) {
        game->transitionRemainingMs = 0U;
        game->state = GAME_STATE_RUNNING;
    } else {
        game->transitionRemainingMs -= elapsedMs;
    }
}

static bool spawn_food_at(Game_t *game, uint8_t foodIndex) {
    uint32_t start = (uint32_t)rand() % MAX_GRID_CELLS;
    for (uint32_t offset = 0; offset < MAX_GRID_CELLS; offset++) {
        uint32_t cell = (start + offset) % MAX_GRID_CELLS;
        Point_t candidate = {(int16_t)(cell % GRID_WIDTH), (int16_t)(cell / GRID_WIDTH)};
        if (game->stage != MAX_GAME_STAGE - 1U && candidate.y == GRID_HEIGHT - 1) continue;
        if (game->stage < FOOD_MAZE_GAP_STAGE && food_between_walls(game, candidate)) continue;
        bool overlapsFood = false;
        for (uint8_t otherIndex = 0U; otherIndex < game->foodCount; otherIndex++) {
            if (otherIndex != foodIndex && same_point(game->foods[otherIndex], candidate)) {
                overlapsFood = true;
                break;
            }
        }
        if (overlapsFood) continue;
        if (!Snake_IsObstacle(game, candidate) &&
            !snake_contains(&game->snake, candidate, game->snake.length) &&
            !occupied_by_enemy(game, candidate)) {
            game->foods[foodIndex] = candidate;
            return true;
        }
    }
    return false;
}

void Snake_RespawnFood(Game_t *game) {
    if (game == NULL) return;
    game->foodCount = food_count_for_stage(game->stage);
    for (uint8_t foodIndex = 0U; foodIndex < game->foodCount; foodIndex++) {
        if (!spawn_food_at(game, foodIndex)) set_game_over(game);
    }
}

static Direction_t enemy_direction(const Game_t *game, uint8_t enemyIndex) {
    const Snake_t *enemy = &game->enemies[enemyIndex];
    Point_t head = enemy->body[0];
    Direction_t best = enemy->dir;
    int16_t bestDistance = 32767;
    Direction_t directions[] = {DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT};
    for (uint16_t i = 0; i < 4; i++) {
        Direction_t direction = directions[i];
        Point_t candidate = next_point(head, direction);
        if ((direction == DIR_UP && enemy->dir == DIR_DOWN) ||
            (direction == DIR_DOWN && enemy->dir == DIR_UP) ||
            (direction == DIR_LEFT && enemy->dir == DIR_RIGHT) ||
            (direction == DIR_RIGHT && enemy->dir == DIR_LEFT) ||
            !valid_point(candidate) || Snake_IsObstacle(game, candidate) ||
            snake_contains(enemy, candidate, enemy->length) ||
            occupied_by_other_enemy(game, candidate, enemyIndex)) continue;
        int16_t distance = abs(candidate.x - game->snake.body[0].x) +
                           abs(candidate.y - game->snake.body[0].y);
        if (distance < bestDistance) {
            bestDistance = distance;
            best = direction;
        }
    }
    return best;
}

static bool move_enemy(Game_t *game, uint8_t enemyIndex) {
    Snake_t *enemy = &game->enemies[enemyIndex];
    if (enemy->length == 0U) return false;
    enemy->dir = enemy_direction(game, enemyIndex);
    Point_t head = next_point(enemy->body[0], enemy->dir);
    if (!valid_point(head) || Snake_IsObstacle(game, head) ||
        snake_contains(enemy, head, enemy->length) ||
        occupied_by_other_enemy(game, head, enemyIndex)) return false;
    if (snake_contains(&game->snake, head, game->snake.length)) {
        if (game->easterEggUnlocked) {
            game->score += (uint32_t)enemy->length * FOOD_SCORE_POINTS;
            enemy->length = 0U;
            return false;
        }
        return true;
    }
    for (uint16_t i = enemy->length - 1U; i > 0U; i--) {
        enemy->body[i] = enemy->body[i - 1];
    }
    enemy->body[0] = head;
    return same_point(head, game->snake.body[0]);
}

bool Snake_Update(Game_t *game) {
    if (game == NULL || game->state != GAME_STATE_RUNNING) return false;
    game->snake.dir = game->snake.nextDir;
    Point_t newHead = next_point(game->snake.body[0], game->snake.dir);
    bool ateFood = false;
    uint8_t eatenFoodIndex = 0U;
    for (uint8_t foodIndex = 0U; foodIndex < game->foodCount; foodIndex++) {
        if (same_point(newHead, game->foods[foodIndex])) {
            ateFood = true;
            eatenFoodIndex = foodIndex;
            break;
        }
    }
    bool growSnake = ateFood && !game->easterEggUnlocked &&
                     ((game->foodsEaten + 1U) % FOODS_PER_SNAKE_GROWTH == 0U);
    uint16_t collisionLength = growSnake ? game->snake.length : game->snake.length - 1U;
    int8_t eatenEnemyIndex = game->easterEggUnlocked ? enemy_at(game, newHead) : -1;

    if (!valid_point(newHead) ||
        (!game->easterEggUnlocked && Snake_IsObstacle(game, newHead)) ||
        snake_contains(&game->snake, newHead, collisionLength) ||
        (!game->easterEggUnlocked && occupied_by_enemy(game, newHead))) {
        set_game_over(game);
        return false;
    }
    if (growSnake && game->snake.length == MAX_SNAKE_LENGTH) {
        set_game_over(game);
        return false;
    }
    bool stageChanged = false;
    bool easterEggTriggered = false;
    if (eatenEnemyIndex >= 0) {
        game->score += (uint32_t)game->enemies[(uint8_t)eatenEnemyIndex].length * FOOD_SCORE_POINTS;
        game->enemies[(uint8_t)eatenEnemyIndex].length = 0U;
    }
    if (ateFood) {
        game->score += FOOD_SCORE_POINTS;
        game->foodsEaten++;
        if (!game->easterEggUnlocked) {
            if (growSnake) game->snake.length++;
            if (game->moveIntervalMs > MIN_MOVE_INTERVAL_MS + MOVE_ACCELERATION_MS) {
                game->moveIntervalMs -= MOVE_ACCELERATION_MS;
            } else {
                game->moveIntervalMs = MIN_MOVE_INTERVAL_MS;
            }
        }
        if (game->foodsEaten % FOODS_PER_STAGE == 0U && game->stage < MAX_GAME_STAGE) {
            game->stage++;
            stageChanged = true;
        }
    }
    if (!stageChanged && is_chry(game) && is_right_corner(newHead) &&
        game->stage < MAX_GAME_STAGE) {
        game->score += FOOD_SCORE_POINTS;
        game->stage++;
        game->easterEggUnlocked = true;
        easterEggTriggered = true;
        stageChanged = true;
    }
    for (uint16_t i = game->snake.length; i > 1U; i--) {
        game->snake.body[i - 1U] = game->snake.body[i - 2U];
    }
    game->snake.body[0] = newHead;

    if (stageChanged) {
        if (game->stage >= MAX_GAME_STAGE) {
            game->easterEggUnlocked = true;
            set_victory(game);
            game->transitionRemainingMs = 0U;
            return true;
        }
        if (easterEggTriggered) {
            game->snake.dir = DIR_RIGHT;
            game->snake.nextDir = DIR_RIGHT;
            game->snake.body[0] = (Point_t){2, GRID_HEIGHT - 1};
            game->snake.body[1] = (Point_t){1, GRID_HEIGHT - 1};
            game->snake.body[2] = (Point_t){0, GRID_HEIGHT - 1};
        }
        update_level_for_stage(game);
        init_enemies(game);
        game->foodCount = food_count_for_stage(game->stage);
        build_maze(game);
        for (uint8_t foodIndex = 0U; foodIndex < game->foodCount; foodIndex++) {
            if (!spawn_food_at(game, foodIndex)) set_game_over(game);
        }
        game->transitionRemainingMs = LEVEL_TRANSITION_MS;
        game->state = GAME_STATE_LEVEL_TRANSITION;
    }
    if (ateFood && !spawn_food_at(game, eatenFoodIndex)) set_game_over(game);
    if (game->state == GAME_STATE_GAMEOVER) return false;
    if (game->state == GAME_STATE_LEVEL_TRANSITION) return true;
    for (uint8_t enemyIndex = 0U; enemyIndex < game->enemyCount; enemyIndex++) {
        if (move_enemy(game, enemyIndex)) {
            set_game_over(game);
            return false;
        }
    }
    return true;
}

uint16_t Snake_GetMoveIntervalMs(const Game_t *game) {
    return game == NULL ? INITIAL_MOVE_INTERVAL_MS : game->moveIntervalMs;
}

uint16_t Snake_GetStage(const Game_t *game) {
    return game == NULL ? 1U : game->stage;
}