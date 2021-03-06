/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4;
   c-indentation-style: gnu -*- */
/*credito.c
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
#include<math.h>

#include"tipos.h"
#include"credito.h"
#include"postgres-functions.h"
#include"errors.h"

#include"printing.h"

#include"utils.h"
#include"boleta.h"
#include"config_file.h"
#include"manejo_productos.h"
#include"caja.h"

GtkWidget *modificar_window;

/////////do not delete
///used for the print in the client and emisores tab
Print *client_list;
Print *client_gf_list;
Print *client_detail;
Print *client_gf_detail;

Print *emisores_list;

Print *deudas_cliente;
/////////

/**
 * Fill the labels and the entry of the credit dialog
 *
 * @param rut the rut of the client
 * @param name the name of the client
 * @param address the address of the client
 * @param phone the phone of the client
 */
void
fill_credit_data (const gchar *rut, const gchar *name, const gchar *address, const gchar *phone)
{
  GtkLabel *label;
  GtkEntry *entry;

  entry= GTK_ENTRY(gtk_builder_get_object(builder, "entry_credit_rut"));
  gtk_entry_set_text(entry, rut);

  label = GTK_LABEL(gtk_builder_get_object(builder, "lbl_client_name"));
  gtk_label_set_text(label, name);

  label = GTK_LABEL(gtk_builder_get_object(builder, "lbl_client_addr"));
  gtk_label_set_text(label, address);

  label = GTK_LABEL(gtk_builder_get_object(builder, "lbl_client_phone"));
  gtk_label_set_text(label, phone);

  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_credit_sale")), TRUE);

  return;
}

void
search_client (GtkWidget *widget, gpointer data)
{
  GtkWidget *aux_widget;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkListStore *store=NULL;
  GtkTreeIter iter;
  gchar *string;
  PGresult *res;
  gint tuples, i;
  gchar *enable, *nombre_entry;
  gchar *q, *tipo;

  string = g_strdup (gtk_entry_get_text (GTK_ENTRY(widget)));
  clean_container (GTK_CONTAINER (builder_get (builder, "wnd_client_search")));
  gtk_entry_set_text (GTK_ENTRY (builder_get (builder, "entry_search_client")), string);

  nombre_entry = g_strdup (gtk_buildable_get_name (GTK_BUILDABLE (widget)));

  if (g_str_equal (nombre_entry, "entry_invoice_rut") )// || // Facturacion-Guia
    //g_str_equal (nombre_entry, "entry_rutcli_pre_factura") ) // Venta a clientes registrados
    client_type = INVOICE;
  else if  (g_str_equal (nombre_entry, "entry_credit_rut") ||
	    g_str_equal (nombre_entry, "entry_rut_mixed_pay") ||
	    g_str_equal (nombre_entry, "entry_rut_mixed_pay_2"))
    client_type = CREDIT;
  else if (g_str_equal (nombre_entry, "entry_reserva_rut"))
    client_type = ALL;
  else
    client_type = ALL;

  //TODO: Debe haber uno para mostrar ambos
  if (client_type == INVOICE)
    tipo = g_strdup ("AND tipo = 'factura'");
  else if (client_type == CREDIT)
    tipo = g_strdup ("AND tipo = 'credito'");
  else if (client_type == ALL)
    tipo = g_strdup("");

  //ANTES: SELECT rut::varchar || '-' || dv, nombre || ' ' || apell_p, telefono, credito_enable, direccion
  q = g_strdup_printf ("SELECT rut, dv, nombre || ' ' || apell_p AS name, telefono, credito_enable, direccion "
                       "FROM cliente WHERE activo = 't' AND (lower(nombre) LIKE lower('%s%%') OR "
                       "lower(apell_p) LIKE lower('%s%%') OR lower(apell_m) LIKE lower('%s%%') OR "
                       "rut::varchar like ('%s%%')) %s",
                       string, string, string, string, tipo);
  res = EjecutarSQL (q);
  g_free (q);

  tuples = PQntuples (res);

  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "treeview_clients"));
  store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(aux_widget)));

  if (store == NULL)
    {
      if (user_data->user_id == 1) // De ser admin adquiere la visibilidad de la columna credito
	store = gtk_list_store_new (4,
				    G_TYPE_STRING,
				    G_TYPE_STRING,
				    G_TYPE_STRING,
				    G_TYPE_BOOLEAN);
      else
	store = gtk_list_store_new (3,
				    G_TYPE_STRING,
				    G_TYPE_STRING,
				    G_TYPE_STRING);

      gtk_tree_view_set_model (GTK_TREE_VIEW(aux_widget),
                               GTK_TREE_MODEL(store));

      //Rut
      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes ("Rut", renderer,
                                                         "text", 0,
                                                         NULL);
      gtk_tree_view_append_column (GTK_TREE_VIEW(aux_widget), column);
      gtk_tree_view_column_set_cell_data_func (column, renderer, control_rut, (gpointer)0, NULL);

      //nombre
      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes ("Nombre", renderer,
                                                         "text", 1,
                                                         NULL);
      gtk_tree_view_append_column (GTK_TREE_VIEW(aux_widget), column);

      //telefono
      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes ("Telefono", renderer,
                                                         "text", 2,
                                                         NULL);
      gtk_tree_view_append_column (GTK_TREE_VIEW(aux_widget), column);

      //credito
      if (user_data->user_id == 1)
	{
	  renderer = gtk_cell_renderer_toggle_new ();
	  column = gtk_tree_view_column_new_with_attributes ("Credito", renderer,
							     "active", 3,
							     NULL);
	  gtk_tree_view_append_column (GTK_TREE_VIEW(aux_widget), column);
	}
    }

  gtk_list_store_clear (store);

  for (i = 0; i < tuples; i++)
    {
      gtk_list_store_append (store, &iter);

      if (user_data->user_id == 1)
        {
          enable = PQvaluebycol (res, i, "credito_enable");
          gtk_list_store_set (store, &iter,
                              0, g_strconcat (PQvaluebycol (res, i, "rut"),
					      PQvaluebycol (res, i, "dv"), NULL),
                              1, PQvaluebycol (res, i, "name"),
                              2, PQvaluebycol (res, i, "telefono"),
                              3, strcmp (enable, "t") ? FALSE : TRUE,
                              -1);
        }
      else
        gtk_list_store_set (store, &iter,
                            0, g_strconcat (PQvaluebycol (res, i, "rut"),
					    PQvaluebycol (res, i, "dv"), NULL),
                            1, PQvaluebycol (res, i, "name"),
                            2, PQvaluebycol (res, i, "telefono"),
                            -1);
    }

  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "wnd_client_search"));
  gtk_widget_show_all (aux_widget);

  if (tuples > 0)
    {
      gtk_widget_grab_focus (GTK_WIDGET (builder_get (builder, "treeview_clients")));
      gtk_tree_selection_select_path (gtk_tree_view_get_selection (GTK_TREE_VIEW (builder_get (builder, "treeview_clients"))),
                                      gtk_tree_path_new_from_string ("0"));

      //Si la opcion CUENTA_RAPIDA esta habilitada, se selecciona el cliente buscado (si solo se ha encontrado 1)
      if (!g_str_equal (rizoma_get_value ("CUENTA_RAPIDA"), "NONE")  && 
	  !HaveCharacters (rizoma_get_value ("CUENTA_RAPIDA")) &&
	  tuples == 1)
	gtk_button_clicked (GTK_BUTTON (builder_get (builder, "btn_client_ok")));
    }
  else
    gtk_widget_grab_focus (widget);
}

/**
 *
 *
 */
void
search_client_abono (GtkWidget *widget, gpointer data)
{
  GtkWidget *aux_widget;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkListStore *store=NULL;
  GtkTreeIter iter;
  gchar *string;
  PGresult *res;
  gint tuples, i;
  gchar *enable;
  gchar *q;

  if (GTK_IS_ENTRY (widget))
    string = g_strdup (gtk_entry_get_text (GTK_ENTRY(widget)));
  else if (data != NULL && GTK_IS_ENTRY ((GtkWidget *) data))
    string = g_strdup (gtk_entry_get_text (GTK_ENTRY((GtkWidget *) data)));
  else
    string = "";

  clean_container (GTK_CONTAINER (builder_get (builder, "wnd_search_client_abono")));
  gtk_entry_set_text (GTK_ENTRY (builder_get (builder, "entry_search_client_abono")), string);

  //ANTES: SELECT rut::varchar || '-' || dv, nombre || ' ' || apell_p, telefono, credito_enable, direccion
  q = g_strdup_printf ("SELECT rut, dv, nombre || ' ' || apell_p || ' ' || apell_m AS name, telefono, credito_enable, direccion "
                       "FROM cliente WHERE activo = 't' AND (lower(nombre) LIKE lower('%s%%') OR "
                       "lower(apell_p) LIKE lower('%s%%') OR lower(apell_m) LIKE lower('%s%%') OR "
                       "rut::varchar like ('%s%%'))",
                       string, string, string, string);
  res = EjecutarSQL (q);
  g_free (q);

  tuples = PQntuples (res);

  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "treeview_sca"));
  store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(aux_widget)));

  if (store == NULL)
    {
      if (user_data->user_id == 1) // De ser admin adquiere la visibilidad de la columna credito
	{
	  store = gtk_list_store_new (4,
				      G_TYPE_STRING,
				      G_TYPE_STRING,
				      G_TYPE_STRING,
				      G_TYPE_BOOLEAN);
	} else {
	  store = gtk_list_store_new (3,
				      G_TYPE_STRING,
				      G_TYPE_STRING,
				      G_TYPE_STRING);
      }

      gtk_tree_view_set_model (GTK_TREE_VIEW(aux_widget),
                               GTK_TREE_MODEL(store));

      //Rut
      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes ("Rut", renderer,
                                                         "text", 0,
                                                         NULL);
      gtk_tree_view_append_column (GTK_TREE_VIEW(aux_widget), column);
      gtk_tree_view_column_set_cell_data_func (column, renderer, control_rut, (gpointer)0, NULL);

      //nombre
      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes ("Nombre", renderer,
                                                         "text", 1,
                                                         NULL);
      gtk_tree_view_append_column (GTK_TREE_VIEW(aux_widget), column);

      //telefono
      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes ("Telefono", renderer,
                                                         "text", 2,
                                                         NULL);
      gtk_tree_view_append_column (GTK_TREE_VIEW(aux_widget), column);

      //credito
      if (user_data->user_id == 1)
	{
	  renderer = gtk_cell_renderer_toggle_new ();
	  column = gtk_tree_view_column_new_with_attributes ("Credito", renderer,
							     "active", 3,
							     NULL);
	  gtk_tree_view_append_column (GTK_TREE_VIEW(aux_widget), column);
	}
    }

  gtk_list_store_clear (store);

  for (i = 0; i < tuples; i++)
    {
      gtk_list_store_append (store, &iter);

      if (user_data->user_id == 1)
        {
          enable = PQvaluebycol (res, i, "credito_enable");
          gtk_list_store_set (store, &iter,
                              0, g_strconcat (PQvaluebycol (res, i, "rut"),
					      PQvaluebycol (res, i, "dv"), NULL),
                              1, PQvaluebycol (res, i, "name"),
                              2, PQvaluebycol (res, i, "telefono"),
                              3, strcmp (enable, "t") ? FALSE : TRUE,
                              -1);
        }
      else
        gtk_list_store_set (store, &iter,
                            0, g_strconcat (PQvaluebycol (res, i, "rut"),
					    PQvaluebycol (res, i, "dv"), NULL),
                            1, PQvaluebycol (res, i, "name"),
                            2, PQvaluebycol (res, i, "telefono"),
                            -1);
    }

  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "wnd_search_client_abono"));
  gtk_widget_show_all (aux_widget);

  if (tuples > 0)
    {
      gtk_widget_grab_focus (GTK_WIDGET (builder_get (builder, "treeview_sca")));
      gtk_tree_selection_select_path (gtk_tree_view_get_selection (GTK_TREE_VIEW (builder_get (builder, "treeview_sca"))),
                                      gtk_tree_path_new_from_string ("0"));
    }
  else
    gtk_widget_grab_focus (widget);
}

