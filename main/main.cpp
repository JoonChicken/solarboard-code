#include <cstdio>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "solarboard/solarboard.h"
#include "driver/i2c_master.h"
#include "i2c/BMI323.h"
#include "i2c/BMP581.h"
#include "i2c/I2C.h"
#include "errors.h"
#include "sd.h"


// current sensor select pins
#define CS_1 GPIO_NUM_4
#define CS_2 GPIO_NUM_20
#define CS_3 GPIO_NUM_1
#define CS_4 GPIO_NUM_0


static const char *TAG = "main";

using namespace seds::errors;

extern "C" void app_main(void)
{
    auto i2c = seds::I2C::create();
    ESP_LOGI(TAG, "I2C initialized successfully");

    std::shared_ptr<seds::BMP581> barometer = std::make_shared<seds::BMP581();


    auto spi = seds::SPI::create();
    ESP_LOGI(TAG, "SPI initialized successfully");
}
