#include "bme280.h"
#include "i2c.h"
#include <stdio.h>
#include <string.h>

#define BME280_REG_CHIP_ID       0xD0U
#define BME280_REG_RESET         0xE0U
#define BME280_REG_CTRL_HUM      0xF2U
#define BME280_REG_STATUS        0xF3U
#define BME280_REG_CTRL_MEAS     0xF4U
#define BME280_REG_CONFIG        0xF5U
#define BME280_REG_DATA          0xF7U

#define BMP280_CHIP_ID           0x58U
#define BME280_CHIP_ID           0x60U

#define BME280_I2C_ADDR_PRIM     (0x76U << 1)
#define BME280_I2C_ADDR_SEC      (0x77U << 1)


typedef struct {
  uint16_t dig_T1;
  int16_t  dig_T2;
  int16_t  dig_T3;

  uint16_t dig_P1;
  int16_t  dig_P2;
  int16_t  dig_P3;
  int16_t  dig_P4;
  int16_t  dig_P5;
  int16_t  dig_P6;
  uint16_t dig_P7;
  int16_t  dig_P8;
  int16_t  dig_P9;

  uint8_t  dig_H1;
  int16_t  dig_H2;
  uint8_t  dig_H3;
  int16_t  dig_H4;
  int16_t  dig_H5;
  int8_t   dig_H6;
} BME280_Calib_t;

static uint16_t active_i2c_addr = 0U;
static SensorType_t detected_type = SENSOR_TYPE_NONE;
static BME280_Calib_t calib;
static int32_t t_fine = 0;
static BME280_Data_t latest_data = {0};

static bool read_registers(uint8_t reg, uint8_t *data, uint16_t len) {
  if (active_i2c_addr == 0U) return false;
  return (HAL_I2C_Mem_Read(&hi2c1, active_i2c_addr, reg, I2C_MEMADD_SIZE_8BIT, data, len, 50U) == HAL_OK);
}

static bool write_register(uint8_t reg, uint8_t value) {
  if (active_i2c_addr == 0U) return false;
  return (HAL_I2C_Mem_Write(&hi2c1, active_i2c_addr, reg, I2C_MEMADD_SIZE_8BIT, &value, 1U, 50U) == HAL_OK);
}

static bool read_calibration_data(void) {
  uint8_t buf[26];

  // 1. Read Temp & Pressure calibration 
  if (!read_registers(0x88U, buf, 24U)) return false;
  calib.dig_T1 = (uint16_t)((buf[1] << 8) | buf[0]);
  calib.dig_T2 = (int16_t)((buf[3] << 8) | buf[2]);
  calib.dig_T3 = (int16_t)((buf[5] << 8) | buf[4]);

  calib.dig_P1 = (uint16_t)((buf[7] << 8) | buf[6]);
  calib.dig_P2 = (int16_t)((buf[9] << 8) | buf[8]);
  calib.dig_P3 = (int16_t)((buf[11] << 8) | buf[10]);
  calib.dig_P4 = (int16_t)((buf[13] << 8) | buf[12]);
  calib.dig_P5 = (int16_t)((buf[15] << 8) | buf[14]);
  calib.dig_P6 = (int16_t)((buf[17] << 8) | buf[16]);
  calib.dig_P7 = (uint16_t)((buf[19] << 8) | buf[18]);
  calib.dig_P8 = (int16_t)((buf[21] << 8) | buf[20]);
  calib.dig_P9 = (int16_t)((buf[23] << 8) | buf[22]);

  if (detected_type == SENSOR_TYPE_BME280) {

    if (!read_registers(0xA1U, &calib.dig_H1, 1U)) return false;
    uint8_t hbuf[7];
    if (!read_registers(0xE1U, hbuf, 7U)) return false;
    calib.dig_H2 = (int16_t)((hbuf[1] << 8) | hbuf[0]);
    calib.dig_H3 = hbuf[2];
    calib.dig_H4 = (int16_t)(((int8_t)hbuf[3] << 4) | (hbuf[4] & 0x0FU));
    calib.dig_H5 = (int16_t)(((int8_t)hbuf[5] << 4) | (hbuf[4] >> 4));
    calib.dig_H6 = (int8_t)hbuf[6];
  }

  return true;
}


