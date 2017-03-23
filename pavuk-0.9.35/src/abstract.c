/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#include "config.h"
#include "doc.h"
#include "file.h"
#include "http.h"
#include "ftp.h"
#include "gopher.h"
#include "url.h"
#include "mode.h"
#include "nscache.h"
#include "iecache.h"
#include "mozcache.h"
#include "errcode.h"
#include "abstract.h"

static void abs_sleep(void)
{
  int st;

  if(cfg.sleep)
  {
    if(cfg.rsleep)
      st = rand() % cfg.sleep;
    else
      st = cfg.sleep;

    xprintf(1, gettext("Suspending download for %d seconds.\n"), st);
    tl_sleep(st);
  }
}

/********************************************************/
/* parameter  -  URL dokumentu        */
/* vracia deskriptor soketu alebo suboru pre dokument */
/* osetrenie vyskytu v lokalnom strome      */
/********************************************************/
bufio *abs_get_data_socket(doc * docp)
{
  char *fn;
  bufio *sock;
  struct stat estat;
  url *urlr = docp->doc_url;

  docp->errcode = ERR_NOERROR;

  urlr->status &= ~URL_REDIRECT;

  if(cfg.mode != MODE_SYNC && cfg.mode != MODE_MIRROR)
  {
    fn = url_to_filename(urlr, TRUE);

    if(!access(fn, R_OK) && urlr->type != URLT_FILE)
    {
      urlr->status |= URL_REDIRECT;
      if(stat(fn, &estat) == 0)
      {
        if(!S_ISDIR(estat.st_mode))
        {
          if(!(sock = bufio_open(fn, O_BINARY | O_RDONLY)))
          {
            xperror(fn);
            docp->errcode = ERR_FILE_OPEN;
          }
          xprintf(1, gettext("File redirect\n"));

          docp->totsz = estat.st_size;
          if(docp->datasock)
          {
            docp->is_persistent = FALSE;
            abs_close_socket(docp, FALSE);
          }
          return sock;
        }
      }
    }
#ifdef HAVE_BDB_18x
    else if(cfg.ns_cache_dir && urlr->type != URLT_FILE)
    {
      char *cfn;
      char *urlstr = url_to_urlstr(urlr, FALSE);

      cfn = ns_cache_find_localname(urlstr);
      _free(urlstr);

      if(cfn)
      {
        sock = bufio_open(cfn, O_BINARY | O_RDONLY);

        if(sock)
        {
          if(docp->datasock)
          {
            docp->is_persistent = FALSE;
            abs_close_socket(docp, FALSE);
          }
          /*!!! clever will be to look at MIME type !!! */
          if(file_is_html(cfn))
            urlr->status |= URL_ISHTML;
          urlr->status |= URL_INNSCACHE;
          xprintf(1, gettext("Loading copy from local NS cache - %s\n"), cfn);
          if(stat(cfn, &estat) == 0)
            docp->totsz = estat.st_size;
          _free(cfn);
          return sock;
        }
        _free(cfn);
      }
    }
    else if(cfg.moz_cache_dir && urlr->type != URLT_FILE)
    {
      char *cfn;
      char *urlstr = url_to_urlstr(urlr, FALSE);

      cfn = moz_cache_find_localname(urlstr);
      _free(urlstr);

      if(cfn)
      {
        sock = bufio_open(cfn, O_BINARY | O_RDONLY);

        if(sock)
        {
          if(docp->datasock)
          {
            docp->is_persistent = FALSE;
            abs_close_socket(docp, FALSE);
          }
          /*!!! clever will be to look at MIME type !!! */
          if(file_is_html(cfn))
            urlr->status |= URL_ISHTML;
          urlr->status |= URL_INNSCACHE;
          xprintf(1, gettext("Loading copy from local Mozilla cache - %s\n"),
            cfn);
          if(stat(cfn, &estat) == 0)
            docp->totsz = estat.st_size;
          _free(cfn);
          return sock;
        }
        _free(cfn);
      }
    }
#endif
#ifdef __CYGWIN__
    else if(cfg.ie_cache && urlr->type != URLT_FILE)
    {
      char *cfn;
      char *urlstr = url_to_urlstr(urlr, FALSE);

      cfn = ie_cache_find_localname(urlstr);
      _free(urlstr);

      if(cfn)
      {
        sock = bufio_open(cfn, O_BINARY | O_RDONLY);

        if(sock)
        {
          if(docp->datasock)
          {
            docp->is_persistent = FALSE;
            abs_close_socket(docp, FALSE);
          }
          if(file_is_html(cfn))
            urlr->status |= URL_ISHTML;
          urlr->status |= URL_INNSCACHE;
          xprintf(1, gettext("Loading copy from local MSIE cache - %s\n"),
            cfn);
          if(stat(cfn, &estat) == 0)
            docp->totsz = estat.st_size;
          _free(cfn);
          return sock;
        }
        _free(cfn);
      }

    }
#endif
  }

  if(docp->is_http_transfer)
  {
    abs_sleep();
    urlr->status &= ~URL_REDIRECT;
    return http_get_data_socket(docp);
  }
  else if(urlr->type == URLT_FTP || urlr->type == URLT_FTPS)
  {
    abs_sleep();
    urlr->status &= ~URL_REDIRECT;
    return ftp_get_data_socket(docp);
  }
  else if(urlr->type == URLT_GOPHER)
  {
    abs_sleep();
    urlr->status &= ~URL_REDIRECT;
    return gopher_get_data_socket(docp);
  }
  else if(urlr->type == URLT_FILE)
  {
    urlr->status &= ~URL_REDIRECT;
    return get_file_data_socket(docp);
  }
  xprintf(1, gettext("Unsupported URL\n"));
  return NULL;
}

