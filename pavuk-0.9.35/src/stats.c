/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"

#include <string.h>
#include <stdio.h>

#include "stats.h"
#ifdef GTK_FACE
#include "gui.h"
#include "gui_api.h"
#endif

int stats_fill_spage(const char *filename, void *lst)
{
  char *pom, *p, p2[8192];
  int total = 0, redir = 0, nsredir = 0, dnld = 0, noproc = 0, moved = 0;
  int brokennr = 0, dexistnr = 0, ferrnr = 0, nferrnr = 0, unsnr = 0;
  int startnr = 0;
  int i;
  dllist *parptr;
  dllist *broken = NULL, *dexist = NULL, *ferr = NULL;
  dllist *nferr = NULL, *uns = NULL, *start = NULL;
  dllist *ptr;
  url *urlp;

  pom = p2;

  if(!cfg.mode_started)
    return -1;

  LOCK_CFG_URLHASH;
  switch (cfg.mode)
  {
  case MODE_NORMAL:
  case MODE_LNUPD:
  case MODE_SYNC:
  case MODE_MIRROR:
  case MODE_SINGLE:
  case MODE_SREGET:
  case MODE_RESUME:
  case MODE_NOSTORE:
    {
      for(i = 0; i < cfg.url_hash_tbl->size; i++)
      {
        ptr = cfg.url_hash_tbl->nodes[i];

        while(ptr)
        {
          url *urlp = (url *) ptr->data;

          total++;

          if(urlp->status & URL_ISSTARTING)
          {
            startnr++;
            start = dllist_append(start, (dllist_t) urlp);
          }

          if(urlp->status & URL_PROCESSED)
          {
            if(urlp->status & URL_INNSCACHE)
              nsredir++;
            else if(urlp->status & URL_REDIRECT)
              redir++;
            else if(urlp->status & URL_DOWNLOADED)
              dnld++;
            else if(urlp->status & URL_MOVED)
              moved++;
            else if(urlp->status & URL_TRUNCATED)
            {
              broken = dllist_append(broken, (dllist_t) urlp);
              brokennr++;
            }
            else if(urlp->status & URL_NOT_FOUND)
            {
              dexist = dllist_append(dexist, (dllist_t) urlp);
              dexistnr++;
            }
            else if(urlp->status & URL_ERR_UNREC)
            {
              ferr = dllist_append(ferr, (dllist_t) urlp);
              ferrnr++;
            }
            else if(urlp->status & URL_ERR_REC)
            {
              nferr = dllist_append(nferr, (dllist_t) urlp);
              nferrnr++;
            }
            else
            {
              uns = dllist_append(uns, (dllist_t) urlp);
              unsnr++;
            }
          }
          else
            noproc++;

          ptr = ptr->next;
        }
      }

    }

    if(filename)
    {
      FILE *f;
#define DMP_URLLIST(lst)\
      ptr = lst;\
      while(ptr)\
      {\
        urlp = (url *)ptr->data;\
        p = url_to_urlstr(urlp, FALSE);\
        fprintf(f, "        %s\n", p);\
        _free(p);\
        LOCK_URL(urlp);\
        for (parptr = urlp->parent_url; parptr ; parptr = parptr->next)\
        {\
          p = url_to_urlstr((url *)parptr->data, FALSE);\
          fprintf(f, "                %s\n", p);\
          _free(p);\
        }\
        UNLOCK_URL(urlp);\
        ptr = ptr->next;\
      }\

      f = fopen(filename, "wb+");
      if(!f)
      {
        UNLOCK_CFG_URLHASH;
        xperror(filename);
        return -1;
      }
      fprintf(f, gettext("Total number of URLs in queue: %d\n"), total);
      fprintf(f, gettext("Starting urls: %d\n"), startnr);
      DMP_URLLIST(start);
      if(noproc)
        fprintf(f, gettext("Not processed yet: %d (%3d%%)\n"),
          noproc, (int) (((float) noproc / (float) total) * 100.0));

      i = dnld + nsredir + redir;
      if(i)
        fprintf(f, gettext("Processed OK: %d (%3d%%)\n"), i,
          (int) (((float) i / (float) total) * 100.0));
      if(redir)
        fprintf(f, gettext("Loaded from local tree: %d (%3d%%)\n"), redir,
          (int) (((float) redir / (float) total) * 100.0));
      if(nsredir)
        fprintf(f,
          gettext("Loaded from Netscape browser cache dir: %d (%3d%%)\n"),
          nsredir, (int) (((float) nsredir / (float) total) * 100.0));
      if(dnld)
        fprintf(f, gettext("Downloaded over network: %d (%3d%%)\n"), dnld,
          (int) (((float) dnld / (float) total) * 100.0));
      if(moved)
        fprintf(f, gettext("Moved to another location: %d (%3d%%)\n"), moved,
          (int) (((float) moved / (float) total) * 100.0));
      if(broken)
      {
        fprintf(f, gettext("Downloaded truncated: %d (%3d%%)\n"), brokennr,
          (int) (((float) brokennr / (float) total) * 100.0));
        DMP_URLLIST(broken);
      }
      if(nferr)
      {
        fprintf(f, gettext("Non fatal errors: %d (%3d%%)\n"), nferrnr,
          (int) (((float) nferrnr / (float) total) * 100.0));
        DMP_URLLIST(nferr);
      }
      if(dexist)
      {
        fprintf(f, gettext("Not found documents: %d (%3d%%)\n"), dexistnr,
          (int) (((float) dexistnr / (float) total) * 100.0));
        DMP_URLLIST(dexist);
      }
      if(ferr)
      {
        fprintf(f, gettext("Documents with fatal errors: %d (%3d%%)\n"),
          ferrnr, (int) (((float) ferrnr / (float) total) * 100.0));
        DMP_URLLIST(ferr);
      }
      if(uns)
      {
        fprintf(f, gettext("Documents with unknown status: %d (%3d%%)\n"),
          unsnr, (int) (((float) unsnr / (float) total) * 100.0));
        DMP_URLLIST(uns);
      }
      fclose(f);
#undef DMP_URLLIST
    }
#ifdef GTK_FACE
/* here will be clist with active URL fields */
    else if(cfg.xi_face)
    {
      GtkWidget *l = GTK_WIDGET(lst);
#define DMP_URLLIST(lst, ajp)\
      ptr = lst;\
      while(ptr)\
      {\
        urlp = (url *)ptr->data;\
        p = url_to_urlstr(urlp, FALSE);\
        sprintf(pom, "        %s\n", p);\
        _free(p);\
        i = gtk_clist_append(GTK_CLIST(l), &pom);\
        gtk_clist_set_row_data(GTK_CLIST(l), i, urlp);\
        LOCK_URL(urlp);\
        for (parptr = urlp->parent_url; ajp && parptr; parptr = parptr->next)\
        {\
          p = url_to_urlstr((url *)parptr->data, FALSE);\
          sprintf(pom, "                %s\n", p);\
          _free(p);\
          gtk_clist_append(GTK_CLIST(l), &pom);\
        }\
        UNLOCK_URL(urlp);\
        ptr = ptr->next;\
      }\

      sprintf(pom, gettext("Total number of URLs in queue: %d\n"), total);
      gtk_clist_append(GTK_CLIST(l), &pom);
      sprintf(pom, gettext("Starting urls: %d\n"), startnr);
      gtk_clist_append(GTK_CLIST(l), &pom);
      DMP_URLLIST(start, FALSE);
      if(noproc)
      {
        sprintf(pom, gettext("Not processed yet: %d (%3d%%)\n"),
          noproc, (int) (((float) noproc / (float) total) * 100.0));
        gtk_clist_append(GTK_CLIST(l), &pom);
      }
      i = dnld + nsredir + redir;
      if(i)
      {
        sprintf(pom, gettext("Processed OK: %d (%3d%%)\n"), i,
          (int) (((float) i / (float) total) * 100.0));
        gtk_clist_append(GTK_CLIST(l), &pom);
      }
      if(redir)
      {
        sprintf(pom, gettext("Loaded from local tree: %d (%3d%%)\n"), redir,
          (int) (((float) redir / (float) total) * 100.0));
        gtk_clist_append(GTK_CLIST(l), &pom);
      }
      if(nsredir)
      {
        sprintf(pom,
          gettext("Loaded from Netscape browser cache dir: %d (%3d%%)\n"),
          nsredir, (int) (((float) nsredir / (float) total) * 100.0));
        gtk_clist_append(GTK_CLIST(l), &pom);
      }
      if(dnld)
      {
        sprintf(pom, gettext("Downloaded over network: %d (%3d%%)\n"), dnld,
          (int) (((float) dnld / (float) total) * 100.0));
        gtk_clist_append(GTK_CLIST(l), &pom);
      }
      if(moved)
      {
        sprintf(pom, gettext("Moved to another location: %d (%3d%%)\n"),
          moved, (int) (((float) moved / (float) total) * 100.0));
        gtk_clist_append(GTK_CLIST(l), &pom);
      }
      if(broken)
      {
        sprintf(pom, gettext("Downloaded truncated: %d (%3d%%)\n"), brokennr,
          (int) (((float) brokennr / (float) total) * 100.0));
        gtk_clist_append(GTK_CLIST(l), &pom);
        DMP_URLLIST(broken, TRUE);
      }
      if(nferr)
      {
        sprintf(pom, gettext("Non fatal errors: %d (%3d%%)\n"), nferrnr,
          (int) (((float) nferrnr / (float) total) * 100.0));
        gtk_clist_append(GTK_CLIST(l), &pom);
        DMP_URLLIST(nferr, TRUE);
      }
      if(dexist)
      {
        sprintf(pom, gettext("Not found documents: %d (%3d%%)\n"), dexistnr,
          (int) (((float) dexistnr / (float) total) * 100.0));
        gtk_clist_append(GTK_CLIST(l), &pom);
        DMP_URLLIST(dexist, TRUE);
      }
      if(ferr)
      {
        sprintf(pom, gettext("Documents with fatal errors: %d (%3d%%)\n"),
          ferrnr, (int) (((float) ferrnr / (float) total) * 100.0));
        gtk_clist_append(GTK_CLIST(l), &pom);
        DMP_URLLIST(ferr, TRUE);
      }
      if(uns)
      {
        sprintf(pom,
          gettext("Documents with unspecific status: %d (%3d%%)\n"), unsnr,
          (int) (((float) unsnr / (float) total) * 100.0));
        gtk_clist_append(GTK_CLIST(l), &pom);
        DMP_URLLIST(uns, TRUE);
      }
#undef DMP_URLLIST
    }
#endif

    break;
  default:
    UNLOCK_CFG_URLHASH;
    return -1;
    break;
  }
  UNLOCK_CFG_URLHASH;
  return 0;
}

