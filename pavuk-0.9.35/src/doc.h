/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#ifndef _doc_h_
#define _doc_h_
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>

#include "url.h"
#include "tools.h"
#include "bufio.h"
#include "myssl_openssl.h"
#include "myssl_nss.h"

typedef struct
{
  bool_t log;
  char *msg;
} doc_msg;

typedef struct
{
  /*******************************/
  /* Basic document infos        */
  /*******************************/
  int doc_nr;                    /*** number of document in queue ***/
  url *doc_url;                  /*** URL of document ***/
  char *mime;                    /*** MIME header of document ***/
  char *type_str;                /*** MIME type of document ***/
  bool_t is_parsable;            /*** documemt dedicated for parsing ? (html,css,scripts) ***/

  ssize_t size;                  /*** document size readed ***/
  ssize_t totsz;                 /*** total size of document if known ***/
  ssize_t origsize;              /*** original size of document ***/
  char *contents;                /*** document content ***/
  ssize_t rest_pos;              /*** restart position ***/
  ssize_t rest_end_pos;          /*** restart end position ***/
  time_t dtime;                  /*** creation time of document ***/
  time_t stime;                  /*** time of document request ***/
  time_t origtime;               /*** modification time of local copy of document */
  int errcode;                   /*** error code for current doc ***/

  /*******************************/
  /* temporary document infos    */
  /*******************************/
  char *lock_fn;                 /*** lock file name for document ***/
  bufio *datasock;               /*** socket for data connection ***/
  bufio *s_sock;                 /*** file where is going online saved data ***/

  /*******************************/
  /* downloading control flags   */
  /*******************************/
  bool_t load;                   /*** load document into ->contents ***/
  bool_t doreget;                /*** continue broken transfer ***/
  bool_t report_size;            /*** show progressmeter while loading of document ***/
  bool_t check_limits;           /*** check limits for adding URLs from this document ***/
  bool_t remove_lock;            /*** remove lock file ***/
  bool_t save_online;            /*** document should be saved online to file ***/
  bool_t is_robot;               /*** redirection of robots.txt documents is handled differently ! ***/
  bool_t is_http_transfer;       /*** is going current transfer over HTTP protocol ? ***/

  /*******************************/
  /* HTTP connection info        */
  /*******************************/
  int request_type;              /*** HTTP request method used for this document ***/
  unsigned short http_proxy_port;/*** used HTTP proxy port ***/
  char *http_proxy;              /*** used HTTP proxy ***/
  bool_t http_proxy_10;          /*** is this HTTP/1.0 proxy ***/
  char *etag;                    /*** ETag or Last-Modified for conditional partial HTTP GET method ***/
  char *connect_host;            /*** hostname for CONNECT request ***/
  unsigned short connect_port;   /*** portnumber for CONNECT request ***/

  /*******************************/
  /* informations for HTTP/1.1   */
  /* (chunked encoding)          */
  /*******************************/
  bool_t is_http11;              /*** we are talking with HTTP/1.1 server ***/
  ssize_t chunk_size;            /*** for HTTP/1.1 chunked transfer encoding ***/
  bool_t is_chunked;             /*** current document is encoded with chunked encoding ***/
  bool_t read_chunksize;         /*** in next read we expect chunksize header ***/
  bool_t read_trailer;           /*** in next read we expect trailer header ***/
  bool_t is_persistent;          /*** is current HTTP connection persistent
                                      (and should leave or not) ***/

  /*******************************/
  /* HTTPS SSL connection info   */
  /* (used also by FTPS datacon) */
  /* need for persistent SSLID   */
  /*******************************/
#ifdef USE_SSL
  ssl_connection ssl_data_con;
#endif

  /*******************************/
  /* HTTP auth informations      */
  /*******************************/
  short num_auth;                /*** number of attempts to authenticate ***/
  short num_proxy_auth;          /*** number of attempts to authenticate with proxy ***/
  void *auth_digest;             /*** HTTP digest access authentification info ***/
  void *auth_proxy_digest;       /*** HTTP digest access proxy authentification info ***/
  char *additional_headers;      /*** additional headers (currently required by NTLM) ***/

  /*******************************/
  /* FTP connection info         */
  /*******************************/
  bool_t ftp_fatal_err;          /*** was FTP error fatal ? ***/
  short ftp_respc;               /*** last FTP response code ***/
  bufio *ftp_control;            /*** socket for FTP control connection ***/
  char *ftp_pasv_host;           /*** info for passive data connection ***/
  unsigned short ftp_pasv_port;
  bool_t ftp_data_con_finished;  /*** FTP data connection was just fully established ***/

  /*******************************/
  /* progress meter informations */
  /*******************************/
#ifdef HAVE_GETTIMEOFDAY
  struct timeval start_time;     /*** for progress metter ***/
  struct timeval hr_start_time;  /*** when documant processing started ***/
  struct timeval redirect_time;  /*** when all redirects finished ***/
  struct timeval dns_time;       /*** when dns lookup finished ***/
  struct timeval connect_time;   /*** when connect(2) finished ***/
  struct timeval first_byte_time;/*** when first byte was received ***/
  struct timeval end_time;       /*** when download finished ***/
#else
  time_t start_time;             /*** for progress metter ***/
#endif
  ssize_t current_size;          /*** size for current speed ***/
  ssize_t adj_sz;                /*** adjustment of doc size (HTTP header size ) ***/

  /*******************************/
  /* per thread informations     */
  /*******************************/
#ifdef HAVE_MT
  dllist *msgbuf;                /*** list of buffered messages ***/
  int __herrno;                  /*** per document h_errno value ***/
  int threadnr;                  /*** number of current thread ***/
#endif

	int http_response_code;
	char sockinfo[128];
} doc;

extern int doc_download_init(doc *, int);
extern int doc_download(doc *, int, int);
extern int doc_store(doc *, int);
extern int doc_remove(url *);
extern int doc_lock(doc *, int);
extern time_t doc_etime(doc *, int);
extern void doc_init(doc *, url *);
extern void doc_cleanup(doc *);
extern void doc_destroy(doc *);
extern void doc_remove_lock(doc *);
extern void doc_update_parent_links(doc *);
#ifdef HAVE_MT
extern void doc_finish_processing(doc *);
#endif

extern void print_all_url_infos(void);
extern void add_urlinfo_to_final_print(doc* docu);

#endif
