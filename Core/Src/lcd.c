#include "lcd.h"
#include "main.h"
#include "fsmc.h"
#include "external_flash.h"
#include "stm32f4xx_hal.h"
#include <stddef.h>
#include <string.h>
#include <stdio.h>

/* COLOR */
#define COLOR_HOST_BG         0x0841U 
#define COLOR_HOST_GRID       0x10A2U 
#define COLOR_HOST_WALL       0x4186U 
#define COLOR_HOST_FOOD       0xF800U 
#define COLOR_HOST_HEAD       0x0560U /* Rich vibrant emerald green for maximum eye/tongue contrast */
#define COLOR_HOST_BODY       0x0440U /* Darker green body so head stands out */
#define COLOR_WHITE           0xFFFFU
#define COLOR_YELLOW          0xFFE0U
#define COLOR_PANEL           0x0841U
#define COLOR_PANEL_LIGHT     0x2965U
#define COLOR_BLUE            0x051FU
#define COLOR_LIME            0xD7E0U
#define COLOR_RED             0xF800U
#define COLOR_ORANGE          0xFD20U
#define COLOR_CYAN            0x07FFU
#define COLOR_DARK_GRAY       0x18C3U
#define COLOR_GRAY            0x4208U
#define COLOR_GREEN           0x0400U

/* GAME BOARD DIMENSIONS */
#define CELL_SIZE             10U   // 32 * 10 = 320px width, 21 * 10 = 210px height
#define BOARD_WIDTH           (GRID_WIDTH * CELL_SIZE)   // 320px
#define BOARD_HEIGHT          (GRID_HEIGHT * CELL_SIZE)  // 210px
#define BOARD_ORIGIN_X        0U                         // 0px        
#define BOARD_ORIGIN_Y        15U                        // starts below the 15px top bar (Y: 15..224)    

#define LCD_DATA_ADDRESS (LCD_FSMC_BASE_ADDRESS + LCD_FSMC_DATA_OFFSET)

// FSMC memory-mapped register pointers 
static volatile uint16_t *const lcdCommand = (volatile uint16_t *)LCD_FSMC_BASE_ADDRESS;
static volatile uint16_t *const lcdData    = (volatile uint16_t *)LCD_DATA_ADDRESS;


static void write_command(uint8_t command) { *lcdCommand = command; }
static void write_data(uint8_t data)       { *lcdData = data; }
static void write_word(uint16_t data)      { *lcdData = data; }

/**
 * Sets the active rectangular drawing window (Column & Page address).
 */
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

/**
 * Fills a solid colored rectangle with boundary clipping.
 */
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

/**
 * Clears the entire display with a solid background color.
 */
void LCD_Clear(uint16_t color)
{
  fill_rect(0, 0, TFT_WIDTH, TFT_HEIGHT, color);
}

static const uint8_t *glyph(char character)
{
  static const uint8_t blank[5] = {0, 0, 0, 0, 0};
  static const uint8_t colon[5]  = {0x00, 0x36, 0x36, 0x00, 0x00}; //  ':'
  static const uint8_t hyphen[5] = {0x08, 0x08, 0x08, 0x08, 0x08}; // '-'
  static const uint8_t excl[5]   = {0x00, 0x00, 0x5F, 0x00, 0x00}; //  '!'
  static const uint8_t dot[5]    = {0x00, 0x60, 0x60, 0x00, 0x00}; //  '.'
  static const uint8_t lbrack[5] = {0x00, 0x7F, 0x41, 0x41, 0x00}; // '['
  static const uint8_t rbrack[5] = {0x00, 0x41, 0x41, 0x7F, 0x00}; // ']'
  static const uint8_t gt[5]     = {0x41, 0x22, 0x14, 0x08, 0x00}; // '>'
  static const uint8_t lt[5]     = {0x08, 0x14, 0x22, 0x41, 0x00}; // '<'
  
  static const uint8_t digits[][5] = {
    {0x3E,0x51,0x49,0x45,0x3E}, // '0'
    {0x00,0x42,0x7F,0x40,0x00}, // '1'
    {0x42,0x61,0x51,0x49,0x46}, // '2'
    {0x21,0x41,0x45,0x4B,0x31}, // '3'
    {0x18,0x14,0x12,0x7F,0x10}, // '4'
    {0x27,0x45,0x45,0x45,0x39}, // '5'
    {0x3C,0x4A,0x49,0x49,0x30}, // '6'
    {0x01,0x71,0x09,0x05,0x03}, // '7'
    {0x36,0x49,0x49,0x49,0x36}, // '8'
    {0x06,0x49,0x49,0x29,0x1E}  // '9'
  };
  static const uint8_t letters[][5] = {
    {0x7E,0x11,0x11,0x11,0x7E}, // 'A'
    {0x7F,0x49,0x49,0x49,0x36}, // 'B'
    {0x3E,0x41,0x41,0x41,0x22}, // 'C'
    {0x7F,0x41,0x41,0x22,0x1C}, // 'D'
    {0x7F,0x49,0x49,0x49,0x41}, // 'E'
    {0x7F,0x09,0x09,0x09,0x01}, // 'F'
    {0x3E,0x41,0x49,0x49,0x7A}, // 'G'
    {0x7F,0x08,0x08,0x08,0x7F}, // 'H'
    {0x00,0x41,0x7F,0x41,0x00}, // 'I'
    {0x20,0x40,0x41,0x3F,0x01}, // 'J'
    {0x7F,0x08,0x14,0x22,0x41}, // 'K'
    {0x7F,0x40,0x40,0x40,0x40}, // 'L'
    {0x7F,0x02,0x0C,0x02,0x7F}, // 'M'
    {0x7F,0x04,0x08,0x10,0x7F}, // 'N'
    {0x3E,0x41,0x41,0x41,0x3E}, // 'O'
    {0x7F,0x09,0x09,0x09,0x06}, // 'P'
    {0x3E,0x41,0x51,0x21,0x5E}, // 'Q'
    {0x7F,0x09,0x19,0x29,0x46}, // 'R'
    {0x46,0x49,0x49,0x49,0x31}, // 'S'
    {0x01,0x01,0x7F,0x01,0x01}, // 'T'
    {0x3F,0x40,0x40,0x40,0x3F}, // 'U'
    {0x1F,0x20,0x40,0x20,0x1F}, // 'V'
    {0x7F,0x20,0x18,0x20,0x7F}, // 'W'
    {0x63,0x14,0x08,0x14,0x63}, // 'X'
    {0x07,0x08,0x70,0x08,0x07}, // 'Y'
    {0x61,0x51,0x49,0x45,0x43}  // 'Z'
  };

  if (character == ':') return colon;
  if (character == '-') return hyphen;
  if (character == '!') return excl;
  if (character == '.') return dot;
  if (character == '[') return lbrack;
  if (character == ']') return rbrack;
  if (character == '>') return gt;
  if (character == '<') return lt;
  if (character >= '0' && character <= '9') return digits[(uint8_t)(character - '0')];
  if (character >= 'A' && character <= 'Z') return letters[(uint8_t)(character - 'A')];
  if (character >= 'a' && character <= 'z') return letters[(uint8_t)(character - 'a')];
  return blank;
}
/**
 * @brief Draws a text string 
 */
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

