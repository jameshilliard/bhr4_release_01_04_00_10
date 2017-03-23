/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"
#include "gui.h"

#ifdef GTK_FACE
#include <stdio.h>
#include <gdk/gdkkeysyms.h>

#include "gtkmulticol.h"
#include "html.h"
#include "uconfig.h"
#include "mimetype.h"
#include "tag_pattern.h"

#include "icons/append.xpm"
#include "icons/modify.xpm"
#include "icons/clear.xpm"
#include "icons/delete.xpm"
#include "icons/ok.xpm"
#include "icons/apply.xpm"
#include "icons/common.xpm"
#include "icons/cancel.xpm"

static void CfgLimits(GtkObject * object, gpointer func_data)
{
  if(!xget_cfg_values_lim())
  {
    if(cfg.use_prefs)
      cfg_dump_pref();
  }
}

static void limtab_tree(GtkWidget * notebook)
{
  GtkWidget *label, *box, *ptab, *frame;

  box = gtk_vbox_new(FALSE, 2);
  gtk_widget_show(box);
  label = gtk_label_new(gettext("Tree"));
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), box, label);

  frame = gtk_frame_new(NULL);
  gtk_box_pack_start(GTK_BOX(box), frame, FALSE, FALSE, 2);
  gtk_widget_show(frame);

  ptab = gtk_table_new(2, 4, FALSE);
  gtk_container_add(GTK_CONTAINER(frame), ptab);
  gtk_widget_show(ptab);

  gui_cfg.cgi_sw =
    gtk_check_button_new_with_label(gettext("Download cgi-generated pages"));
  gtk_widget_show(gui_cfg.cgi_sw);
  gtk_table_attach(GTK_TABLE(ptab), gui_cfg.cgi_sw, 0, 1, 0, 1,
    GTK_FILL, GTK_FILL, 5, 0);

  gui_cfg.ftpd_sw =
    gtk_check_button_new_with_label(gettext("Recurse through FTP directory"));
  gtk_widget_show(gui_cfg.ftpd_sw);
  gtk_table_attach(GTK_TABLE(ptab), gui_cfg.ftpd_sw, 0, 1, 1, 2,
    GTK_FILL, GTK_FILL, 5, 0);

  gui_cfg.robots_sw =
    gtk_check_button_new_with_label(gettext("Allow \"robots.txt\""));
  gtk_widget_show(gui_cfg.robots_sw);
  gtk_table_attach(GTK_TABLE(ptab), gui_cfg.robots_sw, 0, 1, 2, 3,
    GTK_FILL, GTK_FILL, 5, 0);

  gui_cfg.ftp_html =
    gtk_check_button_new_with_label(gettext
    ("Process HTML files downloaded over FTP"));
  gtk_widget_show(gui_cfg.ftp_html);
  gtk_table_attach(GTK_TABLE(ptab), gui_cfg.ftp_html, 0, 1, 3, 4,
    GTK_FILL, GTK_FILL, 5, 0);

  gui_cfg.leaves_sw =
    gtk_check_button_new_with_label(gettext("Don't leave starting site"));
  gtk_widget_show(gui_cfg.leaves_sw);
  gtk_table_attach(GTK_TABLE(ptab), gui_cfg.leaves_sw, 1, 2, 0, 1,
    GTK_FILL, GTK_FILL, 5, 0);

  gui_cfg.leaved_sw =
    gtk_check_button_new_with_label(gettext
    ("Don't leave starting directory"));
  gtk_widget_show(gui_cfg.leaved_sw);
  gtk_table_attach(GTK_TABLE(ptab), gui_cfg.leaved_sw, 1, 2, 1, 2,
    GTK_FILL, GTK_FILL, 5, 0);

  gui_cfg.dont_leave_site_dir =
    gtk_check_button_new_with_label(gettext
    ("Don't leave site enter directory"));
  gtk_widget_show(gui_cfg.dont_leave_site_dir);
  gtk_table_attach(GTK_TABLE(ptab), gui_cfg.dont_leave_site_dir, 1, 2, 2, 3,
    GTK_FILL, GTK_FILL, 5, 0);

  gui_cfg.singlepage =
    gtk_check_button_new_with_label(gettext("Download just single page"));
  gtk_widget_show(gui_cfg.singlepage);
  gtk_table_attach(GTK_TABLE(ptab), gui_cfg.singlepage, 1, 2, 3, 4,
    GTK_FILL, GTK_FILL, 5, 0);

  gui_cfg.limit_inlines =
    gtk_check_button_new_with_label(gettext
    ("Apply limiting options on inline objects"));
  gtk_widget_show(gui_cfg.limit_inlines);
  gtk_table_attach(GTK_TABLE(ptab), gui_cfg.limit_inlines, 0, 1, 4, 5,
    GTK_FILL, GTK_FILL, 5, 0);


  frame = gtk_frame_new(NULL);
  gtk_box_pack_start(GTK_BOX(box), frame, FALSE, FALSE, 2);
  gtk_widget_show(frame);

  ptab = gtk_table_new(4, 3, FALSE);
  gtk_container_add(GTK_CONTAINER(frame), ptab);
  gtk_widget_show(ptab);

  gui_cfg.maxdoc_label = guitl_tab_add_numentry(ptab,
    gettext("Max. count of documents: "), 0, 4, INT_MAX);

  gui_cfg.maxlev_label = guitl_tab_add_numentry(ptab,
    gettext("Max. depth of tree: "), 0, 5, USHRT_MAX);

  gui_cfg.leave_level = guitl_tab_add_numentry(ptab,
    gettext("Max. levels to leave from starting site: "), 0, 6, USHRT_MAX);

  gui_cfg.maxsize_label = guitl_tab_add_numentry(ptab,
    gettext("Max. document size: "), 2, 4, INT_MAX);

  gui_cfg.min_size = guitl_tab_add_numentry(ptab,
    gettext("Min. document size: "), 2, 5, INT_MAX);

  gui_cfg.site_level = guitl_tab_add_numentry(ptab,
    gettext("Max. site levels to leave from starting site: "), 2, 6, INT_MAX);

  frame = gtk_frame_new(NULL);
  gtk_box_pack_start(GTK_BOX(box), frame, FALSE, FALSE, 2);
  gtk_widget_show(frame);

  ptab = gtk_table_new(2, 3, FALSE);
  gtk_container_add(GTK_CONTAINER(frame), ptab);
  gtk_widget_show(ptab);

  gui_cfg.subdir_label = guitl_tab_add_path_entry(ptab,
    gettext("Working subdirectory :"), 0, 0, TRUE);

  gui_cfg.en_uexit = guitl_tab_add_entry(ptab,
    gettext("User condition script: "), 0, 1, FALSE);

  gui_cfg.follow_cmd = guitl_tab_add_entry(ptab,
    gettext("Follow command: "), 0, 2, FALSE);

}

