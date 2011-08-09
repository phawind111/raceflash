/****************************************************************************************
** ruf_extract - Firmware Tool for PerformanceBox
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

/* Decompress from file source to file dest until stream ends or EOF.
   inf() returns Z_OK on success, Z_MEM_ERROR if memory could not be
   allocated for processing, Z_DATA_ERROR if the deflate data is
   invalid or incomplete, Z_VERSION_ERROR if the version of zlib.h and
   the version of the library linked do not match, or Z_ERRNO if there
   is an error reading or writing the files. */
int inf(FILE *source, FILE *dest)
{
    int ret;
    unsigned have;
    z_stream strm;
    unsigned char in[CHUNK];
    unsigned char out[CHUNK];

    /* allocate inflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    ret = inflateInit(&strm);
    if (ret != Z_OK)
        return ret;

    /* decompress until deflate stream ends or end of file */
    do {
        strm.avail_in = fread(in, 1, CHUNK, source);
        if (ferror(source)) {
            (void)inflateEnd(&strm);
            return Z_ERRNO;
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
                ret = Z_DATA_ERROR;     /* and fall through */
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                (void)inflateEnd(&strm);
                return ret;
            }
            have = CHUNK - strm.avail_out;
            if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
                (void)inflateEnd(&strm);
                return Z_ERRNO;
            }
        } while (strm.avail_out == 0);

        /* done when inflate() says it's done */
    } while (ret != Z_STREAM_END);

    /* clean up and return */
    (void)inflateEnd(&strm);
    return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
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

void header(void)
{
    printf("\n");
    printf("   /*********************************/\n");
    printf("  /* INFLATED / FIRMWARE EXTRACTOR */\n");
    printf(" /* (c) CyberNet, 2010            */\n");
    printf("/*********************************/\n");
    printf("\n");
}

/* compress or decompress from stdin to stdout */
int main(int argc, char **argv)
{
    int ret;
    unsigned char *filename;
    unsigned char out_filename[255];
    FILE *in,*devnull,*out;
    struct stat s;
    unsigned long f_size;
    unsigned long offset=0;

    // check args
    if (argc!=2)
    { 
      printf("invalid arguments\n");
      printf(" ./ruf_extract <filename.RUF>\n\n");
      return(-1);
    }
    else
    {
      filename=argv[1];
    }


    header();

    /* check size of firmware file */
    if (stat(filename, &s) == -1)
    {
     printf(" error opening file: %s\n\n",filename);
     return(-1);  
    }

    f_size=s.st_size;
    printf(" Firmware size: %ld bytes\n\n",f_size);    

    /* open firmware file */
    in=fopen(filename, "r");
    /* open /dev/null file */
    devnull=fopen("/dev/null", "w");

    /* avoid end-of-line conversions */
    SET_BINARY_MODE(in);
    SET_BINARY_MODE(stdout);


    printf(" Scanning for inflated data ...\n\n");  
    for (offset=0;offset<f_size;offset++)
    { 
      fseek(in, offset, SEEK_SET);
      ret = inf(in, devnull);
      if (ret == Z_OK)
      {
        printf(" Found data at offset: %X\n",offset);
        /* now do it once more, but this time we redirect it to a real file */
        sprintf(out_filename,"chunk_%X.dat",offset);
        out=fopen(out_filename,"wb");	
        if (!out)
        {
          printf("\n Error: unable to open %s for writing\n\n",out_filename);
        }
        else
        { 
	  printf("   > inflating chunk to: %s ",out_filename);
          fseek(in, offset, SEEK_SET);
          ret=inf(in, out);
          if (ret == Z_OK)
           printf(" [OK]\n");
          else
           printf(" [ERR %d]\n",ret);   

          fclose(out);
        }
      }
    }
    fclose(devnull);
    fclose(in);
    printf("\n");
}
