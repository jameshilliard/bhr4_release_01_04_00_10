/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#include "config.h"
#include "condition.h"
#include "mime.h"
#include "robots.h"
#include "url.h"
#include "tools.h"
#include "doc.h"
#include "abstract.h"
#include "tools.h"
#include "mode.h"
#include "times.h"
#include "errcode.h"
#include "gcinfo.h"
#include "gui_api.h"

#ifdef HAVE_MT
#define LOCK_ROBOTS_ENTRY(ent)  mt_pthread_mutex_lock(&(ent)->lock, "robots entry")
#define UNLOCK_ROBOTS_ENTRY(ent) mt_pthread_mutex_unlock(&(ent)->lock, "robots entry")
#else
#define LOCK_ROBOTS_ENTRY(ent)
#define UNLOCK_ROBOTS_ENTRY(ent)
#endif

static char *get_robots(url * urlp);
static void parse_robots(char *, char *, char ***, char ***);

static robotlim **robots = NULL;

static char *get_max_match(char *str, char **pat)
{
  char *rv = NULL;
  int maxlen = -1;

  while(pat && *pat)
  {
    if(!strncmp(*pat, str, strlen(*pat)))
    {
      int len = strlen(*pat);

      if(len > maxlen)
      {
        rv = *pat;
        maxlen = len;
      }
    }
    pat++;
  }
  return rv;
}

/***************************************************/
/* kontrola ci URL splna podmienky pre WWW robotov */
/* FIXME: Translate me!                            */
/***************************************************/
bool_t robots_check(url * urlp)
{
  char *pom, *mdp, *map;
  int i = 0;
  int rv = TRUE;
  robotlim *tmpr = NULL;
  int dont_have = TRUE;

  if((urlp->type != URLT_HTTP && urlp->type != URLT_HTTPS)
    || !cfg.condition.allow_robots)
    return TRUE;

  LOCK_ROBOTS;
  if(robots)
  {
    while(robots[i] && !(!strcmp(robots[i]->site, urlp->p.http.host) &&
        (robots[i]->port == urlp->p.http.port)))
      i++;
    if(robots[i])
      dont_have = FALSE;
  }

  if(dont_have)
  {
    tmpr = (robotlim *) _malloc(sizeof(robotlim));
    tmpr->site = new_string(urlp->p.http.host);
    tmpr->port = urlp->p.http.port;
    tmpr->dpat = NULL;
    tmpr->apat = NULL;
#ifdef HAVE_MT
    pthread_mutex_init(&tmpr->lock, NULL);
#endif

    robots = (robotlim **) _realloc(robots, (i + 2) * sizeof(robotlim *));
    robots[i] = tmpr;
    robots[i + 1] = NULL;

    /* here is possible to cros enter/leave to critical   */
    /* sections because no chance that anyone else holds  */
    /* lock on robots[i]->lock when it is freshly created */
    LOCK_ROBOTS_ENTRY(robots[i]);
  }
  UNLOCK_ROBOTS;

  if(dont_have)
  {

    pom = get_robots(urlp);
    if(pom)
    {
      parse_robots("pavuk", pom, &tmpr->dpat, &tmpr->apat);
      _free(pom);
    }
  }
  else
  {
    LOCK_ROBOTS_ENTRY(robots[i]);
  }

  mdp = get_max_match(urlp->p.http.document, robots[i]->dpat);
  map = get_max_match(urlp->p.http.document, robots[i]->apat);
  UNLOCK_ROBOTS_ENTRY(robots[i]);

  if(map && mdp && (strlen(map) >= strlen(mdp)))
    rv = TRUE;
  else if(mdp)
    rv = FALSE;

  return rv;
}