static void limtab_patterns(GtkWidget * notebook)
{
  GtkWidget *label, *box, *col, *ptab;

  box = gtk_vbox_new(FALSE, 2);
  gtk_widget_show(box);
  label = gtk_label_new(gettext("Patterns"));
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), box, label);

  ptab = gtk_frame_new(gettext("Wildcard patterns"));
  gtk_box_pack_start(GTK_BOX(box), ptab, FALSE, FALSE, 2);
  gtk_widget_show(ptab);

  col = gtk_table_new(4, 2, FALSE);
  gtk_container_add(GTK_CONTAINER(ptab), col);
  gtk_widget_show(col);

  gui_cfg.pattern_label = guitl_tab_add_edit_entry(col,
    gettext("Documents matching pattern: "), NULL, 0, 0, FALSE);

  gui_cfg.skip_pattern = guitl_tab_add_edit_entry(col,
    gettext("skip: "),
    gettext("Pavuk: edit Documents matching skip pattern"), 2, 0, FALSE);

  gui_cfg.url_pattern_label = guitl_tab_add_edit_entry(col,
    gettext("URL matching pattern: "), NULL, 0, 1, FALSE);

  gui_cfg.skip_url_pattern = guitl_tab_add_edit_entry(col,
    gettext("skip: "),
    gettext("Pavuk: edit URL matching skip pattern"), 2, 1, FALSE);

