/***
 * Discussion:
 *  - IMU gyro will not be calibrated; it's really not worth it since it's
 *    data we don't need at all. It seems that the accelerometer comes
 *    calibrated, which is all we need
 */

#include <cstdio>
#include <math.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "i2c/BMI323.h"
#include "i2c/BMP581.h"
#include "i2c/I2C.h"
#include "spi/INA229Q1.h"
#include "spi/SPI.h"
#include "driver/i2c_master.h"
#include "errors.h"
#include "sd.h"


// current sensor select pins
#define CS_1 GPIO_NUM_4
#define CS_2 GPIO_NUM_20
#define CS_3 GPIO_NUM_1
#define CS_4 GPIO_NUM_0


static const char *TAG = "main";
using namespace seds::errors;

// same NOAA pressure-altitude calculation the rrc3 does
// returns altitude in meters
float pressure_to_altitude(float pressure) {
    float pres_mb = pressure / 100;
    float h_alt = 145366.45 * (1 - powf(pres_mb / 1013.25, 0.190284));
    return h_alt * 0.3048;
}


extern "C" void app_main(void)
{
    auto i2c = seds::I2C::create();
    ESP_LOGI(TAG, "I2C initialized successfully");

    std::shared_ptr<seds::Barometer> barometer = std::make_shared<seds::Barometer>();
    seds::Expected<seds::BMP581> baro_sensor_expected = seds::BMP581::create( unwrap(i2c->get_device(seds::BMP581::address_1)));
    vTaskDelay(pdMS_TO_TICKS(10)); // delay for creation of sensor
    if (baro_sensor_expected.has_value()) {
        barometer = std::make_shared<seds::BMP581>(unwrap(move(baro_sensor_expected)));
    }

    seds::BMI323 imu = unwrap(seds::BMI323::create( unwrap(i2c->get_device(seds::BMI323::default_address)) ));
    vTaskDelay(pdMS_TO_TICKS(10)); // delay for creation of sensor
    if (imu.is_connected()) {
        ESP_LOGI(TAG, "imu connected!");
    } else {
        ESP_LOGE(TAG, "imu not connected!");
    }


    auto spi = seds::SPI::create();
    ESP_LOGI(TAG, "SPI initialized successfully");



    seds::SDCard sd = unwrap(seds::SDCard::create());    
    //Allow other core to finish initialization
    vTaskDelay(pdMS_TO_TICKS(100)); // esp32-c3 has only one core but ok

    while (true) {
        seds::BarometerData baro_data = { .baro_temp = 0, .pressure = 0 };
        seds::IMUData imu_data = { .ax = 0, .ay = 0, .az = 0, .gx = 0, .gy = 0, .gz = 0 };

        auto baro_data_try = barometer->read_data();
        auto imu_data_try = imu.read_imu();

        if (baro_data_try.has_value()) {
            baro_data = baro_data_try.value();
            float altitude = pressure_to_altitude(baro_data.pressure);
            ESP_LOGI("main", "Testing Barometer:");
            printf("BARO TEMP: %.5f  BARO PRESSURE: %.5f  Converted altitude: %.5f\n", baro_data.baro_temp, baro_data.pressure, altitude);
        } else {
            ESP_LOGE(TAG, "baro data read failed");
        }

        if (imu_data_try.has_value()) {
            imu_data = imu_data_try.value();
            ESP_LOGI("main", "Testing IMU:");
            printf("Ax: %.5f  Ay: %.5f  Az: %.5f\n  Gx: %.5f  Gy: %.5f  Gz: %.5f",
                   imu_data.ax, imu_data.ay, imu_data.az, imu_data.gx, imu_data.gy, imu_data.gz);
        } else {
            ESP_LOGE(TAG, "imu data read failed");
        }




        vTaskDelay(pdMS_TO_TICKS(100)); // TO-DO: remove and time cycle based
                                        // on query to current sensors
    }
}
