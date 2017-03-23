/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"

#ifdef WITH_TREE
#ifdef GTK_FACE

#include <gdk/gdkkeysyms.h>
#include <stdio.h>
#include <unistd.h>

#include "form.h"
#include "gaccel.h"
#include "gprop.h"
#include "gui_api.h"
#include "gui.h"
#include "recurse.h"

#include "icons/save.xpm"
#include "icons/cancel.xpm"

#define TREE_GET_URL \
  ((url *) gtk_ctree_node_get_row_data(GTK_CTREE(gui_cfg.tree_widget) , \
  (GtkCTreeNode *)(GTK_CLIST(gui_cfg.tree_widget)->selection->data)))
#define TEST_URL_NODE \
  (GTK_CLIST(gui_cfg.tree_widget)->selection && TREE_GET_URL)

static gint no_destroy(GtkWidget * widget, GdkEvent * event, gpointer data)
{
  if(!gui_cfg._go_bg)
  {
    gtk_widget_hide(GTK_WIDGET(data));
    return (TRUE);
  }
  else
    return (FALSE);
}

void gui_SelectTreeNode(GtkObject * object, gpointer func_data)
{
  gtk_label_set(GTK_LABEL(gui_cfg.tree_help), " ");
}

static void LaunchBrowser(GtkObject * object, gpointer func_data)
{
  char pom[2048];
  char *p;
  url *urlp;

  if(!cfg.browser || !TEST_URL_NODE)
  {
    gdk_beep();
    return;
  }

  urlp = TREE_GET_URL;

  p = tl_strdup(url_to_filename(urlp, TRUE));

  if(access(p, R_OK))
  {
    free(p);
    p = url_to_urlstr(urlp, FALSE);
  }

  snprintf(pom, sizeof(pom), "%s %s &", cfg.browser, p);
  tl_system(pom);

  free(p);
}