/**
 * @brief Draws horizontally centered text 
 */
static void draw_text_centered(uint16_t y, const char *text, uint16_t color, uint8_t scale)
{
  uint16_t length = 0U;
  while (text != NULL && text[length] != '\0') length++;
  uint16_t width = length * 6U * scale;
  draw_text(width >= TFT_WIDTH ? 0U : (TFT_WIDTH - width) / 2U, y, text, color, scale);
}

/**
 * @brief Streams and renders an  image stored in external SPI Flash.
 */
static void draw_flash_image(uint32_t flashAddress, uint16_t x0, uint16_t y0, uint16_t width, uint16_t height)
{
  if (!ExternalFlash_IsReady()) {
    ExternalFlash_Init();
  }

  if (!ExternalFlash_IsReady()) {
    fill_rect(x0, y0, width, height, COLOR_HOST_BG);
    return;
  }

  set_window(x0, y0, (uint16_t)(x0 + width - 1U), (uint16_t)(y0 + height - 1U));

  static uint32_t last_logged_addr = 0xFFFFFFFFU;
  if (flashAddress != last_logged_addr) {
    printf("[FLASH] Caricamento immagine da memoria esterna (Addr: 0x%06lX, Dim: %ux%u)...\r\n",
           (unsigned long)flashAddress, width, height);
    last_logged_addr = flashAddress;
  }

  #define FLASH_CHUNK_PIXELS 512U
  uint8_t byteBuf[FLASH_CHUNK_PIXELS * 2U];
  uint32_t totalPixels = (uint32_t)width * height;
  uint32_t currentAddr = flashAddress;

  while (totalPixels > 0U) {
    uint32_t toRead = (totalPixels > FLASH_CHUNK_PIXELS) ? FLASH_CHUNK_PIXELS : totalPixels;
    if (ExternalFlash_Read(currentAddr, byteBuf, toRead * 2U)) {
      for (uint32_t i = 0U; i < toRead; i++) {
        uint16_t color = ((uint16_t)byteBuf[i * 2U] << 8) | byteBuf[i * 2U + 1U];
        write_word(color);
      }
    } else {
      for (uint32_t i = 0U; i < toRead; i++) {
        write_word(COLOR_HOST_BG);
      }
    }
    currentAddr += toRead * 2U;
    totalPixels -= toRead;
  }
}

/**
 * @brief Draws a full-screen grid background 
 */
static void draw_background(uint16_t baseColor, uint16_t accentColor)
{
  fill_rect(0, 0, TFT_WIDTH, TFT_HEIGHT, baseColor);
  for (uint16_t y = 0U; y < TFT_HEIGHT; y += CELL_SIZE) fill_rect(0U, y, TFT_WIDTH, 1U, accentColor);
  for (uint16_t x = 0U; x < TFT_WIDTH; x += CELL_SIZE) fill_rect(x, 0U, 1U, TFT_HEIGHT, accentColor);
}

/**
 * @brief Renders the full-screen 320x240 Snake background wallpaper from external Flash.
 */
static void draw_snake_background(void)
{
  draw_flash_image(EXTERNAL_FLASH_SNAKE_ADDRESS, 0U, 0U, TFT_WIDTH, TFT_HEIGHT);
}

/**
* LEVEL SELECT PANEL
 */
static void draw_panel(uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
  fill_rect(x, y, width, height, COLOR_PANEL);
  fill_rect(x + 2U, y + 2U, width - 4U, 1U, COLOR_PANEL_LIGHT);
  fill_rect(x + 2U, y + height - 3U, width - 4U, 1U, COLOR_HOST_BG);
}

/**
 *  Interactive button with label, border and selection highlight
 */
static void draw_btn(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const char *label, bool selected, uint16_t activeBgColor, uint16_t textColor)
{
  uint16_t bg = selected ? activeBgColor : COLOR_DARK_GRAY;
  uint16_t border = selected ? COLOR_WHITE : COLOR_GRAY;
  uint16_t txtCol = selected ? COLOR_WHITE : textColor;

  fill_rect(x, y, width, height, bg);
  // Bordo
  fill_rect(x, y, width, 2U, border);
  fill_rect(x, y + height - 2U, width, 2U, border);
  fill_rect(x, y, 2U, height, border);
  fill_rect(x + width - 2U, y, 2U, height, border);

  uint16_t len = 0U;
  while (label != NULL && label[len] != '\0') len++;
  uint16_t txtWidth = len * 6U;
  uint16_t tx = (txtWidth < width) ? (x + (width - txtWidth) / 2U) : (x + 2U);
  uint16_t ty = y + (height > 7U ? (height - 7U) / 2U : 1U);
  draw_text(tx, ty, label, txtCol, 1U);
}


typedef enum {
  CELL_EMPTY = 0,
  CELL_WALL,
  CELL_FOOD,
  CELL_PLAYER_BODY,
  CELL_PLAYER_HEAD_UP,
  CELL_PLAYER_HEAD_DOWN,
  CELL_PLAYER_HEAD_LEFT,
  CELL_PLAYER_HEAD_RIGHT,
  CELL_ENEMY_BASE
} CellType_t;