void
clientes_box ()
{
  GtkWidget *widget;
  GtkWidget *tree;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkTreeSelection *selection;
  GtkListStore *store;
  GtkListStore *ventas;
  GtkListStore *ventas_details;

  client_list = (Print *) malloc (sizeof (Print));
  client_detail = (Print *) malloc (sizeof (Print));
  client_detail->son = (Print *) malloc (sizeof (Print));

  ///////////////// clients
  store = gtk_list_store_new (7,
                              G_TYPE_STRING,  //rut
                              G_TYPE_STRING,  //name + apell_p + apell_m
                              G_TYPE_INT,     //phone
                              G_TYPE_BOOLEAN, //credit enabled
                              G_TYPE_STRING,  //cupo
                              G_TYPE_STRING,  //deuda
                              G_TYPE_STRING); //saldo
  FillClientStore (store);

  tree = GTK_WIDGET (gtk_builder_get_object(builder, "treeview_clients"));
  gtk_tree_view_set_model(GTK_TREE_VIEW(tree), GTK_TREE_MODEL(store));
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));

  gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);

  g_signal_connect (G_OBJECT (selection), "changed",
                    G_CALLBACK (DatosDeudor), NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Rut", renderer,
                                                     "text", 0,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, TRUE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_rut, (gpointer)0, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Nombre", renderer,
                                                     "text", 1,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  gtk_tree_view_column_set_resizable (column, TRUE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Telefono", renderer,
                                                     "text", 2,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, TRUE);

  renderer = gtk_cell_renderer_toggle_new ();
  column = gtk_tree_view_column_new_with_attributes ("Credito", renderer,
                                                     "active", 3,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, TRUE);

  g_signal_connect (G_OBJECT (renderer), "toggled",
                    G_CALLBACK (ToggleClientCredit), NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Cupo", renderer,
                                                     "text", 4,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, TRUE);


  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Deuda", renderer,
                                                     "text", 5,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, TRUE);


  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Saldo", renderer,
                                                     "text", 6,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, TRUE);

  client_list->tree = GTK_TREE_VIEW (tree);
  client_list->title = "Lista de clientes";
  client_list->date_string = NULL;
  client_list->cols[0].name = "Codigo";
  client_list->cols[0].num = 0;
  client_list->cols[1].name = "Nombre";
  client_list->cols[1].num = 1;
  client_list->cols[2].name = "Telefono";
  client_list->cols[2].num = 2;
  client_list->cols[3].name = "Cupo";
  client_list->cols[3].num = 3;
  client_list->cols[4].name = "Deuda";
  client_list->cols[4].num = 4;
  client_list->cols[5].name = "Saldo";
  client_list->cols[5].num = 5;
  client_list->cols[6].name = NULL;

  if (user_data->user_id != 1)
    {
      widget = GTK_WIDGET (gtk_builder_get_object(builder, "btn_admin_del_client"));
      gtk_widget_hide (widget);

      widget = GTK_WIDGET (gtk_builder_get_object(builder, "btn_admin_edit_client"));
      gtk_widget_hide (widget);

      widget = GTK_WIDGET (gtk_builder_get_object(builder, "btn_admin_add_client"));
      gtk_widget_hide (widget);


      widget = GTK_WIDGET (gtk_builder_get_object(builder, "btn_admin_print_client"));
      gtk_widget_hide (widget);

      /* g_signal_connect (G_OBJECT (button), "clicked", */
      /* G_CALLBACK (PrintTree), (gpointer)client_list); */
    }

  ////////////////sales
  ventas = gtk_list_store_new (8,
                               G_TYPE_INT,      //ID Venta
                               G_TYPE_INT,      //Maquina
                               G_TYPE_STRING,   //Vendedor
                               G_TYPE_STRING,   //Fecha
                               G_TYPE_STRING,   //Modo Pago
			       G_TYPE_STRING,   //Tipo Pago
			       G_TYPE_STRING,   //Complemento
			       G_TYPE_STRING);  //Monto

  //FillVentasDeudas ();

  tree = GTK_WIDGET (gtk_builder_get_object(builder, "treeview_sales"));
  gtk_tree_view_set_model (GTK_TREE_VIEW(tree), GTK_TREE_MODEL(ventas));
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));

  g_signal_connect (G_OBJECT (selection), "changed",
                    G_CALLBACK (ChangeDetalle), NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("ID Venta", renderer,
                                                     "text", 0,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Maquina", renderer,
                                                     "text", 1,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Vendedor", renderer,
                                                     "text", 2,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_alignment (column, 0.5);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Fecha", renderer,
                                                     "text", 3,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_alignment (column, 0.5);
  gtk_tree_view_column_set_min_width (column, 120);
  gtk_tree_view_column_set_max_width (column, 120);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Modo Pago", renderer,
                                                     "text", 4,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Tipo Pago", renderer,
                                                     "text", 5,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Complemento", renderer,
                                                     "text", 6,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Monto", renderer,
                                                     "text", 7,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  client_detail->tree = GTK_TREE_VIEW (tree);
  client_detail->title = "Ventas por Cliente";
  client_detail->name = "Clientes";
  client_detail->date_string = NULL;
  client_detail->cols[0].name = "ID Venta";
  client_detail->cols[0].num = 0;
  client_detail->cols[1].name = "Maquina";
  client_detail->cols[1].num = 1;
  client_detail->cols[2].name = "Vendedor";
  client_detail->cols[2].num = 2;
  client_detail->cols[3].name = "Fecha";
  client_detail->cols[3].num = 3;
  client_detail->cols[4].name = "Modo Pago";
  client_detail->cols[4].num = 4;
  client_detail->cols[5].name = "Tipo Pago";
  client_detail->cols[5].num = 5;
  client_detail->cols[6].name = "Complemento";
  client_detail->cols[6].num = 6;
  client_detail->cols[7].name = "Monto";
  client_detail->cols[7].num = 7;
  client_detail->cols[8].name = NULL;


  //sale details
  ventas_details = gtk_list_store_new (6,
                                       G_TYPE_STRING,
                                       G_TYPE_STRING,
                                       G_TYPE_STRING,
                                       G_TYPE_DOUBLE,
                                       G_TYPE_STRING,
                                       G_TYPE_STRING);

  tree = GTK_WIDGET (gtk_builder_get_object(builder, "treeview_sale_details"));
  gtk_tree_view_set_model(GTK_TREE_VIEW(tree), GTK_TREE_MODEL(ventas_details));

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Codigo", renderer,
                                                     "text", 0,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_min_width (column, 100);
  gtk_tree_view_column_set_max_width (column, 100);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Producto", renderer,
                                                     "text", 1,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_min_width (column, 200);
  gtk_tree_view_column_set_max_width (column, 200);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Marca", renderer,
                                                     "text", 2,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_min_width (column, 130);
  gtk_tree_view_column_set_max_width (column, 130);


  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Cantidad", renderer,
                                                     "text", 3,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)3, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Unitario", renderer,
                                                     "text", 4,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Total", renderer,
                                                     "text", 5,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  /* We fill the struct to print the last two tree views */

  client_detail->son->tree = GTK_TREE_VIEW (tree);
  client_detail->son->cols[0].name = "Codigo";
  client_detail->son->cols[0].num = 0;
  client_detail->son->cols[1].name = "Producto";
  client_detail->son->cols[1].num = 1;
  client_detail->son->cols[2].name = "Marca";
  client_detail->son->cols[2].num = 2;
  client_detail->son->cols[3].name = "Cantidad";
  client_detail->son->cols[3].num = 3;
  client_detail->son->cols[4].name = "Unitario";
  client_detail->son->cols[4].num = 4;
  client_detail->son->cols[5].name = "Total";
  client_detail->son->cols[5].num = 5;
  client_detail->son->cols[6].name = NULL;

  /* g_signal_connect (G_OBJECT (button), "clicked", */
  /*     G_CALLBACK (PrintTwoTree), (gpointer)client_detail); */

  setup_print_menu();

}



/**
 * Callback connected to the toggle button of the products_providers treeview
 *
 * This function enable or disable the product's selection.
 * @param toggle the togle button
 * @param path_str the path of the selected row
 * @param data the user data
 */

void
ToggleGuiaSelection (GtkCellRendererToggle *toggle, char *path_str, gpointer data)
{
  GtkWidget *treeview;
  GtkTreeModel *store;
  gboolean enable, valid;
  GtkTreePath *path;
  GtkTreeIter iter;
  gchar *tipo_documento;
  gint subTotal = 0, monto = 0;
  gint id_venta_seleccionado, id_venta, id_documento_seleccionado, id_documento;

  treeview = GTK_WIDGET (gtk_builder_get_object (builder, "treeview_guia_factura"));
  store = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
  path = gtk_tree_path_new_from_string (path_str);

  gtk_tree_model_get_iter (store, &iter, path);
  gtk_tree_model_get (store, &iter,
		      0, &id_documento_seleccionado,
                      1, &enable,
		      2, &id_venta_seleccionado,
		      6, &tipo_documento,
                      -1);

  // Se cambia la seleccion del checkButton (solo si es guia)
  if (g_str_equal (tipo_documento, "GUIA"))
    enable = !(enable);
  gtk_tree_store_set ((GtkTreeStore*)store, &iter,
                      1, enable,
                      -1);

  // Se calcula el subtotal de todas las filas que fueron seleccionadas
  valid = gtk_tree_model_get_iter_first (store, &iter);
  while (valid)
    {
      /*Se seleccionan automáticamente todas las guias asociadas a la misma venta de la guía seleccionada a mano*/
      gtk_tree_model_get (store, &iter,
			  0, &id_documento,
			  2, &id_venta,
			  -1);
      if (id_venta == id_venta_seleccionado && id_documento_seleccionado != id_documento)
	gtk_tree_store_set ((GtkTreeStore*)store, &iter,
			    1, enable,
			    -1);

      /*Se suma el total de todas las guias seleccionadas*/
      gtk_tree_model_get (store, &iter,
			  1, &enable,
			  7, &monto,
			  -1);
      if (enable == TRUE)
	subTotal += monto;

      // Itero a la siguiente fila --
      valid = gtk_tree_model_iter_next (store, &iter); /* Me da TRUE si itera a la siguiente */
    }

  gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "lbl_total_facturar")),
			g_strdup_printf ("<b>%ld</b>", lround (subTotal)));

  if (subTotal > 0)
    gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_facturar_guias")), TRUE);
  else
    gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_facturar_guias")), FALSE);

  gtk_tree_path_free (path);
}


void
guias_facturas_box ()
{
  GtkWidget *tree;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkTreeSelection *selection;
  GtkListStore *store;
  GtkTreeStore *ventas;
  GtkListStore *ventas_details;

  client_gf_list = (Print *) malloc (sizeof (Print));
  client_gf_detail = (Print *) malloc (sizeof (Print));
  client_gf_detail->son = (Print *) malloc (sizeof (Print));

  fill_combo_tipo_documento (GTK_COMBO_BOX (builder_get (builder,"cmb_tipo_documento")), 0);

  ///////////////// clients
  store = gtk_list_store_new (6,
                              G_TYPE_STRING,  //rut
                              G_TYPE_STRING,  //name + apell_p + apell_m
                              G_TYPE_INT,     //phone
                              G_TYPE_STRING,  //cupo
                              G_TYPE_STRING,  //deuda
                              G_TYPE_STRING); //saldo
  //FillClientStore (store);

  tree = GTK_WIDGET (gtk_builder_get_object(builder, "treeview_clients_gf"));
  gtk_tree_view_set_model(GTK_TREE_VIEW(tree), GTK_TREE_MODEL(store));
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));

  gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);

  g_signal_connect (G_OBJECT (selection), "changed",
                    G_CALLBACK (datos_deuda_factura_guia), NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Rut", renderer,
                                                     "text", 0,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, TRUE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_rut, (gpointer)0, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Nombre", renderer,
                                                     "text", 1,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  gtk_tree_view_column_set_resizable (column, TRUE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Telefono", renderer,
                                                     "text", 2,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, TRUE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Cupo", renderer,
                                                     "text", 3,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, TRUE);


  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Deuda", renderer,
                                                     "text", 4,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, TRUE);


  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Saldo", renderer,
                                                     "text", 5,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, TRUE);

  client_gf_list->tree = GTK_TREE_VIEW (tree);
  client_gf_list->title = "Lista de clientes";
  client_gf_list->date_string = NULL;
  client_gf_list->cols[0].name = "Codigo";
  client_gf_list->cols[0].num = 0;
  client_gf_list->cols[1].name = "Nombre";
  client_gf_list->cols[1].num = 1;
  client_gf_list->cols[2].name = "Telefono";
  client_gf_list->cols[2].num = 2;
  client_gf_list->cols[3].name = "Cupo";
  client_gf_list->cols[3].num = 3;
  client_gf_list->cols[4].name = "Deuda";
  client_gf_list->cols[4].num = 4;
  client_gf_list->cols[5].name = "Saldo";
  client_gf_list->cols[5].num = 5;
  client_gf_list->cols[6].name = NULL;


  ////////////////sales
  ventas = gtk_tree_store_new (8,
			       G_TYPE_INT,      //ID documento
			       G_TYPE_BOOLEAN,  //Elegir (Para facturar)
                               G_TYPE_INT,      //ID Venta
                               G_TYPE_INT,      //Maquina
                               G_TYPE_STRING,   //Vendedor
                               G_TYPE_STRING,   //Fecha
                               G_TYPE_STRING,   //Tipo Documento
			       G_TYPE_INT);     //Monto

  //FillVentasDeudas ();

  tree = GTK_WIDGET (gtk_builder_get_object(builder, "treeview_guia_factura"));
  gtk_tree_view_set_model (GTK_TREE_VIEW(tree), GTK_TREE_MODEL(ventas));
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));

  g_signal_connect (G_OBJECT (selection), "changed",
                    G_CALLBACK (change_detalle_guia_factura), NULL);

  renderer = gtk_cell_renderer_toggle_new ();
  column = gtk_tree_view_column_new_with_attributes ("Elegir", renderer,
						     "active", 1,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  //gtk_tree_view_column_set_sort_column_id (column, 0);
  gtk_tree_view_column_set_resizable (column, FALSE);

  g_signal_connect (G_OBJECT (renderer), "toggled",
		    G_CALLBACK (ToggleGuiaSelection), NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("ID Venta", renderer,
                                                     "text", 2,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Maquina", renderer,
                                                     "text", 3,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Vendedor", renderer,
                                                     "text", 4,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_alignment (column, 0.5);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Fecha", renderer,
                                                     "text", 5,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_alignment (column, 0.5);
  gtk_tree_view_column_set_min_width (column, 120);
  gtk_tree_view_column_set_max_width (column, 120);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Tipo Documento", renderer,
                                                     "text", 6,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Monto", renderer,
                                                     "text", 7,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)7, NULL);

  client_gf_detail->tree = GTK_TREE_VIEW (tree);
  client_gf_detail->title = "Ventas por Cliente";
  client_gf_detail->name = "Clientes";
  client_gf_detail->date_string = NULL;
  client_gf_detail->cols[0].name = "ID Venta";
  client_gf_detail->cols[0].num = 2;
  client_gf_detail->cols[1].name = "Maquina";
  client_gf_detail->cols[1].num = 3;
  client_gf_detail->cols[2].name = "Vendedor";
  client_gf_detail->cols[2].num = 4;
  client_gf_detail->cols[3].name = "Fecha";
  client_gf_detail->cols[3].num = 5;
  client_gf_detail->cols[4].name = "Tipo Documento";
  client_gf_detail->cols[4].num = 6;
  client_gf_detail->cols[5].name = "Monto";
  client_gf_detail->cols[5].num = 7;
  client_gf_detail->cols[6].name = NULL;


  //sale details
  ventas_details = gtk_list_store_new (7,
                                       G_TYPE_STRING,
                                       G_TYPE_STRING,
                                       G_TYPE_STRING,
                                       G_TYPE_DOUBLE,
                                       G_TYPE_DOUBLE,
				       G_TYPE_DOUBLE,
                                       G_TYPE_INT);

  tree = GTK_WIDGET (gtk_builder_get_object(builder, "treeview_gf_sale_detail"));
  gtk_tree_view_set_model(GTK_TREE_VIEW(tree), GTK_TREE_MODEL(ventas_details));

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Codigo", renderer,
                                                     "text", 0,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_min_width (column, 100);
  gtk_tree_view_column_set_max_width (column, 100);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Producto", renderer,
                                                     "text", 1,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_min_width (column, 200);
  gtk_tree_view_column_set_max_width (column, 200);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Marca", renderer,
                                                     "text", 2,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_min_width (column, 130);
  gtk_tree_view_column_set_max_width (column, 130);


  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Cantidad", renderer,
                                                     "text", 3,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)3, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Precio Neto", renderer,
                                                     "text", 4,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)4, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Precio", renderer,
                                                     "text", 5,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)5, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Total", renderer,
                                                     "text", 6,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)6, NULL);

  /* We fill the struct to print the last two tree views */

  client_gf_detail->son->tree = GTK_TREE_VIEW (tree);
  client_gf_detail->son->cols[0].name = "Codigo";
  client_gf_detail->son->cols[0].num = 0;
  client_gf_detail->son->cols[1].name = "Producto";
  client_gf_detail->son->cols[1].num = 1;
  client_gf_detail->son->cols[2].name = "Marca";
  client_gf_detail->son->cols[2].num = 2;
  client_gf_detail->son->cols[3].name = "Cantidad";
  client_gf_detail->son->cols[3].num = 3;
  client_gf_detail->son->cols[4].name = "Precio Neto";
  client_gf_detail->son->cols[4].num = 4;
  client_gf_detail->son->cols[5].name = "Precio";
  client_gf_detail->son->cols[5].num = 5;
  client_gf_detail->son->cols[6].name = "Total";
  client_gf_detail->son->cols[6].num = 6;
  client_gf_detail->son->cols[7].name = NULL;

  g_signal_connect (G_OBJECT (builder_get (builder, "btn_print_gf")), "clicked",
		    G_CALLBACK (PrintTwoTree), (gpointer)client_gf_detail);

  //setup_print_menu();

}


/**
 * Obtiene los datos de las guias para facturarlas
 *
 * @param: GtkButton *button
 * @param: gpointer *data
 */
