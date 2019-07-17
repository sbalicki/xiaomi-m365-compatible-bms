#include "bms.h"

#include "config.h"
#include "settings.h"

namespace m365bms
{

BMS::BMS()
: bq769x0(bq76940, 0x08, true)
{
}

void BMS::apply_settings(const Settings& settings)
{
    setBatteryCapacity(settings.params.capacity, settings.params.nominal_voltage, settings.params.full_voltage);

    setShuntResistorValue(settings.params.shuntResistor_uOhm);
    setThermistorBetaValue(settings.params.thermistor_BetaK);

    setTemperatureLimits(settings.params.temp_minDischargeC,
                             settings.params.temp_maxDischargeC,
                             settings.params.temp_minChargeC,
                             settings.params.temp_maxChargeC);
    setShortCircuitProtection(settings.params.SCD_current, settings.params.SCD_delay);
    setOvercurrentChargeProtection(settings.params.OCD_current, settings.params.OCD_delay);
    setOvercurrentDischargeProtection(settings.params.ODP_current, settings.params.ODP_delay);
    setCellUndervoltageProtection(settings.params.UVP_voltage, settings.params.UVP_delay);
    setCellOvervoltageProtection(settings.params.OVP_voltage, settings.params.OVP_delay);

    setBalancingThresholds(settings.params.balance_minIdleTime,
                               settings.params.balance_minVoltage,
                               settings.params.balance_maxVoltageDiff);
    setIdleCurrentThreshold(settings.params.idle_currentThres);
    if(settings.params.balance_enabled)
        enableAutoBalancing();
    else
        disableAutoBalancing();

    setBalanceCharging(true);

    adjADCPackOffset(settings.params.adcPackOffset);
    adjADCCellsOffset(settings.params.adcCellsOffset);

    strncpy(params.serial, settings.params.serial, sizeof(params.serial));
    params.design_capacity = params.real_capacity = settings.params.capacity;
    params.nominal_voltage = settings.params.nominal_voltage;
    params.date = settings.params.date;
    params.num_cycles = settings.params.num_cycles;
    params.num_charged = settings.params.num_charged;
}
} // namespace m365bms
