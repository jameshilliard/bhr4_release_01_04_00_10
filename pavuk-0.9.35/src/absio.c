/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#include "absio.h"
#include "myssl.h"
#include "gui_api.h"

int tl_selectr(int sock, int timeout)
{
  fd_set readfds, exceptfds;
  struct timeval tout, *toutptr = NULL;
  int rv;

  FD_ZERO(&readfds);
  FD_SET(sock, &readfds);
  FD_ZERO(&exceptfds);
  FD_SET(sock, &exceptfds);

  if(timeout)
  {
    tout.tv_sec = timeout;
    tout.tv_usec = 0;
    toutptr = &tout;
  }

  rv = select(sock + 1, &readfds, NULL, &exceptfds, toutptr);

  return rv;
}

int tl_selectw(int sock, int timeout)
{
  fd_set writefds, exceptfds;
  struct timeval tout, *toutptr = NULL;
  int rv;

  FD_ZERO(&writefds);
  FD_SET(sock, &writefds);
  FD_ZERO(&exceptfds);
  FD_SET(sock, &exceptfds);

  if(timeout)
  {
    tout.tv_sec = timeout;
    tout.tv_usec = 0;
    toutptr = &tout;
  }

  rv = select(sock + 1, NULL, &writefds, &exceptfds, toutptr);

  return rv;
}

int tl_unix_read(int sock, char *buf, size_t len, void *ssl_con)
{
  int rv = 0;

  do
  {
#ifdef USE_SSL
    if(!ssl_con || (ssl_con && (my_ssl_data_pending(ssl_con) == 0)))
#endif
    {
#if defined I_FACE && !defined HAVE_MT
      if(cfg.xi_face && cfg.done)
      {
        rv = gui_wait_io(sock, TRUE);
      }
      else
#endif
      {
        while((rv = tl_selectr(sock, (int) (cfg.ctimeout))) == -1
          && errno == EINTR);

        if(rv == 0)
        {
          errno = ETIMEDOUT;
          rv = -1;
        }
      }

      if(rv < 0)
        return -1;
    }

    if(cfg.rbreak)
    {
      errno = EINTR;
      rv = -1;
      cfg.stop = TRUE;
      break;
    }

#ifdef USE_SSL
    if(ssl_con)
    {
      rv = my_ssl_read(ssl_con, buf, len);
    }
    else
#endif
      rv = read(sock, buf, len);

  }
  while(rv == -1 && errno == EINTR && !cfg.rbreak);

  return rv;
}

int tl_unix_write(int sock, char *buf, size_t len, void *ssl_con)
{
  int rv = 0;

  do
  {
#if defined I_FACE && !defined HAVE_MT
    if(cfg.xi_face && cfg.done)
    {
      rv = gui_wait_io(sock, FALSE);
    }
    else
#endif
    {

      while((rv = tl_selectw(sock, (int) (cfg.ctimeout))) == -1
        && errno == EINTR);
      if(rv == 0)
      {
        errno = ETIMEDOUT;
        rv = -1;
      }
    }

    if(rv < 0)
      return -1;

    if(cfg.rbreak)
    {
      errno = EINTR;
      rv = -1;
      cfg.stop = TRUE;
      break;
    }

#ifdef USE_SSL
    if(ssl_con)
    {
      rv = my_ssl_write(ssl_con, buf, len);
    }
    else
#endif
      rv = write(sock, buf, len);

  }
  while(rv == -1 && errno == EINTR && !cfg.rbreak);

  return rv;
}

#ifdef __BEOS__
int tl_beos_recv(int sock, char *buf, int len, void *ssl_con)
{
  int rv = 0;

  do
  {
#ifdef USE_SSL
    if(!ssl_con || (ssl_con && (my_ssl_data_pending(ssl_con) == 0)))
#endif
    {
#if defined I_FACE && !defined HAVE_MT
      if(cfg.xi_face && cfg.done)
      {
        rv = gui_wait_io(sock, TRUE);
      }
      else
#endif
      {
        while((rv = tl_selectr(sock, (int) (cfg.ctimeout))) == -1
          && errno == EINTR);

        if(rv == 0)
        {
          errno = ETIMEDOUT;
          rv = -1;
        }
      }

      if(rv < 0)
        return -1;
    }

    if(cfg.rbreak)
    {
      errno = EINTR;
      rv = -1;
      cfg.stop = TRUE;
      break;
    }

#ifdef USE_SSL
    if(ssl_con)
    {
      rv = my_ssl_read(ssl_con, buf, len);
    }
    else
#endif
      rv = recv(sock, buf, len, 0);

  }
  while(rv == -1 && errno == EINTR && !cfg.rbreak);

  return rv;
}

int tl_beos_send(int sock, char *buf, int len, void *ssl_con)
{
  int rv = 0;

  do
  {
#if defined I_FACE && !defined HAVE_MT
    if(cfg.xi_face && cfg.done)
    {
      rv = gui_wait_io(sock, FALSE);
    }
    else
#endif
    {

      while((rv = tl_selectw(sock, (int) (cfg.ctimeout))) == -1
        && errno == EINTR);
      if(rv == 0)
      {
        errno = ETIMEDOUT;
        rv = -1;
      }
    }

    if(rv < 0)
      return -1;

    if(cfg.rbreak)
    {
      errno = EINTR;
      rv = -1;
      cfg.stop = TRUE;
      break;
    }

#ifdef USE_SSL
    if(ssl_con)
    {
      rv = my_ssl_write(ssl_con, buf, len);
    }
    else
#endif
      rv = send(sock, buf, len, 0);

  }
  while(rv == -1 && errno == EINTR && !cfg.rbreak);

  return rv;
}
#endif /* __BEOS__ */
