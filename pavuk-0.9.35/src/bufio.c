/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "config.h"
#include "bufio.h"
#include "tools.h"
#include "absio.h"
#include "myssl.h"

#ifndef MAX
#define MAX(x,y) ((x) > (y) ? (x) : (y))
#endif

#define BUFIO_BUF_SIZE  4096

static int _bufio_tl_write(bufio * desc, char *buf, size_t size)
{
  if(desc->is_sock)
    return tl_sock_write(desc->fd, buf, size, desc->ssl_hook_data);
  else
    return tl_file_write(desc->fd, buf, size);
}

static int _bufio_tl_read(bufio * desc, char *buf, size_t size)
{
  if(desc->is_sock)
    return tl_sock_read(desc->fd, buf, size, desc->ssl_hook_data);
  else
    return tl_file_read(desc->fd, buf, size);
}

static bufio *bufio_new(void)
{
  bufio *retv;

  retv = _malloc(sizeof(bufio));
  retv->buf = _malloc(BUFIO_BUF_SIZE);
  retv->fd = -1;
  retv->buf_size = BUFIO_BUF_SIZE;
  retv->buf_start = 0;
  retv->buf_end = 0;
  retv->bufio_errno = 0;
  retv->eof = 0;
  retv->is_sock = 0;
  retv->ssl_hook_data = NULL;

  return retv;
}

bufio *bufio_sock_fdopen(int fd)
{
  bufio *retv;

  retv = bufio_fdopen(fd);

  if(retv)
    retv->is_sock = 1;

  return retv;
}

bufio *bufio_fdopen(int fd)
{
  bufio *retv;

  if(fd < 0)
    return NULL;

  retv = bufio_new();
  retv->fd = fd;

  return retv;
}

bufio *bufio_dupfd(int fd)
{
  int cfd;

  if(fd < 0)
    return NULL;

  cfd = dup(fd);

  return bufio_fdopen(cfd);
}

bufio *bufio_open(const char *filename, int flags)
{
  int fd;

  fd = open(filename, flags);

  if(fd < 0)
    return NULL;

  return bufio_fdopen(fd);
}

bufio *bufio_copen(const char *filename, int flags, int mask)
{
  int fd;

  fd = open(filename, flags, mask);

  if(fd < 0)
    return NULL;

  return bufio_fdopen(fd);
}

bufio *bufio_new_sslcon(bufio * sock, void *con)
{
  bufio *retv;

  retv = bufio_new();
  retv->ssl_hook_data = con;
  retv->is_sock = 1;
  retv->fd = bufio_getfd(sock);

  return retv;
}

void bufio_free(bufio * desc)
{
  _free(desc->buf);
  _free(desc);
}

int bufio_close(bufio * desc)
{
  int rv = 0;

#ifdef USE_SSL
  if(desc->ssl_hook_data)
  {
    my_ssl_connection_destroy(desc->ssl_hook_data);
    desc->ssl_hook_data = NULL;
    desc->fd = -1;
  }
#endif


  if(desc->fd >= 0)
  {
    if(desc->is_sock)
    {
      shutdown(desc->fd, 2);
#ifdef __BEOS__
      rv = closesocket(desc->fd);
#else
      rv = close(desc->fd);
#endif
    }
    else
      rv = close(desc->fd);
  }

  desc->fd = -1;

  _free(desc->buf);
  _free(desc);

  return rv;
}

int bufio_write(bufio * desc, char *buf, size_t size)
{
  return _bufio_tl_write(desc, buf, size);
}

