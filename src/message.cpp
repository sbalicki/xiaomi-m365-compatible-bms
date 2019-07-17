#include "message.h"

#include "config.h"

#include <Arduino.h>
#include <EEPROM.h>

namespace m365bms
{

void Message::onNinebotMessage(NinebotMessage &msg)
{
    // Enable TX
    UCSR0B |= (1 << TXEN0);

    if(msg.addr != M365BMS_RADDR)
        return;
    /*
    if(msg.mode == 0x01 || msg.mode == 0xF1)
    {
        if(msg.length != 3)
            return;

        uint16_t ofs = (uint16_t)msg.offset * 2; // word aligned
        uint8_t sz = msg.data[0];

        if(sz > sizeof(NinebotMessage::data))
            return;

        msg.addr = M365BMS_WADDR;
        msg.length = 2 + sz;

        if(msg.mode == 0x01)
        {
            if((ofs + sz) > sizeof(g_M365BMS))
                return;

            memcpy(&msg.data, &((uint8_t *)&g_M365BMS)[ofs], sz);
        }
        else if(msg.mode == 0xF1)
        {
            if((ofs + sz) > sizeof(g_Settings))
                return;

            memcpy(&msg.data, &((uint8_t *)&g_Settings)[ofs], sz);
        }

        ninebotSend(msg);
    }
    else if(msg.mode == 0x03 || msg.mode == 0xF3)
    {
        uint16_t ofs = (uint16_t)msg.offset * 2; // word aligned
        uint8_t sz = msg.length - 2;

        if(msg.mode == 0x03)
        {
            if((ofs + sz) > sizeof(g_M365BMS))
                return;

            memcpy(&((uint8_t *)&g_M365BMS)[ofs], &msg.data, sz);
        }
        else if(msg.mode == 0xF3)
        {
            if((ofs + sz) > sizeof(g_Settings))
                return;

            memcpy(&((uint8_t *)&g_Settings)[ofs], &msg.data, sz);
        }
    }
    else if(msg.mode == 0xFA)
    {
        switch(msg.offset)
        {
            case 1: {
                applySettings();
            } break;
            case 2: {
                EEPROM.get(0, g_Settings);
            } break;
            case 3: {
                EEPROM.put(0, g_Settings);
            } break;
#if BQ769X0_DEBUG
            case 4: {
                g_Debug = msg.data[0];
            } break;
            case 5: {
                debug_print();
            } break;
#endif
            case 6: {
                g_BMS.disableDischarging();
                g_BMS.disableCharging();
            } break;
            case 7: {
                g_BMS.enableDischarging();
                g_BMS.enableCharging();
            } break;

            case 8: {
                digitalWrite(BMS_VDD_EN_PIN, LOW);
            } break;
            case 9: {
                digitalWrite(BMS_VDD_EN_PIN, HIGH);
            } break;
            case 10: {
                // test watchdog
                for (;;) { (void)0; }
            } break;
            case 11: {
                // restart to bootloader
                typedef void (*do_reboot_t)(void);
                const do_reboot_t do_reboot = (do_reboot_t)((FLASHEND - 511) >> 1);
                wdt_disable();
                cli(); TCCR0A = TCCR1A = TCCR2A = 0; // make sure interrupts are off and timers are reset.
                MCUSR = 0;
                do_reboot();
            }
        }
    }
*/
}

void Message::ninebotSend(NinebotMessage &msg)
{
    msg.checksum = (uint16_t)msg.length + msg.addr + msg.mode + msg.offset;

    Serial.write(msg.header[0]);
    Serial.write(msg.header[1]);
    Serial.write(msg.length);
    Serial.write(msg.addr);
    Serial.write(msg.mode);
    Serial.write(msg.offset);
    for(uint8_t i = 0; i < msg.length - 2; i++)
    {
        Serial.write(msg.data[i]);
        msg.checksum += msg.data[i];
    }

    msg.checksum ^= 0xFFFF;
    Serial.write(msg.checksum & 0xFF);
    Serial.write((msg.checksum >> 8) & 0xFF);
}

void Message::ninebotRecv()
{
    static NinebotMessage msg;
    static uint8_t recvd = 0;
    static unsigned long begin = 0;
    static uint16_t checksum;

    while(Serial.available())
    {
        //g_lastActivity = millis();

        if(millis() >= begin + 100)
        { // 100ms timeout
            recvd = 0;
        }

        uint8_t byte = Serial.read();
        recvd++;

        switch(recvd)
        {
            case 1:
            {
                if(byte != 0x55)
                { // header1 mismatch
                    recvd = 0;
                    break;
                }

                msg.header[0] = byte;
                begin = millis();
            } break;

            case 2:
            {
                if(byte != 0xAA)
                { // header2 mismatch
                    recvd = 0;
                    break;
                }

                msg.header[1] = byte;
            } break;

            case 3: // length
            {
                if(byte < 2)
                { // too small
                    recvd = 0;
                    break;
                }

                msg.length = byte;
                checksum = byte;
            } break;

            case 4: // addr
            {
                if(byte != M365BMS_RADDR)
                { // we're not the receiver of this message
                    recvd = 0;
                    break;
                }

                msg.addr = byte;
                checksum += byte;
            } break;

            case 5: // mode
            {
                msg.mode = byte;
                checksum += byte;
            } break;

            case 6: // offset
            {
                msg.offset = byte;
                checksum += byte;
            } break;

            default:
            {
                if(recvd - 7 < msg.length - 2)
                { // data
                    msg.data[recvd - 7] = byte;
                    checksum += byte;
                }
                else if(recvd - 7 - msg.length + 2 == 0)
                { // checksum LSB
                    msg.checksum = byte;
                }
                else
                { // checksum MSB and transmission finished
                    msg.checksum |= (uint16_t)byte << 8;
                    checksum ^= 0xFFFF;

                    if(checksum != msg.checksum)
                    { // invalid checksum
                        recvd = 0;
                        break;
                    }

                    onNinebotMessage(msg);
                    recvd = 0;
                }
            } break;
        }
    }
}

#if BQ769X0_DEBUG
void debug_print()
{
    g_BMS.printRegisters();
    Serial.println(F(""));

    Serial.print(F("Battery voltage: "));
    Serial.print(g_BMS.getBatteryVoltage());
    Serial.print(F(" ("));
    Serial.print(g_BMS.getBatteryVoltage(true));
    Serial.println(F(")"));

    Serial.print(F("Battery current: "));
    Serial.print(g_BMS.getBatteryCurrent());
    Serial.print(F(" ("));
    Serial.print(g_BMS.getBatteryCurrent(true));
    Serial.println(F(")"));

    Serial.print(F("SOC: "));
    Serial.println(g_BMS.getSOC());

    Serial.print(F("Temperature: "));
    Serial.print(g_BMS.getTemperatureDegC(1));
    Serial.print(F(" "));
    Serial.println(g_BMS.getTemperatureDegC(2));

    Serial.print(F("Balancing status: "));
    Serial.println(g_BMS.getBalancingStatus());

    Serial.print(F("Cell voltages ("));
    int numCells = g_BMS.getNumberOfCells();
    Serial.print(numCells);
    Serial.println(F("):"));
    for(int i = 0; i < numCells; i++) {
        Serial.print(g_BMS.getCellVoltage_(i));
        Serial.print(F(" ("));
        Serial.print(g_BMS.getCellVoltage_(i, true));
        Serial.print(F(")"));
        if(i != numCells - 1)
            Serial.print(F(", "));
    }
    Serial.println(F(""));

    Serial.print(F("Cell V: Min: "));
    Serial.print(g_BMS.getMinCellVoltage());
    Serial.print(F(" | Avg: "));
    Serial.print(g_BMS.getAvgCellVoltage());
    Serial.print(F(" | Max: "));
    Serial.print(g_BMS.getMaxCellVoltage());
    Serial.print(F(" | Delta: "));
    Serial.println(g_BMS.getMaxCellVoltage() - g_BMS.getMinCellVoltage());

    Serial.print(F("maxVoltage: "));
    Serial.println(g_M365BMS.max_voltage);
    Serial.print(F("maxDischargeCurrent: "));
    Serial.println(g_M365BMS.max_discharge_current);
    Serial.print(F("maxChargeCurrent: "));
    Serial.println(g_M365BMS.max_charge_current);

    Serial.print(F("XREADY errors: "));
    Serial.println(g_BMS.errorCounter_[ERROR_XREADY]);
    Serial.print(F("ALERT errors: "));
    Serial.println(g_BMS.errorCounter_[ERROR_ALERT]);
    Serial.print(F("UVP errors: "));
    Serial.println(g_BMS.errorCounter_[ERROR_UVP]);
    Serial.print(F("OVP errors: "));
    Serial.println(g_BMS.errorCounter_[ERROR_OVP]);
    Serial.print(F("SCD errors: "));
    Serial.println(g_BMS.errorCounter_[ERROR_SCD]);
    Serial.print(F("OCD errors: "));
    Serial.println(g_BMS.errorCounter_[ERROR_OCD]);
    Serial.println();
    Serial.print(F("DISCHG TEMP errors: "));
    Serial.println(g_BMS.errorCounter_[ERROR_USER_DISCHG_TEMP]);
    Serial.print(F("CHG TEMP errors: "));
    Serial.println(g_BMS.errorCounter_[ERROR_USER_CHG_TEMP]);
    Serial.print(F("CHG OCD errors: "));
    Serial.println(g_BMS.errorCounter_[ERROR_USER_CHG_OCD]);
}
#endif

} // namespace m365
