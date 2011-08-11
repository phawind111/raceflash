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
#include "file.h"

/*
** check if file exists
*/
int file_exists(char *file_name)
{
 return(access(file_name, F_OK));
}

/*
** return length of file
*/
unsigned long get_filesize(char * file_name)
{
 struct stat s;

 if (!file_name) return(0);
 if (stat((char*)file_name, &s) == -1)
 {
  printf(" error opening file: %s\n\n",file_name);
  exit(1);
 }
 return(s.st_size);
}

/*
** read binary file
*/
unsigned char * read_file_bin(char * file_name, unsigned long *size)
{
 FILE	       *file;
 unsigned char *data;
 unsigned long x;

 if (!file_name) return(0);

 *size=get_filesize(file_name);
 data=malloc(*size);
 file=fopen(file_name, "rb");
 SET_BINARY_MODE(file);
 x=fread(data, 1, *size, file);
 fclose(file);   
 if (x != *size)
 {
  printf("error reading from file: '%s'\n",file_name);
  return(NULL);
 } 
 return(data);
}

/*
** write binary file
*/
unsigned long write_file_bin(char * file_name, unsigned char *data, unsigned long size)
{
 FILE  	        *file;
 unsigned long x;

 if (!data) 
 {
   printf("(FILE/write) ERROR: no data\n");
   return(0);
 }
 if (!file_name)
 {
   printf("(FILE/write) ERROR: invalid filename\n");
   return(0);
 }
  
 if ((file=fopen(file_name, "wb")))
 { 
   SET_BINARY_MODE(file);
   x=fwrite(data, 1, size, file);
   fclose(file);
 }
 else
 {
   printf("(FILE/write) ERROR: unable to open file '%s'\n", file_name);
 }

 return(x);
}

/*
** append binary file
*/
unsigned long append_file_bin(char * file_name, unsigned char *data, unsigned long size)
{
 FILE           *file;
 unsigned long x;

 if (!data)
 {
   printf("(FILE/append) ERROR: no data\n");
   return(0);
 }
 if (!file_name)
 {
   printf("(FILE/append) ERROR: invalid filename\n");
   return(0);
 }

 if ((file=fopen(file_name, "ab")))
 {
    SET_BINARY_MODE(file);
    x=fwrite(data, 1, size, file);
    fclose(file);
 }
 else
 {
    printf("(FILE/append) ERROR: unable to open file '%s'\n", file_name);
 }
 return(x);
}

