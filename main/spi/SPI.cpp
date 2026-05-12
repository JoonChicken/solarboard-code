#include "SPI.h"

#include <memory>
#include <utility>
#include <expected>

namespace seds {
    SPI::SPI(Private) {
        // SPI bus setup
        constexpr auto spi_bus_config = spi_bus_config_t {
            .mosi_io_num = MOSI_IO_NUM,
            .miso_io_num = MISO_IO_NUM,
            .sclk_io_num = SCLK_IO_NUM,
            .quadwp_io_num = -1,
            .quadhd_io_num = -1,
            .max_transfer_sz = 0
        };
        // SPI 0 & 1 unusable, only 2 available  
        ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &spi_bus_config, SPI_DMA_CH_AUTO));
    }

    SPI::~SPI() {
        ESP_ERROR_CHECK(spi_bus_free(DEFAULT_HOST));
    }

    Expected<SPIDevice> SPI::get_device(gpio_num_t select_pin) {
        auto [_, did_insert] = this->used_select_pins.insert(select_pin);

        // Don't allow duplicate devices to prevent two subsystems writing to the same place
        // and causing interference.
        if (!did_insert) {
            return std::unexpected(
                std::make_unique<std::runtime_error>("Select pin already in use")
            );
        }

        ESP_LOGI(TAG, "Making SPIDevice");

        return SPIDevice(this->shared_from_this(), select_pin);
    }


    SPIDevice::SPIDevice(std::shared_ptr<SPI> bus, gpio_num_t select_pin)
        : bus(std::move(bus)),
          select_pin(select_pin) {
        spi_device_interface_config_t spi_dev_config = {
            .address_bits = 6,                  // reg address
            .mode = 0b00,                       // CPOL, CPHA
            .clock_speed_hz = INA229Q1_SPI_MAX_FREQ, // 10MHz
            .spics_io_num = (int) select_pin   // cs pin #
        };

        ESP_ERROR_CHECK(
            spi_bus_add_device(DEFAULT_HOST, &spi_dev_config, &this->dev_handle)
        );
    }

    SPIDevice::~SPIDevice() {
        // If this was moved, `bus` will be null.
        // (In that case, there's no need to free anything.)
        if (this->bus) {
            ESP_ERROR_CHECK(spi_bus_remove_device(this->handle()));
            this->bus->used_select_pins.erase(this->select_pin);
        }
    }
}