static void Propert(GtkObject * object, gpointer func_data)
{
  char pom[4096];
  url_prop *prp;
  url *urlp;
  char *p;
  char *p1;

  if(!TEST_URL_NODE)
  {
    gdk_beep();
    return;
  }

  urlp = TREE_GET_URL;

  prp = urlp->prop;

  p = url_to_urlstr(urlp, FALSE);
  snprintf(pom, sizeof(pom), gettext("URL: %s\n"), p);
  _free(p);
  p = g_strdup(pom);

  if(urlp->type == URLT_HTTP || urlp->type == URLT_HTTPS)
  {
    if(urlp->status & URL_FORM_ACTION)
    {
      form_info *fi = (form_info *) urlp->extension;
      dllist *ptr;

      if(fi->method == FORM_M_GET)
      {
        p1 = g_strconcat(p, gettext("Request type: "), "GET\n", NULL);
        _free(p);
        p = p1;
      }
      else
      {
        p1 = g_strconcat(p, gettext("Request type: "), "POST\n", NULL);
        _free(p);
        p = p1;

        p1 = g_strconcat(p, gettext("Request encoding: "),
          (fi->encoding == FORM_E_MULTIPART) ?
          "form/multipart\n" : "application/x-www-urlencoded\n", NULL);
        _free(p);
        p = p1;
      }
      ptr = fi->infos;

      if(ptr)
      {
        p1 = g_strconcat(p, gettext("Query values:\n"), NULL);
        _free(p);
        p = p1;
      }
      while(ptr)
      {
        form_field *ff = (form_field *) ptr->data;
        char *name, *value;

        name = form_encode_urlencoded_str(ff->name);
        value = form_encode_urlencoded_str(ff->value);

        p1 = g_strconcat(p, "    ", name, " = ", value, "\n", NULL);
        _free(p);
        p = p1;

        _free(name);
        _free(value);

        ptr = ptr->next;
      }
    }
    else
    {
      p1 = g_strconcat(p, gettext("Request type: "), "GET\n", NULL);
      _free(p);
      p = p1;
    }
  }

  p1 = g_strconcat(p, gettext("Status: "), NULL);
  _free(p);
  p = p1;

  if(!(urlp->status & URL_PROCESSED))
  {
    p1 = g_strconcat(p, gettext("not processed yet\n"), NULL);
    _free(p);
    p = p1;
  }
  else if(urlp->status & URL_INNSCACHE)
  {
    p1 = g_strconcat(p, gettext("loaded from NS cache\n"), NULL);
    _free(p);
    p = p1;
  }
  else if(urlp->status & URL_REDIRECT)
  {
    p1 = g_strconcat(p, gettext("loaded from local URL tree\n"), NULL);
    _free(p);
    p = p1;
  }
  else if(urlp->status & URL_MOVED)
  {
    p1 = g_strconcat(p, gettext("moved to another URL\n"), NULL);
    _free(p);
    p = p1;
  }
  else if(urlp->status & URL_NOT_FOUND)
  {
    p1 = g_strconcat(p, gettext("URL not found on remote server\n"), NULL);
    _free(p);
    p = p1;
  }
  else if(urlp->status & URL_TRUNCATED)
  {
    p1 = g_strconcat(p, gettext("truncated\n"), NULL);
    _free(p);
    p = p1;
  }
  else if(urlp->status & URL_DOWNLOADED)
  {
    p1 = g_strconcat(p, gettext("downloaded OK\n"), NULL);
    _free(p);
    p = p1;
  }
  else if(urlp->status & URL_REJECTED)
  {
    p1 = g_strconcat(p, gettext("rejected by rules\n"), NULL);
    _free(p);
    p = p1;
  }
  else if(urlp->status & URL_USER_DISABLED)
  {
    p1 = g_strconcat(p, gettext("disabled by user\n"), NULL);
    _free(p);
    p = p1;
  }
  else if(urlp->status & URL_ERR_REC)
  {
    p1 = g_strconcat(p, gettext("probably recoverable error\n"), NULL);
    _free(p);
    p = p1;
  }
  else if(urlp->status & URL_ERR_UNREC)
  {
    p1 = g_strconcat(p, gettext("unrecoverable error\n"), NULL);
    _free(p);
    p = p1;
  }
  else
  {
    p1 = g_strconcat(p, gettext("unknown\n"), NULL);
    _free(p);
    p = p1;
  }


  if(urlp->moved_to)
  {
    char *ps = url_to_urlstr(urlp->moved_to, FALSE);

    snprintf(pom, sizeof(pom), gettext("Moved to URL: %s\n"), ps);
    _free(ps);
    p1 = g_strconcat(p, pom, NULL);
    _free(p);
    p = p1;
  }

  if(prp)
  {
    if(prp->type)
      snprintf(pom, sizeof(pom), gettext("Type: %s\n"), gettext(prp->type));
    else
      snprintf(pom, sizeof(pom), gettext("Type: unknown\n"));

    p1 = g_strconcat(p, pom, NULL);
    _free(p);
    p = p1;

    sprintf(pom, gettext("Size: %d\n"), prp->size);
    p1 = g_strconcat(p, pom, NULL);
    _free(p);
    p = p1;

    if(prp->mdtm)
    {
      LOCK_TIME;
      strftime(pom, sizeof(pom),
        gettext("Modification time: %a, %d %b %Y %H:%M:%S %Z\n"),
        localtime(&prp->mdtm));
      UNLOCK_TIME;
      p1 = g_strconcat(p, pom, NULL);
      _free(p);
      p = p1;
    }
  }

  if(urlp->local_name)
  {
    snprintf(pom, sizeof(pom), gettext("Local filename: %s\n"), urlp->local_name);
    p1 = g_strconcat(p, pom, NULL);
    _free(p);
    p = p1;
  }

  if(urlp->ref_cnt)
  {
    dllist *ptr;
    char *us;

    p1 = g_strconcat(p, gettext("Parent URLs:\n"), NULL);
    _free(p);
    p = p1;
    LOCK_URL(urlp);
    for(ptr = urlp->parent_url; ptr; ptr = ptr->next)
    {
      url *pomurl = (url *) ptr->data;

      us = url_to_urlstr(pomurl, FALSE);
      snprintf(pom, sizeof(pom), "      %s\n", us);
      free(us);
      p1 = g_strconcat(p, pom, NULL);
      _free(p);
      p = p1;
    }
    UNLOCK_URL(urlp);
  }

  if(!p)
    p = "";

  gtk_label_set(GTK_LABEL(gui_cfg.tree_help), p);
  g_free(p);
}

