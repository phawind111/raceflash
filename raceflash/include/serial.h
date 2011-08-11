/****************************************************************************************
** RaceFlash - Firmware Update Tool for PerformanceBox
** (c) Georg Swoboda, 2011
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*****************************************************************************************/


#ifndef _SERIAL_H
#define _SERIAL_H

#define BAUDRATE B115200
#define _POSIX_SOURCE 1 /* POSIX compliant source */

#define MAX_MSG_SIZE	0x4FF
#define MODEL_SIZE	13

#define RL_MSG_VERSION_MODEL 	0x20
#define RL_MSG_GETSEED		0x12
#define RL_MSG_UNLOCK		0x13
#define RL_MSG_OK		0x01
#define RL_MSG_SHOW		0x1B
#define RL_ERASE_FLASH		0xE5
#define RL_WRITE_FLASH		0x03
#define RL_RESET		0x50

#define BIN_CHUNK_SIZE		0x400

int serial_init(char * modem_device);
int serial_send(int serial_fd, unsigned char msg_type, unsigned char *msg_buf, unsigned char *tx_data, unsigned int size);
unsigned int serial_read(int fd, unsigned char *buf, int buf_size, int timeout_s);

#endif
