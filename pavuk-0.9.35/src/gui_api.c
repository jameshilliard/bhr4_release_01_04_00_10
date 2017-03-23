/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"
#include "gui.h"

#ifdef GTK_FACE
#include <stdio.h>
#include <errno.h>
#include <string.h>

#ifndef __CYGWIN__
#include <X11/Xlib.h>
#include <gdk/gdkx.h>
#endif

#include "icons.h"
#include "gui_api.h"
#include "gkeys.h"
#include "gaccel.h"

void gui_beep(void)
{
  gdk_beep();
}

void gui_start_download(int ismain)
{
#ifdef HAVE_MT
  if(!ismain)
  {
    GDK_THREADS_ENTER();
  }
#endif
  gtk_label_set(GTK_LABEL(gui_cfg.status_msg), "");
  gtk_widget_set_sensitive(gui_cfg.bt_rest, FALSE);
  gtk_widget_set_sensitive(gui_cfg.mea_rest, FALSE);
  gtk_widget_set_sensitive(gui_cfg.mtb_rest, FALSE);
  gtk_widget_set_sensitive(gui_cfg.bt_start, FALSE);
  gtk_widget_set_sensitive(gui_cfg.mea_start, FALSE);
  gtk_widget_set_sensitive(gui_cfg.mtb_start, FALSE);
  gtk_widget_set_sensitive(gui_cfg.bt_stop, TRUE);
  gtk_widget_set_sensitive(gui_cfg.mea_stop, TRUE);
  gtk_widget_set_sensitive(gui_cfg.mtb_stop, TRUE);
  gtk_widget_set_sensitive(gui_cfg.bt_break, TRUE);
  gtk_widget_set_sensitive(gui_cfg.mea_break, TRUE);
  gtk_widget_set_sensitive(gui_cfg.mtb_break, TRUE);

#ifdef WITH_TREE
  gtk_label_set(GTK_LABEL(gui_cfg.tree_help), "");
#endif

#ifdef HAVE_MT
  if(!ismain)
  {
    gdk_flush();
    GDK_THREADS_LEAVE();
  }
#endif
}

void gui_finish_download(int ismain)
{
  cfg.processing = FALSE;

#ifdef HAVE_MT
  if(!ismain)
  {
    GDK_THREADS_ENTER();
  }
#endif
  gdk_beep();

  gtk_widget_set_sensitive(gui_cfg.bt_rest, TRUE);
  gtk_widget_set_sensitive(gui_cfg.mea_rest, TRUE);
  gtk_widget_set_sensitive(gui_cfg.mtb_rest, TRUE);
  if(cfg.urlstack)
  {
    gtk_widget_set_sensitive(gui_cfg.bt_start, TRUE);
    gtk_widget_set_sensitive(gui_cfg.mea_start, TRUE);
    gtk_widget_set_sensitive(gui_cfg.mtb_start, TRUE);
  }
  gtk_widget_set_sensitive(gui_cfg.bt_stop, FALSE);
  gtk_widget_set_sensitive(gui_cfg.mea_stop, FALSE);
  gtk_widget_set_sensitive(gui_cfg.mtb_stop, FALSE);
  gtk_widget_set_sensitive(gui_cfg.bt_break, FALSE);
  gtk_widget_set_sensitive(gui_cfg.mea_break, FALSE);
  gtk_widget_set_sensitive(gui_cfg.mtb_break, FALSE);

#ifdef HAVE_MT
  if(!ismain)
  {
    gdk_flush();
    GDK_THREADS_LEAVE();
  }
#endif
  cfg.rbreak = FALSE;
  cfg.stop = FALSE;
}

void gui_finish_document(doc * docp)
{
#ifdef WITH_TREE
  if(cfg.xi_face && GTK_TOGGLE_BUTTON(gui_cfg.watch_download)->active)
  {
    GDK_THREADS_ENTER();
    LOCK_GTKTREE;
    gtk_ctree_select(GTK_CTREE(gui_cfg.tree_widget),
      docp->doc_url->tree_nfo[0]);
    gtk_ctree_node_moveto(GTK_CTREE(gui_cfg.tree_widget),
      docp->doc_url->tree_nfo[0], 0, 0.0, 0.0);
    UNLOCK_GTKTREE;
    GDK_THREADS_LEAVE();
  }
#endif
}

