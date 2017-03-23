/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#ifndef _icons_h_
#define _icons_h_

#include "doc.h"

typedef struct
{
  Icon *audio;
  Icon *binary;
  Icon *html;
  Icon *image;
  Icon *video;
  Icon *text;
  Icon *file;
  Icon *gopherdir;
  Icon *ftpdir;
  Icon *broken;
  Icon *redirected;
  Icon *rejected;
  Icon *notprocessed;
  Icon *cantaccess;
  Icon *incomplete;
  Icon *local;
  Icon *compressed;
} urltype_icon;

extern void icons_load(void);
extern void icons_set_for_doc(doc *);

#define MAX_PIX_HEIGHT 14

#endif
