/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"
#include "debugl.h"
#include "tools.h"
#include "string.h"

#ifdef DEBUG

debug_level_type cfg_debug_levels[] = {
  {DLV1, "html", gettext_nop("HTML parsers")},
  {DLV2, "protos", gettext_nop("Server responses")},
  {DLV3, "protoc", gettext_nop("Client requests")},
  {DLV4, "procs", gettext_nop("Procedure calling")},
  {DLV5, "locks", gettext_nop("File locking")},
  {DLV6, "net", gettext_nop("Networking code")},
  {DLV7, "misc", gettext_nop("Miscelanous")},
  {DLV8, "user", gettext_nop("Extended user infos")},
  {DLV9, "mtlock", gettext_nop("Multithreading - locking")},
  {DLV10, "mtthr", gettext_nop("Multithreading - threads")},
  {DLV11, "protod", gettext_nop("POST request data")},
  {DLV12, "limits", gettext_nop("Limiting conditions")},
  {DLV13, "ssl", gettext_nop("SSL informations")},
  {0, NULL, NULL}
};

int debug_level_parse(char *str)
{
  int dl = 0;
  char *p = str;
  int l, i;
  bool_t found;

  while(*p)
  {
    p += strspn(p, ",");
    l = strcspn(p, ",");
    found = FALSE;
    for(i = 0; cfg_debug_levels[i].id; i++)
    {
      if(!strncasecmp(p, cfg_debug_levels[i].option, l) &&
        (l == strlen(cfg_debug_levels[i].option)))
      {
        found = TRUE;
        dl |= cfg_debug_levels[i].id;
        break;
      }
    }
    if(!found && !strncasecmp(p, "all", l))
    {
      dl = 0x7fffffff;
    }
    else if(!found)
    {
      char *ep;

      i = strtol(p, &ep, 0);

      if((ep - p) != l)
      {
        xprintf(0, gettext("Bad debug level selection : %s\n"), str);
        return -1;
      }
      else
        dl |= i;
    }
    p += l;
  }

  return dl;
}

char *debug_level_construct(int level, char *strbuf)
{
  bool_t frst = TRUE;
  int i;

  if(!level)
  {
    strcpy(strbuf, "0");
    return strbuf;
  }

  for(i = 0; cfg_debug_levels[i].id; i++)
  {
    if(!(cfg_debug_levels[i].id & level))
      continue;

    if(!frst)
      strcat(strbuf, ",");
    strcat(strbuf, cfg_debug_levels[i].option);
    frst = FALSE;
  }

  return strbuf;
}
#endif

#ifndef __GNUC__

void DEBUG_HTML(char *str, ...)
{
#ifdef DEBUG
  va_list args;
  va_start(args, str);
  xvadebug(DLV1, str, &args);
  va_end(args);
#endif
}

void DEBUG_PROTOS(char *str, ...)
{
#ifdef DEBUG
  va_list args;
  va_start(args, str);
  xvadebug(DLV2, str, &args);
  va_end(args);
#endif
}

void DEBUG_PROTOC(char *str, ...)
{
#ifdef DEBUG
  va_list args;
  va_start(args, str);
  xvadebug(DLV3, str, &args);
  va_end(args);
#endif
}

void DEBUG_PROCS(char *str)
{
#ifdef DEBUG
  xdebug(DLV4, "calling - %s\n", str);
#endif
}

void DEBUG_PROCE(char *str)
{
#ifdef DEBUG
  xdebug(DLV4, "exiting - %s\n", str);
#endif
}

void DEBUG_LOCKS(char *str, ...)
{
#ifdef DEBUG
  va_list args;
  va_start(args, str);
  xvadebug(DLV5, str, &args);
  va_end(args);
#endif
}

void DEBUG_NET(char *str, ...)
{
#ifdef DEBUG
  va_list args;
  va_start(args, str);
  xvadebug(DLV6, str, &args);
  va_end(args);
#endif
}

void DEBUG_MISC(char *str, ...)
{
#ifdef DEBUG
  va_list args;
  va_start(args, str);
  xvadebug(DLV7, str, &args);
  va_end(args);
#endif
}

void DEBUG_USER(char *str, ...)
{
#ifdef DEBUG
  va_list args;
  va_start(args, str);
  xvadebug(DLV8, str, &args);
  va_end(args);
#endif
}

void DEBUG_MTLOCK(char *str, ...)
{
#ifdef DEBUG
  va_list args;
  va_start(args, str);
  xvadebug(DLV9, str, &args);
  va_end(args);
#endif
}

void DEBUG_MTTHR(char *str, ...)
{
#ifdef DEBUG
  va_list args;
  va_start(args, str);
  xvadebug(DLV10, str, &args);
  va_end(args);
#endif
}

void DEBUG_PROTOD(char *str, ...)
{
#ifdef DEBUG
  va_list args;
  va_start(args, str);
  xvadebug(DLV11, str, &args);
  va_end(args);
#endif
}

void DEBUG_LIMITS(char *str, ...)
{
#ifdef DEBUG
  va_list args;
  va_start(args, str);
  xvadebug(DLV12, str, &args);
  va_end(args);
#endif
}
void DEBUG_SSL(char *str, ...)
{
#ifdef DEBUG
  va_list args;
  va_start(args, str);
  xvadebug(DLV13, str, &args);
  va_end(args);
#endif
}

#endif
