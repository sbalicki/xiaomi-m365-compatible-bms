#include "settings.h"

#include <EEPROM.h>

namespace m365bms
{

void Settings::read()
{
    if(EEPROM.read(0) != params.header[0] || EEPROM.read(1) != params.header[1])
    {
        for(uint16_t i = 0; i < EEPROM.length(); i++) {
            EEPROM.write(i, 0);
        }
        EEPROM.put(0, params);
    }
    else
    {
        EEPROM.get(0, params);
    }
}

void Settings::write()
{
    EEPROM.put(0, params);
}


} // namespace m365bms