static bool game_board_needs_full_redraw = true;
static const uint16_t enemyColors[MAX_AI_SNAKES] = {0xFD20U, 0x051FU, 0xF81FU, 0xFFE0U};

//render snake body

static void render_body(uint16_t buf[CELL_SIZE][CELL_SIZE], uint16_t bodyColor)
{
  for (int8_t r = 2; r <= 8; r++) {
    for (int8_t c = 2; c <= 8; c++) {
      int8_t dr = r - 5;
      int8_t dc = c - 5;
      if (dr * dr + dc * dc <= 9) {
        buf[r][c] = bodyColor;
      }
    }
  }
}

// render snake head 
static void render_head(uint16_t buf[CELL_SIZE][CELL_SIZE], uint16_t headColor, Direction_t dir)
{

  for (int8_t r = 1; r <= 8; r++) {
    for (int8_t c = 1; c <= 8; c++) {
      int8_t dr = r - 5;
      int8_t dc = c - 5;
      if (dr * dr + dc * dc <= 14) {
        buf[r][c] = headColor;
      }
    }
  }


  if (dir == DIR_RIGHT) {
    // Upper eye: bold 3x2 white sclera with sharp dark pupil
    buf[1][4] = COLOR_WHITE; buf[1][5] = COLOR_WHITE; buf[1][6] = COLOR_WHITE;
    buf[2][4] = COLOR_WHITE; buf[2][5] = 0x0000U;    buf[2][6] = COLOR_WHITE;
    buf[3][4] = COLOR_WHITE; buf[3][5] = 0x0000U;    buf[3][6] = COLOR_WHITE;
    // Lower eye: bold 3x2 white sclera with sharp dark pupil
    buf[5][4] = COLOR_WHITE; buf[5][5] = COLOR_WHITE; buf[5][6] = COLOR_WHITE;
    buf[6][4] = COLOR_WHITE; buf[6][5] = 0x0000U;    buf[6][6] = COLOR_WHITE;
    buf[7][4] = COLOR_WHITE; buf[7][5] = 0x0000U;    buf[7][6] = COLOR_WHITE;
    // Red tongue: elongated stem with forked tips
    buf[4][7] = COLOR_RED; buf[4][8] = COLOR_RED;
    buf[3][9] = COLOR_RED; buf[5][9] = COLOR_RED; // forked tip
  } else if (dir == DIR_LEFT) {
    // Upper eye
    buf[1][3] = COLOR_WHITE; buf[1][4] = COLOR_WHITE; buf[1][5] = COLOR_WHITE;
    buf[2][3] = COLOR_WHITE; buf[2][4] = 0x0000U;    buf[2][5] = COLOR_WHITE;
    buf[3][3] = COLOR_WHITE; buf[3][4] = 0x0000U;    buf[3][5] = COLOR_WHITE;
    // Lower eye
    buf[5][3] = COLOR_WHITE; buf[5][4] = COLOR_WHITE; buf[5][5] = COLOR_WHITE;
    buf[6][3] = COLOR_WHITE; buf[6][4] = 0x0000U;    buf[6][5] = COLOR_WHITE;
    buf[7][3] = COLOR_WHITE; buf[7][4] = 0x0000U;    buf[7][5] = COLOR_WHITE;
    // Red tongue: elongated stem with forked tips
    buf[4][2] = COLOR_RED; buf[4][1] = COLOR_RED;
    buf[3][0] = COLOR_RED; buf[5][0] = COLOR_RED; // forked tip
  } else if (dir == DIR_DOWN) { // hostY = GRID_HEIGHT - 1 - y: DIR_DOWN maps to decreasing row 'r'
    // Left eye
    buf[3][1] = COLOR_WHITE; buf[3][2] = COLOR_WHITE; buf[3][3] = COLOR_WHITE;
    buf[2][1] = COLOR_WHITE; buf[2][2] = 0x0000U;    buf[2][3] = COLOR_WHITE;
    buf[1][1] = COLOR_WHITE; buf[1][2] = 0x0000U;    buf[1][3] = COLOR_WHITE;
    // Right eye
    buf[3][5] = COLOR_WHITE; buf[3][6] = COLOR_WHITE; buf[3][7] = COLOR_WHITE;
    buf[2][5] = COLOR_WHITE; buf[2][6] = 0x0000U;    buf[2][7] = COLOR_WHITE;
    buf[1][5] = COLOR_WHITE; buf[1][6] = 0x0000U;    buf[1][7] = COLOR_WHITE;
    // Red tongue: elongated stem with forked tips
    buf[2][4] = COLOR_RED; buf[1][4] = COLOR_RED;
    buf[0][3] = COLOR_RED; buf[0][5] = COLOR_RED; // forked tip
  } else { // DIR_UP: maps to increasing row 'r' on LCD
    // Left eye
    buf[5][1] = COLOR_WHITE; buf[5][2] = COLOR_WHITE; buf[5][3] = COLOR_WHITE;
    buf[6][1] = COLOR_WHITE; buf[6][2] = 0x0000U;    buf[6][3] = COLOR_WHITE;
    buf[7][1] = COLOR_WHITE; buf[7][2] = 0x0000U;    buf[7][3] = COLOR_WHITE;
    // Right eye
    buf[5][5] = COLOR_WHITE; buf[5][6] = COLOR_WHITE; buf[5][7] = COLOR_WHITE;
    buf[6][5] = COLOR_WHITE; buf[6][6] = 0x0000U;    buf[6][7] = COLOR_WHITE;
    buf[7][5] = COLOR_WHITE; buf[7][6] = 0x0000U;    buf[7][7] = COLOR_WHITE;
    // Red tongue: elongated stem with forked tips
    buf[7][4] = COLOR_RED; buf[8][4] = COLOR_RED;
    buf[9][3] = COLOR_RED; buf[9][5] = COLOR_RED; // forked tip
  }
}

// RENDER A SINGLE CELL IN THE GRID

