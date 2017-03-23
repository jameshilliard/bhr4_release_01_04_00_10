/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#ifndef _myopenssl_h_
#define _myopenssl_h_

#if defined(USE_SSL) && defined(USE_SSL_IMPL_OPENSSL)

#include <stdio.h>

#ifdef OPENSSL
#include <openssl/rsa.h>
#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/md5.h>
#include <openssl/rand.h>
#else
#include <rsa.h>
#include <crypto.h>
#include <x509.h>
#include <pem.h>
#include <ssl.h>
#include <err.h>
#include <md5.h>
#include <rand.h>
#endif

typedef struct
{
  SSL *ssl_con;
  SSL_CTX *ssl_ctx;
  bufio *socket;
} ssl_connection;

#endif

#endif