void
on_btn_facturar_guias_clicked (GtkButton *button, gpointer *data)
{
  GtkTreeView *treeview;
  GtkTreeSelection *selection;
  GtkTreeModel *store;
  gint id_factura, id_venta, id_guia, monto;
  gboolean valid, enable;
  gchar *rut_cliente;
  GtkTreeIter iter;

  //Treeview clientes Guias-Factura
  treeview = GTK_TREE_VIEW (builder_get (builder, "treeview_clients_gf"));
  store = gtk_tree_view_get_model (treeview);
  selection = gtk_tree_view_get_selection (treeview);

  if (gtk_tree_selection_get_selected (selection, NULL, &iter))
    gtk_tree_model_get (store, &iter,
			0, &rut_cliente,
			-1);

  id_factura = InsertNewDocumentVoid (0, FACTURA, CREDITO, g_strndup (rut_cliente, strlen (rut_cliente)-1));

  printf ("Se generó una factura con id: %d", id_factura);

  //Treeview Guia-Factura
  treeview = GTK_TREE_VIEW (builder_get (builder, "treeview_guia_factura"));
  store = gtk_tree_view_get_model (treeview);
  valid = gtk_tree_model_get_iter_first (store, &iter);
  while (valid)
    {
      gtk_tree_model_get (store, &iter,
			  0, &id_guia,
			  1, &enable,
			  2, &id_venta,
			  7, &monto,
			-1);
      if (enable == TRUE)
	facturar_guia (id_factura, id_guia, monto);

      // Itero a la siguiente fila --
      valid = gtk_tree_model_iter_next (store, &iter); /* Me da TRUE si itera a la siguiente */
    }

  //Se vuelve a seleccionar el cliente para actualizar la vista de sus guias sin facturar
  treeview = GTK_TREE_VIEW (builder_get (builder, "treeview_clients_gf"));
  store = gtk_tree_view_get_model (treeview);
  selection = gtk_tree_view_get_selection (treeview);
  datos_deuda_factura_guia (selection, NULL);
  gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "lbl_total_facturar")),
			g_strdup_printf ("<b>0</b>"));
}

/**
 * Obtiene los datos de la factura para marcarla como pagada
 *
 * @param: GtkButton *button
 * @param: gpointer *data
 */
void
on_btn_pagar_factura_clicked (GtkButton *button, gpointer *data)
{
  GtkTreeView *treeview;
  GtkTreeSelection *selection;
  GtkTreeModel *store;
  gint id_factura;
  gint id_venta;
  GtkTreeIter iter;

  treeview = GTK_TREE_VIEW (builder_get (builder, "treeview_guia_factura"));
  store = gtk_tree_view_get_model (treeview);
  selection = gtk_tree_view_get_selection (treeview);

  if (gtk_tree_selection_get_selected (selection, NULL, &iter))
    {
      gtk_tree_model_get (store, &iter,
			  0, &id_factura,
			  2, &id_venta,
			  -1);
    }

  pagar_factura (id_factura, id_venta);

  //Se vuelve a seleccionar el cliente para actualizar la vista de sus facturas impagas
  treeview = GTK_TREE_VIEW (builder_get (builder, "treeview_clients_gf"));
  store = gtk_tree_view_get_model (treeview);
  selection = gtk_tree_view_get_selection (treeview);
  datos_deuda_factura_guia (selection, NULL);
}

/**
 * Realiza la búsqueda de las deudas y abonos de cliente seleccionado
 * (signal-clicked) -> btn_search_abonos
 *
 * @param: GtkButton *button
 * @param: gpointer *data
 */
void
on_btn_search_abonos_clicked (GtkButton *button, gpointer *data)
{
  gchar *q, *rut_cliente;
  PGresult *res;
  gint tuples, i;

  gchar *id_venta, *abono;

  GtkListStore *store;
  GtkTreeView *treeview;
  GtkTreeIter iter;

  /*Variables de fecha*/
  GDate *date_begin;
  GDate *date_end;

  const gchar *str_begin = gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_date_begin")));
  const gchar *str_end = gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_date_end")));

  /*Se inicializan los GDate para la obtención de fechas*/
  date_begin = g_date_new ();
  date_end = g_date_new ();

  treeview = GTK_TREE_VIEW (builder_get (builder, "treeview_abonos_client"));
  store = GTK_LIST_STORE (gtk_tree_view_get_model (treeview));

  tuples = i = 0;
  rut_cliente = g_strdup (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_client_abono"))));

  if (DataExist (g_strdup_printf ("SELECT rut FROM cliente WHERE rut = %s", rut_cliente)))
    q = g_strdup_printf ("SELECT * FROM info_abonos(%s)", rut_cliente);
  else
    {
      ErrorMSG (GTK_WIDGET (builder_get (builder, "entry_client_abono")),
		g_strdup_printf ("No existe un cliente con rut %s", rut_cliente));
      return;
    }


  /*Se agrega el filtro de fechas a la consulta*/
  if (!g_str_equal (str_begin, "") || !g_str_equal (str_end, "")) {

    /*Solo fecha inicio*/
    if (!g_str_equal (str_begin, "") && g_str_equal (str_end, "")) {
      g_date_set_parse (date_begin, str_begin);
      q = g_strdup_printf ("%s WHERE out_fecha >= to_timestamp ('%.4d %.2d %.2d', 'YYYY MM DD')",
			   q, g_date_get_year (date_begin), g_date_get_month (date_begin), g_date_get_day (date_begin));
    }

    /*Solo fecha Termino*/
    else if (!g_str_equal (str_end, "") && g_str_equal (str_begin, "")) {
      g_date_set_parse (date_end, str_end);
      q = g_strdup_printf ("%s WHERE out_fecha < to_timestamp ('%.4d %.2d %.2d', 'YYYY MM DD') + '1 days'",
			   q, g_date_get_year (date_end), g_date_get_month (date_end), g_date_get_day (date_end));
    }

    /*Ambas fechas*/
    else if (!g_str_equal (str_end, "") && !g_str_equal (str_begin, "")) {
      g_date_set_parse (date_begin, str_begin);
      g_date_set_parse (date_end, str_end);
      q = g_strdup_printf ("%s WHERE out_fecha >= to_timestamp ('%.4d %.2d %.2d', 'YYYY MM DD') "
			   "AND out_fecha < to_timestamp ('%.4d %.2d %.2d', 'YYYY MM DD') + '1 days'",
			   q, g_date_get_year (date_begin), g_date_get_month (date_begin), g_date_get_day (date_begin),
			   g_date_get_year (date_end), g_date_get_month (date_end), g_date_get_day (date_end));
    }
  }

  res = EjecutarSQL (q);
  g_free (q);

  tuples = PQntuples (res);
  gtk_list_store_clear (store);

  for (i = 0; i < tuples; i++)
    {
      id_venta = g_strdup (PQvaluebycol (res, i, "out_id_venta"));
      abono = g_strdup (PQvaluebycol (res, i, "out_abono"));

      if (g_str_equal (id_venta, "0")) id_venta = g_strdup_printf ("--");
      if (g_str_equal (abono, "0")) abono = g_strdup_printf ("--");

      if (!g_str_equal (id_venta, "--") || //Para que no muestre el totalizador
	  !g_str_equal (abono, "--"))
	{
	  gtk_list_store_append (store, &iter);
	  gtk_list_store_set (store, &iter,
			      0, id_venta,
			      1, g_strdup (PQvaluebycol (res, i, "out_fecha")),
			      2, atoi (PQvaluebycol (res, i, "out_monto_deuda")),
			      3, PutPoints (abono),
			      4, atoi (PQvaluebycol (res, i, "out_deuda_total")),
			      -1);
	}
      g_free (id_venta);
      g_free (abono);
    }

}


/**
 * Obtiene el rut y nombre del cliente seleccionado
 * y 'setea' los 'widgets' correspondientes.
 *
 * (signal-clicked) -> btn_accept_sca
 *
 * @param: GtkButton *button
 * @param: gpointer *data
 */
void
on_btn_accept_sca_clicked (GtkButton *button, gpointer data)
{
  GtkWidget *aux;
  GtkListStore *store;
  GtkTreeSelection *selection;
  GtkTreeIter iter;
  gchar *rut, *nombre; //*dv

  aux = GTK_WIDGET(gtk_builder_get_object(builder, "treeview_sca"));
  store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(aux)));
  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(aux));

  if (!(gtk_tree_selection_get_selected (selection, NULL, &iter)))
    {
      aux = GTK_WIDGET (builder_get (builder, "wnd_search_client_abono"));
      AlertMSG (aux, "Debe seleccionar un cliente");
      return;
    }

  gtk_tree_model_get (GTK_TREE_MODEL(store), &iter,
                      0, &rut,
		      1, &nombre,
                      -1);

  aux = GTK_WIDGET (gtk_builder_get_object(builder, "wnd_search_client_abono"));
  gtk_widget_hide (aux);

  //dv = invested_strndup (rut, strlen (rut)-1);
  rut = g_strndup (rut, strlen (rut)-1);

  gtk_entry_set_text (GTK_ENTRY (builder_get (builder, "entry_client_abono")), rut);
  gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_name_selected")),
			g_strdup_printf ("<b>%s</b>", nombre));

  //Se habilitan las fechas
  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "entry_date_begin")), TRUE);
  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_date_begin")), TRUE);
  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "entry_date_end")), TRUE);
  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_date_end")), TRUE);

  gtk_widget_grab_focus (GTK_WIDGET (builder_get (builder, "btn_date_begin")));
  on_btn_search_abonos_clicked (GTK_BUTTON (builder_get (builder, "btn_search_abonos")), NULL);
}


/**
 * Limpia los treeviews, entry, y label
 * correspondientes de la pestaña 'abonos'
 *
 * (signal-clicked) -> btn_clean_abonos
 *
 * @param: GtkButton *button
 * @param: gpointer *data
 */
void
on_btn_clean_abonos_clicked (GtkButton *button, gpointer data)
{
  GtkTreeView *tree;

  gtk_entry_set_text (GTK_ENTRY (builder_get (builder, "entry_client_abono")), "");
  gtk_entry_set_text (GTK_ENTRY (builder_get (builder, "entry_date_begin")), "");
  gtk_entry_set_text (GTK_ENTRY (builder_get (builder, "entry_date_end")), "");
  gtk_label_set_text (GTK_LABEL (builder_get (builder, "lbl_name_selected")), "");

  //Se limpian los treeviews
  tree = GTK_TREE_VIEW (builder_get (builder, "treeview_abonos_client"));
  gtk_list_store_clear (GTK_LIST_STORE (gtk_tree_view_get_model (tree)));
  tree = GTK_TREE_VIEW (builder_get (builder, "treeview_detalle_deudas"));
  gtk_list_store_clear (GTK_LIST_STORE (gtk_tree_view_get_model (tree)));

  gtk_widget_grab_focus (GTK_WIDGET (builder_get (builder, "entry_client_abono")));
}

/**
 * To activate the "treeview_abono_sca" row
 */
void
on_treeview_sca_row_activated (GtkTreeView *treeview, GtkTreePath *arg1, GtkTreeViewColumn *arg2, gpointer data)
{
  on_btn_accept_sca_clicked (NULL, NULL);
}


/**
 * Abono Box
 * Inicializa la pestaña "Abonos" del modulo "rizoma-admin"
 */
void
abonos_box ()
{
  //GtkWidget *widget;
  GtkTreeView *tree;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkTreeSelection *selection;
  GtkListStore *store;
  //GtkListStore *ventas;
  GtkListStore *ventas_details;

  deudas_cliente = (Print *) malloc (sizeof (Print));
  deudas_cliente->son = (Print *) malloc (sizeof (Print));

  ////////////// Abonos
  store = gtk_list_store_new (5,
                              G_TYPE_STRING, //Fecha
                              G_TYPE_STRING, //id_venta
                              G_TYPE_INT,    //Monto Deuda
                              G_TYPE_STRING, //Abono
                              G_TYPE_INT);   //Deuda Total

  tree = GTK_TREE_VIEW (gtk_builder_get_object(builder, "treeview_abonos_client"));
  gtk_tree_view_set_model (tree, GTK_TREE_MODEL (store));
  selection = gtk_tree_view_get_selection (tree);

  gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);

  g_signal_connect (G_OBJECT (selection), "changed",
                    G_CALLBACK (ChangeDetalle), NULL);

  //Id Venta
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Id.venta", renderer,
                                                     "text", 0,
                                                     NULL);
  gtk_tree_view_append_column (tree, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);
  //gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)1, NULL);


  //Fecha
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Fecha", renderer,
                                                     "text", 1,
                                                     NULL);
  gtk_tree_view_append_column (tree, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);


  //Monto Deuda
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Monto Deuda", renderer,
                                                     "text", 2,
                                                     NULL);
  gtk_tree_view_append_column (tree, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)2, NULL);


  //Abono
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Abono", renderer,
                                                     "text", 3,
                                                     NULL);
  gtk_tree_view_append_column (tree, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);
  //gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)3, NULL);


  //Deuda Total
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Deuda Total", renderer,
                                                     "text", 4,
                                                     NULL);
  gtk_tree_view_append_column (tree, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)4, NULL);


  // Deudas y abonos del cliente
  deudas_cliente->tree = tree;
  deudas_cliente->title = "Lista de Deudas y Abonos";
  deudas_cliente->name = "Deduas y Abonos";
  deudas_cliente->date_string = NULL;
  deudas_cliente->cols[0].name = "Id Venta";
  deudas_cliente->cols[0].num = 0;
  deudas_cliente->cols[1].name = "Fecha";
  deudas_cliente->cols[1].num = 1;
  deudas_cliente->cols[2].name = "Monto Deuda";
  deudas_cliente->cols[2].num = 2;
  deudas_cliente->cols[3].name = "Abono";
  deudas_cliente->cols[3].num = 3;
  deudas_cliente->cols[4].name = "Deuda Total";
  deudas_cliente->cols[4].num = 4;
  deudas_cliente->cols[5].name = NULL;

  g_signal_connect (G_OBJECT (builder_get (builder, "btn_print_payment_report")), "clicked",
		    G_CALLBACK (PrintTree), (gpointer)deudas_cliente);

  ////////////////sales
  ventas_details = gtk_list_store_new (6,
                                       G_TYPE_STRING,
                                       G_TYPE_STRING,
                                       G_TYPE_STRING,
                                       G_TYPE_DOUBLE,
                                       G_TYPE_STRING,
                                       G_TYPE_STRING);

  tree = GTK_TREE_VIEW (gtk_builder_get_object(builder, "treeview_detalle_deudas"));
  gtk_tree_view_set_model (tree, GTK_TREE_MODEL(ventas_details));

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Codigo", renderer,
                                                     "text", 0,
                                                     NULL);
  gtk_tree_view_append_column (tree, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_min_width (column, 100);
  gtk_tree_view_column_set_max_width (column, 100);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Producto", renderer,
                                                     "text", 1,
                                                     NULL);
  gtk_tree_view_append_column (tree, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_min_width (column, 200);
  gtk_tree_view_column_set_max_width (column, 200);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Marca", renderer,
                                                     "text", 2,
                                                     NULL);
  gtk_tree_view_append_column (tree, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_min_width (column, 130);
  gtk_tree_view_column_set_max_width (column, 130);


  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Cantidad", renderer,
                                                     "text", 3,
                                                     NULL);
  gtk_tree_view_append_column (tree, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)3, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Unitario", renderer,
                                                     "text", 4,
                                                     NULL);
  gtk_tree_view_append_column (tree, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Total", renderer,
                                                     "text", 5,
                                                     NULL);
  gtk_tree_view_append_column (tree, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  /* We fill the struct to print the last two tree views */

  deudas_cliente->son->tree = tree;
  deudas_cliente->son->cols[0].name = "Codigo";
  deudas_cliente->son->cols[0].num = 0;
  deudas_cliente->son->cols[1].name = "Producto";
  deudas_cliente->son->cols[1].num = 1;
  deudas_cliente->son->cols[2].name = "Marca";
  deudas_cliente->son->cols[2].num = 2;
  deudas_cliente->son->cols[3].name = "Cantidad";
  deudas_cliente->son->cols[3].num = 3;
  deudas_cliente->son->cols[4].name = "Unitario";
  deudas_cliente->son->cols[4].num = 4;
  deudas_cliente->son->cols[5].name = "Total";
  deudas_cliente->son->cols[5].num = 5;
  deudas_cliente->son->cols[6].name = NULL;

  g_signal_connect (G_OBJECT (builder_get (builder, "btn_print_payment_detail")), "clicked",
		    G_CALLBACK (PrintTwoTree), (gpointer)deudas_cliente);

  //setup_print_menu();
}


