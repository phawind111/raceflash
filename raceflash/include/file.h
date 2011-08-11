/****************************************************************************************
** RaceFlash - Firmware Update Tool for PerformanceBox
** (c) Georg Swoboda, 2011
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*****************************************************************************************/


#ifndef _FILE_H
#define _FILE_H

#if defined(MSDOS) || defined(OS2) || defined(WIN32) || defined(__CYGWIN__)
#  include <fcntl.h>
#  include <io.h>
#  define SET_BINARY_MODE(file) setmode(fileno(file), O_BINARY)
#else
#  define SET_BINARY_MODE(file)
#endif


int file_exists(char *file_name);
unsigned long get_filesize(char * file_name);
unsigned long write_file_bin(char * file_name, unsigned char *data, unsigned long size);
unsigned char * read_file_bin(char * file_name, unsigned long *size);
unsigned long append_file_bin(char * file_name, unsigned char *data, unsigned long size);

#endif
