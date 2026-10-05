#include "lcd.h"
#include "main.h"
#include "fsmc.h"
#include "stm32f4xx_hal.h"
#include <stddef.h>
#include <string.h>
#include <stdio.h>

/* PALETTA COLORI (Formato RGB565) */
#define COLOR_HOST_BG         0x0841U // Sfondo Scuro Matrice
#define COLOR_HOST_GRID       0x10A2U // Linee Griglia
#define COLOR_HOST_WALL       0x4186U // Ostacoli / Muri
#define COLOR_HOST_FOOD       0xF800U // Cibo (Rosso)
#define COLOR_HOST_HEAD       0x07E0U // Testa Serpente (Verde)
#define COLOR_HOST_BODY       0x57E5U // Corpo Serpente
#define COLOR_WHITE           0xFFFFU
#define COLOR_YELLOW          0xFFE0U
#define COLOR_PANEL           0x0841U
#define COLOR_PANEL_LIGHT     0x2965U
#define COLOR_BLUE            0x051FU
#define COLOR_LIME            0xD7E0U

/* GEOMETRIA CAMPO DA GIOCO FULL SCREEN (320x240) */
#define CELL_SIZE             10U   // 32 * 10 = 320px, 24 * 10 = 240px (Full Screen)
#define BOARD_WIDTH           (GRID_WIDTH * CELL_SIZE)   // 320px
#define BOARD_HEIGHT          (GRID_HEIGHT * CELL_SIZE)  // 240px
#define BOARD_ORIGIN_X        0U                         // Nessun margine a sinistra
#define BOARD_ORIGIN_Y        0U                         // Nessun margine in alto

#define LCD_DATA_ADDRESS (LCD_FSMC_BASE_ADDRESS + LCD_FSMC_DATA_OFFSET)

static volatile uint16_t *const lcdCommand = (volatile uint16_t *)LCD_FSMC_BASE_ADDRESS;
static volatile uint16_t *const lcdData    = (volatile uint16_t *)LCD_DATA_ADDRESS;

static void write_command(uint8_t command) { *lcdCommand = command; }
static void write_data(uint8_t data)       { *lcdData = data; }
static void write_word(uint16_t data)      { *lcdData = data; }

static void set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
  write_command(0x2A);
  write_data((uint8_t)(x0 >> 8)); write_data((uint8_t)(x0 & 0xFF));
  write_data((uint8_t)(x1 >> 8)); write_data((uint8_t)(x1 & 0xFF));

  write_command(0x2B);
  write_data((uint8_t)(y0 >> 8)); write_data((uint8_t)(y0 & 0xFF));
  write_data((uint8_t)(y1 >> 8)); write_data((uint8_t)(y1 & 0xFF));

  write_command(0x2C);
}

static void fill_rect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color)
{
  if (x >= TFT_WIDTH || y >= TFT_HEIGHT || width == 0U || height == 0U) return;
  if ((uint32_t)x + width > TFT_WIDTH)   width = (uint16_t)(TFT_WIDTH - x);
  if ((uint32_t)y + height > TFT_HEIGHT) height = (uint16_t)(TFT_HEIGHT - y);

  set_window(x, y, (uint16_t)(x + width - 1U), (uint16_t)(y + height - 1U));
  
  uint32_t total_pixels = (uint32_t)width * height;
  for (uint32_t i = 0; i < total_pixels; i++) {
    write_word(color);
  }
}

// Funzione per disegnare un cerchio pieno (utile per il corpo del serpente e le mele)
static void fill_circle(uint16_t x0, uint16_t y0, uint16_t radius, uint16_t color)
{
  for (int16_t y = -((int16_t)radius); y <= (int16_t)radius; y++) {
    for (int16_t x = -((int16_t)radius); x <= (int16_t)radius; x++) {
      if (x * x + y * y <= (int16_t)(radius * radius)) {
        fill_rect((uint16_t)(x0 + x), (uint16_t)(y0 + y), 1U, 1U, color);
      }
    }
  }
}