static void render_cell(uint16_t x, uint16_t y, uint8_t cellType)
{
  if (x >= GRID_WIDTH || y >= GRID_HEIGHT) return;

  uint16_t hostY = (uint16_t)(GRID_HEIGHT - 1 - y);
  uint16_t px = BOARD_ORIGIN_X + x * CELL_SIZE;
  uint16_t py = BOARD_ORIGIN_Y + hostY * CELL_SIZE;

  uint16_t buf[CELL_SIZE][CELL_SIZE];

  
  for (uint8_t r = 0; r < CELL_SIZE; r++) {
    for (uint8_t c = 0; c < CELL_SIZE; c++) {
      buf[r][c] = COLOR_HOST_BG;
    }
  }


  for (uint8_t c = 0; c < CELL_SIZE; c++) buf[0][c] = COLOR_HOST_GRID;
  for (uint8_t r = 0; r < CELL_SIZE; r++) buf[r][0] = COLOR_HOST_GRID;
  if (x == (GRID_WIDTH - 1U)) {
    for (uint8_t r = 0; r < CELL_SIZE; r++) buf[r][CELL_SIZE - 1U] = COLOR_HOST_GRID;
  }
  if (hostY == (GRID_HEIGHT - 1U)) {
    for (uint8_t c = 0; c < CELL_SIZE; c++) buf[CELL_SIZE - 1U][c] = COLOR_HOST_GRID;
  }

 
  switch (cellType) {
    case CELL_WALL:
      for (uint8_t r = 1U; r <= 8U; r++) {
        for (uint8_t c = 1U; c <= 8U; c++) {
          buf[r][c] = COLOR_HOST_WALL;
        }
      }
      break;

    case CELL_FOOD:
      for (int8_t r = 2; r <= 8; r++) {
        for (int8_t c = 2; c <= 8; c++) {
          int8_t dr = r - 5;
          int8_t dc = c - 5;
          if (dr * dr + dc * dc <= 10) {
            buf[r][c] = COLOR_HOST_FOOD;
          }
        }
      }
      buf[1][5] = COLOR_LIME;
      buf[2][5] = COLOR_LIME;
      break;

    case CELL_PLAYER_BODY:
      render_body(buf, COLOR_HOST_BODY);
      break;

    case CELL_PLAYER_HEAD_UP:
    case CELL_PLAYER_HEAD_DOWN:
    case CELL_PLAYER_HEAD_LEFT:
    case CELL_PLAYER_HEAD_RIGHT:
      render_head(buf, COLOR_HOST_HEAD, (Direction_t)(cellType - CELL_PLAYER_HEAD_UP));
      break;

    default:
      if (cellType >= CELL_ENEMY_BASE && cellType < (CELL_ENEMY_BASE + (MAX_AI_SNAKES * 5U))) {
        uint8_t enemyIndex = (uint8_t)((cellType - CELL_ENEMY_BASE) / 5U);
        uint8_t sub = (uint8_t)((cellType - CELL_ENEMY_BASE) % 5U);
        if (sub == 0U) {
          render_body(buf, enemyColors[enemyIndex]);
        } else {
          render_head(buf, enemyColors[enemyIndex], (Direction_t)(sub - 1U));
        }
      }
      break;
  }

  set_window(px, py, (uint16_t)(px + CELL_SIZE - 1U), (uint16_t)(py + CELL_SIZE - 1U));
  for (uint8_t r = 0; r < CELL_SIZE; r++) {
    for (uint8_t c = 0; c < CELL_SIZE; c++) {
      write_word(buf[r][c]);
    }
  }
}

static void draw_top_info_bar(const Game_t *game)
{
  fill_rect(0, 0, TFT_WIDTH, 15U, 0x0000U);
  char infoStr[64];
  snprintf(infoStr, sizeof(infoStr), "Player: %s  Score: %lu  Stage: %u", 
           game->playerName[0] != '\0' ? game->playerName : "TEST", 
           (unsigned long)game->score, 
           (unsigned int)game->stage);
  draw_text(6U, 4U, infoStr, COLOR_WHITE, 1U);
}

static void draw_bottom_help_bar(void)
{
  fill_rect(0, 225U, TFT_WIDTH, 15U, 0x0000U);
  draw_text_centered(228U, "PRESS X: PAUSE", COLOR_YELLOW, 1U);
}
// DRAW PAUSE PANEL
static void draw_pause_overlay(const Console_t *console)
{
  draw_panel(50U, 60U, 220U, 115U);
  fill_rect(54U, 64U, 212U, 24U, COLOR_BLUE);
  draw_text_centered(70U, "GAME PAUSED", COLOR_YELLOW, 2U);

  // 2 Botton
  bool resumeSel = (console->pauseBtn == PAUSE_BTN_RESUME);
  bool returnSel = (console->pauseBtn == PAUSE_BTN_RETURN_START);

  draw_btn(70U, 100U, 180U, 28U, "RESUME", resumeSel, COLOR_GREEN, COLOR_LIME);
  draw_btn(70U, 135U, 180U, 28U, "RETURN HOME", returnSel, COLOR_RED, COLOR_WHITE);
}

//DRAW GAME OVER PANEL
static void draw_gameover_overlay(const Game_t *game, const Console_t *console)
{
  //  Game Over Arcade POPUP
  draw_panel(30U, 25U, 260U, 195U);
  fill_rect(34U, 29U, 252U, 26U, COLOR_RED);
  draw_text_centered(34U, "GAME OVER", COLOR_WHITE, 2U);

  char scoreBuf[32];
  snprintf(scoreBuf, sizeof(scoreBuf), "FINAL SCORE: %lu", (unsigned long)game->score);
  draw_text_centered(62U, scoreBuf, COLOR_YELLOW, 1U);

  // TOP PLAYERS
  fill_rect(40U, 76U, 240U, 1U, COLOR_PANEL_LIGHT);
  draw_text(45U, 82U, "TOP PLAYERS:", COLOR_LIME, 1U);
  Snake_Leaderboard_Load();
  uint8_t count = Snake_Leaderboard_Count();
  for (uint8_t i = 0U; i < 3U; i++) {
    char rowStr[36];
    if (i < count) {
      const LeaderboardEntry_t *entry = Snake_Leaderboard_Get(i);
      snprintf(rowStr, sizeof(rowStr), "%u. %-10s %lu", (unsigned int)(i + 1U), entry->playerName, (unsigned long)entry->score);
    } else {
      snprintf(rowStr, sizeof(rowStr), "%u. ---------- 0", (unsigned int)(i + 1U));
    }
    draw_text(45U, 96U + (i * 12U), rowStr, COLOR_WHITE, 1U);
  }

  // 3 Botton
  bool restartSel = (console->gameOverBtn == GAMEOVER_BTN_RESTART);
  bool viewScoreSel = (console->gameOverBtn == GAMEOVER_BTN_VIEW_SCORE);
  bool returnSel = (console->gameOverBtn == GAMEOVER_BTN_RETURN_START);

  draw_btn(40U, 138U, 240U, 20U, "RESTART", restartSel, COLOR_GREEN, COLOR_LIME);
  draw_btn(40U, 162U, 240U, 20U, "VIEW SCORE", viewScoreSel, COLOR_BLUE, COLOR_CYAN);
  draw_btn(40U, 186U, 240U, 20U, "RETURN HOME", returnSel, COLOR_RED, COLOR_WHITE);
}

