/* Almost all code copied from I2C.h/.c
 * refer to the I2C code written by someone who actually know what they're doing
 * (May N and Lily L) for help
 * Helpful SPI docs: https://docs.espressif.com/projects/esp-idf/en/stable/esp32c3/api-reference/peripherals/spi_master.html
 */

#pragma once
#include <expected>
#include <memory>
#include <set>
#include <chrono>
#include <algorithm>
#include <string>
#include <bit>
#include <type_traits>

#include "driver/spi_common.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "errors.h"
#include "esp_log.h"
#include "utils.h"

// General SPI pin defines
#define MOSI_IO_NUM GPIO_NUM_7
#define MISO_IO_NUM GPIO_NUM_2
#define SCLK_IO_NUM GPIO_NUM_6

#define INA229Q1_SPI_MAX_FREQ 10000000 // max is 10 MHz
static const spi_host_device_t DEFAULT_HOST = SPI2_HOST;

static const char *TAG = "spi";

namespace seds {
    using namespace std::chrono_literals;
    using namespace seds::errors;

    class SPIDevice;

    /*********************
     *     SPI BUS       *
     *********************/
    class SPI : public std::enable_shared_from_this<SPI> {
        // Used to prevent construction except by create().
        // what the hell
        struct Private {
            explicit Private() = default;
        };

    public:
        // returns shared pointer to SPI object??
        static std::shared_ptr<SPI> create() {
            return std::make_shared<SPI>(Private());
        }

        explicit SPI(Private);
        ~SPI();

        // Returns a handle to an SPI device given its select pin
        [[nodiscard]]
        Expected<SPIDevice> get_device(gpio_num_t select_pin);

        /// Returns a handle to an SPI device using its select pin.
        template<typename Device>
        Expected<Device> get_device() {
            ESP_LOGI(TAG, "Getting device with select pin %x", Device::select_pin);
            auto handle = TRY(this->get_device(Device::select_pin));
            ESP_LOGI(TAG, "Got device");

            return Device(std::move(handle));
        }

    private:
        friend class SPIDevice;

        spi_host_device_t bus_handle = DEFAULT_HOST;
        std::set<gpio_num_t> used_select_pins;
    };




    /*********************
     *     SPI DEVICE    *
     *********************/
    class SPIDevice {
    public:
        static constexpr auto timeout = 1000ms;

        // Disallow accidentally making duplicates
        SPIDevice(SPIDevice const&) = delete;
        SPIDevice& operator=(SPIDevice const&) = delete;
        SPIDevice(SPIDevice&&) = default;
        SPIDevice& operator=(SPIDevice&&) = default;

        ~SPIDevice();


        /// Perform a write-read transaction on the SPI bus. The given byte buffer is written
        /// to the device and then `ReadN` bytes are read back.
        /// !!! All readable registers are 16 bits wide !!!
        template<size_t ReadN, size_t WriteN>
        Expected<std::array<uint16_t, ReadN>> write_read(
            std::array<uint16_t, WriteN> const& write_buf
        ) {
            std::array<uint16_t, ReadN> read_buf;

            spi_transaction_t spi_t = {
                
            }

            ESP_TRY(
                spi_device_transmit();
            );

            return read_buf;
        }

        /// Write a byte buffer to the SPI bus. No bytes are read back.
        template<size_t WriteN>
        [[nodiscard]]
        Expected<std::monostate> write(
            std::array<uint8_t, WriteN> const& write_buf
        ) {
            ESP_TRY(
                i2c_master_transmit(
                    this->handle(),
                    write_buf.data(),
                    write_buf.size(),
                    // I'm not totally sure why we need to divide by the tick period here, but it
                    // was in the I2C example.
                    timeout.count() / portTICK_PERIOD_MS
                )
            );

            return std::monostate {};
        }


        [[nodiscard]]
        std::shared_ptr<SPI> get_bus() const {
            return this->bus;
        }

        [[nodiscard]]
        spi_device_handle_t handle() const {
            return this->dev_handle;
        }

    private:
        // This constructor is called from SPI::get_device, which has some extra checks.
        friend class SPI;
        SPIDevice(std::shared_ptr<SPI> bus, gpio_num_t select_pin);
        
        std::shared_ptr<SPI> bus;
        gpio_num_t select_pin;
        spi_device_handle_t dev_handle {nullptr};
    };
}