// Disegna un'icona a forma di mela con picciolo verde
static void draw_apple(Point_t point)
{
  if (point.x < 0 || point.x >= GRID_WIDTH || point.y < 0 || point.y >= GRID_HEIGHT) return;

  uint16_t hostY = (uint16_t)(GRID_HEIGHT - 1 - point.y);
  uint16_t px = BOARD_ORIGIN_X + (uint16_t)point.x * CELL_SIZE + (CELL_SIZE / 2U);
  uint16_t py = BOARD_ORIGIN_Y + hostY * CELL_SIZE + (CELL_SIZE / 2U);

  // Corpo rosso della mela
  fill_circle(px, py + 1U, 4U, COLOR_HOST_FOOD);
  
  // Picciolo verde in alto
  fill_rect(px, py - 4U, 1U, 2U, COLOR_LIME);
}

// Disegna la testa del serpente con occhi e lingua orientati in base al movimento
static void draw_snake_head(Point_t head, Point_t neck, uint16_t headColor)
{
  uint16_t hostY = (uint16_t)(GRID_HEIGHT - 1 - head.y);
  uint16_t px = BOARD_ORIGIN_X + (uint16_t)head.x * CELL_SIZE + (CELL_SIZE / 2U);
  uint16_t py = BOARD_ORIGIN_Y + hostY * CELL_SIZE + (CELL_SIZE / 2U);

  // Disegna il corpo/testa tonda principale
  fill_circle(px, py, 4U, headColor);

  // Calcola la direzione del movimento (differenza tra testa e collo)
  int dx = (int)head.x - (int)neck.x;
  int dy = (int)head.y - (int)neck.y;

  // Posiziona occhi e lingua in base a dove si sta muovendo
  if (dx > 0) { // Si muove a Destra
    fill_rect(px + 1U, py - 2U, 1U, 1U, COLOR_WHITE); // Occhio
    fill_rect(px + 1U, py + 1U, 1U, 1U, COLOR_WHITE); // Occhio
    fill_rect(px + 4U, py, 3U, 1U, 0xF800U);         // Lingua a destra
  } else if (dx < 0) { // Si muove a Sinistra
    fill_rect(px - 1U, py - 2U, 1U, 1U, COLOR_WHITE); // Occhio
    fill_rect(px - 1U, py + 1U, 1U, 1U, COLOR_WHITE); // Occhio
    fill_rect(px - 6U, py, 3U, 1U, 0xF800U);         // Lingua a sinistra
  } else if (dy > 0) { // Si muove in Alto (nota: su schermo l'asse Y scende, ma logicamente dy>0 è su)
    fill_rect(px - 2U, py - 1U, 1U, 1U, COLOR_WHITE); // Occhio
    fill_rect(px + 1U, py - 1U, 1U, 1U, COLOR_WHITE); // Occhio
    fill_rect(px, py - 6U, 1U, 3U, 0xF800U);         // Lingua in alto
  } else { // Si muove in Basso
    fill_rect(px - 2U, py + 1U, 1U, 1U, COLOR_WHITE); // Occhio
    fill_rect(px + 1U, py + 1U, 1U, 1U, COLOR_WHITE); // Occhio
    fill_rect(px, py + 4U, 1U, 3U, 0xF800U);         // Lingua in basso
  }
}

void LCD_Clear(uint16_t color)
{
  fill_rect(0, 0, TFT_WIDTH, TFT_HEIGHT, color);
}

static void draw_host_cell(Point_t point, uint16_t color, uint8_t inset)
{
  if (point.x < 0 || point.x >= GRID_WIDTH || point.y < 0 || point.y >= GRID_HEIGHT) return;
  
  uint16_t hostY = (uint16_t)(GRID_HEIGHT - 1 - point.y);
  uint16_t px = BOARD_ORIGIN_X + (uint16_t)point.x * CELL_SIZE + inset;
  uint16_t py = BOARD_ORIGIN_Y + hostY * CELL_SIZE + inset;
  uint16_t sz = CELL_SIZE - inset * 2U;

  fill_rect(px, py, sz, sz, color);
}

