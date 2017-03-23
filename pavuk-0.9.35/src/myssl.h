/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#ifndef _myssl_h_
#define _myssl_h_

#ifdef USE_SSL

#include "bufio.h"
#include "doc.h"
#include "myssl_openssl.h"
#include "myssl_nss.h"

extern void my_ssl_init(void);
extern void my_ssl_init_once(void);
extern void my_ssl_init_start(void);
extern void my_ssl_cleanup(void);
extern bufio *my_ssl_do_connect(doc *, bufio *, ssl_connection *);
extern int my_ssl_read(ssl_connection *, char *, size_t);
extern int my_ssl_write(ssl_connection *, char *, size_t);
extern void my_ssl_connection_close(ssl_connection *);
extern void my_ssl_connection_destroy(ssl_connection *);
extern int my_ssl_data_pending(ssl_connection *);
extern void my_ssl_print_last_error(void);
#endif

#endif
