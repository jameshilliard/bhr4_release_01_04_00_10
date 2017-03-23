/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#ifndef _gcinfo_h_
#define _gcinfo_h_

#include "doc.h"
#include "ftp.h"
#include "myssl.h"
#include "http.h"

typedef struct
{
  ftp_connection ftp_con;
  http_connection http_con;
#ifdef USE_SSL
  ssl_connection ssl_con;
#endif
} global_connection_info;

extern void save_global_connection_data(global_connection_info *, doc *);
extern void restore_global_connection_data(global_connection_info *, doc *);
extern void kill_global_connection_data(global_connection_info *);
extern void init_global_connection_data(global_connection_info *);

#endif