//DRAW VICTORY PANEL
static void draw_victory_overlay(const Game_t *game, const Console_t *console)
{
  // Victory Arcade POPUP
  draw_panel(30U, 25U, 260U, 195U);
  fill_rect(34U, 29U, 252U, 26U, COLOR_GREEN);
  draw_text_centered(34U, "VICTORY! YOU WIN!", COLOR_WHITE, 2U);

  char scoreBuf[32];
  snprintf(scoreBuf, sizeof(scoreBuf), "FINAL SCORE: %lu", (unsigned long)game->score);
  draw_text_centered(62U, scoreBuf, COLOR_YELLOW, 1U);

  // TOP PLAYERS
  fill_rect(40U, 76U, 240U, 1U, COLOR_PANEL_LIGHT);
  draw_text(45U, 82U, "TOP PLAYERS:", COLOR_LIME, 1U);
  Snake_Leaderboard_Load();
  uint8_t count = Snake_Leaderboard_Count();
  for (uint8_t i = 0U; i < 3U; i++) {
    char rowStr[36];
    if (i < count) {
      const LeaderboardEntry_t *entry = Snake_Leaderboard_Get(i);
      snprintf(rowStr, sizeof(rowStr), "%u. %-10s %lu", (unsigned int)(i + 1U), entry->playerName, (unsigned long)entry->score);
    } else {
      snprintf(rowStr, sizeof(rowStr), "%u. ---------- 0", (unsigned int)(i + 1U));
    }
    draw_text(45U, 96U + (i * 12U), rowStr, COLOR_WHITE, 1U);
  }

  // 3 Botton (stessi tasti del GameOver: RESTART, VIEW SCORE, RETURN HOME)
  bool restartSel = (console->gameOverBtn == GAMEOVER_BTN_RESTART);
  bool viewScoreSel = (console->gameOverBtn == GAMEOVER_BTN_VIEW_SCORE);
  bool returnSel = (console->gameOverBtn == GAMEOVER_BTN_RETURN_START);

  draw_btn(40U, 138U, 240U, 20U, "RESTART", restartSel, COLOR_GREEN, COLOR_LIME);
  draw_btn(40U, 162U, 240U, 20U, "VIEW SCORE", viewScoreSel, COLOR_BLUE, COLOR_CYAN);
  draw_btn(40U, 186U, 240U, 20U, "RETURN HOME", returnSel, COLOR_RED, COLOR_WHITE);
}

