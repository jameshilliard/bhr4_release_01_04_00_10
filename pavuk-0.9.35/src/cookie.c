/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"

#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <stdio.h>

#include "tools.h"
#include "url.h"
#include "times.h"
#include "cookie.h"
#include "bufio.h"

static int cookies_num = 0;
static cookie_entry *cookies = NULL;
static cookie_entry *last_cookie = NULL;
static int cookie_changed = 0;
static time_t cookie_updated = 0L;
enum CookieFileType { COOKIEFILETYPE_NS = 0, COOKIEFILETYPE_KDE2 = 1 };
enum CookieFileType cookiefiletype = COOKIEFILETYPE_NS;

static void cookie_deep_free(cookie_entry * centry)
{
  _free(centry->domain);
  _free(centry->path);
  _free(centry->name);
  _free(centry->value);
  _free(centry);
}

static void cookie_free_all(cookie_entry * centry)
{
  cookie_entry *pce;

  for(pce = centry; pce; pce = pce->next)
    cookie_deep_free(pce);
}

static cookie_entry *cookie_get_last_entry(cookie_entry * centry, int *num)
{
  if(num)
    *num = 0;

  if(!centry)
    return NULL;

  if(num)
    (*num)++;
  while(centry->next)
  {
    centry = centry->next;
    if(num)
      (*num)++;
  }

  return centry;
}

static void cookie_remove_oldest_entry(cookie_entry ** centry,
  cookie_entry ** lastentry, int *num)
{
  cookie_entry *last = NULL;

  if(*lastentry)
    last = *lastentry;
  else
    last = cookie_get_last_entry(*centry, NULL);

  if(last && last->prev)
  {
    if(*centry == last)
      (*centry) = last->next;
    if(last->prev)
      last->prev->next = last->next;
    if(last->next)
      last->next->prev = last->prev;
    if(*lastentry)
      (*lastentry) = last->prev;
    cookie_deep_free(last);
    if(num)
      (*num)--;
  }
}

static int cookie_match(url * urlp, cookie_entry * cookiep)
{
  int retv = TRUE;
  int cl, hl;
  char *c, *h;

  retv &= (urlp->type == URLT_HTTP) || (urlp->type == URLT_HTTPS);

  if(cookiep->secure)
    retv &= (urlp->type == URLT_HTTPS);

  retv &=
    !strncmp(urlp->p.http.document, cookiep->path, strlen(cookiep->path));

  c = cookiep->domain;
  h = urlp->p.http.host;
  cl = strlen(c);
  hl = strlen(h);

  if(cl == hl+1 && cookiep->domain[0] == '.')
    ++c;
  else if(hl > cl)
    h += hl-cl;

  retv &= !strcasecmp(c, h);

  return retv;
}

static int cookie_eq(cookie_entry * c1, cookie_entry * c2)
{
  return (!strcmp(c1->name, c2->name) && !strcmp(c1->domain, c2->domain) &&
    !strcmp(c1->path, c2->path));
}

static cookie_entry *cookie_parse_ns(char *line)
{
  cookie_entry *centry;
  char *p, *strtokbuf;
  int i;
  char *ln;


  if(!line)
    return NULL;

  ln = tl_strdup(line);

  centry = _malloc(sizeof(cookie_entry));

  centry->loaded = TRUE;
  centry->next = NULL;
  centry->prev = NULL;
  centry->path = NULL;
  centry->domain = NULL;
  centry->name = NULL;
  centry->value = NULL;
  centry->host = NULL;

  p = strtokc_r(ln, '\t', &strtokbuf);

  for(i = 0; i < 7; i++)
  {
    if(!p)
    {
      cookie_deep_free(centry);
      _free(ln);
      return NULL;
    }

    switch (i)
    {
    case 0:
      centry->domain = tl_strdup(p);
      break;
    case 1:
      centry->flag = (bool_t) strcasecmp(p, "FALSE");
      break;
    case 2:
      centry->path = tl_strdup(p);
      break;
    case 3:
      centry->secure = (bool_t) strcasecmp(p, "FALSE");
      break;
    case 4:
      centry->expires = _atoi(p);
      break;
    case 5:
      centry->name = tl_strdup(p);
      break;
    case 6:
      centry->value = tl_strdup(p);
      break;
    }
    p = strtokc_r(NULL, '\t', &strtokbuf);
  }

  _free(ln);
  return centry;
}

