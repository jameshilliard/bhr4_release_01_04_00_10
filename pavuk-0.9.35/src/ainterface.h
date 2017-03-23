/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#ifndef _ainterface_h_
#define _ainterface_h_

#include "url.h"

#ifdef GETTEXT_NLS
extern char **get_available_languages(void);
#endif
extern void absi_cont(void);
extern void absi_restart(void);
extern void free_all(void);
extern url *append_starting_url(url_info *, url *);

#endif
