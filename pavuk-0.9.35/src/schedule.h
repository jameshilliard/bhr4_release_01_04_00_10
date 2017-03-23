/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#ifndef _schedule_h_
#define _schedule_h_

#ifndef AT_CMD
#define AT_CMD "at -f %f %t %d.%m.%Y"
#endif

extern int at_schedule(void);

#endif
