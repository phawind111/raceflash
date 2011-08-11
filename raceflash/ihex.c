/*
 *  ihex.c
 *  Utility functions to create, read, write, and print Intel HEX8 binary records.
 *
 *  Written by Vanya A. Sergeev <vsergeev@gmail.com>
 *  Version 1.0.5 - February 2011
 *  Modified by Georg Swoboda to include reading/writing IHEX Records from/to a string
 *
 */

#include "ihex.h"
#include "file.h"
#include "flash.h"

/*
** create a string suitable for an .hex file from an ihex_record
*/
char * Str_from_IHexRecord(IHexRecord *ihex)
{
  char           out_hdr[32];
  char           out_data[64];
  char           out_crc[16];
  static char    out_all[255];
  unsigned char	 j;

  if (ihex->dataLen > 16) // not supported, due buffer size
  {
   printf("(IHEX) datalen > 16 is not supported\n");
   return(NULL);
  }

  sprintf(out_hdr, "%c%2.2X%2.4X%2.2X", IHEX_START_CODE, ihex->dataLen, ihex->address, ihex->type);
  /* Write the data bytes */
  for (j = 0; j < ihex->dataLen; j++)
  {
    sprintf(out_data+(j*2), "%2.2X", ihex->data[j]);
  }
  /* Calculate and write the checksum field */
  sprintf(out_crc, "%2.2X", Checksum_IHexRecord(ihex));
  sprintf(out_all,"%s%s%s\r\n", out_hdr, out_data, out_crc);
  return(out_all);
}

/*
** convert binary data to ihex data
*/ 
unsigned char * IHexRecord_from_Binary(unsigned char *bin_data, unsigned long size, unsigned long from, unsigned long to)
{
 unsigned long	addr;
 unsigned long  ihex_len;
 IHexRecord 	ihex;
 char          * out_ihex;
 unsigned char * out=NULL;
 // unsigned long	out_len=1;
 unsigned long	this_len;
 unsigned long  from_swapped;


 if (!size || !bin_data)
 {
   /* invalid arguments */
   printf("(IHEX) ERROR: invalid arguments\n");
   return 0;
 } 

 out=malloc(size*3);

 addr=0x0;
 /***********************/
 /* create data records */ 
 /***********************/
 while (addr < size)
 {
  if ( ((from+addr) == 0x40008000) || ((from+addr) == 0x40008210) || ((from+addr) == 0x40010000) || ((from+addr) == 0x40020000) || ((from+addr) == 0x40030000) || ((from+addr) == 0x400C0000)) 
  {
//     printf("header at  0x%08lX\n", addr+from);
     /***************************************/
     /* make IHEX segment header - type 0x5 */
     /***************************************/
     from_swapped=rf_long_swap(from+addr);
     New_IHexRecord(0x5, 0x0, (unsigned char*)&from_swapped, 4, &ihex);
     out_ihex=Str_from_IHexRecord(&ihex);
     this_len=strlen(out_ihex);
//     out_len+=this_len;
//     out=realloc(out, out_len);
     strncat((char*)out, (char*)out_ihex, this_len);
     /***************************************/
     /* make IHEX segment header - type 0x4 */
     /***************************************/
     from_swapped=rf_long_swap((from+addr)&0xFFFF0000);
     New_IHexRecord(0x4, 0x0, (unsigned char*)&from_swapped, 2, &ihex);
     out_ihex=Str_from_IHexRecord(&ihex);
     this_len=strlen(out_ihex);
//     out_len+=this_len;
//     out=realloc(out, out_len);
     strncat((char*)out, (char*)out_ihex, this_len);
 //    printf("header at  0x%08lX\n", addr+from);
  }

  ihex_len=(size-addr); // how many byte left to write ? 
  if (ihex_len >=16)
  {
   ihex_len=16;
  }
//  printf("ihex_len: %d\n",ihex_len);
//  printf("offset: 0x%08lX\n", addr+from);

  New_IHexRecord(0, addr+(from&0xFFFF), bin_data+addr, ihex_len, &ihex);
  addr+=ihex_len;
//  Print_IHexRecord(&ihex); 

  // get a string from the filled record
  out_ihex=Str_from_IHexRecord(&ihex);
  this_len=strlen(out_ihex);
//  out_len+=this_len;
//  out=realloc(out, out_len+1); 
  strncat((char*)out, (char*)out_ihex, this_len);
 }
// printf("(IHEX) Generated Data:\n%s\n",out);
 return(out);
}