void gui_start(int *argc, char **argv)
{
  char *p;
  char gtkrc[PATH_MAX];

#ifdef __CYGWIN__
  snprintf(gtkrc, sizeof(gtkrc), "%s/%s-gtkrc", cfg.install_path, PACKAGE);
  gtk_rc_add_default_file(gtkrc);
#endif
  /* Parse the file ~/.pavuk-gtkrc. This makes Pavuk themeable :) */
  if((p = getenv("HOME")))
  {
    snprintf(gtkrc, sizeof(gtkrc), "%s/.%s-gtkrc", p, PACKAGE);
    gtk_rc_add_default_file(gtkrc);
  }

  gtk_init(argc, &argv);
  gkey_load();

  gui_cfg._go_bg = FALSE;
  gui_cfg.toplevel = NULL;
  gui_cfg.about_shell = NULL;
  gui_cfg.scn_load_shell = NULL;
  gui_cfg.scn_save_shell = NULL;
  gui_cfg.cfg_limits = NULL;
  gui_cfg.config_shell = NULL;
  gui_cfg.cfg_sch = NULL;
#ifdef WITH_TREE
  gui_cfg.tree_shell = NULL;
#endif
}

void gui_set_doccounter(void)
{
  char pom[256];

  if(cfg.xi_face)
  {
    GDK_THREADS_ENTER();
    sprintf(pom, gettext("Processed: %5ld"), cfg.process_cnt);
    gtk_label_set(GTK_LABEL(gui_cfg.status_done), pom);
    sprintf(pom, gettext("Queued: %5ld"), cfg.total_cnt);
    gtk_label_set(GTK_LABEL(gui_cfg.status_queue), pom);
    sprintf(pom, gettext("Failed: %4ld"), cfg.fail_cnt);
    gtk_label_set(GTK_LABEL(gui_cfg.status_fail), pom);
    sprintf(pom, gettext("Rejected: %5ld"), cfg.reject_cnt);
    gtk_label_set(GTK_LABEL(gui_cfg.status_rej), pom);
    gdk_flush();
    GDK_THREADS_LEAVE();

    _Xt_Serve;
  }
}

void gui_set_progress(char *sz, char *rate, char *etime, char *rtime)
{
#ifdef HAVE_MT
  int thrnr;

  thrnr = (int) pthread_getspecific(cfg.thrnr_key);

  GDK_THREADS_ENTER();
  gtk_clist_set_text(GTK_CLIST(gui_cfg.status_list), thrnr, 3, sz);
  gtk_clist_set_text(GTK_CLIST(gui_cfg.status_list), thrnr, 4, rate);
  gtk_clist_set_text(GTK_CLIST(gui_cfg.status_list), thrnr, 5, etime);
  gtk_clist_set_text(GTK_CLIST(gui_cfg.status_list), thrnr, 6, rtime);
  gdk_flush();
  GDK_THREADS_LEAVE();
#else
  char pom[40];

  snprintf(pom, sizeof(pom), "S: %s", sz);
  gtk_label_set(GTK_LABEL(gui_cfg.status_size), pom);
  snprintf(pom, sizeof(pom), "R: %s", rate);
  gtk_label_set(GTK_LABEL(gui_cfg.status_rate), pom);
  snprintf(pom, sizeof(pom), "ET: %s", etime);
  gtk_label_set(GTK_LABEL(gui_cfg.status_et), pom);
  snprintf(pom, sizeof(pom), "RT: %s", rtime);
  gtk_label_set(GTK_LABEL(gui_cfg.status_rt), pom);
  _Xt_Serve;
#endif
}

void gui_clear_status(void)
{
  if(cfg.xi_face)
  {
#ifdef HAVE_MT
    int thrnr;

    thrnr = (int) pthread_getspecific(cfg.thrnr_key);

    GDK_THREADS_ENTER();
    gtk_clist_set_text(GTK_CLIST(gui_cfg.status_list), thrnr, 1, "");
    gtk_clist_set_text(GTK_CLIST(gui_cfg.status_list), thrnr, 2, "");
    gtk_clist_set_text(GTK_CLIST(gui_cfg.status_list), thrnr, 3, "");
    gtk_clist_set_text(GTK_CLIST(gui_cfg.status_list), thrnr, 4, "");
    gtk_clist_set_text(GTK_CLIST(gui_cfg.status_list), thrnr, 5, "");
    gtk_clist_set_text(GTK_CLIST(gui_cfg.status_list), thrnr, 6, "");
    gdk_flush();
    GDK_THREADS_LEAVE();
#else
    gtk_entry_set_text(GTK_ENTRY(gui_cfg.minitb_label), "");
    gtk_label_set(GTK_LABEL(gui_cfg.status_size), "");
    gtk_label_set(GTK_LABEL(gui_cfg.status_rate), "");
    gtk_label_set(GTK_LABEL(gui_cfg.status_et), "");
    gtk_label_set(GTK_LABEL(gui_cfg.status_rt), "");
#endif
  }
}

