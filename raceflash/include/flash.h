/****************************************************************************************
** RaceFlash - Firmware Update Tool for PerformanceBox
** (c) Georg Swoboda, 2011
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*****************************************************************************************/


#ifndef _FLASH_H
#define _FLASH_H

/* 224K */
#define FLASH_0_LOW_ADDR	0x40008000
#define FLASH_0_HIGH_ADDR	0x4003FFFF

/* 16K */
#define FLASH_1_LOW_ADDR	0x400C0000
#define FLASH_1_HIGH_ADDR	0x400C3FFF

/* 32K */
#define FLASH_BL_LOW_ADDR	0x40000000
#define FLASH_BL_HIGH_ADDR	0x40007FFF

/* 32 KB*/
#define FLASH_BF04_LOW_ADDR	0x40008000
#define FLASH_BF04_HIGH_ADDR	0x4000FFFF
/* 64 KB */
#define FLASH_BF05_LOW_ADDR     0x40010000
#define FLASH_BF05_HIGH_ADDR    0x4001FFFF
/* 64 KB */
#define FLASH_BF06_LOW_ADDR     0x40020000
#define FLASH_BF06_HIGH_ADDR    0x4002FFFF
/* 64 KB */
#define FLASH_BF07_LOW_ADDR     0x40030000
#define FLASH_BF07_HIGH_ADDR    0x4003FFFF
/* 8 KB*/
#define FLASH_BF10_LOW_ADDR     0x400C0000
#define FLASH_BF10_HIGH_ADDR    0x400C1FFF
/* 8 KB*/
#define FLASH_BF11_LOW_ADDR     0x400C2000
#define FLASH_BF11_HIGH_ADDR    0x400C3FFF

/* flash sizes */
#define FLASH_BF04_SIZE		(FLASH_BF04_HIGH_ADDR - FLASH_BF04_LOW_ADDR)
#define FLASH_BF05_SIZE         (FLASH_BF05_HIGH_ADDR - FLASH_BF05_LOW_ADDR)
#define FLASH_BF06_SIZE         (FLASH_BF06_HIGH_ADDR - FLASH_BF06_LOW_ADDR)
#define FLASH_BF07_SIZE         (FLASH_BF07_HIGH_ADDR - FLASH_BF07_LOW_ADDR)
#define FLASH_BF10_SIZE         (FLASH_BF10_HIGH_ADDR - FLASH_BF10_LOW_ADDR)
#define FLASH_BF11_SIZE         (FLASH_BF11_HIGH_ADDR - FLASH_BF11_LOW_ADDR)
#define FLASH_0_SIZE            (FLASH_0_HIGH_ADDR - FLASH_0_LOW_ADDR +1)
#define FLASH_1_SIZE            (FLASH_1_HIGH_ADDR - FLASH_1_LOW_ADDR +1)
#define FLASH_BL_SIZE           (FLASH_BL_HIGH_ADDR - FLASH_BL_LOW_ADDR +1)








unsigned short rf_short_swap(unsigned short in);
unsigned long rf_long_swap(unsigned long in);
int rf_set_flash_key(int serial_fd, unsigned long key);
unsigned long rf_get_flash_key(unsigned char *bf0, unsigned long bf0_len, unsigned char *bf1, unsigned long bf1_len);
int rf_programm_flash(int serial_fd, unsigned char *bin_data, unsigned long from, unsigned long to);
void rf_set_progress_bar(int serial_fd, unsigned long size);
int rf_erase_flash(int serial_fd);
int rf_unlock(int serial_fd);
unsigned long rf_get_model_version(int serial_fd, unsigned char *model_buf);
unsigned short rf_short_swap(unsigned short in);
unsigned long rf_long_swap(unsigned long in);
void rf_reset_device(int serial_fd);

#endif
