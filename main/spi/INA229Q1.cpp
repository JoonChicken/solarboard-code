#include "INA229Q1.h"


/***
 * SPI frame structure is as follows:
 *  - Reads: MOSI: 6 addr bits, 1 zero, 1 READ |
 *           MISO:                             | 16 to 40 big endian bits
 * 
 *  - Writes: MOSI: 6 addr bits, 1 zero, 1 WRITE | 16 be bits, new data to write
 *            MISO:                              | 16 be bits, old data from reg
 */

namespace seds {
    /// List of all register addresses; refer to pg 20 of datasheet
    /// shifted left 2 bits - INA229Q1
    enum class INA229Q1Register : uint8_t {
        CONFIG          = 0x0 << 2,
        ADC_CONFIG      = 0x1 << 2,
        SHUNT_CAL       = 0x2 << 2,
        SHUNT_TEMPCO    = 0x3 << 2,
        VSHUNT          = 0x4 << 2,
        VBUS            = 0x5 << 2,
        DIETEMP         = 0x6 << 2,
        CURRENT         = 0x7 << 2,
        POWER           = 0x8 << 2,
        ENERGY          = 0x9 << 2,
        CHARGE          = 0xA << 2,
        DIAG_ALRT       = 0xB << 2,
        SOVL            = 0xC << 2,
        SUVL            = 0xD << 2,
        BOVL            = 0xE << 2,
        BUVL            = 0xF << 2,
        TEMP_LIMIT      = 0x10 << 2,
        PWR_LIMIT       = 0x11 << 2,
        MANUFACTURER_ID = 0x3E << 2,
        DEVICE_ID       = 0x3F << 2
    };

    // List of some maybe useful register masks
    enum class INA229Q1Mask : uint32_t {
        // CONFIG register                                 ╶┐
        RST_msk        = (1 << 15),  // 15th bit            │
        TEMPCOMP_msk   = (1 << 5),   // 5th bit             │
        ADCRANGE_msk   = (1 << 4),   // 4th bit             │
        // ADC_CONFIG register                              │
        MODE_msk       = 0b1111000000000000,  // [15:12]    │
        VBUSCT_msk     = 0b0000111000000000,  // [11:9]     │  config
        VSHCT_msk      = 0b0000000111000000,  // [8:6]      │  registers
        VTCT_msk       = 0b0000000000111000,  // [5:3]      │
        AVG_msk        = 0b0000000000000111,  // [2:0]      │
        // SHUNT_CAL register                               │
        SHUNT_CAL_msk  = 0b0111111111111111,  // [14:0]     │
        // SHUNT_TEMPCO register                            │
        TEMPCO_msk     = 0b0011111111111111,  // [13:0]     │
        //                                                 ╶┘

        // VSHUNT register                                 ╶┐
        VSHUNT_msk     = 0xFFFFFFF0,  // [23:4]             │
        // VBUS register                                    │
        VBUS_msk       = 0xFFFFFFF0,  // [23:4]             │  data
        // CURRENT register                                 │  registers
        CURRENT_msk    = 0xFFFFFFF0,  // [23:4]             │
        //                                                 ╶┘

        // DIAG_ALRT register                              ╶┐
        ALATCH_msk     = (1 << 15),  // 15th bit            │
        CNVR_msk       = (1 << 14),  // 14th bit            │
        SLOWALERT_msk  = (1 << 13),  // 13th bit            │
        APOL_msk       = (1 << 12),  // 12th bit            │
        ENERGYOF_msk   = (1 << 11),  // 11th bit            │
        CHARGEOF_msk   = (1 << 10),  // 10th bit            │
        MATHOF_msk     = (1 << 9),   // 9th bit             │
        TMPOL_msk      = (1 << 7),   // 7th bit             │
        SHNTOL_msk     = (1 << 6),   // 6th bit             │  diagnostic
        SHNTUL_msk     = (1 << 5),   // 5th bit             │  registers
        BUSOL_msk      = (1 << 4),   // 4th bit             │
        BUSUL_msk      = (1 << 3),   // 3rd bit             │
        POL_msk        = (1 << 2),   // 2nd bit             │
        CNVRF_msk      = (1 << 1),   // 1st bit             │
        MEMSTA_msk     = (1 << 0),   // 0th bit             │
        // MANUFACTURER_ID register                         │
        MANFID_msk     = 0xFFFF,  // all of them            │
        // DEVICE_ID register                               │
        DIEID_msk      = 0xFFF0,  // [15:4]                 │
        REV_ID_msk     = 0x000F  // [3:0]                   │
        //                                                 ╶┘
    };