void gui_set_status(char *text)
{

  if(cfg.xi_face)
  {
#ifdef HAVE_MT
    int thrnr;

    thrnr = (int) pthread_getspecific(cfg.thrnr_key);
    GDK_THREADS_ENTER();
    gtk_clist_set_text(GTK_CLIST(gui_cfg.status_list), thrnr, 2, text);
    gdk_flush();
    GDK_THREADS_LEAVE();
#else
    gtk_label_set(GTK_LABEL(gui_cfg.status_msg), text);
    _Xt_Serve;
#endif
  }
  else
  {
    DEBUG_USER(" - %s\n", text);
  }
}


void gui_set_msg(char *text, int tout)
{
  if(cfg.xi_face)
  {
    GDK_THREADS_ENTER();
    gtk_label_set(GTK_LABEL(gui_cfg.status_msg), text);
    gdk_flush();
    _Xt_Serve;
    GDK_THREADS_LEAVE();
  }
  else
  {
    DEBUG_USER(" - %s\n", text);
  }
}

void gui_set_url(char *str)
{
  if(cfg.xi_face)
  {
#ifdef HAVE_MT
    int thrnr;

    thrnr = (int) pthread_getspecific(cfg.thrnr_key);
    GDK_THREADS_ENTER();
    gtk_clist_set_text(GTK_CLIST(gui_cfg.status_list), thrnr, 1, str);
    gdk_flush();
    GDK_THREADS_LEAVE();
#else
    gtk_entry_set_text(GTK_ENTRY(gui_cfg.minitb_label), str);
#endif
  }
}

#ifdef WITH_TREE
void *gui_tree_make_entry(url * urlp)
{
  GtkCTreeNode *retv;
  GtkCTreeNode *parent;
  GdkBitmap *shape;
  GdkPixmap *pixmap;
  gchar *text[1];
  dllist *par;
  url *parurl;

  par = dllist_last(urlp->parent_url);

  if(par)
    parurl = (url *) par->data;
  else
    parurl = NULL;

  LOCK_GTKTREE;
  GDK_THREADS_ENTER();
  if(parurl && parurl->tree_nfo && parurl->tree_nfo[0])
    parent = (GtkCTreeNode *) parurl->tree_nfo[0];
  else
    parent = (GtkCTreeNode *) gui_cfg.root;

  g_return_val_if_fail(parent != NULL, NULL);

  if(urlp->status & URL_PROCESSED)
  {
    gchar *text;
    guint8 sp;
    GdkPixmap *p;
    GdkBitmap *m;
    gboolean il, ex;

    gtk_ctree_get_node_info(GTK_CTREE(gui_cfg.tree_widget),
      urlp->tree_nfo[0], &text, &sp, &p, &m, &pixmap, &shape, &il, &ex);
  }
  else
  {
    pixmap = gui_cfg.icon.notprocessed->pixmap;
    shape = gui_cfg.icon.notprocessed->shape;
  }

  text[0] = url_to_urlstr(urlp, FALSE);

  retv = gtk_ctree_insert_node(GTK_CTREE(gui_cfg.tree_widget), parent,
    NULL, text, 8, pixmap, shape, pixmap, shape, FALSE, TRUE);

  _free(text[0]);

  gtk_ctree_node_set_row_data(GTK_CTREE(gui_cfg.tree_widget), retv,
    (gpointer) urlp);

  _Xt_Serve;
  GDK_THREADS_LEAVE();
  UNLOCK_GTKTREE;

  return retv;
}
#endif

void gui_create_tree_root_node(void)
{
#ifdef WITH_TREE
  if(cfg.xi_face && !gui_cfg.root)
  {
    gchar *text[1];

    text[0] = gettext("Start");

    LOCK_GTKTREE;
    GDK_THREADS_ENTER();
    gui_cfg.root =
      (GtkCTreeNode *) gtk_ctree_insert_node(GTK_CTREE(gui_cfg.tree_widget),
      NULL, NULL, text, 8, gui_cfg.icon.notprocessed->pixmap,
      gui_cfg.icon.notprocessed->shape, gui_cfg.icon.notprocessed->pixmap,
      gui_cfg.icon.notprocessed->shape, FALSE, TRUE);
    GDK_THREADS_LEAVE();
    UNLOCK_GTKTREE;
  }
#endif /* WITH_TREE */
}

