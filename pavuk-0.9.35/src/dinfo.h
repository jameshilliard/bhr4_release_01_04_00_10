/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#ifndef _dinfo_h_
#define _dinfo_h_

#include "url.h"
#include "doc.h"

extern int dinfo_save(doc *);
extern char *dinfo_load(char *);
extern char *dinfo_get_unique_name(url *, char *, int);
extern url *dinfo_get_url_for_filename(char *);
extern void dinfo_remove(char *);

#endif
