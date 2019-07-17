#include <Arduino.h>
#include <PinChangeInterrupt.h>

#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>

#include "controller.h"

m365bms::Controller controller;

void setup()
{
    controller.init();
}

void loop()
{
    controller.update();
}
