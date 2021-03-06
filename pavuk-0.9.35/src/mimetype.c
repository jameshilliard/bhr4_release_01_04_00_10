/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "mimetype.h"

const char *mimetypes[] = {
  "application/activemessage",
  "application/andrew-insert",
  "application/applefile",
  "application/atomicmail",
  "application/dca-rft",
  "application/dec-dx",
  "application/mac-binhex40",
  "application/macwriteii",
  "application/msword",
  "application/news-message-id",
  "application/news-transmission",
  "application/octet-stream",
  "application/oda",
  "application/pdf",
  "application/postscript",
  "application/remote-printing",
  "application/rtf",
  "application/slate",
  "application/x-mif",
  "application/wita",
  "application/wordperfect5.1",
  "application/x-csh",
  "application/x-dvi",
  "application/x-hdf",
  "application/x-latex",
  "application/x-netcdf",
  "application/x-sh",
  "application/x-tcl",
  "application/x-tex",
  "application/x-texinfo",
  "application/x-troff",
  "application/x-troff-man",
  "application/x-troff-me",
  "application/x-troff-ms",
  "application/x-wais-source",
  "application/zip",
  "application/x-bcpio",
  "application/x-cpio",
  "application/x-gtar",
  "application/x-shar",
  "application/x-sv4cpio",
  "application/x-sv4crc",
  "application/x-tar",
  "application/x-ustar",
  "audio/basic",
  "audio/x-aiff",
  "audio/x-wav",
  "image/gif",
  "image/ief",
  "image/jpeg",
  "image/pjpeg",
  "image/tiff",
  "image/x-cmu-raster",
  "image/x-portable-anymap",
  "image/x-portable-bitmap",
  "image/x-portable-graymap",
  "image/x-portable-pixmap",
  "image/x-rgb",
  "image/x-xbitmap",
  "image/x-xpixmap",
  "image/x-xwindowdump",
  "message/external-body",
  "message/news",
  "message/partial",
  "message/rfc822",
  "multipart/alternative",
  "multipart/appledouble",
  "multipart/digest",
  "multipart/mixed",
  "multipart/parallel",
  "text/html",
  "text/plain",
  "text/richtext",
  "text/tab-separated-values",
  "text/x-setext",
  "video/mpeg",
  "video/quicktime",
  "video/x-msvideo",
  "video/x-sgi-movie",
  0
};

const struct mime_type_ext mime_type_exts[] = {
  {"text/html*", ".html"},
  {"text/js", ".js"},
  {"text/plain", ".txt"},
  {"image/jpeg", ".jpg"},
  {"image/pjpeg", ".jpg"},
  {"image/gif", ".gif"},
  {"image/png", ".png"},
  {"image/tiff", ".tiff"},
  {"application/pdf", ".pdf"},
  {"application/msword", ".doc"},
  {"application/postscript", ".ps"},
  {"application/rtf", ".rtf"},
  {"application/wordperfect5.1", ".wps"},
  {"application/zip", ".zip"},
  {"video/mpeg", ".mpg"},
  {0, 0}
};