static int should_leave_persistent(doc * docp)
{
  return (docp->is_persistent &&
    !(docp->doc_url->status & URL_REDIRECT) &&
    docp->errcode != ERR_STORE_DOC &&
    docp->errcode != ERR_UNKNOWN &&
    docp->errcode != ERR_READ &&
    docp->errcode != ERR_BIGGER &&
    docp->errcode != ERR_NOMIMET &&
    docp->errcode != ERR_BREAK &&
    docp->errcode != ERR_OUTTIME &&
    docp->errcode != ERR_SMALLER &&
    docp->errcode != ERR_LOW_TRANSFER_RATE &&
    docp->errcode != ERR_QUOTA_FILE &&
    docp->errcode != ERR_QUOTA_TRANS &&
    docp->errcode != ERR_QUOTA_FS &&
    docp->errcode != ERR_QUOTA_TIME &&
    docp->errcode != ERR_HTTP_UNKNOWN &&
    docp->errcode != ERR_HTTP_TRUNC &&
    docp->errcode != ERR_HTTP_SNDREQ &&
    docp->errcode != ERR_HTTP_NOREGET &&
    docp->errcode != ERR_HTTP_CLOSURE && docp->errcode != ERR_HTTP_TIMEOUT);
}

/********************************************************/
/* close socket for current document if should    */
/********************************************************/
void abs_close_socket(doc * docp, int read_status)
{
  url *urlr = docp->doc_url;

  if(!docp->datasock)
    return;

  switch (urlr->type)
  {
  case URLT_FILE:
    bufio_close(docp->datasock);
    docp->datasock = NULL;
    break;
  case URLT_HTTP:
  case URLT_HTTPS:
    if(should_leave_persistent(docp))
    {
      DEBUG_NET("Leaving opened persistent HTTP connection\n");
      break;
    }
    else
    {
      bufio_close(docp->datasock);
      docp->datasock = NULL;
    }
    break;
  case URLT_GOPHER:
    if(docp->is_http_transfer && should_leave_persistent(docp))
    {
      DEBUG_NET("Leaving opened persistent HTTP connection\n");
      break;
    }
    else
    {
      bufio_close(docp->datasock);
      docp->datasock = NULL;
    }
    break;
  case URLT_FTP:
  case URLT_FTPS:
    if(docp->is_http_transfer && should_leave_persistent(docp))
    {
      DEBUG_NET("Leaving opened persistent HTTP connection\n");
      break;
    }
    else
    {
      bufio_close(docp->datasock);
      docp->datasock = NULL;
    }

    if(urlr->status & URL_REDIRECT)
      return;

    if(docp->ftp_control && read_status)
    {
      if(ftp_get_response(docp, NULL, FALSE) >= 400)
      {
        xprintf(1, gettext("Warning: broken ftp transfer ...\n"));
        docp->errcode = ERR_FTP_TRUNC;
        docp->ftp_fatal_err = TRUE;
      }
    }

    if((docp->errcode == ERR_NOERROR) && cfg.del_after)
    {
      if(ftp_remove(docp))
        xprintf(1,
          gettext("Error removing FTP document from remote server\n"));
    }

    if(docp->ftp_control && docp->ftp_fatal_err)
    {
      bufio_close(docp->ftp_control);
      docp->ftp_control = NULL;
    }
    break;
  default:
    bufio_close(docp->datasock);
    docp->datasock = NULL;
  }
}

