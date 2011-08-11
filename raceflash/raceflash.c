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
#include "ruf.h"
#include "ihex.h"
#include "serial.h"
#include "crc.h"
#include "file.h"
#include "flash.h"

static int verbose_flag=0;
static int query_mode=0;
static int update_mode=0;
static int extract_mode=0;
static int convert_mode=0;

void print_usage(void);

int main(int argc, char * argv[])
{
 int c;
 char 	          *in_file_name=NULL;
 char             *out_file_name=NULL;
 char	  	  *serial_device=NULL;
 struct ruf_file  *rf=NULL;
 unsigned char	  *ptr,*bf0=NULL,*bf1=NULL;
 char    	  *ihex0=NULL,*ihex1=NULL;
 unsigned long	  bf0_len=0, bf1_len=0;
 FILE		  *file;
 int		  serial_fd=0;
 unsigned char	  model_type=0;
 unsigned char	  model[MODEL_SIZE];
 unsigned long	  serial;
 unsigned long	  flash_key=0;
 unsigned char	  is_ruffile=0, is_hexfile=0, is_binfile=0;
 unsigned long	  fsize=0;
 unsigned char	  *hexbin_data;

 while ((c = getopt (argc, argv, "h?quxcm:f:o:d:a:")) != -1)
  switch (c)
  {
   case 'q':
     query_mode = 1;
     break;
   case 'v':
     verbose_flag = 1;
     break;
   case 'f':
     in_file_name = optarg;
     break;
   case 'o':
     out_file_name = optarg;
     break;
   case 'd':
     serial_device = optarg;
     break;
   case 'm':
     model_type =atoi(optarg);
     break;
   case 'u':
     update_mode=1;
     break;
   case 'x':
     extract_mode=1;
     break;
   case 'c':
     convert_mode=1;
     break;
   case '?':
     print_usage(); 
     exit(-1);
   case 'h':
     print_usage();
     exit(-1);
   default:
     print_usage();
     exit(-1);
 } 

 if (argc==1) 
 {
   print_usage();  
   exit(-1);
 }
 /* check file format if nessecary */
 if (in_file_name)
 {
    /* is it a ruf file or a hex file ? */
    if (strstr(in_file_name, ".hex") || strstr(in_file_name, ".HEX"))
    { 
     is_hexfile=1;
    }
    /* is it a ruf file or a hex file ? */
    if (strstr(in_file_name, ".bin") || strstr(in_file_name, ".BIN"))
    {
     is_binfile=1;
    }
    if (strstr(in_file_name, ".RUF") || strstr(in_file_name, ".ruf"))
    {
     is_ruffile=1;
    }
    if (file_exists(in_file_name))
    {
     printf(" ERROR: no such file '%s'\n", in_file_name);
     exit(-1);
    }
    if (!is_hexfile && !is_binfile && !is_ruffile)
    {
     printf(" ERROR: input filenames must end with .RUF/.ruf or .BIN/.bin\n");
     exit(-1);
    }
 }

 /* check if serial is available for modi that use it */
 if (update_mode || query_mode)
 {
  if (!serial_device)
  {
   printf(" ERROR: no serial device specified\n");
   exit(-1);
  }
  /* serial port here ? */
  if (!(serial_fd=serial_init(serial_device)))
  {
   printf(" ERROR: unable to open serial device: %s\n", serial_device);
   exit(-1);
  }
  /* is something attached ? */
  if ( (serial=rf_get_model_version(serial_fd, (unsigned char*)&model)) )
  {
  }
  else
  {
     printf("\n Device not responding, make sure the device is in the bootloader mode and connected to your computer\n");
     printf(" To enable the bootloader mode hold 'MODE' while powering on the device\n"); 
     exit(-1);
  }
  if (query_mode)
  {
   exit(0);
  }
 }

 /*********************************************
 ** CONVERT MODE
 **********************************************/
 if (convert_mode)
 {
     if (!in_file_name)
     {
         printf(" ERROR: no input filename given (-f)\n");
         exit(-1);
     }
     if (!out_file_name)
     {
         printf(" ERROR: no output filename given (-o)\n");
         exit(-1);
     }
     /*********************************************/
     /* .RUF FILE -> .BIN FILE                   */
     /*********************************************/
     if  (is_ruffile)
     {
       if (!(rf=(struct ruf_file*)ruf_read(in_file_name)))
       {
          printf("Error decompressing .RUF file\n");
          exit(1);
       }
       else
       {
          /* we got the RUF file disected, good */
          ptr=Binary_from_IHexRecord((char*)rf->firmware, rf->firmware_size, FLASH_0_LOW_ADDR, FLASH_0_HIGH_ADDR, &bf0_len);
          write_file_bin(out_file_name, ptr, FLASH_0_SIZE);
          ptr=Binary_from_IHexRecord((char*)rf->firmware, rf->firmware_size, FLASH_1_LOW_ADDR, FLASH_1_HIGH_ADDR, &bf1_len);
          append_file_bin(out_file_name, ptr, FLASH_1_SIZE);
     }
    } /* END of .RUF FILE -> .BIN FILES */

    /*********************************************/
    /* .BIN FILES to RUF FILE                    */
    /*********************************************/
    if  (is_binfile)
    {

       if ((model_type < 1) || (model_type > 2))
       {
        printf(" ERROR: invalid model type selected (1=PerformanceBox, 2=Driftbox)\n");
        exit(-1);
       } 
 
       bf0=read_file_bin(in_file_name, &bf0_len);

       /* check if we need both flashes */
       if (bf0_len > FLASH_0_SIZE)
       {
        printf("(BIN) Flash0 size: %d\n", FLASH_0_SIZE);
        printf("(BIN) Flash1 size: %ld\n", bf0_len-FLASH_0_SIZE);
        ihex0=(char*)IHexRecord_from_Binary((unsigned char*) bf0              , FLASH_0_SIZE, FLASH_0_LOW_ADDR, FLASH_0_HIGH_ADDR);
        ihex1=(char*)IHexRecord_from_Binary((unsigned char*)(bf0+FLASH_0_SIZE), bf0_len-FLASH_0_SIZE, FLASH_1_LOW_ADDR, FLASH_1_HIGH_ADDR);
       }
       else
       {
        printf("(BIN) Flash0 size: %ld\n", bf0_len);
        ihex0=(char*)IHexRecord_from_Binary(bf0, bf0_len, FLASH_0_LOW_ADDR, FLASH_0_HIGH_ADDR);
       }


       if (bf0_len > FLASH_0_SIZE)
       {
        ptr=malloc(strlen((char*)ihex0)+strlen((char*)ihex1)+1);
       }
       else
        ptr=malloc(strlen((char*)ihex0)+strlen(IHEX_END_TAG)+1);


       /* concat the two firmware chunks */
       strcpy((char*)ptr, ihex0);
       free(ihex0);
       if (ihex1)
       {
        strcat((char*)ptr, ihex1);
        free(ihex1);
       }

       /* append ihex file end tag */
       strcat((char*)ptr, IHEX_END_TAG);

     //  write_file_bin("/tmp/hex0", ptr, strlen((char*)ptr));

       rf=(struct ruf_file*)malloc(sizeof(struct ruf_file));
       rf->firmware=ptr;
       rf->firmware_size=strlen((char*)ptr);
       printf("(IHEX) Firmware size: %ld\n", rf->firmware_size);

       /* setup model string */
       rf->model=malloc(RUF_MODEL_SIZE);
       memcpy(rf->model, ruf_model[model_type-1], RUF_MODEL_SIZE);
       rf->model_size=RUF_MODEL_SIZE;

       /* write firmware file */
       ruf_write(out_file_name, rf);
    } /* END of .BIN FILES to -> .RUF FILE */
 }

 /*********************************************
 ** UPDATE MODE 
 **********************************************/
 if (update_mode)
 {
     if (!in_file_name) 
     {
        printf(" ERROR: no input filename given\n");
        exit(-1);
     }

     /*********************************************/
     /* .RUF FILE MODE                            */
     /*********************************************/
     if (is_ruffile)
     {
        if (!(rf=(struct ruf_file*)ruf_read(in_file_name))) 
        {
             printf("(RUF) ERROR: Decompressing .RUF file\n");
             exit(1);
        }
        else
        {
            printf("(FLASH) Found device '%s' with serial# '%ld'\n", model, serial); 
            /* we got the RUF file disected, good */
            /* convert ihex records to binary - two sections flash bf0 and bf1 */

            bf0=Binary_from_IHexRecord((char*)rf->firmware, rf->firmware_size, FLASH_0_LOW_ADDR, FLASH_0_HIGH_ADDR, &bf0_len);
            bf1=Binary_from_IHexRecord((char*)rf->firmware, rf->firmware_size, FLASH_1_LOW_ADDR, FLASH_1_HIGH_ADDR, &bf1_len);
	    if (bf0_len > FLASH_0_SIZE)
	    {
             printf("(IHEX) ERROR: ihex data for flash0 is too big (224k max)\n");
             exit(-1);
            }
            if (bf1_len > FLASH_1_SIZE)
            {
             printf("(IHEX) ERROR: ihex data for flash1 is too big (16k max)\n");
             exit(-1);
            }
	    if (bf0_len+bf1_len==0)
	    {
	     printf("(FLASH) No data in either Flash to be programmed\n");
	     exit(-1);
            }
//	    write_file_bin("/tmp/MAIN/rf_bf0", bf0, FLASH_0_SIZE);
//          write_file_bin("/tmp/MAIN/rf_bf1", bf1, FLASH_1_SIZE);

            if (!rf_unlock(serial_fd))
            {
             printf("(FLASH) ERROR: unable to unlock bootloader\n");
             exit(-1);
            }
	    else
             printf("(FLASH) Bootloader succesfully unlocked\n");

            flash_key=rf_get_flash_key(bf0, 0x38000, bf1, 0x3FFF);
	    printf("(FLASH) Calculated FLASH Key=0x%04lX\n", flash_key);

            if (bf0 && bf1)
            {
                /* we got the binary data lets clear the flash on the device */
                if (rf_erase_flash(serial_fd))
                { 
		  rf_set_progress_bar(serial_fd, bf0_len);
                  rf_programm_flash(serial_fd, bf0, FLASH_0_LOW_ADDR, FLASH_0_LOW_ADDR + bf0_len ); 
		  rf_set_progress_bar(serial_fd, bf1_len);
                  rf_programm_flash(serial_fd, bf1, FLASH_1_LOW_ADDR, FLASH_1_LOW_ADDR + bf1_len ); 
                  rf_set_flash_key(serial_fd, flash_key);
	          rf_reset_device(serial_fd);
                }
                else
                {
                  printf("(FLASH) ERROR: eraseing flash\n");
                  exit(-1);
                }
            }
            else
            {
                printf("(IHEX) ERROR: converting ihex to binary\n");
            }
        }
    } /* END of .RUF */               

    /*********************************************/
    /* .BIN FILE MODE                            */
    /*********************************************/
    if (is_binfile)
    {
        if ((fsize=get_filesize(in_file_name)))
        { 
	 printf("(FLASH) Found device '%s' with serial# '%ld'\n", model, serial);
	 printf("(FILE) Binary file size: %ld\n", fsize);
 	 hexbin_data=malloc(0x38000+0x4000);
         memset((unsigned char*)hexbin_data, 0xFF, FLASH_0_SIZE + FLASH_1_SIZE);
         file=fopen(in_file_name, "rb"); 
	 fseek(file, 0, 0);
         fread(hexbin_data, fsize, 1, file);
         if (!rf_unlock(serial_fd))
         {
             printf("(FLASH) ERROR: unable to unlock bootloader\n");
             exit(-1);
         }
         else
             printf("(FLASH) Bootloader succesfully unlocked\n");

         flash_key=rf_get_flash_key(hexbin_data, 0x38000, hexbin_data+0x38000, 0x3FFF);
	 printf("(FLASH) Calculated FLASH Key=0x%04lX\n", flash_key);
         if (rf_erase_flash(serial_fd))
         {
	    rf_set_progress_bar(serial_fd, FLASH_0_SIZE);
	    if (fsize < FLASH_0_SIZE)
            { 
              rf_programm_flash(serial_fd, hexbin_data, FLASH_0_LOW_ADDR, FLASH_0_LOW_ADDR + fsize -1);
            }
            else
            {
              rf_programm_flash(serial_fd, hexbin_data, FLASH_0_LOW_ADDR, FLASH_0_HIGH_ADDR);
            }

	    if (fsize > FLASH_0_SIZE) // check if have to programm FLASH_1
	    {
             rf_set_progress_bar(serial_fd, FLASH_1_SIZE);	
             rf_programm_flash(serial_fd, hexbin_data + FLASH_0_SIZE, FLASH_1_LOW_ADDR, FLASH_1_LOW_ADDR + (fsize-FLASH_0_SIZE)-1);
            }
            rf_set_flash_key(serial_fd, flash_key);
	    rf_reset_device(serial_fd);
         }
         else
         {
            printf("(FLASH) ERROR: eraseing flash\n");
            exit(-1);
         }

        }
    } /* END of .HEX */
}
printf("\n");
exit(0);
}


void print_usage(void)
{
 printf("\n");
 printf("Usage: raceflash [OPTIONS]\n");
 printf("\n");
 printf("\t -u -f <firmware.RUF> -d <serial_device>                - write <firmware.RUF> to flash\n");
 printf("\t -u -f <firmware.bin> -d <serial_device>                - write binary file to flash\n");
 printf("\t -c -f <firmware.RUF> -o <firmware1.bin>                - convert <firmware.RUF> to binary file\n");
 printf("\t -c -m <model> -f <firmware1.bin> -o <firmware.RUF>     - convert binary file to <firmware.RUF>\n");
 printf("\n");
 printf(" <model> is 1 for PerformanceBox, 2 for DriftBox\n");
 printf("\n\n");
}
