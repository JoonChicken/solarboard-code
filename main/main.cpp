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
#include <sys/time.h>


// indicator LED pins
#define BAROSTAT_GPIO GPIO_NUM_9
#define CURRSTAT_GPIO GPIO_NUM_21

// current sensor select pins
#define CS_1 GPIO_NUM_4
#define CS_2 GPIO_NUM_20
#define CS_3 GPIO_NUM_1
#define CS_4 GPIO_NUM_0

// sd card cs pin
#define SDSEL GPIO_NUM_10


// SD Data stuff
struct SolarBoardData {
    int32_t time_ms;
    seds::BarometerData baro_data;
    seds::IMUData imu_data;
    seds::INAData ina1_data;
    seds::INAData ina2_data;
    seds::INAData ina3_data;
    seds::INAData ina4_data;
};
size_t SBDATA_SIZE = sizeof(SolarBoardData);

// BEWARE OF THE 8.3 RULE OF FAT32
// mount point, slash, 'sund', 3 numbers, '.raw'
static constexpr size_t buf_len = MOUNT_POINT_LEN + 1 + 4 + 3 + 4 + 1;
char mainfilename[buf_len] = MOUNT_POINT"/sund.raw";


static const char *TAG = "main";
using namespace seds::errors;

// same NOAA pressure-altitude calculation the rrc3 does
// returns altitude in meters
float pressure_to_altitude(float pressure) {
    float pres_mb = pressure / 100;
    float h_alt = 145366.45 * (1 - powf(pres_mb / 1013.25, 0.190284));
    return h_alt * 0.3048;
}


