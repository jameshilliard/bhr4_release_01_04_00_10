/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#ifndef _uexit_h_
#define _uexit_h_

#include <time.h>
#include "doc.h"

extern int uexit_condition(url *, int *, time_t);
extern int uexit_follow_cmd(doc *);

#endif
