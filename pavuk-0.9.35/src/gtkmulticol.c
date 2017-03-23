/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"

#ifdef GTK_FACE
/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 * GtkMultiCol Copyright (C) 1998 Stefan Ondrejicka <ondrej@idata.sk>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "gtkmulticol.h"
#include <gtk/gtk.h>
#include <stdlib.h>

enum
{
  ARG_0,
  ARG_VSPACING,
  ARG_HSPACING
};

static void gtk_multicol_class_init(GtkMultiColClass * klass);
static void gtk_multicol_init(GtkMultiCol * multicol);
static void gtk_multicol_get_arg(GtkObject * object,
  GtkArg * arg, guint arg_id);
static void gtk_multicol_set_arg(GtkObject * object,
  GtkArg * arg, guint arg_id);
static void gtk_multicol_size_request(GtkWidget * widget,
  GtkRequisition * requisition);
static void gtk_multicol_size_allocate(GtkWidget * widget,
  GtkAllocation * allocation);

guint gtk_multicol_get_type()
{
  static guint multicol_type = 0;

  if(!multicol_type)
  {
    GtkTypeInfo multicol_info = {
      "GtkMultiCol",
      sizeof(GtkMultiCol),
      sizeof(GtkMultiColClass),
      (GtkClassInitFunc) gtk_multicol_class_init,
      (GtkObjectInitFunc) gtk_multicol_init,
      NULL,
      NULL,
    };

    multicol_type = gtk_type_unique(gtk_box_get_type(), &multicol_info);
  }

  return multicol_type;
}

static void gtk_multicol_class_init(GtkMultiColClass * class)
{
  GtkWidgetClass *widget_class;
  GtkObjectClass *object_class;

  widget_class = (GtkWidgetClass *) class;
  object_class = (GtkObjectClass *) class;

  widget_class->size_request = gtk_multicol_size_request;
  widget_class->size_allocate = gtk_multicol_size_allocate;

  gtk_object_add_arg_type("GtkBox::hspacing", GTK_TYPE_INT, GTK_ARG_READWRITE,
    ARG_HSPACING);
  gtk_object_add_arg_type("GtkBox::vspacing", GTK_TYPE_INT, GTK_ARG_READWRITE,
    ARG_VSPACING);

#if defined(GTK_CHECK_VERSION)
#if GTK_CHECK_VERSION(1 , 2 , 0)
  object_class->set_arg = gtk_multicol_set_arg;
  object_class->get_arg = gtk_multicol_get_arg;
#endif
#endif
}

static void gtk_multicol_init(GtkMultiCol * multicol)
{
  multicol->vspacing = 3;
  multicol->hspacing = 3;
}

static void gtk_multicol_get_arg(GtkObject * object,
  GtkArg * arg, guint arg_id)
{
  GtkMultiCol *multicol = GTK_MULTICOL(object);

  switch (arg_id)
  {
  case ARG_VSPACING:
    GTK_VALUE_INT(*arg) = multicol->vspacing;
    break;
  case ARG_HSPACING:
    GTK_VALUE_INT(*arg) = multicol->hspacing;
    break;
  default:
    arg->type = GTK_TYPE_INVALID;
    break;
  }
}

static void gtk_multicol_set_arg(GtkObject * object,
  GtkArg * arg, guint arg_id)
{
  GtkMultiCol *multicol = GTK_MULTICOL(object);

  switch (arg_id)
  {
  case ARG_VSPACING:
    gtk_multicol_set_spacing(multicol, multicol->hspacing,
      GTK_VALUE_INT(*arg));
    break;
  case ARG_HSPACING:
    gtk_multicol_set_spacing(multicol, GTK_VALUE_INT(*arg),
      multicol->vspacing);
    break;
  default:
    arg->type = GTK_TYPE_INVALID;
    break;
  }
}


GtkWidget *gtk_multicol_new(gint nrows)
{
  GtkMultiCol *multicol;

  multicol = gtk_type_new(gtk_multicol_get_type());

  multicol->nrows = nrows;

  return GTK_WIDGET(multicol);
}