static cookie_entry *cookie_parse_kde2(char *line)
{
  cookie_entry *centry;
  int i;
  char *p, *ln, *next;

  if(!line)
    return NULL;

  next = ln = tl_strdup(line);

  centry = _malloc(sizeof(cookie_entry));

  centry->loaded = TRUE;
  centry->next = NULL;
  centry->prev = NULL;
  centry->path = NULL;
  centry->domain = NULL;
  centry->name = NULL;
  centry->value = NULL;
  centry->host = NULL;

  for(i = 0; i < 8 && *next; ++i)
  {
    p = next;
    while(*p == ' ' || *p == '\t')
      ++p; /* skip white space */
    if(*p == '\"')
    {
      next = ++p;
      while(*next && *next != '\"')
        ++next;
    }
    else
    {
      next = p;
      while(*next && *next != ' ' && *next != '\t')
        ++next;
    }
    if(*next)
    {
      *(next++) = 0;
    }
    if(p == next)
    {
      break;
    }

    switch (i)
    {
    case 0:
      centry->host = tl_strdup(p);
      break;
    case 1:
      centry->domain = tl_strdup(p);
      break;
    case 2:
      centry->path = tl_strdup(p);
      break;
    case 3:
      centry->expires = _atoi(p);
      break;
    case 4:
      centry->flag = _atoi(p);
      break;
    case 5:
      centry->name = tl_strdup(p);
      break;
    case 6:
      centry->secure = _atoi(p) ^ 4;
      break;
    case 7:
      centry->value = tl_strdup(p);
      break;
    }
  }

  if(i < 7 || *next)
  {
    cookie_deep_free(centry);
    centry = NULL;
  }

  _free(ln);

  return centry;
}

static cookie_entry *cookie_read_file_real(bufio * fd)
{
  char line[5000];
  cookie_entry *centry;
  cookie_entry *retv = NULL;
  cookie_entry *prev = NULL;

  while(bufio_readln(fd, line, sizeof(line)) > 0)
  {
    if(line[0] == '#')
    {
      if(!strncmp("# KDE Cookie File v2", line, 20))
      {
        cookiefiletype = COOKIEFILETYPE_KDE2;
      }
      continue;
    }
    if(cookiefiletype == COOKIEFILETYPE_KDE2 && line[0] == '[')
      continue;

    strip_nl(line);

    if(*line)
    {
      switch(cookiefiletype)
      {
      case COOKIEFILETYPE_NS: centry = cookie_parse_ns(line); break;
      case COOKIEFILETYPE_KDE2: centry = cookie_parse_kde2(line); break;
      }

      if(centry)
      {
        if(!retv)
          retv = centry;
        else
        {
          prev->next = centry;
          centry->prev = prev;
        }
        prev = centry;
      }
      else
        xprintf(1, gettext("Unable to parse : %s\n"), line);
    }
  }

  return retv;
}

int cookie_read_file(const char *filename)
{
  bufio *fd;
  cookie_entry *retv = NULL;

  LOCK_COOKIES;
  cookie_free_all(cookies);

  if(!(fd = bufio_open(filename, O_BINARY | O_RDONLY)))
  {
    xperror(filename);
    UNLOCK_COOKIES;
    return -1;
  }

  if(_flock(bufio_getfd(fd), filename, O_BINARY | O_RDONLY, FALSE))
  {
    xperror(filename);
    UNLOCK_COOKIES;
    return -2;
  }

  retv = cookie_read_file_real(fd);

  cookie_updated = time(NULL);

  _funlock(bufio_getfd(fd));
  bufio_close(fd);

  cookies = retv;
  last_cookie = cookie_get_last_entry(cookies, &cookies_num);
  UNLOCK_COOKIES;
  return 0;
}

static int cookie_write_file(bufio * fd)
{
  char line[5000];
  cookie_entry *centry;
  time_t t = time(NULL);

  ftruncate(bufio_getfd(fd), 0);
  lseek(bufio_getfd(fd), 0, SEEK_SET);
  bufio_reset(fd);

  for(centry = cookies; centry; centry = centry->next)
  {
    if(!centry->expires || centry->expires > t)
    {
      snprintf(line, sizeof(line), "%s\t%s\t%s\t%s\t%ld\t%s\t%s\n",
        centry->domain,
        centry->flag ? "TRUE" : "FALSE",
        centry->path,
        centry->secure ? "TRUE" : "FALSE",
        centry->expires, centry->name, centry->value);
      bufio_write(fd, line, strlen(line));
    }
  }

  return 0;
}

