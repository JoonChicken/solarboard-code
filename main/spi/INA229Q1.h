// For the INA229-Q1 Current sensor
// Datasheet can be found here: https://www.ti.com/lit/ds/symlink/ina229-q1.pdf
// This devices communicates through SPI in big endian

#pragma once
#include "SPI.h"
#include <stdint.h>
#include <expected>


namespace seds {
    using namespace seds::errors;

    struct INAData {
        int32_t current_raw;
        float current;
    };

    class INA229Q1 {
    public:
        /// INA229Q1 supports two ranges of voltage across the shunt resistor:
        enum class ADCRange : uint16_t {
            _WIDE = 0x0,        // Supports +/- 163.84 mV
            _NARROW = 0x1       // Supports +/- 40.96 mV
        };

        /// Whether to factor in internal temperature data in ADC conversion.
        /// Enable to use this data to mitigate the effect temperature has on
        /// the shunt resistor's resistance
        enum class TempComp {
            _OFF = 0x0,
            _ON = 0x1
        };

        /// Various measuring modes can be selected. Shutdown option exists also.
        enum class Mode {
            _SHUTDOWN = 0x0,   // Shutdown

            _SINGLE_BV      = 0x1,   // Triggered bus voltage, single shot
            _SINGLE_SV      = 0x2,   // Triggered shunt voltage, single shot
            _SINGLE_SV_BV   = 0x3,   // Triggered shunt voltage and bus voltage, single shot
            _SINGLE_T       = 0x4,   // Triggered temperature, single shot
            _SINGLE_T_VB    = 0x5,   // Triggered temperature and bus voltage, single shot
            _SINGLE_T_SV    = 0x6,   // Triggered temperature and shunt voltage, single shot
            _SINGLE_T_SV_BV = 0x7,   // Triggered bus voltage, shunt voltage and temperature, single shot

            _CONT_BV      = 0x9,   // Continuous bus voltage only
            _CONT_SV      = 0xA,   // Continuous shunt voltage only
            _CONT_SV_BV   = 0xB,   // Continuous shunt and bus voltage
            _CONT_T       = 0xC,   // Continuous temperature only
            _CONT_T_BV    = 0xD,   // Continuous bus voltage and temperature
            _CONT_T_SV    = 0xE,   // Continuous temperature and shunt voltage
            _CONT_T_SV_BV = 0xF    // Continuous bus voltage, shunt voltage and temperature
        };

        /// Basically time it takes for one sensor read to occur.
        /// MIGHT BE WRONG!!!!!
        enum class ConvTime {
            _50us   = 0x0,  // 50 us
            _84us   = 0x1,  // 84 us
            _150us  = 0x2,  // 150 us
            _280us  = 0x3,  // 280 us
            _540us  = 0x4,  // 540 us
            _1052us = 0x5,  // 1052 us
            _2074us = 0x6,  // 2074 us
            _4120us = 0x7   // 4120 us
        };

        /// How many samples to do a non-moving (!!) average over. To find total
        /// time for each output to be written to output register, multiply
        /// configured conversion time with the sample average count. E.g., 
        /// 1050us conv time with 4x averaging would require around 4 ms/sample.
        /// Not very useful for high-frequency measurements as is done for solarboard
        enum class SampleAvgCount {
            _1    = 0x0,  // 1
            _4    = 0x1,  // 4
            _16   = 0x2,  // 16
            _64   = 0x3,  // 64
            _128  = 0x4,  // 128
            _256  = 0x5,  // 256
            _512  = 0x6,  // 512
            _1024 = 0x7   // 1024
        };

        static Expected<INA229Q1> create(SPIDevice&& device);

        INA229Q1(INA229Q1&&) = default;
        INA229Q1& operator=(INA229Q1&&) = default;
        INA229Q1(INA229Q1 const&) = delete;
        INA229Q1& operator=(INA229Q1 const&) = delete;

        bool is_connected();

        [[nodiscard]]
        Expected<std::monostate> reset();

        [[nodiscard]]
        Expected<std::monostate> set_adc_range(ADCRange range);

        [[nodiscard]]
        Expected<std::monostate> set_temp_comp(TempComp temp_comp);

        [[nodiscard]]
        Expected<std::monostate> set_mode(Mode mode);

        [[nodiscard]]
        Expected<std::monostate> set_conv_time(ConvTime conv_time);

        [[nodiscard]]
        Expected<std::monostate> set_avg_count(SampleAvgCount avg_count);

        [[nodiscard]]
        Expected<std::monostate> set_shunt_val(float ohms);

        [[nodiscard]]
        Expected<std::monostate> set_max_current(float amps);

        [[nodiscard]]
        Expected<bool> is_ready_for_read();

        [[nodiscard]]
        Expected<INAData> read_INA229Q1();

    private:
        explicit INA229Q1(SPIDevice&& device);

        SPIDevice device;

        const float shunt_multiplier = 13107.2e6;
        const int current_divider = 524288;

        ADCRange adc_range;
        float shunt_resistor_val;
        float max_expected_current;
        float current_lsb;
    };
}