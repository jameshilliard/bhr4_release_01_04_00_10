/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#ifndef _condition_h_
#define _condition_h_

#include <time.h>

#include "url.h"
#include "tools.h"
#include "dllist.h"

typedef struct
{
  dllist *ports;                /*** list of allowed/denied ports ***/
  bool_t allow_ports;
  bool_t limit_inlines;         /*** == FALSE used to disable checking of limiting rules on inline objects ***/
  int leave_level;              /*** how many levels leave from starting site ***/
  bool_t dont_leave_site_dir;   /*** don't leave dir which we entered first on this site ***/
  bool_t dont_leave_site;       /*** dont leave site of starting URL ***/
  bool_t dont_leave_dir;        /*** dont leave directory of starting URL ***/
  time_t btime;                 /*** file never than ***/
  time_t etime;                 /*** file older than ***/
  int max_size;                 /*** maximal size of document to exclude big documents ***/
  int min_size;                 /*** minimal size of document to exclude small documents ***/
  int max_levels;               /*** obmedzenie na maximalnu hlbku vnorenia sa do stromu - 0 neobmedzene ****/
  int max_documents;            /*** obmedzenie na maximalny pocet dokumentov - 0 neobmedzene ***/
  bool_t cgi;                   /*** povolenie / zakazanie prenosu dokumentov enerovanych cez CGI ***/
  char **sites;                 /*** zoznam povolenych / zakazanych serverov ***/
  bool_t allow_site;            /*** indikacia ci v "sites" su povolene alebo zakazane servery ***/
  char **sufix;                 /*** zoznam povolenych / zakazanych pripon dokumentov ***/
  bool_t allow_sufix;           /*** indikacia ci v zozname "sufix" su povolene alebo zakazane pripony ***/
  char **dir_prefix;            /*** zozname povolenych / zakazanych prefixov dokumentov ***/
  bool_t allow_prefix;          /*** indikacia ci v zozname "dir_prefix" su povolene alebo zakazane prefixy ***/
  char **domains;               /*** list of domains to check in URLs ***/
  bool_t allow_domain;          /*** domains in list are allowed ***/
  char **mime;                  /*** list of mime types to check ***/
  bool_t allow_mime;            /*** allowed/disallowed mime types ***/
  bool_t ftp;                   /*** povolenie / zakazanie protokolu FTP ***/
  bool_t ftpdir;                /*** povolenie / zakazanie rekurzivneho prenosu FTP adresarov ***/
  bool_t http;                  /*** povolenie / zakazanie protokolu HTTP ***/
  bool_t gopher;                /*** povolenie / zakazanie protokolu Gopher ***/
  bool_t allow_robots;          /*** povolenie / zakazanie pouzitia suborov "robots.txt" pri kontrole pristupu k dokumentu ***/
  char **pattern;               /*** fnmatch patterns for matching document names ***/
  char **url_pattern;           /*** fnmatch patterns for matching url ***/
  char **skip_pattern;          /*** fnmatch patterns to exclude matching document names ***/
  char **skip_url_pattern;      /*** fnmatch patterns to exclude matching url ***/
  char *uexit;                  /*** user exit script ***/
  char *follow_cmd;             /*** user exit script to allow/disallow following links of current doc ***/
  int site_level;               /*** maximum site level to leave ***/

#ifdef USE_SSL
  bool_t https;                 /*** povolenie / zakazanie protokolu HTTPS (HTTP nad SSL) ***/
  bool_t ftps;                  /*** povolenie / zakazanie protokolu FTPS ***/
  bool_t verify;                /*** Verify SSL certs */
#endif

#ifdef HAVE_REGEX
  dllist *rpattern;             /*** regular patterns for matching document names ***/
  dllist *rurl_pattern;         /*** regular patterns for matching url ***/
  dllist *rskip_pattern;        /*** regular patterns to exclude matching document names ***/
  dllist *rskip_url_pattern;    /*** regular patterns to exclude matching url ***/

  dllist *aip;                  /*** allowed IP addresses ***/
  dllist *skipip;               /*** disalloved IP addreses ***/
#endif
  dllist *tag_patterns;         /*** tag attrib url (RE,WC) patterns ***/
} cond;

typedef enum
{
  CONDT_UNSUP,
  CONDT_NOFTP,
  CONDT_NOHTTP,
  CONDT_NOSSL,
  CONDT_NOGOPHER,
  CONDT_NOFTPS,
  CONDT_NOCGI,
  CONDT_LMAX,
  CONDT_DMAX,
  CONDT_ASITE,
  CONDT_DSITE,
  CONDT_ADOMAIN,
  CONDT_DDOMAIN,
  CONDT_APREFIX,
  CONDT_DPREFIX,
  CONDT_ASFX,
  CONDT_DSFX,
  CONDT_DONT_LEAVE_SITE,
  CONDT_DONT_LEAVE_DIR,
  CONDT_SITE_LEVEL,
  CONDT_LEAVE_LEVEL,
  CONDT_DONT_LEAVE_SITE_ENTER_DIR,
  CONDT_APORTS,
  CONDT_DPORTS,
  CONDT_MAX_SIZE,
  CONDT_MIN_SIZE,
  CONDT_AMIME_TYPE,
  CONDT_DMIME_TYPE,
  CONDT_NEWER_THAN,
  CONDT_OLDER_THAN,
  CONDT_AIP_PATTERN,
  CONDT_DIP_PATTERN,
  CONDT_PATTERN,
  CONDT_RPATTERN,
  CONDT_SKIP_PATTERN,
  CONDT_SKIP_RPATTERN,
  CONDT_URL_PATTERN,
  CONDT_URL_RPATTERN,
  CONDT_SKIP_URL_PATTERN,
  CONDT_SKIP_URL_RPATTERN,
  CONDT_TAG_PATTERN,
  CONDT_TAG_RPATTERN,
  CONDT_USER_CONDITION
} cond_type_t;

typedef struct
{
  int level;
  int urlnr;
  int size;
  time_t time;
  char *mimet;
  char *full_tag;
  dllist *params;
  cond_type_t reason;
  char *html_doc;
  int html_doc_offset;
  char *tag;
  char *attrib;
} cond_info_t;

extern int url_append_condition(url *, cond_info_t *);
extern int url_append_one_condition(char *, url *, cond_info_t *);

#endif
