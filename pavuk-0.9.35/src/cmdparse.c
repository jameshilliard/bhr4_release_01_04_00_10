/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"

#ifdef __CYGWIN__
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <process.h>

#include "tools.h"

static char *tl_cmdln_parse_unescape_str(char *str, int len)
{
  char *rv;
  char *p;
  int i;

  rv = _malloc(len + 1);
  p = rv;
  for(i = 0; i < len; i++)
  {
    if(str[i] == '\\')
      i++;
    *p = str[i];
    p++;
  }
  *p = '\0';

  return rv;
}

static int tl_cmdln_parse_get_param(char *str, char **param)
{
  int i, j;
  int was_start = FALSE;
  char delim = -1;
  char *p;

  p = str;
  i = 0;
  while(p[i])
  {
    if(!was_start && tl_ascii_isspace(p[i]))
    {
      i++;
      continue;
    }
    else if(!was_start)
    {
      was_start = TRUE;
      if(p[i] == '\"' || p[i] == '\'')
      {
        delim = p[i];
        i++;
      }
      else
        delim = '\0';
      break;
    }
  }

  if(delim < 0)
    return 0;

  p += i;
  j = 0;
  while(p[j])
  {
    if(p[j] == '\\')
    {
      if(p[j + 1])
        j += 2;
      else
        return -1;
      continue;
    }
    if(delim && p[j] == delim && (tl_ascii_isspace(p[j + 1]) || !p[j + 1]))
    {
      break;
    }
    if(!delim && tl_ascii_isspace(p[j]))
    {
      break;
    }
    j++;
  }

  if(delim && !p[j])
    return -1;

  *param = (delim == '\'') ? tl_strndup(p, j) :
    tl_cmdln_parse_unescape_str(p, j);

  return i + j + (delim ? 1 : 0);
}

char **tl_cmdln_parse(char *cmdln)
{
  char *p, *param;
  char **rv = NULL;
  int nr = 0, l;

  p = cmdln;
  while(*p)
  {
    l = tl_cmdln_parse_get_param(p, &param);

    if(l < 0)
    {
      char **pp;
      for(pp = rv; pp && *pp; pp++)
        _free(*pp);
      _free(rv);
      printf(gettext("Error parsing commandline at: %s\n"), p);
      return NULL;
    }
    if(!l)
      break;
    nr++;
    rv = _realloc(rv, sizeof(char *) * (nr + 1));
    rv[nr - 1] = param;
    rv[nr] = NULL;
    p += l;
  }

  return rv;
}

int tl_win32_system(char *cmd)
{
  if(!access("/bin/sh", X_OK))
  {
    return system(cmd);
  }
  else
  {
    int rv = -1;
    int pid;
    char **params;

    params = tl_cmdln_parse(cmd);

    if(params)
    {
      char **pp;

#if 0
      rv = spawnvp(_P_WAIT, params[0], params + 1);
#else
      pid = spawnvp(_P_NOWAIT, params[0], params + 1);
      if(pid >= 0)
      {
        if(_cwait(&rv, pid, WAIT_CHILD) < 0)
          rv = -1;
      }
      else
        rv = -1;
#endif

      pp = params;
      while(pp && *pp)
      {
        _free(*pp);
        pp++;
      }
      _free(params);
    }
    else
    {
      xprintf(1, gettext("Unable to parse commandline: %s\n"), cmd);
    }
    return rv;
  }
}

#if 0
int main(int argc, char **argv)
{
  char **pp;

  pp = tl_cmdln_parse(argv[1]);

  while(pp && *pp)
  {
    printf("%s\n", *pp);
    pp++;
  }

  return 0;
}
#endif

#endif
