/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#ifdef BSDEBUG
#define _malloc malloc
#define _free free
#define TRUE  1
#define FALSE 0
#else
#include "config.h"
#include "tools.h"
#endif

#include "base64.h"

static const char base64_etab[] =
  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

char *base64_encode(const char *str)
{
  return base64_encode_data(str, strlen(str));
}

char *base64_encode_data(const char *data, int len)
{
  char *outstr = (char *) _malloc((len / 3 + 1) * 4 + 1);
  unsigned char *instr = (unsigned char *) _malloc(len + 3);
  int idx = 0, outidx = 0;

  memset(instr, '\0', len + 3);
  memcpy(instr, data, len);
  memset(outstr, '\0', (len / 3 + 1) * 4 + 1);

  while(idx < len)
  {
    if(idx % 3 == 0)
    {
      outstr[outidx] = base64_etab[(int) (instr[idx] >> 2)];
      outidx++;
    }
    else if(idx % 3 == 1)
    {
      outstr[outidx] = base64_etab[(int)
        (((instr[idx - 1] << 4) & 0x30) | ((instr[idx] >> 4) & 0x0f))];
      outidx++;

      outstr[outidx] = base64_etab[(int)
        (((instr[idx] << 2) & 0x3c) | ((instr[idx + 1] >> 6) & 0x03))];
      outidx++;
    }
    else
    {
      outstr[outidx] = base64_etab[(int) (instr[idx] & 0x3f)];
      outidx++;
    }
    idx++;
  }
  if(idx % 3 == 1)
  {
    outstr[outidx] = base64_etab[(int)
      (((instr[idx - 1] << 4) & 0x30) | ((instr[idx] >> 4) & 0x0f))];
    outstr[outidx + 1] = '=';
    outstr[outidx + 2] = '=';
  }
  else if(idx % 3 == 2)
  {
    outstr[outidx] = '=';
  }

  _free(instr);

  return outstr;
}

static int base64_chrindex(const char *str, int chr)
{
  char *p;

  p = strchr(str, chr);

  if(!p)
    return -1;
  else
    return p - str;
}

int base64_decode_data(const char *inbuf, char **outbuf)
{
  char *rv = _malloc(((strlen(inbuf) + 1) * 3) / 4 + 1);
  int len = 0;
  int stop = FALSE, err = FALSE;

  while(*inbuf)
  {
    int n = 0;
    int c[4] = { 0, 0, 0, 0 };
    int nt[5] = { 0, 1, 1, 2, 3 };
    unsigned char triple[3] = { '\0', '\0', '\0' };

    for(n = 0; n < 4; n++)
    {
      if(!inbuf[n] || inbuf[n] == '=')
      {
        stop = TRUE;
        break;
      }
      else
        c[n] = base64_chrindex(base64_etab, inbuf[n]);

      if(c[n] < 0)
      {
        err = TRUE;
        stop = TRUE;
        break;
      }
    }

    triple[0] = (c[0] << 2) | (c[1] >> 4);
    triple[1] = (c[1] & 0x3f) << 4 | ((c[2] & 0x3c) >> 2);
    triple[2] = (c[2] & 0x3) << 6 | (c[3] & 0x3f);

    memcpy(rv + len, triple, nt[n]);

    if(stop)
    {
      len += nt[n];
      break;
    }
    else
      len += 3;

    inbuf += 4;
  }

  if(err)
  {
    _free(rv);
    len = -1;
  }
  else
  {
    rv[len] = '\0';
    *outbuf = rv;
  }

  return len;
}
