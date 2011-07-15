/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4;
   c-indentation-style: gnu -*- */
/*proveedores.c
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

#include"tipos.h"

#include"postgres-functions.h"
#include"printing.h"

#include"compras.h"
#include"credito.h"
#include"errors.h"
#include"utils.h"

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

/**
 * Es llamada cuando el boton "btn_prov_search" es presionado (signal click).
 * 
 * Esta funcion llama a una consulta de sql que a su vez llama a la funcion
 * buscar_proveedor(), esta retorna al o a los proveedores, segun por los
 * parametros que se ingreso. Despues este o estos los agrega a la tree_view
 * correspondiente.
 *
 * @param widget the widget that emited the signal
 * @param data the user data
 *
 */

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

  /*consulta de sql que llama a la funcion que retorna a el o los
    proveedores, dependiendo de los datos de entrada*/
  q = g_strdup_printf ("SELECT rut, dv, nombre, giro, contacto, lapso_reposicion "
                       "FROM buscar_proveedor('%%%s%%')",
                       string);
  res = EjecutarSQL (q);
  g_free (q);

  tuples = PQntuples (res);


  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "treeview_prov_search"));
  store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(aux_widget)));
  gtk_list_store_clear (store);

  /* se agrega al proveedor al tree view y sus datos*/
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

/**
 * Es llamada por la funcion "AddProveedor()".
 * 
 * Esta funcion agrega al proveedor a la lista (tree view) de la busqueda de proveedores.
 *
 */

