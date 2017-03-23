/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "tools.h"
#include "tr.h"

#define HEXASC2HEXNR(x) (((x) >= '0' && (x) <= '9') ? \
  ((x) - '0') : (tl_ascii_tolower(x) - 'a' + 10))

typedef enum
{
  TR_ALPHA,
  TR_ALNUM,
  TR_NUM,
  TR_XNUM,
  TR_SPACE,
  TR_BLANK,
  TR_CTRL,
  TR_PRINTABLE,
  TR_UPPER,
  TR_LOWER,
  TR_PUNCT,
  TR_GRAPH,
  TR_NONPRINTABLE,
  TR_BADCLS
} tr_cls;

typedef struct
{
  char *name;
  tr_cls cls;
} tr_cls_str;

static const tr_cls_str tr_cls_map[] = {
  {"[:upper:]", TR_UPPER},
  {"[:lower:]", TR_LOWER},
  {"[:alpha:]", TR_ALPHA},
  {"[:alnum:]", TR_ALNUM},
  {"[:digit:]", TR_NUM},
  {"[:xdigit:]", TR_XNUM},
  {"[:space:]", TR_SPACE},
  {"[:blank:]", TR_BLANK},
  {"[:cntrl:]", TR_CTRL},
  {"[:print:]", TR_PRINTABLE},
  {"[:nprint:]", TR_NONPRINTABLE},
  {"[:punct:]", TR_PUNCT},
  {"[:graph:]", TR_GRAPH}
};

static int tr_is_cls(char c, tr_cls cls)
{
  switch (cls)
  {
  case TR_ALPHA:
    return tl_ascii_isalpha(c);
    break;
  case TR_ALNUM:
    return tl_ascii_isalnum(c);
    break;
  case TR_NUM:
    return tl_ascii_isdigit(c);
    break;
  case TR_XNUM:
    return tl_ascii_isxdigit(c);
    break;
  case TR_SPACE:
    return tl_ascii_isspace(c);
    break;
  case TR_BLANK:
    return tl_ascii_isblank(c);
    break;
  case TR_CTRL:
    return tl_ascii_iscntrl(c);
    break;
  case TR_PRINTABLE:
    return tl_ascii_isprint(c);
    break;
  case TR_UPPER:
    return tl_ascii_isupper(c);
    break;
  case TR_LOWER:
    return tl_ascii_islower(c);
    break;
  case TR_PUNCT:
    return tl_ascii_ispunct(c);
    break;
  case TR_GRAPH:
    return tl_ascii_isgraph(c);
    break;
  case TR_NONPRINTABLE:
    return !tl_ascii_isprint(c);
    break;
  default:
    return FALSE;
  }
}

static unsigned int tr_append_cls(char *p, tr_cls cls)
{
  unsigned int i;
  unsigned int apnd = 0;

  for(i = 0; i <= 255; i++)
  {
    if(tr_is_cls(i, cls))
    {
      p[apnd] = i;
      p[apnd + 1] = '\0';
      apnd++;
    }
  }

  return apnd;
}

static unsigned char tr_get_escaped_str(char **p)
{
  char *ps = *p;
  char rc;

  switch (ps[1])
  {
  case 'n':
    rc = '\n';
    (*p)++;
    break;
  case 'r':
    rc = '\r';
    (*p)++;
    break;
  case 't':
    rc = '\t';
    (*p)++;
    break;
    break;
  case '0':
    if(ps[2] == 'x')
    {
      if(tl_ascii_isxdigit(ps[3]) && tl_ascii_isxdigit(ps[4]))
      {
        rc = (HEXASC2HEXNR(ps[3]) << 4) + HEXASC2HEXNR(ps[4]);
        (*p) += 4;
      }
      else
        rc = ps[0];
    }
    else
      rc = ps[0];
    break;
  default:
    rc = ps[1];
    (*p)++;
  }

  return rc;
}

