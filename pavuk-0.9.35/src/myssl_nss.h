/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#ifndef _myssl_nss_h_
#define _myssl_nss_h_

#if defined(USE_SSL) && defined(USE_SSL_IMPL_NSS)

#include "bufio.h"

#include <nspr.h>
#include <prio.h>
#include <pk11func.h>
#include <nss.h>
#include <ssl.h>

typedef struct
{
  PRFileDesc *fd;
  bufio *socket;
} ssl_connection;

#endif

#endif
