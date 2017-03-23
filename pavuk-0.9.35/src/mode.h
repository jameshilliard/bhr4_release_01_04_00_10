/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#ifndef _mode_h_
#define _mode_h_

typedef enum
{
  MODE_NORMAL,
  MODE_LNUPD,
  MODE_SYNC,
  MODE_SINGLE,
  MODE_SREGET,
  MODE_RESUME,
  MODE_NOSTORE,
  MODE_REMIND,
  MODE_FTPDIR,
  MODE_MIRROR,
  /*MODE_DNLDR , */
  NUM_MODES,
  MODE_UNKNOWN
} pavuk_mode;

typedef struct
{
  pavuk_mode mode;
  char *mode_name;
} pavuk_modes;

extern pavuk_mode mode_get_by_str(char *);
extern const char *mode_get_str(pavuk_mode);

#endif
