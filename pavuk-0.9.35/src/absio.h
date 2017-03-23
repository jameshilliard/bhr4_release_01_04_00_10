/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#ifndef _absio_h_
#define _absio_h_

#ifdef __QNX__
#define tl_sock_read(fd, buf, size, ssld) tl_unix_read(fd, buf, size, ssld)
#define tl_sock_write(fd, buf, size, ssld) tl_unix_write(fd, buf, size, ssld)
#define tl_file_read(fd, buf, size) read(fd, buf, size)
#define tl_file_write(fd, buf, size) write(fd, buf, size)
#elif defined __BEOS__
#define tl_sock_read(fd, buf, size, ssld) tl_beos_recv(fd, buf, size, ssld)
#define tl_sock_write(fd, buf, size, ssld) tl_beos_send(fd, buf, size, ssld)
#define tl_file_read(fd, buf, size) read(fd, buf, size)
#define tl_file_write(fd, buf, size) write(fd, buf, size)
#else
#define tl_sock_read(fd, buf, size, ssld) tl_unix_read(fd, buf, size, ssld)
#define tl_sock_write(fd, buf, size, ssld) tl_unix_write(fd, buf, size, ssld)
#define tl_file_read(fd, buf, size) tl_unix_read(fd, buf, size, NULL)
#define tl_file_write(fd, buf, size) tl_unix_write(fd, buf, size, NULL)
#endif

extern int tl_unix_read(int, char *, size_t, void *);
extern int tl_unix_write(int, char *, size_t, void *);
extern int tl_beos_recv(int, char *, int, void *);
extern int tl_beos_send(int, char *, int, void *);
extern int tl_selectr(int, int);
extern int tl_selectw(int, int);

#endif
