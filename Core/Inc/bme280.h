#ifndef BME280_H
#define BME280_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
  SENSOR_TYPE_NONE = 0,
  SENSOR_TYPE_BMP280,
  SENSOR_TYPE_BME280
} SensorType_t;

// Offset di calibrazione termica per compensare il calore locale della console/schermo
#define BME280_TEMPERATURE_OFFSET  (-8.0f)

typedef struct {
  float temperature;   // Gradi Celsius (°C)
  float pressure;      // Pressione atmosferica (hPa / mbar)
  float humidity;      // Umidita' relativa (% RH)
  SensorType_t type;   // SENSOR_TYPE_BMP280 o SENSOR_TYPE_BME280
  bool valid;          // true se l'ultima lettura e' andata a buon fine
} BME280_Data_t;

/**
 * @brief Inizializza il sensore su bus I2C1 (auto-detect indirizzo 0x76 o 0x77 e tipo BMP/BME280)
 * @return true se il sensore e' stato riconosciuto e inizializzato
 */
bool BME280_Init(void);

/**
 * @brief Legge i dati di temperatura, pressione e umidita' dal sensore
 * @param data Puntatore alla struttura dove salvare i dati letti
 * @return true se la lettura ha avuto successo
 */
bool BME280_Read(BME280_Data_t *data);

/**
 * @brief Restituisce gli ultimi dati letti in modo thread-safe
 */
void BME280_GetLatestData(BME280_Data_t *data);

#endif /* BME280_H */