    INA229Q1::INA229Q1(SPIDevice&& device) : device(std::move(device)) {}

    Expected<INA229Q1> INA229Q1::create(SPIDevice&& device) {
        auto ina = INA229Q1(std::move(device));

        // force soft reset
        ina.reset();

        // Set configuration after write in case of failure
        TRY(ina.set_adc_range(ADCRange::_NARROW));
        TRY(ina.set_temp_comp(TempComp::_ON));
        TRY(ina.set_conv_time(ConvTime::_4120us));  // already set to this value on reset
        // TRY(ina.set_avg_count(SampleAvgCount::_1); // already set to this value on reset
        // TRY(ina.set_mode(Mode::_CONT_T_SV_BV); // already set to this value on reset
        ina.shunt_resistor_val = 1.0;
        ina.max_expected_current = 1.0;
        ina.current_lsb = 1.0;

        // pause
        vTaskDelay(pdMS_TO_TICKS(10));
        
        ESP_LOGI("INA229Q1", "INA229Q1 created. Make sure to configure this sensor\n\
                  especially the shunt resistance and max current values as they are\n\
                  initially set to invalid values.");

        return ina;
    }

    /// Checks if the Device ID register can be read from the INA229Q1
    bool INA229Q1::is_connected() {
        constexpr uint16_t dev_id_contents = 0x2291;

        auto result = this->device.read_be_register<uint16_t>(INA229Q1Register::DEVICE_ID);
        uint16_t result_val = 0;
        if (result.has_value()) {
            result_val = result.value();
            ESP_LOGE("INA229Q1", "device id: 0x%X", (uint16_t)(result_val));
        } else {
            ESP_LOGE("INA229Q1", "device id read failed");
        }

        if (!result.has_value() || (uint16_t)(result_val) != dev_id_contents) {
            ESP_LOGE("INA229Q1", "Chip ID read failed or incorrect");
            return false;
        }

        return true;
    }


    Expected<std::monostate> INA229Q1::reset() {
        // read current register contents
        uint16_t regval = TRY(
            this->device.read_be_register<uint16_t>(
                INA229Q1Register::CONFIG
            )
        );

        // set reset bit to 1 (auto clears when reset)
        regval |= static_cast<uint16_t>(INA229Q1Mask::RST_msk);

        // write new reg
        TRY(
            this->device.write_be_register<uint16_t>(
                INA229Q1Register::CONFIG,
                regval
            )
        );

        // wait just to be safe; don't know if I actually need this
        vTaskDelay(pdMS_TO_TICKS(20));

        return std::monostate {};
    }


    Expected<std::monostate> INA229Q1::set_adc_range(ADCRange range) {
        this->adc_range = range;

        // read current register contents and clear bits
        uint16_t regval = TRY(
            this->device.read_be_register<uint16_t>(
                INA229Q1Register::CONFIG
            )
        ) & ~static_cast<uint16_t>(INA229Q1Mask::ADCRANGE_msk);

        // put in new bit
        regval |= static_cast<uint16_t>(range) << 4;

        // write new reg
        TRY(
            this->device.write_be_register<uint16_t>(
                INA229Q1Register::CONFIG,
                regval
            )
        );

        return std::monostate {};
    }


    Expected<std::monostate> INA229Q1::set_temp_comp(TempComp temp_comp) {
        // read current register contents and clear bits
        uint16_t regval = TRY(
            this->device.read_be_register<uint16_t>(
                INA229Q1Register::CONFIG
            )
        ) & ~static_cast<uint16_t>(INA229Q1Mask::TEMPCOMP_msk);

        // put in new bit
        regval |= static_cast<uint16_t>(temp_comp) << 5;
        
        // write new reg
        TRY(
            this->device.write_be_register<uint16_t>(
                INA229Q1Register::CONFIG,
                regval
            )
        );

        return std::monostate {};
    }


    Expected<std::monostate> INA229Q1::set_mode(Mode mode) {
        // read current register contents and clear bits
        uint16_t regval = TRY(
            this->device.read_be_register<uint16_t>(
                INA229Q1Register::ADC_CONFIG
            )
        ) & ~static_cast<uint16_t>(INA229Q1Mask::MODE_msk);

        // put in new bit
        regval |= static_cast<uint16_t>(mode) << 12;

        TRY(
            this->device.write_be_register<uint16_t>(
                INA229Q1Register::ADC_CONFIG,
                regval
            )
        );

        return std::monostate {};
    }

