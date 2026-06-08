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
    // List of all register addresses
    // refer to pg 20 of datasheet
    enum class INA229Q1Register : uint8_t {
        CONFIG          = 0x0,
        ADC_CONFIG      = 0x1,
        SHUNT_CAL       = 0x2,
        SHUNT_TEMPCO    = 0x3,
        VSHUNT          = 0x4,
        VBUS            = 0x5,
        DIETEMP         = 0x6,
        CURRENT         = 0x7,
        POWER           = 0x8,
        ENERGY          = 0x9,
        CHARGE          = 0xA,
        DIAG_ALRT       = 0xB,
        SOVL            = 0xC,
        SUVL            = 0xD,
        BOVL            = 0xE,
        BUVL            = 0xF,
        TEMP_LIMIT      = 0x10,
        PWR_LIMIT       = 0x11,
        MANUFACTURER_ID = 0x3E,
        DEVICE_ID       = 0x3F
    };

    // List of some maybe useful register masks
    enum class INA229Q1Masks : uint32_t {
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
        SHUNT_msk      = 0b0111111111111111,  // [14:0]     │
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

        // Set configuration after write in case of failure
        ina.adc_range = ADCRange::_NARROW;
        ina.shunt_resistor_val = 0.0;
        ina.max_expected_current = 0.0;
        ina.current_lsb = 0.0;

        // force soft reset
        // TRY(ina.device.write_be_register<uint16_t>(INA229Q1Register::CONFIG, INA229Q1Masks::RST_msk));
        // pause
        vTaskDelay(pdMS_TO_TICKS(10));

        // uint16_t acc_cfg = TRY(ina.device.read_be_register<uint32_t>(INA229Q1Register::ACC_CFG)) >> 16;
        // ESP_LOGI("INA229Q1", "acc cfg: %x", acc_cfg);

        // TRY(ina.write_accel_config(ina.accel_range, ina.sensor_hz));
        // TRY(ina.write_gyro_config(ina.gyro_range, ina.sensor_hz));
        
        ESP_LOGI("INA229Q1", "ina created");

        return ina;
    }
}