/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4;
   c-indentation-style: gnu -*- */
/*datos_negocio.c
 *
 *    Copyright (C) 2004,2008 Rizoma Tecnologia Limitada <info@rizoma.cl>
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
#include<stdlib.h>

#include"tipos.h"
#include"postgres-functions.h"
#include"errors.h"
#include"datos_negocio.h"
#include"utils.h"

gchar *razon_social_value = NULL;
gchar *rut_value = NULL;
gchar *nombre_fantasia_value = NULL;
gchar *fono_value = NULL;
gchar *direccion_value = NULL;
gchar *comuna_value = NULL;
gchar *ciudad_value = NULL;
gchar *fax_value = NULL;
gchar *giro_value = NULL;
gchar *at_value = NULL;

void
refresh_labels (void)
{
  GtkWidget *widget;

  gchar **rut;

  /* We get all the new data */
  get_datos ();

  /* We refresh all labesl */

  //Razon
  if (razon_social_value != NULL)
    {
      widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_admin_razon"));
      gtk_entry_set_text (GTK_ENTRY (widget), razon_social_value);
    }

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_admin_razon"));
  gtk_label_set_markup (GTK_LABEL (widget),
                        razon_social_value !=NULL ?
                        "*RazÃ³n Social:":
                        "<span color=\"red\">*Razón Social:</span>");

  //rut
  if (rut_value != NULL)
    {
      widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_admin_rut"));
      rut = parse_rut (rut_value);
      gtk_entry_set_text (GTK_ENTRY (widget), g_strdup_printf ("%s-%s", rut[0], rut[1]));
    }

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_admin_rut"));
  gtk_label_set_markup (GTK_LABEL (widget),
                        rut_value != NULL ?
                        "*Rut:":
                        "<span color=\"red\">*Rut:</span>");

  //nombre de fantasia
  if (nombre_fantasia_value != NULL)
    {
      widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_admin_fantasy"));
      gtk_entry_set_text (GTK_ENTRY (widget), nombre_fantasia_value);
    }
  widget = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_admin_fantasy"));
  gtk_label_set_markup (GTK_LABEL (widget),
                        nombre_fantasia_value != NULL ?
                        "Nombre de Fantasia:":
                        "<span color=\"red\">Nombre de Fantasia</span>");

  //direccion
  if (direccion_value != NULL)
    {
      widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_admin_addr"));
      gtk_entry_set_text (GTK_ENTRY (widget), direccion_value);
    }
  widget = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_admin_addr"));
  gtk_label_set_markup (GTK_LABEL (widget),
                        direccion_value != NULL ?
                        "*Direccion:":
                        "<span color=\"red\">*Dirección</span>");

  //comuna
  if (comuna_value != NULL)
    {
      widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_admin_comuna"));
      gtk_entry_set_text (GTK_ENTRY (widget), comuna_value);
    }
  widget = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_admin_comuna"));
  gtk_label_set_markup (GTK_LABEL (widget),
                        comuna_value != NULL ?
                        "*Comuna:":
                        "<span color=\"red\">*Comuna</span>");

  //ciudad
  if (ciudad_value != NULL)
    {
      widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_admin_city"));
      gtk_entry_set_text (GTK_ENTRY (widget), ciudad_value);
    }
  widget = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_admin_city"));
  gtk_label_set_markup (GTK_LABEL (widget),
                        ciudad_value != NULL ?
                        "*Ciudad:":
                        "<span color=\"red\">*Ciudad</span>");


  //fono
  if (fono_value != NULL)
    {
      widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_admin_phone"));
      gtk_entry_set_text (GTK_ENTRY (widget), fono_value);
    }
  widget = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_admin_phone"));
  gtk_label_set_markup (GTK_LABEL (widget),
                        fono_value != NULL ?
                        "*Fono":
                        "<span color=\"red\">*Fono</span>");

  //fax
  if (fax_value != NULL)
    {
      widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_admin_fax"));
      gtk_entry_set_text (GTK_ENTRY (widget), fax_value);
    }
  widget = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_admin_fax"));
  gtk_label_set_markup (GTK_LABEL (widget),
                        fax_value != NULL ?
                        "Fax:":
                        "<span color=\"red\">Fax</span>");

  //Giro
  if (giro_value != NULL)
    {
      widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_admin_giro"));
      gtk_entry_set_text (GTK_ENTRY (widget), giro_value);
    }
  widget = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_admin_giro"));
  gtk_label_set_markup (GTK_LABEL (widget),
                        giro_value != NULL ?
                        "*Giro:":
                        "<span color=\"red\">*Giro</span>");

  //AT
  if (at_value != NULL)
    {
      widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_admin_at"));
      gtk_entry_set_text (GTK_ENTRY (widget), at_value);
    }
  widget = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_admin_at"));
  gtk_label_set_markup (GTK_LABEL (widget),
                        at_value != NULL ?
                        "A.T.:":
                        "<span color=\"red\">A.T.</span>");
}