/*
** convert ihex records to binary
** ihex_data, size - string and size of the string containing the firmware 
** from, to - 32bit addresses encoded in ihex file that shall be extracted
*/
unsigned char * Binary_from_IHexRecord(char * ihex_data, unsigned long size, unsigned long from, unsigned long to, unsigned long *extracted_len) 
{
 char  		 *p,*n;
 unsigned int    s;
 IHexRecord      ihex;
 unsigned char  *bin_data;
 unsigned long   bin_addr;
 unsigned long	 ihex_lar=0x0, ihex_addr=0x0;
 unsigned int	 i;

 if (!size || !ihex_data)
 {
   /* invalid arguments */
   printf("(IHEX) ERROR: invalid arguments\n");
   return 0;
 }

 p=strstr(ihex_data,"\r\n");
 if (!p)
 { /* no line break ? no ihex file .. */
   printf("(IHEX) ERROR: no line break found in hex file, make sure it has dos style linebreaks\n");
   return 0;
 }


 s=p-ihex_data; // size of first chunk
 if (s<1)
 { /* first chunk not existing */
   printf("(IHEX) ERROR: no binary data found hex file (emtpy?)\n");
   return 0;
 }

 /* allocate the bin_data structure */
 /* we pad unused bytes with 0xFF - this means empty flash cells */
 bin_data=(unsigned char*)malloc(to-from+1);
 memset(bin_data, 0xFF, (to-from+1));		/* 0xFF = empty flash cell */

 p=ihex_data;
 *extracted_len=0;
 while ( (p-ihex_data) < size)
 {
//   printf("parsing %d\n", s);
   if (Parse_IHexRecord(&ihex, (unsigned char*)p, s) == IHEX_OK)
   {
//      Print_IHexRecord(&ihex);
      /* type 4 update LAR */
      if (ihex.type == 4)
      {
        ihex_lar=ihex.data[0] << 24 | ihex.data[1] << 16; 
      }
      /* type 0 flash addr =  LAR + address */
      if (ihex.type == 0)
      {
        ihex_addr=ihex_lar | (ihex.address&0xFFFF);
      }
      if (ihex_addr >= from && ihex_addr <= to)
      {
       if (ihex.type == 0)
       {
	  bin_addr=(ihex_addr-from); /* calc offset in binary from ihex address */
//          printf("Addr: %08lX / %08lX - %02d bytes\n", ihex_addr, bin_addr, ihex.dataLen);

          /* write ihex record as binary to 'bin_data' */
          for (i = 0; i < ihex.dataLen; i++) {
            bin_data[bin_addr+i]=ihex.data[i];
	    (*extracted_len)++;
          }
       }
      }
      if (ihex_addr > to) break;
   }
   else
   {
    printf("(IHEX) Error parsing ihex record\n");
   }

   if (!(p=strstr(p,"\r\n")))	// find next line
    break;
   p=p+2;	 	       // jump to next line
   if (!(n=strstr(p,"\r\n")))  // find end of next line
    break;
   s=n-p;		      // size = next line - this line
 }

 printf("(IHEX) Extracted %ld bytes from ihex data (0x%08lX-0x%08lX)\n", *extracted_len, from, to);
 /* return extracted binary data */
 return(bin_data);
}



/* Initializes a new IHexRecord structure that the paramater ihexRecord points to with the passed
 * record type, 16-bit integer address, 8-bit data array, and size of 8-bit data array. */