static void item_set_disabled(GtkCTreeNode * widget, gboolean disabled)
{
  static GtkStyle *disabled_style = NULL;
  static GtkStyle *normal_style = NULL;

  LOCK_GTKTREE;
  if(!disabled_style)
  {
    normal_style =
      gtk_ctree_node_get_cell_style(GTK_CTREE(gui_cfg.tree_widget), widget,
      0);
    if(!normal_style)
      normal_style = gtk_widget_get_style(gui_cfg.tree_widget);
    disabled_style = normal_style ? gtk_style_copy(normal_style) :
      gtk_style_new();

    disabled_style->fg[GTK_STATE_NORMAL] =
      disabled_style->fg[GTK_STATE_INSENSITIVE];
    disabled_style->bg[GTK_STATE_NORMAL] =
      disabled_style->bg[GTK_STATE_INSENSITIVE];
    disabled_style->base[GTK_STATE_NORMAL] =
      disabled_style->base[GTK_STATE_INSENSITIVE];
  }

  gtk_ctree_node_set_cell_style(GTK_CTREE(gui_cfg.tree_widget), widget, 0,
    disabled ? disabled_style : normal_style);

  UNLOCK_GTKTREE;
}


static void DisableURL(GtkObject * object, gpointer func_data)
{
  url *urlp;
  int i;

  if(!TEST_URL_NODE)
  {
    gdk_beep();
    return;
  }

  urlp = TREE_GET_URL;

  urlp->status |= URL_USER_DISABLED;

  LOCK_URL(urlp);
  for(i = 0; i < urlp->ref_cnt; i++)
    item_set_disabled(urlp->tree_nfo[i], TRUE);
  UNLOCK_URL(urlp);
}

static void EnableURL(GtkObject * object, gpointer func_data)
{
  url *urlp;
  int i;

  if(!TEST_URL_NODE)
  {
    gdk_beep();
    return;
  }

  urlp = TREE_GET_URL;

  urlp->status &= ~URL_USER_DISABLED;

  LOCK_URL(urlp);
  for(i = 0; i < urlp->ref_cnt; i++)
    item_set_disabled(urlp->tree_nfo[i], FALSE);
  UNLOCK_URL(urlp);

  if(urlp->status & URL_PROCESSED)
  {
    urlp->status &= ~URL_PROCESSED;

    LOCK_CFG_URLSTACK;
    cfg.urlstack = dllist_append(cfg.urlstack, (dllist_t) urlp);
    cfg.total_cnt++;
    UNLOCK_CFG_URLSTACK;
  }
}

static void DownloadThisURL(GtkObject * object, gpointer func_data)
{
  url *urlp;

  if(!TEST_URL_NODE || cfg.processing)
  {
    gdk_beep();
    return;
  }

  urlp = TREE_GET_URL;

#ifdef HAVE_MT
  {
    pthread_attr_t thrdattr;

    pthread_attr_init(&thrdattr);
    pthread_attr_setscope(&thrdattr, PTHREAD_SCOPE_SYSTEM);
    pthread_attr_setstacksize(&thrdattr, MT_STACK_SIZE);

    gui_start_download(TRUE);

    if(pthread_create(&cfg.mainthread,
        &thrdattr, (void *) download_single_doc, (void *) urlp))
    {
      xperror("Create downloading thread");
      gui_finish_download(TRUE);
    }
  }
#else
  download_single_doc(urlp);
#endif

}

