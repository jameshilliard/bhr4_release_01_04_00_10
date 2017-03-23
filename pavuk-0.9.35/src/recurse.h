/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#ifndef _recurse_h_
#define _recurse_h_

#include "doc.h"
#include "dllist.h"

extern int process_document(doc *, int);
extern int download_single_doc(url *);
extern void recurse(int);
extern void get_urls_to_resume(char *);
extern void get_urls_to_synchronize(char *, dllist **);

#endif