int New_IHexRecord(int type, uint16_t address, const uint8_t *data, int dataLen, IHexRecord *ihexRecord) {
	/* Data length size check, assertion of ihexRecord pointer */
	if (dataLen < 0 || dataLen > IHEX_MAX_DATA_LEN/2 || ihexRecord == NULL)
		return IHEX_ERROR_INVALID_ARGUMENTS;
	
	ihexRecord->type = type;
	ihexRecord->address = address;
	memcpy(ihexRecord->data, data, dataLen);
	ihexRecord->dataLen = dataLen;
	ihexRecord->checksum = Checksum_IHexRecord(ihexRecord);
	
	return IHEX_OK;		
}

/* Utility function to parse an Intel HEX8 record from a string */
int Parse_IHexRecord(IHexRecord *ihexRecord, unsigned char *in, unsigned int size) 
{
        char recordBuff[IHEX_RECORD_BUFF_SIZE];
        /* A temporary buffer to hold ASCII hex encoded data, set to the maximum length we would ever need */
        char hexBuff[IHEX_ADDRESS_LEN+1];
        int dataCount, i;

        /* Check our record pointer and file pointer */
        if (ihexRecord == NULL || in == NULL)
                return IHEX_ERROR_INVALID_ARGUMENTS;

	if  ( (size > IHEX_RECORD_BUFF_SIZE) || (size==0))
        {
          return IHEX_ERROR_INVALID_ARGUMENTS;
        }

        memcpy(recordBuff,in,size);

        /* Null-terminate the string at the first sign of a \r or \n */
        for (i = 0; i < (int)strlen(recordBuff); i++) {
                if (recordBuff[i] == '\r' || recordBuff[i] == '\n') {
                        recordBuff[i] = 0;
                        break;
                }
        }


        /* Check if we hit a newline */
        if (strlen(recordBuff) == 0)
                return IHEX_ERROR_NEWLINE;

        /* Size check for start code, count, addess, and type fields */
        if (strlen(recordBuff) < (unsigned int)(1+IHEX_COUNT_LEN+IHEX_ADDRESS_LEN+IHEX_TYPE_LEN))
                return IHEX_ERROR_INVALID_RECORD;

        /* Check the for colon start code */
        if (recordBuff[IHEX_START_CODE_OFFSET] != IHEX_START_CODE)
                return IHEX_ERROR_INVALID_RECORD;

       /* Copy the ASCII hex encoding of the count field into hexBuff, convert it to a usable integer */
        strncpy(hexBuff, recordBuff+IHEX_COUNT_OFFSET, IHEX_COUNT_LEN);
        hexBuff[IHEX_COUNT_LEN] = 0;
        dataCount = strtol(hexBuff, (char **)NULL, 16);

        /* Copy the ASCII hex encoding of the address field into hexBuff, convert it to a usable integer */
        strncpy(hexBuff, recordBuff+IHEX_ADDRESS_OFFSET, IHEX_ADDRESS_LEN);
        hexBuff[IHEX_ADDRESS_LEN] = 0;
        ihexRecord->address = strtol(hexBuff, (char **)NULL, 16);

        /* Copy the ASCII hex encoding of the address field into hexBuff, convert it to a usable integer */
        strncpy(hexBuff, recordBuff+IHEX_TYPE_OFFSET, IHEX_TYPE_LEN);
        hexBuff[IHEX_TYPE_LEN] = 0;
        ihexRecord->type = strtol(hexBuff, (char **)NULL, 16);

        /* Size check for start code, count, address, type, data and checksum fields */
        if (strlen(recordBuff) < (unsigned int)(1+IHEX_COUNT_LEN+IHEX_ADDRESS_LEN+IHEX_TYPE_LEN+dataCount*2+IHEX_CHECKSUM_LEN))
                return IHEX_ERROR_INVALID_RECORD;

        /* Loop through each ASCII hex byte of the data field, pull it out into hexBuff,
         * convert it and store the result in the data buffer of the Intel HEX8 record */
        for (i = 0; i < dataCount; i++) {
                /* Times two i because every byte is represented by two ASCII hex characters */
                strncpy(hexBuff, recordBuff+IHEX_DATA_OFFSET+2*i, IHEX_ASCII_HEX_BYTE_LEN);
                hexBuff[IHEX_ASCII_HEX_BYTE_LEN] = 0;
                ihexRecord->data[i] = strtol(hexBuff, (char **)NULL, 16);
        }
        ihexRecord->dataLen = dataCount;

        /* Copy the ASCII hex encoding of the checksum field into hexBuff, convert it to a usable integer */
        strncpy(hexBuff, recordBuff+IHEX_DATA_OFFSET+dataCount*2, IHEX_CHECKSUM_LEN);
        hexBuff[IHEX_CHECKSUM_LEN] = 0;
        ihexRecord->checksum = strtol(hexBuff, (char **)NULL, 16);

        if (ihexRecord->checksum != Checksum_IHexRecord(ihexRecord))
                return IHEX_ERROR_INVALID_RECORD;

        return IHEX_OK;
}

