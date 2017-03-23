/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#ifndef _jsbind_h_
#define _jsbind_h_

#include "condition.h"
#include "lfname.h"

extern int pjs_destroy(void);
extern int pjs_init(void);
extern int pjs_load_script(char *);
extern int pjs_load_script_string(char *);
extern int pjs_execute(char *);

extern int pjs_run_cond_check_func(url *, cond_info_t *);
extern char *pjs_run_fnrules_func(char *, struct lfname_lsp_interp *);

#endif