#ifdef HAVE_REGEX
  ptab = gtk_frame_new(gettext("RE patterns"));
  gtk_box_pack_start(GTK_BOX(box), ptab, FALSE, FALSE, 2);
  gtk_widget_show(ptab);

  col = gtk_table_new(4, 2, FALSE);
  gtk_container_add(GTK_CONTAINER(ptab), col);
  gtk_widget_show(col);

  gui_cfg.rpattern = guitl_tab_add_edit_entry(col,
    gettext("Documents matching pattern: "), NULL, 0, 0, TRUE);

  gui_cfg.skip_rpattern = guitl_tab_add_edit_entry(col,
    gettext("skip: "),
    gettext("Pavuk: edit Documents matching skip pattern"), 2, 0, TRUE);

  gui_cfg.url_rpattern = guitl_tab_add_edit_entry(col,
    gettext("URL matching pattern: "), NULL, 0, 1, TRUE);

  gui_cfg.url_skip_rpattern = guitl_tab_add_edit_entry(col,
    gettext("skip: "),
    gettext("Pavuk: edit URL matching skip pattern"), 2, 1, TRUE);
#endif
}

static void limtab_hosts(GtkWidget * notebook)
{
  GtkWidget *brow, *label, *box, *col, *ptab;

  box = gtk_table_new(2, 3, FALSE);
  gtk_widget_show(box);
  label = gtk_label_new(gettext("Hosts"));
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), box, label);

  col = gtk_table_new(2, 1, FALSE);
  gtk_widget_show(col);
  gtk_table_attach(GTK_TABLE(box), col, 0, 1, 0, 1,
    GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 5, 5);

  gui_cfg.hosts_sw =
    gtk_check_button_new_with_label(gettext("Allow / Disallow sites"));
  gtk_widget_show(gui_cfg.hosts_sw);
  gtk_table_attach(GTK_TABLE(col), gui_cfg.hosts_sw, 0, 1, 0, 1,
    GTK_FILL, GTK_FILL, 5, 5);

  brow = guitl_new_edit_list(&gui_cfg.hosts_list, &gui_cfg.hosts_entry,
    gettext("Site: "), NULL, NULL, NULL, NULL, TRUE, NULL);

  gtk_table_attach(GTK_TABLE(col), brow, 0, 1, 1, 2,
    GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 5, 5);

  col = gtk_table_new(2, 1, FALSE);
  gtk_widget_show(col);
  gtk_table_attach(GTK_TABLE(box), col, 1, 2, 0, 1,
    GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 5, 5);

  gui_cfg.domain_sw =
    gtk_check_button_new_with_label(gettext("Allow / Disallow domains"));
  gtk_widget_show(gui_cfg.domain_sw);
  gtk_table_attach(GTK_TABLE(col), gui_cfg.domain_sw, 0, 1, 0, 1,
    GTK_FILL, GTK_FILL, 5, 5);

  brow = guitl_new_edit_list(&gui_cfg.domain_list, &gui_cfg.domain_entry,
    gettext("Domain: "), NULL, NULL, NULL, NULL, TRUE, NULL);

  gtk_table_attach(GTK_TABLE(col), brow, 0, 1, 1, 2,
    GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 5, 5);

#ifdef HAVE_REGEX
  ptab = gtk_frame_new(gettext("IP address RE patterns"));
  gtk_table_attach(GTK_TABLE(box), ptab, 0, 2, 1, 2,
    GTK_FILL, GTK_FILL, 5, 5);
  gtk_widget_show(ptab);

  col = gtk_table_new(4, 1, FALSE);
  gtk_container_add(GTK_CONTAINER(ptab), col);
  gtk_widget_show(col);

  gui_cfg.aip = guitl_tab_add_edit_entry(col,
    gettext("Server IP address matching pattern: "), NULL, 0, 0, TRUE);

  gui_cfg.skipip = guitl_tab_add_edit_entry(col,
    gettext("skip: "),
    gettext("Pavuk: edit Server IP address matching skip pattern"),
    2, 0, TRUE);
#endif

  ptab = gtk_frame_new(gettext("Server ports"));
  gtk_table_attach(GTK_TABLE(box), ptab, 0, 2, 2, 3,
    GTK_FILL, GTK_FILL, 5, 5);
  gtk_widget_show(ptab);

  col = gtk_table_new(3, 1, FALSE);
  gtk_container_add(GTK_CONTAINER(ptab), col);
  gtk_widget_show(col);

  gui_cfg.allow_ports =
    gtk_check_button_new_with_label(gettext("Allow/deny"));
  gtk_table_attach(GTK_TABLE(col), gui_cfg.allow_ports, 0, 1, 0, 1, GTK_FILL,
    GTK_FILL, 1, 1);
  gtk_widget_show(gui_cfg.allow_ports);

  gui_cfg.ports = guitl_tab_add_edit_entry(col, gettext("Ports: "),
    NULL, 1, 0, FALSE);
}

