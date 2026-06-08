#include "SPI.h"

#include <memory>
#include <utility>
#include <expected>

namespace seds {
    SPI::SPI(Private) {
        // SPI bus setup
        spi_bus_config_t spi_bus_config = spi_bus_config_t {};
        spi_bus_config.mosi_io_num = MOSI_IO_NUM;
        spi_bus_config.miso_io_num = MISO_IO_NUM;
        spi_bus_config.sclk_io_num = SCLK_IO_NUM;

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

        ESP_LOGI("spi", "Making SPIDevice");

        return SPIDevice(this->shared_from_this(), select_pin);
    }


    SPIDevice::SPIDevice(std::shared_ptr<SPI> bus, gpio_num_t select_pin)
        : bus(std::move(bus)),
          select_pin(select_pin) {
        spi_device_interface_config_t spi_dev_config = {};
        spi_dev_config.address_bits = 6;                     // reg address
        spi_dev_config.mode = 0b01;                          // CPOL, CPHA !! INA229-specific !!
        spi_dev_config.clock_speed_hz = SPI_MASTER_FREQ_10M; // 10MHz max for INA229Q1
        spi_dev_config.spics_io_num = select_pin;            // cs pin #
        spi_dev_config.queue_size = 8;                        // idk how much to give it

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