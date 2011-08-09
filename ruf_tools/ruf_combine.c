/****************************************************************************************
** ruf_combine - Firmware Tool for PerformanceBox
** (c) Georg Swoboda, 2011
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*****************************************************************************************/

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "zlib.h"

#if defined(MSDOS) || defined(OS2) || defined(WIN32) || defined(__CYGWIN__)
#  include <fcntl.h>
#  include <io.h>
#  define SET_BINARY_MODE(file) setmode(fileno(file), O_BINARY)
#else
#  define SET_BINARY_MODE(file)
#endif

#define CHUNK 16384

/* Compress from file source to file dest until EOF on source.
   def() returns Z_OK on success, Z_MEM_ERROR if memory could not be
   allocated for processing, Z_STREAM_ERROR if an invalid compression
   level is supplied, Z_VERSION_ERROR if the version of zlib.h and the
   version of the library linked do not match, or Z_ERRNO if there is
   an error reading or writing the files. */
int def(FILE *source, FILE *dest, int level)
{
    int ret, flush;
    unsigned have;
    z_stream strm;
    unsigned char in[CHUNK];
    unsigned char out[CHUNK];

    /* allocate deflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    ret = deflateInit(&strm, level);
    if (ret != Z_OK)
        return ret;

    /* compress until end of file */
    do {
        strm.avail_in = fread(in, 1, CHUNK, source);
        if (ferror(source)) {
            (void)deflateEnd(&strm);
            return Z_ERRNO;
        }
        flush = feof(source) ? Z_FINISH : Z_NO_FLUSH;
        strm.next_in = in;

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

/* report a zlib or i/o error */
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

void prt_header(void)
{
    printf("\n");
    printf("   /*********************************/\n");
    printf("  /* DEFLATE FIRMWARE COMBINER     */\n");
    printf(" /* (c) CyberNet, 2010            */\n");
    printf("/*********************************/\n");
    printf("\n");
}

/* compress or decompress from stdin to stdout */
int main(int argc, char **argv)
{
    int ret;
    unsigned char *filename;
    unsigned char *header;
    unsigned char *tail;
    unsigned char out_filename[255];
    FILE *in,*devnull,*out;
    FILE *in_header, *in_tail;
    struct stat s;
    unsigned long f_size;
    unsigned long offset=0;
    int c;
    unsigned char fbuf; 
    int fbuf_size;
    unsigned char filler[4]="AAAA";
    unsigned long magic=0;
    // check args
    if (argc<5)
    { 
      printf("invalid arguments\n");
      printf(" ./ruf_combine <filename.RUF> <header> <chunk1> (<chunk2> ...) <tail>\n\n");
      return(-1);
    }
    else
    {
      filename=argv[1];
    }
    prt_header();
    header=argv[2];
    tail=argv[argc-1];

    printf("Header -> %s\n",header); 
    printf("Tail -> %s\n",tail);

    /****************************/ 
    /* write header to filename */
    /****************************/
    in_header=fopen(header, "r");
    /* check size of firmware file */
    if (stat(header, &s) == -1)
    {
     printf(" error opening file: %s\n\n",header);
     return(-1);
    }
    out=fopen(filename, "wb");
    while (!feof(in_header))
    {
      fbuf_size=fread(&fbuf, 1, 1, in_header); 
      fwrite(&fbuf,fbuf_size,1,out);
    } 
    fclose(in_header);

    /*************************************************************/
    /* loop the chunks, compress em, and append to firmware file */
    /*************************************************************/
    for (c=3; c<argc-1; c++)
    {
      printf("Chunk %d -> %s\n",c-3,argv[c]);
      /* check size of firmware file */
      if (stat(argv[c], &s) == -1)
      {
       printf(" error opening file: %s\n\n",argv[c]);
       return(-1);
      }
      in=fopen(argv[c], "rb");
      def(in, out, 9);
      if (c==3) 
      {
        /* these 4 bytes will be filled with the filesize-118 at the end */
        /* used as a checksum for the upgrader.exe to recognize a valid RUF file */
        filler[0]=0x0; 
        filler[1]=0x0;
        filler[2]=0x0;
        filler[3]=0x0;
        fwrite(filler,4,1,out);
      }
      if (c==4)
       {
        filler[0]=0x0;
        filler[1]=0x1A;
        fwrite(filler,2,1,out);
      }
      fclose(in);
    }

    /****************************/
    /* write tail to filename */
    /****************************/
    in_tail=fopen(tail, "r");
    if (stat(tail, &s) == -1)
    {
     printf(" error opening file: %s\n\n",tail);
     return(-1);
    }
    while (!feof(in_tail))
    {
      fbuf_size=fread(&fbuf, 1, 1, in_tail);
      fwrite(&fbuf,fbuf_size,1,out);
    }
    fclose(in_tail);
    fflush(out);
    /***********************************/
    /* check size of firmware file     */
    /* calc size-118 and write to file */ 
    /***********************************/
    if (stat(filename, &s) == -1)
    {
     printf(" error opening file: %s\n\n",filename);
     return(-1);
    }

    f_size=s.st_size;
    magic=f_size-118;
    fseek(out, 0x2C, SEEK_SET);
    filler[0]=(magic >> 24) & 0xFF;
    filler[1]=(magic >> 16) & 0xFF;
    filler[2]=(magic >> 8) & 0xFF;
    filler[3]=(magic ) & 0xFF;
    fwrite(filler,4,1,out);
    printf("Magic Bytes: %04X\n", magic);   
    printf("Firmware size: %ld bytes\n\n",f_size);
    fflush(out);
    fclose(out);
}