void gui_clear_tree(void)
{
#ifdef WITH_TREE
  if(cfg.xi_face && gui_cfg.root)
  {
    LOCK_GTKTREE;
    if(!MT_IS_MAIN_THREAD())
    {
      GDK_THREADS_ENTER();
    }
    gtk_clist_freeze(GTK_CLIST(gui_cfg.tree_widget));
    gtk_clist_clear(GTK_CLIST(gui_cfg.tree_widget));
    gtk_clist_thaw(GTK_CLIST(gui_cfg.tree_widget));

    gui_cfg.root = NULL;
    if(!MT_IS_MAIN_THREAD())
    {
      GDK_THREADS_LEAVE();
    }
    UNLOCK_GTKTREE;
  }
#endif /* WITH_TREE */
}

void gui_tree_add_start(void)
{
#if defined(WITH_TREE) && !defined(HAVE_MT)
  if(cfg.xi_face)
  {
    GDK_THREADS_ENTER();
    gtk_clist_freeze(GTK_CLIST(gui_cfg.tree_widget));
    GDK_THREADS_LEAVE();
  }
#endif

}

void gui_tree_add_end(void)
{
#if defined(WITH_TREE) && !defined(HAVE_MT)
  if(cfg.xi_face)
  {
    GDK_THREADS_ENTER();
    gtk_clist_thaw(GTK_CLIST(gui_cfg.tree_widget));
    GDK_THREADS_LEAVE();
  }
#endif
}

void gui_tree_set_icon_for_doc(doc * docp)
{
  icons_set_for_doc(docp);
}

#ifndef HAVE_MT
void gui_loop_serve(void)
{
  if(cfg.xi_face)
  {
    gui_cfg.endloop = FALSE;
    while(!gui_cfg.endloop && gdk_events_pending() && !gtk_main_iteration());
    if(gui_cfg._go_bg)
      gui_do_ui_cleanup();
  }
}

void gui_loop_do(void)
{
  if(cfg.xi_face)
  {
    gui_cfg.endloop = FALSE;
    while(!gui_cfg.endloop && !cfg.rbreak)
    {
      gtk_main_iteration();
    }
    if(gui_cfg._go_bg)
      gui_do_ui_cleanup();
  }
}

void gui_loop_escape(void)
{
  if(cfg.xi_face)
    gui_cfg.endloop = TRUE;
}
#endif /* HAVE_MT */

#ifdef HAVE_MT

void gui_mt_thread_start(int thrnr)
{
  if(cfg.xi_face)
  {
    char *ep[7];
    char pom[10];

    sprintf(pom, "%d", thrnr);
    ep[0] = pom;
    ep[1] = "";
    ep[2] = gettext("Starting ...");
    ep[3] = "";
    ep[4] = "";
    ep[5] = "";
    ep[6] = "";
    GDK_THREADS_ENTER();
    gtk_clist_append(GTK_CLIST(gui_cfg.status_list), ep);
    GDK_THREADS_LEAVE();
  }
}

void gui_mt_thread_end(int thrnr)
{
  if(cfg.xi_face)
  {
    GDK_THREADS_ENTER();
    gtk_clist_clear(GTK_CLIST(gui_cfg.status_list));
    GDK_THREADS_LEAVE();
  }
}

#endif

static gint TOut(gpointer data)
{
  *(int *) data = 0;
  _Xt_EscLoop;
  return FALSE;
}

void gui_msleep(long msec)
{
  gint inid;

  inid = gtk_timeout_add(msec, (GtkFunction) TOut, &inid);
  _Xt_ServeLoop;
  if(inid)
    gtk_timeout_remove(inid);
}

static void rwCB(gpointer data, gint source, GdkInputCondition condition)
{
  gdk_input_remove(*(int *) data);
  *(int *) data = 0;
  _Xt_EscLoop;
}

int gui_wait_io(int sock, int for_read)
{
  int rv = 0;
  gint iid = 0;
  gint inid = 0;

  if(cfg.ctimeout > 0.0)
    inid = gtk_timeout_add((int) (cfg.ctimeout * 1000.0),
      (GtkFunction) TOut, &inid);

  if(cfg.ctimeout > (double) (ULONG_MAX / 1000))
    xprintf(0,
      gettext
      ("Too high timeout value for GUI interface implementation of timeout\n"));

  iid = gdk_input_add(sock, for_read ? GDK_INPUT_READ : GDK_INPUT_WRITE,
    (GdkInputFunction) rwCB, (gpointer) & iid);

  _Xt_ServeLoop;
  if(inid)
  {
    gtk_timeout_remove(inid);
    inid = 0;
  }
  else if(cfg.ctimeout > 0.0)
  {
    errno = ETIMEDOUT;
    rv = -1;
  }

  if(iid)
  {
    gdk_input_remove(iid);
    iid = 0;
  }

  return rv;
}