void
SaveDatosNegocio (GtkWidget *widget, gpointer data)
{
  PGresult *res, *res2;
  GtkWidget *aux_widget;
  gchar **rut;

  aux_widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_admin_razon"));
  razon_social_value = g_strdup (gtk_entry_get_text (GTK_ENTRY (aux_widget)));

  aux_widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_admin_rut"));
  rut_value = g_strdup (gtk_entry_get_text (GTK_ENTRY (aux_widget)));

  rut = parse_rut (rut_value);

  aux_widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_admin_fantasy"));
  nombre_fantasia_value = g_strdup (gtk_entry_get_text (GTK_ENTRY (aux_widget)));

  aux_widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_admin_phone"));
  fono_value = g_strdup (gtk_entry_get_text (GTK_ENTRY (aux_widget)));

  aux_widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_admin_addr"));
  direccion_value = g_strdup (gtk_entry_get_text (GTK_ENTRY (aux_widget)));

  aux_widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_admin_comuna"));
  comuna_value = g_strdup (gtk_entry_get_text (GTK_ENTRY (aux_widget)));

  aux_widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_admin_city"));
  ciudad_value = g_strdup (gtk_entry_get_text (GTK_ENTRY (aux_widget)));

  aux_widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_admin_fax"));
  fax_value = g_strdup (gtk_entry_get_text (GTK_ENTRY (aux_widget)));

  aux_widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_admin_giro"));
  giro_value = g_strdup (gtk_entry_get_text (GTK_ENTRY (aux_widget)));

  aux_widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_admin_at"));
  at_value = g_strdup (gtk_entry_get_text (GTK_ENTRY (aux_widget)));

  res = EjecutarSQL ("SELECT count(*) FROM negocio");

  if (g_str_equal(PQgetvalue (res, 0, 0), "0"))
    {
      res = EjecutarSQL
        (g_strdup_printf
         ("INSERT INTO negocio (razon_social, rut, dv, nombre, fono, fax, direccion, comuna, ciudad, giro, at) "
          "VALUES ('%s', '%d', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s')", razon_social_value, atoi (rut[0]), rut[1], nombre_fantasia_value,
          fono_value, fax_value, direccion_value, comuna_value, ciudad_value, giro_value, at_value));
    }

  else
    {
      res = EjecutarSQL
        (g_strdup_printf
         ("UPDATE negocio SET razon_social='%s', rut='%d', dv='%s', nombre='%s', fono='%s', fax='%s', "
          "direccion='%s', comuna='%s', ciudad='%s', giro='%s', at='%s'", razon_social_value, atoi (rut[0]), rut[1], nombre_fantasia_value,
          fono_value, fax_value, direccion_value, comuna_value, ciudad_value, giro_value, at_value));

      res2 = EjecutarSQL(g_strdup_printf ("UPDATE bodega SET nombre='%s' where id=1", nombre_fantasia_value));
    }

  if (res != NULL)
    {
      aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "statusbar"));
      statusbar_push (GTK_STATUSBAR(aux_widget), "Los datos del negocio se actualizaron exitosamente", 3000);
    }
  else
    AlertMSG (aux_widget, "Se produjo un error al intentar actualizar los datos!");

  refresh_labels ();
}

