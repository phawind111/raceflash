/****************************************************************************************
** RaceFlash - Firmware Update Tool for PerformanceBox
** (c) Georg Swoboda, 2011
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*****************************************************************************************/

#include "crc.h"


/*
** calc CRCIIT CRC for PB communication
*/
int calccrc(unsigned char *ptr, int count, int poly)
{
    short crc;
    char i;

    crc = 0;
    while (--count >= 0)
    {
        crc = crc ^ (int) *ptr++ << 8;
        i = 8;
        do
        {
            if (crc & 0x8000)
                crc = crc << 1 ^ poly;
            else
                crc = crc << 1;
        } while(--i);
    }
    return (crc);
}

