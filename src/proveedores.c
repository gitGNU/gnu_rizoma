/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4;
       c-indentation-style: gnu -*- */
/*proveedores.c
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

#include"postgres-functions.h"
#include"printing.h"
#include"dimentions.h"
#include"compras.h"
#include"credito.h"
#include"errors.h"

GtkWidget *rut;
GtkWidget *razon;
GtkWidget *direccion;
GtkWidget *comuna;
GtkWidget *ciudad;
GtkWidget *fono;
GtkWidget *fax;
GtkWidget *web;
GtkWidget *contacto;
GtkWidget *email;
GtkWidget *fono_directo;
GtkWidget *giro;

GtkWidget *compras_totales;
GtkWidget *contrib_total;
GtkWidget *contrib_proyect;
GtkWidget *inci_compras;
GtkWidget *stock_valorizado;
GtkWidget *merma_uni;
GtkWidget *merma_porc;
GtkWidget *ventas_totales;
GtkWidget *contrib_agreg;
GtkWidget *inci_ventas;
GtkWidget *total_pen_fact;
GtkWidget *indice_t;


GtkWidget *search_entry;

GtkTreeStore *proveedores_store;
GtkWidget *proveedores_tree;

void
BuscarProveedor (GtkWidget *widget, gpointer data)
{
  GtkWidget *aux_widget;
  GtkTreeIter iter;
  GtkListStore *store;
  PGresult *res;
  gint tuples, i;
  gchar *str_axu;
  gchar *q;
  gchar *string;

  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_prov_search"));
  string = g_strdup (gtk_entry_get_text (GTK_ENTRY (aux_widget)));

  q = g_strdup_printf ("SELECT rut, dv, nombre, giro, contacto "
		       "FROM buscar_proveedor('%%%s%%')",
		       string);
  res = EjecutarSQL (q);
  g_free (q);

  tuples = PQntuples (res);


  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "treeview_prov_search"));
  store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(aux_widget)));
  gtk_list_store_clear (store);

  for (i = 0; i < tuples; i++)
    {
      str_axu = g_strconcat(PQvaluebycol (res, i, "rut"),"-",
			    PQvaluebycol (res, i, "dv"), NULL);
      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
			  0, str_axu,
			  1, PQvaluebycol (res, i, "nombre"),
			  2, PQvaluebycol (res, i, "giro"),
			  3, PQvaluebycol (res, i, "contacto"),
			  -1);
      g_free (str_axu);
    }
}

void
ListarProveedores (void)
{
  PGresult *res;
  gint tuples, i;
  GtkTreeIter iter;
  gchar *str_axu;

  res = EjecutarSQL ("SELECT rut, dv, nombre, giro, contacto FROM buscar_proveedor('%') ORDER BY nombre ASC");

  tuples = PQntuples (res);

  gtk_tree_store_clear (GTK_TREE_STORE (proveedores_store));

  for (i = 0; i < tuples; i++)
    {
      str_axu = g_strconcat(PQvaluebycol(res, i, "rut"), "-", PQvaluebycol(res, i, "dv"), NULL);
      gtk_tree_store_append (GTK_TREE_STORE (proveedores_store), &iter, NULL);
      gtk_tree_store_set (GTK_TREE_STORE (proveedores_store), &iter,
			  0, str_axu,
			  1, PQvaluebycol (res, i, "nombre"),
			  2, PQvaluebycol (res, i, "giro"),
			  3, PQvaluebycol (res, i, "contacto"),
			  -1);
      g_free (str_axu);
    }
}

void
LlenarDatosProveedor (GtkTreeSelection *selection, gpointer data)
{
  PGresult *res;
  GtkTreeIter iter;
  gchar *rut_proveedor;
  gchar **aux, *q;

  if (gtk_tree_selection_get_selected (selection, NULL, &iter) == FALSE)
    return;

  gtk_tree_model_get (GTK_TREE_MODEL (proveedores_store), &iter,
		      0, &rut_proveedor,
		      -1);

  aux = g_strsplit(rut_proveedor, "-", 0);
  q = g_strdup_printf ("SELECT * FROM select_proveedor(%s)", aux[0]);
  res = EjecutarSQL (q);
  g_free (q);
  g_strfreev(aux);

  if (res == NULL || PQntuples (res) == 0)
    return;

  gtk_entry_set_text (GTK_ENTRY (razon), PQvaluebycol (res, 0, "nombre"));

  q = g_strconcat(PQvaluebycol (res, 0, "rut"), "-", PQvaluebycol(res, 0, "dv"), NULL);
  gtk_label_set_text (GTK_LABEL (rut), q);
  g_free (q);

  gtk_entry_set_text (GTK_ENTRY (direccion), PQvaluebycol (res, 0, "direccion"));

  gtk_entry_set_text (GTK_ENTRY (ciudad), PQvaluebycol (res, 0, "ciudad"));

  gtk_entry_set_text (GTK_ENTRY (comuna), PQvaluebycol (res, 0, "comuna"));

  gtk_entry_set_text (GTK_ENTRY (fono), PQvaluebycol (res, 0, "telefono"));

  gtk_entry_set_text (GTK_ENTRY (web), PQvaluebycol (res, 0, "web"));

  gtk_entry_set_text (GTK_ENTRY (contacto), PQvaluebycol (res, 0, "contacto"));

  gtk_entry_set_text (GTK_ENTRY (email), PQvaluebycol (res, 0, "email"));

  gtk_entry_set_text (GTK_ENTRY (giro), PQvaluebycol (res, 0, "giro"));
}

void
CloseAgregarProveedorWindow (GtkWidget *widget, gpointer user_data)
{
  GtkWidget *window;

  window = GTK_WIDGET(gtk_builder_get_object(builder, "wnd_addprovider"));

  gtk_window_set_transient_for(GTK_WINDOW(window), NULL);

  gtk_widget_hide(window);

}

void
AgregarProveedor (GtkWidget *widget, gpointer user_data)
{
  GtkWidget *aux_widget;
  GtkWidget *wnd;
  gchar *str_rut;
  gchar *rut_c;
  gchar *rut_ver;
  gchar *nombre_c;
  gchar *direccion_c;
  gchar *ciudad_c;
  gchar *comuna_c;
  gchar *telefono_c;
  gchar *email_c;
  gchar *web_c;
  gchar *contacto_c;
  gchar *giro_c;

  wnd = GTK_WIDGET(gtk_builder_get_object(builder, "wnd_addprovider"));

  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_prov_rut"));
  rut_c = g_strdup (gtk_entry_get_text (GTK_ENTRY (aux_widget)));

  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_prov_dv"));
  rut_ver = g_strdup (gtk_entry_get_text (GTK_ENTRY (aux_widget)));

  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_prov_name"));
  nombre_c = g_strdup (gtk_entry_get_text (GTK_ENTRY (aux_widget)));

  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_prov_addr"));
  direccion_c = g_strdup (gtk_entry_get_text (GTK_ENTRY (aux_widget)));

  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_prov_city"));
  ciudad_c = g_strdup (gtk_entry_get_text (GTK_ENTRY (aux_widget)));

  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_prov_comuna"));
  comuna_c = g_strdup (gtk_entry_get_text (GTK_ENTRY (aux_widget)));

  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_prov_phone"));
  telefono_c = g_strdup (gtk_entry_get_text (GTK_ENTRY (aux_widget)));

  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_prov_email"));
  email_c = g_strdup (gtk_entry_get_text (GTK_ENTRY (aux_widget)));

  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_prov_web"));
  web_c = g_strdup (gtk_entry_get_text (GTK_ENTRY (aux_widget)));

  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_prov_contact"));
  contacto_c = g_strdup (gtk_entry_get_text (GTK_ENTRY (aux_widget)));

  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_prov_giro"));
  giro_c = g_strdup (gtk_entry_get_text (GTK_ENTRY (aux_widget)));


  if (g_strcmp0 (rut_c, "") == 0)
    {
      ErrorMSG (wnd, "Debe Escribir el rut completo");
      return;
    }
  else if (g_strcmp0 (rut_ver, "") == 0)
    {
      ErrorMSG (wnd, "Debe ingresar el digito verificador del rut");
      return;
    }
  else if ((GetDataByOne
	    (g_strdup_printf ("SELECT * FROM proveedor WHERE rut=%s", rut_c))) != NULL)
    {
      ErrorMSG (wnd, "Ya existe un proveedor con el mismo rut");
      return;
    }
  else if (g_strcmp0 (nombre_c, "") == 0)
    {
      ErrorMSG (wnd, "Debe escribir el nombre del proveedor");
      return;
    }
  else if (g_strcmp0 (direccion_c, "") == 0)
    {
      ErrorMSG (wnd, "Debe escribir la direccion");
      return;
    }
  else if (g_strcmp0 (comuna_c, "") == 0)
    {
      ErrorMSG (wnd, "Debe escribir la comuna");
      return;
    }
  else if (g_strcmp0 (telefono_c, "") == 0)
    {
      ErrorMSG (wnd, "Debe escribir el telefono");
      return;
    }
  else if (g_strcmp0 (giro_c, "") == 0)
    {
      ErrorMSG (wnd, "Debe escribir el giro");
      return;
    }

  if (VerificarRut (rut_c, rut_ver) != TRUE)
    {
      ErrorMSG (wnd, "El rut no es valido!");
      return;
    }

  CloseAgregarProveedorWindow (NULL, user_data);

  str_rut = g_strdup_printf ("%s-%s", rut_c, rut_ver);
  AddProveedorToDB (str_rut, nombre_c, direccion_c, ciudad_c, comuna_c, telefono_c, email_c, web_c, contacto_c, giro_c);
  g_free (str_rut);
  //ListarProveedores (); <- this does not correspond

}

void
AgregarProveedorWindow (GtkWidget *widget, gpointer user_data)
{
  GtkWidget *aux_widget;

  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_prov_rut"));
  gtk_entry_set_text(GTK_ENTRY(aux_widget), "");

  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_prov_dv"));
  gtk_entry_set_text(GTK_ENTRY(aux_widget), "");

  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_prov_name"));
  gtk_entry_set_text(GTK_ENTRY(aux_widget), "");

  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_prov_addr"));
  gtk_entry_set_text(GTK_ENTRY(aux_widget), "");

  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_prov_comuna"));
  gtk_entry_set_text(GTK_ENTRY(aux_widget), "");

  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_prov_city"));
  gtk_entry_set_text(GTK_ENTRY(aux_widget), "");

  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_prov_phone"));
  gtk_entry_set_text(GTK_ENTRY(aux_widget), "");

  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_prov_email"));
  gtk_entry_set_text(GTK_ENTRY(aux_widget), "");

  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_prov_web"));
  gtk_entry_set_text(GTK_ENTRY(aux_widget), "");

  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_prov_contact"));
  gtk_entry_set_text(GTK_ENTRY(aux_widget), "");

  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_prov_giro"));
  gtk_entry_set_text(GTK_ENTRY(aux_widget), "");

  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "wnd_addprovider"));
  gtk_window_set_transient_for(GTK_WINDOW(aux_widget),
  			       GTK_WINDOW(gtk_widget_get_toplevel(widget)));
  gtk_window_set_modal (GTK_WINDOW(aux_widget), TRUE);
  gtk_widget_show (aux_widget);
}

void
ModificarProveedor (void)
{
  gchar *rut_c = (gchar *) gtk_label_get_text (GTK_LABEL (rut));
  gchar *razon_c = (gchar *) gtk_entry_get_text (GTK_ENTRY (razon));
  gchar *direccion_c = (gchar *) gtk_entry_get_text (GTK_ENTRY (direccion));
  gchar *comuna_c = (gchar *) gtk_entry_get_text (GTK_ENTRY (comuna));
  gchar *ciudad_c = (gchar *) gtk_entry_get_text (GTK_ENTRY (ciudad));
  gchar *fono_c = (gchar *) gtk_entry_get_text (GTK_ENTRY (fono));
  gchar *web_c = (gchar *) gtk_entry_get_text (GTK_ENTRY (web));
  gchar *contacto_c = (gchar *) gtk_entry_get_text (GTK_ENTRY (contacto));
  gchar *email_c = (gchar *) gtk_entry_get_text (GTK_ENTRY (email));
  gchar *giro_c = (gchar *) gtk_entry_get_text (GTK_ENTRY (giro));

  SetModificacionesProveedor (rut_c, razon_c, direccion_c, comuna_c, ciudad_c, fono_c,
                              web_c, contacto_c, email_c, giro_c);
}

void
proveedores_box ()
{
  GtkListStore *store;
  GtkWidget *button;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  Print *proveedores_print;

  proveedores_print = (Print *) g_malloc0 (sizeof (Print));

  //setup the gtktreeview and all the necesary objects
  store = gtk_list_store_new (4,
			      G_TYPE_STRING,
			      G_TYPE_STRING,
			      G_TYPE_STRING,
			      G_TYPE_STRING);

  proveedores_tree = GTK_WIDGET(gtk_builder_get_object(builder, "treeview_prov_search"));
  gtk_tree_view_set_model(GTK_TREE_VIEW(proveedores_tree), GTK_TREE_MODEL(store));

  /* g_signal_connect (G_OBJECT (gtk_tree_view_get_selection */
  /*                             (GTK_TREE_VIEW (proveedores_tree))), "changed", */
  /*                   G_CALLBACK (LlenarDatosProveedor), NULL); */

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Proveedor", renderer,
                                                     "text", 0,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (proveedores_tree), column);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Rut Proveedor", renderer,
                                                     "text", 1,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (proveedores_tree), column);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Giro", renderer,
                                                     "text", 2,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (proveedores_tree), column);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Contacto", renderer,
                                                     "text", 3,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (proveedores_tree), column);
  gtk_tree_view_column_set_resizable (column, FALSE);

  button = GTK_WIDGET(gtk_builder_get_object(builder, "btn_prov_print"));

  proveedores_print->tree = GTK_TREE_VIEW (proveedores_tree);
  proveedores_print->title = "Lista de Proveedores";
  proveedores_print->name = "proveedores";
  proveedores_print->date_string = NULL;
  proveedores_print->cols[0].name = "Nombre";
  proveedores_print->cols[0].num = 0;
  proveedores_print->cols[1].name = "Rut";
  proveedores_print->cols[1].num = 1;
  proveedores_print->cols[2].name = NULL;

  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (PrintTree), (gpointer)proveedores_print);
}
