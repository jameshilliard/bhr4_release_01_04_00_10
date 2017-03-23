/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#ifndef _mimetype_h_
#define _mimetype_h_

struct mime_type_ext
{
  const char *mimet;
  const char *ext;
};

extern const char *mimetypes[];
extern const struct mime_type_ext mime_type_exts[];

#endif
