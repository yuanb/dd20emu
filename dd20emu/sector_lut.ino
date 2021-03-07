#include "sector_lut.h"

sector_lut::sector_lut()
{
  
}

int sector_lut::sync_gap1(uint8_t* buf, uint8_t &TR, uint8_t &SEC)
{
    int gap1_size = -1;

    //Sync bytes
    if (*buf++ == 0x80 && *buf++ == 0x80 && *buf++ == 0x80 && *buf++ == 0x80 && *buf++ == 0x80)
    {
        if (*buf == 0x80) {
            buf++;
            if (*buf++ == 0x00) {
                gap1_size = 7;
            }
        } else if (*buf++ == 0x00) {
            gap1_size = 6;
        }
        //IDAM
        if (*buf++ == 0xFE && *buf++ == 0xE7 && *buf++ == 0x18 && *buf++ == 0xC3) {
            TR = *buf++;
            SEC = *buf;
        }
    }

    return gap1_size;
}
