/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include <stdarg.h>

#include "url.h"
#include "doc.h"
#include "bufio.h"

#ifdef I_FACE

extern void gui_beep(void);

extern void gui_loop_serve(void);
extern void gui_loop_escape(void);
extern void gui_loop_do(void);

extern void gui_start(int *, char **);
extern void gui_main(void);

extern void gui_create_tree_root_node(void);
extern void gui_clear_tree(void);
extern void *gui_tree_make_entry(url *);
extern void gui_tree_set_icon(url *);
extern void gui_tree_add_start(void);
extern void gui_tree_add_end(void);
extern void gui_tree_set_icon_for_doc(doc *);

extern void gui_start_download(int);
extern void gui_finish_download(int);
extern void gui_finish_document(doc *);

extern void gui_mt_thread_start(int);
extern void gui_mt_thread_end(int);

extern void gui_set_doccounter(void);
extern void gui_set_progress(char *, char *, char *, char *);
extern void gui_set_msg(char *, int);
extern void gui_set_status(char *);
extern void gui_set_url(char *);
extern void gui_clear_status(void);

extern void gui_msleep(long);
extern int gui_wait_io(int, int);

extern int gui_xvaprintf(char *, va_list *);
extern int gui_xprint(char *);

extern void gui_do_ui_cleanup(void);

#else

#define gui_beep()

#define gui_loop_serve()
#define gui_loop_escape()
#define gui_loop_do()

#define gui_start(n,l)
#define gui_main()

#define gui_create_tree_root_node()
#define gui_clear_tree()
#define gui_tree_make_entry(u)
#define gui_tree_set_icon(u)
#define gui_tree_add_start()
#define gui_tree_add_end()
#define gui_tree_set_icon_for_doc(d)

#define gui_start_download(d)
#define gui_finish_download(d)
#define gui_finish_document(d)

#define gui_mt_thread_start(i)
#define gui_mt_thread_end(i)

#define gui_set_doccounter()
#define gui_set_progress(s, p, e, r)
#define gui_set_msg(t, n)
#define gui_set_status(t) DEBUG_USER(" - %s\n", t)
#define gui_set_url(u)
#define gui_clear_status()

#define gui_msleep(s)
#define gui_wait_io(f, s) TRUE

#endif
