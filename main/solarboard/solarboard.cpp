#include "solarboard.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <sys/time.h>

static const char *TAG = "solarboard";

namespace seds {

Expected<std::monostate> SolarBoard::init() {
    char data[] = "timestamp, current1, current2, current3, current4, accel x, accel y, accel z, degrees x, degrees y, degrees z, baro temp, baro pressure\n";
    // test different filenames
    bool broke = false;
    struct stat st;
    for (int i = 0; i < 1000; i++) {
        // should write SD functions for this
        // TODO
        // also improve interface so we dont have to do what we do in process()
        snprintf(this->filename, SolarBoard::buf_len, "%s/data%d.csv", MOUNT_POINT, i);
        ESP_LOGI(TAG, "filename: %s, res: %d", this->filename, stat(this->filename, &st));
        if (stat(this->filename, &st) == -1) {
            // doesn't exist, we go with it
            broke = true;
            break;
        }
    } 
    
    if (!broke) {
        strcpy(this->filename, MOUNT_POINT"/data.csv");
    }
    ESP_LOGI(TAG, "filename: %s", this->filename);

    return this->sd.create_file(this->filename, (uint8_t *)data, sizeof(data)-1); // subtract one
}

constexpr size_t LOOPS_BEFORE_FLUSH = 100;
char buffer[LOOPS_BEFORE_FLUSH * (40 + 14 * 20 + 13 + 1 + 1)];

void SolarBoard::process(uint32_t times, bool endless) {
    FILE *data_file = fopen(this->filename, "a");

    struct timeval tv_now;
    memset(buffer, 'X', sizeof(buffer));
    size_t idx = 0;
    for (int i = 0; i < times || endless; i++) {
        gettimeofday(&tv_now, NULL);
        int64_t time_ms = (int64_t)tv_now.tv_sec * 1000L + (int64_t)tv_now.tv_usec / 1000L;

        IMUData imu_data = {.ax = 0, .ay = 0, .az = 0, .gx = 0, .gy = 0, .gz = 0};
        BarometerData baro_data = { .baro_temp = 0, .pressure = 0 };
        float tmp = 0.0;

        auto imu_data_try = this->imu.read_imu();
        if (imu_data_try.has_value()) {
            imu_data = imu_data_try.value();
        } else {
            ESP_LOGE(TAG, "imu data read failed");
        }

        auto baro_data_try = this->baro.read_data();
        if (baro_data_try.has_value()) {
            baro_data = baro_data_try.value();
        } else {
            ESP_LOGE(TAG, "baro data read failed");
        }
       
        idx += snprintf(&buffer[idx], 40 + 20 * 14 + 14, "%lld,%g,%g,%g,%g,%g,%g,%g,%g\n",
            time_ms,
            imu_data.ax, imu_data.ay, imu_data.az, imu_data.gx, imu_data.gy, imu_data.gz,
            baro_data.baro_temp, baro_data.pressure
        );
        
        // speed this up!s
        if ((i % LOOPS_BEFORE_FLUSH) == LOOPS_BEFORE_FLUSH - 1) {
            ESP_LOGI(TAG, "Flushing");
            errno = 0;
            //fwrite((void *)buffer, 1, idx, data_file);
            if (errno != 0) {
                ESP_LOGE(TAG, "err: %s", strerror(errno));
            }
            errno = 0;
            fflush(data_file);
            if (errno != 0) {
                ESP_LOGE(TAG, "err: %s", strerror(errno));
            }
            errno = 0;
            fsync(fileno(data_file));
            if (errno != 0) {
                ESP_LOGE(TAG, "err: %s", strerror(errno));
            }
            
            auto append_res = sd.append_file(SolarBoard::filename, (uint8_t *)buffer, idx);
            if (!append_res.has_value()) {
                ESP_LOGE(TAG, "solar board append error: %s", append_res.error()->what());
            }

            memset(buffer, 'X', sizeof(buffer));
            idx = 0;
        }

    }
    // fixme: with finite loops we could lose data at the end
    // this will happen with infinite loops too, but there its kind of unavoidable

    fclose(data_file);
}

}