static void draw_game(const Game_t *game, const Console_t *console)
{
  if (game == NULL || console == NULL) return;

  static uint8_t prev_board[GRID_WIDTH][GRID_HEIGHT];
  static uint8_t curr_board[GRID_WIDTH][GRID_HEIGHT];
  static uint32_t last_rendered_score = 0xFFFFFFFFUL;
  static uint16_t last_rendered_stage = 0xFFFFU;
  static GameState_t last_rendered_game_state = (GameState_t)-1;
  static PauseButton_t last_pause_btn = (PauseButton_t)-1;
  static GameOverButton_t last_gameover_btn = (GameOverButton_t)-1;


  if (game_board_needs_full_redraw || game->stage != last_rendered_stage) {
    memset(prev_board, 0xFF, sizeof(prev_board));
    draw_top_info_bar(game);
    draw_bottom_help_bar();
    last_rendered_score = game->score;
    last_rendered_stage = game->stage;
    game_board_needs_full_redraw = false;
  }

  memset(curr_board, CELL_EMPTY, sizeof(curr_board));

  // 1. WALLS
  for (int16_t y = 0; y < GRID_HEIGHT; y++) {
    for (int16_t x = 0; x < GRID_WIDTH; x++) {
      if (Snake_IsObstacle(game, (Point_t){x, y})) {
        curr_board[x][y] = CELL_WALL;
      }
    }
  }

  // 2. FOOD
  for (uint8_t foodIndex = 0U; foodIndex < game->foodCount; foodIndex++) {
    Point_t pt = game->foods[foodIndex];
    if (pt.x >= 0 && pt.x < GRID_WIDTH && pt.y >= 0 && pt.y < GRID_HEIGHT) {
      curr_board[pt.x][pt.y] = CELL_FOOD;
    }
  }

  // 3. AI SNAKES 
  for (uint8_t e = 0U; e < game->enemyCount; e++) {
    if (game->enemies[e].length > 0U) {
      for (uint16_t i = 1U; i < game->enemies[e].length; i++) {
        Point_t pt = game->enemies[e].body[i];
        if (pt.x >= 0 && pt.x < GRID_WIDTH && pt.y >= 0 && pt.y < GRID_HEIGHT) {
          curr_board[pt.x][pt.y] = (uint8_t)(CELL_ENEMY_BASE + e * 5U);
        }
      }
      Point_t head = game->enemies[e].body[0];
      if (head.x >= 0 && head.x < GRID_WIDTH && head.y >= 0 && head.y < GRID_HEIGHT) {
        Direction_t dir = game->enemies[e].dir;
        curr_board[head.x][head.y] = (uint8_t)(CELL_ENEMY_BASE + e * 5U + 1U + (uint8_t)dir);
      }
    }
  }

  // 4. SNAKE PLAYER
  if (game->snake.length > 0U) {
    for (uint16_t i = 1U; i < game->snake.length; i++) {
      Point_t pt = game->snake.body[i];
      if (pt.x >= 0 && pt.x < GRID_WIDTH && pt.y >= 0 && pt.y < GRID_HEIGHT) {
        curr_board[pt.x][pt.y] = CELL_PLAYER_BODY;
      }
    }
    Point_t head = game->snake.body[0];
    if (head.x >= 0 && head.x < GRID_WIDTH && head.y >= 0 && head.y < GRID_HEIGHT) {
      Direction_t dir = game->snake.dir;
      curr_board[head.x][head.y] = (uint8_t)(CELL_PLAYER_HEAD_UP + (uint8_t)dir);
    }
  }


  bool row_near_top_changed = false;
  bool row_near_bottom_changed = false;
  for (uint8_t y = 0; y < GRID_HEIGHT; y++) {
    for (uint8_t x = 0; x < GRID_WIDTH; x++) {
      if (curr_board[x][y] != prev_board[x][y]) {
        render_cell(x, y, curr_board[x][y]);
        prev_board[x][y] = curr_board[x][y];

        uint16_t hostY = (uint16_t)(GRID_HEIGHT - 1 - y);
        if (hostY <= 1U) {
          row_near_top_changed = true;
        } else if (hostY >= (GRID_HEIGHT - 2U)) {
          row_near_bottom_changed = true;
        }
      }
    }
  }

 
  if (game->score != last_rendered_score || game->stage != last_rendered_stage || row_near_top_changed) {
    draw_top_info_bar(game);
    last_rendered_score = game->score;
    last_rendered_stage = game->stage;
  }
  if (row_near_bottom_changed) {
    draw_bottom_help_bar();
  }

// 7. GAME STATE  HANDLER (PAUSE, GAMEOVER, VICTORY, LEVEL_TRANSITION)
  if (game->state != last_rendered_game_state) {
    if (last_rendered_game_state == GAME_STATE_PAUSED ||
        last_rendered_game_state == GAME_STATE_GAMEOVER ||
        last_rendered_game_state == GAME_STATE_VICTORY ||
        last_rendered_game_state == GAME_STATE_LEVEL_TRANSITION) {
      game_board_needs_full_redraw = true;
    }

    if (game->state == GAME_STATE_PAUSED) {
      draw_pause_overlay(console);
      last_pause_btn = console->pauseBtn;
    } else if (game->state == GAME_STATE_GAMEOVER) {
      draw_gameover_overlay(game, console);
      last_gameover_btn = console->gameOverBtn;
    } else if (game->state == GAME_STATE_VICTORY) {
      draw_victory_overlay(game, console);
      last_gameover_btn = console->gameOverBtn;
    } else if (game->state == GAME_STATE_LEVEL_TRANSITION) {
      draw_panel(70U, 85U, 180U, 60U);
      char transStr[24];
      snprintf(transStr, sizeof(transStr), "STAGE %u", (unsigned int)game->stage);
      draw_text_centered(105U, transStr, COLOR_LIME, 2U);
    }

    last_rendered_game_state = game->state;
  } else {
    
    if (game->state == GAME_STATE_PAUSED && console->pauseBtn != last_pause_btn) {
      draw_pause_overlay(console);
      last_pause_btn = console->pauseBtn;
    } else if (game->state == GAME_STATE_GAMEOVER && console->gameOverBtn != last_gameover_btn) {
      draw_gameover_overlay(game, console);
      last_gameover_btn = console->gameOverBtn;
    } else if (game->state == GAME_STATE_VICTORY && console->gameOverBtn != last_gameover_btn) {
      draw_victory_overlay(game, console);
      last_gameover_btn = console->gameOverBtn;
    }
  }
}

// DRAW VIRTUAL KEYBOARD

static void draw_keyboard_key(uint8_t r, uint8_t c, bool isSelected)
{
  static const char *labels[4][7] = {
    {"A", "B", "C", "D", "E", "F", "G"},
    {"H", "I", "J", "K", "L", "M", "N"},
    {"O", "P", "Q", "R", "S", "T", "U"},
    {"V", "W", "X", "Y", "Z", "DEL", "OK"}
  };

  const uint16_t startX = 26U;
  const uint16_t startY = 96U;
  const uint16_t keyW = 34U;
  const uint16_t keyH = 26U;
  const uint16_t gapX = 5U;
  const uint16_t gapY = 6U;

  if (r >= 4U || c >= 7U) return;

  uint16_t kx = startX + c * (keyW + gapX);
  uint16_t ky = startY + r * (keyH + gapY);

  uint16_t activeBg = COLOR_BLUE;
  uint16_t textCol = COLOR_WHITE;

  if (r == 3U && c == 5U) {
    activeBg = COLOR_RED;
    textCol = COLOR_YELLOW;
  } else if (r == 3U && c == 6U) {
    activeBg = COLOR_GREEN;
    textCol = COLOR_LIME;
  }

  draw_btn(kx, ky, keyW, keyH, labels[r][c], isSelected, activeBg, textCol);
}

static void draw_virtual_keyboard(const Console_t *console, uint8_t prevRow, uint8_t prevCol)
{
  if (prevRow == 0xFFU || prevCol == 0xFFU) {
    
    for (uint8_t r = 0U; r < 4U; r++) {
      for (uint8_t c = 0U; c < 7U; c++) {
        bool isSelected = (console->kbdRow == r && console->kbdCol == c);
        draw_keyboard_key(r, c, isSelected);
      }
    }
  } else {

    if (prevRow < 4U && prevCol < 7U) {
      draw_keyboard_key(prevRow, prevCol, false);
    }
    draw_keyboard_key(console->kbdRow, console->kbdCol, true);
  }
}

// DRAW LEADERBOARD SCREEN