#ifdef GTK_FACE
#include <gdk/gdkkeysyms.h>
#include "gaccel.h"
#include "recurse.h"

#include "icons/cancel.xpm"
#include "icons/save.xpm"
#include "icons/restart_small.xpm"

static GtkWidget *stat_list, *dmi;
static GtkWidget *sstatw = NULL;

static void PopdownW(GtkWidget * w, gpointer fdata)
{
  gtk_widget_destroy(GTK_WIDGET(fdata));
}

static void Save(GtkWidget * w, gpointer fdata)
{
  const char *fn = gtk_file_selection_get_filename(GTK_FILE_SELECTION(fdata));

  if(fn && *fn)
  {
    if(stats_fill_spage(fn, NULL))
    {
      gdk_beep();
    }
    else
      gtk_widget_destroy(GTK_WIDGET(fdata));
  }
  else
  {
    gdk_beep();
  }
}

static void SaveDia(void)
{
  static GtkWidget *sw;

  if(!sw)
  {
    sw = gtk_file_selection_new(gettext("Pavuk: save status page"));
    gtk_signal_connect(GTK_OBJECT(sw), "destroy",
      GTK_SIGNAL_FUNC(gtk_widget_destroyed), &sw);

    gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(sw)->ok_button),
      "clicked", GTK_SIGNAL_FUNC(Save), sw);

    gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(sw)->cancel_button),
      "clicked", GTK_SIGNAL_FUNC(PopdownW), sw);
  }
  gtk_widget_show(sw);
  if(GTK_WIDGET_REALIZED(sw))
    gdk_window_raise(sw->window);
}