FILE *get_next_available_file(int starting_num, char *header) {
    // test different filenames
    char newfilename[buf_len];
    bool broke = false;
    struct stat st;
    for (uint8_t i = starting_num; i < 1000; i++) {
        // should write SD functions for this
        // TODO
        // also improve interface so we dont have to do what we do in process()
        snprintf(newfilename, buf_len, "%s/sund%d.raw", MOUNT_POINT, i);
        if (stat(newfilename, &st) == -1) {
            // doesn't exist, we go with it
            broke = true;
            break;
        }
    } 
    if (!broke) {
        snprintf(newfilename, buf_len, "%s/sund%d.raw", MOUNT_POINT, starting_num);
    }
    ESP_LOGI("main", "opening file: %s", newfilename);
    vTaskDelay(pdMS_TO_TICKS(10));
    FILE *f = fopen(newfilename, "ab");
    while (f == NULL) {
        ESP_LOGE("main", "Failed to open file for writing");
        f = fopen(newfilename, "ab");
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    fprintf(f, header);
    return f;
}



extern "C" void app_main(void)
{
    // reset pins that UART might have changed
    gpio_reset_pin(CS_1);
    gpio_reset_pin(CS_2);
    gpio_reset_pin(CS_3);
    gpio_reset_pin(CS_4);
    gpio_reset_pin(BAROSTAT_GPIO);
    gpio_reset_pin(CURRSTAT_GPIO);
    gpio_reset_pin(SDSEL);


    // initialize I2C bus and devices    
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


    // initialize SPI bus and devices
    auto spi = seds::SPI::create();
    ESP_LOGI(TAG, "SPI initialized successfully");


    // also initialize SD card here, on the existing SPI bus
    // more info here: https://docs.espressif.com/projects/esp-idf/en/stable/esp32c3/api-reference/peripherals/sdspi_share.html
    // first, make sure all CS pins are set to idle state:
    gpio_set_level(CS_1, 1);
    gpio_set_level(CS_2, 1);
    gpio_set_level(CS_3, 1);
    gpio_set_level(CS_4, 1);
    gpio_set_level(SDSEL, 1);

    // then initialize the SD card
    seds::SDCard sd = unwrap(seds::SDCard::create_with_existing_spi_bus());


    // setup INA229Q1s
    seds::INA229Q1 ina1 = unwrap(seds::INA229Q1::create(unwrap(spi->get_device(GPIO_NUM_4))));
    vTaskDelay(pdMS_TO_TICKS(10));
    seds::INA229Q1 ina2 = unwrap(seds::INA229Q1::create(unwrap(spi->get_device(GPIO_NUM_20))));
    vTaskDelay(pdMS_TO_TICKS(10));
    seds::INA229Q1 ina3 = unwrap(seds::INA229Q1::create(unwrap(spi->get_device(GPIO_NUM_1))));
    vTaskDelay(pdMS_TO_TICKS(10));
    seds::INA229Q1 ina4 = unwrap(seds::INA229Q1::create(unwrap(spi->get_device(GPIO_NUM_0))));
    vTaskDelay(pdMS_TO_TICKS(10));

    if (ina1.is_connected()) ESP_LOGI(TAG, "ina1 connected!");
    else ESP_LOGE(TAG, "ina1 not connected!");
    if (ina2.is_connected()) ESP_LOGI(TAG, "ina2 connected!");
    else ESP_LOGE(TAG, "ina2 not connected!");
    if (ina3.is_connected()) ESP_LOGI(TAG, "ina3 connected!");
    else ESP_LOGE(TAG, "ina3 not connected!");
    if (ina4.is_connected()) ESP_LOGI(TAG, "ina4 connected!");
    else ESP_LOGE(TAG, "ina4 not connected!");

    ina1.set_shunt_val(0.191);
    ina1.set_max_current(0.21);
    ina2.set_shunt_val(0.191);
    ina2.set_max_current(0.21);
    ina3.set_shunt_val(0.191);
    ina3.set_max_current(0.21);
    ina4.set_shunt_val(0.191);
    ina4.set_max_current(0.21);




    /*#region initialize sd output*/


    char categories[] = "timestamp, current1, current2, current3, current4, accel x, accel y, accel z, degrees x, degrees y, degrees z, baro temp, baro pressure\n";
    size_t HEADER_SIZE = SBDATA_SIZE + 1 + sizeof(categories);
    // metadata number + newline char + sizeof header
    char header[2 + 1 + sizeof(categories)];
    snprintf(header, HEADER_SIZE, "%u\n%s", SBDATA_SIZE, categories);
    int current_file_number = 0;

    printf("%s\n", header);

    FILE *f = get_next_available_file(current_file_number, header);

    struct timeval tv_now;
    gettimeofday(&tv_now, NULL);
    int64_t last_flush_timestamp = (int64_t)tv_now.tv_sec * 1000L + (int64_t)tv_now.tv_usec / 1000L;
    int64_t last_newfile_timestamp = (int64_t)tv_now.tv_sec * 1000L + (int64_t)tv_now.tv_usec / 1000L;


    /*#endregion*/

    // blink lights to indicate init complete, then turn off to save power
    gpio_set_direction(BAROSTAT_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(CURRSTAT_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(BAROSTAT_GPIO, 1);
    gpio_set_level(CURRSTAT_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(500));
    gpio_set_level(BAROSTAT_GPIO, 0);
    gpio_set_level(CURRSTAT_GPIO, 0);
    vTaskDelay(pdMS_TO_TICKS(500));
    gpio_set_level(BAROSTAT_GPIO, 1);
    gpio_set_level(CURRSTAT_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(500));
    gpio_set_level(BAROSTAT_GPIO, 0);
    gpio_set_level(CURRSTAT_GPIO, 0);
    vTaskDelay(pdMS_TO_TICKS(500));
    gpio_set_level(BAROSTAT_GPIO, 1);
    gpio_set_level(CURRSTAT_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(500));
    gpio_set_level(BAROSTAT_GPIO, 0);
    gpio_set_level(CURRSTAT_GPIO, 0);



    //Allow other core to finish initialization
    // vTaskDelay(pdMS_TO_TICKS(100)); // esp32-c3 has only one core but ok

    char FAKE_BUF[1000];

    // int loopcount = 0;

    // TO-DO: make sure the current sensors are really reading once per ms
    while (true) {
        // read time
        gettimeofday(&tv_now, NULL);
        int64_t time_ms = (int64_t)tv_now.tv_sec * 1000L + (int64_t)tv_now.tv_usec / 1000L;


        // we can't tell the other core to flush it once the buffer is full
        // since there's only one core, so we just push to sdcard every read,
        // flush all data lingering in memory to the sdcard every 5 seconds,
        // and start a new file every 30 min to stave off corruption
        if (time_ms > 5000 + last_flush_timestamp) {
            fflush(f); 
            fsync(fileno(f));
            last_flush_timestamp = time_ms;
            ESP_LOGI("main", "flushed to sdcard");
            // printf("did %d write cycles in 5000 ms\n", loopcount);
            // loopcount = 0;
        }
        if (time_ms > 1.8e6 + last_newfile_timestamp) { // 30 min
            fflush(f); 
            fsync(fileno(f));
            fclose(f);
            current_file_number++;
            f = get_next_available_file(current_file_number, header);
            last_newfile_timestamp = time_ms;
            ESP_LOGI("main", "starting new file");
        }
        // loopcount++;


        // setup data stucts
        seds::BarometerData baro_data = { .baro_temp = 0, .pressure = 0 };
        seds::IMUData imu_data = { .ax = 0, .ay = 0, .az = 0, .gx = 0, .gy = 0, .gz = 0 };
        seds::INAData ina1_data = { .current_raw = 0, .current = 0 };
        seds::INAData ina2_data = { .current_raw = 0, .current = 0 };
        seds::INAData ina3_data = { .current_raw = 0, .current = 0 };
        seds::INAData ina4_data = { .current_raw = 0, .current = 0 };

        // read all sensors
        // first, wait for all the current sensors to be ready (conversion complete)
        while(!ina1.is_ready_for_read()) {}
        while(!ina2.is_ready_for_read()) {}
        while(!ina3.is_ready_for_read()) {}
        while(!ina4.is_ready_for_read()) {}
        auto ina1_data_try = ina1.read_INA229Q1();
        auto ina2_data_try = ina2.read_INA229Q1();
        auto ina3_data_try = ina3.read_INA229Q1();
        auto ina4_data_try = ina4.read_INA229Q1();
        auto baro_data_try = barometer->read_data();
        auto imu_data_try = imu.read_imu();

        // get current sensor data first
        if (ina1_data_try.has_value()) {
            ina1_data = ina1_data_try.value();
            // printf("INA1 Current: %.10f\n", ina1_data_try.value().current);
        } else {
            ESP_LOGE(TAG, "INA1 data read failed");
        }
        if (ina2_data_try.has_value()) {
            ina2_data = ina2_data_try.value();
            // printf("INA2 Current: %.10f\n", ina2_data_try.value().current);
        } else {
            ESP_LOGE(TAG, "INA2 data read failed");
        }
        if (ina3_data_try.has_value()) {
            ina3_data = ina3_data_try.value();
            // printf("INA3 Current: %.10f\n", ina3_data_try.value().current);
        } else {
            ESP_LOGE(TAG, "INA3 data read failed");
        }
        if (ina4_data_try.has_value()) {
            ina4_data = ina4_data_try.value();
            // printf("INA4 Current: %.10f\n", ina4_data_try.value().current);
        } else {
            ESP_LOGE(TAG, "INA4 data read failed");
        }

        // then the other sensors
        // float altitude = 0;
        if (baro_data_try.has_value()) {
            baro_data = baro_data_try.value();
            // altitude = pressure_to_altitude(baro_data.pressure);
            // printf("BARO TEMP: %.5f  BARO PRESSURE: %.5f  Converted altitude: %.5f\n", baro_data.baro_temp, baro_data.pressure, altitude);
        } else {
            ESP_LOGE(TAG, "baro data read failed");
        }

        if (imu_data_try.has_value()) {
            imu_data = imu_data_try.value();
            // printf("Ax: %.5f  Ay: %.5f  Az: %.5f\nGx: %.5f  Gy: %.5f  Gz: %.5f\n",
                //    imu_data.ax, imu_data.ay, imu_data.az, imu_data.gx, imu_data.gy, imu_data.gz);
        } else {
            ESP_LOGE(TAG, "imu data read failed");
        }
        

        // output all data to sd card
        SolarBoardData sbdata = {
            .time_ms = (int32_t) time_ms,
            .baro_data = baro_data,
            .imu_data = imu_data,
            .ina1_data = ina1_data,
            .ina2_data = ina2_data,
            .ina3_data = ina3_data,
            .ina4_data = ina4_data
        };
        
        // Write data in raw bits
        fwrite(&sbdata, SBDATA_SIZE, 1, f);
        

        // feed the dawg
        vTaskDelay(1);
    }

    ESP_LOGE("main", "If we get here, uh oh");
    fclose(f);
}