static void limtab_docs(GtkWidget * notebook)
{
  GtkWidget *brow, *label, *box, *col;

  box = gtk_table_new(1, 2, FALSE);
  gtk_widget_show(box);
  label = gtk_label_new(gettext("Documents"));
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), box, label);

  col = gtk_table_new(2, 1, FALSE);
  gtk_widget_show(col);
  gtk_table_attach(GTK_TABLE(box), col, 0, 1, 0, 1,
    GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 5, 5);

  gui_cfg.sufix_sw =
    gtk_check_button_new_with_label(gettext("Allow / Disallow suffix"));
  gtk_widget_show(gui_cfg.sufix_sw);
  gtk_table_attach(GTK_TABLE(col), gui_cfg.sufix_sw, 0, 1, 0, 1,
    GTK_FILL, GTK_FILL, 5, 5);

  brow = guitl_new_edit_list(&gui_cfg.sufixlist, &gui_cfg.sufix_label,
    gettext("Suffix: "), NULL, NULL, NULL, NULL, TRUE, NULL);

  gtk_table_attach(GTK_TABLE(col), brow, 0, 1, 1, 2,
    GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 5, 5);

  col = gtk_table_new(2, 1, FALSE);
  gtk_widget_show(col);
  gtk_table_attach(GTK_TABLE(box), col, 1, 2, 0, 1,
    GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 5, 5);

  gui_cfg.prefix_sw =
    gtk_check_button_new_with_label(gettext("Allow / Disallow prefix"));
  gtk_widget_show(gui_cfg.prefix_sw);
  gtk_table_attach(GTK_TABLE(col), gui_cfg.prefix_sw, 0, 1, 0, 1,
    GTK_FILL, GTK_FILL, 5, 5);

  brow = guitl_new_edit_list(&gui_cfg.prefixlist, &gui_cfg.prefix_label,
    gettext("Prefix: "), NULL, NULL, NULL, NULL, TRUE, NULL);

  gtk_table_attach(GTK_TABLE(col), brow, 0, 1, 1, 2,
    GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 5, 5);

}

static void limtab_mime(GtkWidget * notebook)
{
  GtkWidget *col, *box, *label, *frame, *entry, *pbox;

  box = gtk_hbox_new(0, 5);
  gtk_widget_show(box);
  label = gtk_label_new(gettext("MIME types"));
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), box, label);

  pbox = gtk_vbox_new(FALSE, 2);
  gtk_box_pack_start(GTK_BOX(box), pbox, FALSE, FALSE, 5);
  gtk_widget_show(pbox);

  gui_cfg.mime_sw =
    gtk_check_button_new_with_label(gettext("Allow / Disallow MIME type"));
  gtk_box_pack_start(GTK_BOX(pbox), gui_cfg.mime_sw, FALSE, FALSE, 5);
  gtk_widget_show(gui_cfg.mime_sw);

  frame = gtk_frame_new(gettext("MIME types"));
  gtk_box_pack_start(GTK_BOX(pbox), frame, TRUE, TRUE, 5);
  gtk_widget_show(frame);

  col = guitl_new_edit_list(&gui_cfg.amimelist, &entry,
    gettext("MIME type: "), NULL, NULL, NULL, NULL, TRUE, mimetypes);

  gtk_container_add(GTK_CONTAINER(frame), col);
}

