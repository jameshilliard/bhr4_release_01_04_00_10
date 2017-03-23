/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#ifndef _decode_h_
#define _decode_h_

#define GZIP_CMD "gunzip"

#ifdef NeedFunctionPrototypes
extern int gzip_decode(char *, int, char **, ssize_t *, char *);
extern int inflate_decode(char *, int, char **, ssize_t *, char *);
#else
extern int gzip_decode();
extern int inflate_decode();
#endif

#endif
