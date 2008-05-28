/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4;
       c-indentation-style: gnu -*- */
/*usuarios.c
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
#include"credito.h"
#include"usuario.h"
#include"encriptar.h"
#include"dimentions.h"

#include<rizoma_errors.h>

GtkWidget *old_pass;
GtkWidget *new_pass_1;
GtkWidget *new_pass_2;

GtkWidget *rut_1;
GtkWidget *rut_check;
GtkWidget *nombre;
GtkWidget *apell_p;
GtkWidget *apell_m;
GtkWidget *username;
GtkWidget *new_pass1;
GtkWidget *new_pass2;
GtkWidget *id_box;

GtkWidget *tree_users;
GtkListStore *store_users;

void
FillUsers (void)
{
  GtkWidget *widget;
  GtkListStore *store;
  GtkTreeIter iter;
  PGresult *res;
  PGresult *res2;
  gint i, tuples;
  gchar *entrada, *salida;
  gchar *q;

  res = EjecutarSQL ("SELECT id, rut||'-'||dv, nombre, apell_p, apell_m "
		     "FROM select_usuario() as "
		     "(id int,rut int4, dv varchar(1),usuario varchar(30),"
		     "passwd varchar(400),nombre varchar(75),"
		     "apell_p varchar(75),apell_m varchar(75),"
		     "fecha_ingreso timestamp,\"level\" int2);");

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "treeview_users"));
  store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(widget)));

  tuples = PQntuples (res);

  if (res != NULL && tuples != 0)
    {
      gtk_list_store_clear (store);

      for (i = 0; i < tuples; i++)
	{
	  q = g_strdup_printf ("select * from select_asistencia(%s)",
			       PQvaluebycol (res, i, "id"));
	  res2 = EjecutarSQL (q);
	  g_free (q);

	  if (res2 != NULL && PQntuples (res2) != 0)
	    {
	      entrada = g_strdup_printf ("%s/%s/%s %s:%s",
					 PQvaluebycol (res2, 0, "entrada_day"),
					 PQvaluebycol (res2, 0, "entrada_month"),
					 PQvaluebycol (res2, 0, "entrada_year"),
					 PQvaluebycol (res2, 0, "entrada_hour"),
					 PQvaluebycol (res2, 0, "entrada_min"));

	      if (!(g_str_equal(PQvaluebycol (res2, 0, "salida_year"), "-1")))
		salida = g_strdup_printf ("%s/%s/%s %s:%s",
					  PQvaluebycol (res2, 0, "salida_day"),
					  PQvaluebycol (res2, 0, "salida_month"),
					  PQvaluebycol (res2, 0, "salida_year"),
					  PQvaluebycol (res2, 0, "salida_hour"),
					  PQvaluebycol (res2, 0, "salida_min"));
	      else
		salida = "Trabajando";
	    }
	  else
	    {
	      entrada = "No ha Ingresado";
	      salida = "No ha Ingresado";
	    }

	  gtk_list_store_append (store, &iter);
	  gtk_list_store_set (store, &iter,
			      0, PQvaluebycol (res, i, "id"),
			      1, PQgetvalue (res, i, 1), //rut
			      2, PQvaluebycol (res, i, "nombre"),
			      3, PQvaluebycol (res, i, "apell_p"),
			      4, PQvaluebycol (res, i, "apell_m"),
			      5, entrada,
			      6, salida,
			      -1);
	}
    }
}

void
DeleteUser (GtkWidget *widget, gpointer data)
{
  GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree_users));
  GtkTreeIter iter;
  gchar *id;
  gboolean accepted = (gboolean) data;
  PGresult *res;

  if (gtk_tree_selection_get_selected (selection, NULL, &iter) == TRUE &&
      accepted == TRUE)
    {
      gtk_tree_model_get (GTK_TREE_MODEL (store_users), &iter,
			  0, &id,
			  -1);

      if (strcmp (id, "1") != 0)
	{
	  res = EjecutarSQL (g_strdup_printf ("select * from delete_user(%s)", id));

	  FillUsers ();
	}
      else
	AlertMSG (GTK_WIDGET (store_users), "No se puede elminar el usuario admin");

      gtk_widget_destroy (gtk_widget_get_toplevel (widget));
    }
  else
    gtk_widget_destroy (gtk_widget_get_toplevel (widget));
}

void
AskDelete (void)
{
  GtkWidget *window;
  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *image;
  GtkWidget *label;
  GtkWidget *button;
  GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree_users));
  GtkTreeIter iter;

  if (gtk_tree_selection_get_selected (selection, NULL, &iter) == TRUE)
    {
      window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
      gtk_window_set_title (GTK_WINDOW (window), "Borrar Usuario");
      gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER_ALWAYS);
      //      gtk_window_set_transient_for (GTK_WINDOW (window), GTK_WINDOW (main_window));
      gtk_widget_show (window);

      vbox = gtk_vbox_new (FALSE, 3);
      gtk_container_add (GTK_CONTAINER (window), vbox);
      gtk_widget_show (vbox);

      hbox = gtk_hbox_new (FALSE, 3);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
      gtk_widget_show (hbox);

      image = gtk_image_new_from_stock (GTK_STOCK_DIALOG_QUESTION, GTK_ICON_SIZE_DIALOG);
      gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 3);
      gtk_widget_show (image);

      label = gtk_label_new ("Desea eliminar al usuario");
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
      gtk_widget_show (label);

      hbox = gtk_hbox_new (FALSE, 3);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
      gtk_widget_show (hbox);

      button = gtk_button_new_from_stock (GTK_STOCK_NO);
      gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 3);
      gtk_widget_show (button);

      g_signal_connect (G_OBJECT (button), "clicked",
			G_CALLBACK (DeleteUser), (gboolean)FALSE);

      button = gtk_button_new_from_stock (GTK_STOCK_YES);
      gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);
      gtk_widget_show (button);

      g_signal_connect (G_OBJECT (button), "clicked",
			G_CALLBACK (DeleteUser), (gpointer)TRUE);
    }
}

void
user_box ()
{
  GtkWidget *widget;
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;
  GtkListStore *store_users;

  store_users = gtk_list_store_new (7,
				    G_TYPE_STRING, //ID
				    G_TYPE_STRING, //RUT
				    G_TYPE_STRING, //nombre
				    G_TYPE_STRING,
				    G_TYPE_STRING,
				    G_TYPE_STRING,
				    G_TYPE_STRING);

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "treeview_users"));
  gtk_tree_view_set_model(GTK_TREE_VIEW(widget), GTK_TREE_MODEL(store_users));

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("ID", renderer,
						     "text", 0,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (widget), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_min_width (column, 40);
  gtk_tree_view_column_set_max_width (column, 40);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Rut", renderer,
						     "text", 1,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (widget), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_min_width (column, 80);
  gtk_tree_view_column_set_max_width (column, 80);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Nombre", renderer,
						     "text", 2,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (widget), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_min_width (column, 100);
  gtk_tree_view_column_set_max_width (column, 100);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Apellido P.", renderer,
						     "text", 3,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (widget), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_min_width (column, 100);
  gtk_tree_view_column_set_max_width (column, 100);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Apellido M.", renderer,
						     "text", 4,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (widget), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_min_width (column, 100);
  gtk_tree_view_column_set_max_width (column, 100);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Ingreso", renderer,
						     "text", 5,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (widget), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_min_width (column, 100);
  gtk_tree_view_column_set_max_width (column, 100);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Salida", renderer,
						     "text", 6,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (widget), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_min_width (column, 100);
  gtk_tree_view_column_set_max_width (column, 100);

  FillUsers ();
}

gint
ChangePasswd (GtkWidget *widget, gpointer data)
{
  gchar *passwd_new1 = g_strdup (gtk_entry_get_text (GTK_ENTRY (new_pass_1)));
  gchar *passwd_new2 = g_strdup (gtk_entry_get_text (GTK_ENTRY (new_pass_2)));
  GtkWidget *combo = (GtkWidget *) data;

  GtkTreeModel *model;
  GtkTreeIter iter;
  gchar *usuario;

  if (gtk_combo_box_get_active (GTK_COMBO_BOX (combo)) != -1)
    {
      model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));
      gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combo), &iter);

      gtk_tree_model_get (model, &iter,
			  0, &usuario,
			  -1);
    }
  else
    {
      AlertMSG (combo, "Debe seleccionar un usuario");
      return -1;
    }

  if (strcmp (passwd_new1, passwd_new2) != 0)
    {
      AlertMSG (new_pass_2, "La Password nueva no coincide");
      return -1;
    }

  if (SaveNewPassword (passwd_new1, usuario) == TRUE)
    {
      gtk_entry_set_text (GTK_ENTRY (old_pass), "");
      gtk_entry_set_text (GTK_ENTRY (new_pass_1), "");
      gtk_entry_set_text (GTK_ENTRY (new_pass_2), "");
      ExitoMSG (main_window, "Su Password fue cambiada con exito");
    }
  else
    {
      gtk_entry_set_text (GTK_ENTRY (old_pass), "");
      gtk_entry_set_text (GTK_ENTRY (new_pass_1), "");
      gtk_entry_set_text (GTK_ENTRY (new_pass_2), "");
      ErrorMSG (main_window, "No se pudo cambiar su password");
    }

  return 0;
}

gint
AddSeller (void)
{
  gchar *rut_seller = g_strdup (gtk_entry_get_text (GTK_ENTRY (rut_1)));
  gchar *check_seller = g_strdup (gtk_entry_get_text (GTK_ENTRY (rut_check)));
  gchar *nombre_seller = g_strdup (gtk_entry_get_text (GTK_ENTRY (nombre)));
  gchar *p_seller = g_strdup (gtk_entry_get_text (GTK_ENTRY (apell_p)));
  gchar *m_seller = g_strdup (gtk_entry_get_text (GTK_ENTRY (apell_m)));
  gchar *user_name = g_strdup (gtk_entry_get_text (GTK_ENTRY (username)));
  gchar *passwd_new1 = g_strdup (gtk_entry_get_text (GTK_ENTRY (new_pass1)));
  gchar *passwd_new2 = g_strdup (gtk_entry_get_text (GTK_ENTRY (new_pass2)));
  gchar *id = g_strdup (gtk_entry_get_text (GTK_ENTRY (id_box)));

  if (VerificarRut (rut_seller, check_seller) == FALSE)
    {
      AlertMSG (rut_1, "El Rut no es valido");
      return -1;
    }
  if (strcmp (nombre_seller, "") == 0)
    {
      AlertMSG (nombre, "Debe Ingresar un nombre al vendedor");
      return -1;
    }
  else if (strcmp (user_name, "") == 0)
    {
      AlertMSG (username, "Debe Ingresar un nombre de usuario");
      return -1;
    }
  else if (strcmp (passwd_new1, "") == 0 || strcmp (passwd_new2, "") == 0)
    {
      AlertMSG (new_pass1, "El vendedor debe tener password");
      return -1;
    }

  if (strcmp (passwd_new1, passwd_new2) != 0)
    {
      AlertMSG (new_pass1, "La Password nueva no coincide");
      return -1;
    }

  if (ReturnUserExist (user_name) == TRUE)
    {
      AlertMSG (username, "Ya existe el nombre de usuario");

      gtk_entry_set_text (GTK_ENTRY (username), "");

      return -1;
    }
  if (g_ascii_isdigit (id[0]) ==  FALSE && strcmp (id, "") != 0)
    {
      rizoma_errors_set ("El indentificador debe ser un numero", "AddSeller()", ALERT);

      rizoma_error_window (id_box);

      return -1;
    }

 if (AddNewSeller (rut_seller, nombre_seller, p_seller, m_seller, user_name, passwd_new1, id) == TRUE)
   {
     //ExitoMSG (rut_1, "Usuario agregado con exito");
     rizoma_error_window (rut_1);

     gtk_entry_set_text (GTK_ENTRY (rut_1), "");
     gtk_entry_set_text (GTK_ENTRY (rut_check), "");
     gtk_entry_set_text (GTK_ENTRY (nombre), "");
     gtk_entry_set_text (GTK_ENTRY (apell_p), "");
     gtk_entry_set_text (GTK_ENTRY (apell_m), "");
     gtk_entry_set_text (GTK_ENTRY (username), "");
     gtk_entry_set_text (GTK_ENTRY (new_pass1), "");
     gtk_entry_set_text (GTK_ENTRY (new_pass2), "");
     gtk_entry_set_text (GTK_ENTRY (id_box), "");

     FillUsers ();
   }
 else
   {
     // ErrorMSG (rut_1, "El Usuario no pudo ser agregado");
     rizoma_error_window (rut_1);

     gtk_entry_set_text (GTK_ENTRY (rut_1), "");
     gtk_entry_set_text (GTK_ENTRY (rut_check), "");
     gtk_entry_set_text (GTK_ENTRY (nombre), "");
     gtk_entry_set_text (GTK_ENTRY (apell_p), "");
     gtk_entry_set_text (GTK_ENTRY (apell_m), "");
     gtk_entry_set_text (GTK_ENTRY (username), "");
     gtk_entry_set_text (GTK_ENTRY (new_pass1), "");
     gtk_entry_set_text (GTK_ENTRY (new_pass2), "");
     gtk_entry_set_text (GTK_ENTRY (id_box), "");
   }

  return 0;
}