static void cookie_sync(cookie_entry * centry)
{
  cookie_entry *cmem;
  cookie_entry *cfile;
  cookie_entry **uentry;
  cookie_entry *p;
  bool_t found;
  time_t t = time(NULL);

  for(cfile = centry; cfile; cfile = p)
  {
    found = FALSE;
    p = cfile->next;

    for(cmem = cookies, uentry = &cookies;
      cmem; uentry = &cmem->next, cmem = cmem->next)
    {
      if(cookie_eq(cfile, cmem))
      {
        found = TRUE;
        if((cmem->expires && cmem->expires < t && !cmem->loaded)
          || (cmem->loaded && !strcmp(cmem->value, cfile->value)))
        {
          cookie_deep_free(cfile);
        }
        else
        {
          *uentry = cfile;
          cfile->next = cmem->next;
          cfile->prev = cmem->prev;
          if(cmem->next)
            cmem->next->prev = cfile;
          cookie_deep_free(cmem);
        }
        break;

      }
    }

    if(!found)
    {
      cfile->next = cookies;
      cfile->prev = NULL;
      if(cookies)
        cookies->prev = cfile;
      cookies = cfile;
      cookies_num++;
      if(cookies_num && cookies_num > cfg.cookies_max)
      {
        cookie_remove_oldest_entry(&cookies, &last_cookie, &cookies_num);
      }
    }
  }
}

int cookie_update_file(int force)
{
  struct stat estat;
  cookie_entry *centry = NULL;
  bufio *fd;

  if(!cookie_changed)
    return 0;

  if(!force && cookie_changed < 10)
    return 0;

  if(!cfg.cookie_file)
    return -1;

  if(cookiefiletype != COOKIEFILETYPE_NS)
  {
    xprintf(1, gettext("Cookie file format not supported for writing.\n"));
    return -2;
  }

  LOCK_COOKIES;
  xprintf(1, gettext("Updating cookie file\n"));

  if(!(fd = bufio_copen(cfg.cookie_file, O_BINARY | O_RDWR | O_CREAT,
        S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR)))
  {
    xperror(cfg.cookie_file);
    UNLOCK_COOKIES;
    return -1;
  }

  if(_flock(bufio_getfd(fd), cfg.cookie_file, O_BINARY | O_WRONLY | O_CREAT,
      FALSE))
  {
    xperror(cfg.cookie_file);
    bufio_close(fd);
    UNLOCK_COOKIES;
    return -1;
  }

  if(fstat(bufio_getfd(fd), &estat))
  {
    xperror(cfg.cookie_file);
    _funlock(bufio_getfd(fd));
    bufio_close(fd);
    UNLOCK_COOKIES;
    return -1;
  }

  if(estat.st_mtime > cookie_updated)
  {
    xprintf(1, gettext("Cookie file has changed - > synchronizing\n"));
    centry = cookie_read_file_real(fd);
    cookie_sync(centry);
  }

  cookie_write_file(fd);

  cookie_changed = 0;
  cookie_updated = time(NULL);

  _funlock(bufio_getfd(fd));
  bufio_close(fd);

  UNLOCK_COOKIES;
  return 0;
}

char *cookie_get_field(url * urlp)
{
  int sel = 0;
  char *retv;
  cookie_entry *centry;
  time_t t = time(NULL);

  if(urlp->type != URLT_HTTP && urlp->type != URLT_HTTPS)
    return NULL;

  retv = tl_strdup("Cookie:");

  LOCK_COOKIES;
  for(centry = cookies; centry; centry = centry->next)
  {
    if((!centry->expires || centry->expires > t) &&
      cookie_match(urlp, centry))
    {
      retv = _realloc(retv, strlen(retv) +
        strlen(centry->name) + strlen(centry->value) + 6);

      if(!sel)
        strcat(retv, " ");
      else
        strcat(retv, "; ");

      strcat(retv, centry->name);
      strcat(retv, "=");
      strcat(retv, centry->value);

      sel++;
    }
  }
  UNLOCK_COOKIES;

  if(sel)
    strcat(retv, "\r\n");
  else
    _free(retv);

  return retv;
}

