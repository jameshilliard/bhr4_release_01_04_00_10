/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#ifndef _url_h_
#define _url_h_

#include "tools.h"
#include "dllist.h"
#include "mt.h"

typedef struct
{
  char *host;                           /*** HTTP host address ***/
  unsigned short port;                  /*** HTTP service port number ***/
  char *document;                       /*** document path ***/
  char *searchstr;                      /*** query string ***/
  char *anchor_name;                    /*** anchor name ***/
  char *user;                           /*** username for authorization ***/
  char *password;                       /*** password for authorization ***/
} url_http_t;

typedef struct
{
  char *host;                           /*** FTP host address ***/
  unsigned short port;                  /*** FTP service port number ***/
  char *user;                           /*** username for authorization ***/
  char *password;                       /*** password for authorization ***/
  char *path;                           /*** document path ***/
  char *anchor_name;                    /*** anchor name ***/
  bool_t dir;                           /*** is this FTP directory URL ? ***/
} url_ftp_t;

typedef struct
{
  char *filename;                       /*** file path ***/
  char *searchstr;                      /*** query string ***/
  char *anchor_name;                    /*** anchor name ***/
} url_file_t;

typedef struct
{
  char *host;                           /*** GOPHER host address ***/
  unsigned short port;                  /*** GOPHER service port number ***/
  char *selector;                       /*** document selector ***/
} url_gopher_t;

typedef struct
{
  char *urlstr;                         /*** url string for unsupported type of URLs ***/
} url_unsup_t;

typedef enum
{                       /*** id for URL types ***/
  URLT_UNKNOWN,
  URLT_HTTP,
  URLT_HTTPS,
  URLT_FTP,
  URLT_FTPS,
  URLT_FILE,
  URLT_GOPHER,
  URLT_FROMPARENT
} protocol;

/* refers to RFC 2396 */
#define URL_PATH_UNSAFE " <>\"#%{}|\\^[]`+@=&$?;"
#define URL_QUERY_UNSAFE " <>\"#%{}|\\^[]`"
#define URL_RQUERY_UNSAFE " ;/?:@&=+$,<>#%\"{}|\\^[]`"
#define URL_AUTH_UNSAFE " <>\"#%{}|\\^[]`+@=&:$?;/"


#define URL_REDIRECT            (unsigned int) (1 << 0)
#define URL_INLINE_OBJ          (unsigned int) (1 << 1)
#define URL_PROCESSED           (unsigned int) (1 << 2)
#define URL_DOWNLOADED          (unsigned int) (1 << 3)
#define URL_ERR_UNREC           (unsigned int) (1 << 4)
#define URL_MOVED               (unsigned int) (1 << 5)
#define URL_REJECTED            (unsigned int) (1 << 6)
#define URL_USER_DISABLED       (unsigned int) (1 << 7)
#define URL_NOT_FOUND           (unsigned int) (1 << 8)
#define URL_TRUNCATED           (unsigned int) (1 << 9)
#define URL_ERR_REC             (unsigned int) (1 << 10)
#define URL_STYLE               (unsigned int) (1 << 11)
#define URL_INNSCACHE           (unsigned int) (1 << 12)
#define URL_ISHTML              (unsigned int) (1 << 13)
#define URL_ISLOCAL             (unsigned int) (1 << 14)
#define URL_NORECURSE           (unsigned int) (1 << 15)
#define URL_FORM_ACTION         (unsigned int) (1 << 16)
#define URL_HAVE_FORMS          (unsigned int) (1 << 17)
#define URL_ISFIRST             (unsigned int) (1 << 18)
#define URL_ISSTARTING          (unsigned int) (1 << 19)
#define URL_ISSCRIPT            (unsigned int) (1 << 20)

typedef struct _protinfo
{
  protocol id;
  char *dirname;
  char *urlid;
  char *typestr;
  int default_port;
  bool_t supported;
} protinfo;

typedef struct
{                                       /*** properties of document ***/
  char *type;                           /*** MIME type of document ***/
  ssize_t size;                         /*** size of document ***/
  time_t mdtm;                          /*** modification time ***/
} url_prop;

