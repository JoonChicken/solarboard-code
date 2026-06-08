// For the INA229-Q1 Current sensor
// Datasheet can be found here: https://www.ti.com/lit/ds/symlink/ina229-q1.pdf
// This devices communicates through SPI in big endian

#pragma once
#include "SPI.h"


namespace seds {
    using namespace seds::errors;

    struct CurrentSensData {
        float vshunt;
        float vbus;
        float current;
        float power;
    };

    class INA229Q1 {
    public:
        enum class ADCRange : uint16_t {
            _WIDE = 0x0,        // Supports +/- 163.84 mV
            _NARROW = 0x1       // Supports +/- 40.96 mV
        };      

        static Expected<INA229Q1> create(SPIDevice&& device);

        INA229Q1(INA229Q1&&) = default;
        INA229Q1& operator=(INA229Q1&&) = default;
        INA229Q1(INA229Q1 const&) = delete;
        INA229Q1& operator=(INA229Q1 const&) = delete;

        bool is_connected();

        [[nodiscard]]
        Expected<std::monostate> set_adc_range(ADCRange range);

        [[nodiscard]]
        Expected<std::monostate> set_shunt_val(float ohms);

        [[nodiscard]]
        Expected<std::monostate> set_max_current(float amps);

        [[nodiscard]]
        Expected<CurrentSensData> read_INA229Q1();

    private:
        explicit INA229Q1(SPIDevice&& device);

        SPIDevice device;
        ADCRange adc_range;
        float shunt_resistor_val;
        float max_expected_current;
        float current_lsb;
    };
}