int bufio_read(bufio * desc, char *buf, size_t size)
{
  int read_sz = 0;
  int read_act;
  int miss_size = size;
  int acnt;

  if(desc->eof)
  {
    return 0;
  }

  if(desc->bufio_errno)
  {
    errno = desc->bufio_errno;
    return -1;
  }

  for(;;)
  {
    if(desc->buf_start < desc->buf_end)
    {
      read_act = miss_size < desc->buf_end - desc->buf_start ?
        miss_size : desc->buf_end - desc->buf_start;

      memcpy(buf + read_sz, desc->buf + desc->buf_start, read_act);

      desc->buf_start += read_act;
      miss_size -= read_act;
      read_sz += read_act;

      if(read_sz == size)
        return read_sz;
    }
    else
    {
      desc->buf_start = 0;
      desc->buf_end = 0;
      acnt = _bufio_tl_read(desc, desc->buf, desc->buf_size);

      if(acnt <= 0)
      {
        if(acnt == 0)
          desc->eof = 1;
        if(read_sz)
        {
          desc->bufio_errno = errno;
          return read_sz;
        }
        else
          return acnt;
      }

      desc->buf_end = acnt;
    }
  }
}

int bufio_nbfread(bufio * desc, char *buf, size_t size)
{
  int read_sz;

  if(desc->eof)
  {
    return 0;
  }
  if(desc->bufio_errno)
  {
    errno = desc->bufio_errno;
    return -1;
  }

  if(desc->buf_start < desc->buf_end)
  {
    read_sz = size < desc->buf_end - desc->buf_start ?
      size : desc->buf_end - desc->buf_start;

    memcpy(buf, desc->buf + desc->buf_start, read_sz);

    desc->buf_start += read_sz;

    return read_sz;
  }
  else
  {
    desc->buf_start = 0;
    desc->buf_end = 0;
    read_sz = _bufio_tl_read(desc, buf, size);

    if(read_sz <= 0)
    {
      if(read_sz == 0)
        desc->eof = 1;
      if(read_sz < 0)
        desc->bufio_errno = errno;
    }
    return read_sz;
  }
}

int bufio_readln(bufio * desc, char *buf, size_t size)
{
  int read_sz = 0;
  int read_act;
  int miss_size = size - 1;
  int acnt;
  char *mc = NULL;

  if(desc->eof)
  {
    return 0;
  }

  if(desc->bufio_errno)
  {
    errno = desc->bufio_errno;
    return -1;
  }

  size--;

  for(;;)
  {
    if(desc->buf_start < desc->buf_end)
    {
      mc = memchr(desc->buf + desc->buf_start, '\n',
        desc->buf_end - desc->buf_start);

      if(mc && (mc - (desc->buf + desc->buf_start) < miss_size))
      {
        read_act = mc + 1 - (desc->buf + desc->buf_start);
      }
      else
        read_act = miss_size < desc->buf_end - desc->buf_start ?
          miss_size : desc->buf_end - desc->buf_start;

      memcpy(buf + read_sz, desc->buf + desc->buf_start, read_act);

      desc->buf_start += read_act;
      miss_size -= read_act;
      read_sz += read_act;

      if(read_sz == size || mc)
      {
        *(buf + read_sz) = '\0';
        return read_sz;
      }
    }
    else
    {
      desc->buf_start = 0;
      desc->buf_end = 0;
      acnt = _bufio_tl_read(desc, desc->buf, desc->buf_size);

      if(acnt <= 0)
      {
        if(acnt == 0)
          desc->eof = 1;
        if(read_sz)
        {
          desc->bufio_errno = errno;
          *(buf + read_sz + 1) = '\0';
          return read_sz;
        }
        else
          return acnt;
      }

      desc->buf_end = acnt;
    }
  }
}

void bufio_unread(bufio * desc, char *buf, size_t size)
{
  char *p = desc->buf;

  desc->buf_size =
    MAX(desc->buf_size, (size + desc->buf_end - desc->buf_start));

  desc->buf = _malloc(desc->buf_size);

  memmove(desc->buf, buf, size);
  memmove(desc->buf + size, p + desc->buf_start,
    desc->buf_end - desc->buf_start);

  desc->buf_end = size + desc->buf_end - desc->buf_start;
  desc->buf_start = 0;

  _free(p);
}

void bufio_reset(bufio * desc)
{
  desc->buf_start = 0;
  desc->buf_end = 0;
}
