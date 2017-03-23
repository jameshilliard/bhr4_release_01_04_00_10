/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#define _GNU_SOURCE
#define __USE_GNU

#include "config.h"

#include <unistd.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <time.h>
#include <math.h>

#include "url.h"
#include "doc.h"
#include "tools.h"
#include "log.h"
#include "errcode.h"

static void _log_str(char *);

#ifndef HAVE_DPRINTF
#include <stdarg.h>
static int dprintf(int fd, const char *format, ...)
{
  int n;
  va_list ap;
  static char buffer[1024];
  va_start(ap, format);
  n = vsnprintf(buffer, 1024, format, ap);
  va_end(ap);
  return write(fd, buffer, n);
}
#endif

static char *errcodetype(int ecode)
{
  switch (ecode)
  {
  case ERR_NOERROR:
    return "OK";
  case ERR_STORE_DOC:
  case ERR_FILE_OPEN:
  case ERR_DIR_URL:
  case ERR_UNKNOWN:
  case ERR_PROXY_CONNECT:
  case ERR_FTP_UNKNOWN:
  case ERR_FTP_BUSER:
  case ERR_FTP_BPASS:
  case ERR_HTTP_UNKNOWN:
  case ERR_HTTP_AUTH:
  case ERR_HTTP_PAY:
  case ERR_HTTP_BADRQ:
  case ERR_HTTP_FORB:
  case ERR_HTTP_SERV:
  case ERR_GOPHER_UNKNOWN:
    return "FATAL";
  case ERR_LOCKED:
  case ERR_BIGGER:
  case ERR_NOMIMET:
  case ERR_BREAK:
  case ERR_OUTTIME:
  case ERR_SCRIPT_DISABLED:
  case ERR_SMALLER:
  case ERR_ZERO_SIZE:
  case ERR_PROCESSED:
  case ERR_UDISABLED:
  case ERR_RDISABLED:
  case ERR_FTP_NOREGET:
  case ERR_FTP_ACTUAL:
  case ERR_FTP_NOTRANSFER:
  case ERR_FTP_NOMDTM:
  case ERR_FTP_DIRNO:
  case ERR_HTTP_NOREGET:
  case ERR_HTTP_REDIR:
  case ERR_HTTP_ACTUAL:
    return "WARN";
  case ERR_READ:
  case ERR_FTP_BDIR:
  case ERR_FTP_CONNECT:
  case ERR_FTP_DATACON:
  case ERR_FTP_GET:
  case ERR_FTP_NODIR:
  case ERR_FTP_TRUNC:
  case ERR_HTTP_CONNECT:
  case ERR_HTTP_SNDREQ:
  case ERR_HTTP_TRUNC:
  case ERR_HTTP_CYCLIC:
  case ERR_HTTP_NFOUND:
  case ERR_GOPHER_CONNECT:
  case ERR_HTTPS_CONNECT:
    return "ERR";
  default:
    return "UNKNOWN";
  }
}

void short_log(doc * docp, url * urlp)
{
  int fd;
  char pom[1024];
  char *p, *p1;
  time_t t = time(NULL);

  if(!cfg.short_logfile)
    return;
  LOCK_SLOG;
  fd =
    open(cfg.short_logfile, O_BINARY | O_CREAT | O_APPEND | O_WRONLY,
    S_IRUSR | S_IWUSR);

  if(fd < 0)
  {
    xperror("shortlog");
    UNLOCK_SLOG;
    return;
  }

  if(_flock(fd, cfg.short_logfile, O_BINARY | O_CREAT | O_APPEND | O_WRONLY,
      TRUE))
  {
    close(fd);
    UNLOCK_SLOG;
    return;
  }

  p1 = ctime(&t);
  p = strchr(p1, '\n');
  if(p)
    *p = '\0';

  snprintf(pom, sizeof(pom), "%d %s %d/%ld %s %d ", (int) getpid(), p1,
    docp->doc_nr, cfg.total_cnt, errcodetype(docp->errcode), docp->errcode);
  write(fd, pom, strlen(pom));

  p = url_to_urlstr(urlp, FALSE);
  write(fd, p, strlen(p));
  _free(p);

  if(urlp->parent_url)
  {
    write(fd, " ", 1);
    LOCK_URL(urlp);
    p = url_to_urlstr((url *) urlp->parent_url->data, FALSE);
    UNLOCK_URL(urlp);
    write(fd, p, strlen(p));
    _free(p);
  }
  else
  {
    write(fd, " [none]", 7);
  }

  p = url_to_filename(urlp, FALSE);

  write(fd, " ", 1);
  write(fd, p, strlen(p));

  sprintf(pom, " %ld", (long) docp->size);
  write(fd, pom, strlen(pom));

  t = doc_etime(docp, FALSE);
  sprintf(pom, " %ld.%03ld", t / 1000, t % 1000);
  write(fd, pom, strlen(pom));

  if(docp->mime)
  {
    int l = strcspn(docp->mime, "\r\n");
    write(fd, " ", 1);
    write(fd, docp->mime, l);
  }

  write(fd, "\n", 1);

  _funlock(fd);
  close(fd);
  UNLOCK_SLOG;
}