void
ListarProveedores (void)
{
  GtkWidget *treeview;
  GtkListStore *store;
  PGresult *res;
  gint tuples, i;
  GtkTreeIter iter;
  gchar *str_axu;

  res = EjecutarSQL ("SELECT rut, dv, nombre, giro, contacto FROM buscar_proveedor('%') ORDER BY nombre ASC");

  tuples = PQntuples (res);
  treeview = GTK_WIDGET(gtk_builder_get_object(builder, "treeview_prov_search"));
  store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(treeview)));

  gtk_list_store_clear (store);

  for (i = 0; i < tuples; i++)
    {
      str_axu = g_strconcat(PQvaluebycol(res, i, "rut"), "-", PQvaluebycol(res, i, "dv"), NULL);
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



/**
 * Es llamada cuando se selecciona un proveedor de la lista
 * "treeview_prov_search".
 * 
 * Esta funcionagrega la informacion del proveedor a los
 * respectivos campos de texto (entry)
 *
 * @param selection the row data
 * @param data the user data
 *
 */

void
LlenarDatosProveedor (GtkTreeSelection *selection,
                      gpointer           user_data)
{
  GtkWidget *widget;
  GtkTreeView *tree_view;
  GtkListStore *store;
  PGresult *res;
  GtkTreeIter iter;
  gchar *rut_proveedor;
  gchar **aux, *q;

  tree_view = gtk_tree_selection_get_tree_view(selection);
  store = GTK_LIST_STORE(gtk_tree_view_get_model(tree_view));

  if (gtk_tree_selection_get_selected (selection, NULL, &iter) == FALSE)
    return;

  gtk_tree_model_get (GTK_TREE_MODEL (store), &iter,
                      0, &rut_proveedor,
                      -1);

  aux = g_strsplit(rut_proveedor, "-", 0);

  /* consulta de sql que llama a funcion select_proveedor, que dado el id de
     este devuelve la informacion del mismo*/
  
  q = g_strdup_printf ("SELECT * FROM select_proveedor(%s)", aux[0]);
  res = EjecutarSQL (q);
  g_free (q);
  g_strfreev(aux);

  if ((res == NULL) || (PQntuples (res) == 0))
    return;

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_prov_name"));
  gtk_entry_set_text (GTK_ENTRY (widget), PQvaluebycol (res, 0, "nombre"));

  q = g_strconcat(PQvaluebycol (res, 0, "rut"), "-", PQvaluebycol(res, 0, "dv"), NULL);
  widget = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_prov_rut"));
  gtk_label_set_text (GTK_LABEL (widget), q);
  g_free (q);

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_prov_addr"));
  gtk_entry_set_text (GTK_ENTRY (widget), PQvaluebycol (res, 0, "direccion"));

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_prov_city"));
  gtk_entry_set_text (GTK_ENTRY (widget), PQvaluebycol (res, 0, "ciudad"));

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_prov_comuna"));
  gtk_entry_set_text (GTK_ENTRY (widget), PQvaluebycol (res, 0, "comuna"));

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_prov_phone"));
  gtk_entry_set_text (GTK_ENTRY (widget), PQvaluebycol (res, 0, "telefono"));

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_prov_web"));
  gtk_entry_set_text (GTK_ENTRY (widget), PQvaluebycol (res, 0, "web"));

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_prov_contact"));
  gtk_entry_set_text (GTK_ENTRY (widget), PQvaluebycol (res, 0, "contacto"));

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_prov_mail"));
  gtk_entry_set_text (GTK_ENTRY (widget), PQvaluebycol (res, 0, "email"));

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_prov_giro"));
  gtk_entry_set_text (GTK_ENTRY (widget), PQvaluebycol (res, 0, "giro"));

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_lap_rep"));
  gtk_entry_set_text (GTK_ENTRY (widget), PQvaluebycol (res, 0, "lapso_reposicion"));
}

/**
 * Es llamada por la funcion "AddProveedor()".
 * 
 * Esta funcion cierra la ventana de "wnd_addprovider".
 *
 * @param widget the widget that emited the signal
 * @param data the user data
 *
 */

void
CloseAgregarProveedorWindow (GtkWidget *widget, gpointer user_data)
{
  GtkWidget *window;

  window = GTK_WIDGET(gtk_builder_get_object(builder, "wnd_addprovider"));

  gtk_window_set_transient_for(GTK_WINDOW(window), NULL);

  gtk_widget_hide(window);

}

/**
 * Es llamada cuando el boton "btn_prov_ad" es presionado (signal click).
 * 
 * Esta funcion verifica que todos los campos de proveedor sean correctamente
 * rellenados y luego llama a la funcion "AddProveedorToDB()" que registra al
 * proveedor en la base de datos.
 *
 * @param widget the widget that emited the signal
 * @param data the user data
 *
 */

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

  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_addprov_rut"));
  rut_c = g_strdup (gtk_entry_get_text (GTK_ENTRY (aux_widget)));

  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_addprov_dv"));
  rut_ver = g_strdup (gtk_entry_get_text (GTK_ENTRY (aux_widget)));

  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_addprov_name"));
  nombre_c = g_strdup (gtk_entry_get_text (GTK_ENTRY (aux_widget)));

  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_addprov_addr"));
  direccion_c = g_strdup (gtk_entry_get_text (GTK_ENTRY (aux_widget)));

  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_addprov_city"));
  ciudad_c = g_strdup (gtk_entry_get_text (GTK_ENTRY (aux_widget)));

  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_addprov_comuna"));
  comuna_c = g_strdup (gtk_entry_get_text (GTK_ENTRY (aux_widget)));

  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_addprov_phone"));
  telefono_c = g_strdup (gtk_entry_get_text (GTK_ENTRY (aux_widget)));

  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_addprov_email"));
  email_c = g_strdup (gtk_entry_get_text (GTK_ENTRY (aux_widget)));

  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_addprov_web"));
  web_c = g_strdup (gtk_entry_get_text (GTK_ENTRY (aux_widget)));

  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_addprov_contact"));
  contacto_c = g_strdup (gtk_entry_get_text (GTK_ENTRY (aux_widget)));

  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_addprov_giro"));
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

  if (atoi(telefono_c) == 0)
    {
      ErrorMSG (wnd, "Debe ingresar sÃ³lo nÃºmeros en el campo telefono");
      return;
    }

  CloseAgregarProveedorWindow (NULL, user_data);

  str_rut = g_strdup_printf ("%s-%s", rut_c, rut_ver);
  AddProveedorToDB (str_rut, nombre_c, direccion_c, ciudad_c, comuna_c, telefono_c, email_c, web_c, contacto_c, giro_c);
  g_free (str_rut);
  //ListarProveedores (); <- this does not correspond

}

/**
 * Es llamada cuando el boton "btn_prov_ad" es presionado (signal click).
 * 
 * Esta funcion visualiza la ventana para agregar un proveedor y sus
 * respectivos entry para llenar con los campos de este.
 *
 * @param widget the widget that emited the signal
 * @param data the user data
 *
 */

