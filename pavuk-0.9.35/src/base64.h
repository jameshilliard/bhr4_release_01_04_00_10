/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#ifndef _base64_h_
#define _base64_h_

extern char *base64_encode_data(const char *, int);
extern char *base64_encode(const char *);
extern int base64_decode_data(const char *, char **);

#endif
