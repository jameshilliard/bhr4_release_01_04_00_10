/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#ifndef _abstract_h_
#define _abstract_h_

#include <time.h>
#include "doc.h"
#include "bufio.h"

extern bufio *abs_get_data_socket(doc *);
extern void abs_close_socket(doc *, int);
extern int abs_read(bufio *, char *, size_t);
extern int abs_readln(bufio *, char *, size_t);
extern int abs_write(bufio *, char *, size_t);
extern int abs_read_data(doc *, bufio *, char *, size_t);

#endif
