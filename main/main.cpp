#include <cstdio>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "solarboard/solarboard.h"
#include "driver/i2c_master.h"
#include "i2c/BMI323.h"
#include "i2c/BMP581.h"
#include "i2c/high_g_accel.h"
#include "i2c/I2C.h"
#include "i2c/MLX90395.h"
#include "i2c/segment7.h"
#include "i2c/TMP1075.h"
#include "errors.h"
#include "sd.h"
static const char *TAG = "main";

#define I2C_PORT_AUTOSELECT -1
#define I2C_SCL 3
#define I2C_SDA 8

extern "C" app_main(void)
{
    ESP_LOGI(TAG, "Hello World!");
}