/**
 * Emisores box
 *
 */
void
emisores_box ()
{
  //GtkWidget *widget;
  GtkWidget *tree;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkTreeSelection *selection;
  GtkListStore *store;
  GtkListStore *cheques;

  emisores_list = (Print *) malloc (sizeof (Print));
  emisores_list->son = (Print *) malloc (sizeof (Print));

  // Emisores ----------------------------------------------
  store = gtk_list_store_new (5,
			      G_TYPE_INT,     //Id
                              G_TYPE_STRING,  //rut
                              G_TYPE_STRING,  //razon social
                              G_TYPE_STRING,  //phone
                              G_TYPE_STRING); //direccion
  //FillClientStore (store);

  tree = GTK_WIDGET (gtk_builder_get_object(builder, "treeview_emisores"));
  gtk_tree_view_set_model (GTK_TREE_VIEW(tree), GTK_TREE_MODEL(store));
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));

  gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);

  g_signal_connect (G_OBJECT (selection), "changed",
		    G_CALLBACK (datos_cheques_restaurant), NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Rut", renderer,
                                                     "text", 1,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_min_width (column, 150);
  gtk_tree_view_column_set_max_width (column, 200);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_rut, (gpointer)1, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Razon Social", renderer,
                                                     "text", 2,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  gtk_tree_view_column_set_resizable (column, TRUE);
  gtk_tree_view_column_set_min_width (column, 200);
  gtk_tree_view_column_set_max_width (column, 300);

  column = gtk_tree_view_column_new_with_attributes ("Telefono", renderer,
                                                     "text", 3,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_min_width (column, 100);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Direccion", renderer,
                                                     "text", 4,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_min_width (column, 200);

  emisores_list->tree = GTK_TREE_VIEW (tree);
  emisores_list->title = "Lista de Emisores";
  emisores_list->name = "Emisores";
  emisores_list->date_string = NULL;
  emisores_list->cols[0].name = "Rut";
  emisores_list->cols[0].num = 1;
  emisores_list->cols[1].name = "Razon Social";
  emisores_list->cols[1].num = 2;
  emisores_list->cols[2].name = "Telefono";
  emisores_list->cols[2].num = 3;
  emisores_list->cols[3].name = "Direccion";
  emisores_list->cols[3].num = 4;
  emisores_list->cols[4].name = NULL;

  // Cheques ----------------------------------------------
  cheques = gtk_list_store_new (8,
				G_TYPE_INT,     //id
				G_TYPE_INT,     //id_venta
				G_TYPE_STRING,  //Fecha Venta
				G_TYPE_STRING,  //Monto Venta
				G_TYPE_STRING,  //Codigo Cheque
				G_TYPE_STRING,  //Monto Cheque
				G_TYPE_STRING,  //Fecha Vencimiento
				G_TYPE_STRING);  //Facturado (Si - No)
  //G_TYPE_STRING); //Pagado (Si - No)

  tree = GTK_WIDGET (gtk_builder_get_object (builder, "treeview_cheques"));
  gtk_tree_view_set_model(GTK_TREE_VIEW (tree), GTK_TREE_MODEL (cheques));

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("ID Venta", renderer,
                                                     "text", 1,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_min_width (column, 100);
  gtk_tree_view_column_set_max_width (column, 100);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("F.Venta", renderer,
                                                     "text", 2,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_min_width (column, 100);
  gtk_tree_view_column_set_max_width (column, 100);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Monto Venta", renderer,
                                                     "text", 3,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_min_width (column, 100);
  gtk_tree_view_column_set_max_width (column, 100);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Cod. Cheque", renderer,
                                                     "text", 4,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_min_width (column, 100);
  gtk_tree_view_column_set_max_width (column, 100);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Monto Cheque", renderer,
                                                     "text", 5,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_min_width (column, 100);
  gtk_tree_view_column_set_max_width (column, 100);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Fecha Venc.", renderer,
                                                     "text", 6,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_min_width (column, 100);
  gtk_tree_view_column_set_max_width (column, 100);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Facturado", renderer,
                                                     "text", 7,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_min_width (column, 65);
  gtk_tree_view_column_set_max_width (column, 65);

  /* renderer = gtk_cell_renderer_text_new (); */
  /* column = gtk_tree_view_column_new_with_attributes ("Pagado", renderer, */
  /*                                                    "text", 8, */
  /*                                                    NULL); */
  /* gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column); */
  /* gtk_tree_view_column_set_alignment (column, 0.5); */
  /* g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL); */
  /* gtk_tree_view_column_set_resizable (column, FALSE); */
  /* gtk_tree_view_column_set_min_width (column, 65); */
  /* gtk_tree_view_column_set_max_width (column, 65); */

  /* We fill the struct to print the last two tree views */
  emisores_list->son->tree = GTK_TREE_VIEW (tree);
  emisores_list->son->cols[0].name = "ID Venta";
  emisores_list->son->cols[0].num = 1;
  emisores_list->son->cols[1].name = "Fecha Venta";
  emisores_list->son->cols[1].num = 2;
  emisores_list->son->cols[2].name = "Monto Venta";
  emisores_list->son->cols[2].num = 3;
  emisores_list->son->cols[3].name = "Codigo Cheque";
  emisores_list->son->cols[3].num = 4;
  emisores_list->son->cols[4].name = "Monto Cheque";
  emisores_list->son->cols[4].num = 5;
  emisores_list->son->cols[5].name = "Fecha Venc.";
  emisores_list->son->cols[5].num = 6;
  emisores_list->son->cols[6].name = "Facturado";
  emisores_list->son->cols[6].num = 7;
  //emisores_list->son->cols[7].name = "Pagado";
  //emisores_list->son->cols[7].num = 8;
  emisores_list->son->cols[7].name = NULL;

  g_signal_connect (G_OBJECT (builder_get (builder, "btn_print_emisores")), "clicked",
		    G_CALLBACK (PrintTwoTree), (gpointer)emisores_list);

  search_emisor ();
  //setup_print_menu();
}

void
AddClient (GtkWidget *widget, gpointer data)
{
  GtkWidget *wnd;
  GtkWidget *aux;

  wnd = gtk_widget_get_toplevel(widget);

  aux = GTK_WIDGET(gtk_builder_get_object(builder, "entry_client_rut"));
  gtk_widget_grab_focus (aux);

  aux = GTK_WIDGET(gtk_builder_get_object(builder, "wnd_addclient"));
  gtk_window_set_transient_for(GTK_WINDOW(aux), GTK_WINDOW(wnd));
  gtk_window_set_modal (GTK_WINDOW(aux), TRUE);
  gtk_widget_show_all (aux);
}


void
add_emisor (GtkWidget *widget, gpointer data)
{
  GtkWidget *wnd;
  GtkWidget *aux;
  wnd = gtk_widget_get_toplevel(widget);

  aux = GTK_WIDGET (gtk_builder_get_object (builder, "entry_srch_emisor"));
  gtk_widget_grab_focus (aux);

  aux = GTK_WIDGET (gtk_builder_get_object (builder, "wnd_add_check_manager"));
  clean_container (GTK_CONTAINER (aux));
  gtk_window_set_transient_for (GTK_WINDOW (aux), GTK_WINDOW (wnd));
  gtk_window_set_modal (GTK_WINDOW (aux), TRUE);
  gtk_widget_show_all (aux);
}


void
CloseClientWindow (GtkWindow *window, gpointer user_data)
{
  clean_container (GTK_CONTAINER (window));

  gtk_widget_hide (GTK_WIDGET (window));
}


GtkWidget *
caja_entrada (gchar *text, gint largo_maximo, gint ancho, GtkWidget *entry)
{
  GtkWidget *box;
  GtkWidget *etiqueta;

  box = gtk_vbox_new (TRUE, 2);
  gtk_widget_show (box);

  if (text != FALSE)
    {
      etiqueta = gtk_label_new (text);
      gtk_widget_show (etiqueta);

      gtk_box_pack_start (GTK_BOX (box), etiqueta, FALSE, FALSE, 0);
    }

  entry = gtk_entry_new_with_max_length (largo_maximo);
  gtk_widget_set_size_request (entry, ancho, -1);
  gtk_box_pack_start (GTK_BOX (box), entry, FALSE, FALSE, 0);

  gtk_widget_show (entry);

  return box;
}

void
AgregarClienteABD (GtkWidget *widget, gpointer data)
{
  gchar *nombre;
  gchar *paterno;
  gchar *materno;
  gchar *rut;
  gchar *ver;
  gchar *direccion;
  gchar *fono;
  gchar *giro;
  gint credito;
  GtkWidget *wnd;

  wnd = GTK_WIDGET(gtk_builder_get_object(builder, "wnd_addclient"));

  nombre = g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object(builder, "entry_client_name"))));
  paterno = g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object(builder, "entry_client_apell_p"))));
  materno = g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object(builder, "entry_client_apell_m"))));
  rut = g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object(builder, "entry_client_rut"))));
  ver = g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object(builder, "entry_client_dv"))));
  direccion = g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object(builder, "entry_client_addr"))));
  fono = g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object(builder, "entry_client_phone"))));
  giro = g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object(builder, "entry_client_giro"))));
  credito = atoi (g_strdup (gtk_entry_get_text (GTK_ENTRY(gtk_builder_get_object(builder, "entry_client_limit_credit")))));

  if (strcmp (nombre, "") == 0)
    AlertMSG (wnd, "El Campo Nombre no puede estar vacío");
  else if (strcmp (paterno, "") == 0)
    AlertMSG (wnd, "El Apellido Paterno no puede estar vacío");
  else if (strcmp (materno, "") == 0)
    AlertMSG (wnd, "El Apellido Materno no puede estar vacío");
  else if (strcmp (rut, "") == 0)
    AlertMSG (wnd, "El Campo rut no debe estar vacío");
  else if (strcmp (ver, "") == 0)
    AlertMSG (wnd, "El Campo verificador del rut no puede estar vacío");
  else if (strcmp (direccion, "") == 0)
    AlertMSG (wnd, "La dirección no puede estar vacía");
  else if (strcmp (fono, "") == 0)
    AlertMSG (wnd, "El campo telefónico no puede estar vacío");
  else if (strcmp (giro, "") == 0)
    AlertMSG (wnd, "El campo giro no puede estar vacío");
  else if (RutExist (rut) == TRUE)
    AlertMSG (wnd, "Ya existe alguien con el mismo rut");
  /*else if (credito == 0)
    AlertMSG (wnd, "El campo credito no puede estar vacío");*/
  else
    {
      if (VerificarRut (rut, ver) == TRUE)
        {
          if (!(InsertClient (nombre, paterno, materno, rut, ver, direccion, fono, credito, giro, client_type)))
            {
              ErrorMSG(wnd, "No fue posible agregar el cliente a la base de datos");
              return;
            }
          else
            CloseClientWindow (GTK_WINDOW (wnd), NULL);
        }
      else
        AlertMSG (wnd, "El Rut no es valido!!");
    }
}

gboolean
VerificarRut (gchar *rut, gchar *ver)
{
  gint multi[] = {2, 3, 4, 5, 6, 7, 2, 3};
  gint result[] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
  gint largo, suma = 0, i, resto, j = 0, o = 0;

  largo = (gint) strlen (rut);

  for (i = largo; i >= 0; i--)
    {
      result[o++] = multi[j++] * (rut[i-1] - 48);
    }

  for (i = 0; i < largo; i++)
    suma += result[i];

  resto = suma % 11;

  switch (resto)
    {
    case 1:
      if ((strcmp (ver, "K") == 0) || (strcmp (ver, "k") == 0))
        return TRUE;
      else
        return FALSE;
      break;
    case 0:
      if (resto == (atoi (ver)))
        return TRUE;
      else
        return FALSE;
      break;
    default:
      if ((11-resto) == (atoi (ver)))
        return TRUE;
      else
        return FALSE;
      break;
    }
}

gint
FillClientStore (GtkListStore *store)
{
  GtkTreeIter iter;

  PGresult *res;
  gint tuples, i;
  gchar *enable;
  res = EjecutarSQL ("SELECT rut, dv, nombre, apell_p, apell_m, telefono, credito_enable FROM select_cliente() "
		     "WHERE activo = 't' AND tipo = 'credito'");

  tuples = PQntuples (res);

  gtk_list_store_clear (store);

  for (i = 0; i < tuples; i++)
    {
      gtk_list_store_append (store, &iter);
      if (user_data->user_id == 1)
        {
          enable = PQvaluebycol (res, i, "credito_enable");
          gtk_list_store_set (store, &iter,
                              0, g_strconcat (PQvaluebycol (res, i, "rut"),
					      PQvaluebycol (res, i, "dv"), NULL),
                              1, g_strdup_printf ("%s %s %s", PQvaluebycol (res, i, "nombre"),
                                                  PQvaluebycol (res, i, "apell_p"), PQvaluebycol (res, i, "apell_m")),
                              2, atoi (PQvaluebycol (res, i, "telefono")),
                              3, strcmp (enable, "t") ? FALSE : TRUE,
                              4, PutPoints (g_strdup_printf ("%d",
							     ReturnClientCredit (atoi (PQvaluebycol (res, i, "rut"))))),
                              5, PutPoints (g_strdup_printf ("%d",
							     DeudaTotalCliente (atoi (PQvaluebycol(res, i, "rut"))))),
                              6, PutPoints (g_strdup_printf ("%d",
							     CreditoDisponible (atoi (PQvaluebycol(res, i, "rut"))))),
                              -1);
        }
      else
        gtk_list_store_set (store, &iter,
                            0, g_strconcat (PQvaluebycol (res, i, "rut"),
					    PQvaluebycol (res, i, "dv"), NULL),
                            1, g_strdup_printf ("%s %s %s", PQgetvalue (res, i, 1),
                                                PQgetvalue (res, i, 2), PQgetvalue (res, i, 3)),
                            2, atoi (PQgetvalue (res, i, 7)),
                            4, PutPoints (g_strdup_printf ("%d",
							   ReturnClientCredit (atoi (PQvaluebycol (res, i, "rut"))))),
                            5, PutPoints (g_strdup_printf ("%d",
							   DeudaTotalCliente (atoi (PQvaluebycol(res, i, "rut"))))),
                            6, PutPoints (g_strdup_printf ("%d",
							   CreditoDisponible (atoi (PQvaluebycol(res, i, "rut"))))),
                            -1);
    }
  return 0;
}


