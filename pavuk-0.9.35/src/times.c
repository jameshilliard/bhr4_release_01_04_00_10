/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include <time.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "config.h"
#include "tools.h"
#include "times.h"

typedef struct _dayn
{
  char *name;
  int dayn;
} dayn;

static dayn dnames[] = {
  {"Sun", 0},
  {"Mon", 1},
  {"Tue", 2},
  {"Wed", 3},
  {"Thu", 4},
  {"Fri", 5},
  {"Sat", 6},
  {"Sunday", 0},
  {"Monday", 1},
  {"Tuesday", 2},
  {"Wednesday", 3},
  {"Thursday", 4},
  {"Friday", 5},
  {"Saturday", 6},
};

static dayn mnames[] = {
  {"Jan", 0},
  {"Feb", 1},
  {"Mar", 2},
  {"Apr", 3},
  {"May", 4},
  {"Jun", 5},
  {"Jul", 6},
  {"Aug", 7},
  {"Sep", 8},
  {"Oct", 9},
  {"Nov", 10},
  {"Dec", 11},
  {"January", 0},
  {"February", 1},
  {"March", 2},
  {"April", 3},
  {"May", 4},
  {"Jun", 5},
  {"July", 6},
  {"August", 7},
  {"September", 8},
  {"October", 9},
  {"November", 10},
  {"December", 11},
};

#ifndef HAVE_GMTOFF
static long difftm(struct tm *, struct tm *);
#endif

/**************************/
/* make copy of struct tm */
/**************************/
struct tm *new_tm(struct tm *t)
{
  struct tm *tn;

  if(!t)
    return NULL;

  tn = _malloc(sizeof(struct tm));
  memcpy((char *) tn, (char *) t, sizeof(struct tm));
  return tn;
}

/************************/
/* scan GMT time string */
/************************/
time_t scntime(char *timestr)
{
  char *pom = new_string(timestr);
  int i;
  char *p;
  struct tm otm;
  time_t rv;
  int ilen;
  bool_t last = 1;

  memset((void *) &otm, '\0', sizeof(otm));

  otm.tm_isdst = -1;

  p = pom;
  p += strspn(p, ", -:");
  ilen = strcspn(p, ", -:");
  if(*(p + ilen))
    *(p + ilen) = '\0';
  else
    last = 0;

  if(!*p)
    return 0L;
  for(i = 0; i < NUM_ELEM(dnames); i++)
  {
    if(!strcasecmp(p, dnames[i].name))
    {
      otm.tm_wday = dnames[i].dayn;
      break;
    }
  }
  if(!(i < NUM_ELEM(dnames)))
    return 0L;

#define NEXT_TOKEN \
  p += ilen+last;\
  p += strspn(p , ", -:");\
  ilen = strcspn(p , ", -:");\
  if (*(p+ilen)) *(p+ilen) = '\0';\
  else last = 0;\

  NEXT_TOKEN;
  otm.tm_mday = _atoi(p);
  if(errno == ERANGE)
  {
    for(i = 0; i < NUM_ELEM(mnames); i++)
    {
      if(!strcasecmp(p, mnames[i].name))
      {
        otm.tm_mon = mnames[i].dayn;
        break;
      }
    }
    if(!(i < NUM_ELEM(mnames)))
      return 0L;

    NEXT_TOKEN;
    otm.tm_mday = _atoi(p);
    if(errno == ERANGE)
      return 0L;

    NEXT_TOKEN;
    otm.tm_hour = _atoi(p);
    if(errno == ERANGE)
      return 0L;

    NEXT_TOKEN;
    otm.tm_min = _atoi(p);
    if(errno == ERANGE)
      return 0L;

    NEXT_TOKEN;
    otm.tm_sec = _atoi(p);
    if(errno == ERANGE)
      return 0L;

    NEXT_TOKEN;
    otm.tm_year = _atoi(p);
    if(errno == ERANGE)
      return 0L;
    otm.tm_year = (otm.tm_year > 1900) ? otm.tm_year - 1900 : otm.tm_year;
  }
  else
  {
    NEXT_TOKEN;
    for(i = 0; i < NUM_ELEM(mnames); i++)
    {
      if(!strcasecmp(p, mnames[i].name))
      {
        otm.tm_mon = mnames[i].dayn;
        break;
      }
    }
    if(!(i < NUM_ELEM(mnames)))
      return 0L;

    NEXT_TOKEN;
    otm.tm_year = _atoi(p);
    if(errno == ERANGE)
      return 0L;
    otm.tm_year = (otm.tm_year > 1900) ? otm.tm_year - 1900 : otm.tm_year;

    NEXT_TOKEN;
    otm.tm_hour = _atoi(p);
    if(errno == ERANGE)
      return 0L;

    NEXT_TOKEN;
    otm.tm_min = _atoi(p);
    if(errno == ERANGE)
      return 0L;

    NEXT_TOKEN;
    otm.tm_sec = _atoi(p);
    if(errno == ERANGE)
      return 0L;
  }

  rv = mktime(&otm);

#ifdef HAVE_GMTOFF
  rv += otm.tm_gmtoff;
#else
  {
    struct tm *gt, *lt;

    LOCK_TIME;
    lt = new_tm(localtime(&rv));
    gt = new_tm(gmtime(&rv));
    UNLOCK_TIME;

    rv += difftm(lt, gt);

    free(lt);
    free(gt);
  }
#endif

  free(pom);
  return rv;
}

