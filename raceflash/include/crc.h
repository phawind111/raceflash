/****************************************************************************************
** RaceFlash - Firmware Update Tool for PerformanceBox
** (c) Georg Swoboda, 2011
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*****************************************************************************************/


#ifndef _CRC_H
#define _CRC_H


#define PB_CRC_POLY	0x1021
#define PB_UNLOCK_POLY	0x45DC

int calccrc(unsigned char *ptr, int count, int poly);

#endif