typedef union
{
  url_http_t http;
  url_file_t file;
  url_ftp_t ftp;
  url_gopher_t gopher;
  url_unsup_t unsup;
} url_union_t;

typedef struct _url
{
  protocol type;                        /*** type of URL ***/

  dllist *parent_url;                   /*** list of parent URLs ***/
  struct _url *moved_to;                /*** pointer to new URL if document was moved ***/

  unsigned short level;                 /*** tree level of document ***/
  unsigned short ref_cnt;               /*** number of references to this URL structure ***/
  unsigned int status;                  /*** status flags of URL ***/

  url_union_t p;                        /*** parsed URL infos ***/
  char *local_name;                     /*** assigned local filename ***/
  void *extension;                      /*** posible url extensions ***/

#ifdef WITH_TREE
#ifdef I_FACE
  url_prop *prop;                       /*** document properties ***/
  void **tree_nfo;                      /*** UI representation of tree nodes ***/
#endif
#endif                          /* WITH_TREE */
#ifdef HAVE_MT
  pthread_mutex_t lock;                 /*** mt lock ***/
#endif
} url;

#define URLI_NORMAL             1
#define URLI_FORM               2

typedef enum
{
  FORM_M_GET,
  FORM_M_POST,
  FORM_M_UNKNOWN
} form_method;

typedef enum
{
  FORM_E_MULTIPART,
  FORM_E_URLENCODED,
  FORM_E_UNKNOWN
} form_encoding;

typedef struct
{
  char *urlstr;
  int type;
  form_method method;
  form_encoding encoding;
  dllist *fields;
  char *localname;
} url_info;

extern url_info *url_info_new(char *);
extern url_info *url_info_parse(char *);
extern char *url_info_dump(url_info *);
extern void url_info_free(url_info *);
extern url_info *url_info_duplicate(url_info *);

extern url *url_parse(char *);
extern url *url_dup_url(url *);
extern char *url_parse_scheme(char *);
extern protocol url_scheme_to_schemeid(char *);
extern int dllist_url_compare(dllist_t key1, dllist_t key2);
extern int url_compare(url *, url *);
extern char *url_to_absolute_url(char *, char *, url *, char *);
extern char *url_encode_str(char *, char *);
extern char *url_decode_str(const char *, int);
extern url *new_url(url *);
extern char *url_to_filename(url *, int);
extern char *url_to_filename_with_type(url *, const char *, int);
extern char *url_get_default_local_name(url *);
extern char *url_get_local_name_real(url *, const char *, int);
extern void url_changed_filename(url *);
extern char *url_to_in_filename(url *);
extern void free_deep_url(url *);
extern char *get_redirect_abs_path(url *, char *);
extern char *url_to_urlstr(url *, int);
extern char *url_to_request_urlstr(url *, int);
extern void url_path_abs(url *);
extern url *filename_to_url(char *);
extern void cat_links_to_url_list(dllist *);
extern void append_url_to_list(url *);
extern void append_url_list_to_list(dllist *, dllist *);
extern char *url_get_site(url *);
extern int url_get_port(url *);
extern char *url_get_pass(url *, char *);
extern char *url_get_user(url *, char *);
extern int url_get_auth_scheme(url *, char *);
extern char *url_get_path(url *);
extern char *url_get_full_path(url *);
extern char *url_get_anchor_name(url *);
extern void url_clear_anchor(url *);
extern char *url_get_search_str(url *);
extern void url_set_path(url *, char *);
extern int url_is_dir_index(url *);
extern int url_is_same_site(url *, url *);
extern void url_add_to_url_hash_tab(url *);
extern void url_remove_from_url_hash_tab(url *);
extern void url_add_to_file_hash_tab(url *);
extern void url_remove_from_file_hash_tab(url *);
extern url *url_was_befor(url *);
extern void replace_url_in_list(url *, int);
extern void link_url_in_list(url *, url *);
extern int url_redirect_to(url *, url *, int);
extern void url_forget_filename(url *);
extern void url_set_filename(url *, char *);

extern const protinfo prottable[9];

#endif