void
DatosDeudor (GtkTreeSelection *treeselection,
             gpointer user_data)
{
  GtkWidget *widget;
  GtkTreeIter iter;
  GtkTreeView *treeview;
  GtkListStore *store;
  gchar *rut;
  gint rut_n;

  if (gtk_tree_selection_get_selected (treeselection, NULL, &iter))
    {
      treeview = gtk_tree_selection_get_tree_view (treeselection);

      gtk_tree_model_get (gtk_tree_view_get_model(treeview), &iter,
                          0, &rut,
                          -1);

      rut_n = atoi (g_strndup (rut, strlen (rut)-1));
      /* deuda = DeudaTotalCliente (rut); */

      // Limpiando treeviews //TODO: crear funciones que simplifiquen limpiar treeviews
      widget = GTK_WIDGET(gtk_builder_get_object(builder, "treeview_sales"));
      store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(widget)));
      gtk_list_store_clear (store);

      widget = GTK_WIDGET(gtk_builder_get_object(builder, "treeview_sale_details"));
      store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(widget)));
      gtk_list_store_clear (store);

      FillVentasDeudas (rut_n);
    }
}

void
datos_deuda_factura_guia (GtkTreeSelection *treeselection,
			  gpointer user_data)
{
  GObject *object;
  GtkTreeIter iter;
  GtkTreeView *treeview;
  gchar *rut;
  gint rut_n;

  GtkWidget *combo;
  GtkTreeModel *model_combo;
  GtkTreeIter iter_combo;
  gint tipo_documento;
  gint active;

  // Combo-Box
  combo = GTK_WIDGET (gtk_builder_get_object(builder, "cmb_tipo_documento"));
  active = gtk_combo_box_get_active (GTK_COMBO_BOX (combo));

  /* Verifica si hay alguna opción seleccionada*/
  if (active == -1)
    tipo_documento = FACTURA;
  else
    {
      model_combo = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));
      gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combo), &iter_combo);

      gtk_tree_model_get (model_combo, &iter_combo,
                          0, &tipo_documento,
                          -1);
    }

  // Treeview
  if (gtk_tree_selection_get_selected (treeselection, NULL, &iter))
    {
      treeview = gtk_tree_selection_get_tree_view (treeselection);

      gtk_tree_model_get (gtk_tree_view_get_model(treeview), &iter,
                          0, &rut,
                          -1);

      rut_n = atoi (g_strndup (rut, strlen (rut)-1));
      /* deuda = DeudaTotalCliente (rut); */

      //Limpiando treeviews //TODO: crear funciones que simplifiquen limpiar treeviews
      treeview = GTK_TREE_VIEW (builder_get (builder, "treeview_guia_factura"));
      object = G_OBJECT (gtk_tree_view_get_model (treeview));
      gtk_tree_store_clear ((GtkTreeStore*)object);

      treeview = GTK_TREE_VIEW (builder_get (builder, "treeview_gf_sale_detail"));
      object = G_OBJECT (gtk_tree_view_get_model (treeview));
      gtk_list_store_clear (GTK_LIST_STORE (object));

      fill_deudas_facturas_guias (rut_n, tipo_documento);
    }
}


void
datos_cheques_restaurant (GtkTreeSelection *treeselection,
			  gpointer user_data)
{
  GtkWidget *widget;
  GtkTreeIter iter;
  GtkTreeView *treeview;
  GtkListStore *store;
  gint id;

  gint tuples, i;
  PGresult *res;
  gchar *q;

  if (gtk_tree_selection_get_selected (treeselection, NULL, &iter))
    {
      treeview = gtk_tree_selection_get_tree_view (treeselection);

      gtk_tree_model_get (gtk_tree_view_get_model(treeview), &iter,
                          0, &id,
                          -1);

      /* deuda = DeudaTotalCliente (rut); */
      widget = GTK_WIDGET (gtk_builder_get_object (builder, "treeview_cheques"));
      store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (widget)));
      gtk_list_store_clear (store);

      q = g_strdup_printf ("SELECT cr.id, cr.id_venta, cr.codigo, cr.facturado, cr.monto, "
			   "cr.pagado, v.monto AS monto_venta, "

			   "date_part ('year', v.fecha) AS fvta_year, "
			   "date_part ('month', v.fecha) AS fvta_month, "
			   "date_part ('day', v.fecha) AS fvta_day, "

			   "date_part ('year', fecha_vencimiento) AS fvto_year, "
			   "date_part ('month', fecha_vencimiento) AS fvto_month, "
			   "date_part ('day', fecha_vencimiento) AS fvto_day "

			   "FROM cheque_rest cr INNER JOIN venta v ON cr.id_venta = v.id "
			   "WHERE id_emisor = %d", id);
      res = EjecutarSQL (q);
      tuples = PQntuples (res);

      for (i = 0; i < tuples; i++)
	{
	  gtk_list_store_append (store, &iter);
	  gtk_list_store_set (store, &iter,
			      0, atoi (PQvaluebycol (res, i, "id")),
			      1, atoi (PQvaluebycol (res, i, "id_venta")),
			      2, g_strdup_printf ("%s-%s-%s",
						  PQvaluebycol (res, i, "fvta_day"),
						  PQvaluebycol (res, i, "fvta_month"),
						  PQvaluebycol (res, i, "fvta_year")),
			      3, PutPoints (PQvaluebycol (res, i, "monto_venta")),
			      4, PQvaluebycol (res, i, "codigo"),
			      5, PutPoints (PQvaluebycol (res, i, "monto")),
			      6, g_strdup_printf ("%s-%s-%s",
						  PQvaluebycol (res, i, "fvto_day"),
						  PQvaluebycol (res, i, "fvto_month"),
						  PQvaluebycol (res, i, "fvto_year")),
			      7, g_strdup_printf ("%s", g_str_equal (PQvaluebycol (res, i, "facturado"), "f") ? "NO" : "SI"),
			      //8, g_strdup_printf ("%s", g_str_equal (PQvaluebycol (res, i, "pagado"), "f") ? "NO" : "SI"),
			      -1);
	}
    }
}


void
FillVentasDeudas (gint rut)
{
  GtkWidget *widget;
  GtkListStore *store;
  gint i;
  gint tuples;
  GtkTreeIter iter;
  PGresult *res;
  gchar *monto_complementario, *tipo_complementario;
  gint tipo_comp;

  res = SearchDeudasCliente (rut);

  tuples = PQntuples (res);

  widget = GTK_WIDGET (gtk_builder_get_object(builder, "treeview_sales"));
  store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (widget)));
  gtk_list_store_clear (store);

  for (i = 0; i < tuples; i++)
    {
      if (atoi (PQvaluebycol (res, i, "tipo_venta")) == MIXTO)
	{
	  tipo_comp = atoi (PQvaluebycol (res, i, "tipo_complementario"));
	  if (tipo_comp == CASH)
	    tipo_complementario = g_strdup_printf ("EFECTIVO");
	  else if (tipo_comp == CREDITO)
	    tipo_complementario = g_strdup_printf ("CREDITO");
	  else if (tipo_comp == CHEQUE_RESTAURANT)
	    tipo_complementario = g_strdup_printf ("CHEQ. REST");
	  else
	    tipo_complementario = g_strdup_printf ("OTRO");
	}
      else
	tipo_complementario = g_strdup_printf ("--");

      if (!g_str_equal (PQvaluebycol (res, i, "monto_complementario"), "0"))
	monto_complementario = PutPoints (g_strdup (PQvaluebycol (res, i, "monto_complementario")));
      else
	monto_complementario = g_strdup_printf ("--");

      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
                          0, atoi (PQvaluebycol (res, i, "id")),
                          1, atoi (PQvaluebycol (res, i, "maquina")),
                          2, PQvaluebycol (res, i, "vendedor"),
                          3, g_strdup_printf ("%.2d/%.2d/%.4d %.2d:%.2d", atoi (PQvaluebycol (res, i, "day")),
					      atoi (PQvaluebycol (res, i, "month")), atoi (PQvaluebycol (res, i, "year")),
					      atoi (PQvaluebycol (res, i, "hour")), atoi (PQvaluebycol (res, i, "minute"))),
			  4, g_strdup_printf ("%s", (atoi (PQvaluebycol (res, i, "tipo_venta")) == MIXTO) ? "Mixto" : "Crédito"),
			  5, tipo_complementario,
			  6, monto_complementario,
                          7, PutPoints (PQvaluebycol (res, i, "monto")),
                          -1);
    }
}


void
fill_deudas_facturas_guias (gint rut, gint tipo_documento_n)
{
  GtkTreeView *treeview;
  GtkTreeModel *store;
  gint i, j;
  gint tuples;
  GtkTreeIter iter_hijo, iter_padre;
  PGresult *res;
  gchar *tipo_documento;

  if (tipo_documento_n == FACTURA)
    tipo_documento = g_strdup ("FACTURA");
  else if (tipo_documento_n == GUIA)
    tipo_documento = g_strdup ("GUIA");

  res = search_deudas_guias_facturas_cliente (rut, "", tipo_documento_n);
  tuples = PQntuples (res);

  treeview = GTK_TREE_VIEW (builder_get (builder, "treeview_guia_factura"));
  store = gtk_tree_view_get_model (treeview);
  gtk_tree_store_clear ((GtkTreeStore*)store);

  for (i = 0; i < tuples; i++)
    {
      if ((tipo_documento_n == FACTURA && atoi (PQvaluebycol (res, i, "tipo_documento")) == FACTURA) ||
	  tipo_documento_n == GUIA)
	{
	  gtk_tree_store_append ((GtkTreeStore*)store, &iter_padre, NULL);
	  gtk_tree_store_set ((GtkTreeStore*)store, &iter_padre,
			      0, atoi (PQvaluebycol (res, i, "id_documento")),
			      1, FALSE,
			      2, atoi (PQvaluebycol (res, i, "id_venta")),
			      3, atoi (PQvaluebycol (res, i, "maquina")),
			      4, PQvaluebycol (res, i, "vendedor"),
			      5, g_strdup_printf ("%.2d/%.2d/%.4d %.2d:%.2d", atoi (PQvaluebycol (res, i, "day")),
						  atoi (PQvaluebycol (res, i, "month")), atoi (PQvaluebycol (res, i, "year")),
						  atoi (PQvaluebycol (res, i, "hour")), atoi (PQvaluebycol (res, i, "minute"))),
			      6, tipo_documento,
			      7, atoi (g_strdup (PQvaluebycol (res, i, "monto"))),
			      -1);

	  if (tipo_documento_n == FACTURA && atoi (PQvaluebycol (res, i, "id_venta")) == 0)
	    {
	      for (j = 0; j < tuples; j++)
		{
		  if (atoi (PQvaluebycol (res, i, "id_documento")) == atoi (PQvaluebycol (res, j, "id_factura")))
		    {
		      gtk_tree_store_append ((GtkTreeStore*)store, &iter_hijo, &iter_padre);
		      gtk_tree_store_set ((GtkTreeStore*)store, &iter_hijo,
					  0, atoi (PQvaluebycol (res, j, "id_documento")),
					  1, FALSE,
					  2, atoi (PQvaluebycol (res, j, "id_venta")),
					  3, atoi (PQvaluebycol (res, j, "maquina")),
					  4, PQvaluebycol (res, j, "vendedor"),
					  5, g_strdup_printf ("%.2d/%.2d/%.4d %.2d:%.2d", atoi (PQvaluebycol (res, j, "day")),
							      atoi (PQvaluebycol (res, j, "month")), atoi (PQvaluebycol (res, j, "year")),
							      atoi (PQvaluebycol (res, j, "hour")), atoi (PQvaluebycol (res, j, "minute"))),
					  6, "GUIA",
					  7, atoi (g_strdup (PQvaluebycol (res, j, "monto"))),
					  -1);
		    }
		}
	    }


	}
    }
}



void
ChangeDetalle (GtkTreeSelection *treeselection, gpointer user_data)
{
  GtkWidget *widget;
  GtkTreeView *treeview;
  GtkListStore *store;
  GtkListStore *store_detalle;
  GtkTreeIter iter;
  gchar *q, *treeview_name;
  gchar *id_venta_char;
  gint id_venta, tuples;
  gint i;
  gboolean numerico;
  PGresult *res;
  void *data_get;

  treeview = gtk_tree_selection_get_tree_view(treeselection);
  store = GTK_LIST_STORE(gtk_tree_view_get_model(treeview));

  //De acuerdo al treeview desde donde se llama, rellenará el store correspondiente
  treeview_name = g_strdup (gtk_buildable_get_name (GTK_BUILDABLE (treeview)));

  if (g_str_equal (treeview_name, "treeview_sales")) {
    widget = GTK_WIDGET (gtk_builder_get_object(builder, "treeview_sale_details"));
    numerico = TRUE;
  } else {
    widget = GTK_WIDGET (gtk_builder_get_object(builder, "treeview_detalle_deudas"));
    numerico = FALSE;
  }

  if (gtk_tree_selection_get_selected (treeselection, NULL, &iter))
    {
      gtk_tree_model_get (GTK_TREE_MODEL (store), &iter,
                          0, (numerico) ? &data_get : &data_get,
                          -1);

      if (numerico)
	id_venta = (gint)data_get;
      else
	id_venta_char = (gchar *)data_get;

      if (numerico == FALSE && g_str_equal (id_venta_char, "--"))
	return;

      store_detalle = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(widget)));
      gtk_list_store_clear (store_detalle);

      q = g_strdup_printf ("SELECT producto.codigo_corto, producto.descripcion, producto.marca, venta_detalle.cantidad, venta_detalle.precio "
                           "FROM venta_detalle inner join producto on producto.barcode=venta_detalle.barcode "
                           "WHERE venta_detalle.id_venta=%d", (numerico) ? id_venta : atoi(id_venta_char));
      res = EjecutarSQL (q);
      g_free(q);

      tuples = PQntuples (res);

      for (i = 0; i < tuples; i++)
        {
          gtk_list_store_append (store_detalle, &iter);
          gtk_list_store_set (store_detalle, &iter,
                              0, PQgetvalue (res, i, 0),
                              1, PQgetvalue (res, i, 1),
                              2, PQgetvalue (res, i, 2),
                              3, strtod (PUT (PQgetvalue (res, i, 3)),(char **)NULL),
                              4, PutPoints (PQgetvalue (res, i, 4)),
                              5, PutPoints (g_strdup_printf("%ld", lround (strtod (PUT (PQgetvalue (res, i, 3)), (char **)NULL) *
									   strtod (PUT (PQgetvalue (res, i, 4)), (char **)NULL)))),
                              -1);
        }
    }
}


