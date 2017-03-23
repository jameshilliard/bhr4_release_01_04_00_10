/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#include "config.h"
#include "schedule.h"
#include "mode.h"

/************************************************/
/* naplanovanie vykonania programu v danom case */
/* pri aktualnej konfiguracii                   */
/* FIXME: Translate me!                         */
/************************************************/
int at_schedule(void)
{
  int tfd;
  char *p, *op;
  char pcmd[10*PATH_MAX];
  char cmd[PATH_MAX];
  char tmp[PATH_MAX];
  char tform[3] = "%s";
#ifdef I_FACE
  bool_t xi_save;
#endif
  strcpy(tmp, "pavuk_schedule.tmp.XXXXXX");
  tfd = tl_mkstemp(tmp);
  if(tfd < 0)
  {
    xperror("tl_mkstemp");
    return -1;
  }

  strcpy(cmd, cfg.sched_cmd ? cfg.sched_cmd : AT_CMD);

  p = cmd;
  op = pcmd;
  *op = '\0';

  while(*p)
  {
    if(*p == '%')
    {
      p++;
      switch (*p)
      {
      case 'f':
        strcat(op, tmp);
        break;
      case 't':
        strftime(op, 10, "%H:%M", cfg.time);
        break;
      default:
        tform[1] = *p;
        strftime(op, 10, tform, cfg.time);
      }
      p++;
      while(*op)
        op++;
    }
    else
    {
      *op = *p;
      op++;
      p++;
      *op = '\0';
    }
  }
#ifdef I_FACE
  xi_save = cfg.xi_face;
  cfg.xi_face = FALSE;
#endif
  cfg_dump_cmd_fd(tfd);
#ifdef I_FACE
  cfg.xi_face = xi_save;
#endif

  if(tl_system(pcmd))
  {
    xperror(pcmd);
    if(unlink(tmp))
      xperror(tmp);
    return -1;
  }

  if(unlink(tmp))
    xperror(tmp);
  close(tfd);

  return 0;
}
