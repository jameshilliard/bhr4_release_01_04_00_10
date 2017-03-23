/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#ifndef _dlhash_tools_h_
#define _dlhash_tools_h_

extern unsigned int str_hash_func(unsigned int, dllist_t);
extern unsigned int url_hash_func(unsigned int, dllist_t);
extern int str_comp_func(dllist_t, dllist_t);
extern dllist_t url_key_func(dllist_t);
extern dllist_t fn_key_func(dllist_t);
extern void url_free_func(dllist_t);

#endif