static const uint8_t *glyph(char character)
{
  static const uint8_t blank[5] = {0, 0, 0, 0, 0};
  static const uint8_t colon[5]  = {0x00, 0x36, 0x36, 0x00, 0x00}; // Due punti ':'
  
  static const uint8_t digits[][5] = {
    {0x3E,0x51,0x49,0x45,0x3E}, {0x00,0x42,0x7F,0x40,0x00},
    {0x42,0x61,0x51,0x49,0x46}, {0x21,0x41,0x45,0x4B,0x31},
    {0x18,0x14,0x12,0x7F,0x10}, {0x27,0x45,0x45,0x45,0x39},
    {0x3C,0x4A,0x49,0x49,0x30}, {0x01,0x71,0x09,0x05,0x03},
    {0x36,0x49,0x49,0x49,0x36}, {0x06,0x49,0x49,0x29,0x1E}
  };
  static const uint8_t letters[][5] = {
    {0x7E,0x11,0x11,0x11,0x7E}, {0x7F,0x49,0x49,0x49,0x36},
    {0x3E,0x41,0x41,0x41,0x22}, {0x7F,0x41,0x41,0x22,0x1C},
    {0x7F,0x49,0x49,0x49,0x41}, {0x7F,0x09,0x09,0x09,0x01},
    {0x3E,0x41,0x49,0x49,0x7A}, {0x7F,0x08,0x08,0x08,0x7F},
    {0x00,0x41,0x7F,0x41,0x00}, {0x20,0x40,0x41,0x3F,0x01},
    {0x7F,0x08,0x14,0x22,0x41}, {0x7F,0x40,0x40,0x40,0x40},
    {0x7F,0x02,0x0C,0x02,0x7F}, {0x7F,0x04,0x08,0x10,0x7F},
    {0x3E,0x41,0x41,0x41,0x3E}, {0x7F,0x09,0x09,0x09,0x06},
    {0x3E,0x41,0x51,0x21,0x5E}, {0x7F,0x09,0x19,0x29,0x46},
    {0x46,0x49,0x49,0x49,0x31}, {0x01,0x01,0x7F,0x01,0x01},
    {0x3F,0x40,0x40,0x40,0x3F}, {0x1F,0x20,0x40,0x20,0x1F},
    {0x7F,0x20,0x18,0x20,0x7F}, {0x63,0x14,0x08,0x14,0x63},
    {0x07,0x08,0x70,0x08,0x07}, {0x61,0x51,0x49,0x45,0x43}
  };

  if (character == ':') return colon;
  if (character >= '0' && character <= '9') return digits[(uint8_t)(character - '0')];
  if (character >= 'A' && character <= 'Z') return letters[(uint8_t)(character - 'A')];
  if (character >= 'a' && character <= 'z') return letters[(uint8_t)(character - 'a')];
  return blank;
}
static void draw_text(uint16_t x, uint16_t y, const char *text, uint16_t color, uint8_t scale)
{
  while (text != NULL && *text != '\0') {
    const uint8_t *bitmap = glyph(*text++);
    for (uint8_t column = 0; column < 5U; column++) {
      for (uint8_t row = 0; row < 7U; row++) {
        if ((bitmap[column] & (1U << row)) != 0U) {
          fill_rect(x + column * scale, y + row * scale, scale, scale, color);
        }
      }
    }
    x += 6U * scale;
    if (x >= TFT_WIDTH) break;
  }
}

static void draw_text_centered(uint16_t y, const char *text, uint16_t color, uint8_t scale)
{
  uint16_t length = 0U;
  while (text != NULL && text[length] != '\0') length++;
  uint16_t width = length * 6U * scale;
  draw_text(width >= TFT_WIDTH ? 0U : (TFT_WIDTH - width) / 2U, y, text, color, scale);
}

static void draw_background(uint16_t baseColor, uint16_t accentColor)
{
  fill_rect(0, 0, TFT_WIDTH, TFT_HEIGHT, baseColor);
  for (uint16_t y = 0U; y < TFT_HEIGHT; y += CELL_SIZE) fill_rect(0U, y, TFT_WIDTH, 1U, accentColor);
  for (uint16_t x = 0U; x < TFT_WIDTH; x += CELL_SIZE) fill_rect(x, 0U, 1U, TFT_HEIGHT, accentColor);
}

static void draw_panel(uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
  fill_rect(x, y, width, height, COLOR_PANEL);
  fill_rect(x + 2U, y + 2U, width - 4U, 1U, COLOR_PANEL_LIGHT);
  fill_rect(x + 2U, y + height - 3U, width - 4U, 1U, COLOR_HOST_BG);
}

//versione vecchia

