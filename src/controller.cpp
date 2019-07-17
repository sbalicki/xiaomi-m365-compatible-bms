#include "controller.h"

#include "config.h"

#include <Arduino.h>
#include <PinChangeInterrupt.h>

#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>

extern volatile unsigned long timer0_millis;
volatile unsigned int g_timer2Overflows = 0;

// I2CAddress = 0x08, crcEnabled = true
volatile bool g_interruptFlag = false;
unsigned long g_lastActivity = 0;
unsigned long g_lastUpdate = 0;
volatile bool g_uartRxInterrupted = false;
volatile bool g_wakeupFlag = false;

void uartRxISR()
{
    g_uartRxInterrupted = true;
    g_wakeupFlag = true;
}

ISR(TIMER2_OVF_vect)
{
    // only used to keep track of time while sleeping to adjust millis()
    g_timer2Overflows++;
}

namespace m365bms
{

void Controller::init()
{
    MCUSR = 0;
    wdt_disable();

    Serial.begin(76800);
    Serial.println(F("BOOTED!"));

    power_adc_disable();
    power_spi_disable();
    power_timer1_disable();
    power_twi_disable();
    delay(1000);

    settings.read();

    bms.begin(BMS_BOOT_PIN);
    bms.setThermistors(0b110);
    bms.apply_settings(settings);

    // Bluetooth power pin
    pinMode(BMS_VDD_EN_PIN, OUTPUT);

    // attach ALERT interrupt
    pinMode(BMS_ALERT_PIN, INPUT);
    auto callback = reinterpret_cast<void(*)()>(&this->alert_callback);
    attachPCINT(digitalPinToPCINT(BMS_ALERT_PIN), callback, RISING);

    // attach UART RX pin interrupt to wake from deep sleep
    attachPCINT(digitalPinToPCINT(0), uartRxISR, CHANGE);
    disablePCINT(digitalPinToPCINT(0));

    interrupts();

    delay(1000);
    bms.update();
    bms.resetSOC(100);

    bms.enableDischarging();
    bms.enableCharging();

    wdt_enable(WDTO_1S);
}

void Controller::update()
{
    unsigned long now = millis();
    if(g_interruptFlag || (unsigned long)(now - g_lastUpdate) >= 500)
    {
        if(g_interruptFlag)
            g_interruptFlag = false;
    {
    uint8_t error = bms.update(); // should be called at least every 250 ms
    g_lastUpdate = now;

    // update M365BMS struct
        // charging state
        if(bms.getBatteryCurrent() > (int16_t)settings.params.idle_currentThres)
            bms.params.status |= (1 << 6); // charging
        else if(bms.getBatteryCurrent() < (int16_t)settings.params.idle_currentThres / 2)
            bms.params.status &= ~(1 << 6);

        if(error & STAT_OV) {
            bms.params.status |= (1 << 9); // overvoltage
            error &= ~STAT_OV;
        }
        else
            bms.params.status &= ~(1 << 9);

        uint16_t batVoltage = bms.getBatteryVoltage() / 10;
        if(batVoltage > bms.params.max_voltage)
            bms.params.max_voltage = batVoltage;

        int16_t batCurrent = bms.getBatteryCurrent() / 10;
        if(batCurrent > 0 && (uint16_t)batCurrent > bms.params.max_charge_current)
            bms.params.max_charge_current = batCurrent;
        else if(batCurrent < 0 && (uint16_t)-batCurrent > bms.params.max_discharge_current)
            bms.params.max_discharge_current = -batCurrent;

        bms.params.capacity_left = bms.params.design_capacity * bms.getSOC() / 100.0;
        bms.params.percent_left = bms.getSOC();
        bms.params.current = -batCurrent;
        bms.params.voltage = batVoltage;
        bms.params.temperature[0] = bms.getTemperatureDegC(1) + 20.0;
        bms.params.temperature[1] = bms.getTemperatureDegC(2) + 20.0;

        if(bms.getHighestTemperature() > (settings.params.temp_maxDischargeC - 3) * 10)
            bms.params.status |= (1 << 10); // overheat
        else
            bms.params.status &= ~(1 << 10);

        if(bms.batCycles_) {
            bms.params.num_cycles += bms.batCycles_;
            bms.batCycles_ = 0;
            settings.write();
        }

        if(bms.chargedTimes_) {
            bms.params.num_charged += bms.chargedTimes_;
            bms.chargedTimes_ = 0;
        }

        uint8_t numCells = bms.getNumberOfConnectedCells();
        for(uint8_t i = 0; i < numCells; i++)
            bms.params.cell_voltages[i] = bms.getCellVoltage(i);

        // cell voltage difference too big
        uint16_t bigDelta = bms.getMaxCellVoltage() - bms.getMinCellVoltage();
        if(bigDelta > 100)
            error = 1;

        if(error)
            bms.params.status &= ~1;
        else
            bms.params.status |= 1;
    }
    }

    message.ninebotRecv();

    if((unsigned long)(now - g_lastActivity) >= 5000)
    {
        // Disable TX
        UCSR0B &= ~(1 << TXEN0);

        // go into deep sleep, will wake up every 250ms by BQ769x0 ALERT or from USART1 RX (first byte will be lost)
        noInterrupts();
        set_sleep_mode(SLEEP_MODE_PWR_SAVE);

        // Timer/Counter2 8-byte OVF 8MHz /1024 = 32.64ms
        TCCR2A = 0;
        TCCR2B = (1<<CS22)|(1<<CS21)|(1<<CS20);
        TCNT2 = 0;
        TIMSK2 = (1<<TOIE2);

        UCSR0B &= ~(1 << RXEN0); // Disable RX
        enablePCINT(digitalPinToPCINT(0));

        wdt_reset();
        g_wakeupFlag = false;

        sleep_enable();
        interrupts();
        do // go to sleep if it's just timer2 that woke us up (unless we were idle for longer than 500ms)
        {
            sleep_cpu();
        } while(!g_wakeupFlag && g_timer2Overflows < 16);
        sleep_disable();

        // Disable Timer/Counter2 and add elapsed time to Arduinos 'timer0_millis'
        TCCR2B = 0;
        TIMSK2 = 0;
        float elapsed_time = g_timer2Overflows * 32.64 + TCNT2 * 32.64 / 255.0;
        timer0_millis += (unsigned long)elapsed_time;
        g_timer2Overflows = 0;

        if(g_uartRxInterrupted)
            g_lastActivity = millis();
        g_uartRxInterrupted = false;

        disablePCINT(digitalPinToPCINT(0));
        UCSR0B |= (1 << RXEN0); // Enable RX

        interrupts();
    }

    wdt_reset();
}

void Controller::alert_callback()
{
    bms.setAlertInterruptFlag();
    g_interruptFlag = true;
    g_wakeupFlag = true;
}

} // namespace m365bms
