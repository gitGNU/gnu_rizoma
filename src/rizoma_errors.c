/* -*- Mode: C; tab-width: 4; indent-tabs-mode: f; c-basic-offset: 4 -*- */
/*rizoma_errors.c
*
*    Copyright (C) 2006 Rizoma Tecnologia Limitada <info@rizoma.cl>
*
*    This file is part of rizoma.
*
*    Rizoma is free software; you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation; either version 2 of the License, or
*    (at your option) any later version.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program; if not, write to the Free Software
*    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include<stdio.h>
#include<stdlib.h>
#include<gtk/gtk.h>

#include<rizoma_errors.h>

GtkWidget *error_window;

gboolean rizoma_error_closing = FALSE;

gint
rizoma_errors_set (gchar *error, const gchar *function, gint type)
{
  if (rizoma_error == NULL)
    {
      rizoma_error = (RizomaErrors *) malloc (sizeof (RizomaErrors));
      rizoma_error->motivo = g_strdup (error);
      rizoma_error->funcion = function;
      rizoma_error->type = type;
    }

  return 0;
}

gint
rizoma_errors_clean (void)
{
  free (rizoma_error);

  rizoma_error = NULL;

  return 0;
}

void
close_rizoma_error_window (GtkButton *button, gpointer data)
{
  GtkWidget *widget = (GtkWidget *) data;

  if (rizoma_error_closing == FALSE)
    {
      rizoma_error_closing = TRUE;

      gtk_widget_set_sensitive (gtk_widget_get_toplevel (widget), TRUE);

      gtk_window_set_focus (GTK_WINDOW (gtk_widget_get_toplevel (widget)),
			    widget);

      gtk_widget_destroy (error_window);

      error_window = NULL;

      rizoma_errors_clean ();

      rizoma_error_closing = FALSE;
    }
}

gint
rizoma_error_window (GtkWidget *widget)
{
  GtkWidget *image;
  GtkWidget *label;
  GtkWidget *button;

  gchar *trace;

  GtkWidget *vbox;
  GtkWidget *hbox;

  if (error_window != NULL)
    return -1;

  if( widget != NULL )
	  gtk_widget_set_sensitive (gtk_widget_get_toplevel (widget), FALSE);

  error_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_widget_set_size_request (error_window, -1, -1);
  gtk_window_set_resizable (GTK_WINDOW (error_window), FALSE);
  gtk_window_set_position (GTK_WINDOW (error_window), GTK_WIN_POS_CENTER_ALWAYS);
  gtk_widget_show (error_window);
  gtk_window_present (GTK_WINDOW (error_window));

  gtk_container_set_border_width (GTK_CONTAINER (error_window), 20);

  g_signal_connect (G_OBJECT (error_window), "destroy",
		    G_CALLBACK (close_rizoma_error_window), NULL);

  vbox = gtk_vbox_new (FALSE, 3);
  gtk_widget_show (vbox);
  gtk_container_add (GTK_CONTAINER (error_window), vbox);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  switch (rizoma_error->type)
    {
    case APPLY:
      image = gtk_image_new_from_stock (GTK_STOCK_APPLY, GTK_ICON_SIZE_DIALOG);
      break;
    case ERROR:
      image = gtk_image_new_from_stock (GTK_STOCK_DIALOG_ERROR, GTK_ICON_SIZE_DIALOG);
      break;
    case ALERT:
      image = gtk_image_new_from_stock (GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_DIALOG);
      break;
    default:
      image = gtk_image_new_from_stock (GTK_STOCK_DIALOG_ERROR, GTK_ICON_SIZE_DIALOG);
      break;
    }


  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 3);
  gtk_widget_show (image);

  label = gtk_label_new (rizoma_error->motivo);
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  if (rizoma_error->type == APPLY)
    trace = g_strdup_printf ("\nAcción en la función: %s\n", rizoma_error->funcion);
  else
    trace = g_strdup_printf ("\nError en la función: %s\n", rizoma_error->funcion);


  label = gtk_label_new (trace);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
  gtk_widget_show (label);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  button = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
  gtk_widget_show (button);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);

  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (close_rizoma_error_window), (gpointer)widget);

  gtk_window_set_focus (GTK_WINDOW (error_window), button);

  rizoma_errors_clean ();

  return 0;
}