static void i2c_bus_recovery(void) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_I2C_DISABLE(&hi2c1);


  GPIO_InitStruct.Pin = GPIO_PIN_6;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

 
  GPIO_InitStruct.Pin = GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  for (uint8_t i = 0; i < 9; i++) {
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET);
    for (volatile uint32_t d = 0; d < 100; d++);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);
    for (volatile uint32_t d = 0; d < 100; d++);

  GPIO_InitStruct.Pin = GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_RESET);
  for (volatile uint32_t d = 0; d < 100; d++);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);
  for (volatile uint32_t d = 0; d < 100; d++);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_SET);
  for (volatile uint32_t d = 0; d < 100; d++);

  HAL_I2C_DeInit(&hi2c1);
  MX_I2C1_Init();
}

bool BME280_Init(void) {
  uint16_t probe_addrs[2] = { BME280_I2C_ADDR_PRIM, BME280_I2C_ADDR_SEC };
  uint8_t chip_id = 0U;

  detected_type = SENSOR_TYPE_NONE;
  active_i2c_addr = 0U;

  i2c_bus_recovery();

  printf("[I2C] Ricerca sensore ambientale su I2C1 (PB6/PB7)...\r\n");

  for (int i = 0; i < 2; i++) {
    uint16_t test_addr = probe_addrs[i];
    
    chip_id = 0U;
    HAL_StatusTypeDef st = HAL_I2C_Mem_Read(&hi2c1, test_addr, BME280_REG_CHIP_ID, 
                                            I2C_MEMADD_SIZE_8BIT, &chip_id, 1U, 50U);
    if (st == HAL_OK) {
      if (chip_id == BMP280_CHIP_ID) {
        active_i2c_addr = test_addr;
        detected_type = SENSOR_TYPE_BMP280;
        break;
      } else if (chip_id == BME280_CHIP_ID) {
        active_i2c_addr = test_addr;
        detected_type = SENSOR_TYPE_BME280;
        break;
      } else {
        printf("[I2C] Trovato dispositivo a 0x%02X con ChipID sconosciuto: 0x%02X\r\n", 
               (test_addr >> 1), chip_id);
      }
    }
  }

  if (detected_type == SENSOR_TYPE_NONE) {
    active_i2c_addr = 0U;
    return false;
  }

  const char *type_name = (detected_type == SENSOR_TYPE_BME280) ? "BME280 (Temp/Press/Umidita')" : "BMP280 (Temp/Press)";
  printf("[I2C] [SUCCESSO] Rilevato sensore: %s all'indirizzo 0x%02X (ID=0x%02X)\r\n", 
         type_name, (active_i2c_addr >> 1), chip_id);


  write_register(BME280_REG_RESET, 0xB6U);
  HAL_Delay(30U); 

  if (!read_calibration_data()) {
    printf("[I2C] [ERRORE] Lettura coefficienti calibrazione fallita!\r\n");
    return false;
  }



  if (detected_type == SENSOR_TYPE_BME280) {
   
    write_register(BME280_REG_CTRL_HUM, 0x01U);
  }


  write_register(BME280_REG_CONFIG, 0x04U);


  write_register(BME280_REG_CTRL_MEAS, 0x24U);

  printf("[I2C] Sensore configurato in modalita' FORCED MODE (Zero Self-Heating)\r\n");
  return true;
}

static float compensate_temperature(int32_t adc_T) {
  int32_t var1 = ((((adc_T >> 3) - ((int32_t)calib.dig_T1 << 1))) * ((int32_t)calib.dig_T2)) >> 11;
  int32_t var2 = (((((adc_T >> 4) - ((int32_t)calib.dig_T1)) * ((adc_T >> 4) - ((int32_t)calib.dig_T1))) >> 12) *
                  ((int32_t)calib.dig_T3)) >> 14;
  t_fine = var1 + var2;
  float T = (float)((t_fine * 5 + 128) >> 8);
  return T / 100.0f;
}