gint gui_tree_list_events(GtkWidget * widget, GdkEvent * event)
{
  GdkEventButton *bevent;
  url *urlp;

  if(!TEST_URL_NODE)
    return FALSE;

  switch (event->type)
  {
  case GDK_BUTTON_PRESS:
    bevent = (GdkEventButton *) event;
    if(bevent->button == 3)
    {
      urlp = TREE_GET_URL;

      if(!urlp)
      {
        gtk_widget_set_sensitive(gui_cfg.me_disable_url, FALSE);
        gtk_widget_set_sensitive(gui_cfg.me_enable_url, FALSE);
        gtk_widget_set_sensitive(gui_cfg.me_browse_url, FALSE);
        gtk_widget_set_sensitive(gui_cfg.me_prop_url, FALSE);
        gtk_widget_set_sensitive(gui_cfg.me_download_url, FALSE);
      }
      else
      {
        gtk_widget_set_sensitive(gui_cfg.me_browse_url, TRUE);
        gtk_widget_set_sensitive(gui_cfg.me_prop_url, TRUE);

        if(urlp->status & URL_PROCESSED
          && !(urlp->status & URL_USER_DISABLED))
        {
          gtk_widget_set_sensitive(gui_cfg.me_disable_url, FALSE);
          gtk_widget_set_sensitive(gui_cfg.me_enable_url, FALSE);
          gtk_widget_set_sensitive(gui_cfg.me_download_url,
            (urlp->status & URL_REJECTED) ||
            (urlp->status & URL_TRUNCATED) || (urlp->status & URL_ERR_REC));
        }
        else
        {
          gtk_widget_set_sensitive(gui_cfg.me_disable_url,
            !(urlp->status & URL_USER_DISABLED));
          gtk_widget_set_sensitive(gui_cfg.me_enable_url,
            (urlp->status & URL_USER_DISABLED));
          gtk_widget_set_sensitive(gui_cfg.me_download_url,
            cfg.processing ? FALSE : !(urlp->status & URL_USER_DISABLED));
        }
      }
      gtk_menu_popup(GTK_MENU(gui_cfg.tmenu), NULL, NULL, NULL, NULL, 3,
        bevent->time);
    }
    break;
  default:
    break;
  }

  return FALSE;
}

static void DumpTree(GtkCTree * tree, GtkCTreeNode * node, gpointer data)
{
  url *urlp = (url *) gtk_ctree_node_get_row_data(tree, node);

  if(urlp)
  {
    int i;
    char *p;
    FILE *f = (FILE *) data;
    GtkCTreeRow *row = (GtkCTreeRow *) node->list.data;

    for(i = 2; i < row->level; i++)
      fprintf(f, "    ");

    p = url_to_urlstr(urlp, FALSE);
    fprintf(f, "%s\n", p);
    _free(p);
  }
}

static void StoreTreeOK(GtkWidget * w, gpointer fdata)
{
  const char *p = gtk_file_selection_get_filename(GTK_FILE_SELECTION(fdata));
  FILE *f;

  if(!p || !*p)
  {
    gdk_beep();
    return;
  }

  if(!(f = fopen(p, "wb+")))
  {
    xperror(p);
    return;
  }
  LOCK_GTKTREE;
  gtk_ctree_pre_recursive(GTK_CTREE(gui_cfg.tree_widget), gui_cfg.root,
    GTK_CTREE_FUNC(DumpTree), f);
  UNLOCK_GTKTREE;

  fclose(f);

  gtk_widget_destroy(GTK_WIDGET(fdata));
}

static void StoreTree(void)
{
  static GtkWidget *fsw = NULL;

  if(!fsw)
  {
    fsw = gtk_file_selection_new(gettext("Pavuk: Store tree"));
    gtk_signal_connect(GTK_OBJECT(fsw), "destroy",
      GTK_SIGNAL_FUNC(gtk_widget_destroyed), &fsw);

    gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(fsw)->ok_button),
      "clicked", GTK_SIGNAL_FUNC(StoreTreeOK), fsw);

    gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(fsw)->cancel_button),
      "clicked", GTK_SIGNAL_FUNC(guitl_PopdownW), fsw);
  }

  gtk_widget_show(fsw);
  if(GTK_WIDGET_REALIZED(fsw))
    gdk_window_raise(fsw->window);
}

