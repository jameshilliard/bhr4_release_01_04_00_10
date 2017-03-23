/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#ifndef _times_h_
#define _times_h_

#include <time.h>

extern time_t scntime(char *);
extern struct tm *new_tm(struct tm *);
extern time_t time_ftp_scn(char *);
extern time_t time_scn_cmd(char *);

#endif