static void gtk_multicol_size_request(GtkWidget * widget,
  GtkRequisition * requisition)
{
  GtkBox *box;
  GtkMultiCol *multicol;
  GtkBoxChild *child;
  GList *children;
  gint maxwidth, maxheight, nm, ncol, diff, diffp;

  g_return_if_fail(widget != NULL);
  g_return_if_fail(GTK_IS_MULTICOL(widget));
  g_return_if_fail(requisition != NULL);

  box = GTK_BOX(widget);
  multicol = GTK_MULTICOL(widget);

  maxwidth = 0;
  maxheight = 0;
  nm = 0;

  children = box->children;

  while(children)
  {
    child = children->data;
    children = children->next;

    if(GTK_WIDGET_VISIBLE(child->widget))
    {
      gtk_widget_size_request(child->widget, &child->widget->requisition);

      maxwidth =
        MAX(maxwidth,
        (child->widget->requisition.width + child->padding * 2));
      maxheight =
        MAX(maxheight,
        child->widget->requisition.height + child->padding * 2);

      nm++;
    }
  }

  if(multicol->nrows)
    ncol = nm / multicol->nrows + (nm % multicol->nrows != 0);
  else
  {
    ncol = 0;
    diff = 20000000;
    do
    {
      diffp = diff;
      ncol++;
      diff =
        abs((nm / ncol + (nm % ncol != 0)) * maxheight - ncol * maxwidth);
    }
    while(diffp > diff);

    ncol--;
  }

  if(nm)
  {
    requisition->width =
      ncol * (maxwidth + multicol->hspacing) + multicol->hspacing;

    requisition->height =
      (nm / ncol + (nm % ncol !=
        0)) * (maxheight + multicol->vspacing) + multicol->vspacing;
  }

  requisition->width += GTK_CONTAINER(box)->border_width * 2;
  requisition->height += GTK_CONTAINER(box)->border_width * 2;
}

static void gtk_multicol_size_allocate(GtkWidget * widget,
  GtkAllocation * allocation)
{
  GtkBox *box;
  GtkMultiCol *multicol;
  GtkBoxChild *child;
  GList *children;
  GList *pchildren;
  GtkAllocation child_allocation;
  gint maxwidth, maxheight, nrow, nm, col, row;

  g_return_if_fail(widget != NULL);
  g_return_if_fail(GTK_IS_MULTICOL(widget));
  g_return_if_fail(allocation != NULL);

  box = GTK_BOX(widget);
  multicol = GTK_MULTICOL(widget);

  widget->allocation = *allocation;

  maxwidth = 0;
  maxheight = 0;
  nm = 0;

  children = box->children;
  while(children)
  {
    child = children->data;
    children = children->next;

    if(GTK_WIDGET_VISIBLE(child->widget))
    {
      gtk_widget_size_request(child->widget, &child->widget->requisition);

      maxwidth =
        MAX(maxwidth,
        (child->widget->requisition.width + child->padding * 2));
      maxheight =
        MAX(maxheight,
        child->widget->requisition.height + child->padding * 2);

      nm++;
    }
  }

  children = box->children;
  if(nm)
  {
    if(multicol->nrows)
      nrow = multicol->nrows;
    else
      nrow = (allocation->height - multicol->hspacing) /
        (maxheight + multicol->hspacing);

    col = 0;
    row = 0;

    pchildren = children;

    while(children)
    {
      child = children->data;
      pchildren = children;
      children = children->next;

      if((child->pack == GTK_PACK_START) && GTK_WIDGET_VISIBLE(child->widget))
      {
        child_allocation.x = allocation->x + multicol->hspacing +
          col * (maxwidth + multicol->hspacing);
        child_allocation.y = allocation->y + multicol->vspacing +
          row * (maxheight + multicol->vspacing);

        child_allocation.width = maxwidth;
        child_allocation.height = maxheight;

        gtk_widget_size_allocate(child->widget, &child_allocation);

        row++;

        if(row >= nrow)
        {
          row = 0;
          col++;
        }
      }
    }

    children = pchildren;
    while(children)
    {
      child = children->data;
      children = children->prev;

      if((child->pack == GTK_PACK_END) && GTK_WIDGET_VISIBLE(child->widget))
      {
        child_allocation.x = allocation->x + multicol->hspacing +
          col * (maxwidth + multicol->hspacing);
        child_allocation.y = allocation->y + multicol->vspacing +
          row * (maxheight + multicol->vspacing);

        child_allocation.width = maxwidth;
        child_allocation.height = maxheight;

        gtk_widget_size_allocate(child->widget, &child_allocation);

        row++;

        if(row >= nrow)
        {
          row = 0;
          col++;
        }
      }
    }
  }
}

void gtk_multicol_set_spacing(GtkMultiCol * multicol,
  gint hspacing, gint vspacing)
{
  if(multicol->hspacing == hspacing && multicol->vspacing == vspacing)
    return;

  multicol->hspacing = hspacing;
  multicol->vspacing = vspacing;

  if(GTK_WIDGET_VISIBLE(multicol))
    gtk_widget_queue_resize(GTK_WIDGET(multicol));
}

void gtk_multicol_set_number_of_rows(GtkMultiCol * multicol, gint nrows)
{
  if(multicol->nrows == nrows)
    return;

  multicol->nrows = nrows;

  if(GTK_WIDGET_VISIBLE(multicol))
    gtk_widget_queue_resize(GTK_WIDGET(multicol));
}

#endif /*** GTK_FACE ***/
