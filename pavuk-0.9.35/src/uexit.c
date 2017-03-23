/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"

#include <string.h>
#include <stdio.h>
#include <sys/wait.h>
#include "uexit.h"
#include "tools.h"
#include "doc.h"

static int uexit_run_cmd(char *cmd)
{
  int retv;

  retv = tl_system(cmd);


#ifdef WIFEXITED
  if(retv < 0 || !WIFEXITED(retv))
#else
  if(retv < 0 || (retv & 0x00FF) > 127)
#endif
  {
    xperror(gettext("Error occured while executing user-exit script"));
    return -1;
  }
#ifdef WEXITSTATUS
  return WEXITSTATUS(retv);
#else
  return (retv & 0xFF00) >> 8;
#endif
}

int uexit_condition(url * urlp, int *size, time_t date)
{
  char *cmd;
  char pom[256];
  char *urlstr;
  int retv;
  dllist *ptr;

  urlstr = url_to_urlstr(urlp, FALSE);
  cmd = tl_str_concat(NULL, priv_cfg.condition.uexit,
    " -url '", urlstr, "'", NULL);
  _free(urlstr);

  LOCK_URL(urlp);
  for(ptr = urlp->parent_url; ptr; ptr = ptr->next)
  {
    urlstr = url_to_urlstr((url *) ptr->data, FALSE);
    cmd = tl_str_concat(cmd, " -parent '", urlstr, "'", NULL);
    _free(urlstr);
  }
  UNLOCK_URL(urlp);

  sprintf(pom, "-level %hu ", urlp->level);
  cmd = tl_str_concat(cmd, pom, NULL);

  if(size)
  {
    strcat(cmd, "-size ");
    sprintf(pom, "%d ", *size);
    cmd = tl_str_concat(cmd, pom, NULL);
  }

  if(date)
  {
    LOCK_TIME;
    strftime(pom, sizeof(pom), "-date %Y%m%d%H%M%S ", gmtime(&date));
    UNLOCK_TIME;
    cmd = tl_str_concat(cmd, pom, NULL);
  }

  retv = uexit_run_cmd(cmd);
  _free(cmd);

  return retv;
}

int uexit_follow_cmd(doc * docp)
{
  char *cmd;
  char *p;
  int retv;

  p = url_to_urlstr(docp->doc_url, FALSE);
  cmd = tl_str_concat(NULL, priv_cfg.condition.follow_cmd,
    " -url '", p, "'", NULL);
  _free(p);

  p = url_to_in_filename(docp->doc_url);
  cmd = tl_str_concat(cmd, " -infile '", p, "'", NULL);
  _free(p);

  retv = uexit_run_cmd(cmd);
  _free(cmd);

  return retv;
}