int abs_read(bufio * sock, char *buf, size_t bufsize)
{
  return bufio_nbfread(sock, buf, bufsize);
}

int abs_readln(bufio * sock, char *buf, size_t bufsize)
{
  return bufio_readln(sock, buf, bufsize);
}

int abs_write(bufio * sock, char *buf, size_t bufsize)
{
  return bufio_write(sock, buf, bufsize);
}

int abs_read_data(doc * docp, bufio * sock, char *buf, size_t bufsize)
{
  int rv;

  if(docp->is_http11 && docp->is_chunked)
  {
    char pombuf[1024];

    rv = 0;
    if(docp->read_chunksize)
    {
      char *endp;

      rv = abs_readln(sock, pombuf, sizeof(pombuf) - 1);
      if(rv <= 0)
      {
        xprintf(1,
          gettext
          ("Error reading document with \"chunked\" transfer encoding!\n"));
        rv = -1;
      }
      else
      {
        docp->chunk_size = strtol(pombuf, &endp, 16);
        docp->read_chunksize = FALSE;
        if(docp->chunk_size == 0)
          docp->read_trailer = TRUE;
        rv = 0;
      }
    }
    if(!rv && docp->read_trailer)
    {
      while((rv = abs_readln(sock, pombuf, sizeof(pombuf) - 1)) >= 0)
      {
        if(!rv)
        {
          rv = -1;
          break;
        }
        if(strcspn(pombuf, "\r\n") == 0)
        {
          rv = 0;
          break;
        }
      }
    }
    if(!rv && docp->chunk_size > 0)
    {
      size_t rs;
      rs =
        (bufsize <
        (size_t) docp->chunk_size) ? bufsize : (size_t) docp->chunk_size;
      rv = bufio_nbfread(sock, buf, rs);

      if(rv > 0)
        docp->chunk_size -= rv;

      if(docp->chunk_size == 0)
      {
        abs_readln(sock, pombuf, sizeof(pombuf) - 1);
        docp->read_chunksize = TRUE;
      }
    }
  }
  else if((docp->is_persistent ||
      (cfg.check_size && docp->doc_url->type == URLT_HTTP)) &&
    !(docp->doc_url->status & URL_REDIRECT) && docp->totsz >= 0)
  {
    size_t rs;

    rs = (docp->totsz - docp->rest_pos) - docp->size;
    if(rs > bufsize)
      rs = bufsize;

    if(rs)
    {
      rv = bufio_nbfread(sock, buf, rs);
    }
    else
      rv = 0;
  }
  else
  {
    rv = bufio_nbfread(sock, buf, bufsize);
  }

  return rv;
}
