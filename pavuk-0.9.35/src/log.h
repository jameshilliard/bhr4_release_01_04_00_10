/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#ifndef _log_h_
#define _log_h_

#include "doc.h"
#include "url.h"

extern int log_start(char *);
extern void log_str(char *);
extern void short_log(doc *, url *);
extern void time_log(doc *);
#endif