void
change_detalle_guia_factura (GtkTreeSelection *treeselection, gpointer user_data)
{
  GtkWidget *widget;
  GtkTreeView *treeview;
  GtkTreeStore *store;
  GtkListStore *store_detalle;
  GtkTreeIter iter;
  gchar *q, *tipo_documento;
  gint id_venta, id_documento, tuples;
  gint i;
  PGresult *res;

  treeview = gtk_tree_selection_get_tree_view(treeselection);
  store = GTK_TREE_STORE (gtk_tree_view_get_model(treeview));

  widget = GTK_WIDGET (builder_get (builder, "treeview_gf_sale_detail"));

  if (gtk_tree_selection_get_selected (treeselection, NULL, &iter))
    {
      gtk_tree_model_get (GTK_TREE_MODEL (store), &iter,
			  0, &id_documento,
                          2, &id_venta,
			  6, &tipo_documento,
                          -1);

      store_detalle = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (widget)));
      gtk_list_store_clear (store_detalle);

      if (id_venta == 0)
	q = g_strdup_printf ("SELECT producto.codigo_corto, producto.descripcion, producto.marca, venta_detalle.cantidad, venta_detalle.precio_neto, venta_detalle.precio "
			     "FROM venta_detalle INNER JOIN producto on producto.barcode=venta_detalle.barcode "
			     "WHERE venta_detalle.id_venta IN (SELECT id_venta FROM documentos_emitidos WHERE id_factura = %d)", id_documento);
      else
	q = g_strdup_printf ("SELECT producto.codigo_corto, producto.descripcion, producto.marca, venta_detalle.cantidad, venta_detalle.precio_neto, venta_detalle.precio "
			     "FROM venta_detalle INNER JOIN producto on producto.barcode=venta_detalle.barcode "
			     "WHERE venta_detalle.id_venta=%d", id_venta);

      res = EjecutarSQL (q);
      g_free(q);

      tuples = PQntuples (res);

      for (i = 0; i < tuples; i++)
        {
          gtk_list_store_append (store_detalle, &iter);
          gtk_list_store_set (store_detalle, &iter,
                              0, PQgetvalue (res, i, 0),
                              1, PQgetvalue (res, i, 1),
                              2, PQgetvalue (res, i, 2),
                              3, strtod (PUT (PQvaluebycol (res, i, "cantidad")), (char **)NULL),
			      4, strtod (PUT (PQvaluebycol (res, i, "precio_neto")), (char **)NULL),
			      5, strtod (PUT (PQvaluebycol (res, i, "precio")), (char **)NULL),
                              6, lround (strtod (PUT (PQvaluebycol (res, i, "precio")), (char **)NULL) *
					 strtod (PUT (PQvaluebycol (res, i, "cantidad")), (char **)NULL)),
                              -1);
        }
    }

  if (g_str_equal (tipo_documento, "FACTURA"))
    gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_pagar_factura")), TRUE);
  else if (g_str_equal (tipo_documento, "GUIA"))
    gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_pagar_factura")), FALSE);
}


/**
 *
 */
void
on_cmb_tipo_documento_changed (GtkComboBox *combo, gpointer user_data)
{
  GtkTreeView *tree;
  GtkTreeSelection *selection;
  GtkTreeModel *model_combo;
  GtkTreeIter iter_combo;
  gint active, tipo_documento;

  tree = GTK_TREE_VIEW (builder_get (builder, "treeview_clients_gf"));
  selection = gtk_tree_view_get_selection (tree);
  datos_deuda_factura_guia (selection, NULL);

  // Si es guia o factura
  active = gtk_combo_box_get_active (combo);

  /* Verifica si hay alguna opción seleccionada*/
  if (active == -1)
    gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_pagar_factura")), FALSE);
  else
    {
      model_combo = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));
      gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combo), &iter_combo);

      gtk_tree_model_get (model_combo, &iter_combo,
                          0, &tipo_documento,
                          -1);

      if (tipo_documento == FACTURA)
	{
	  if (get_treeview_length (GTK_TREE_VIEW (builder_get (builder, "treeview_guia_factura"))) > 0)
	    gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_pagar_factura")), TRUE);
	}
      else if (tipo_documento == GUIA)
	gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_pagar_factura")), FALSE);
    }
}


void
CloseAbonarWindow (void)
{
  GtkWidget *widget;

  widget = GTK_WIDGET (gtk_builder_get_object(builder, "wnd_abonar"));
  gtk_widget_hide(widget);

  widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_admin_abonar_amount"));
  gtk_entry_set_text(GTK_ENTRY(widget), "");
}


gint
AbonarWindow (void)
{
  GtkWidget *widget;
  GtkTreeIter iter;
  GtkTreeSelection *selection;
  GtkListStore *store;
  gchar *rut;
  gint rut_n;
  gint deuda;

  widget = GTK_WIDGET (gtk_builder_get_object(builder, "treeview_clients"));
  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(widget));

  if (!(gtk_tree_selection_get_selected (selection, NULL, &iter)))
    {
      widget = GTK_WIDGET (gtk_builder_get_object(builder, "statusbar"));
      statusbar_push (GTK_STATUSBAR(widget), "Debe seleccionar un cliente del listado para abonar", 3000);
      return 0;
    }
  else
    {
      store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(widget)));

      gtk_tree_model_get (GTK_TREE_MODEL(store), &iter,
                          0, &rut,
                          -1);
    }

  rut_n = atoi (g_strndup (rut, strlen (rut)-1));

  deuda = DeudaTotalCliente(rut_n);

  if (deuda == 0)
    {
      AlertMSG(widget, "No puede abonar a un cliente con deuda 0");
      return 0;
    }

  widget = GTK_WIDGET (gtk_builder_get_object(builder, "lbl_admin_abonar_rut"));
  gtk_label_set_text(GTK_LABEL(widget), g_strdup_printf("%s", formato_rut (rut)));

  widget = GTK_WIDGET (gtk_builder_get_object(builder, "lbl_admin_abonar_deuda"));
  gtk_label_set_text(GTK_LABEL(widget), g_strdup_printf("%d", deuda));

  widget = GTK_WIDGET (gtk_builder_get_object(builder, "wnd_abonar"));
  gtk_widget_show_all(widget);

  widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_admin_abonar_amount"));
  gtk_widget_grab_focus(widget);

  return 0;
}

void
on_entry_admin_abonar_amount_changed (GtkEditable *editable, gpointer user_data)
{
  GtkWidget *widget;
  gchar *rut;
  gint abono;
  gint deuda;

  widget = GTK_WIDGET (builder_get (builder, "lbl_admin_abonar_rut"));
  rut = g_strdup (gtk_label_get_text (GTK_LABEL (widget)));
  rut = CutPoints (g_strndup (rut, strlen (rut)-1));

  deuda = DeudaTotalCliente (atoi (rut));
  abono = atoi (gtk_entry_get_text (GTK_ENTRY(editable)));

  widget = GTK_WIDGET (gtk_builder_get_object(builder, "lbl_admin_abonar_deuda"));
  gtk_label_set_text(GTK_LABEL(widget), g_strdup_printf("%d", deuda-abono));

}

void
Abonar (void)
{
  GtkWidget *widget;
  GtkTreeIter iter;
  GtkTreeSelection *selection;
  GtkTreeModel *store;
  gint abonar;
  gchar *rut;
  gint rut_n;

  /*De estar habilitada caja, se asegura que ésta se encuentre
    abierta al momento de vender*/

  //TODO: Unificar esta comprobación en las funciones primarias
  //      encargadas de hacer cualquier movimiento de caja

  if (rizoma_get_value_boolean ("CAJA"))
    if (check_caja()) // Se abre la caja en caso de que está cerrada
      open_caja (TRUE);

  widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_admin_abonar_amount"));
  abonar = atoi (gtk_entry_get_text (GTK_ENTRY (widget)));

  if (abonar == 0)
    {
      AlertMSG(widget, "No puede abonar 0 pesos a una deuda");
      return;
    }

  widget = GTK_WIDGET (gtk_builder_get_object(builder, "treeview_clients"));
  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(widget));
  store = gtk_tree_view_get_model(GTK_TREE_VIEW(widget));

  if (gtk_tree_selection_get_selected (selection, NULL, &iter))
    {
      gtk_tree_model_get (store, &iter,
                          0, &rut,
                          -1);

      rut = g_strndup (rut, strlen (rut)-1);
      CloseAbonarWindow ();
      rut_n = atoi (rut);

      if (CancelarDeudas (abonar, rut_n) == 0)
        {
          gchar *msg;

          widget = GTK_WIDGET (gtk_builder_get_object(builder, "statusbar"));

          if (GetResto (rut_n) > 0)
            msg = g_strdup_printf("Se ha abonado con exito al cliente %d, actualmente tiene un monto de <b>%d</b> a favor",
                                  rut_n, GetResto (rut_n));
          else
            msg = g_strdup_printf("Se ha abonado con exito al cliente %d",
                                  rut_n);

          ExitoMSG(widget, msg);
          FillClientStore(GTK_LIST_STORE(store));

	  // Limpiando treeviews //TODO: crear funciones que simplifiquen limpiar treeviews
	  widget = GTK_WIDGET(gtk_builder_get_object(builder, "treeview_sales"));
	  store = GTK_TREE_MODEL (gtk_tree_view_get_model(GTK_TREE_VIEW(widget)));
	  gtk_list_store_clear (GTK_LIST_STORE(store));

	  widget = GTK_WIDGET(gtk_builder_get_object(builder, "treeview_sale_details"));
	  store = GTK_TREE_MODEL(gtk_tree_view_get_model(GTK_TREE_VIEW(widget)));
	  gtk_list_store_clear (GTK_LIST_STORE(store));
        }
      else
        ErrorMSG(widget, "No se pudo abonar el monto a la deuda");
    }
}

void
CloseModificarWindow (void)
{
  GtkWidget *widget;
  int i;
  gchar *widgets_list[7] = {"entry_modclient_name",
                            "entry_modclient_apell_p",
                            "entry_modclient_apell_m",
                            "entry_modclient_addr",
                            "entry_modclient_phone",
                            "entry_modclient_giro",
                            "entry_modclient_limit_credit"};

  for (i=0 ; i<7 ; i++)
    {
      widget = GTK_WIDGET (gtk_builder_get_object(builder, widgets_list[i]));
      gtk_entry_set_text(GTK_ENTRY(widget), "");
    }

  widget = GTK_WIDGET (gtk_builder_get_object(builder, "wnd_modclient"));
  gtk_widget_hide(widget);
}

void
ModificarCliente (void)
{
  GtkWidget *widget;
  GtkTreeView *treeview;
  GtkTreeIter iter;
  GtkTreeSelection *selection;
  GtkListStore *store;

  PGresult *res;
  gchar *q;
  gchar *nombre;
  gchar *apellido_materno;
  gchar *apellido_paterno;
  gchar *fono;
  gchar *direccion;
  gchar *credito;
  gchar *giro; //*rut_ver
  gchar *rut;
  gchar *rut_split;
  gint rut_n;

  treeview = GTK_TREE_VIEW(gtk_builder_get_object(builder, "treeview_clients"));
  selection = gtk_tree_view_get_selection(treeview);
  store = GTK_LIST_STORE(gtk_tree_view_get_model(treeview));

  if (gtk_tree_selection_get_selected (selection, NULL, &iter))
    {
      gtk_tree_model_get (GTK_TREE_MODEL (store), &iter,
                          0, &rut,
                          -1);

      rut_split = g_strndup (rut, strlen (rut)-1);
      rut_n = atoi (rut_split);

      q = g_strdup_printf ("select nombre, apell_p, apell_m, telefono, direccion, credito, dv, giro "
                           "FROM cliente where rut=%d", rut_n);
      res = EjecutarSQL(q);
      g_free(q);

      nombre = PQvaluebycol(res, 0, "nombre");
      apellido_materno = PQvaluebycol(res, 0, "apell_m");
      apellido_paterno = PQvaluebycol(res, 0, "apell_p");
      fono = PQvaluebycol(res, 0, "telefono");
      direccion = PQvaluebycol(res, 0, "direccion");
      credito = PQvaluebycol(res, 0, "credito");
      //rut_ver = PQvaluebycol(res, 0, "dv");
      giro = PQvaluebycol(res, 0, "giro");

      widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_modclient_name"));
      gtk_entry_set_text (GTK_ENTRY (widget), nombre);

      widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_modclient_apell_p"));
      gtk_entry_set_text (GTK_ENTRY (widget), apellido_paterno);

      widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_modclient_apell_m"));
      gtk_entry_set_text (GTK_ENTRY (widget), apellido_materno);

      widget = GTK_WIDGET (gtk_builder_get_object(builder, "lbl_modclient_rut"));
      //gtk_label_set_text (GTK_LABEL (widget), g_strdup_printf ("%d-%s", rut_n, rut_ver));
      gtk_label_set_text (GTK_LABEL (widget), formato_rut (rut));

      widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_modclient_addr"));
      gtk_entry_set_text (GTK_ENTRY (widget), direccion);

      widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_modclient_phone"));
      gtk_entry_set_text (GTK_ENTRY (widget), fono);

      widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_modclient_giro"));
      gtk_entry_set_text (GTK_ENTRY (widget), giro);

      widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_modclient_limit_credit"));
      gtk_entry_set_text (GTK_ENTRY (widget), credito);

      widget = GTK_WIDGET (gtk_builder_get_object(builder, "wnd_modclient"));
      gtk_window_set_transient_for(GTK_WINDOW(widget), GTK_WINDOW(gtk_builder_get_object(builder, "wnd_admin")));
      gtk_widget_show_all(widget);
    }
}

/**
 * Checks if it's possible sale a given amount to a given client,
 * based in the credit available.
 *
 * @param rut the rut of the client
 * @param total_venta the amount requested
 *
 * @return TRUE if the client has enough credit, otherwise returns FALSE
 */
gboolean
VentaPosible (gint rut, gint total_venta)
{
  gint saldo = CreditoDisponible (rut);

  if (total_venta <= saldo)
    return TRUE;
  else
    return FALSE;
}

/**
 * Callback connected to the toggle button of the clients treeview
 *
 * This function enable or disable the credit state of the client selected.
 * @param toggle the togle button
 * @param path_str the path of the selected row
 * @param data the user data
 *
 * @return 0 on succesfull execution
 */
gint
ToggleClientCredit (GtkCellRendererToggle *toggle, char *path_str, gpointer data)
{
  GtkWidget *treeview;
  GtkTreeModel *store;
  gboolean enable;
  gchar *rut;
  gint rut_n;
  GtkTreePath *path;
  GtkTreeIter iter;

  treeview = GTK_WIDGET (gtk_builder_get_object(builder, "treeview_clients"));
  store = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
  path = gtk_tree_path_new_from_string (path_str);

  gtk_tree_model_get_iter (store, &iter, path);
  gtk_tree_model_get (store, &iter,
                      0, &rut,
                      3, &enable,
                      -1);

  rut = g_strndup (rut, strlen (rut)-1);
  enable = !(enable);
  rut_n = atoi (rut);

  ChangeEnableCredit (enable, rut_n);

  gtk_list_store_set (GTK_LIST_STORE(store), &iter,
                      3, enable,
                      -1);

  gtk_tree_path_free (path);

  return 0;
}

/**
 * Callback connected to the delele client button present in the
 * clients tab.
 *
 * This function deletes the client selected in the treeview
 */
void
EliminarCliente (void)
{
  GtkWidget *widget;
  GtkTreeView *treeview;
  GtkListStore *store;
  GtkTreeSelection *selection;
  gchar *rut;
  gint rut_n;
  GtkTreeIter iter;

  treeview = GTK_TREE_VIEW(gtk_builder_get_object(builder, "treeview_clients"));
  store = GTK_LIST_STORE(gtk_tree_view_get_model(treeview));
  selection = gtk_tree_view_get_selection(treeview);

  if (gtk_tree_selection_get_selected (selection, NULL, &iter) && user_data->user_id == 1)
    {
      gtk_tree_model_get (GTK_TREE_MODEL (store), &iter,
                          0, &rut,
                          -1);

      rut = g_strndup (rut, strlen (rut)-1);
      rut_n = atoi (rut);

      if (ClientDelete (rut_n))
        {
          gtk_list_store_remove(store, &iter);
          widget = GTK_WIDGET (gtk_builder_get_object(builder, "statusbar"));
          statusbar_push (GTK_STATUSBAR(widget), "El cliente fue eliminado con exito", 3000);
        }
      else
        ErrorMSG (GTK_WIDGET (treeview), "No se pudo elminar el cliente, compruebe que no tenga saldo deudor");
    }
}

