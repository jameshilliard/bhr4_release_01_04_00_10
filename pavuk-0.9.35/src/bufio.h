/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#ifndef _bufio_h_
#define _bufio_h_

typedef struct
{
  int fd;
  char *buf;
  int buf_size;
  int buf_start;
  int buf_end;
  int bufio_errno;
  int eof;
  int is_sock;
  void *ssl_hook_data;
} bufio;

extern bufio *bufio_fdopen(int);
extern bufio *bufio_sock_fdopen(int);
extern bufio *bufio_open(const char *, int);
extern bufio *bufio_copen(const char *, int, int);
extern bufio *bufio_dupfd(int);
extern bufio *bufio_new_sslcon(bufio *, void *);
extern int bufio_read(bufio *, char *, size_t);
extern int bufio_nbfread(bufio *, char *, size_t);
extern int bufio_readln(bufio *, char *, size_t);
extern void bufio_unread(bufio *, char *, size_t);
extern int bufio_write(bufio *, char *, size_t);
extern int bufio_close(bufio *);
extern void bufio_free(bufio *);
extern void bufio_reset(bufio *);

#define bufio_sslread(d, b, l, s) _bufio_read(d, b, l)
#define bufio_sslnbfread(d, b, l, s) _bufio_nbfread(d, b, l)
#define bufio_sslreadln(d, b, l, s) _bufio_readln(d, b, l)
#define bufio_sslwrite(d, b, l, s) _bufio_write(d, b, l)

#define bufio_getfd(s) ((s)->fd)
#define bufio_is_sock(s) ((s)->is_sock)
#define bufio_get_ssl_hook_data(s) ((s)->ssl_hook_data)

#endif
