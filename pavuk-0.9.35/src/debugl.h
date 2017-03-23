/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#ifndef _debugl_h_
#define _debugl_h_

#define DLV1     (1U << 0)
#define DLV2     (1U << 1)
#define DLV3     (1U << 2)
#define DLV4     (1U << 3)
#define DLV5     (1U << 4)
#define DLV6     (1U << 5)
#define DLV7     (1U << 6)
#define DLV8     (1U << 7)
#define DLV9     (1U << 8)
#define DLV10    (1U << 9)
#define DLV11    (1U << 10)
#define DLV12    (1U << 11)
#define DLV13    (1U << 12)
#define DLV14    (1U << 13)
#define DLV15    (1U << 14)
#define DLV16    (1U << 15)
#define DLV17    (1U << 16)
#define DLV18    (1U << 17)
#define DLV19    (1U << 18)
#define DLV20    (1U << 19)
#define DLV21    (1U << 20)
#define DLV22    (1U << 21)
#define DLV23    (1U << 22)
#define DLV24    (1U << 23)
#define DLV25    (1U << 24)
#define DLV26    (1U << 25)
#define DLV27    (1U << 26)
#define DLV28    (1U << 27)
#define DLV29    (1U << 28)
#define DLV30    (1U << 29)
#define DLV31    (1U << 30)

#ifdef DEBUG
#ifdef __GNUC__
/*** for HTML debug messages ***/
#define DEBUG_HTML(s...)        xdebug(DLV1 , ## s)
/*** for server responses ***/
#define DEBUG_PROTOS(s...)      xdebug(DLV2 , ## s)
/*** for client requests ***/
#define DEBUG_PROTOC(s...)      xdebug(DLV3 , ## s)
/*** function start message ***/
#define DEBUG_PROCS(s)          xdebug(DLV4 , "calling - %s\n", s)
/*** function exit message ***/
#define DEBUG_PROCE(s)          xdebug(DLV4 , "exiting - %s\n", s)
/*** locking ***/
#define DEBUG_LOCKS(s...)       xdebug(DLV5 , ## s)
/*** network stuff ***/
#define DEBUG_NET(s...)         xdebug(DLV6 , ## s)
/*** some misc debug messages ***/
#define DEBUG_MISC(s...)        xdebug(DLV7 , ## s)
/*** user level infos ***/
#define DEBUG_USER(s...)        xdebug(DLV8 , ## s)
/*** multithreading ***/
#define DEBUG_MTLOCK(s...)      xdebug(DLV9 , ## s)
#define DEBUG_MTTHR(s...)       xdebug(DLV10 , ## s)
#define DEBUG_PROTOD(s...)      xdebug(DLV11 , ## s)
#define DEBUG_LIMITS(s...)      xdebug(DLV12 , ## s)
#define DEBUG_SSL(s...)         xdebug(DLV13 , ## s)
#else
extern void DEBUG_HTML(char *, ...);
extern void DEBUG_PROTOS(char *, ...);
extern void DEBUG_PROTOC(char *, ...);
extern void DEBUG_PROCS(char *);
extern void DEBUG_PROCE(char *);
extern void DEBUG_LOCKS(char *, ...);
extern void DEBUG_NET(char *, ...);
extern void DEBUG_MISC(char *, ...);
extern void DEBUG_USER(char *, ...);
extern void DEBUG_MTLOCK(char *, ...);
extern void DEBUG_MTTHR(char *, ...);
extern void DEBUG_PROTOD(char *, ...);
extern void DEBUG_LIMITS(char *, ...);
extern void DEBUG_SSL(char *, ...);
#endif

typedef struct
{
  long id;
  char *option;
  char *label;
} debug_level_type;

extern debug_level_type cfg_debug_levels[];

extern int debug_level_parse(char *);
extern char *debug_level_construct(int, char *);

#else
#ifdef __GNUC__
#define DEBUG_HTML(s...)
#define DEBUG_PROTOS(s...)
#define DEBUG_PROTOC(s...)
#define DEBUG_URL(s...)
#define DEBUG_PROCS(s...)
#define DEBUG_PROCE(s...)
#define DEBUG_LOCKS(s...)
#define DEBUG_NET(s...)
#define DEBUG_MISC(s...)
#define DEBUG_USER(s...)
#define DEBUG_MTLOCK(s...)
#define DEBUG_MTTHR(s...)
#define DEBUG_PROTOD(s...)
#define DEBUG_LIMITS(s...)
#define DEBUG_SSL(s...)
#else
extern void DEBUG_HTML(char *, ...);
extern void DEBUG_PROTOS(char *, ...);
extern void DEBUG_PROTOC(char *, ...);
extern void DEBUG_PROCS(char *);
extern void DEBUG_PROCE(char *);
extern void DEBUG_LOCKS(char *, ...);
extern void DEBUG_NET(char *, ...);
extern void DEBUG_MISC(char *, ...);
extern void DEBUG_USER(char *, ...);
extern void DEBUG_MTLOCK(char *, ...);
extern void DEBUG_MTTHR(char *, ...);
extern void DEBUG_PROTOD(char *, ...);
extern void DEBUG_LIMITS(char *, ...);
extern void DEBUG_SSL(char *, ...);
#endif
#endif

#endif