/**********************************************/
/* parse time string in format YYYYMMDDHHmmss */
/**********************************************/
time_t time_ftp_scn(char *timestr)
{
  char pom[10];
  struct tm ftm;
  time_t rv;

  memset((void *) &ftm, '\0', sizeof(ftm));
  ftm.tm_isdst = -1;
  strncpy(pom, timestr, 4);
  *(pom + 4) = '\0';
  ftm.tm_year = _atoi(pom) - 1900;

  strncpy(pom, timestr + 4, 2);
  *(pom + 2) = '\0';
  ftm.tm_mon = _atoi(pom) - 1;

  strncpy(pom, timestr + 6, 2);
  *(pom + 2) = '\0';
  ftm.tm_mday = _atoi(pom);

  strncpy(pom, timestr + 8, 2);
  *(pom + 2) = '\0';
  ftm.tm_hour = _atoi(pom);

  strncpy(pom, timestr + 10, 2);
  *(pom + 2) = '\0';
  ftm.tm_min = _atoi(pom);

  strncpy(pom, timestr + 12, 2);
  *(pom + 2) = '\0';
  ftm.tm_sec = _atoi(pom);

  rv = mktime(&ftm);

#ifdef HAVE_GMTOFF
  rv += ftm.tm_gmtoff;
#else
  {
    struct tm *gt, *lt;

    LOCK_TIME;
    lt = new_tm(localtime(&rv));
    gt = new_tm(gmtime(&rv));
    UNLOCK_TIME;

    rv += difftm(lt, gt);

    free(lt);
    free(gt);
  }
#endif

  return rv;
}

time_t time_scn_cmd(char *timestr)
{
  time_t t = 0;
  struct tm ftm;

  if(sscanf(timestr, "%d.%d.%d.%d:%d", &ftm.tm_year, &ftm.tm_mon,
      &ftm.tm_mday, &ftm.tm_hour, &ftm.tm_min) == 5)
  {
    ftm.tm_year = ftm.tm_year - 1900;
    ftm.tm_mon -= 1;
    ftm.tm_isdst = -1;
    ftm.tm_sec = 0;
    t = mktime(&ftm);
  }

  return t;
}

#ifndef HAVE_GMTOFF
/************************************************************************/
/*   this function I steal from gettext-0.10.35/src/xgettext.c    */
/*      here is copyright         */
/*   copyright (C) 1995, 1996, 1997, 1998 Free Software Foundation, Inc.*/
/*   Written by Ulrich Drepper <drepper@gnu.ai.mit.edu>, April 1995.    */
/************************************************************************/

#define TM_YEAR_ORIGIN 1900

/* Yield A - B, measured in seconds.  */
static long difftm(struct tm *a, struct tm *b)
{
  int ay = a->tm_year + (TM_YEAR_ORIGIN - 1);
  int by = b->tm_year + (TM_YEAR_ORIGIN - 1);
  /* Some compilers cannot handle this as a single return statement.  */
  long days = (
    /* difference in day of year  */
    a->tm_yday - b->tm_yday
    /* + intervening leap days  */
    + ((ay >> 2) - (by >> 2))
    - (ay / 100 - by / 100) + ((ay / 100 >> 2) - (by / 100 >> 2))
    /* + difference in years * 365  */
    + (long) (ay - by) * 365l);

  return 60l * (60l * (24l * days + (a->tm_hour - b->tm_hour))
    + (a->tm_min - b->tm_min)) + (a->tm_sec - b->tm_sec);
}
#endif