/**
 * Callback connected to the delele an emisor button present in the
 * emisor tab.
 *
 * This function deletes the emisor selected in the treeview
 */
void
eliminar_emisor (void)
{
  GtkWidget *widget;
  GtkTreeView *treeview;
  GtkListStore *store;
  GtkTreeSelection *selection;
  gint id;
  GtkTreeIter iter;

  treeview = GTK_TREE_VIEW (gtk_builder_get_object (builder, "treeview_emisores"));
  store = GTK_LIST_STORE (gtk_tree_view_get_model (treeview));
  selection = gtk_tree_view_get_selection (treeview);

  if (gtk_tree_selection_get_selected (selection, NULL, &iter) && user_data->user_id == 1)
    {
      gtk_tree_model_get (GTK_TREE_MODEL (store), &iter,
                          0, &id,
                          -1);

      if (emisor_delete (id))
        {
          gtk_list_store_remove(store, &iter);
          widget = GTK_WIDGET (gtk_builder_get_object (builder, "statusbar"));
          statusbar_push (GTK_STATUSBAR (widget), "El emisor de cheques fue eliminado con exito", 3000);
        }
      else
        ErrorMSG (GTK_WIDGET (treeview), "No se pudo elminar el emisor de cheques, compruebe que no tenga cheques por cobrar");
    }
}


/**
 * Callback connected to "facturar" button on the
 * emisor tab.
 *
 * This function change the status "facturar" on "cheque_rest" table
 */
void
facturar_cheque_restaurant (void)
{
  GtkWidget *widget;
  GtkTreeView *treeview;
  GtkListStore *store;
  GtkTreeSelection *selection;
  gint id;
  gchar *codigo;
  GtkTreeIter iter;

  treeview = GTK_TREE_VIEW (gtk_builder_get_object (builder, "treeview_cheques"));
  store = GTK_LIST_STORE (gtk_tree_view_get_model (treeview));
  selection = gtk_tree_view_get_selection (treeview);

  if (gtk_tree_selection_get_selected (selection, NULL, &iter) && user_data->user_id == 1)
    {
      gtk_tree_model_get (GTK_TREE_MODEL (store), &iter,
                          0, &id,
			  4, &codigo,
                          -1);

      if (fact_cheque_rest (id))
        {
	  //Actualiza la info
	  //search_emisor ();
	  treeview = GTK_TREE_VIEW (gtk_builder_get_object (builder, "treeview_emisores"));
	  store = GTK_LIST_STORE (gtk_tree_view_get_model (treeview));
	  selection = gtk_tree_view_get_selection (treeview);
	  datos_cheques_restaurant (selection, NULL);

          widget = GTK_WIDGET (gtk_builder_get_object (builder, "statusbar"));
          statusbar_push (GTK_STATUSBAR (widget), g_strdup_printf ("El cheque de restaurant %s ha sido facturado", codigo), 3000);
        }
      else
        ErrorMSG (GTK_WIDGET (treeview), g_strdup_printf ("El cheque de restaurant %s ya estaba facturado", codigo));
    }
}


/**
 * Callback connected to the accept button present in the modificate
 * client dialog.
 *
 * This function validates the information entered by the user
 */void
 ModificarClienteDB (void)
 {
   GtkWidget *widget;
   gchar *q;
   gchar *nombre;
   gchar *paterno;
   gchar *materno;
   gchar **rut;
   gchar *direccion;
   gchar *fono;
   gchar *giro;
   gint credito;

   widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_modclient_name"));
   nombre = g_strdup (gtk_entry_get_text (GTK_ENTRY (widget)));

   widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_modclient_apell_p"));
   paterno = g_strdup (gtk_entry_get_text (GTK_ENTRY (widget)));

   widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_modclient_apell_m"));
   materno = g_strdup (gtk_entry_get_text (GTK_ENTRY (widget)));

   widget = GTK_WIDGET (gtk_builder_get_object(builder, "lbl_modclient_rut"));
   rut = g_strsplit (gtk_label_get_text (GTK_LABEL (widget)), "-", 0);
   rut[0] = CutPoints (rut[0]);

   widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_modclient_addr"));
   direccion = g_strdup (gtk_entry_get_text (GTK_ENTRY (widget)));

   widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_modclient_phone"));
   fono = g_strdup (gtk_entry_get_text (GTK_ENTRY (widget)));

   widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_modclient_giro"));
   giro = g_strdup (gtk_entry_get_text (GTK_ENTRY (widget)));

   widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_modclient_limit_credit"));
   credito = atoi (g_strdup (gtk_entry_get_text (GTK_ENTRY (widget))));

   if (strcmp (nombre, "") == 0)
     AlertMSG (widget, "El Campo Nombre no puede estar vacío");
   else if (strcmp (paterno, "") == 0)
     AlertMSG (widget, "El Apellido Paterno no puede estar vacío");
   else if (strcmp (materno, "") == 0)
     AlertMSG (widget, "El Apellido Materno no puede estar vacío");
   else if (strcmp (rut[0], "") == 0)
     AlertMSG (widget, "El Campo rut no debe estar vacío");
   else if (strcmp (rut[1], "") == 0)
     AlertMSG (widget, "El Campo verificador del rut no puede estar vacío");
   else if (strcmp (direccion, "") == 0)
     AlertMSG (widget, "La direccion no puede estar vacía");
   else if (strcmp (fono, "") == 0)
     AlertMSG (widget, "El campo telefonico no puede estar vacío");
   else if (strcmp (giro, "") == 0)
     AlertMSG (widget, "El campo giro no puede estar vacío");
   /*else if (credito == 0)
     AlertMSG (widget, "El campo credito no puede estar vacío");*/
   else
     {
       if (VerificarRut (rut[0], rut[1]))
         {
           q = g_strdup_printf ("UPDATE cliente SET nombre='%s', apell_p='%s', apell_m='%s', "
                                "direccion='%s', telefono='%s', credito=%d, giro='%s' "
				"WHERE rut=%d",
                                nombre, paterno, materno, direccion, fono, credito, giro, atoi (rut[0]));

           if (EjecutarSQL (q) != NULL)
             {
               widget = GTK_WIDGET (gtk_builder_get_object(builder, "statusbar"));
               statusbar_push (GTK_STATUSBAR(widget), "Se modificaron los datos con exito", 3000);
             }
           else
             ErrorMSG (widget, "No se puedieron modificar los datos");

           CloseModificarWindow ();
           widget = GTK_WIDGET (gtk_builder_get_object(builder, "treeview_clients"));
           FillClientStore (GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(widget))));
           g_free(q);
         }
       else
         AlertMSG (widget, "El Rut no es valido!!");
     }
 }

/**
 * Obtains the credit limit of a given client
 *
 * @param rut the rut of the client with the format '12345678-9'
 *
 * @return the credit limit, if fails returns -1
 */
gint
LimiteCredito (const gchar *rut)
{
  PGresult *res;
  res = EjecutarSQL (g_strdup_printf ("SELECT credito FROM cliente WHERE rut=%s", strtok(g_strdup(rut),"-")));

  if (res != NULL)
    return atoi(PQgetvalue (res, 0, 0));
  else
    return -1;
}

/**
 * Callback connected to the print list of clients option
 *
 */
void
on_print_client_list_clicked()
{
  PrintTree(NULL, (gpointer) client_list);
}

/**
 * Callback connected to the print sales of a client option
 *
 */
void
on_print_sales_of_client_clicked()
{
  PrintTwoTree(NULL, (gpointer) client_detail);
}


/**
 * Setup the popup menu that must appear when user clicks in the print
 * button present in the clients tab
 *
 */
void
setup_print_menu()
{
  GError *error = NULL;
  GtkWidget *widget;
  GtkUIManager *manager;
  GtkAccelGroup *accel_group;
  GtkActionGroup *action_group;
  GtkActionEntry entry[] =
    {
      {"PrintClients", NULL, "Lista de clientes", NULL, NULL, on_print_client_list_clicked},
      {"PrintSalesOfClient", NULL, "Ventas de cliente", NULL, NULL, on_print_sales_of_client_clicked}
    };
  manager = gtk_ui_manager_new();
  accel_group = gtk_ui_manager_get_accel_group (manager);

  widget = GTK_WIDGET (gtk_builder_get_object(builder, "wnd_admin"));
  gtk_window_add_accel_group(GTK_WINDOW(widget), accel_group);

  action_group = gtk_action_group_new ("my print menu");

  gtk_action_group_add_actions (action_group, entry, 2, NULL);

  gtk_ui_manager_insert_action_group (manager, action_group, 0);
  gtk_ui_manager_add_ui_from_file (manager, DATADIR"/ui/print-menu.xml", &error);
  if (error != NULL)
    g_print("%s: %s\n", G_STRFUNC, error->message);

  widget = GTK_WIDGET (gtk_builder_get_object(builder, "btn_admin_print"));
  g_object_set_data (G_OBJECT(widget), "uimanager", (gpointer) manager);
}

void
on_btn_admin_print_clicked(GtkButton *button, gpointer user_data)
{
  GtkUIManager *uimanager;
  GtkWidget *widget;

  uimanager = GTK_UI_MANAGER (g_object_get_data (G_OBJECT (button), "uimanager"));

  widget = gtk_ui_manager_get_widget (uimanager, "/popup");

  gtk_menu_popup (GTK_MENU (widget), NULL, NULL, NULL, NULL, 1, gtk_get_current_event_time ());

  gtk_widget_show_all (widget);
}

void
admin_search_client (GtkButton *button)
{
  GtkWidget *widget;
  GtkWidget *aux_widget;
  GtkListStore *store;
  GtkTreeIter iter;
  gchar *string;
  gchar *q;
  gchar *enable;
  gint i;
  gint tuples;
  PGresult *res;

  widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_search_client"));
  string = g_strdup (gtk_entry_get_text (GTK_ENTRY(widget)));

  //ANTES: SELECT rut, nombre || ' ' || apell_p || ' ' || apell_m, telefono, credito_enable
  q = g_strdup_printf ("SELECT rut, dv, nombre || ' ' || apell_p || ' ' || apell_m AS name, telefono, credito_enable "
                       "FROM cliente WHERE activo = 't' AND (lower(nombre) LIKE lower('%s%%') OR "
                       "lower(apell_p) LIKE lower('%s%%') OR lower(apell_m) LIKE lower('%s%%') OR "
                       "rut::varchar like ('%s%%')) AND tipo = 'credito'",
                       string, string, string, string);
  res = EjecutarSQL (q);
  g_free (q);

  tuples = PQntuples (res);

  // Limpiando treeviews //TODO: crear funciones que simplifiquen limpiar treeviews
  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "treeview_sales"));
  store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(aux_widget)));
  gtk_list_store_clear (store);

  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "treeview_sale_details"));
  store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(aux_widget)));
  gtk_list_store_clear (store);

  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "treeview_clients"));
  store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(aux_widget)));
  gtk_list_store_clear (store);

  for (i = 0; i < tuples; i++)
    {
      gtk_list_store_append (store, &iter);

      if (user_data->user_id == 1)
        {
          enable = PQvaluebycol (res, i, "credito_enable");
          gtk_list_store_set (store, &iter,
                              0, g_strconcat (PQvaluebycol (res, i, "rut"),
					      PQvaluebycol (res, i, "dv"), NULL),
                              1, g_strdup (PQvaluebycol (res, i, "name")),
                              2, atoi (PQvaluebycol (res, i, "telefono")),
                              3, strcmp (enable, "t") ? FALSE : TRUE,
                              4, PutPoints (g_strdup_printf ("%d",
							     ReturnClientCredit (atoi (PQvaluebycol (res, i, "rut"))))),
			      5, PutPoints (g_strdup_printf ("%d",
							     DeudaTotalCliente (atoi(PQvaluebycol(res, i, "rut"))))),
                              6, PutPoints (g_strdup_printf ("%d",
							     CreditoDisponible (atoi(PQvaluebycol(res, i, "rut"))))),
                              -1);
        }
      else
        gtk_list_store_set (store, &iter,
                            0, g_strconcat (PQvaluebycol (res, i, "rut"),
					    PQvaluebycol (res, i, "dv"), NULL),
                            1, g_strdup (PQvaluebycol (res, i, "name")),
                            2, atoi (PQvaluebycol (res, i, "telefono")),
                            4, PutPoints (g_strdup_printf ("%d",
							   ReturnClientCredit (atoi (PQvaluebycol (res, i, "rut"))))),
			    5, PutPoints (g_strdup_printf ("%d",
							   DeudaTotalCliente (atoi(PQvaluebycol(res, i, "rut"))))),
			    6, PutPoints (g_strdup_printf ("%d",
							   CreditoDisponible (atoi(PQvaluebycol(res, i, "rut"))))),
                            -1);
    }
}


void
admin_search_client_gf (GtkButton *button)
{
  GtkWidget *widget;
  GObject *store;
  GtkTreeIter iter;
  gchar *string;
  gchar *q;
  gint i;
  gint tuples;
  PGresult *res;

  widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_search_client"));
  string = g_strdup (gtk_entry_get_text (GTK_ENTRY(widget)));

  //ANTES: SELECT rut, nombre || ' ' || apell_p || ' ' || apell_m, telefono, credito_enable
  q = g_strdup_printf ("SELECT rut, dv, nombre || ' ' || apell_p || ' ' || apell_m AS name, telefono, credito_enable "
                       "FROM cliente WHERE activo = 't' AND (lower(nombre) LIKE lower('%s%%') OR "
                       "lower(apell_p) LIKE lower('%s%%') OR lower(apell_m) LIKE lower('%s%%') OR "
                       "rut::varchar like ('%s%%')) AND tipo = 'factura'",
                       string, string, string, string);
  res = EjecutarSQL (q);
  g_free (q);

  tuples = PQntuples (res);

  // Limpiando treeviews //TODO: crear funciones que simplifiquen limpiar treeviews
  widget = GTK_WIDGET (builder_get (builder, "treeview_guia_factura"));
  store = G_OBJECT (gtk_tree_view_get_model (GTK_TREE_VIEW (widget)));
  gtk_tree_store_clear (GTK_TREE_STORE (store));

  widget = GTK_WIDGET (builder_get (builder, "treeview_gf_sale_detail"));
  store = G_OBJECT (gtk_tree_view_get_model (GTK_TREE_VIEW (widget)));
  gtk_list_store_clear (GTK_LIST_STORE (store));

  widget = GTK_WIDGET (builder_get (builder, "treeview_clients_gf"));
  store = G_OBJECT (gtk_tree_view_get_model (GTK_TREE_VIEW (widget)));
  gtk_list_store_clear (GTK_LIST_STORE (store));

  for (i = 0; i < tuples; i++)
    {
      gtk_list_store_append (GTK_LIST_STORE (store), &iter);

      gtk_list_store_set (GTK_LIST_STORE (store), &iter,
			  0, g_strconcat (PQvaluebycol (res, i, "rut"),
					  PQvaluebycol (res, i, "dv"), NULL),
			  1, g_strdup (PQvaluebycol (res, i, "name")),
			  2, atoi (PQvaluebycol (res, i, "telefono")),
			  3, PutPoints (g_strdup_printf ("%d",
							 ReturnClientCredit (atoi (PQvaluebycol (res, i, "rut"))))),
			  4, PutPoints (g_strdup_printf ("%d",
							 DeudaTotalCliente (atoi(PQvaluebycol(res, i, "rut"))))),
			  5, PutPoints (g_strdup_printf ("%d",
							 CreditoDisponible (atoi(PQvaluebycol(res, i, "rut"))))),
			  -1);
    }
}


