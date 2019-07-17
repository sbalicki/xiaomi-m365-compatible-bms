#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdint.h>

namespace m365bms
{

struct BMSSettings
{
    uint8_t header[2] = {0xB0, 0x0B};
    uint16_t version = 1;
    char serial[14] = "BOTOX001";
    uint32_t capacity = 7800; // mAh
    uint16_t nominal_voltage = 3600; // mV
    uint16_t full_voltage = 4150; // mV
    uint16_t num_cycles = 0;
    uint16_t num_charged = 0;
    uint16_t date = (18 << 9) | (10 << 5) | 1; // MSB (7 bits year, 4 bits month, 5 bits day) LSB

    // setShuntResistorValue
    uint16_t shuntResistor_uOhm = 1000;

    // setThermistorBetaValue
    uint16_t thermistor_BetaK = 3435;

    // setTemperatureLimits
    int16_t temp_minDischargeC = -20; // °C
    int16_t temp_maxDischargeC = 60; // °C
    int16_t temp_minChargeC = 0; // °C
    int16_t temp_maxChargeC = 45; // °C

    // setShortCircuitProtection
    uint32_t SCD_current = 80000; // mA
    uint16_t SCD_delay = 200; // us

    // setOvercurrentChargeProtection
    uint32_t OCD_current = 6000; // mA
    uint16_t OCD_delay = 3000; // ms

    // setOvercurrentDischargeProtection
    uint32_t ODP_current = 35000; // mA
    uint16_t ODP_delay = 1280; // ms

    // setCellUndervoltageProtection
    uint16_t UVP_voltage = 2800; // mV
    uint16_t UVP_delay = 2; // s

    // setCellOvervoltageProtection
    uint16_t OVP_voltage = 4200; // mV
    uint16_t OVP_delay = 2; // s

    // setBalancingThresholds
    uint16_t balance_minIdleTime = 1800; // s
    uint16_t balance_minVoltage = 3600; // mV
    uint16_t balance_maxVoltageDiff = 10; // mV

    // setIdleCurrentThreshold
    uint16_t idle_currentThres = 500; // mA

    // enableAutoBalancing
    uint16_t balance_enabled = 1;

    // adjADCPackOffset
    int16_t adcPackOffset = 0;

    // adjADCCellsOffset
    int16_t adcCellsOffset[15] = {0};
} __attribute__((packed));

class Settings
{
public:
    void read();
    void write();

public:    
    BMSSettings params;
};

} // namespace m365bms

#endif // SETTINGS_H
