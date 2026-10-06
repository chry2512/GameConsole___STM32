#ifndef TOUCH_H
#define TOUCH_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Inizializza il controller Touch resistivo (XPT2046) su bus SPI1
 */
void Touch_Init(void);

/**
 * @brief Verifica se lo schermo e' attualmente premuto
 * @return true se premuto, false altrimenti
 */
bool Touch_IsPressed(void);

/**
 * @brief Legge le coordinate touch filtrate (0..320, 0..240)
 * @param x Puntatore a coordinata X display
 * @param y Puntatore a coordinata Y display
 * @return true se la lettura e' valida, false altrimenti
 */
bool Touch_GetCoordinates(uint16_t *x, uint16_t *y);

/**
 * @brief Rileva se e' stato toccato il centro dello schermo (ideale per conferma/Start)
 * @return true se tocco centrale rilevato
 */
bool Touch_IsCenterTapped(void);

#endif /* TOUCH_H */

