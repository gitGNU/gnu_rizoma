/*impuestos.c
*
*    Copyright 2004 Rizoma Tecnologia Limitada <jonathan@rizoma.cl>
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
*    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include<gtk/gtk.h>

#include<string.h>

#include"tipos.h"
#include"postgres-functions.h"
#include"errors.h"
#include"dimentions.h"

GtkWidget *impuesto_tasa;
GtkWidget *impuesto_name;

GtkWidget *edit_tasa;
GtkWidget *edit_name;

GtkWidget *tree_tasks;
GtkListStore *store_tasks;

void
FillImpuestos (void)
{
  GtkTreeIter iter;
  PGresult *res;
  gint i, tuples;
  gtk_list_store_clear (store_tasks); 
      
  res = EjecutarSQL ("SELECT id, descripcion, monto FROM select_impuesto()");

  if (res != NULL)
    tuples = PQntuples (res);
  
  if (res != NULL && tuples != 0)
    {
      //      gtk_list_store_clear (store_tasks); 
      
      for (i = 0; i < tuples; i++)
	{
	  gtk_list_store_append (store_tasks, &iter);
	  gtk_list_store_set (store_tasks, &iter,
			      0, PQgetvalue (res, i, 0),
			      1, PQgetvalue (res, i, 1),
			      2, PQgetvalue (res, i, 2),
			      -1);
	}
    }  
}

void
AddImpuesto (void)
{
  gchar *task_name = g_strdup (gtk_entry_get_text (GTK_ENTRY (impuesto_name)));
  gchar *task_tasa = g_strdup (gtk_entry_get_text (GTK_ENTRY (impuesto_tasa)));
  PGresult *res;

  res = EjecutarSQL (g_strdup_printf ("SELECT insert_impuesto('%s', %s)",
				      task_name, task_tasa));

  if (res != NULL)
    {
      ExitoMSG (impuesto_name, "Se agrego el impuesto con exito");
      gtk_entry_set_text (GTK_ENTRY (impuesto_name), "");
      gtk_entry_set_text (GTK_ENTRY (impuesto_tasa), "");
      FillImpuestos ();
    }
  else
    {
      ErrorMSG (impuesto_name, "No se pudo agregar el impuesto");
      gtk_entry_set_text (GTK_ENTRY (impuesto_name), "");
      gtk_entry_set_text (GTK_ENTRY (impuesto_tasa), "");
    }
}

void
DelTask (void)
{
  GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree_tasks));
  GtkTreeIter iter;
  PGresult *res;
  gchar *id;
  gchar *q;

  if (gtk_tree_selection_get_selected (selection, NULL, &iter) == TRUE)
    {
      gtk_tree_model_get (GTK_TREE_MODEL (store_tasks), &iter,
			  0, &id,
			  -1);

      if (strcmp (id, "0") != 0)
	{
	  res = EjecutarSQL (g_strdup_printf ("DELETE FROM impuesto WHERE id=%s", id));

	  if (res != NULL)
	    {
	      ExitoMSG (impuesto_name, "El impuesto se elimino con exito");
	      q = g_strdup_printf ("UPDATE producto SET otros=-1 WHERE otros=%s", id);
	      res = EjecutarSQL (q);
	      FillImpuestos ();
	    }
	  else
	    ErrorMSG (impuesto_name, "No se pudo eliminar el impuesto");
	}
      else
	ErrorMSG (impuesto_name, "No se puede eliminar el imipuesto IVA");
    }   
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
Impuestos (GtkWidget *main_box)
{
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *vbox2;
  GtkWidget *hbox;
  GtkWidget *hbox2;
  GtkWidget *label;
  GtkWidget *button;

  GtkWidget *scroll;
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;

  vbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (vbox);
  gtk_box_pack_start (GTK_BOX (main_box), vbox, FALSE, FALSE, 3);

  frame = gtk_frame_new ("Agregar Impuestos");
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 3);
  gtk_widget_show (frame);

  vbox2 = gtk_vbox_new (FALSE, 3);
  gtk_widget_show (vbox2);
  gtk_container_add (GTK_CONTAINER (frame), vbox2);
  
  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 3);

  hbox2 = gtk_vbox_new (TRUE, 2);
  gtk_widget_show (hbox2);
  label = gtk_label_new ("Tasa %");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox2), label, FALSE, FALSE, 0);
  impuesto_tasa = gtk_entry_new_with_max_length (5);
  gtk_widget_set_size_request (impuesto_tasa, 30, -1);
  gtk_box_pack_start (GTK_BOX (hbox2), impuesto_tasa, FALSE, FALSE, 0);
  gtk_widget_show (impuesto_tasa);

  gtk_box_pack_start (GTK_BOX (hbox), hbox2, FALSE, FALSE, 3);
  
  hbox2 = gtk_vbox_new (TRUE, 2);
  gtk_widget_show (hbox2);
  label = gtk_label_new ("Nombre");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox2), label, FALSE, FALSE, 0);
  impuesto_name = gtk_entry_new_with_max_length (60);
  gtk_widget_set_size_request (impuesto_name, 150, -1);
  gtk_box_pack_start (GTK_BOX (hbox2), impuesto_name, FALSE, FALSE, 0);
  gtk_widget_show (impuesto_name);

  gtk_box_pack_start (GTK_BOX (hbox), hbox2, FALSE, FALSE, 3);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 3);

  button = gtk_button_new_from_stock (GTK_STOCK_ADD);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (AddImpuesto), NULL);

  frame = gtk_frame_new ("Agregar Impuestos");
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 3);
  gtk_widget_show (frame);

  hbox2 = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox2);
  gtk_container_add (GTK_CONTAINER (frame), hbox2);
    
  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_show (scroll);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_AUTOMATIC);
  gtk_widget_set_size_request (scroll, 300, 120);
  gtk_box_pack_start (GTK_BOX (hbox2), scroll, FALSE, FALSE, 3);

  store_tasks = gtk_list_store_new (3,
				    G_TYPE_STRING,
				    G_TYPE_STRING,
				    G_TYPE_STRING);

  tree_tasks = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store_tasks));
  gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (tree_tasks), TRUE);
  gtk_container_add (GTK_CONTAINER (scroll), tree_tasks);
  gtk_widget_show (tree_tasks);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("ID", renderer,
						     "text", 0,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree_tasks), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_min_width (column, 25);
  gtk_tree_view_column_set_max_width (column, 25);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Nombre", renderer,
						     "text", 1,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree_tasks), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_min_width (column, 200);
  gtk_tree_view_column_set_max_width (column, 200);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Tasa %", renderer,
						     "text", 2,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree_tasks), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_min_width (column, 30);
  gtk_tree_view_column_set_max_width (column, 30);

  FillImpuestos ();

  vbox2 = gtk_vbox_new (FALSE, 3);
  gtk_box_pack_end (GTK_BOX (hbox2), vbox2, FALSE, FALSE, 3);
  gtk_widget_show (vbox2);
  
  button = gtk_button_new_from_stock (GTK_STOCK_DELETE);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 3);
  gtk_widget_show (button);
  
  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (DelTask), NULL);

  button = gtk_button_new_with_label ("Editar");
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 3);
  gtk_widget_show (button);
  
  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (EditTaskWin), NULL);
  
}
