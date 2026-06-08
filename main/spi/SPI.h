/* Almost all code copied from I2C.h/.c
 * refer to the I2C code written by someone who actually know what they're doing
 * (Lily L, May N) for help
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

static const spi_host_device_t DEFAULT_HOST = SPI2_HOST;

namespace seds {
    using namespace std::chrono_literals;
    using namespace seds::errors;

    class SPIDevice;

    /// An SPI bus
    class SPI : public std::enable_shared_from_this<SPI> {
        // Used to prevent construction except by create().
        struct Private {
            explicit Private() = default;
        };

    public:
        // Returns shared pointer to SPI object
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
            ESP_LOGI("spi", "Getting device with select pin %x", Device::select_pin);
            auto handle = TRY(this->get_device(Device::select_pin));
            ESP_LOGI("spi", "Got device");

            return Device(std::move(handle));
        }

    private:
        friend class SPIDevice;

        spi_host_device_t bus_handle = DEFAULT_HOST;
        std::set<gpio_num_t> used_select_pins;
    };




    /// An SPI Device
    class SPIDevice {
    public:
        static constexpr auto timeout = 1000ms;

        // Disallow accidentally making duplicates
        SPIDevice(SPIDevice const&) = delete;
        SPIDevice& operator=(SPIDevice const&) = delete;
        SPIDevice(SPIDevice&&) = default;
        SPIDevice& operator=(SPIDevice&&) = default;

        ~SPIDevice();


        /// Enum input into read/write function.
        /// 
        enum class Endianness { BIG, LITTLE };

        /// Generic write-read a byte buffer to the SPI bus.
        /// Bytes that were previously in the register are read back.
        /// Read-only and Write-only versions not necessary for INA229
        template<size_t WriteN, size_t ReadN>
        Expected<std::array<uint8_t, ReadN>> write_read(
            std::array<uint8_t, WriteN> const& write_buf
        ) {
            std::array<uint8_t, ReadN> read_buf;

            spi_transaction_t trans = {
                .length = WriteN * 8,  // # of bits, not bytes
                .tx_buffer = write_buf,
                .rx_buffer = read_buf
            };

            ESP_TRY(
                spi_device_transmit(this->handle(), &trans)
            );

            return read_buf;
        }


        /// Read a big-endian number from the given register of this SPI device.
        ///
        /// To make it easier to keep track of register constants, you can pass in a custom register
        /// enum variant as long as it can be statically cast to a `uint8_t`.
        template<typename ReadT, typename RegisterT>
        [[nodiscard]]
        Expected<ReadT> read_be_register(RegisterT const reg) {
            static_assert(std::is_arithmetic_v<ReadT>, "ReadT must be a number");

            auto write_buf = std::array { static_cast<uint8_t>(reg) };
            auto read_buf = TRY(this->write_read<sizeof(ReadT)>(write_buf));

            return num::from_be_bytes<ReadT>(read_buf);
        }

        /// Write a big-endian number to the given register of this SPI device.
        ///
        /// To make it easier to keep track of register constants, you can pass in a custom register
        /// enum variant as long as it can be statically cast to a `uint8_t`.
        template<typename WriteT, typename RegisterT>
        [[nodiscard]]
        Expected<std::monostate> write_be_register(RegisterT const reg, WriteT const new_value) {
            static_assert(std::is_arithmetic_v<WriteT>, "WriteT must be a number");

            std::array<uint8_t, 1 + sizeof(WriteT)> write_buf = {
                static_cast<uint8_t>(reg),
                // ...temporarily unfilled
            };

            // Write new value's bytes into the write buffer.
            std::ranges::copy(num::to_be_bytes(new_value), &write_buf[1]);

            // execute spi write-read, and discard all data read back
            this->write_read(write_buf);

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