/****************************************************************************************
** RaceOS - RUF parser/extractor
** (c) Georg Swoboda, 2011
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*****************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "zlib.h"
#include "ruf.h"
#include "file.h"
#include "ihex.h"

#define CHUNK 16384

/* Compress from file source to file dest until EOF on source.
   def() returns Z_OK on success, Z_MEM_ERROR if memory could not be
   allocated for processing, Z_STREAM_ERROR if an invalid compression
   level is supplied, Z_VERSION_ERROR if the version of zlib.h and the
   version of the library linked do not match, or Z_ERRNO if there is
   an error reading or writing the files. */
int ruf_def(unsigned char *data, unsigned long data_len, FILE *dest, int level)
{
    int ret, flush;
    unsigned have;
    z_stream strm;
    unsigned char out[CHUNK];
    unsigned char *ptr;
    unsigned long ptr_loc;

    /* allocate deflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    ret = deflateInit(&strm, level);
    if (ret != Z_OK)
        return ret;

    ptr=data;
    ptr_loc=0; 
    /* compress until end of file */
    do {
        if ((data_len-ptr_loc) >= CHUNK)
         strm.avail_in = CHUNK;
        else
         strm.avail_in = data_len-ptr_loc;
 //        printf("(ZLIB) data in this chunk= strm.avail_in=%d\n",strm.avail_in);
        if (ptr_loc==data_len)
         flush = Z_FINISH;
        else
         flush = Z_NO_FLUSH;

        strm.next_in = data+ptr_loc;
        ptr_loc+=strm.avail_in; 
//        printf("(ZLIB) data already compressed: %d\n",ptr_loc);

        /* run deflate() on input until output buffer not full, finish
           compression if all of source has been read in */
        do {
            strm.avail_out = CHUNK;
            strm.next_out = out;
            ret = deflate(&strm, flush);    /* no bad return value */
            assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
            have = CHUNK - strm.avail_out;
            if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
                (void)deflateEnd(&strm);
                return Z_ERRNO;
            }
        } while (strm.avail_out == 0);
        assert(strm.avail_in == 0);     /* all input will be used */

        /* done when last data in file processed */
    } while (flush != Z_FINISH);

    assert(ret == Z_STREAM_END);        /* stream will be complete */
    /* clean up and return */
    (void)deflateEnd(&strm);
    return Z_OK;
}


/* Decompress from file source to a *ruf_chunk until stream ends or EOF.
   inf() returns Z_OK on success, Z_MEM_ERROR if memory could not be
   allocated for processing, Z_DATA_ERROR if the deflate data is
   invalid or incomplete, Z_VERSION_ERROR if the version of zlib.h and
   the version of the library linked do not match, or Z_ERRNO if there
   is an error reading or writing the files. */
struct ruf_chunk * ruf_inf(FILE *source)
{
    int ret;
    unsigned have;
    z_stream strm;
    unsigned char in[CHUNK];
    unsigned char out[CHUNK];
    unsigned long total_size=0;


    /* ruf chunk */
    struct ruf_chunk *rc=NULL;

    /* allocate inflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;

    rc=(struct ruf_chunk*)malloc(sizeof(struct ruf_chunk));
    memset(rc, 0, sizeof(struct ruf_chunk));

    ret = inflateInit(&strm);
    if (ret != Z_OK)
    {
      rc->ret=ret;
      return rc;
    }
    
    /* decompress until deflate stream ends or end of file */
    do {
        strm.avail_in = fread(in, 1, CHUNK, source);
        if (ferror(source)) {
            (void)inflateEnd(&strm);
	    rc->ret=Z_ERRNO;
            return rc;
        }
        if (strm.avail_in == 0)
            break;
        strm.next_in = in;

        /* run inflate() on input until output buffer not full */
        do {
            strm.avail_out = CHUNK;
            strm.next_out = out;
            ret = inflate(&strm, Z_NO_FLUSH);
            assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
            switch (ret) {
            case Z_NEED_DICT:
                rc->ret = Z_DATA_ERROR;     /* and fall through */
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                (void)inflateEnd(&strm);
		rc->ret=ret;
                return rc;
            }
            have = CHUNK - strm.avail_out;
            if (have)
	    { 
		rc->data=realloc(rc->data, total_size+have);
                if (!rc->data)	
		{
		   printf("out of memory while allocating inflate buffer\n");
		   (void)inflateEnd(&strm);
		   rc->ret=Z_ERRNO;
		   return rc;
                }
	        memcpy(rc->data+total_size, out, have);
		total_size+=have;
		rc->size=total_size;
	    }
        } while (strm.avail_out == 0);

        /* done when inflate() says it's done */
    } while (ret != Z_STREAM_END);

    /* clean up and return */
    (void)inflateEnd(&strm);
//    rc->ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
    if (ret == Z_STREAM_END)
     rc->ret = Z_OK;
    else
     rc->ret = Z_DATA_ERROR;


    return rc;
}


/*
** report a zlib or i/o error 
*/
void zerr(int ret)
{
    fputs("zpipe: ", stderr);
    switch (ret) {
    case Z_ERRNO:
        if (ferror(stdin))
            fputs("error reading stdin\n", stderr);
        if (ferror(stdout))
            fputs("error writing stdout\n", stderr);
        break;
    case Z_STREAM_ERROR:
        fputs("invalid compression level\n", stderr);
        break;
    case Z_DATA_ERROR:
        fputs("invalid or incomplete deflate data\n", stderr);
        break;
    case Z_MEM_ERROR:
        fputs("out of memory\n", stderr);
        break;
    case Z_VERSION_ERROR:
        fputs("zlib version mismatch!\n", stderr);
    }
}