int
get_datos (void)
{
  PGresult *res;

  res = EjecutarSQL ("SELECT razon_social, rut, dv, nombre, fono, fax, direccion, comuna, ciudad, giro, at "
                     "FROM negocio");

  if (PQntuples (res) != 1)
    return -1;

  if (strcmp (PQvaluebycol (res, 0, "razon_social"), "") != 0)
    razon_social_value = PQvaluebycol (res, 0, "razon_social");
  else
    razon_social_value = NULL;

  if (strcmp (PQvaluebycol (res, 0, "rut"), "") != 0)
    rut_value = g_strdup_printf ("%s-%s", PQvaluebycol (res, 0, "rut"), PQvaluebycol (res, 0, "dv"));
  else
    rut_value = NULL;

  if (strcmp (PQvaluebycol (res, 0, "nombre"), "") != 0)
    nombre_fantasia_value = PQvaluebycol (res, 0, "nombre");
  else
    nombre_fantasia_value = NULL;

  if (strcmp (PQvaluebycol (res, 0, "fono"), "") != 0)
    fono_value = PQvaluebycol (res, 0, "fono");
  else
    fono_value = NULL;

  if (strcmp (PQvaluebycol (res, 0, "fax"), "") != 0)
    fax_value = PQvaluebycol (res, 0, "fax");
  else
    fax_value = NULL;

  if (strcmp (PQvaluebycol (res, 0, "direccion"), "") != 0)
    direccion_value = PQvaluebycol (res, 0, "direccion");
  else
    direccion_value = NULL;

  if (strcmp (PQvaluebycol (res, 0, "comuna"), "") != 0)
    comuna_value = PQvaluebycol (res, 0, "comuna");
  else
    comuna_value = NULL;

  if (strcmp (PQvaluebycol (res, 0, "ciudad"), "") != 0)
    ciudad_value = PQvaluebycol (res, 0, "ciudad");
  else
    ciudad_value = NULL;

  if (strcmp (PQvaluebycol (res, 0, "giro"), "") != 0)
    giro_value = PQvaluebycol (res, 0, "giro");
  else
    giro_value = NULL;

  if (strcmp (PQvaluebycol (res, 0, "at"), "") != 0)
    at_value = PQvaluebycol (res, 0, "at");
  else
    at_value = NULL;

  return 1;
}

void
datos_box ()
{
  refresh_labels();
}

/**
 * Is called by btn_save_stores (clicked signal)
 *
 * Save the modified store names
 */
void
on_btn_save_stores_clicked()
{
  GtkTreeView *treeview = GTK_TREE_VIEW (builder_get (builder, "treeview_stores"));
  GtkTreeModel *model = gtk_tree_view_get_model (treeview);
  GtkTreeIter iter;
  gboolean valid;
  gchar *id, *name, *q;
  PGresult *res;

  valid = gtk_tree_model_get_iter_first (model, &iter);
  while (valid)
    {
      // getting the treeview values  --
      gtk_tree_model_get (model, &iter,
			  0, &id,
			  1, &name,
                          -1);
      
      q = g_strdup_printf ("UPDATE bodega SET nombre=upper('%s') WHERE id=%s", name, id);
      res = EjecutarSQL (q);
      g_free (q);
      
      // Iterates to the next row --
      valid = gtk_tree_model_iter_next (model, &iter);
    }
  
// Disabling save stores button --
gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "btn_save_stores")), FALSE);
}

/**
 * Is called when "nombre" cell has been edited (signal edited).
 * 
 * Update "nombre" cell with the new text
 *
 */