static void limtab_time(GtkWidget * notebook)
{
  GtkWidget *tbox, *box, *col, *label, *frame;

  tbox = gtk_vbox_new(FALSE, 1);
  gtk_widget_show(tbox);
  label = gtk_label_new(gettext("Time"));
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), tbox, label);

  box = gtk_hbox_new(FALSE, 5);
  gtk_widget_show(box);
  gtk_box_pack_start(GTK_BOX(tbox), box, FALSE, FALSE, 4);
  frame = gtk_frame_new(gettext("Lower document time limit"));
  gtk_widget_show(frame);
  gtk_box_pack_start(GTK_BOX(box), frame, FALSE, FALSE, 4);

  col = guitl_timesel_new(&gui_cfg.btime_cal, &gui_cfg.btime_h_entry,
    &gui_cfg.btime_min_entry, &gui_cfg.btime_mon, &gui_cfg.btime_yentry);
  gtk_container_add(GTK_CONTAINER(frame), col);

  gui_cfg.btime_sw =
    gtk_check_button_new_with_label(gettext
    ("Check if doc. time newer than this"));
  gtk_widget_show(gui_cfg.btime_sw);
  gtk_box_pack_start(GTK_BOX(col), gui_cfg.btime_sw, FALSE, FALSE, 4);

  frame = gtk_frame_new(gettext("Upper document time limit"));
  gtk_widget_show(frame);
  gtk_box_pack_start(GTK_BOX(box), frame, FALSE, FALSE, 4);

  col = guitl_timesel_new(&gui_cfg.etime_cal, &gui_cfg.etime_h_entry,
    &gui_cfg.etime_min_entry, &gui_cfg.etime_mon, &gui_cfg.etime_yentry);
  gtk_container_add(GTK_CONTAINER(frame), col);

  gui_cfg.etime_sw =
    gtk_check_button_new_with_label(gettext
    ("Check if doc. time older than this"));
  gtk_widget_show(gui_cfg.etime_sw);
  gtk_box_pack_start(GTK_BOX(col), gui_cfg.etime_sw, FALSE, FALSE, 4);

  frame = gtk_table_new(3, 1, FALSE);
  gtk_box_pack_start(GTK_BOX(tbox), frame, FALSE, FALSE, 4);
  gtk_widget_show(frame);

  gui_cfg.max_time = guitl_tab_add_doubleentry(frame,
    gettext("Maximal allowed time of downloading: "), 0, 0, INT_MAX, 2);

  label = gtk_label_new(gettext(" min"));
  gtk_table_attach(GTK_TABLE(frame), label, 2, 3, 0, 1,
    GTK_SHRINK, GTK_FILL, 2, 2);
  gtk_widget_show(label);
}

static void limtab_html(GtkWidget * notebook)
{
  GtkWidget *box, *label, *frame, *check;
  int i, j;
  char pom[100];

  frame = gtk_frame_new(gettext("Select allowed HTML tags and attributes"));
  gtk_widget_show(frame);
  label = gtk_label_new(gettext("HTML"));
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), frame, label);


  gui_cfg.html_tags = box = gtk_multicol_new(10);
  gtk_multicol_set_number_of_rows(GTK_MULTICOL(gui_cfg.html_tags), 14);
  gtk_widget_show(box);
  gtk_container_add(GTK_CONTAINER(frame), box);

  for(i = 0; i < html_link_tags_num(); i++)
  {
    for(j = 0; html_link_tags[i].attribs[j].attrib; j++)
    {
      if(!(html_link_tags[i].attribs[j].stat & LINK_STYLE))
      {
        snprintf(pom, sizeof(pom), gettext("%s of %s"),
          html_link_tags[i].attribs[j].attrib, html_link_tags[i].tag);

        check = gtk_check_button_new_with_label(pom);
        gtk_object_set_user_data(GTK_OBJECT(check),
          (gpointer) & html_link_tags[i].attribs[j].stat);
        gtk_widget_show(check);
        gtk_container_add(GTK_CONTAINER(box), check);
      }
    }
  }
}

struct cfg_tag_patterns_t
{
  GtkWidget *list;
  GtkWidget *tag;
  GtkWidget *attrib;
  GtkWidget *urlp;
  GtkWidget *re_wc;
};

static struct cfg_tag_patterns_t tagpat;

static void TagPatSelectRow(void)
{
  char *p;
  int type;
  int row = GPOINTER_TO_INT(GTK_CLIST(tagpat.list)->selection->data);

  type = (int) gtk_clist_get_row_data(GTK_CLIST(tagpat.list), row);
  gtk_option_menu_set_history(GTK_OPTION_MENU(tagpat.re_wc), type);

  gtk_clist_get_text(GTK_CLIST(tagpat.list), row, 1, &p);
  gtk_entry_set_text(GTK_ENTRY(tagpat.tag), p);

  gtk_clist_get_text(GTK_CLIST(tagpat.list), row, 2, &p);
  gtk_entry_set_text(GTK_ENTRY(tagpat.attrib), p);

  gtk_clist_get_text(GTK_CLIST(tagpat.list), row, 2, &p);
  gtk_entry_set_text(GTK_ENTRY(tagpat.urlp), p);
}

