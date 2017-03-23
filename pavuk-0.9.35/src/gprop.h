/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#ifndef _gprop_h_
#define _gprop_h_

#include <stdio.h>

typedef struct
{
  enum
  {
    GPROP_STR,
    GPROP_INT,
    GPROP_BOOL
  } type;
  char *name;
  void *value;
} gprop;

extern gprop *gprop_parse(char *);
extern char *gprop_dump(gprop *);
extern void gprop_add(gprop *);
extern void gprop_set_str(char *, char *);
extern void gprop_set_int(char *, int);
extern void gprop_set_bool_t(char *, int);
extern int gprop_get_str(char *, char **);
extern int gprop_get_int(char *, int *);
extern int gprop_get_bool_t(char *, int *);
extern void gprop_save(FILE *);

#endif
