#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdint.h>

namespace m365bms
{

struct NinebotMessage
{
    uint8_t header[2]; // 0x55, 0xAA

    uint8_t length; // length of data + 2
    uint8_t addr; // receiver address
    uint8_t mode; // read = 1 / write = 3
    uint8_t offset; // data offset/index in array
    uint8_t data[253]; // write = data, read = uint8_t read length

    uint16_t checksum; // (bytes without header) XOR 0xFFFF
};

class Message
{
public:
    void onNinebotMessage(NinebotMessage &msg);
    void ninebotSend(NinebotMessage &msg);
    void ninebotRecv();

    void debug_print();
};

} // namespace m365

#endif // MESSAGE_H
