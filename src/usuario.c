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

#include"utils.h"

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

	      if (!(g_str_equal(PQvaluebycol (res2, 0, "salida_year"), "-1")) && !(g_str_equal(PQvaluebycol (res2, 0, "salida_year"), "")))
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
  GtkWidget *tree;
  GtkListStore *store;
  GtkTreeSelection *selection;
  GtkTreeIter iter;
  gchar *id;
  PGresult *res;

  tree = GTK_WIDGET(gtk_builder_get_object(builder, "treeview_users"));
  store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(tree)));
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));

  if (gtk_tree_selection_get_selected (selection, NULL, &iter))
    {
      gtk_tree_model_get (GTK_TREE_MODEL (store), &iter,
			  0, &id,
			  -1);

      if (strcmp (id, "1") != 0)
	{
	  res = EjecutarSQL (g_strdup_printf ("select * from delete_user(%s)", id));
	  FillUsers ();
	}
      else
	AlertMSG (tree, "No se puede elminar el usuario admin");
    }
}

void
on_ask_delete_user_response (GtkDialog *dialog,
			     gint       response_id,
			     gpointer   user_data)
{
  if (response_id == GTK_RESPONSE_YES)
    DeleteUser (NULL, (gpointer)TRUE);

  gtk_widget_hide(GTK_WIDGET(dialog));
}

/**
 * Callback connected to the delete user button in rizoma-admin
 *
 */
void
AskDelete (void)
{
  GtkWidget *window;
  GtkWidget *tree;
  GtkListStore *store;
  GtkTreeSelection *selection;
  GtkTreeIter iter;

  tree = GTK_WIDGET(gtk_builder_get_object(builder, "treeview_users"));
  store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(tree)));
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));

  if (gtk_tree_selection_get_selected (selection, NULL, &iter) == TRUE)
    {
      window = GTK_WIDGET(gtk_builder_get_object(builder, "ask_delete_user"));

      gtk_widget_show_all(window);

    }
  else
    AlertMSG (tree, "Debe haber seleccionado un usuario de la lista previamente");
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
				    G_TYPE_STRING, //apellido Paterno
				    G_TYPE_STRING, //apellido materno
				    G_TYPE_STRING, //ingreso
				    G_TYPE_STRING);//salida

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

void
CloseChangePasswdWin (GtkButton *button, gpointer user_data)
{
  GtkWidget *widget;

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "wnd_changepasswd"));
  gtk_widget_hide(widget);

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_changepasswd_1"));
  gtk_entry_set_text(GTK_ENTRY(widget), "");

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_changepasswd_2"));
  gtk_entry_set_text(GTK_ENTRY(widget), "");

}

gint
ChangePasswd (GtkWidget *widget, gpointer data)
{
  GtkWidget *aux_widget;
  GtkWidget *tree;
  GtkListStore *store;
  GtkTreeSelection *selection;
  GtkTreeIter iter;
  gchar *user_id;
  gchar *passwd_new1;
  gchar *passwd_new2;

  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_changepasswd_1"));
  passwd_new1 = g_strdup (gtk_entry_get_text (GTK_ENTRY (aux_widget)));

  aux_widget  = GTK_WIDGET(gtk_builder_get_object(builder, "entry_changepasswd_2"));
  passwd_new2 = g_strdup (gtk_entry_get_text (GTK_ENTRY (aux_widget)));

  tree = GTK_WIDGET(gtk_builder_get_object(builder, "treeview_users"));
  store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(tree)));
  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));

  if (gtk_tree_selection_get_selected(selection, NULL, &iter))
    {
      gtk_tree_model_get (GTK_TREE_MODEL(store), &iter,
			  0, &user_id,
			  -1);
    }
  else
    {
      AlertMSG (aux_widget, "Debe seleccionar un usuario");
      return -1;
    }

  if (!(g_str_equal (passwd_new1, passwd_new2)))
    {
      AlertMSG (aux_widget, "La Password nueva no coincide");
      return -1;
    }

  if (SaveNewPassword (passwd_new1, user_id) == TRUE)
    {
      CloseChangePasswdWin(NULL, NULL);

      aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "statusbar"));
      statusbar_push (GTK_STATUSBAR(aux_widget), "Su Password fue cambiada con exito", 3000);
    }
  else
    {
      CloseChangePasswdWin(NULL, NULL);

      aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "statusbar"));
      ErrorMSG (aux_widget, "No se pudo cambiar la password");
    }

  return 0;
}

void
ChangePasswdWin (GtkButton *button, gpointer user_data)
{
  GtkWidget *widget;
  GtkWidget *tree;
  GtkTreeSelection *selection;
  GtkListStore *store;
  GtkTreeIter iter;
  gchar *user_id;
  PGresult *res;
  gchar *q;

  tree = GTK_WIDGET(gtk_builder_get_object(builder, "treeview_users"));
  store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(tree)));
  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));

  if (gtk_tree_selection_get_selected(selection, NULL, &iter))
    {
      gtk_tree_model_get (GTK_TREE_MODEL(store), &iter,
			  0, &user_id,
			  -1);

      q = g_strdup_printf("select usuario from select_usuario() as "
			  "(id int,rut int4, dv varchar(1),usuario varchar(30),"
			  "passwd varchar(400),nombre varchar(75),"
			  "apell_p varchar(75),apell_m varchar(75),"
			  "fecha_ingreso timestamp,\"level\" int2)"
			  " WHERE id = %s", user_id);
      res = EjecutarSQL(q);
      g_free(q);

      if ((res == NULL) || (PQntuples(res) == 0))
	{
	  g_printerr("%s: no se pudo obtener el nombre de usuario para el id=%s",
		     G_STRFUNC, user_id);
	  return;
	}

      widget = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_changepasswd_username"));
      gtk_label_set_text(GTK_LABEL(widget), PQvaluebycol(res, 0, "usuario"));

      widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_changepasswd_1"));
      gtk_widget_grab_focus(widget);

      widget = GTK_WIDGET(gtk_builder_get_object(builder, "wnd_changepasswd"));
      gtk_widget_show_all(widget);
    }
  else
    AlertMSG(tree, "Debe seleccionar un usuario de la lista para cambiar la contraseña");

}