void cookie_insert_field(char *field, url * urlp)
{
  int slen, plen;
  char *p, *p1, *strtokbuf;
  char *pom = tl_strdup(field);
  cookie_entry *centry;
  cookie_entry *pcentry;
  cookie_entry *uentry;
  bool_t found = FALSE;
  char **pp;

  centry = _malloc(sizeof(cookie_entry));

  centry->next = NULL;
  centry->prev = NULL;
  centry->loaded = FALSE;
  centry->domain = NULL;
  centry->flag = TRUE;
  centry->path = NULL;
  centry->expires = 0;
  centry->secure = FALSE;
  centry->name = NULL;
  centry->value = NULL;

  p = strtokc_r(pom, ';', &strtokbuf);
  while(p)
  {
    while(tl_ascii_isspace(*p))
      p++;

    if(!strncasecmp(p, "expires=", 8))
    {
      centry->expires = scntime(p + 8);
    }
    else if(!strncasecmp(p, "path=", 5))
    {
      centry->path = tl_strdup(p + 5);
    }
    else if(!strncasecmp(p, "domain=", 7))
    {
      centry->domain = tl_strdup(p + 7);
    }
    else if(!strcasecmp(p, "secure"))
    {
      centry->secure = TRUE;
    }
    else
    {
      p1 = strchr(p, '=');
      if(p1)
      {
        /* FIXME: multiple assignment -- memory leak */
        centry->name = tl_strndup(p, (p1 - p));
        centry->value = tl_strdup(p1 + 1);
      }
    }

    p = strtokc_r(NULL, ';', &strtokbuf);
  }

  _free(pom);

  if(!centry->value || !centry->name)
  {
    cookie_deep_free(centry);
    centry = NULL;
  }

  if(centry)
  {
    if(!centry->path)
      centry->path = tl_strdup(url_get_path(urlp));

    if(!centry->domain)
      centry->domain = tl_strdup(url_get_site(urlp));
    lowerstr(centry->domain);

    /*** check if cookie is set for source domain ***/
    if(cfg.cookie_check_domain)
    {
      p = url_get_site(urlp);
      plen = strlen(p);
      slen = strlen(centry->domain);
      if(plen < slen || strcmp(centry->domain, p + (plen - slen)))
      {
        xprintf(1,
          gettext("Server %s is trying to set cookie for %s domain\n"), p,
          centry->domain);
        cookie_deep_free(centry);
        UNLOCK_COOKIES;
        return;
      }
    }

    for(pp = priv_cfg.cookies_disabled_domains; pp && *pp; pp++)
    {
      if(!strcasecmp(*pp, centry->domain))
      {
        xprintf(1, gettext("Removing cookie from disabled domain %s\n"),
          centry->domain);
        cookie_deep_free(centry);
        return;
      }
    }

    LOCK_COOKIES;
    for(pcentry = cookies, uentry = NULL;
      pcentry; uentry = pcentry, pcentry = pcentry->next)
    {
      if(cookie_eq(centry, pcentry))
      {
        found = TRUE;
        if(centry->expires && centry->expires < time(NULL))
        {
          pcentry->expires = centry->expires;
          cookie_deep_free(centry);
          cookie_changed++;
          centry = NULL;
          break;
        }
        else
        {
          if(centry->expires != pcentry->expires ||
            strcmp(centry->value, pcentry->value))
          {
            if(uentry)
              uentry->next = centry;
            else
              cookies = centry;
            centry->next = pcentry->next;
            centry->prev = uentry;
            if(centry->next)
              centry->next->prev = centry;
            if(last_cookie == pcentry)
              last_cookie = centry;
            cookie_deep_free(pcentry);
            cookie_changed++;
            break;
          }
        }
      }
    }
    if(!found)
    {
      if(centry->expires && centry->expires < time(NULL))
        cookie_deep_free(centry);
      else
      {
        centry->next = cookies;
        if(cookies)
          cookies->prev = centry;
        if(!last_cookie)
          last_cookie = centry;
        cookies = centry;
        cookie_changed++;
        cookies_num++;
        if(cfg.cookies_max && cookies_num > cfg.cookies_max)
        {
          cookie_remove_oldest_entry(&cookies, &last_cookie, &cookies_num);
        }
      }
    }
    UNLOCK_COOKIES;
  }
}