static float compensate_pressure(int32_t adc_P) {
  int64_t var1 = ((int64_t)t_fine) - 128000;
  int64_t var2 = var1 * var1 * (int64_t)calib.dig_P6;
  var2 = var2 + ((var1 * (int64_t)calib.dig_P5) << 17);
  var2 = var2 + (((int64_t)calib.dig_P4) << 35);
  var1 = ((var1 * var1 * (int64_t)calib.dig_P3) >> 8) + ((var1 * (int64_t)calib.dig_P2) << 12);
  var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)calib.dig_P1) >> 33;

  if (var1 == 0) return 0.0f;

  int64_t p = 1048576 - adc_P;
  p = (((p << 31) - var2) * 3125) / var1;
  var1 = (((int64_t)calib.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
  var2 = (((int64_t)calib.dig_P8) * p) >> 19;
  p = ((p + var1 + var2) >> 8) + (((int64_t)calib.dig_P7) << 4);

  return (float)p / 25600.0f; 
}

static float compensate_humidity(int32_t adc_H) {
  int32_t v_x1_u32r = (t_fine - ((int32_t)76800));
  v_x1_u32r = (((((adc_H << 14) - (((int32_t)calib.dig_H4) << 20) - (((int32_t)calib.dig_H5) * v_x1_u32r)) +
                 ((int32_t)16384)) >> 15) *
               (((((((v_x1_u32r * ((int32_t)calib.dig_H6)) >> 10) *
                    (((v_x1_u32r * ((int32_t)calib.dig_H3)) >> 11) + ((int32_t)32768))) >> 10) +
                  ((int32_t)2097152)) * ((int32_t)calib.dig_H2) + 8192) >> 14));
  v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) * ((int32_t)calib.dig_H1)) >> 4));
  v_x1_u32r = (v_x1_u32r < 0 ? 0 : v_x1_u32r);
  v_x1_u32r = (v_x1_u32r > 419430400 ? 419430400 : v_x1_u32r);
  return (float)(v_x1_u32r >> 12) / 1024.0f;
}

bool BME280_Read(BME280_Data_t *data) {
  if (data == NULL || detected_type == SENSOR_TYPE_NONE) return false;

  
  write_register(BME280_REG_CTRL_MEAS, 0x25U);

  
  HAL_Delay(15U);

  uint8_t raw[8];
  uint16_t len = (detected_type == SENSOR_TYPE_BME280) ? 8U : 6U;

  if (!read_registers(BME280_REG_DATA, raw, len)) {
    data->valid = false;
    return false;
  }

  int32_t adc_P = (int32_t)((((uint32_t)raw[0]) << 12) | (((uint32_t)raw[1]) << 4) | (((uint32_t)raw[2]) >> 4));
  int32_t adc_T = (int32_t)((((uint32_t)raw[3]) << 12) | (((uint32_t)raw[4]) << 4) | (((uint32_t)raw[5]) >> 4));
  int32_t adc_H = 0;
  if (detected_type == SENSOR_TYPE_BME280) {
    adc_H = (int32_t)((((uint32_t)raw[6]) << 8) | ((uint32_t)raw[7]));
  }

  
  static uint8_t raw_print_cnt = 0U;
  if (++raw_print_cnt >= 2U) {
    printf("[I2C RAW] T_raw=0x%lX, P_raw=0x%lX, H_raw=0x%lX\r\n", 
           (unsigned long)adc_T, (unsigned long)adc_P, (unsigned long)adc_H);
    raw_print_cnt = 0U;
  }

  if (adc_T == 0x80000 || adc_T == 0) {
    printf("[I2C] [ATTESA] Conversione in corso (T_raw=0x%lX)\r\n", (unsigned long)adc_T);
    data->valid = false;
    return false;
  }

  data->temperature = compensate_temperature(adc_T) + BME280_TEMPERATURE_OFFSET;
  data->pressure    = compensate_pressure(adc_P);
  data->type        = detected_type;

  if (detected_type == SENSOR_TYPE_BME280) {
    data->humidity = compensate_humidity(adc_H);
  } else {
    data->humidity = 0.0f;
  }

  data->valid = true;
  latest_data = *data;
  return true;
}

void BME280_GetLatestData(BME280_Data_t *data) {
  if (data != NULL) {
    *data = latest_data;
  }
}

