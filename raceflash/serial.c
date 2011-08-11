/****************************************************************************************
** RaceFlash - Firmware Update Tool for PerformanceBox
** (c) Georg Swoboda, 2011
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*****************************************************************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/select.h>
#include <fcntl.h>
#include <termios.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include "ihex.h"
#include "serial.h"
#include "crc.h"

//#define SERIAL_DEBUG


/*
** init serial device return port filehandle
*/
int serial_init(char * modem_device)
{
  int fd;
  struct termios newtio;

  fd = open(modem_device, O_RDWR | O_NOCTTY | O_NDELAY );
  if (fd <0) { printf("(SERIAL) ERROR: unable to open serial port: %s\n", modem_device); exit(-1); }

  bzero(&newtio, sizeof(newtio));
  newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
  newtio.c_iflag = IGNPAR;
  newtio.c_oflag &= ~OPOST;
  newtio.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

  /* set input mode (non-canonical, no echo,...) */
  newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
  newtio.c_cc[VMIN]     = 0;   /* blocking read until 5 chars received */
  tcflush(fd, TCIFLUSH);
  tcsetattr(fd,TCSANOW,&newtio);
  fcntl(fd, F_SETFL, 0);
  printf("(SERIAL) Serial device '%s' opened\n", modem_device);
  return(fd);
}

/*
** send message to PB
**
** if no payload shall be send, set tx_data=NULL
*/
int serial_send(int serial_fd, unsigned char msg_type, unsigned char *msg_buf, unsigned char *tx_data, unsigned int size)
{
// unsigned char 	msg_buf[MAX_MSG_SIZE]="";
 unsigned short crc=0x0;
 unsigned int  	ts,i;

 ts=6+size;
 if (size > MAX_MSG_SIZE-4)
 {
  printf("serial buffer not big enough\n");
  return(0);
 }

 msg_buf[0]=0xFF;
 msg_buf[1]=msg_type;
 msg_buf[2]=((ts)>>8)&0xFF;
 msg_buf[3]=((ts)&0xFF);
 if (tx_data)
 {
  memcpy(msg_buf+4, tx_data, size);
 }
 msg_buf[ts]='\0';

 crc=calccrc(msg_buf, 4+size, PB_CRC_POLY);

 msg_buf[ts-2]=crc>>8&0xFF;
 msg_buf[ts-1]=crc&0xFF;
 msg_buf[ts]='\0';

#ifdef SERIAL_DEBUG
 printf("\nTX: ");
 for (i=0;i<ts;i++)
 {
   printf("%02X ", msg_buf[i]);
 } 
 printf("\n");
 printf("size: %d\n",ts);
#endif

 i=write(serial_fd, msg_buf, ts);
#ifdef SERIAL_DEBUG
 printf("Sent (%d)\n",i);
#endif

 ts=serial_read(serial_fd, (unsigned char*)msg_buf, MAX_MSG_SIZE, 5);
#ifdef SERIAL_DEBUG
 printf("Answer: (%d)\n",ts);
 for(i=0;i<ts;i++)
 {
   printf("%02X ", msg_buf[i]);
 }
 printf("\n");
#endif
 return(ts);
}

/*
** receive from serial port
*/
unsigned int serial_read(int fd, unsigned char *buf, int buf_size, int timeout_s)
{
  fd_set 	readfs;
  int 		maxfd;
  int 		res;
  unsigned 	long total_received;
  unsigned 	int msg_len=0;
  unsigned 	short crc=0;

  maxfd=fd+1;
  FD_SET(fd, &readfs);
  struct timeval Timeout;

  /* setup receive buffer */
  memset(buf, 0x0, buf_size);

  Timeout.tv_usec = 0;  /* milliseconds */
  Timeout.tv_sec  = timeout_s;  /* seconds */

  total_received=0;
  while (total_received<buf_size)
  {
     res=select(maxfd, &readfs, NULL, NULL, &Timeout);
     if (res==0)
     {
   //    printf("timeout while reading\n");
       return(0);
     }
     else
     {
      if (FD_ISSET(fd, &readfs))
      {
       res = read(fd, buf+total_received, buf_size-total_received);
       total_received=total_received+res;
       Timeout.tv_usec = 0;  /* milliseconds */
       Timeout.tv_sec  = timeout_s;  /* seconds */
      }
     }
     /* check what we've got */
    if (!msg_len && total_received >= 4)
    {
     msg_len=(buf[2]<<8) | (buf[3]);
    } 
    /* total message received ? */
    if (total_received == msg_len)
    {
     crc=calccrc(buf, msg_len-2, PB_CRC_POLY);
     /* crc good ? */
     if ( (((crc>>8)&0xFF)==buf[msg_len-2]) &&  ((crc&0xFF)==buf[msg_len-1]) )
     {
#ifdef SERIAL_DEBUG
      printf("CRC good\n");
#endif
      return(total_received);
     } 
     else
      return(0); // invalid crc
    }
 }
 return(0);
}