int gui_xprint(char *str)
{
  char *p;
  bool_t last = 1;
  int ilen;
  char *pline = NULL;
  int row;

  LOCK_GTKLOG;
  GDK_THREADS_ENTER();
  gtk_clist_freeze(GTK_CLIST(gui_cfg.logw));
  p = str;
  if(GTK_CLIST(gui_cfg.logw)->rows)
  {
    row = GTK_CLIST(gui_cfg.logw)->rows - 1;
    if(gtk_clist_get_row_data(GTK_CLIST(gui_cfg.logw), row))
    {
      gtk_clist_get_text(GTK_CLIST(gui_cfg.logw), row, 0, &pline);
    }
  }
  while(*p)
  {
    ilen = strcspn(p, "\r\n");
    if(*(p + ilen))
      *(p + ilen) = '\0';
    else
      last = 0;

    if(pline)
    {
      row = GTK_CLIST(gui_cfg.logw)->rows - 1;

      pline = g_strconcat(pline, p, NULL);
      gtk_clist_set_text(GTK_CLIST(gui_cfg.logw), row, 0, pline);
      g_free(pline);
      pline = NULL;

      gtk_clist_set_row_data(GTK_CLIST(gui_cfg.logw), row, (gpointer) ! last);
    }
    else
    {
      row = gtk_clist_append(GTK_CLIST(gui_cfg.logw), &p);

      if(!last)
        gtk_clist_set_row_data(GTK_CLIST(gui_cfg.logw),
          row, (gpointer) ! last);
    }

    p += ilen + last;
    p += strspn(p, "\r\n");
  }

  if(cfg.xlogsize)
  {
    while(cfg.xlogsize < GTK_CLIST(gui_cfg.logw)->rows)
      gtk_clist_remove(GTK_CLIST(gui_cfg.logw), 0);
  }

  gtk_clist_thaw(GTK_CLIST(gui_cfg.logw));
  if(cfg.log_autoscroll)
  {
    gui_cfg.logvadj->value = gui_cfg.logvadj->upper >
      gui_cfg.logvadj->page_size ? gui_cfg.logvadj->upper -
      gui_cfg.logvadj->page_size : 0;
    gtk_signal_emit_by_name(GTK_OBJECT(gui_cfg.logvadj), "value_changed");
  }
  GDK_THREADS_LEAVE();
  UNLOCK_GTKLOG;

  return strlen(str);
}

int gui_xvaprintf(char *strs, va_list * args)
{
  int rv;
  char *buf = g_strdup_vprintf(strs, *args);

  rv = gui_xprint(buf);

  g_free(buf);

  return rv;
}

void gui_do_ui_cleanup(void)
{
  if(cfg.xi_face)
  {
    cfg.xi_face = FALSE;

#ifdef HAVE_MT
    if(cfg.processing)
    {
      GDK_THREADS_LEAVE();
      tl_sleep(2);
    }
#endif

    if(gui_cfg.cfg_sch)
      gtk_widget_destroy(gui_cfg.cfg_sch);
    if(gui_cfg.about_shell)
      gtk_widget_destroy(gui_cfg.about_shell);
    if(gui_cfg.scn_load_shell)
      gtk_widget_destroy(gui_cfg.scn_load_shell);
    if(gui_cfg.scn_add_shell)
      gtk_widget_destroy(gui_cfg.scn_add_shell);
    if(gui_cfg.scn_save_shell)
      gtk_widget_destroy(gui_cfg.scn_save_shell);
    if(gui_cfg.cfg_limits)
      gtk_widget_destroy(gui_cfg.cfg_limits);
    if(gui_cfg.config_shell)
      gtk_widget_destroy(gui_cfg.config_shell);
#ifdef WITH_TREE
    if(gui_cfg.tree_shell)
      gtk_widget_destroy(gui_cfg.tree_shell);
#endif

    if(gui_cfg.toplevel)
      gtk_widget_destroy(gui_cfg.toplevel);

    tl_msleep(100);

#ifndef __CYGWIN__
    XCloseDisplay(gdk_display);
#endif
#ifndef HAVE_MT
    dns_server_kill();
#endif
  }
}

#endif /* GTK_FACE */