void
AgregarProveedorWindow (GtkWidget *widget, gpointer user_data)
{
  GtkWidget *aux_widget;

  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_addprov_rut"));
  gtk_entry_set_text(GTK_ENTRY(aux_widget), "");

  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_addprov_dv"));
  gtk_entry_set_text(GTK_ENTRY(aux_widget), "");

  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_addprov_name"));
  gtk_entry_set_text(GTK_ENTRY(aux_widget), "");

  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_addprov_addr"));
  gtk_entry_set_text(GTK_ENTRY(aux_widget), "");

  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_addprov_comuna"));
  gtk_entry_set_text(GTK_ENTRY(aux_widget), "");

  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_addprov_city"));
  gtk_entry_set_text(GTK_ENTRY(aux_widget), "");

  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_addprov_phone"));
  gtk_entry_set_text(GTK_ENTRY(aux_widget), "");

  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_addprov_email"));
  gtk_entry_set_text(GTK_ENTRY(aux_widget), "");

  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_addprov_web"));
  gtk_entry_set_text(GTK_ENTRY(aux_widget), "");

  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_addprov_contact"));
  gtk_entry_set_text(GTK_ENTRY(aux_widget), "");

  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_addprov_giro"));
  gtk_entry_set_text(GTK_ENTRY(aux_widget), "");

  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "wnd_addprovider"));
  gtk_window_set_transient_for(GTK_WINDOW(aux_widget),
                               GTK_WINDOW(gtk_widget_get_toplevel(widget)));
  gtk_window_set_modal (GTK_WINDOW(aux_widget), TRUE);
  gtk_widget_show (aux_widget);
}

/**
 * Es llamada cuando el boton "btn_prov_save" es presionado (signal click).
 * 
 * Esta funcion carga los datos llenados con los datos del proveedor y luego
 * llama a la funcion "SetModificacionesProveedor() que actualiza los datos
 * de este en la base de datos.
 */

void
ModificarProveedor (void)
{
  GtkWidget *widget;
  gchar *rut_c;
  gchar *nombre_c;
  gchar *direccion_c;
  gchar *comuna_c;
  gchar *ciudad_c;
  gchar *fono_c;
  gchar *web_c;
  gchar *contacto_c;
  gchar *email_c;
  gchar *giro_c;
  gchar *lap_rep_c;

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_prov_rut"));
  rut_c = g_strdup(gtk_label_get_text (GTK_LABEL (widget)));

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_prov_name"));
  nombre_c = g_strdup(gtk_entry_get_text (GTK_ENTRY (widget)));

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_prov_addr"));
  direccion_c = g_strdup(gtk_entry_get_text (GTK_ENTRY (widget)));

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_prov_comuna"));
  comuna_c = g_strdup(gtk_entry_get_text (GTK_ENTRY (widget)));

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_prov_city"));
  ciudad_c = g_strdup(gtk_entry_get_text (GTK_ENTRY (widget)));

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_prov_phone"));
  fono_c = g_strdup(gtk_entry_get_text (GTK_ENTRY (widget)));

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_prov_web"));
  web_c = g_strdup(gtk_entry_get_text (GTK_ENTRY (widget)));

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_prov_contact"));
  contacto_c = g_strdup(gtk_entry_get_text (GTK_ENTRY (widget)));

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_prov_mail"));
  email_c = g_strdup(gtk_entry_get_text (GTK_ENTRY (widget)));

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_prov_giro"));
  giro_c = g_strdup(gtk_entry_get_text (GTK_ENTRY (widget)));

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_lap_rep"));
  lap_rep_c = g_strdup(gtk_entry_get_text (GTK_ENTRY (widget)));

  gint respuesta = SetModificacionesProveedor (rut_c, nombre_c, direccion_c, comuna_c, ciudad_c, fono_c,
					       web_c, contacto_c, email_c, giro_c, lap_rep_c);
  widget = GTK_WIDGET(gtk_builder_get_object(builder, "statusbar"));
  if (respuesta == 0)
    statusbar_push (GTK_STATUSBAR(widget), "El proveedor ha sido actualizado exitosamente", 3000);
  else if (respuesta == -1)
    statusbar_push (GTK_STATUSBAR(widget), "Error: Lapso Reposición debe ser un valor numérico y mayor a cero", 3000);
}

/**
 * Es llamada por la funcion "compras_win()"[compras.c] 
 * 
 * Esta funcion visualiza los nombres de cada columna del tree view de
 * proveedores, y los datos para poder exportarlos a Gnumeric.
 *
 */

void
proveedores_box ()
{
  GtkWidget *proveedores_tree;
  GtkListStore *store;
  GtkTreeSelection *selection;
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
  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(proveedores_tree));

  g_signal_connect (G_OBJECT (selection), "changed",
                    G_CALLBACK (LlenarDatosProveedor), NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Rut", renderer,
                                                     "text", 0,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (proveedores_tree), column);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Proveedor", renderer,
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
