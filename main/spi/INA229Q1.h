// For the INA229-Q1 Current sensor
// Datasheet can be found here: https://www.ti.com/lit/ds/symlink/ina229-q1.pdf
// This devices communicates through SPI in big endian

#pragma once

#include "driver/spi_master.h"

////////////////////////////////////
// List of all register addresses //
//   refer to pg 20 of datasheet  //
////////////////////////////////////
#define CONFIG           0x0 
#define ADC_CONFIG       0x1 
#define SHUNT_CAL        0x2 
#define SHUNT_TEMPCO     0x3 
#define VSHUNT           0x4 
#define VBUS             0x5 
#define DIETEMP          0x6
#define CURRENT          0x7 
#define POWER            0x8
#define ENERGY           0x9
#define CHARGE           0xA
#define DIAG_ALRT        0xB
#define SOVL             0xC 
#define SUVL             0xD
#define BOVL             0xE
#define BUVL             0xF
#define TEMP_LIMIT      0x10
#define PWR_LIMIT       0x11
#define MANUFACTURER_ID 0x3E
#define DEVICE_ID       0x3F

////////////////////////////////////////
// List of some useful register masks //
////////////////////////////////////////
// CONFIG register                                         ╶┐
#define RST_msk         (1 << 15)  // 15th bit              │
#define TEMPCOMP_msk    (1 << 5)   // 5th bit               │
#define ADCRANGE_msk    (1 << 4)   // 4th bit               │
// ADC_CONFIG register                                      │
#define MODE_msk        0b1111_0000_0000_0000  // [15:12]   │
#define VBUSCT_msk      0b0000_1110_0000_0000  // [11:9]    │  config
#define VSHCT_msk       0b0000_0001_1100_0000  // [8:6]     │  registers
#define VTCT_msk        0b0000_0000_0011_1000  // [5:3]     │
#define AVG_msk         0b0000_0000_0000_0111  // [2:0]     │
// SHUNT_CAL register                                       │
#define SHUNT_msk       0b0111_1111_1111_1111  // [14:0]    │
// SHUNT_TEMPCO register                                    │
#define TEMPCO_msk      0b0011_1111_1111_1111  // [13:0]    │
//                                                         ╶┘ 
// VSHUNT register                                         ╶┐
#define VSHUNT_msk      0xFFFF_FFF0  // [23:4]              │
// VBUS register                                            │
#define VBUS_msk        0xFFFF_FFF0  // [23:4]              │  data
// CURRENT register                                         │  registers
#define CURRENT_msk     0xFFFF_FFF0  // [23:4]              │
//                                                         ╶┘
// DIAG_ALRT register                                      ╶┐
#define ALATCH_msk      (1 << 15)  // 15th bit              │
#define CNVR_msk        (1 << 14)  // 14th bit              │
#define SLOWALERT_msk   (1 << 13)  // 13th bit              │
#define APOL_msk        (1 << 12)  // 12th bit              │
#define ENERGYOF_msk    (1 << 11)  // 11th bit              │
#define CHARGEOF_msk    (1 << 10)  // 10th bit              │
#define MATHOF_msk      (1 << 9)   // 9th bit               │
#define TMPOL_msk       (1 << 7)   // 7th bit               │
#define SHNTOL_msk      (1 << 6)   // 6th bit               │  diagnostic
#define SHNTUL_msk      (1 << 5)   // 5th bit               │  registers
#define BUSOL_msk       (1 << 4)   // 4th bit               │
#define BUSUL_msk       (1 << 3)   // 3rd bit               │
#define POL_msk         (1 << 2)   // 2nd bit               │
#define CNVRF_msk       (1 << 1)   // 1st bit               │
#define MEMSTA_msk      (1 << 0)   // 0th bit               │
// MANUFACTURER_ID register                                 │
#define MANFID_msk      0xFFFF  // all of them              │
// DEVICE_ID register                                       │
#define DIEID_msk       0xFFF0  // [15:4]                   │
#define REV_ID_msk      0x000F  // [3:0]                    │
//                                                         ╶┘


namespace seds {
    struct CurrentSensData {
        float vshunt;
        float vbus;
        float current;
    };

    class INA229Q1 {
    public:

    private:
        
    };
}