void 
on_name_cell_renderer_edited (GtkCellRendererText *cell, gchar *path_string, gchar *new_name, gpointer data)
{
  GtkTreeModel *model = GTK_TREE_MODEL (data);
  GtkTreePath *path = gtk_tree_path_new_from_string (path_string);
  GtkTreeIter iter;
  gchar *current_name;
  GtkWidget *aux_widget = GTK_WIDGET (gtk_builder_get_object(builder, "treeview_stores"));

  gtk_tree_model_get_iter (model, &iter, path);
  gtk_tree_path_free (path);

  gtk_tree_model_get (model, &iter,
                      1, &current_name,
                      -1);

  new_name = g_strstrip (new_name);
  if (g_str_equal(new_name, ""))
    {
      ErrorMSG(aux_widget, "Debe ingresar un nombre");
      return;
    }
  
  if (g_str_equal(new_name, current_name))
    return;

  gchar *q;
  PGresult *res;
  q = g_strdup_printf ("SELECT nombre FROM bodega WHERE nombre=upper('%s') AND estado = true", new_name);
  res = EjecutarSQL (q);
  g_free (q);
  
  gint tuples = PQntuples (res);
  if (tuples > 0)
    {      
      ErrorMSG(aux_widget, g_strdup_printf ("El nombre %s ya se encuantra asignado, use un nombre distinto", new_name));
      return;
    }

  if (!(g_str_equal (new_name, current_name)))
    {
      gtk_list_store_set (GTK_LIST_STORE (model), &iter,
			  1, new_name,
			  -1);

      // The save button is enabled
      gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "btn_save_stores")), TRUE);
    }
}


/**
 * Contruye el "treeview_stores" de la pestaña "Locales"
 * en rizoma-admin
 */
void
stores_box ()
{
  GtkListStore *store;
  GtkTreeView *treeview;
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;
  GtkTreeSelection *selection;

  store = gtk_list_store_new (2,
                              G_TYPE_STRING,  //ID
			      G_TYPE_STRING); //Nombre local

  treeview = GTK_TREE_VIEW (builder_get (builder, "treeview_stores"));
  gtk_tree_view_set_model (treeview, GTK_TREE_MODEL (store));

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("ID", renderer,
                                                     "text", 0,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 2);
  gtk_tree_view_column_set_min_width (column, 60);
  gtk_tree_view_column_set_resizable (column, FALSE);


  renderer = gtk_cell_renderer_text_new ();
  g_object_set (renderer,
		"editable", TRUE,
		NULL);
  g_signal_connect (G_OBJECT (renderer), "edited",
		    G_CALLBACK (on_name_cell_renderer_edited), (gpointer)store);
  column = gtk_tree_view_column_new_with_attributes ("Nombre", renderer,
                                                     "text", 1,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 0);
  gtk_tree_view_column_set_min_width (column, 160);
  gtk_tree_view_column_set_resizable (column, FALSE);

  gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "btn_save_stores")), FALSE);
}

/**
 * Callback connected to the search button (btn_srch_stores)
 * and entry search stores (entry_srch_stores)
 *
 * @param button the button that emits the signal
 * @param user_data the user data
 */
void
on_btn_srch_stores_clicked (GtkButton *button, gpointer user_data)
{
  GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (gtk_builder_get_object (builder, "treeview_stores"))));
  GtkTreeIter iter;
  gint i, tuples;
  PGresult *res;
  char *sql, *store_name;
  GtkWidget *aux_widget;

  gtk_list_store_clear (store);

  aux_widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_srch_stores"));
  store_name = g_strdup (gtk_entry_get_text (GTK_ENTRY (aux_widget)));

  if ( g_str_equal(store_name, "") )
    sql = g_strdup_printf ("SELECT id, nombre FROM bodega where estado = true "
			   "AND nombre!=(SELECT nombre FROM negocio)");
  else
    sql = g_strdup_printf ("SELECT id, nombre FROM bodega where nombre LIKE upper('%s%%') "
			   "AND estado = true "
			   "AND nombre!=(SELECT nombre FROM negocio)", store_name);

  res = EjecutarSQL (sql);
  tuples = PQntuples (res);
  g_free (sql);

  if (res == NULL) return;

  for (i = 0; i < tuples; i++)
    {
      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
			  0, PQvaluebycol (res, i, "id"),
			  1, PQvaluebycol (res, i, "nombre"),
			  -1);
    }
}