static int time_relative_object(void)
{
  return cfg.time_relative && !strcmp(cfg.time_relative, "object");
}

static int time_relative_program(void)
{
  return cfg.time_relative && !strcmp(cfg.time_relative, "program");
}

static int log_num(int fd, const char *name, int width, long num)
{
  int rv = -1;
  /* we print space before the number, but decrease
   * width to ensure there is always at least
   * one space before the number
   */
  rv = dprintf(fd, " %*ld", width - 1, num);
  return rv;
}

static int log_time(int fd, const char *name, int width, doc * docp,
  const struct timeval *end, const struct timeval *begin)
{
  long time_diff = -1;
  const struct timeval *relative = begin;

  if(time_relative_object())
  {
    relative = &docp->hr_start_time;
  }
  else if(time_relative_program())
  {
    relative = &cfg.hr_start_time;
  }

  if(timerisset(end) && timerisset(begin) && timerisset(relative))
  {
    time_diff = (end->tv_sec - relative->tv_sec) * 1000
      + (end->tv_usec - relative->tv_usec) / 1000.0;
    return log_num(fd, name, width, time_diff);
  }
  else
  {
    if(cfg.sdemo_mode)
    {
      return log_num(fd, name, width, time_diff);
    }
    else
    {
      return dprintf(fd, " %*s", width - 1, "*");
    }
  }

}

/*** Logs timings information. Modelled after short_log() above ***/
void time_log(doc * docp)
{
  int fd = 1;
  char *p;
  char pom[1024] = "\0";
  static int header_printed = 0;

  /* The following are not enough in all cases, but the space
   * is just too precious in common case to care
   */
  int time_width = 6;
  const int size_width = 8;
  const int result_width = strlen(" HTTP/1.1 302 Moved Temporarily");


  if(!cfg.time_logfile)
  {
    return;
  }
  LOCK_TLOG;
  if(strcmp(cfg.time_logfile, "-"))
  {
    fd = open(cfg.time_logfile,
      O_BINARY | O_CREAT | O_APPEND | O_WRONLY, S_IRUSR | S_IWUSR);

    if(fd < 0)
    {
      xperror("timelog");
      UNLOCK_TLOG;
      return;
    }

    if(_flock(fd, cfg.time_logfile,
        O_BINARY | O_CREAT | O_APPEND | O_WRONLY, TRUE))
    {
      close(fd);
      UNLOCK_TLOG;
      return;
    }
  }

  if(time_relative_object())
  {
    time_width = 6;
  }
  else if(time_relative_program())
  {
    time_width = 7;
  }

  /* Log file is locked. No race here */
  if(!header_printed)
  {
    dprintf(fd, "%*s", time_width, "START");
    dprintf(fd, "%*s", time_width, "END");
    dprintf(fd, "%*s", time_width, "DNS");
    dprintf(fd, "%*s", time_width, "CONN");
    dprintf(fd, "%*s", time_width, "FB");
    dprintf(fd, "%*s", time_width, "LB");
    dprintf(fd, "%*s", time_width, "TOTAL");
    dprintf(fd, "%*s", size_width, "SIZE");
    dprintf(fd, " %-*s", result_width - 1, "RESULT");

    dprintf(fd, " %s\n", "URL");

    header_printed = 1;
  }

  log_time(fd, "START", time_width, docp,
    &docp->hr_start_time, &cfg.hr_start_time);
  log_time(fd, "END", time_width, docp, &docp->end_time, &cfg.hr_start_time);
  log_time(fd, "DNS", time_width, docp,
    &docp->dns_time, &docp->hr_start_time);
  log_time(fd, "CONN", time_width, docp,
    &docp->connect_time, &docp->dns_time);
  log_time(fd, "FB", time_width, docp,
    &docp->first_byte_time, &docp->connect_time);
  log_time(fd, "LB", time_width, docp,
    &docp->end_time, &docp->first_byte_time);
  log_time(fd, "TOTAL", time_width, docp,
    &docp->end_time, &docp->hr_start_time);

  log_num(fd, "SIZE", size_width, docp->size);

  if(docp->mime)
  {
    int len = strcspn(docp->mime, "\r\n");
    if(len >= result_width)
      len = result_width - 1;
    if(len)
    {
      strncpy(pom, docp->mime, len); /* FIXME: Security */
      *(pom + len) = '\0';
    }
    else
    {
      strcat(pom, errcodetype(docp->errcode)); /* FIXME: Security */
    }
    dprintf(fd, " %-*s", result_width - 1, pom);
  }
  else
  {
    dprintf(fd, " %-*s", result_width - 1, errcodetype(docp->errcode));
  }

  p = url_to_urlstr(docp->doc_url, FALSE);
  dprintf(fd, " %s\n", p);
  _free(p);


  if(fd != 1)
  {
    _funlock(fd);
    close(fd);
  }
  UNLOCK_TLOG;
}

