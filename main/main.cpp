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
static const char *TAG = "main";

#define I2C_PORT_AUTOSELECT -1
#define I2C_SCL 3
#define I2C_SDA 8

// Flash pin defines
#define F_HD_IO_NUM 12
#define F_WP_IO_NUM 13
#define F_CS_IO_NUM 14
#define F_CLK_IO_NUM 15
#define F_MOSI_IO_NUM 16
#define F_MISO_IO_NUM 17

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "Hello World!");

}
