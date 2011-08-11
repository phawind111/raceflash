/****************************************************************************************
** RaceFlash - Firmware Update Tool for PerformanceBox
** (c) Georg Swoboda, 2011
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*****************************************************************************************/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include "ihex.h"
#include "serial.h"
#include "crc.h"
#include "file.h"
#include "flash.h"

unsigned short rf_short_swap(unsigned short in);
unsigned long rf_long_swap(unsigned long in);

/*
** close communication and try to reset device
*/
void rf_reset_device(int serial_fd)
{
 unsigned char  buf[MAX_MSG_SIZE-4];

 printf("(FLASH) Reseting device\n");
 /* reset device */
 serial_send(serial_fd, RL_RESET, (unsigned char*)&buf, NULL, 0);
 return;
}

/*
** write the calculated flash key to 0x400C3FFC
*/
int rf_set_flash_key(int serial_fd, unsigned long key) 
{
 unsigned char buf[MAX_MSG_SIZE-4];
 unsigned int  len;
 unsigned char prog_buf[8];

 /* address */
 prog_buf[0]=0x40;
 prog_buf[1]=0x0C;
 prog_buf[2]=0x3F;
 prog_buf[3]=0xFC;
 /* flash key */
 memcpy(prog_buf+4, &key, 4);
 printf("(FLASH) Writing FLASH Key=0x%08lX - ",key);
 if ((len=serial_send(serial_fd, RL_WRITE_FLASH, buf, prog_buf, 8)) == 0x6)
 {
    if (buf[1] == RL_MSG_OK)
    {
      /* flash key programmed */
      printf("done\n");
      return(1);
    }
    else
     printf("failed\n");
 }
 return(0);
}


/*
** calculate the flash key that the bootldr uses to verify
** the application image
*/
unsigned long rf_get_flash_key(unsigned char *bf0, unsigned long bf0_len, unsigned char *bf1, unsigned long bf1_len)
{
 unsigned long sum_bf0, sum_bf1, sum_all;
 unsigned long c;
 unsigned long i;
 unsigned long r=0x00000000;
 unsigned long key;

 sum_bf0=0;
 sum_bf1=0;

 if (!bf0)
 {
   printf("(FLASH) ERROR: unable to extract flash key, invalid binary data0\n");
   exit(-1);
 }
 if (!bf1)
 {
   printf("(FLASH) ERROR: unable to extract flash key, invalid binary data1\n");
   exit(-1);
 }

 for(i=0; i<bf0_len; i=i+4)
 { 
  memcpy(&c, bf0+i, 4);
  sum_bf0+=c;
 }

 for(i=0; i<=(bf1_len-4); i=i+4)
 {
  memcpy(&c, bf1+i, 4);
  sum_bf1+=c;
 }

// printf("sum_bf0: %08lX\n", sum_bf0);
// printf("sum_bf1: %08lX\n", sum_bf1);
 sum_all=sum_bf0+sum_bf1;
 key=((long long)r-(long)sum_all);
// printf("flash key: %8lX\n",key);
 return(key);
}

int rf_programm_flash(int serial_fd, unsigned char *bin_data, unsigned long from, unsigned long to)
{
 unsigned char buf[MAX_MSG_SIZE-4];
 unsigned long size;
 unsigned long i;
 unsigned long chunk_size=BIN_CHUNK_SIZE;
 unsigned char prog_buf[0x4FF];
 unsigned long addr;
 unsigned int  len;
 unsigned int  rounds;
 unsigned int  rounds_ok=0;
 unsigned long laddr;

 if (!bin_data || (to-from<=0)) 
 {
  printf("(FLASH) programming (0x%08lX - 0x%08lX) [ no data to program ]\n", from, to);
  return(0);
 }
 if (to < 0x40008000)
 {
  printf("(FLASH/programm) ERROR: you better not try to program the bootloader area\n");
  exit(-1);
 }

 printf("(FLASH) programming (0x%08lX - 0x%08lX) [", from, to); 
 fflush(stdout);
 size=to-from;
 
 if (chunk_size*(size/chunk_size) == size)
 {
  rounds=(size/chunk_size)+1;
 }
 else
  rounds=(size/chunk_size);

 for (i=0; i <= rounds; i++)
 {
    addr=from+(i*chunk_size);
    laddr=addr;
    addr=rf_long_swap(addr);  
    /* setup programming buffer with address+binary data */
    memcpy(prog_buf, (unsigned char*)&addr, 4);
    memcpy(prog_buf +4, bin_data+(i*chunk_size), BIN_CHUNK_SIZE);

    /* send serial command to program chunk */
    if ((len=serial_send(serial_fd, RL_WRITE_FLASH, (unsigned char*)&buf, (unsigned char*)&prog_buf, BIN_CHUNK_SIZE+0x4)))
    {
      if (buf[1] == RL_MSG_OK)
      {
 	/* programming this sector was okay */
	rounds_ok++;
        printf("*");
        fflush(stdout);
      }
      else
      {
        printf("F");
        fflush(stdout);
      }
    } 
 }
 if (rounds_ok == rounds+1)
 {
  printf("] - successfully programmed\n(FLASH) Last chunk was at 0x%08lX\n", laddr);
  return(1);
 }
 else
 {
  printf("] (FLASH) ERROR: while programming flash at address 0x%08lX\n (FLASH) Last was at 0x%08lX\n", rf_long_swap(addr), laddr);
  exit(-1);
 } 
}

