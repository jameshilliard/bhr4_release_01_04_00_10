/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "mode.h"
#include <string.h>

static const pavuk_modes pavuk_all_modes[] = {
  {MODE_NORMAL, "normal"},
  {MODE_LNUPD, "linkupdate"},
  {MODE_SYNC, "sync"},
  {MODE_MIRROR, "mirror"},
  {MODE_SINGLE, "singlepage"},
  {MODE_SREGET, "singlereget"},
  {MODE_RESUME, "resumeregets"},
  {MODE_NOSTORE, "dontstore"},
  {MODE_REMIND, "reminder"},
  {MODE_FTPDIR, "ftpdir"},
};

pavuk_mode mode_get_by_str(char *str)
{
  int i;

  for(i = 0; i < NUM_MODES; i++)
  {
    if(!strcasecmp(pavuk_all_modes[i].mode_name, str))
      return pavuk_all_modes[i].mode;
  }
  return MODE_UNKNOWN;
}

const char *mode_get_str(pavuk_mode mode)
{
  return pavuk_all_modes[mode].mode_name;
}