static char *tr_expand_str(char *str, int *rlen)
{
  char *p;
  char pom[4096];
  int i;

  pom[0] = '\0';

  for(i = 0, p = str; *p; p++)
  {
    switch (*p)
    {
    case '\\':
      pom[i] = tr_get_escaped_str(&p);
      pom[i + 1] = '\0';
      i++;
      break;
    case '[':
      {
        int j;
        tr_cls cls = TR_BADCLS;

        for(j = 0; j < TR_BADCLS; j++)
        {
          if(!strncmp(p, tr_cls_map[j].name, strlen(tr_cls_map[j].name)))
          {
            cls = tr_cls_map[j].cls;
            p += strlen(tr_cls_map[j].name) - 1;
            break;
          }
        }
        if(cls != TR_BADCLS)
          i += tr_append_cls((pom + i), cls);
        else
        {
          pom[i] = *p;
          pom[i + 1] = '\0';
          i++;
        }
      }
      break;
    case '-':
      {
        char strtc;
        char endc;
        int pc;

        if(i)
          strtc = pom[i - 1] + 1;
        else
          strtc = '\0';

        if(*(p + 1))
        {
          p++;
          if(*p == '\\')
          {
            endc = tr_get_escaped_str(&p);
          }
          else
            endc = *p;
        }
        else
          endc = '\255';

        for(pc = strtc; pc <= endc; pc++)
        {
          pom[i] = pc;
          i++;
        }
      }
      break;
    default:
      pom[i] = *p;
      pom[i + 1] = '\0';
      i++;
    }
  }

  if(rlen)
    *rlen = i;

  return (tl_strdup(pom));
}

char *tr_chr_chr(char *fset, char *tset, char *str)
{
  char *p, *d;
  int i;
  int tsetlen = strlen(tset);
  char *retv = tl_strdup(str);

  for(p = str, d = retv; *p; p++, d++)
  {
    for(i = 0; fset[i]; i++)
    {
      if(fset[i] == *p)
      {
        *d = tset[(tsetlen > i) ? i : (tsetlen - 1)];
        break;
      }
    }
  }

  return retv;
}

char *tr_del_chr(char *set, char *str)
{
  char *p, *d;
  int i;
  char *retv = tl_strdup(str);
  int found;

  for(p = str, d = retv; *p; p++)
  {
    found = FALSE;
    for(i = 0; set[i]; i++)
    {
      if(set[i] == *p)
      {
        found = TRUE;
        break;
      }
    }
    if(!found)
    {
      *d = *p;
      d++;
      *d = '\0';
    }
  }

  return retv;
}

char *tr_str_str(char *s1, char *s2, char *str)
{
  char *p = str, *p1, *retv;
  int i = 0;

  while(p)
  {
    if((p = strstr(p, s1)))
    {
      i++;
      p += strlen(s1);
    }
  }

  retv = (char *) malloc(1 + strlen(str) - i * strlen(s1) +
    i * (s2 ? strlen(s2) : 0));
  memset(retv, '\0',
    1 + strlen(str) - i * strlen(s1) + i * (s2 ? strlen(s2) : 0));

  p = p1 = str;

  while(p1)
  {
    p1 = strstr(p, s1);
    if(p1)
    {
      strncat(retv, p, p1 - p);
      if(s2)
        strcat(retv, s2);
    }
    else
      strcat(retv, p);

    p = p1 + strlen(s1);
  }

  return retv;
}

char *tr(char *str)
{
  char *p1, *p2;
  char *s1, *s2;

  if(priv_cfg.tr_str_s1 && priv_cfg.tr_str_s2)
  {
    p1 = tr_str_str(priv_cfg.tr_str_s1, priv_cfg.tr_str_s2, str);
  }
  else
  {
    p1 = tl_strdup(str);
  }

  if(priv_cfg.tr_del_chr)
  {
    s1 = tr_expand_str(priv_cfg.tr_del_chr, NULL);
    p2 = tr_del_chr(s1, p1);
    _free(s1);
    _free(p1);
  }
  else
  {
    p2 = p1;
  }

  if(priv_cfg.tr_chr_s1 && priv_cfg.tr_chr_s2)
  {
    s1 = tr_expand_str(priv_cfg.tr_chr_s1, NULL);
    s2 = tr_expand_str(priv_cfg.tr_chr_s2, NULL);
    p1 = tr_chr_chr(s1, s2, p2);
    _free(s1);
    _free(s2);
    _free(p2);
  }
  else
  {
    p1 = p2;
  }

  return p1;
}
