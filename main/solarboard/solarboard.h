#pragma once

#include <expected>

#include "esp_err.h"
#include "errors.h"
#include "esp_log.h"
#include "i2c/INA229.h"
#include "i2c/BMI323.h"
#include "i2c/BMP581.h"
#include "i2c/I2C.h"
#include "sd.h"
#include "utils.h"

namespace seds {
    using namespace seds::errors;

    class SolarBoard {
    private:
        static constexpr size_t buf_len = MOUNT_POINT_LEN + 1 + 3 + 4 + 4 + 1;
    public:
        
        BMP581 baro;
        BMI323 imu;
        SDCard sd;
        // mount point, slash, 3 numbers, 'data', '.csv', '\0'
        char filename[SolarBoard::buf_len] = MOUNT_POINT"/data.csv"; 

        Expected<std::monostate> init(void);
 
        void process(uint32_t times, bool endless);
    };
}