////////////// Add Seller

gint
AddSeller (void)
{
  GtkWidget *widget;
  gchar *rut_seller;
  gchar *check_seller;
  gchar *nombre_seller;
  gchar *p_seller;
  gchar *m_seller;
  gchar *user_name;
  gchar *passwd_new1;
  gchar *passwd_new2;
  gchar *id;
  gchar *rut_full;

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_adduser_rut"));
  rut_seller = g_strdup (gtk_entry_get_text (GTK_ENTRY (widget)));

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_adduser_dv"));
  check_seller  = g_strdup (gtk_entry_get_text (GTK_ENTRY (widget)));

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_adduser_name"));
  nombre_seller = g_strdup (gtk_entry_get_text (GTK_ENTRY (widget)));

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_adduser_apell_p"));
  p_seller = g_strdup (gtk_entry_get_text (GTK_ENTRY (widget)));

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_adduser_apell_m"));
  m_seller = g_strdup (gtk_entry_get_text (GTK_ENTRY (widget)));

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_adduser_username"));
  user_name = g_strdup (gtk_entry_get_text (GTK_ENTRY (widget)));

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_adduser_passwd"));
  passwd_new1 = g_strdup (gtk_entry_get_text (GTK_ENTRY (widget)));

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_adduser_passwd2"));
  passwd_new2 = g_strdup (gtk_entry_get_text (GTK_ENTRY (widget)));

  id = ""; //use the default id, assigned by the Database

  if (VerificarRut (rut_seller, check_seller) == FALSE)
    {
      widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_adduser_rut"));
      gtk_editable_select_region(GTK_EDITABLE(widget), 0, -1);

      AlertMSG (widget, "El Rut no es valido");

      return -1;
    }

  if (strcmp (nombre_seller, "") == 0)
    {
      widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_adduser_name"));
      gtk_editable_select_region(GTK_EDITABLE(widget), 0, -1);

      AlertMSG (widget, "Debe Ingresar un nombre al vendedor");
      return -1;
    }
  else if (strcmp (user_name, "") == 0)
    {
      widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_adduser_username"));
      gtk_editable_select_region(GTK_EDITABLE(widget), 0, -1);

      AlertMSG (widget, "Debe Ingresar un nombre de usuario");
      return -1;
    }
  else if (strcmp (passwd_new1, "") == 0 || strcmp (passwd_new2, "") == 0)
    {
      widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_adduser_passwd"));
      gtk_editable_select_region(GTK_EDITABLE(widget), 0, -1);

      AlertMSG (widget, "El vendedor debe tener password");
      return -1;
    }

  if (strcmp (passwd_new1, passwd_new2) != 0)
    {
      widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_adduser_passwd"));
      gtk_editable_select_region(GTK_EDITABLE(widget), 0, -1);

      AlertMSG (widget, "La Password nueva no coincide");
      return -1;
    }

  if (ReturnUserExist (user_name) == TRUE)
    {
      widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_adduser_username"));
      gtk_editable_select_region(GTK_EDITABLE(widget), 0, -1);

      AlertMSG (widget, "Ya existe el nombre de usuario");

      return -1;
    }
  if (g_ascii_isdigit (id[0]) ==  FALSE && strcmp (id, "") != 0)
    {
      rizoma_errors_set ("El indentificador debe ser un numero", "AddSeller()", ALERT);

      rizoma_error_window (id_box);

      return -1;
    }

  rut_full = g_strdup_printf("%s-%s", rut_seller, check_seller);
  if (AddNewSeller (rut_full, nombre_seller, p_seller, m_seller, user_name, passwd_new1, id))
    {
      CloseAddSellerWin(NULL, NULL);
      FillUsers ();
    }
  else
    {
      ErrorMSG (rut_1, "El Usuario no pudo ser agregado");
      rizoma_error_window (widget);
    }
  g_free (rut_full);

  return 0;
}

void
CloseAddSellerWin (GtkButton *button, gpointer user_data)
{
  GtkWidget *widget;
  int i;

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "wnd_adduser"));
  gtk_widget_hide (widget);

  gchar *entries[8] = {"entry_adduser_rut",
		       "entry_adduser_dv",
		       "entry_adduser_username",
		       "entry_adduser_name",
		       "entry_adduser_apell_p",
		       "entry_adduser_apell_m",
		       "entry_adduser_passwd",
		       "entry_adduser_passwd2"};

  for (i=0 ; i < 8 ; i++)
    {
      widget = GTK_WIDGET(gtk_builder_get_object(builder, entries[i]));
      g_assert(widget != NULL);
      gtk_entry_set_text(GTK_ENTRY(widget), "");
    }
}

void
AddSellerWin (GtkButton *button, gpointer user_data)
{
  GtkWidget *widget;

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "wnd_adduser"));
  gtk_widget_show_all(widget);
}
