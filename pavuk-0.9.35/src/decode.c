/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "config.h"
#include "decode.h"
#include "tools.h"

#ifdef HAVE_ZLIB
#include <zlib.h>
#endif

#define BUFSIZE PATH_MAX

/********************************************************/
/* zapise obsah pamete na adrese buf s dlzkou len       */
/* na deskriptor fd                                     */
/* FIXME: Translate me!                                 */
/********************************************************/
static int writerm(int len, char *buf, int fd)
{
  char *p = buf;
  int fsize = BUFSIZE;


  while(p < (buf + len))
  {
    if((p + BUFSIZE) > (buf + len))
      fsize = buf + len - p;
    if(write(fd, p, fsize) < 0)
    {
      return -1;
    }
    p += BUFSIZE;
  }

  return 0;
}

/********************************************************/
/* vypis subor do deskriptora fd                        */
/* FIXME: Translate me!                                 */
/********************************************************/
static int writerf(char *fname, int fd)
{
  char buf[BUFSIZE];
  int len;
  int fnfd;


  fnfd = open(fname, O_BINARY | O_RDONLY);
  if(fnfd < 0)
  {
    perror(fname);
    return -1;
  }

  while((len = read(fnfd, buf, sizeof(buf))) > 0 || errno == EWOULDBLOCK)
  {
    if(write(fd, buf, len) != len)
    {
      close(fnfd);
      return -1;
    }
  }

  close(fnfd);
  return 0;
}

static char inflate_buffer[1024 * 1024];
int inflate_decode(char *inbuf, int insize, char **outbuf,
  ssize_t *outsize, char *fname)
{
#ifdef HAVE_ZLIB

  int i;
  unsigned long retlen;
  char *ret = 0;
  z_streamp zp;

  /* "deflate" inflation is tricky because there are two possible */
  /* encodings. We try one, if that fails, we try the other. If */
  /* that fails, we give up. */

  retlen = sizeof(inflate_buffer);
  i = uncompress(inflate_buffer, &retlen, inbuf, insize);

  if(i == Z_OK)
  {
    ret = malloc(retlen);
    if(ret == 0)
    {
      fprintf(stderr, "inflate: out of memory\n");
    }
    else
    {
      memcpy(ret, inflate_buffer, retlen);
    }
    *outbuf = ret;
    *outsize = retlen;
    return 0;
  }
  else if(i != Z_DATA_ERROR)
  {
    fprintf(stderr, "inflate: uncompress error %s\n", zError(i));
    return -1;
  }


  zp = alloca(sizeof(z_stream));
  assert(zp);                   /* If alloca fails we're doomed anyway */
  zp->next_in = inbuf;
  zp->next_out = &inflate_buffer[0];
  zp->avail_in = insize;
  zp->avail_out = sizeof(inflate_buffer);
  zp->zalloc = 0;
  zp->zfree = 0;
  zp->opaque = 0;
  *outbuf = 0;
  i = inflateInit(zp);
  if(i != Z_OK)
  {
    fprintf(stderr, "inflate: inflateInit error %s\n", zError(i));
    if(zp->msg)
      fprintf(stderr, "zlib: %s\n", zp->msg);
    return -1;
  }

  if((i = inflate(zp, Z_FINISH)) == Z_STREAM_END)
  {
    ret = malloc(zp->avail_out);
    if(ret == 0)
    {
      fprintf(stderr, "Can't copy inflated file.\n");
    }
    else
    {
      memcpy(ret, zp->next_out, zp->avail_out);
    }
    *outbuf = ret;
    *outsize = zp->avail_out;
    inflateEnd(zp);
    return 0;
  }
  else
  {
    if(i == Z_OK)
    {
      fprintf(stderr, "Whoops, coding problem.\n");
    }
    else
    {
      fprintf(stderr, "inflate: inflate error %s\n", zError(i));
      if(zp->msg)
        fprintf(stderr, "zlib: %s\n", zp->msg);
    }
    inflateEnd(zp);
    return -1;
  }
#else
  fprintf(stderr, "inflate mode not supported.\n");
  return -1;
#endif
}

int gzip_decode(char *inbuf, int insize, char **outbuf,
  ssize_t *outsize, char *fname)
{
  int writepipe[2];
  pid_t pidg;
  char *out = NULL;
  int outcnt = 0, len;
  char buf[BUFSIZE];
  bool_t err = TRUE;
  int outf;

  if(cfg.cache_dir)
  {
    snprintf(buf, sizeof(buf), "%s/pavuk_decoder.tmp.XXXXXX", cfg.cache_dir);
  }
  else
  {
    strncpy(buf, "pavuk_decoder.tmp.XXXXXX", sizeof(buf));
    buf[sizeof(buf) - 1] = '\0';
  }

  outf = tl_mkstemp(buf);
  if(outf == -1)
  {
    xperror("tl_mkstemp");
    return -1;
  }
  unlink(buf);

  *outsize = 0;
  *outbuf = NULL;

  if(pipe(writepipe))
  {
    xperror("pipe");
    return -1;
  }

  pidg = fork();

  if(pidg < 0)
  {
    xperror("fork");
    close(writepipe[0]);
    close(writepipe[1]);
    return -1;
  }
  if(pidg == 0)
  {
    close(0);
    dup(writepipe[0]);
    close(writepipe[0]);
    close(writepipe[1]);

#ifdef HAVE_ZLIB
    {
      gzFile gzfd;
      int ilen, olen;

      if(!(gzfd = gzdopen(0, "rb")))
      {
        perror("gzdopen");
        exit(1);
      }

      while((ilen = gzread(gzfd, buf, sizeof(buf))) > 0)
      {
        olen = write(outf, buf, ilen);

        if(olen != ilen)
          break;
      }
      if(ilen < 0)
      {
        int err;
        printf("decode: %s\n", gzerror(gzfd, &err));
        exit(1);
      }
      else if(ilen)
      {
        perror("decode");
        exit(1);
      }
      gzclose(gzfd);
      close(outf);
      exit(0);
    }
    exit(0);
#else
    close(1);
    dup(outf);
    close(outf);
    execlp(GZIP_CMD, "gunzip", "-cf", NULL);
    xperror("gunzip exec");
    exit(1);
#endif
  }

  close(writepipe[0]);

  err = fname ? writerf(fname, writepipe[1]) :
    writerm(insize, inbuf, writepipe[1]);

  if(err)
    xperror(gettext("decode"));

  close(writepipe[1]);

  waitpid(pidg, NULL, 0);

  if((off_t) - 1 == lseek(outf, 0, SEEK_SET))
  {
    close(outf);
    return -1;
  }

  while((len = read(outf, buf, sizeof(buf))) > 0)
  {
    out = (char *) _realloc(out, outcnt + len + 1);
    memcpy(out + outcnt, buf, len);
    outcnt += len;
    *(out + outcnt) = '\0';
  }

  close(outf);

  *outsize = outcnt;
  *outbuf = out;
  return 0;
}