/************************************************/
/* prenos suboru "robots.txt" pre dane URL      */
/* FIXME: Translate me!                         */
/************************************************/
static char *get_robots(url * urlp)
{
  url *purl = _malloc(sizeof(url));
  doc docu;
  int rstat;
  char *ret = NULL;
  char *pom;
  int nredir = 0, nreget = 0;
  struct stat estat;
  char *pp;
  int f;
  global_connection_info con_info;

#ifdef I_FACE
  if(cfg.xi_face)
  {
    gui_set_status(gettext("transfering \"robots.txt\""));
  }
#endif
  xprintf(1, gettext("transfering \"robots.txt\"\n"));

  memset(purl, '\0', sizeof(url));
  purl->type = urlp->type;
  purl->parent_url = NULL;
  purl->status = URL_INLINE_OBJ; /*** required if -store_name option used ***/
  purl->extension = NULL;
  purl->local_name = NULL;

#ifdef HAVE_MT
  pthread_mutex_init(&purl->lock, NULL);
#endif

#ifdef WITH_TREE
#ifdef I_FACE
  purl->prop = NULL;
  purl->tree_nfo = NULL;
#endif
#endif

  purl->level = 0;
  purl->p.http.user = new_string(urlp->p.http.user);
  purl->p.http.password = new_string(urlp->p.http.password);
  purl->p.http.host = new_string(urlp->p.http.host);
  purl->p.http.port = urlp->p.http.port;
  purl->p.http.document = new_string("/robots.txt");
  purl->p.http.anchor_name = NULL;
  purl->p.http.searchstr = NULL;

  doc_init(&docu, purl);
  docu.is_robot = TRUE;
  docu.save_online = FALSE;
  docu.report_size = FALSE;
  docu.check_limits = FALSE;

  if(cfg.mode == MODE_SYNC || cfg.mode == MODE_MIRROR)
  {
    pp = url_to_filename(purl, TRUE);
    if(!stat(pp, &estat) && !S_ISDIR(estat.st_mode))
    {
      docu.dtime = estat.st_mtime;
    }
  }

  init_global_connection_data(&con_info);

  while((rstat = doc_download(&docu, TRUE, FALSE)) &&
    ((nredir < cfg.nredir && docu.errcode == ERR_HTTP_REDIR) ||
      (nreget < cfg.nreget && docu.errcode == ERR_HTTP_TRUNC)))
  {
    if(docu.errcode)
      report_error(&docu, "robots.txt");

    save_global_connection_data(&con_info, &docu);

    nredir += docu.errcode == ERR_HTTP_REDIR;
    nreget += docu.errcode == ERR_HTTP_TRUNC;

    if(docu.errcode == ERR_HTTP_REDIR)
    {
      purl = docu.doc_url->moved_to;
      pom = url_to_urlstr(purl, FALSE);
      xprintf(1, gettext("Hmm: redirecting \"robots.txt\" to %s ???\n"), pom);
      _free(pom);
      free_deep_url(docu.doc_url);
      _free(docu.doc_url) docu.doc_url = purl;
    }

    _free(docu.contents);
    _free(docu.mime);
    _free(docu.type_str);

    doc_remove_lock(&docu);

    if(cfg.mode == MODE_SYNC || cfg.mode == MODE_MIRROR)
    {
      pp = url_to_filename(purl, TRUE);
      if(!stat(pp, &estat) && !S_ISDIR(estat.st_mode))
      {
        docu.dtime = estat.st_mtime;
      }
    }
    restore_global_connection_data(&con_info, &docu);
  }

  if(docu.errcode)
    report_error(&docu, "robots.txt");

  save_global_connection_data(&con_info, &docu);
  kill_global_connection_data(&con_info);

  if(!rstat)
  {
    if(cfg.dumpfd < 0)
    {
      doc_store(&docu, TRUE);
    }
    ret = docu.contents;
  }
  else if(docu.errcode == ERR_HTTP_NFOUND || docu.errcode == ERR_HTTP_GONE)
  {
    pp = url_to_filename(purl, TRUE);

    if(cfg.dumpfd < 0)
    {
      if((f =
          open(pp, O_BINARY | O_CREAT | O_TRUNC | O_WRONLY,
            S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR)) > 0)
        close(f);
    }
  }
  else
  {
    _free(docu.contents);
  }

  doc_remove_lock(&docu);

  _free(docu.type_str);
  _free(docu.mime);

  if(purl && purl->moved_to)
  {
    free_deep_url(purl->moved_to);
    free(purl->moved_to);
  }
  if(purl)
  {
    free_deep_url(purl);
    free(purl);
  }
  return ret;
}

/*******************************/
/* analyza suboru "robots.txt" */
/* FIXME: Translate me!        */
/*******************************/

static void parse_robots(char *agent, char *file, char ***dpat, char ***apat)
{
  char *p, *p1, *p2;
  bool_t is_me = FALSE;
  int n_dret = 0, n_aret = 0;
  bool_t last = 1;
  int ilen;

  *apat = NULL;
  *dpat = NULL;

  p = file;
  while(*p)
  {
    ilen = strcspn(p, "\r\n");
    if(*(p + ilen))
      *(p + ilen) = '\0';
    else
      last = 0;

    while(*p == ' ' || *p == '\t')
      p++;

    if(!*p)
    {
      is_me = FALSE;
    }
    else if(!strncasecmp("User-Agent: ", p, 12))
    {
      p2 = p + 12;
      while(*p2 == ' ' || *p2 == '\t')
        p2++;
      p1 = p2 + strlen(p2);
      while(*p1 == ' ' || *p1 == '\t')
      {
        *p1 = '\0';
        p1--;
      }

      if(*p2 == '*')
        is_me = TRUE;
      else if(!strncmp(agent, p2, strlen(agent)))
        is_me = TRUE;
    }
    else if(is_me && !strncasecmp("Disallow: ", p, 10))
    {
      p2 = p + 10;
      while(*p2 == ' ' || *p2 == '\t')
        p2++;
      p1 = p2 + strlen(p2);
      while(*p1 == ' ' || *p1 == '\t')
      {
        *p1 = '\0';
        p1--;
      }

      if(*p2)
      {
        *dpat = (char **) _realloc(*dpat, (n_dret + 2) * sizeof(char *));
        (*dpat)[n_dret + 1] = NULL;
        (*dpat)[n_dret] = new_string(p2);
        n_dret++;
      }
    }
    else if(is_me && !strncasecmp("Allow: ", p, 7))
    {
      p2 = p + 7;
      while(*p2 == ' ' || *p2 == '\t')
        p2++;
      p1 = p2 + strlen(p2);
      while(*p1 == ' ' || *p1 == '\t')
      {
        *p1 = '\0';
        p1--;
      }

      if(*p2)
      {
        *apat = (char **) _realloc(*apat, (n_aret + 2) * sizeof(char *));
        (*apat)[n_aret + 1] = NULL;
        (*apat)[n_aret] = new_string(p2);
        n_aret++;
      }
    }

    p += ilen + last;
    p += strspn(p, "\n\r");
  }
}

void robots_do_cleanup(void)
{
  int i, j;

  for(i = 0; robots && robots[i]; i++)
  {
    _free(robots[i]->site);
    for(j = 0; robots[i]->apat && robots[i]->apat[j]; j++)
      _free(robots[i]->apat[j]);
    _free(robots[i]->apat);
    for(j = 0; robots[i]->dpat && robots[i]->dpat[j]; j++)
      _free(robots[i]->dpat[j]);
    _free(robots[i]->dpat);
#ifdef HAVE_MT
    pthread_mutex_destroy(&(robots[i]->lock));
#endif
    _free(robots[i]);
  }
  _free(robots);
}