/**
 * Callback connected to the save button (btn_save_new_store)
 *
 * @param button the button that emits the signal
 * @param user_data the user data
 */
void
on_btn_save_new_store_clicked (GtkButton *button, gpointer user_data)
{
  gchar *q, *store_name;
  GtkWidget *aux_widget;
  PGresult *res;
  
  aux_widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_name_new_store"));
  store_name = g_strdup (gtk_entry_get_text (GTK_ENTRY (aux_widget)));
  
  store_name = g_strstrip (store_name);
  if (g_str_equal(store_name, ""))
    {
      ErrorMSG(aux_widget, "Debe ingresar un nombre");
      gtk_entry_set_text (GTK_ENTRY (aux_widget), "");
      return;
    }

  q = g_strdup_printf ("SELECT nombre FROM bodega WHERE nombre=upper('%s') AND estado = true", store_name);
  res = EjecutarSQL (q);
  g_free (q);
  
  gint tuples = PQntuples (res);
  if (tuples > 0)
    {      
      ErrorMSG(aux_widget, g_strdup_printf("El nombre %s esta asignado, use un nombre distinto", store_name));
      return;
    }

  q = g_strdup_printf ("INSERT INTO bodega (nombre) values(upper('%s'))", store_name);
  res = EjecutarSQL (q);
  g_free (q);

  aux_widget = GTK_WIDGET (gtk_builder_get_object(builder, "wnd_new_store"));
  gtk_widget_hide (aux_widget);
}

/**
 * Callback connected to the add button (btn_add_stores)
 *
 * @param button the button that emits the signal
 * @param user_data the user data
 */
void
on_btn_add_stores_clicked (GtkButton *button, gpointer user_data)
{
  GtkWindow *window;

  window = GTK_WINDOW (gtk_builder_get_object (builder, "wnd_new_store"));
  clean_container (GTK_CONTAINER (window));
  gtk_widget_show (window);
}


/**
 * Callback connected to the delete store button in rizoma-admin
 */
void
ask_delete_store (void)
{
  GtkWidget *window;
  GtkWidget *tree;
  GtkListStore *store;
  GtkTreeSelection *selection;
  GtkTreeIter iter;

  tree = GTK_WIDGET(gtk_builder_get_object(builder, "treeview_stores"));
  store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(tree)));
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));

  if (gtk_tree_selection_get_selected (selection, NULL, &iter) == TRUE)
    {
      window = GTK_WIDGET(gtk_builder_get_object(builder, "ask_delete_store"));
      gtk_widget_show_all(window);
    }
  else
    AlertMSG (tree, "Debe seleccionar un local de la lista");
}

/**
 * Callback to delete store
 */
void
on_ask_delete_store_response (GtkDialog *dialog,
			      gint       response_id,
			      gpointer   user_data)
{
  if (response_id == GTK_RESPONSE_YES)
    {
      GtkTreeView *tree = GTK_TREE_VIEW (builder_get (builder, "treeview_stores"));
      GtkTreeModel *model = gtk_tree_view_get_model (tree);
      GtkTreeSelection *selection = gtk_tree_view_get_selection (tree);
      GtkTreeIter iter;

      gchar *q, *id_bodega;
      PGresult *res;

      if (gtk_tree_selection_get_selected (selection, NULL, &iter) == TRUE)
	{
	  gtk_tree_model_get (model, &iter,
			      0, &id_bodega,
			      -1);

	  q = g_strdup_printf ("UPDATE bodega SET estado = false WHERE id = %s", id_bodega);
	  res = EjecutarSQL (q);
	  g_free (q);
	}
      
      on_btn_srch_stores_clicked(NULL,NULL);
    }
  gtk_widget_hide(GTK_WIDGET(dialog));
}