static void TagPatNew(int row)
{
  char *pp[4];
  tag_pattern_t *tp;
  int type;

  type = GTK_OPTION_MENU(tagpat.re_wc)->menu_item ?
    (int) gtk_object_get_user_data(GTK_OBJECT(GTK_OPTION_MENU(tagpat.re_wc)->
      menu_item)) : TAGP_WC;

  pp[0] = type == TAGP_WC ? "WC" : "RE";
  pp[1] = (char *) gtk_entry_get_text(GTK_ENTRY(tagpat.tag));
  pp[2] = (char *) gtk_entry_get_text(GTK_ENTRY(tagpat.attrib));
  pp[3] = (char *) gtk_entry_get_text(GTK_ENTRY(tagpat.urlp));

  if(!(tp = tag_pattern_new(type, pp[1], pp[2], pp[3])))
  {
    gdk_beep();
    return;
  }
  tag_pattern_free(tp);

  if(row < 0)
    row = gtk_clist_append(GTK_CLIST(tagpat.list), pp);
  else
  {
    int i;
    for(i = 0; i < 4; i++)
      gtk_clist_set_text(GTK_CLIST(tagpat.list), row, i, pp[i]);
  }

  gtk_clist_set_row_data(GTK_CLIST(tagpat.list), row, (gpointer) type);
}

static void TagPatModify(void)
{
  if(!GTK_CLIST(tagpat.list)->selection)
  {
    gdk_beep();
    return;
  }

  TagPatNew(GPOINTER_TO_INT(GTK_CLIST(tagpat.list)->selection->data));
}

static void TagPatAppend(void)
{
  TagPatNew(-1);
}

static void limtab_tags(GtkWidget * notebook)
{
  GtkWidget *pbox, *label, *frame, *box, *smenu, *mi, *ptab, *bbox;
  GtkWidget *button;
  int i, j, k;
  char **pp1, **pp2;

  box = gtk_hbox_new(FALSE, 2);
  gtk_widget_show(box);
  label = gtk_label_new(gettext("Tag patterns"));
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), box, label);

  frame = gtk_frame_new(NULL);
  gtk_box_pack_start(GTK_BOX(box), frame, FALSE, FALSE, 2);
  gtk_widget_show(frame);

  pbox = gtk_vbox_new(FALSE, 2);
  gtk_container_add(GTK_CONTAINER(frame), pbox);
  gtk_widget_show(pbox);

  gui_cfg.tag_patterns = tagpat.list = gtk_clist_new(4);
  gtk_clist_set_column_title(GTK_CLIST(tagpat.list), 0, gettext("Type"));
  gtk_clist_set_column_title(GTK_CLIST(tagpat.list), 1,
    gettext("Tag pattern"));
  gtk_clist_set_column_title(GTK_CLIST(tagpat.list), 2,
    gettext("Attribute pattern"));
  gtk_clist_set_column_title(GTK_CLIST(tagpat.list), 3,
    gettext("URL pattern"));
  gtk_clist_set_column_auto_resize(GTK_CLIST(tagpat.list), 0, TRUE);
  gtk_clist_set_column_auto_resize(GTK_CLIST(tagpat.list), 1, TRUE);
  gtk_clist_set_column_auto_resize(GTK_CLIST(tagpat.list), 2, TRUE);
  gtk_clist_set_column_auto_resize(GTK_CLIST(tagpat.list), 3, TRUE);
  gtk_clist_column_titles_show(GTK_CLIST(tagpat.list));
  gtk_signal_connect(GTK_OBJECT(tagpat.list), "select_row",
    GTK_SIGNAL_FUNC(TagPatSelectRow), NULL);
  gtk_container_add(GTK_CONTAINER(pbox), tagpat.list);
  gtk_widget_show(tagpat.list);

  ptab = gtk_table_new(2, 2, FALSE);
  gtk_box_pack_start(GTK_BOX(pbox), ptab, FALSE, FALSE, 2);
  gtk_widget_show(ptab);

  label = gtk_label_new(gettext("Pattern type: "));
  gtk_table_attach(GTK_TABLE(ptab), label, 0, 1, 0, 1,
    GTK_FILL, GTK_FILL, 2, 2);
  gtk_widget_show(label);

  tagpat.re_wc = gtk_option_menu_new();
  gtk_table_attach(GTK_TABLE(ptab), tagpat.re_wc, 1, 2, 0, 1,
    GTK_FILL, GTK_FILL, 2, 2);

  smenu = gtk_menu_new();
  gtk_widget_realize(smenu);

  mi = gtk_menu_item_new_with_label(gettext("Wildcard patterns"));
  gtk_menu_append(GTK_MENU(smenu), mi);
  gtk_widget_show(mi);
  gtk_object_set_user_data(GTK_OBJECT(mi), (gpointer) TAGP_WC);