/* Utility function to read an Intel HEX8 record from a file */
int Read_IHexRecord(IHexRecord *ihexRecord, FILE *in) {
	char recordBuff[IHEX_RECORD_BUFF_SIZE];
	/* A temporary buffer to hold ASCII hex encoded data, set to the maximum length we would ever need */
	char hexBuff[IHEX_ADDRESS_LEN+1];
	int dataCount, i;
		
	/* Check our record pointer and file pointer */
	if (ihexRecord == NULL || in == NULL)
		return IHEX_ERROR_INVALID_ARGUMENTS;
		
	if (fgets(recordBuff, IHEX_RECORD_BUFF_SIZE, in) == NULL) {
			/* In case we hit EOF, don't report a file error */
			if (feof(in) != 0)
				return IHEX_ERROR_EOF;
			else
				return IHEX_ERROR_FILE;
	}
	/* Null-terminate the string at the first sign of a \r or \n */
	for (i = 0; i < (int)strlen(recordBuff); i++) {
		if (recordBuff[i] == '\r' || recordBuff[i] == '\n') {
			recordBuff[i] = 0;
			break;
		}
	}


	/* Check if we hit a newline */
	if (strlen(recordBuff) == 0)
		return IHEX_ERROR_NEWLINE;
	
	/* Size check for start code, count, addess, and type fields */
	if (strlen(recordBuff) < (unsigned int)(1+IHEX_COUNT_LEN+IHEX_ADDRESS_LEN+IHEX_TYPE_LEN))
		return IHEX_ERROR_INVALID_RECORD;
		
	/* Check the for colon start code */
	if (recordBuff[IHEX_START_CODE_OFFSET] != IHEX_START_CODE)
		return IHEX_ERROR_INVALID_RECORD;
	
	/* Copy the ASCII hex encoding of the count field into hexBuff, convert it to a usable integer */
	strncpy(hexBuff, recordBuff+IHEX_COUNT_OFFSET, IHEX_COUNT_LEN);
	hexBuff[IHEX_COUNT_LEN] = 0;
	dataCount = strtol(hexBuff, (char **)NULL, 16);
	
	/* Copy the ASCII hex encoding of the address field into hexBuff, convert it to a usable integer */
	strncpy(hexBuff, recordBuff+IHEX_ADDRESS_OFFSET, IHEX_ADDRESS_LEN);
	hexBuff[IHEX_ADDRESS_LEN] = 0;
	ihexRecord->address = strtol(hexBuff, (char **)NULL, 16);
	
	/* Copy the ASCII hex encoding of the address field into hexBuff, convert it to a usable integer */
	strncpy(hexBuff, recordBuff+IHEX_TYPE_OFFSET, IHEX_TYPE_LEN);
	hexBuff[IHEX_TYPE_LEN] = 0;
	ihexRecord->type = strtol(hexBuff, (char **)NULL, 16);
	
	/* Size check for start code, count, address, type, data and checksum fields */
	if (strlen(recordBuff) < (unsigned int)(1+IHEX_COUNT_LEN+IHEX_ADDRESS_LEN+IHEX_TYPE_LEN+dataCount*2+IHEX_CHECKSUM_LEN))
		return IHEX_ERROR_INVALID_RECORD;
	
	/* Loop through each ASCII hex byte of the data field, pull it out into hexBuff,
	 * convert it and store the result in the data buffer of the Intel HEX8 record */
	for (i = 0; i < dataCount; i++) {
		/* Times two i because every byte is represented by two ASCII hex characters */
		strncpy(hexBuff, recordBuff+IHEX_DATA_OFFSET+2*i, IHEX_ASCII_HEX_BYTE_LEN);
		hexBuff[IHEX_ASCII_HEX_BYTE_LEN] = 0;
		ihexRecord->data[i] = strtol(hexBuff, (char **)NULL, 16);
	}
	ihexRecord->dataLen = dataCount;	
	
	/* Copy the ASCII hex encoding of the checksum field into hexBuff, convert it to a usable integer */
	strncpy(hexBuff, recordBuff+IHEX_DATA_OFFSET+dataCount*2, IHEX_CHECKSUM_LEN);
	hexBuff[IHEX_CHECKSUM_LEN] = 0;
	ihexRecord->checksum = strtol(hexBuff, (char **)NULL, 16);

	if (ihexRecord->checksum != Checksum_IHexRecord(ihexRecord))
		return IHEX_ERROR_INVALID_RECORD;
	
	return IHEX_OK;
}