/**
 * This function search the emisor filtering according to the search criteria
 * on the "entry_search_emisores" entry. (activate-signal)
 *
 *
 */
void
search_emisor (void)
{
  GtkWidget *widget;
  GtkWidget *aux_widget;
  GtkListStore *store;
  GtkTreeIter iter;
  gchar *string;
  gchar *q;
  gint i;
  gint tuples;
  PGresult *res;

  widget = GTK_WIDGET (gtk_builder_get_object (builder, "entry_search_emisores"));
  string = g_strdup (gtk_entry_get_text (GTK_ENTRY(widget)));

  q = g_strdup_printf ("SELECT id, rut, dv, razon_social, telefono, direccion, comuna, ciudad, giro "
                       "FROM emisor_cheque WHERE activo = 't' AND (lower(razon_social) LIKE lower('%s%%') OR "
                       "rut::varchar like ('%s%%')) ",
                       string, string);
  res = EjecutarSQL (q);
  g_free (q);
  tuples = PQntuples (res);

  // Limpiando treeviews //TODO: crear funciones que simplifiquen limpiar treeviews
  aux_widget = GTK_WIDGET (gtk_builder_get_object (builder, "treeview_cheques"));
  store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (aux_widget)));
  gtk_list_store_clear (store);

  aux_widget = GTK_WIDGET (gtk_builder_get_object (builder, "treeview_emisores"));
  store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (aux_widget)));
  gtk_list_store_clear (store);

  for (i = 0; i < tuples; i++)
    {
      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
			  0, atoi (PQvaluebycol (res, i, "id")),
			  1, g_strconcat (PQvaluebycol (res, i, "rut"),
					  PQvaluebycol (res, i, "dv"), NULL),
			  2, PQvaluebycol (res, i, "razon_social"),
			  3, PQvaluebycol (res, i, "telefono"),
			  4, PQvaluebycol (res, i, "direccion"),
			  -1);
    }
}

/**
 * This function shows the 'wnd_add_check_manager' window
 * to add a check manager (an "emisor"). (signal-clicked)
 *
 * @param: GtkButton *button
 * @param: gpointer user_data
 */
void
on_btn_add_emisores_clicked (GtkButton *button, gpointer user_data)
{
  gtk_widget_show_all (GTK_WIDGET (builder_get (builder, "wnd_add_check_manager")));
  clean_container (GTK_CONTAINER (builder_get (builder, "wnd_add_check_manager")));
  gtk_widget_grab_focus (GTK_WIDGET (builder_get (builder, "entry_cm_rut")));
}


/**
 * This function add the check manager (an "emisor")
 * (signal-clicked)
 *
 * @param: GtkButton *button
 * @param: gpointer user_data
 */
void
on_btn_add_cm_clicked (GtkButton *button, gpointer user_data)
{
  gchar *rut, *dv, *rs, *tel, *dir, *comuna, *ciudad, *giro;
  GtkEntry *rut_w, *dv_w, *rs_w, *tel_w, *dir_w, *comuna_w, *ciudad_w, *giro_w;
  GtkWidget *wnd;

  wnd = GTK_WIDGET(gtk_builder_get_object(builder, "wnd_add_check_manager"));

  rut_w = GTK_ENTRY (gtk_builder_get_object(builder, "entry_cm_rut"));
  dv_w = GTK_ENTRY (gtk_builder_get_object(builder, "entry_cm_dv"));
  rs_w = GTK_ENTRY (gtk_builder_get_object(builder, "entry_cm_rs"));
  tel_w = GTK_ENTRY (gtk_builder_get_object(builder, "entry_cm_tel"));
  dir_w = GTK_ENTRY (gtk_builder_get_object(builder, "entry_cm_dir"));
  comuna_w = GTK_ENTRY (gtk_builder_get_object(builder, "entry_cm_comuna"));
  ciudad_w = GTK_ENTRY (gtk_builder_get_object(builder, "entry_cm_ciudad"));
  giro_w = GTK_ENTRY (gtk_builder_get_object(builder, "entry_cm_giro"));

  rut = g_strdup (gtk_entry_get_text (rut_w));
  dv = g_strdup (gtk_entry_get_text (dv_w));
  rs = g_strdup (gtk_entry_get_text (rs_w));
  tel = g_strdup (gtk_entry_get_text (tel_w));
  dir = g_strdup (gtk_entry_get_text (dir_w));
  comuna = g_strdup (gtk_entry_get_text (comuna_w));
  ciudad = g_strdup (gtk_entry_get_text (ciudad_w));
  giro = g_strdup (gtk_entry_get_text (giro_w));

  if (strcmp (rut, "") == 0)
    AlertMSG (GTK_WIDGET(rut_w), "Debe ingresar un rut válido");
  else if (strcmp (dv, "") == 0)
    AlertMSG (GTK_WIDGET(dv_w), "Debe ingresar un dígito verificador");
  else if (strcmp (rs, "") == 0)
    AlertMSG (GTK_WIDGET(rs_w), "Debe ingresar una razón social");
  else if (strcmp (tel, "") == 0)
    AlertMSG (GTK_WIDGET(tel_w), "Debe ingresar un telefono");
  else if (!is_numeric (tel))
    AlertMSG (GTK_WIDGET(tel_w), "Telefono debe ser un valor numérico");
  else if (strcmp (dir, "") == 0)
    AlertMSG (GTK_WIDGET(dir_w), "Debe ingresar una dirección");
  else if (strcmp (comuna, "") == 0)
    AlertMSG (GTK_WIDGET(comuna_w), "Debe ingresar una comuna");
  else if (strcmp (ciudad, "") == 0)
    AlertMSG (GTK_WIDGET(ciudad_w), "Debe ingresar una ciudad");
  else if (strcmp (giro, "") == 0)
    AlertMSG (GTK_WIDGET(giro_w), "Debe especificar el giro del emisor");
  else
    {
      if (VerificarRut (rut, dv) == TRUE)
        {
          if (!(insert_emisores (rut, dv, rs, tel, dir, comuna, ciudad, giro)))
            {
              ErrorMSG (wnd, "No fue posible agregar el emisor a la base de datos");
              return;
            }
          else
	    gtk_widget_hide (GTK_WIDGET(gtk_builder_get_object(builder, "wnd_add_check_manager")));

	  search_emisor ();
        }
      else
        AlertMSG (GTK_WIDGET(rut_w), "El Rut no es valido!!");
    }
}


/**
 * This function modify the check manager (an "emisor")
 * (signal-clicked)
 *
 * @param: GtkButton *button
 * @param: gpointer user_data
 */
void
on_btn_edit_emisores_clicked (GtkButton *button, gpointer user_data)
{
  GtkWidget *widget;
  GtkTreeView *treeview;
  GtkTreeIter iter;
  GtkTreeSelection *selection;
  GtkListStore *store;

  PGresult *res;
  gchar *q;
  gint id;
  gchar *rut, *dv, *rs, *tel, *dir, *comuna, *ciudad, *giro;

  treeview = GTK_TREE_VIEW (gtk_builder_get_object(builder, "treeview_emisores"));
  selection = gtk_tree_view_get_selection (treeview);
  store = GTK_LIST_STORE (gtk_tree_view_get_model(treeview));

  if (gtk_tree_selection_get_selected (selection, NULL, &iter))
    {
      gtk_tree_model_get (GTK_TREE_MODEL (store), &iter,
                          0, &id,
                          -1);

      q = g_strdup_printf ("SELECT rut, dv, razon_social, telefono, direccion, comuna, ciudad, giro "
                           "FROM emisor_cheque WHERE id=%d", id);
      res = EjecutarSQL(q);
      g_free(q);

      rut = PQvaluebycol (res, 0, "rut");
      dv = PQvaluebycol (res, 0, "dv");
      rs = PQvaluebycol (res, 0, "razon_social");
      tel = PQvaluebycol (res, 0, "telefono");
      dir = PQvaluebycol (res, 0, "direccion");
      comuna = PQvaluebycol (res, 0, "comuna");
      ciudad = PQvaluebycol (res, 0, "ciudad");
      giro = PQvaluebycol (res, 0, "giro");

      widget = GTK_WIDGET (gtk_builder_get_object (builder, "entry_mod_cm_rut"));
      gtk_entry_set_text (GTK_ENTRY (widget), rut);

      widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_mod_cm_dv"));
      gtk_entry_set_text (GTK_ENTRY (widget), dv);

      widget = GTK_WIDGET (gtk_builder_get_object (builder, "entry_mod_cm_rs"));
      gtk_entry_set_text (GTK_ENTRY (widget), rs);

      widget = GTK_WIDGET (gtk_builder_get_object (builder, "entry_mod_cm_tel"));
      gtk_entry_set_text (GTK_ENTRY (widget), tel);

      widget = GTK_WIDGET (gtk_builder_get_object (builder, "entry_mod_cm_dir"));
      gtk_entry_set_text (GTK_ENTRY (widget), dir);

      widget = GTK_WIDGET (gtk_builder_get_object (builder, "entry_mod_cm_comuna"));
      gtk_entry_set_text (GTK_ENTRY (widget), comuna);

      widget = GTK_WIDGET (gtk_builder_get_object (builder, "entry_mod_cm_ciudad"));
      gtk_entry_set_text (GTK_ENTRY (widget), ciudad);

      widget = GTK_WIDGET (gtk_builder_get_object (builder, "entry_mod_cm_giro"));
      gtk_entry_set_text (GTK_ENTRY (widget), giro);

      widget = GTK_WIDGET (gtk_builder_get_object (builder, "lbl_mod_cm_id"));
      gtk_label_set_text (GTK_LABEL (widget), g_strdup_printf ("%d",id));

      widget = GTK_WIDGET (gtk_builder_get_object (builder, "wnd_mod_check_manager"));
      gtk_window_set_transient_for (GTK_WINDOW (widget), GTK_WINDOW (gtk_builder_get_object (builder, "wnd_admin")));
      gtk_widget_show_all(widget);
    }
}

/**
 * Callback connected to the accept button present in the modificate
 * client dialog.
 *
 * This function validates the information entered by the user
 */
void
on_btn_mod_cm_clicked (GtkButton *button, gpointer user_data)
{
  GtkWidget *widget;
  gchar *q;

  gint id;
  gchar *rut, *dv, *rs, *tel, *dir, *comuna, *ciudad, *giro;
  GtkEntry *rut_w, *dv_w, *rs_w, *tel_w, *dir_w, *comuna_w, *ciudad_w, *giro_w;
  //GtkWidget *wnd;

  //wnd = GTK_WIDGET(gtk_builder_get_object(builder, "wnd_mod_check_manager"));
  widget = GTK_WIDGET (gtk_builder_get_object(builder, "statusbar"));

  rut_w = GTK_ENTRY (gtk_builder_get_object(builder, "entry_mod_cm_rut"));
  dv_w = GTK_ENTRY (gtk_builder_get_object(builder, "entry_mod_cm_dv"));
  rs_w = GTK_ENTRY (gtk_builder_get_object(builder, "entry_mod_cm_rs"));
  tel_w = GTK_ENTRY (gtk_builder_get_object(builder, "entry_mod_cm_tel"));
  dir_w = GTK_ENTRY (gtk_builder_get_object(builder, "entry_mod_cm_dir"));
  comuna_w = GTK_ENTRY (gtk_builder_get_object(builder, "entry_mod_cm_comuna"));
  ciudad_w = GTK_ENTRY (gtk_builder_get_object(builder, "entry_mod_cm_ciudad"));
  giro_w = GTK_ENTRY (gtk_builder_get_object(builder, "entry_mod_cm_giro"));

  rut = g_strdup (gtk_entry_get_text (rut_w));
  dv = g_strdup (gtk_entry_get_text (dv_w));
  rs = g_strdup (gtk_entry_get_text (rs_w));
  tel = g_strdup (gtk_entry_get_text (tel_w));
  dir = g_strdup (gtk_entry_get_text (dir_w));
  comuna = g_strdup (gtk_entry_get_text (comuna_w));
  ciudad = g_strdup (gtk_entry_get_text (ciudad_w));
  giro = g_strdup (gtk_entry_get_text (giro_w));
  id = atoi (g_strdup (gtk_label_get_text (GTK_LABEL (builder_get (builder, "lbl_mod_cm_id")))));

  if (strcmp (rut, "") == 0)
    AlertMSG (GTK_WIDGET (rut_w), "Debe ingresar un rut válido");
  else if (strcmp (dv, "") == 0)
    AlertMSG (GTK_WIDGET (dv_w), "Debe ingresar un dígito verificador");
  else if (strcmp (rs, "") == 0)
    AlertMSG (GTK_WIDGET (rs_w), "Debe ingresar una razón social");
  else if (strcmp (tel, "") == 0)
    AlertMSG (GTK_WIDGET (tel_w), "Debe ingresar un telefono");
  else if (strcmp (dir, "") == 0)
    AlertMSG (GTK_WIDGET (dir_w), "Debe ingresar una dirección");
  else if (strcmp (comuna, "") == 0)
    AlertMSG (GTK_WIDGET (comuna_w), "Debe ingresar una comuna");
  else if (strcmp (ciudad, "") == 0)
    AlertMSG (GTK_WIDGET (ciudad_w), "Debe ingresar una ciudad");
  else if (strcmp (giro, "") == 0)
    AlertMSG (GTK_WIDGET (giro_w), "Debe especificar el giro del emisor");
  else
    {
      if (VerificarRut (rut, dv))
	{
	  //Verifica que el no haya otro emisor con el mismo rut
	  if (DataExist (g_strdup_printf ("SELECT rut FROM emisor_cheque WHERE rut=%s AND id != %d", rut, id)))
	    {
	      ErrorMSG (GTK_WIDGET (rut_w), g_strdup_printf ("Ya existe un emisor con el rut %s", rut));
	      return;
	    }

	  q = g_strdup_printf ("UPDATE emisor_cheque SET rut=%s, dv='%s', razon_social='%s', "
			       "telefono='%s', direccion='%s', comuna='%s', ciudad='%s', giro='%s' "
			       "WHERE id=%d",
			       rut, dv, rs, tel, dir, comuna, ciudad, giro, id);

	  if (EjecutarSQL (q) != NULL)
	    statusbar_push (GTK_STATUSBAR(widget), "Se modificaron los datos con exito", 3000);
	  else
	    ErrorMSG (widget, "No se puedo modificar el emisor de cheques");

	  gtk_widget_hide (GTK_WIDGET(gtk_builder_get_object(builder, "wnd_mod_check_manager")));
	  search_emisor ();
	}
      else
	AlertMSG (creditos->rut, "El Rut no es valido!!");
    }
}