static void pane_resize(GtkWidget * widget, GtkAllocation * sallocation)
{
  gprop_set_int("tree_height", sallocation->height);
}

static void top_resize(GtkWidget * widget, GtkAllocation * sallocation)
{
  gprop_set_int("treeview_width", sallocation->width);
  gprop_set_int("treeview_height", sallocation->height);
}

#if GTK_FACE < 2
static void gtk_button_set_label(GtkButton * butt, const char *text)
{
  GList *chlist;
  GtkWidget *w;

  w = butt->child;
  for(chlist = GTK_BOX(w)->children; chlist; chlist = chlist->next)
  {
    if(GTK_IS_LABEL(((struct _GtkBoxChild *) chlist->data)->widget))
    {
      gtk_label_set(GTK_LABEL(((struct _GtkBoxChild *) chlist->data)->widget),
      text);
    }
  }
}
#endif

void gui_build_tree_preview(int popup)
{
  GtkWidget *col, *swin, *frame, *mi, *bbox, *pane;
  GdkFont *font;
  int h, w;
  static GtkWidget *cbutton = NULL;
  static GtkWidget *sbutton = NULL;
  GtkAccelGroup *accel_group;

  if(gui_cfg.tree_shell)
  {
    gtk_window_set_title(GTK_WINDOW(gui_cfg.tree_shell),
      gettext("Pavuk: URL tree preview"));
    gtk_button_set_label(GTK_BUTTON(cbutton), gettext("Cancel"));
    gtk_button_set_label(GTK_BUTTON(sbutton), gettext("Store tree ..."));
    gtk_button_set_label(GTK_BUTTON(gui_cfg.watch_download),
      gettext("Automaticaly watch last processed URLs"));
    gtk_label_set(GTK_LABEL(GTK_BIN(gui_cfg.me_prop_url)->child),
      gettext("Properties"));
    gtk_label_set(GTK_LABEL(GTK_BIN(gui_cfg.me_browse_url)->child),
      gettext("Launch browser"));
    gtk_label_set(GTK_LABEL(GTK_BIN(gui_cfg.me_disable_url)->child),
      gettext("Disable URL"));
    gtk_label_set(GTK_LABEL(GTK_BIN(gui_cfg.me_enable_url)->child),
      gettext("Enable URL"));
    gtk_label_set(GTK_LABEL(GTK_BIN(gui_cfg.me_download_url)->child),
      gettext("Download URL"));
    gtk_label_set(GTK_LABEL(gui_cfg.tree_help), "");
    gtk_clist_set_column_title(GTK_CLIST(gui_cfg.tree_widget), 0,
      gettext("URL tree"));

    if(popup)
    {
      gtk_widget_show(gui_cfg.tree_shell);
      if(GTK_WIDGET_REALIZED(gui_cfg.tree_shell))
        gdk_window_raise(gui_cfg.tree_shell->window);
    }
    return;
  }

  gui_cfg.tree_shell = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_container_border_width(GTK_CONTAINER(gui_cfg.tree_shell), 3);
  gtk_window_set_title(GTK_WINDOW(gui_cfg.tree_shell),
    gettext("Pavuk: URL tree preview"));
  gtk_signal_connect(GTK_OBJECT(gui_cfg.tree_shell), "delete_event",
    GTK_SIGNAL_FUNC(no_destroy), gui_cfg.tree_shell);

  w = -1;
  h = -1;
  gprop_get_int("treeview_height", &h);
  gprop_get_int("treeview_width", &w);
  gtk_window_set_default_size(GTK_WINDOW(gui_cfg.tree_shell), w, h);

  gtk_signal_connect(GTK_OBJECT(gui_cfg.tree_shell), "size_allocate",
    GTK_SIGNAL_FUNC(top_resize), NULL);
  gtk_window_set_policy(GTK_WINDOW(gui_cfg.tree_shell), FALSE, TRUE, TRUE);

  col = gtk_vbox_new(0, 5);
  gtk_container_add(GTK_CONTAINER(gui_cfg.tree_shell), col);
  gtk_widget_show(col);

  gui_cfg.watch_download =
    gtk_check_button_new_with_label(gettext
    ("Automaticaly watch last processed URLs"));
  gtk_box_pack_start(GTK_BOX(col), gui_cfg.watch_download, FALSE, FALSE, 1);
  gtk_widget_show(gui_cfg.watch_download);

  pane = gtk_vpaned_new();
  gtk_box_pack_start(GTK_BOX(col), pane, TRUE, TRUE, 1);
  gtk_widget_show(pane);

  swin = gtk_scrolled_window_new(NULL, NULL);
  if(!gprop_get_int("tree_height", &h))
    h = 400;
  gtk_widget_set_usize(swin, 500, h);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin),
    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_widget_show(swin);
  gtk_container_add(GTK_CONTAINER(pane), swin);

  gui_cfg.tree_widget = gtk_ctree_new(1, 0);
  gtk_clist_set_column_title(GTK_CLIST(gui_cfg.tree_widget), 0,
    gettext("URL tree"));
  gtk_clist_column_titles_show(GTK_CLIST(gui_cfg.tree_widget));
