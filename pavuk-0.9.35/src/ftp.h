/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#ifndef __ftp_h__
#define __ftp_h__

#include <time.h>
#include "doc.h"
#include "myssl.h"
#include "bufio.h"
#include "dllist.h"

#define DEFAULT_FTP_PORT 21
#define DEFAULT_FTP_PROXY_PORT  22

#define FTP_TYPE_F      0
#define FTP_TYPE_D      1
#define FTP_TYPE_L      2

typedef struct
{
  protocol proto;
  char *user;
  char *passwd;
  char *host;
  unsigned short port;
  bufio *control;
} ftp_connection;

typedef struct _ftp_url_extension
{
  char type;                    /*** type of file ***/
  short int perm;               /*** permisions ***/
  ssize_t size;                 /*** size of file ***/
  char *slink;                  /*** where point symlink ***/
  time_t time;                  /*** last modification time ***/
} ftp_url_extension;

typedef struct _ftp_handshake_info_data
{
  char *cmd;
  int response;
} ftp_handshake_info_data;

typedef struct _ftp_handshake_info
{
  char *host;
  unsigned short port;
  dllist *infos;
} ftp_handshake_info;

extern bufio *ftp_get_data_socket(doc *);
extern void ftp_dir_to_html(doc *);
extern void ftp_dir_list_to_html(doc *);
extern int ftp_get_response(doc *, char **, int);
extern int ftp_make_symlink(url *);
extern int ftp_remove(doc *);

extern ftp_url_extension *ftp_parse_ftpinf_ext(char *);
extern ftp_url_extension *ftp_url_ext_new(int, int, ssize_t, char *, time_t);
extern ftp_url_extension *ftp_url_ext_dup(ftp_url_extension *);
extern void ftp_url_ext_free(ftp_url_extension *);

extern void ftp_handshake_info_free(ftp_handshake_info *);
extern ftp_handshake_info *ftp_handshake_info_parse(char *, char *);
extern char *ftp_handshake_info_data_dump(ftp_handshake_info *);
extern ftp_handshake_info *ftp_handshake_info_dup(ftp_handshake_info *);


#endif