/*static void draw_game(const Game_t *game)
{
  // Pulisce l'intero schermo
  fill_rect(0, 0, TFT_WIDTH, TFT_HEIGHT, COLOR_HOST_BG);

  // 1. Linee Griglia Full Screen
  for (uint8_t x = 0; x <= GRID_WIDTH; x++) {
    fill_rect(BOARD_ORIGIN_X + x * CELL_SIZE, BOARD_ORIGIN_Y, 1U, BOARD_HEIGHT, COLOR_HOST_GRID);
  }
  for (uint8_t y = 0; y <= GRID_HEIGHT; y++) {
    fill_rect(BOARD_ORIGIN_X, BOARD_ORIGIN_Y + y * CELL_SIZE, BOARD_WIDTH, 1U, COLOR_HOST_GRID);
  }

  // 2. Muri / Ostacoli
  for (int16_t y = 0; y < GRID_HEIGHT; y++) {
    for (int16_t x = 0; x < GRID_WIDTH; x++) {
      if (Snake_IsObstacle(game, (Point_t){x, y})) {
        draw_host_cell((Point_t){x, y}, COLOR_HOST_WALL, 1U);
      }
    }
  }

  // 3. Cibo (Mele)
  for (uint8_t foodIndex = 0U; foodIndex < game->foodCount; foodIndex++) {
    draw_apple(game->foods[foodIndex]);
  }

  // 4. Serpe Giocatore
  if (game->snake.length > 0) {
    for (uint16_t i = game->snake.length; i > 0U; i--) {
      Point_t pt = game->snake.body[i - 1U];
      if (pt.x < 0 || pt.x >= GRID_WIDTH || pt.y < 0 || pt.y >= GRID_HEIGHT) continue;

      if (i == 1U) {
        // Se il serpente è lungo almeno 2, usa il collo per la direzione, altrimenti usa la testa stessa
        Point_t neck = (game->snake.length > 1U) ? game->snake.body[1U] : pt;
        draw_snake_head(pt, neck, COLOR_HOST_HEAD);
      } else {
        uint16_t hostY = (uint16_t)(GRID_HEIGHT - 1 - pt.y);
        uint16_t px = BOARD_ORIGIN_X + (uint16_t)pt.x * CELL_SIZE + (CELL_SIZE / 2U);
        uint16_t py = BOARD_ORIGIN_Y + hostY * CELL_SIZE + (CELL_SIZE / 2U);
        fill_circle(px, py, 3U, COLOR_HOST_BODY);
      }
    }
  }

  // 5. Serpi Nemiche AI
  static const uint16_t enemyColors[MAX_AI_SNAKES] = {0xFD20U, 0x051FU, 0xF81FU, 0xFFE0U};
  for (uint8_t e = 0U; e < game->enemyCount; e++) {
    for (uint16_t i = game->enemies[e].length; i > 0U; i--) {
      Point_t pt = game->enemies[e].body[i - 1U];
      if (pt.x < 0 || pt.x >= GRID_WIDTH || pt.y < 0 || pt.y >= GRID_HEIGHT) continue;

      if (i == 1U) {
        Point_t neck = (game->enemies[e].length > 1U) ? game->enemies[e].body[1U] : pt;
        draw_snake_head(pt, neck, enemyColors[e]);
      } else {
        uint16_t hostY = (uint16_t)(GRID_HEIGHT - 1 - pt.y);
        uint16_t px = BOARD_ORIGIN_X + (uint16_t)pt.x * CELL_SIZE + (CELL_SIZE / 2U);
        uint16_t py = BOARD_ORIGIN_Y + hostY * CELL_SIZE + (CELL_SIZE / 2U);
        fill_circle(px, py, 3U, enemyColors[e]);
      }
    }
  }
  // 6. Barra Info Superiore (Player Name, Punteggio, Stage)
  fill_rect(0, 0, TFT_WIDTH, 14U, 0x0000U); // Sfondo nero per la barra in alto
  
  char infoStr[40];
  snprintf(infoStr, sizeof(infoStr), "Player: %s  Score: %lu  Stage: %u", 
           game->playerName[0] != '\0' ? game->playerName : "TEST", 
           (unsigned long)game->score, 
           game->stage);
  
  draw_text(6U, 3U, infoStr, COLOR_WHITE, 1U);
}*/