static void Refresh(void)
{
  gtk_clist_freeze(GTK_CLIST(stat_list));
  gtk_clist_clear(GTK_CLIST(stat_list));
  if(stats_fill_spage(NULL, stat_list))
  {
    char *p = gettext("Not available yet");
    gtk_clist_append(GTK_CLIST(stat_list), &p);
  }
  gtk_clist_thaw(GTK_CLIST(stat_list));
}

static void TryDownload(void)
{
  GList *ptr = GTK_CLIST(stat_list)->selection;
  url *urlp =
    (url *) gtk_clist_get_row_data(GTK_CLIST(stat_list),
    GPOINTER_TO_INT(ptr->data));

  if(!urlp || cfg.processing)
  {
    gdk_beep();
    return;
  }

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

static gint list_events(GtkWidget * widget, GdkEvent * event, gpointer fdata)
{
  GdkEventButton *bevent;

  switch (event->type)
  {
  case GDK_BUTTON_PRESS:
    bevent = (GdkEventButton *) event;
    if(bevent->button == 3)
    {
      if(GTK_CLIST(stat_list)->selection)
      {
        GList *ptr = GTK_CLIST(stat_list)->selection;
        url *urlp =
          (url *) gtk_clist_get_row_data(GTK_CLIST(stat_list),
          GPOINTER_TO_INT(ptr->data));

        gtk_widget_set_sensitive(dmi, (urlp != NULL) && !cfg.processing);
        gtk_menu_popup(GTK_MENU(fdata), NULL, NULL,
          NULL, NULL, 3, bevent->time);
      }
    }
    break;
  default:
    break;
  }
  return FALSE;
}

void stats_show(void)
{

  if(!sstatw)
  {
    GtkWidget *box, *swin, *bbox, *button, *menu;
    GtkAccelGroup *accel_group;
    accel_group = gtk_accel_group_new();

    sstatw = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(sstatw), gettext("Pavuk: status page"));
    gtk_signal_connect(GTK_OBJECT(sstatw), "destroy",
      GTK_SIGNAL_FUNC(gtk_widget_destroyed), &sstatw);

    box = gtk_vbox_new(FALSE, 2);
    gtk_container_add(GTK_CONTAINER(sstatw), box);
    gtk_widget_show(box);

    swin = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_usize(swin, 500, 400);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin),
      GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_show(swin);
    gtk_container_add(GTK_CONTAINER(box), swin);

    stat_list = gtk_clist_new(1);
    gtk_clist_set_selection_mode(GTK_CLIST(stat_list), GTK_SELECTION_BROWSE);
    gtk_clist_set_column_title(GTK_CLIST(stat_list), 0,
      gettext("Status page"));
    gtk_clist_column_titles_show(GTK_CLIST(stat_list));
    gtk_clist_set_column_auto_resize(GTK_CLIST(stat_list), 0, TRUE);
    gtk_widget_show(stat_list);
    gtk_container_add(GTK_CONTAINER(swin), stat_list);

    bbox = gtk_hbutton_box_new();
    gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_SPREAD);
    gtk_box_pack_end(GTK_BOX(box), bbox, FALSE, FALSE, 2);
    gtk_widget_show(bbox);

    button = guitl_pixmap_button(save_xpm, NULL, gettext("Save ..."));
    gtk_container_add(GTK_CONTAINER(bbox), button);
    GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
      GTK_SIGNAL_FUNC(SaveDia), NULL);

    button = guitl_pixmap_button(restart_small_xpm, NULL, gettext("Refresh"));
    gtk_container_add(GTK_CONTAINER(bbox), button);
    GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
    gtk_widget_grab_default(button);
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
      GTK_SIGNAL_FUNC(Refresh), NULL);

    button = guitl_pixmap_button(cancel_xpm, NULL, gettext("Cancel"));
    gtk_widget_add_accelerator(button, "clicked", accel_group,
      GDK_Escape, 0, GTK_ACCEL_VISIBLE);
    gtk_window_add_accel_group(GTK_WINDOW(sstatw), accel_group);
    gtk_container_add(GTK_CONTAINER(bbox), button);
    gtk_widget_show(button);
    GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
      GTK_SIGNAL_FUNC(PopdownW), sstatw);

    menu = gtk_menu_new();
    guitl_menu_attach(menu, sstatw);
    gtk_widget_realize(menu);

    gtk_signal_connect(GTK_OBJECT(stat_list), "button_press_event",
      GTK_SIGNAL_FUNC(list_events), menu);

    dmi = gtk_menu_item_new_with_label(gettext("Try download"));
    gaccel_bind_widget("stat/download", "activate", dmi, accel_group, NULL);
    gtk_menu_append(GTK_MENU(menu), dmi);
    gtk_widget_show(dmi);
    gtk_signal_connect(GTK_OBJECT(dmi), "activate",
      GTK_SIGNAL_FUNC(TryDownload), (gpointer) NULL);
  }
  gtk_clist_freeze(GTK_CLIST(stat_list));
  gtk_clist_clear(GTK_CLIST(stat_list));
  if(stats_fill_spage(NULL, stat_list))
  {
    char *p = gettext("Not available yet");
    gtk_clist_append(GTK_CLIST(stat_list), &p);
  }
  gtk_clist_thaw(GTK_CLIST(stat_list));
  gtk_widget_show(sstatw);
  if(GTK_WIDGET_REALIZED(sstatw))
    gdk_window_raise(sstatw->window);
}

void stats_clear(void)
{
  if(sstatw)
  {
    if(!MT_IS_MAIN_THREAD())
    {
      GDK_THREADS_ENTER();
    }
    gtk_clist_freeze(GTK_CLIST(stat_list));
    gtk_clist_clear(GTK_CLIST(stat_list));
    gtk_clist_thaw(GTK_CLIST(stat_list));
    if(!MT_IS_MAIN_THREAD())
    {
      GDK_THREADS_LEAVE();
    }

  }
}

#endif
