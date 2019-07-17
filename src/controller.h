#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "bms.h"
#include "message.h"
#include "settings.h"

namespace m365bms
{

class Controller
{
public:
    void init();
    void update();

    void alert_callback();

public:
    BMS bms;
    Message message;
    Settings settings;
};

} // namespace m365bms

#endif // CONTROLLER_H
