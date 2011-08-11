/****************************************************************************************
** RaceFlash - Firmware Update Tool for PerformanceBox
** (c) Georg Swoboda, 2011
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*****************************************************************************************/

/*
** RUF files contain a header + chunks of compressed data (the intel hex files) and a tail
** zlib inflate/deflate is used to access the intel hex files
*/

#ifndef _RUF_H
#define _RUF_H


#define RUF_HDR_SIZE	0x21		/* size of the RUF header */
#define RUF_TAIL_SIZE	0x2A		/* size of the RUF tail */
#define RUF_MODEL_SIZE  0x8		/* size of the RUF model string */
#define RUF_NO_MODELS 	0x2		/* currently 2 models are known (PB/DB)*/
#define RUF_PAD_BYTES   0x1000 		/* 0x00 padding within RUF files */

static unsigned char ruf_hdr[RUF_HDR_SIZE]={0x52,0x61,0x63,0x65,0x6c,0x6f,0x67,0x69,0x63,0x20,0x55,0x70,0x67,0x72,0x61,0x64, /*Racelogic Upgrad*/
		 	               0x65,0x20,0x46,0x69,0x6c,0x65,0x20,0x56,0x34,0x53,0x0d,0x0a,0x2d,0x00,0x00,0x00, /*e File V4S..-...*/
 				       0x0b };

static unsigned char ruf_tail[RUF_TAIL_SIZE]={0x45,0x65,0x70,0x72,0x6f,0x6d,0x20,0x4e,0x4f,0x54,0x20,0x52,0x65,0x71,0x75,0x69, /*|Eeprom NOT Requi|*/ 
					 0x72,0x65,0x64,0x0d,0x0a,0x42,0x61,0x6e,0x6e,0x6f,0x72,0x20,0x4e,0x4f,0x54,0x20, /*|red..Bannor NOT |*/
					 0x52,0x65,0x71,0x75,0x69,0x72,0x65,0x64,0x0d,0x0a};				  /*|Required..|*/


static unsigned char ruf_model[RUF_NO_MODELS][RUF_MODEL_SIZE]= {
	{0x4e,   0x30,   0x30,   0x30,   0x30,   0x30,   0x30,   0x30   },   /* N00000000 - PB */
	{0x43,   0x30,   0x30,   0x30,   0x30,   0x30,   0x30,   0x30   }    /* C00000000 - DB */
};

/**********************/
/* ruf file structure */
/**********************/

struct ruf_file {
	unsigned char * header;
	unsigned long   header_size;
	unsigned char * model;
	unsigned long	model_size;
	unsigned char * firmware;
	unsigned long	firmware_size;
	unsigned char * padding;
	unsigned long	padding_size;
	unsigned char * tail;
	unsigned long	tail_size;
};

struct ruf_chunk {
	unsigned int   ret;
	unsigned char  *data;
	unsigned long  size;
};

struct ruf_file * ruf_read(char *file_name);
int ruf_write(char *out_file_name, struct ruf_file * rf);

#endif
