/* -*- Mode: C; tab-width: 4; ident-tabs-mode: nil; c-basic-offset: 4 -*- */
/*errors.c
*
*    Copyright (C) 2004 Rizoma Tecnologia Limitada <info@rizoma.cl>
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

#include<gtk/gtk.h>

#include"tipos.h"

GtkWidget *error_window;

gboolean closing = FALSE;

void
CloseErrorWindow (GtkButton *button, gpointer data)
{
  GtkWidget *widget = (GtkWidget *) data;
  
  if (closing == FALSE)
    {
      closing = TRUE;
      
      gtk_widget_set_sensitive (gtk_widget_get_toplevel (widget), TRUE);
      
      gtk_window_set_focus (GTK_WINDOW (gtk_widget_get_toplevel (widget)), 
			    widget);

      gtk_widget_destroy (error_window);
      
      error_window = NULL;

      closing = FALSE;
    }
}

gint
ErrorMSG (GtkWidget *widget, gchar *motivo)
{
  GtkWidget *image;
  GtkWidget *label;
  GtkWidget *button;

  GtkWidget *vbox;
  GtkWidget *hbox;
  
  if (error_window != NULL)
    return -1;
  
  gtk_widget_set_sensitive (gtk_widget_get_toplevel (widget), FALSE);
  
  error_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_widget_set_size_request (error_window, -1, 130);
  gtk_window_set_resizable (GTK_WINDOW (error_window), FALSE);
  gtk_window_set_position (GTK_WINDOW (error_window), GTK_WIN_POS_CENTER_ALWAYS);
  gtk_widget_show (error_window);
  gtk_window_present (GTK_WINDOW (error_window));
  //  gtk_window_set_transient_for (GTK_WINDOW (error_window), GTK_WINDOW (main_window));
  gtk_container_set_border_width (GTK_CONTAINER (error_window), 20);

  g_signal_connect (G_OBJECT (error_window), "destroy",
		   G_CALLBACK (CloseErrorWindow), (gpointer) widget);
  
  vbox = gtk_vbox_new (FALSE, 3);
  gtk_widget_show (vbox);
  gtk_container_add (GTK_CONTAINER (error_window), vbox);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  image = gtk_image_new_from_stock (GTK_STOCK_DIALOG_ERROR, GTK_ICON_SIZE_DIALOG);
  gtk_widget_show (image);
  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 3);

  label = gtk_label_new (motivo);
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
  
  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  button = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
  gtk_widget_show (button);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);

  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (CloseErrorWindow), (gpointer)widget);

  gtk_window_set_focus (GTK_WINDOW (error_window), button);

  return 0;
}

gint
AlertMSG (GtkWidget *widget, gchar *motivo)
{
  GtkWidget *image;
  GtkWidget *label;
  GtkWidget *button;

  GtkWidget *vbox;
  GtkWidget *hbox;

  if (error_window != NULL)
    return -1;

  gtk_widget_set_sensitive (gtk_widget_get_toplevel (widget), FALSE);

  error_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_widget_set_size_request (error_window, -1, 130);
  gtk_window_set_resizable (GTK_WINDOW (error_window), FALSE);
  gtk_window_set_position (GTK_WINDOW (error_window), GTK_WIN_POS_CENTER_ALWAYS);
  gtk_widget_show (error_window);
  gtk_window_present (GTK_WINDOW (error_window));
  //  gtk_window_set_transient_for (GTK_WINDOW (error_window), GTK_WINDOW (main_window));

  gtk_container_set_border_width (GTK_CONTAINER (error_window), 20);

  g_signal_connect (G_OBJECT (error_window), "destroy",
		    G_CALLBACK (CloseErrorWindow), NULL);

  
  vbox = gtk_vbox_new (FALSE, 3);
  gtk_widget_show (vbox);
  gtk_container_add (GTK_CONTAINER (error_window), vbox);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  image = gtk_image_new_from_stock (GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_DIALOG);
  gtk_widget_show (image);
  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 3);

  label = gtk_label_new (motivo);
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
  
  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  button = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
  gtk_widget_show (button);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);

  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (CloseErrorWindow), (gpointer)widget);

  gtk_window_set_focus (GTK_WINDOW (error_window), button);

  return 0;
}

gint
ExitoMSG (GtkWidget *widget, gchar *motivo)
{
  GtkWidget *image;
  GtkWidget *label;
  GtkWidget *button;

  GtkWidget *vbox;
  GtkWidget *hbox;

  if (error_window != NULL)
    return -1;

  gtk_widget_set_sensitive (gtk_widget_get_toplevel (widget), FALSE);
  
  error_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_widget_set_size_request (error_window, -1, 130);
  gtk_window_set_resizable (GTK_WINDOW (error_window), FALSE);
  gtk_window_set_position (GTK_WINDOW (error_window), GTK_WIN_POS_CENTER_ALWAYS);
  gtk_widget_show (error_window);
  gtk_window_present (GTK_WINDOW (error_window));
  //  gtk_window_set_transient_for (GTK_WINDOW (error_window), GTK_WINDOW (main_window));

  gtk_container_set_border_width (GTK_CONTAINER (error_window), 20);

  g_signal_connect (G_OBJECT (error_window), "destroy",
		    G_CALLBACK (CloseErrorWindow), NULL);
  
  vbox = gtk_vbox_new (FALSE, 3);
  gtk_widget_show (vbox);
  gtk_container_add (GTK_CONTAINER (error_window), vbox);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  image = gtk_image_new_from_stock (GTK_STOCK_APPLY, GTK_ICON_SIZE_DIALOG);
  gtk_widget_show (image);
  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 3);

  label = gtk_label_new (motivo);
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
  
  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  button = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
  gtk_widget_show (button);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);

  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (CloseErrorWindow), (gpointer)widget);

  gtk_window_set_focus (GTK_WINDOW (error_window), button);
  
  return 0;
}
