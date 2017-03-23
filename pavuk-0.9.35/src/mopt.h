/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#ifndef _mopt_h_
#define _mopt_h_

#include "dlhash.h"
#include "config.h"

typedef struct
{
  int argc;
  char **argv;
  int nparams;
  cfg_param_t *params;
  dlhash *short_hash;
  dlhash *long_hash;
  int current;
  int short_offset;
  int option_type;
  char *args[4];
} mopt_t;

#define MOPT_OK          0      /* on success              */
#define MOPT_END         1      /* on end                  */
#define MOPT_PARAM       2      /* on no option param      */
#define MOPT_MISSINGP   -1      /* on bad number of params */
#define MOPT_UNKNOWN    -2      /* on unknown param        */
#define MOPT_BAD        -3      /* on wrong formated param */
#define MOPT_ERR        -4      /* on parser error         */

#define MOPT_OPT_NONE           0
#define MOPT_OPT_SHORT          1
#define MOPT_OPT_LONG           2
#define MOPT_OPT_COMPAT         3

extern void mopt_init(mopt_t *, int, cfg_param_t *, int, char **);
extern void mopt_destroy(mopt_t *);

extern int mopt_get_next_param(mopt_t *, cfg_param_t **);

#endif