#if GTK_FACE < 2
  font = gui_cfg.tree_widget->style->font;
#else
  font = gtk_style_get_font(gui_cfg.tree_widget->style);
#endif
  gtk_clist_set_row_height(GTK_CLIST(gui_cfg.tree_widget),
    TL_MAX(font->ascent + font->descent + 1, MAX_PIX_HEIGHT));
  gtk_clist_set_column_width(GTK_CLIST(gui_cfg.tree_widget), 0, 500);
  gtk_clist_set_selection_mode(GTK_CLIST(gui_cfg.tree_widget),
    GTK_SELECTION_SINGLE);
  gtk_clist_set_column_auto_resize(GTK_CLIST(gui_cfg.tree_widget), 0, TRUE);
  gtk_ctree_set_line_style(GTK_CTREE(gui_cfg.tree_widget),
    GTK_CTREE_LINES_DOTTED);
  gtk_signal_connect(GTK_OBJECT(gui_cfg.tree_widget), "button_press_event",
    (GtkSignalFunc) gui_tree_list_events, gui_cfg.tree_widget);
  gtk_signal_connect(GTK_OBJECT(gui_cfg.tree_widget), "select_row",
    (GtkSignalFunc) gui_SelectTreeNode, (gpointer) NULL);
  gtk_container_add(GTK_CONTAINER(swin), gui_cfg.tree_widget);
  gtk_widget_show(gui_cfg.tree_widget);

  swin = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin),
    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_widget_show(swin);
  gtk_container_add(GTK_CONTAINER(pane), swin);

  frame = gtk_frame_new(NULL);
  gtk_widget_show(frame);
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(swin), frame);

  gui_cfg.tree_help = gtk_label_new("   ");
  gtk_label_set_justify(GTK_LABEL(gui_cfg.tree_help), GTK_JUSTIFY_LEFT);
  gtk_misc_set_alignment(GTK_MISC(gui_cfg.tree_help), 0.0, 0.0);
  gtk_misc_set_padding(GTK_MISC(gui_cfg.tree_help), 3, 3);
  gtk_widget_show(gui_cfg.tree_help);
  gtk_container_add(GTK_CONTAINER(frame), gui_cfg.tree_help);


  gtk_signal_connect(GTK_OBJECT(GTK_PANED(pane)->child1), "size_allocate",
    GTK_SIGNAL_FUNC(pane_resize), NULL);

  bbox = gtk_hbutton_box_new();
  gtk_box_pack_start(GTK_BOX(col), bbox, FALSE, FALSE, 0);
  gtk_widget_show(bbox);

  sbutton = guitl_pixmap_button(save_xpm, NULL, gettext("Store tree ..."));
  gtk_container_add(GTK_CONTAINER(bbox), sbutton);
  gtk_signal_connect(GTK_OBJECT(sbutton), "clicked",
    GTK_SIGNAL_FUNC(StoreTree), (gpointer) NULL);
  GTK_WIDGET_SET_FLAGS(sbutton, GTK_CAN_DEFAULT);
  gtk_widget_show(sbutton);

  cbutton = guitl_pixmap_button(cancel_xpm, NULL, gettext("Cancel"));

  accel_group = gtk_accel_group_new();
  gtk_widget_add_accelerator(cbutton, "clicked", accel_group,
    GDK_Escape, 0, GTK_ACCEL_VISIBLE);
  gtk_window_add_accel_group(GTK_WINDOW(gui_cfg.tree_shell), accel_group);

  gtk_container_add(GTK_CONTAINER(bbox), cbutton);
  gtk_signal_connect(GTK_OBJECT(cbutton), "clicked",
    GTK_SIGNAL_FUNC(guitl_PopdownW), (gpointer) gui_cfg.tree_shell);
  GTK_WIDGET_SET_FLAGS(cbutton, GTK_CAN_DEFAULT);
  gtk_widget_grab_default(cbutton);
  gtk_widget_show(cbutton);

  gui_cfg.tmenu = gtk_menu_new();
  guitl_menu_attach(gui_cfg.tmenu, gui_cfg.tree_shell);
  gtk_widget_realize(gui_cfg.tmenu);

  gui_cfg.me_prop_url = mi =
    gtk_menu_item_new_with_label(gettext("Properties"));
  gaccel_bind_widget("tree/properties", "activate", mi, NULL,
    gui_cfg.tree_shell);
  gtk_menu_append(GTK_MENU(gui_cfg.tmenu), mi);
  gtk_widget_show(mi);
  gtk_signal_connect(GTK_OBJECT(mi), "activate",
    GTK_SIGNAL_FUNC(Propert), (gpointer) NULL);

  gui_cfg.me_browse_url = mi =
    gtk_menu_item_new_with_label(gettext("Launch browser"));
  gaccel_bind_widget("tree/launch", "activate", mi, NULL, gui_cfg.tree_shell);
  gtk_menu_append(GTK_MENU(gui_cfg.tmenu), mi);
  gtk_widget_show(mi);
  gtk_signal_connect(GTK_OBJECT(mi), "activate",
    GTK_SIGNAL_FUNC(LaunchBrowser), (gpointer) NULL);

  gui_cfg.me_disable_url = mi =
    gtk_menu_item_new_with_label(gettext("Disable URL"));
  gaccel_bind_widget("tree/disable", "activate", mi, NULL,
    gui_cfg.tree_shell);
  gtk_menu_append(GTK_MENU(gui_cfg.tmenu), mi);
  gtk_widget_show(mi);
  gtk_signal_connect(GTK_OBJECT(mi), "activate",
    GTK_SIGNAL_FUNC(DisableURL), (gpointer) NULL);

  gui_cfg.me_enable_url = mi =
    gtk_menu_item_new_with_label(gettext("Enable URL"));
  gaccel_bind_widget("tree/enable", "activate", mi, NULL, gui_cfg.tree_shell);
  gtk_menu_append(GTK_MENU(gui_cfg.tmenu), mi);
  gtk_widget_show(mi);
  gtk_signal_connect(GTK_OBJECT(mi), "activate",
    GTK_SIGNAL_FUNC(EnableURL), (gpointer) NULL);

  gui_cfg.me_download_url = mi =
    gtk_menu_item_new_with_label(gettext("Download URL"));
  gaccel_bind_widget("tree/download", "activate", mi, NULL,
    gui_cfg.tree_shell);
  gtk_menu_append(GTK_MENU(gui_cfg.tmenu), mi);
  gtk_widget_show(mi);
  gtk_signal_connect(GTK_OBJECT(mi), "activate",
    GTK_SIGNAL_FUNC(DownloadThisURL), (gpointer) NULL);

  if(popup)
    gtk_widget_show(gui_cfg.tree_shell);
}

#endif  /*** GTK_FACE ***/
#endif /*** WITH_TREE ***/