#ifdef HAVE_REGEX
  mi = gtk_menu_item_new_with_label(gettext("RE patterns"));
  gtk_menu_append(GTK_MENU(smenu), mi);
  gtk_widget_show(mi);
  gtk_object_set_user_data(GTK_OBJECT(mi), (gpointer) TAGP_RE);
#endif

  gtk_option_menu_set_menu(GTK_OPTION_MENU(tagpat.re_wc), smenu);
  gtk_widget_show(tagpat.re_wc);

/*
  tagpat.tag = guitl_tab_add_entry(ptab, gettext("Tag pattern"),
    0, 1, FALSE);

  tagpat.attrib = guitl_tab_add_entry(ptab, gettext("Attribute pattern"),
    0, 2, FALSE);
*/
  pp2 = NULL;
  k = 0;
  pp1 = _malloc((html_link_tags_num() + 2) * sizeof(char *));
  for(i = 0; i < html_link_tags_num(); i++)
  {
    pp1[i] = html_link_tags[i].tag;
    pp1[i + 1] = NULL;
    for(j = 0; html_link_tags[i].attribs[j].attrib; j++)
    {
      if(0 > tl_strv_find(pp2, html_link_tags[i].attribs[j].attrib))
      {
        pp2 = _realloc(pp2, sizeof(char *) * (3 + k));
        pp2[k] = html_link_tags[i].attribs[j].attrib;
        pp2[k + 1] = NULL;
        k++;
      }
    }
  }
  pp1[i] = "";
  pp1[i + 1] = NULL;
  tl_strv_sort(pp1);
  pp2[k] = "";
  pp2[k + 1] = NULL;
  tl_strv_sort(pp2);

  tagpat.tag = guitl_tab_add_enum(ptab, gettext("Tag pattern"), 0, 1,
    (const char **) pp1, TRUE);
  _free(pp1);

  tagpat.attrib = guitl_tab_add_enum(ptab, gettext("Attribute pattern"),
    0, 2, (const char **) pp2, TRUE);
  _free(pp2);

  tagpat.urlp = guitl_tab_add_entry(ptab, gettext("URL pattern"),
    0, 3, FALSE);

  bbox = gtk_hbutton_box_new();
  gtk_widget_show(bbox);
  gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_SPREAD);
  gtk_box_pack_start(GTK_BOX(pbox), bbox, FALSE, FALSE, 5);

  button = guitl_pixmap_button(append_xpm, NULL, gettext("Append"));
  gtk_container_add(GTK_CONTAINER(bbox), button);
  gtk_widget_show(button);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
    GTK_SIGNAL_FUNC(TagPatAppend), &tagpat);

  button = guitl_pixmap_button(modify_xpm, NULL, gettext("Modify"));
  gtk_container_add(GTK_CONTAINER(bbox), button);
  gtk_widget_show(button);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
    GTK_SIGNAL_FUNC(TagPatModify), &tagpat);

  button = guitl_pixmap_button(clear_xpm, NULL, gettext("Clear"));
  gtk_container_add(GTK_CONTAINER(bbox), button);
  gtk_widget_show(button);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
    GTK_SIGNAL_FUNC(guitl_ListClear), tagpat.list);

  button = guitl_pixmap_button(delete_xpm, NULL, gettext("Delete"));
  gtk_container_add(GTK_CONTAINER(bbox), button);
  gtk_widget_show(button);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
    GTK_SIGNAL_FUNC(guitl_ListDeleteSelected), tagpat.list);

}

