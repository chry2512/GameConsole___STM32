#ifndef BME280_H
#define BME280_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
  SENSOR_TYPE_NONE = 0,
  SENSOR_TYPE_BMP280,
  SENSOR_TYPE_BME280
} SensorType_t;

// Offset di calibrazione termica 
#define BME280_TEMPERATURE_OFFSET  (-2.0f)

typedef struct {
  float temperature;   // Gradi Celsius
  float pressure;      // Pressione mBar
  float humidity;      // Umidita' relativa %
  SensorType_t type;  
  bool valid;          
} BME280_Data_t;


bool BME280_Init(void);


bool BME280_Read(BME280_Data_t *data);


void BME280_GetLatestData(BME280_Data_t *data);

#endif /* BME280_H */