/* Utility function to write an Intel HEX8 record to a file */
int Write_IHexRecord(const IHexRecord *ihexRecord, FILE *out) {
	int i;
	
	/* Check our record pointer and file pointer */
	if (ihexRecord == NULL || out == NULL)
		return IHEX_ERROR_INVALID_ARGUMENTS;
		
	/* Check that the data length is in range */
	if (ihexRecord->dataLen > IHEX_MAX_DATA_LEN/2)
		return IHEX_ERROR_INVALID_RECORD;
	
	/* Write the start code, data count, address, and type fields */
	if (fprintf(out, "%c%2.2X%2.4X%2.2X", IHEX_START_CODE, ihexRecord->dataLen, ihexRecord->address, ihexRecord->type) < 0)
		return IHEX_ERROR_FILE;
		
	/* Write the data bytes */
	for (i = 0; i < ihexRecord->dataLen; i++) {
		if (fprintf(out, "%2.2X", ihexRecord->data[i]) < 0)
			return IHEX_ERROR_FILE;
	}
	
	/* Calculate and write the checksum field */
	if (fprintf(out, "%2.2X\r\n", Checksum_IHexRecord(ihexRecord)) < 0)
		return IHEX_ERROR_FILE;
		
	return IHEX_OK;
}

/* Utility function to print the information stored in an Intel HEX8 record */
void Print_IHexRecord(const IHexRecord *ihexRecord) {
	int i;
	printf("Intel HEX8 Record Type: \t%d\n", ihexRecord->type);
	printf("Intel HEX8 Record Address: \t0x%2.4X\n", ihexRecord->address);
	printf("Intel HEX8 Record Data: \t{");
	for (i = 0; i < ihexRecord->dataLen; i++) {
		if (i+1 < ihexRecord->dataLen)
			printf("0x%02X, ", ihexRecord->data[i]);
		else
			printf("0x%02X", ihexRecord->data[i]);
	}
	printf("}\n");
	printf("Intel HEX8 Record Checksum: \t0x%2.2X\n", ihexRecord->checksum);
}

/* Utility function to calculate the checksum of an Intel HEX8 record */
uint8_t Checksum_IHexRecord(const IHexRecord *ihexRecord) {
	uint8_t checksum;
	int i;

	/* Add the data count, type, address, and data bytes together */
	checksum = ihexRecord->dataLen;
	checksum += ihexRecord->type;
	checksum += (uint8_t)ihexRecord->address;
	checksum += (uint8_t)((ihexRecord->address & 0xFF00)>>8);
	for (i = 0; i < ihexRecord->dataLen; i++)
		checksum += ihexRecord->data[i];
	
	/* Two's complement on checksum */
	checksum = ~checksum + 1;

	return checksum;
}

