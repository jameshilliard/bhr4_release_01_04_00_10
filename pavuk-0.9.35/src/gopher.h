/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#ifndef _gopher_h_
#define _gopher_h_

#include "bufio.h"
#include "doc.h"

#define DEFAULT_GOPHER_PORT 70
#define DEFAULT_GOPHER_PROXY_PORT 8080

extern bufio *gopher_get_data_socket(doc *);
extern void gopher_dir_to_html(doc *);
#endif
