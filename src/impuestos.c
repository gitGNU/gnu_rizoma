/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4;
       c-indentation-style: gnu -*- */
/*impuestos.c
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

#include<string.h>

#include"tipos.h"
#include"postgres-functions.h"
#include"errors.h"
#include"dimentions.h"
#include"utils.h"

GtkWidget *impuesto_tasa;
GtkWidget *impuesto_name;

GtkWidget *edit_tasa;
GtkWidget *edit_name;

GtkWidget *tree_tasks;
GtkListStore *store_tasks;

void
FillImpuestos (void)
{
  GtkWidget *widget;
  GtkListStore *store;
  GtkTreeIter iter;
  PGresult *res;
  gint i, tuples;

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "treeview_taxes"));
  store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(widget)));

  gtk_list_store_clear (store);

  res = EjecutarSQL ("SELECT id, descripcion, monto FROM select_impuesto()");

  if (res != NULL)
    tuples = PQntuples (res);

  if (tuples != 0)
    {
      for (i = 0; i < tuples; i++)
	{
	  gtk_list_store_append (store, &iter);
	  gtk_list_store_set (store, &iter,
			      0, PQvaluebycol (res, i, "id"),
			      1, PQvaluebycol (res, i, "descripcion"),
			      2, PQvaluebycol (res, i, "monto"),
			      -1);
	}
    }
}

void
AddImpuesto (void)
{
  GtkWidget *widget;
  gchar *task_name;
  gchar *task_tasa;
  PGresult *res;

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_admin_addtax_name"));
  task_name = g_strdup (gtk_entry_get_text (GTK_ENTRY (widget)));

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_admin_addtax_value"));
  task_tasa = g_strdup (gtk_entry_get_text (GTK_ENTRY (widget)));

  res = EjecutarSQL (g_strdup_printf ("SELECT insert_impuesto('%s', %s)",
				      task_name, task_tasa));

  if (res != NULL)
    {
      widget = GTK_WIDGET(gtk_builder_get_object(builder, "statusbar"));
      statusbar_push (GTK_STATUSBAR(widget), "Se agrego el impuesto con exito", 3000);
      FillImpuestos ();
    }
  else
    {
      ErrorMSG (impuesto_name, "No se pudo agregar el impuesto");
    }

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_admin_addtax_name"));
  gtk_entry_set_text (GTK_ENTRY (widget), "");

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_admin_addtax_value"));
  gtk_entry_set_text (GTK_ENTRY (widget), "");
}

void
DelTask (void)
{
  GtkWidget *widget;
  GtkTreeSelection *selection;
  GtkListStore *store;
  GtkTreeIter iter;
  PGresult *res;
  gchar *id;
  gchar *q;
  gboolean borrar = FALSE;

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "treeview_taxes"));
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (widget));
  store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(widget)));

  if (gtk_tree_selection_get_selected (selection, NULL, &iter))
    {
      gtk_tree_model_get (GTK_TREE_MODEL (store), &iter,
			  0, &id,
			  -1);

      if (!(g_str_equal (id, "0")))
	{
	  q = g_strdup_printf("select count(*) from producto where otros = %s", id);
	  res = EjecutarSQL(q);
	  g_free (q);

	  if ((res != NULL) && (g_str_equal(PQgetvalue(res, 0, 0), "0")))
	    borrar = TRUE;
	  else
	    {
	      GtkWidget *dialog;
	      gint result;
	      GtkWidget *label;
	      dialog = gtk_dialog_new_with_buttons ("Impuestos",
						    GTK_WINDOW(gtk_widget_get_toplevel(widget)),
						    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
						    GTK_STOCK_CANCEL,
						    GTK_RESPONSE_REJECT,
						    GTK_STOCK_OK,
						    GTK_RESPONSE_ACCEPT,
						    NULL);
	      label = gtk_label_new(g_strdup_printf("Actualmente hay <b>%s</b> productos utilizando el impuesto que ud desea borrar.\n"
						    "¿Desea realmente eliminar el impuesto?", PQgetvalue(res, 0, 0)));
	      gtk_label_set_use_markup(GTK_LABEL(label), TRUE);

	      gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox),
				 label);
	      gtk_widget_show_all (dialog);
	      result = gtk_dialog_run (GTK_DIALOG (dialog));
	      if (result == GTK_RESPONSE_ACCEPT)
		borrar = TRUE;

	      gtk_widget_destroy (dialog);

	    }
	  if (borrar)
	    {
	      q = g_strdup_printf ("UPDATE producto SET otros=-1 WHERE otros=%s", id);
	      res = EjecutarSQL (q);
	      g_free(q);

	      res = EjecutarSQL (g_strdup_printf ("DELETE FROM impuesto WHERE id=%s", id));

	      if (res != NULL)
		{
		  widget = GTK_WIDGET(gtk_builder_get_object(builder, "statusbar"));
		  statusbar_push (widget, "El impuesto se elimino con exito", 3000);
		  FillImpuestos ();
		}
	      else
		ErrorMSG (impuesto_name, "No se pudo eliminar el impuesto");
	    }
	}
      else
	ErrorMSG (impuesto_name, "No se puede eliminar el imipuesto IVA");
    }
  else
    AlertMSG(widget, "Debe seleccionar un impuesto de la lista");
}

void
EditTask (GtkWidget *widget, gpointer data)
{
  gchar *new_name = g_strdup (gtk_entry_get_text (GTK_ENTRY (edit_name)));
  gchar *new_tasa = g_strdup (gtk_entry_get_text (GTK_ENTRY (edit_tasa)));
  PGresult *res;
  gchar *q;
  gboolean edit = (gboolean) data;
  GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree_tasks));
  GtkTreeIter iter;
  gchar *id;

  if (edit == TRUE)
    {
      if (gtk_tree_selection_get_selected (selection, NULL, &iter) == TRUE)
	{
	  gtk_tree_model_get (GTK_TREE_MODEL (store_tasks), &iter,
			      0, &id,
			      -1);
	  q = g_strdup_printf ("UPDATE impuestos SET descripcion='%s', "
			       "monto=%s WHERE id=%s", new_name, CUT(new_tasa), id);
	  res = EjecutarSQL (q);

	  if (res != NULL)
	    {
	      ExitoMSG (main_window, "Su Impuesto se edito con exito");
	      FillImpuestos ();
	    }
	  else
	    ErrorMSG (GTK_WIDGET (selection), "No se pudo editar su impuesto");
	}
      else
	ErrorMSG (GTK_WIDGET (selection), "No se pudo editar el impuesto");
    }

  gtk_widget_destroy (gtk_widget_get_toplevel (widget));
  gtk_widget_set_sensitive (main_window, TRUE);

}

void
EditTaskWin (void)
{
  GtkWidget *window;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *hbox2;
  GtkWidget *label;
  GtkWidget *button;

  GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree_tasks));
  GtkTreeIter iter;
  gchar *name;
  gchar *tasa;


  if (gtk_tree_selection_get_selected (selection, NULL, &iter) == TRUE)
    {
      gtk_tree_model_get (GTK_TREE_MODEL (store_tasks), &iter,
			  1, &name,
			  2, &tasa,
			  -1);

      gtk_widget_set_sensitive (main_window, FALSE);

      window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
      gtk_window_set_title (GTK_WINDOW (window), "Editar Impuesto");
      gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER_ALWAYS);
      //      gtk_window_set_transient_for (GTK_WINDOW (window), GTK_WINDOW (main_window));
      gtk_widget_set_size_request (window, -1, 100);
      gtk_widget_show (window);

      g_signal_connect (G_OBJECT (window), "destroy",
			G_CALLBACK (EditTask), (gpointer)FALSE);

      vbox = gtk_vbox_new (FALSE, 3);
      gtk_container_add (GTK_CONTAINER (window), vbox);
      gtk_widget_show (vbox);

      hbox = gtk_hbox_new (FALSE, 3);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
      gtk_widget_show (hbox);

      hbox2 = gtk_vbox_new (TRUE, 2);
      gtk_box_pack_start (GTK_BOX (hbox), hbox2, FALSE, FALSE, 3);
      gtk_widget_show (hbox2);
      label = gtk_label_new ("Tasa %s");
      gtk_widget_show (label);
      gtk_box_pack_start (GTK_BOX (hbox2), label, FALSE, FALSE, 0);
      edit_tasa = gtk_entry_new_with_max_length (5);
      gtk_entry_set_text (GTK_ENTRY (edit_tasa), tasa);
      gtk_widget_set_size_request (edit_tasa, 30, -1);
      gtk_box_pack_start (GTK_BOX (hbox2), edit_tasa, FALSE, FALSE, 0);
      gtk_widget_show (edit_tasa);

      hbox2 = gtk_vbox_new (TRUE, 2);
      gtk_box_pack_start (GTK_BOX (hbox), hbox2, FALSE, FALSE, 3);
      gtk_widget_show (hbox2);
      label = gtk_label_new ("Nombre");
      gtk_widget_show (label);
      gtk_box_pack_start (GTK_BOX (hbox2), label, FALSE, FALSE, 0);
      edit_name = gtk_entry_new_with_max_length (60);
      gtk_entry_set_text (GTK_ENTRY (edit_name), name);
      gtk_widget_set_size_request (edit_name, 150, -1);
      gtk_box_pack_start (GTK_BOX (hbox2), edit_name, FALSE, FALSE, 0);
      gtk_widget_show (edit_name);

      hbox = gtk_hbox_new (FALSE, 3);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
      gtk_widget_show (hbox);

      button = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
      gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 3);
      gtk_widget_show (button);

      g_signal_connect (G_OBJECT (button), "clicked",
			G_CALLBACK (EditTask), (gpointer)FALSE);

      button = gtk_button_new_from_stock (GTK_STOCK_OK);
      gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);
      gtk_widget_show (button);

      g_signal_connect (G_OBJECT (button), "clicked",
			G_CALLBACK (EditTask), (gpointer)TRUE);
    }
}

void
taxes_box ()
{
  GtkListStore *store;
  GtkWidget *widget;

  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;

  store = gtk_list_store_new (3,
			      G_TYPE_STRING, //ID
			      G_TYPE_STRING,
			      G_TYPE_STRING);

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "treeview_taxes"));
  gtk_tree_view_set_model(GTK_TREE_VIEW(widget), GTK_TREE_MODEL(store));

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("ID", renderer,
						     "text", 0,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (widget), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_min_width (column, 25);
  gtk_tree_view_column_set_max_width (column, 25);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Nombre", renderer,
						     "text", 1,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (widget), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_min_width (column, 200);
  gtk_tree_view_column_set_max_width (column, 200);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Tasa %", renderer,
						     "text", 2,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (widget), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_min_width (column, 30);
  gtk_tree_view_column_set_max_width (column, 30);

  FillImpuestos ();
}