/*
** update the progress bar on the bootloader
** data is the number of bytes that are expected to be send/read to show 100% bar
** seems to count *any* payload, so its not really correctly used here, but also not needed  ...
*/
void rf_set_progress_bar(int serial_fd, unsigned long size)
{
 unsigned char  buf[MAX_MSG_SIZE-4];
 unsigned int   len;
 unsigned long  x1b_data=0x0;

 x1b_data=rf_long_swap(size+4); 
 /* show and set the percent bar to 0% */
 if ((len=serial_send(serial_fd, RL_MSG_SHOW, (unsigned char*)&buf, (unsigned char*)&x1b_data, 4)) == 0x06)
 {
 }
 return;
}

/*
** erase all flash areas except the bootloader
*/
int rf_erase_flash(int serial_fd)
{
 unsigned char  buf[MAX_MSG_SIZE-4];
 unsigned int   len;
 unsigned short x, addr;
 unsigned long  x1b_data=0x06<<24;

 /* show and set the percent bar to 0% */
 if ((len=serial_send(serial_fd, RL_MSG_SHOW, (unsigned char*)&buf, (unsigned char*)&x1b_data, 4)) == 0x06)
 {
  if (buf[1] == RL_MSG_OK)
  {
   /* start erasing the BF0/BF1 flash sectors */
   for (x=0x200;x>=0x10;x=x/2)
   {
      addr=rf_short_swap(x);
      printf("(FLASH) Erasing sector 0x%04X - ", x);
      fflush(stdout);
      if ((len=serial_send(serial_fd, RL_ERASE_FLASH, (unsigned char*)&buf, (unsigned char*)&addr, 2)) == 0x06)
      {
        if (buf[1] == RL_MSG_OK)
        {
         printf("done\n");
        }
        else
	 printf("failed\n");
      }
      else
       return(0);
   }   
  } 
  else
   return(0);
 }
 return(1);
}

/*
** unlock the device
*/
int rf_unlock(int serial_fd)
{
 unsigned char  buf[MAX_MSG_SIZE-4];
 unsigned int   len;
 unsigned short seed;
 unsigned short key=0x0;

 /* get the seed */
 if ((len=serial_send(serial_fd, RL_MSG_GETSEED, (unsigned char*)&buf, NULL, 0)) == 0x08)
 {
  if (buf[1] == RL_MSG_OK)
  {
    memcpy(&seed,buf+4,2); 
    key=calccrc((unsigned char*)&seed, 2, PB_UNLOCK_POLY);
    key=rf_short_swap(key);
    /* send the unlock key */
    if ((len=serial_send(serial_fd, RL_MSG_UNLOCK, (unsigned char*)&buf, (unsigned char*)&key, 2)) == 0x06)
    {
     if (buf[1] == RL_MSG_OK)
     {
      return(1); // unlocked
     }
   }
  }
 }
 return(0); // not unlocked
}

/*
** read serial & model type
*/
unsigned long rf_get_model_version(int serial_fd, unsigned char *model_buf)
{
 unsigned char buf[MAX_MSG_SIZE-4];
 unsigned int  len;
 unsigned long serial;
 unsigned int  i; 

 if ((len=serial_send(serial_fd, RL_MSG_VERSION_MODEL, buf, NULL, 0)) == 0x16)
 {
   memcpy(model_buf, buf+4, 12);
   model_buf[12]='\0'; 
   for (i=0;i<12;i++)
   {
    if (model_buf[i]==0xFF)
     model_buf[i]='\0';
   } 
   serial=(buf[len-3]<<24) | (buf[len-4]<<16) | (buf[len-5]<<8) | (buf[len-6]); 
   return(serial);
 }
 return(0);
}

/*
** 16 bit byteswap helper
*/
unsigned short rf_short_swap(unsigned short in)
{
 unsigned short out;

 out= ((in&0xFF) << 8) | ((in>>8)&0xFF);
 return out;
}

/*
** 32 bit byteswap helper
*/
unsigned long rf_long_swap(unsigned long in)
{
 unsigned long out;

 out= ((in&0xFF) << 24) | (((in>>8)&0xFF)<<16) | (((in>>16)&0xFF)<<8) | ((in>>24)&0xFF);
 return out;
}