/*** LIMITS CFG ***/
void gui_build_config_limits(int popup)
{
  GtkWidget *col, *brow, *button, *notebook;
  GtkAccelGroup *accel_group;

  if(gui_cfg.cfg_limits)
  {
    if(popup)
    {
      if(!GTK_WIDGET_VISIBLE(gui_cfg.cfg_limits))
        xset_cfg_values_lim();
      gtk_widget_show_all(gui_cfg.cfg_limits);
      if(GTK_WIDGET_REALIZED(gui_cfg.cfg_limits))
        gdk_window_raise(gui_cfg.cfg_limits->window);
    }
    return;
  }

  gui_cfg.cfg_limits = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_container_border_width(GTK_CONTAINER(gui_cfg.cfg_limits), 3);
  gtk_window_set_title(GTK_WINDOW(gui_cfg.cfg_limits),
    gettext("Pavuk: Limits config"));
  gtk_widget_realize(gui_cfg.cfg_limits);
  gtk_signal_connect(GTK_OBJECT(gui_cfg.cfg_limits), "destroy",
    GTK_SIGNAL_FUNC(gtk_widget_destroyed), &gui_cfg.cfg_limits);

  col = gtk_table_new(2, 1, FALSE);
  gtk_container_add(GTK_CONTAINER(gui_cfg.cfg_limits), col);
  gtk_widget_show(col);

  notebook = gtk_notebook_new();
  gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_TOP);
  gtk_notebook_set_scrollable(GTK_NOTEBOOK(notebook), TRUE);
  gtk_table_attach_defaults(GTK_TABLE(col), notebook, 0, 1, 0, 1);
  gtk_widget_show(notebook);

  limtab_tree(notebook);

  limtab_patterns(notebook);

  limtab_hosts(notebook);

  limtab_docs(notebook);

  limtab_mime(notebook);

  limtab_time(notebook);

  limtab_html(notebook);

  limtab_tags(notebook);

  brow = gtk_hbutton_box_new();
  gtk_table_attach(GTK_TABLE(col), brow, 0, 1, 1, 2,
    GTK_EXPAND | GTK_FILL, GTK_FILL, 2, 5);
  gtk_hbutton_box_set_spacing_default(1);
  gtk_widget_show(brow);
  gtk_button_box_set_layout(GTK_BUTTON_BOX(brow), GTK_BUTTONBOX_SPREAD);

  button = guitl_pixmap_button(ok_xpm, NULL, gettext("OK"));
  gtk_container_add(GTK_CONTAINER(brow), button);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
    GTK_SIGNAL_FUNC(gui_PopdownWC), (gpointer) gui_cfg.cfg_limits);
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
  gtk_widget_grab_default(button);
  gtk_widget_show(button);

  button = guitl_pixmap_button(apply_xpm, NULL, gettext("Apply"));
  gtk_container_add(GTK_CONTAINER(brow), button);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
    GTK_SIGNAL_FUNC(CfgLimits), (gpointer) gui_cfg.config_shell);
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
  gtk_widget_show(button);

  button = guitl_pixmap_button(common_xpm, NULL, gettext("Common ..."));
  gtk_container_add(GTK_CONTAINER(brow), button);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
    GTK_SIGNAL_FUNC(gui_PopupW), (gpointer) PAVUK_CFGCOMM);
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
  gtk_widget_show(button);

  button = guitl_pixmap_button(cancel_xpm, NULL, gettext("Cancel"));

  accel_group = gtk_accel_group_new();
  gtk_widget_add_accelerator(button, "clicked", accel_group,
    GDK_Escape, 0, GTK_ACCEL_VISIBLE);
  gtk_window_add_accel_group(GTK_WINDOW(gui_cfg.cfg_limits), accel_group);

  gtk_container_add(GTK_CONTAINER(brow), button);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
    GTK_SIGNAL_FUNC(guitl_PopdownW), (gpointer) gui_cfg.cfg_limits);
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
  gtk_widget_show(button);

  if(popup)
    gtk_widget_show(gui_cfg.cfg_limits);

  xset_cfg_values_lim();
}
#endif