static void draw_leaderboard_screen(const Console_t *console)
{
  draw_background(COLOR_HOST_BG, COLOR_HOST_GRID);
  draw_panel(15U, 10U, 290U, 220U);

  // Title
  fill_rect(20U, 14U, 280U, 24U, COLOR_BLUE);
  draw_text_centered(18U, "HALL OF FAME", COLOR_YELLOW, 2U);

  // Header TOP PLAYERS
  draw_text(25U, 45U, "RANK   PLAYER      SCORE", COLOR_LIME, 1U);
  fill_rect(25U, 57U, 270U, 1U, COLOR_PANEL_LIGHT);

  Snake_Leaderboard_Load();
  uint8_t count = Snake_Leaderboard_Count();

  for (uint8_t i = 0U; i < 7U; i++) {
    char entryLine[40];
    if (i < count) {
      const LeaderboardEntry_t *e = Snake_Leaderboard_Get(i);
      snprintf(entryLine, sizeof(entryLine), " #%u   %-12s %lu", 
               (unsigned int)(i + 1U), e->playerName, (unsigned long)e->score);
    } else {
      snprintf(entryLine, sizeof(entryLine), " #%u   ------------ 0", (unsigned int)(i + 1U));
    }
    uint16_t color = (i == 0U) ? COLOR_YELLOW : (i < 3U ? COLOR_CYAN : COLOR_WHITE);
    draw_text(25U, 63U + (i * 15U), entryLine, color, 1U);
  }

  // BOTTON
  bool restartSel = (console->leaderboardBtn == LEADERBOARD_BTN_RESTART);
  bool returnSel = (console->leaderboardBtn == LEADERBOARD_BTN_RETURN_HOME);

  draw_btn(30U, 182U, 125U, 32U, "RESTART", restartSel, COLOR_GREEN, COLOR_LIME);
  draw_btn(165U, 182U, 125U, 32U, "RETURN HOME", returnSel, COLOR_ORANGE, COLOR_WHITE);
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

static ConsoleState_t last_rendered_state = (ConsoleState_t)-1;
static uint32_t last_clock_sec = 0xFFFFFFFFUL;

void LCD_Sleep(void)
{
#if LCD_POWER_SAVE_DEMO_MODE
  // DEMO MODE: Pure black screen (prevents white display when backlight is permanently 3.3V)
  fill_rect(0, 0, TFT_WIDTH, TFT_HEIGHT, 0x0000U);
  HAL_Delay(5U);
#else
  // HARDWARE SLEEP MODE: Native ILI9341 commands for true power saving
  write_command(0x28); // Display OFF
  HAL_Delay(5U);
  write_command(0x10); // Enter Sleep Mode
  HAL_Delay(120U);
#endif
}

void LCD_Wakeup(void)
{
#if LCD_POWER_SAVE_DEMO_MODE
  // DEMO MODE: No controller wakeup commands required
  HAL_Delay(10U);
#else
  // HARDWARE SLEEP MODE: Restore ILI9341 controller
  write_command(0x11); // Exit Sleep Mode
  HAL_Delay(120U);
  write_command(0x29); // Display ON
  HAL_Delay(20U);
#endif
  // Reset window coordinates to standard full screen
  set_window(0U, 0U, TFT_WIDTH - 1U, TFT_HEIGHT - 1U);
  last_rendered_state = (ConsoleState_t)-1;
  last_clock_sec = 0xFFFFFFFFUL;
  game_board_needs_full_redraw = true;
}

void LCD_ShowError(const char *msg)
{
  // Reset window and clean screen to pure black
  set_window(0U, 0U, TFT_WIDTH - 1U, TFT_HEIGHT - 1U);
  fill_rect(0U, 0U, TFT_WIDTH, TFT_HEIGHT, 0x0000U);

  // Red emergency alert panel centered
  draw_panel(30U, 45U, 260U, 150U);
  fill_rect(34U, 49U, 252U, 32U, COLOR_RED);
  draw_text_centered(56U, "ERROR", COLOR_WHITE, 3U);

  // Subtitle / message
  fill_rect(40U, 95U, 240U, 1U, COLOR_PANEL_LIGHT);
  draw_text_centered(110U, (msg != NULL && msg[0] != '\0') ? msg : "SYSTEM ERROR DETECTED", COLOR_YELLOW, 1U);
}

void LCD_Render(const Game_t *game, const Console_t *console)
{
  if (game == NULL || console == NULL) return;

  static Level_t last_selected_level = (Level_t)-1;
  static uint8_t last_kbd_row = 0xFFU;
  static uint8_t last_kbd_col = 0xFFU;
  static LeaderboardButton_t last_lb_btn = (LeaderboardButton_t)-1;

  if (console->state != last_rendered_state)
  {
    switch (console->state)
    {
      case CONSOLE_STATE_OFF:
        draw_background(COLOR_HOST_BG, COLOR_HOST_GRID);
        draw_panel(20U, 40U, 280U, 160U);
        draw_text_centered(55U, "CONSOLE", COLOR_WHITE, 1U);
        draw_text_centered(75U, "CUBENIRO", COLOR_LIME, 2U);
        draw_text_centered(120U, "PRESS X TO START", COLOR_YELLOW, 1U);
        break;

      case CONSOLE_STATE_LOAD:

        draw_flash_image(EXTERNAL_FLASH_CUBENIRO_ADDRESS, 0U, 0U, TFT_WIDTH, TFT_HEIGHT);
       
        fill_rect(0U, 212U, TFT_WIDTH, 20U, 0x0000U);
        draw_text_centered(218U, "CUBENIRO ARCADE", COLOR_WHITE, 1U);
        break;

      case CONSOLE_STATE_START:

        draw_snake_background();
      
        draw_panel(45U, 60U, 230U, 120U);
        draw_text_centered(72U, "SNAKE", COLOR_LIME, 2U);

        draw_text_centered(108U, "NEW GAME", COLOR_WHITE, 2U);
        draw_text_centered(150U, "PRESS X TO START", COLOR_YELLOW, 1U);
        break;

      case CONSOLE_STATE_NAME:
        draw_snake_background();
        draw_panel(15U, 10U, 290U, 220U);
        draw_text_centered(18U, "ENTER PLAYER NAME", COLOR_WHITE, 1U);

        // BOX NAME
        fill_rect(35U, 38U, 250U, 42U, COLOR_DARK_GRAY);
        fill_rect(35U, 38U, 250U, 2U, COLOR_CYAN);
        fill_rect(35U, 78U, 250U, 2U, COLOR_CYAN);
        fill_rect(35U, 38U, 2U, 42U, COLOR_CYAN);
        fill_rect(283U, 38U, 2U, 42U, COLOR_CYAN);

        last_kbd_row = 0xFFU;
        last_kbd_col = 0xFFU;
        break;

      case CONSOLE_STATE_DIFFICULTY:
       // LEVEL SELECT SCREEN
        draw_snake_background();
        draw_panel(40U, 50U, 240U, 140U);
        draw_text_centered(70U, "DIFFICULTY", COLOR_WHITE, 1U);
        draw_text_centered(140U, "LEFT/RIGHT SELECT", COLOR_WHITE, 1U);
        break;

      case CONSOLE_STATE_GAME:
        game_board_needs_full_redraw = true;
        break;

      case CONSOLE_STATE_LEADERBOARD:
        draw_leaderboard_screen(console);
        last_lb_btn = console->leaderboardBtn;
        break;

      default:
        break;
    }

    last_rendered_state = console->state;
    last_selected_level = (Level_t)-1;
  }


  switch (console->state)
  {
    case CONSOLE_STATE_OFF:
    {
      static uint32_t last_sec = 0xFFFFFFFFUL;
      uint32_t total_sec = HAL_GetTick() / 1000U;

      if (total_sec != last_sec) {
        last_sec = total_sec;

        // TIME CONSOLE
        static uint8_t base_h = 16U, base_m = 48U, base_s = 0U;
        static uint8_t base_day = 5U, base_month = 10U;
        static uint16_t base_year = 2026U;
        static bool time_parsed = false;

        if (!time_parsed) {
          const char *build_time = __TIME__; // "HH:MM:SS"
          base_h = (uint8_t)((build_time[0] - '0') * 10 + (build_time[1] - '0'));
          base_m = (uint8_t)((build_time[3] - '0') * 10 + (build_time[4] - '0'));
          base_s = (uint8_t)((build_time[6] - '0') * 10 + (build_time[7] - '0'));

          const char *build_date = __DATE__; // "Mmm dd yyyy"
          base_day = (uint8_t)((build_date[4] == ' ' ? 0 : build_date[4] - '0') * 10 + (build_date[5] - '0'));
          base_year = (uint16_t)((build_date[7] - '0') * 1000 + (build_date[8] - '0') * 100 +
                                 (build_date[9] - '0') * 10 + (build_date[10] - '0'));
          const char *months[] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
          for (uint8_t m = 0; m < 12U; m++) {
            if (strncmp(build_date, months[m], 3) == 0) {
              base_month = m + 1U;
              break;
            }
          }
          time_parsed = true;
        }

        uint32_t current_sec_of_day = (uint32_t)base_h * 3600U + (uint32_t)base_m * 60U + base_s + total_sec;
        uint32_t days_elapsed = current_sec_of_day / 86400U;
        current_sec_of_day %= 86400U;

        uint8_t cur_h = (uint8_t)(current_sec_of_day / 3600U);
        uint8_t cur_m = (uint8_t)((current_sec_of_day / 60U) % 60U);
        uint8_t cur_s = (uint8_t)(current_sec_of_day % 60U);
        uint8_t cur_d = (uint8_t)(base_day + days_elapsed);

        char dateTimeStr[36];
        snprintf(dateTimeStr, sizeof(dateTimeStr), "%02u-%02u-%04u %02u:%02u:%02u - 35 C", 
                 cur_d, base_month, base_year, cur_h, cur_m, cur_s);
        
        fill_rect(25U, 145U, 270U, 12U, COLOR_PANEL);
        draw_text_centered(147U, dateTimeStr, COLOR_WHITE, 1U);
      }
      break;
    }
    case CONSOLE_STATE_NAME:
    {
      static char last_name[SNAKE_NAME_MAX_LENGTH + 1U] = {0};
      if (strcmp(game->playerName, last_name) != 0 || last_kbd_row == 0xFFU) {
        fill_rect(40U, 46U, 240U, 26U, COLOR_DARK_GRAY);
        char displayBuf[SNAKE_NAME_MAX_LENGTH + 3U];
        snprintf(displayBuf, sizeof(displayBuf), "%s_", game->playerName);
        draw_text_centered(50U, displayBuf, COLOR_LIME, 2U);
        strncpy(last_name, game->playerName, sizeof(last_name) - 1U);
      }

      if (console->kbdRow != last_kbd_row || console->kbdCol != last_kbd_col) {
        draw_virtual_keyboard(console, last_kbd_row, last_kbd_col);
        last_kbd_row = console->kbdRow;
        last_kbd_col = console->kbdCol;
      }
      break;
    }

    case CONSOLE_STATE_DIFFICULTY:
      if (console->selectedLevel != last_selected_level) {
        fill_rect(70U, 100U, 180U, 20U, COLOR_GREEN); 
        
        if (console->selectedLevel == LEVEL_EASY) 
          draw_text_centered(104U, "> EASY <", COLOR_LIME, 2U);
        else if (console->selectedLevel == LEVEL_MEDIUM) 
          draw_text_centered(104U, "> MEDIUM <", COLOR_YELLOW, 2U);
        else 
          draw_text_centered(104U, "> HARD <", 0xF800U, 2U);
          
        last_selected_level = console->selectedLevel;
      }
      break;

    case CONSOLE_STATE_LEADERBOARD:
      if (console->leaderboardBtn != last_lb_btn) {
        bool restartSel = (console->leaderboardBtn == LEADERBOARD_BTN_RESTART);
        bool returnSel = (console->leaderboardBtn == LEADERBOARD_BTN_RETURN_HOME);

        draw_btn(30U, 182U, 125U, 32U, "RESTART", restartSel, COLOR_GREEN, COLOR_LIME);
        draw_btn(165U, 182U, 125U, 32U, "RETURN HOME", returnSel, COLOR_ORANGE, COLOR_WHITE);
        last_lb_btn = console->leaderboardBtn;
      }
      break;

    case CONSOLE_STATE_GAME:
      draw_game(game, console);
      break;

    default:
      break;
  }
}