static void draw_game(const Game_t *game)
{
  if (game == NULL) return;

  // 1. NON cancellare l'intero schermo a ogni frame! 
  // Puliamo o aggiorniamo solo l'area di gioco in modo stabile.
  
  // Ridisegniamo lo sfondo della scacchiera di gioco una volta sola per frame
  fill_rect(BOARD_ORIGIN_X, BOARD_ORIGIN_Y, BOARD_WIDTH, BOARD_HEIGHT, COLOR_HOST_BG);

  // 2. Linee Griglia Full Screen
  for (uint8_t x = 0; x <= GRID_WIDTH; x++) {
    fill_rect(BOARD_ORIGIN_X + x * CELL_SIZE, BOARD_ORIGIN_Y, 1U, BOARD_HEIGHT, COLOR_HOST_GRID);
  }
  for (uint8_t y = 0; y <= GRID_HEIGHT; y++) {
    fill_rect(BOARD_ORIGIN_X, BOARD_ORIGIN_Y + y * CELL_SIZE, BOARD_WIDTH, 1U, COLOR_HOST_GRID);
  }

  // 3. Muri / Ostacoli
  for (int16_t y = 0; y < GRID_HEIGHT; y++) {
    for (int16_t x = 0; x < GRID_WIDTH; x++) {
      if (Snake_IsObstacle(game, (Point_t){x, y})) {
        draw_host_cell((Point_t){x, y}, COLOR_HOST_WALL, 1U);
      }
    }
  }

  // 4. Cibo (Mele)
  for (uint8_t foodIndex = 0U; foodIndex < game->foodCount; foodIndex++) {
    draw_apple(game->foods[foodIndex]);
  }

  // 5. Serpe Giocatore (CORRETTO: iterazione dal collo verso la coda e testa disegnata correttamente)
  if (game->snake.length > 0) {
    for (int16_t i = (int16_t)game->snake.length; i > 0; i--) {
      Point_t pt = game->snake.body[i - 1U];
      if (pt.x < 0 || pt.x >= GRID_WIDTH || pt.y < 0 || pt.y >= GRID_HEIGHT) continue;

      if (i == 1U) {
        Point_t neck = (game->snake.length > 1U) ? game->snake.body[1U] : pt;
        draw_snake_head(pt, neck, COLOR_HOST_HEAD);
      } else {
        uint16_t hostY = (uint16_t)(GRID_HEIGHT - 1 - pt.y);
        uint16_t px = BOARD_ORIGIN_X + (uint16_t)pt.x * CELL_SIZE + (CELL_SIZE / 2U);
        uint16_t py = BOARD_ORIGIN_Y + hostY * CELL_SIZE + (CELL_SIZE / 2U);
        fill_circle(px, py, 3U, COLOR_HOST_BODY);
      }
    }
  }

  // 6. Serpi Nemiche AI
  static const uint16_t enemyColors[MAX_AI_SNAKES] = {0xFD20U, 0x051FU, 0xF81FU, 0xFFE0U};
  for (uint8_t e = 0U; e < game->enemyCount; e++) {
    for (int16_t i = (int16_t)game->enemies[e].length; i > 0; i--) {
      Point_t pt = game->enemies[e].body[i - 1U];
      if (pt.x < 0 || pt.x >= GRID_WIDTH || pt.y < 0 || pt.y >= GRID_HEIGHT) continue;

      if (i == 1U) {
        Point_t neck = (game->enemies[e].length > 1U) ? game->enemies[e].body[1U] : pt;
        draw_snake_head(pt, neck, enemyColors[e]);
      } else {
        uint16_t hostY = (uint16_t)(GRID_HEIGHT - 1 - pt.y);
        uint16_t px = BOARD_ORIGIN_X + (uint16_t)pt.x * CELL_SIZE + (CELL_SIZE / 2U);
        uint16_t py = BOARD_ORIGIN_Y + hostY * CELL_SIZE + (CELL_SIZE / 2U);
        fill_circle(px, py, 3U, enemyColors[e]);
      }
    }
  }

  // 7. Barra Info Superiore (Player Name, Punteggio, Stage)
  fill_rect(0, 0, TFT_WIDTH, 14U, 0x0000U); // Sfondo nero per la barra in alto
  
  char infoStr[40];
  snprintf(infoStr, sizeof(infoStr), "Player: %s  Score: %lu  Stage: %u", 
           game->playerName[0] != '\0' ? game->playerName : "TEST", 
           (unsigned long)game->score, 
           game->stage);
  
  draw_text(6U, 3U, infoStr, COLOR_WHITE, 1U);
}