static int log_fd = -1;

int log_start(char *filename)
{
  static char *log_filename = NULL;
  bool_t start_log = FALSE;
  bool_t stop_log = FALSE;

  LOCK_LOG;
  if(!filename)
    stop_log = TRUE;
  else
  {
    if(log_filename)
    {
      if(strcmp(log_filename, filename))
      {
        start_log = TRUE;
        stop_log = TRUE;
      }
    }
    else
      start_log = TRUE;
  }

  if(stop_log)
  {
    if(log_fd >= 0)
    {
      char pom[1024];
      time_t t = time(NULL);
      LOCK_TIME;
      strftime(pom, sizeof(pom),
        gettext("Ending log : %H:%M:%S %d.%m.%Y\n"), localtime(&t));
      UNLOCK_TIME;
      _log_str(pom);
      _funlock(log_fd);
      close(log_fd);
    }
  }
  if(start_log)
  {
    int nr = 0;
    char nfn[PATH_MAX];

    strncpy(nfn, filename, sizeof(nfn));
    nfn[sizeof(nfn) - 1] = '\0';

    while(TRUE)
    {
      log_fd = open(nfn, O_BINARY | O_CREAT | O_APPEND | O_WRONLY, 0644);

      if(log_fd < 0)
      {
        xperror(nfn);
        xprintf(0, gettext("Unable to open log file - disabling logging\n"));
        break;
      }

      if(_flock(log_fd, nfn, O_BINARY | O_CREAT | O_APPEND | O_WRONLY, FALSE))
      {
        close(log_fd);
        log_fd = -1;
        xprintf(0, gettext("Log file is locked by another process - "));
        if(cfg.gen_logname)
        {
          snprintf(nfn, sizeof(nfn), "%s.%04d", filename, nr);
          xprintf(0, gettext("generating new log filename\n"));
          nr++;
        }
        else
        {
          xprintf(0, gettext("disabling logging\n"));
          break;
        }
      }
      else
      {
        break;
      }
    }
    if(nr > 0)
    {
      filename = nfn;
      _free(cfg.logfile);
      cfg.logfile = tl_strdup(nfn);
    }

    if(log_fd >= 0)
    {
      char pom[1024];
      time_t t = time(NULL);
      LOCK_TIME;
      strftime(pom, sizeof(pom),
        gettext("Starting log : %H:%M:%S %d.%m.%Y\n"), localtime(&t));
      UNLOCK_TIME;
      _log_str(pom);
    }

  }
  _free(log_filename);
  log_filename = tl_strdup(filename);
  UNLOCK_LOG;
  return 0;
}

static void _log_str(char *str)
{
  if(log_fd >= 0)
    write(log_fd, str, strlen(str));
}

void log_str(char *str)
{
  LOCK_LOG;
  _log_str(str);
  UNLOCK_LOG;
}