/*
** create RUF file from 2 binary files (BF0 & BF1 flash)
*/
int ruf_write(char *out_file_name, struct ruf_file * rf)
{
 FILE *out;
 unsigned char padding[RUF_PAD_BYTES];
 unsigned int i;
 unsigned char filler[4];
 unsigned long magic;

 if (!rf)
 {
  printf("(RUF/write) ERROR: no ruf file data provided\n");
  return(0);
 }

 /* open new .RUF file */
 if ((out=fopen(out_file_name, "wb")))
 {
    /* write the .RUF file header */
    fwrite(ruf_hdr, RUF_HDR_SIZE, 1, out);
    if (ruf_def(rf->model, rf->model_size, out, 9)==Z_OK)
    {
      printf("(RUF) Model type successfully compressed\n");
    }
    /* write 4 filler bytes */
    filler[0]=filler[1]=filler[2]=filler[3]=0x0;
    fwrite(filler, 4, 1, out);
 
    if (ruf_def(rf->firmware, rf->firmware_size, out, 9)==Z_OK)
    {
      printf("(RUF) Firmware successfully compressed\n");
    }
    /* write 2 filler bytes */
    filler[0]=0x0;
    filler[1]=0x1A;
    fwrite(filler, 2, 1, out);

    /* compress 4096 padding bytes */
    for (i=0;i<RUF_PAD_BYTES;i++)
     padding[i]=0x0;
    if (ruf_def(padding, RUF_PAD_BYTES, out, 9)==Z_OK)
    {
      printf("(RUF) Padding bytes successfully compressed\n");
    }
    fwrite(ruf_tail, RUF_TAIL_SIZE, 1, out);
    fflush(out);

    /* write the magic bytes to make it usable with RL updater */
    fseek(out, 0x2C, SEEK_SET);
    magic=get_filesize(out_file_name)-118;
    filler[0]=(magic >> 24) & 0xFF;
    filler[1]=(magic >> 16) & 0xFF;
    filler[2]=(magic >> 8) & 0xFF;
    filler[3]=(magic ) & 0xFF;
    fwrite(filler,4,1,out);
    printf("(RUF) Magic Bytes: %08lX\n", magic);
    fflush(out);
    fclose(out);
 }
 return(1);
}

/*
** open RUF file, and return contents
*/
struct ruf_file * ruf_read(char *file_name)
{
 FILE *in;
 unsigned long f_size;
 unsigned long offset=0;
 struct stat s;
 // unsigned char *dest;
 struct ruf_chunk *ruf;
 struct ruf_file  *rf;

 /* check size of firmware file */
 if (stat((char*)file_name, &s) == -1)
 {
  printf("(RUF) ERROR: error opening file: %s\n\n",file_name);
  exit(1);
 }
 f_size=s.st_size;
 printf("(RUF) .RUF file size: %ld bytes\n",f_size);

 /* init ruf structure */
 rf=(struct ruf_file*)malloc(sizeof(struct ruf_file));
 if (!rf)
 {
  printf("(RUF) ERROR: unable to allocate file structure buffer\n");
  exit(1);
 }
 memset(rf, 0, sizeof(struct ruf_file));
 
 /* open firmware file */
 in=fopen((char*)file_name, "r");

 /* avoid end-of-line conversions */
 SET_BINARY_MODE(in);

 /* search for first data chunk (model type)*/ 
 printf("(RUF) Searching for model string ...\n");
 offset=RUF_HDR_SIZE;
 do
 { 
   fseek(in, offset, SEEK_SET);
   ruf=ruf_inf(in); 
   if (ruf->ret==Z_OK)
   {
    rf->model=ruf->data;
    rf->model_size=ruf->size;
    printf("(RUF) Model: '%s' found\n", rf->model);
    break;
   }
   else
   {
    offset++; // try to find compressed data at next offset
   }
 } while (!rf->model_size && offset<f_size); 

 /* search ihex chunk (the firmware) */
 printf("(RUF) Searching for firmware(ihex) string ...\n");
 offset++;
 do
 {
   fseek(in, offset, SEEK_SET);
   ruf=ruf_inf(in);
   if (ruf->ret==Z_OK)
   {
    rf->firmware=ruf->data;
    rf->firmware_size=ruf->size;
    printf("(RUF) Firmware found, size=%ld\n", rf->firmware_size);
    break;
   }
   else
   {
    offset++; // try to find compressed data at next offset
   }
 } while (!rf->firmware_size && offset<f_size);
 
 /* search padding chunk (unknown purpose) */
 offset++;
 printf("(RUF) Searching for 4096 bytes padding string ...\n");
 do
 {
   fseek(in, offset, SEEK_SET);
   ruf=ruf_inf(in);
   if (ruf->ret==Z_OK)
   {
    rf->padding=ruf->data;
    rf->padding_size=ruf->size;
    printf("(RUF) Padding found, size=%ld\n", rf->padding_size);
    if (rf->padding_size != 4096)
    {
     printf("(RUF) ERROR: padding size is supposed to be 4096 bytes\n");
     exit(-1);
    }
    break;
   }
   else
   {
    offset++; // try to find compressed data at next offset
   }
 } while (!rf->padding_size && offset<f_size);

 /* return the result */
 return(rf); 
}