void LCD_Init(void)
{
  HAL_Delay(5U);
  write_command(0x01); HAL_Delay(120U); 
  write_command(0x11); HAL_Delay(120U); // Sleep Out

  write_command(0x3A); write_data(0x55); 
  write_command(0x36); write_data(0x28); 

  write_command(0x29); HAL_Delay(20U); 

  fill_rect(0, 0, TFT_WIDTH, TFT_HEIGHT, COLOR_HOST_BG);
}

void LCD_Render(const Game_t *game, const Console_t *console)
{
  if (game == NULL || console == NULL) return;

  static ConsoleState_t last_rendered_state = (ConsoleState_t)-1;
  static Level_t last_selected_level = (Level_t)-1; // Per aggiornare la difficoltà a schermo

  // Se cambia lo stato della console, ridisegniamo lo sfondo fisso del menu
  if (console->state != last_rendered_state)
  {
    switch (console->state)
    {
      case CONSOLE_STATE_OFF:
        draw_background(COLOR_HOST_BG, COLOR_HOST_GRID);
        draw_panel(20U, 40U, 280U, 160U);
        draw_text_centered(55U, "CONSOLE", COLOR_WHITE, 1U);
        draw_text_centered(75U, "CUBENIRO", COLOR_LIME, 2U);
        draw_text_centered(115U, "K1 POWER", COLOR_YELLOW, 1U);
        break;

      case CONSOLE_STATE_START:
        draw_background(COLOR_HOST_BG, COLOR_HOST_GRID);
        draw_panel(40U, 50U, 240U, 140U);
        draw_text_centered(70U, "SNAKE", COLOR_LIME, 2U);
        draw_text_centered(140U, "NEW GAME", COLOR_YELLOW, 1U);
        break;

      case CONSOLE_STATE_NAME:
        draw_background(COLOR_HOST_BG, COLOR_HOST_GRID);
        draw_panel(20U, 10U, 280U, 220U);
        draw_text_centered(25U, "ENTER PLAYER NAME", COLOR_WHITE, 1U);
        break;

      case CONSOLE_STATE_DIFFICULTY:
        draw_background(COLOR_BLUE, COLOR_HOST_GRID);
        draw_panel(40U, 50U, 240U, 140U);
        draw_text_centered(70U, "DIFFICULTY", COLOR_WHITE, 1U);
        draw_text_centered(140U, "UP/DOWN SELECT", COLOR_WHITE, 1U);
        draw_text_centered(160U, "CENTER START", COLOR_YELLOW, 1U);
        break;

      default:
        break;
    }

    last_rendered_state = console->state;
    last_selected_level = (Level_t)-1; // Forza il refresh della difficoltà
  }

  // Rendering dinamico in base allo stato attuale
  switch (console->state)
  {
    case CONSOLE_STATE_OFF:
    {
      uint32_t total_seconds = HAL_GetTick() / 1000U;
      uint8_t hours   = (uint8_t)((total_seconds / 3600U) % 24U);
      uint8_t minutes = (uint8_t)((total_seconds / 60U) % 60U);
      char dateTimeStr[32];
      snprintf(dateTimeStr, sizeof(dateTimeStr), "16/01/2026 %02u:%02u - 35 C", hours, minutes);
      draw_text_centered(145U, dateTimeStr, COLOR_WHITE, 1U);
      break;
    }
    case CONSOLE_STATE_NAME:
      draw_text(60U, 50U, game->playerName, COLOR_YELLOW, 1U);
      draw_text_centered(88U, game->playerName, COLOR_LIME, 2U);
      break;

    case CONSOLE_STATE_DIFFICULTY:
      // Aggiorna a schermo la selezione della difficoltà se è cambiata
      if (console->selectedLevel != last_selected_level) {
        // Pulisci l'area della scritta prima di scriverla sopra
        fill_rect(70U, 100U, 180U, 20U, COLOR_BLUE); 
        
        if (console->selectedLevel == LEVEL_EASY) 
          draw_text_centered(104U, "> EASY <", COLOR_LIME, 2U);
        else if (console->selectedLevel == LEVEL_MEDIUM) 
          draw_text_centered(104U, "> MEDIUM <", COLOR_YELLOW, 2U);
        else 
          draw_text_centered(104U, "> HARD <", 0xF800U, 2U);
          
        last_selected_level = console->selectedLevel;
      }
      break;

    default:
      if (console->state != CONSOLE_STATE_START) {
        draw_game(game);
      }
      break;
  }
}