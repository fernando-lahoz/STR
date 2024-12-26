/*
 * Módulo para acceder al sensor de humedad y temperatura HTS221
 * de la extensión X-NUCLEO-IKS01A3
 */

#ifndef HTS221_H
#define HTS221_H

#include "stm32wlxx_hal.h"

enum HTS221_Temp_Avg {
    HTS221_TEMP_AVG_2 = 0,
    HTS221_TEMP_AVG_4,
    HTS221_TEMP_AVG_8,
    HTS221_TEMP_AVG_16,
    HTS221_TEMP_AVG_32,
    HTS221_TEMP_AVG_64,
    HTS221_TEMP_AVG_128,
    HTS221_TEMP_AVG_256
};

enum HTS221_Hum_Avg {
    HTS221_HUM_AVG_4 = 0,
    HTS221_HUM_AVG_8,
    HTS221_HUM_AVG_16,
    HTS221_HUM_AVG_32,
    HTS221_HUM_AVG_64,
    HTS221_HUM_AVG_128,
    HTS221_HUM_AVG_256,
    HTS221_HUM_AVG_512
};

enum HTS221_ODR {
    HTS221_ODR_ONESHOT = 0,
    HTS221_ODR_1HZ,
    HTS221_ODR_7HZ,
    HTS221_ODR_12_5HZ
};

int HTS221_check_dev(void);

void HTS221_configure(enum HTS221_Temp_Avg avg_samples_temp,
        enum HTS221_Hum_Avg avg_samples_hum, enum HTS221_ODR output_data_rate);

void HTS221_start(void);

void HTS221_sample(void);

int HTS221_is_ready(void);

void HTS221_get_data(int16_t *temperature_x8, int16_t *humidity_x2);

#endif