    /// Sets conversion times for all three of the following:
    /// - VBUS
    /// - VSHUNT
    /// - TEMP (for temp compensation)
    Expected<std::monostate> INA229Q1::set_conv_time(ConvTime conv_time) {
        // read register contents and clear space
        uint16_t regval = TRY(
            this->device.read_be_register<uint16_t>(
                INA229Q1Register::ADC_CONFIG
            )
        ) & ~(static_cast<uint16_t>(INA229Q1Mask::VBUSCT_msk) |
              static_cast<uint16_t>(INA229Q1Mask::VSHCT_msk) |
              static_cast<uint16_t>(INA229Q1Mask::VTCT_msk));

        // enter new values
        regval |= (static_cast<uint16_t>(conv_time) << 9 |
                   static_cast<uint16_t>(conv_time) << 6 |
                   static_cast<uint16_t>(conv_time) << 3);

        // write new reg
        TRY(
            this->device.write_be_register<uint16_t>(
                INA229Q1Register::ADC_CONFIG,
                regval
            )
        );

        return std::monostate {};
    }


    Expected<std::monostate> INA229Q1::set_avg_count(SampleAvgCount avg_count) {
        // read current value in reg & clear space
        uint16_t regval = TRY(
            this->device.read_be_register<uint16_t>(
                INA229Q1Register::ADC_CONFIG
            )
        ) & ~static_cast<uint16_t>(INA229Q1Mask::AVG_msk);

        // put in new val
        regval |= (static_cast<uint16_t>(avg_count) & static_cast<uint16_t>(INA229Q1Mask::AVG_msk));

        // write new reg contents
        TRY(
            this->device.write_be_register<uint16_t>(
                INA229Q1Register::ADC_CONFIG,
                regval
            )
        );

        return std::monostate {};
    }


    Expected<std::monostate> INA229Q1::set_shunt_val(float ohms) {
        this->shunt_resistor_val = ohms;
        int range_multiplier = this->adc_range == ADCRange::_NARROW ? 4 : 1;

        uint16_t shuntcal = this->shunt_multiplier * this->current_lsb *
                            this->shunt_resistor_val * range_multiplier;

        // check if number is too high to fit in register
        if (shuntcal & ~static_cast<uint16_t>(INA229Q1Mask::SHUNT_CAL_msk)) {
            ESP_LOGE("INA229Q1", "Shunt val set, but value made SHUNT_CAL invalid");
            shuntcal &= static_cast<uint16_t>(INA229Q1Mask::SHUNT_CAL_msk);
        } else {
            ESP_LOGI("INA229Q1", "Shunt_val successfully set. SHUNT_CAL is valid.");
        }

        // set shunt calibration value based on this number
        TRY(
            this->device.write_be_register<uint16_t>(
                INA229Q1Register::SHUNT_CAL,
                static_cast<uint16_t>(shuntcal & static_cast<uint16_t>(INA229Q1Mask::SHUNT_CAL_msk))
            )
        );

        return std::monostate {};
    }


    Expected<std::monostate> INA229Q1::set_max_current(float amps) {
        this->max_expected_current = amps;
        this->current_lsb = amps / this->current_divider;
        int range_multiplier = this->adc_range == ADCRange::_NARROW ? 4 : 1;

        uint16_t shuntcal = this->shunt_multiplier * this->current_lsb *
                            this->shunt_resistor_val * range_multiplier;

        // check if number is too high to fit in register
        if (shuntcal & ~static_cast<uint16_t>(INA229Q1Mask::SHUNT_CAL_msk)) {
            ESP_LOGE("INA229Q1", "Max current set, but value made SHUNT_CAL invalid");
            shuntcal &= static_cast<uint16_t>(INA229Q1Mask::SHUNT_CAL_msk);
        } else {
            ESP_LOGI("INA229Q1", "Max_current successfully set. SHUNT_CAL is valid.");
        }
        
        // set shunt calibration value based on this number
        TRY(
            this->device.write_be_register<uint16_t>(
                INA229Q1Register::SHUNT_CAL,
                static_cast<uint16_t>(shuntcal)
            )
        );

        return std::monostate {};
    }


    Expected<bool> INA229Q1::is_ready_for_read() {
        uint16_t reg = TRY(
            this->device.read_be_register<uint16_t>(
                INA229Q1Register::DIAG_ALRT
            )
        );

        return reg & static_cast<uint16_t>(INA229Q1Mask::CNVRF_msk);
    }


    Expected<INAData> INA229Q1::read_INA229Q1() {
        seds::INAData imu_data = {.current_raw = 0, .current = 0};

        int32_t current_raw = TRY(
            this->device.read_be_register<uint32_t>(
                INA229Q1Register::CURRENT
            )
        );

        imu_data.current_raw = current_raw >> 4;
        imu_data.current = (current_raw >> 4) * this->current_lsb;

        return imu_data;
    }

}