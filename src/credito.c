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
///used for the print in the client tab
Print *client_list;
Print *client_detail;
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
  gchar *enable;
  gchar *q;

  string = g_strdup (gtk_entry_get_text (GTK_ENTRY(widget)));
  clean_container (GTK_CONTAINER (builder_get (builder, "wnd_client_search")));
  gtk_entry_set_text (GTK_ENTRY (builder_get (builder, "entry_search_client")), string);

  q = g_strdup_printf ("SELECT rut::varchar || '-' || dv, nombre || ' ' || apell_p, telefono, credito_enable, direccion "
                       "FROM cliente WHERE lower(nombre) LIKE lower('%s%%') OR "
                       "lower(apell_p) LIKE lower('%s%%') OR lower(apell_m) LIKE lower('%s%%') OR "
                       "rut::varchar like ('%s%%')",
                       string, string, string, string);
  res = EjecutarSQL (q);
  g_free (q);

  tuples = PQntuples (res);

  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "treeview_clients"));
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
                              0, PQgetvalue (res, i, 0),
                              1, PQgetvalue (res, i, 1),
                              2, PQvaluebycol (res, i, "telefono"),
                              3, strcmp (enable, "t") ? FALSE : TRUE,
                              -1);
        }
      else
        gtk_list_store_set (store, &iter,
                            0, PQgetvalue (res, i, 0),
                            1, PQgetvalue (res, i, 1),
                            2, PQvaluebycol (res, i, "telefono"),
                            -1);
    }

  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "wnd_client_search"));
  gtk_widget_show_all (aux_widget);
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
                              G_TYPE_INT,    // id
                              G_TYPE_STRING, //name + apell_p + apell_m
                              G_TYPE_INT,    //phone
                              G_TYPE_BOOLEAN,//credit enabled
                              G_TYPE_INT, // Cupo
                              G_TYPE_INT,   //deuda
                              G_TYPE_INT); // saldo
  FillClientStore (store);

  tree = GTK_WIDGET (gtk_builder_get_object(builder, "treeview_clients"));
  gtk_tree_view_set_model(GTK_TREE_VIEW(tree), GTK_TREE_MODEL(store));
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));

  gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);

  g_signal_connect (G_OBJECT (selection), "changed",
                    G_CALLBACK (DatosDeudor), NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Codigo", renderer,
                                                     "text", 0,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, TRUE);

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
  client_list->cols[3].name = "Credito";
  client_list->cols[3].num = 3;
  client_list->cols[4].name = NULL;


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
  ventas = gtk_list_store_new (5,
                               G_TYPE_INT,
                               G_TYPE_INT,
                               G_TYPE_STRING,
                               G_TYPE_STRING,
                               G_TYPE_STRING);

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
  column = gtk_tree_view_column_new_with_attributes ("Monto", renderer,
                                                     "text", 4,
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
  client_detail->cols[4].name = "Monto";
  client_detail->cols[4].num = 4;
  client_detail->cols[5].name = NULL;


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
CloseAddClientWindow (void)
{
  clean_container (GTK_CONTAINER (builder_get (builder, "wnd_addclient")));

  gtk_widget_hide (GTK_WIDGET(gtk_builder_get_object(builder, "wnd_addclient")));
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
    AlertMSG (wnd, "El Campo Nombre no puede estar vacio");
  else if (strcmp (paterno, "") == 0)
    AlertMSG (wnd, "El Apellido Paterno no puede estar vacio");
  else if (strcmp (materno, "") == 0)
    AlertMSG (wnd, "El Apellido Materno no puede estar vacio");
  else if (strcmp (rut, "") == 0)
    AlertMSG (wnd, "El Campo rut no debe estar vacio");
  else if (strcmp (ver, "") == 0)
    AlertMSG (wnd, "El Campo verificador del ru no puede estar vacio");
  else if (strcmp (direccion, "") == 0)
    AlertMSG (wnd, "La direccion no puede estar vacia");
  else if (strcmp (fono, "") == 0)
    AlertMSG (wnd, "El campo telefonico no puede estar vacio");
  else if (strcmp (giro, "") == 0)
    AlertMSG (wnd, "El campo giro no puede estar vacio");
  else if (RutExist (rut) == TRUE)
    AlertMSG (wnd, "Ya existe alguien con el mismo rut");
  else if (credito == 0)
    AlertMSG (wnd, "El campo credito no puede estar vacio");
  else
    {
      if (VerificarRut (rut, ver) == TRUE)
        {
          if (!(InsertClient (nombre, paterno, materno, rut, ver, direccion, fono, credito, giro)))
            {
              ErrorMSG(wnd, "No fue posible agregar el cliente a la base de datos");
              return;
            }
          else
            CloseAddClientWindow ();
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
  res = EjecutarSQL ("SELECT rut, nombre, apell_p, apell_m, telefono, credito_enable FROM select_cliente()");

  tuples = PQntuples (res);

  gtk_list_store_clear (store);

  for (i = 0; i < tuples; i++)
    {
      gtk_list_store_append (store, &iter);
      if (user_data->user_id == 1)
        {
          enable = PQvaluebycol (res, i, "credito_enable");
          gtk_list_store_set (store, &iter,
                              0, atoi (PQvaluebycol(res, i, "rut")),
                              1, g_strdup_printf ("%s %s %s", PQvaluebycol(res, i, "nombre"),
                                                  PQvaluebycol(res, i, "apell_p"), PQvaluebycol(res, i, "apell_m")),
                              2, atoi (PQvaluebycol (res, i, "telefono")),
                              3, strcmp (enable, "t") ? FALSE : TRUE,
                              4, ReturnClientCredit (atoi (PQvaluebycol (res, i, "rut"))),
                              5, DeudaTotalCliente (atoi(PQvaluebycol(res, i, "rut"))),
                              6, CreditoDisponible (atoi(PQvaluebycol(res, i, "rut"))),
                              -1);
        }
      else
        gtk_list_store_set (store, &iter,
                            0, atoi (PQgetvalue (res, i, 4)),
                            1, g_strdup_printf ("%s %s %s", PQgetvalue (res, i, 1),
                                                PQgetvalue (res, i, 2), PQgetvalue (res, i, 3)),
                            2, atoi (PQgetvalue (res, i, 7)),
                            4, ReturnClientCredit (atoi (PQvaluebycol (res, i, "rut"))),
                            5, DeudaTotalCliente (atoi(PQvaluebycol(res, i, "rut"))),
                            6, CreditoDisponible (atoi(PQvaluebycol(res, i, "rut"))),
                            -1);
    }
  return 0;
}

void
DatosDeudor (GtkTreeSelection *treeselection,
             gpointer          user_data)
{
  GtkWidget *widget;
  GtkTreeIter iter;
  GtkTreeView *treeview;
  GtkListStore *store;
  gint rut;

  if (gtk_tree_selection_get_selected (treeselection, NULL, &iter))
    {
      treeview = gtk_tree_selection_get_tree_view (treeselection);

      gtk_tree_model_get (gtk_tree_view_get_model(treeview), &iter,
                          0, &rut,
                          -1);

      /* deuda = DeudaTotalCliente (rut); */

      // Limpiando treeviews //TODO: crear funciones que simplifiquen limpiar treeviews
      widget = GTK_WIDGET(gtk_builder_get_object(builder, "treeview_sales"));
      store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(widget)));
      gtk_list_store_clear (store);

      widget = GTK_WIDGET(gtk_builder_get_object(builder, "treeview_sale_details"));
      store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(widget)));
      gtk_list_store_clear (store);

      /* abono = GetResto (rut); */
      widget = GTK_WIDGET (gtk_builder_get_object(builder, "treeview_sales"));
      store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(widget)));
      gtk_list_store_clear (store);

      FillVentasDeudas (rut);
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

  res = SearchDeudasCliente (rut);

  tuples = PQntuples (res);

  widget = GTK_WIDGET (gtk_builder_get_object(builder, "treeview_sales"));
  store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(widget)));
  gtk_list_store_clear (store);

  for (i = 0; i < tuples; i++)
    {
      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
                          0, atoi (PQgetvalue (res, i, 0)),
                          1, atoi (PQgetvalue (res, i, 2)),
                          2, PQgetvalue (res, i, 3),
                          3, g_strdup_printf
                          ("%.2d/%.2d/%.4d %.2d:%.2d", atoi (PQgetvalue (res, i, 4)),
                           atoi (PQgetvalue (res, i, 5)), atoi (PQgetvalue (res, i, 6)),
                           atoi (PQgetvalue (res, i, 7)), atoi (PQgetvalue (res, i, 8))),
                          4, PutPoints (PQgetvalue (res, i, 1)),
                          -1);
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
  gchar *q;
  gint id_venta, tuples;
  gint i;
  PGresult *res;

  treeview = gtk_tree_selection_get_tree_view(treeselection);
  store = GTK_LIST_STORE(gtk_tree_view_get_model(treeview));

  if (gtk_tree_selection_get_selected (treeselection, NULL, &iter))
    {
      gtk_tree_model_get (GTK_TREE_MODEL (store), &iter,
                          0, &id_venta,
                          -1);

      widget = GTK_WIDGET (gtk_builder_get_object(builder, "treeview_sale_details"));
      store_detalle = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(widget)));
      gtk_list_store_clear (store_detalle);

      q = g_strdup_printf ("SELECT producto.codigo_corto, producto.descripcion, producto.marca, venta_detalle.cantidad, venta_detalle.precio "
                           "FROM venta_detalle inner join producto on producto.barcode=venta_detalle.barcode "
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
                              3, g_strtod (PUT(PQgetvalue (res, i, 3)),(gchar **)NULL),
                              4, PQgetvalue (res, i, 4),
                              5, PutPoints (g_strdup_printf("%ld", lround(g_strtod (PQgetvalue (res, i, 3), (gchar **)NULL) * 
									  g_strtod (PQgetvalue (res, i, 4), (gchar **)NULL)))) ,
                              -1);
        }
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
  gint rut;
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

  deuda = DeudaTotalCliente(rut);

  if (deuda == 0)
    {
      AlertMSG(widget, "No puede abonar a un cliente con deuda 0");
      return 0;
    }

  widget = GTK_WIDGET (gtk_builder_get_object(builder, "lbl_admin_abonar_rut"));
  gtk_label_set_text(GTK_LABEL(widget), g_strdup_printf("%d", rut));

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
  gint abono;
  gint deuda;

  widget = GTK_WIDGET (gtk_builder_get_object(builder, "lbl_admin_abonar_rut"));
  deuda = DeudaTotalCliente(atoi(gtk_label_get_text(GTK_LABEL(widget))));
  abono = atoi(gtk_entry_get_text(GTK_ENTRY(editable)));

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
  gint rut;

  /*De estar habilitada caja, se asegura que ésta se encuentre 
    abierta al momento de vender*/

  //TODO: Unificar esta comprobación en las funciones primarias 
  //      encargadas de hacer cualquier movimiento de caja

  if (rizoma_get_value_boolean ("CAJA"))
    if (check_caja()) // Se abre la caja en caso de que esté cerrada
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

      CloseAbonarWindow ();

      if (CancelarDeudas (abonar, rut) == 0)
        {
          gchar *msg;

          widget = GTK_WIDGET (gtk_builder_get_object(builder, "statusbar"));

          if (GetResto(rut) > 0)
            msg = g_strdup_printf("Se ha abonado con exito al cliente %d, actualmente tiene un monto de <b>%d</b> a favor",
                                  rut, GetResto(rut));
          else
            msg = g_strdup_printf("Se ha abonado con exito al cliente %d",
                                  rut);

          ExitoMSG(widget, msg);
          FillClientStore(GTK_LIST_STORE(store));
	  
	  // Limpiando treeviews //TODO: crear funciones que simplifiquen limpiar treeviews
	  widget = GTK_WIDGET(gtk_builder_get_object(builder, "treeview_sales"));
	  store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(widget)));
	  gtk_list_store_clear (store);

	  widget = GTK_WIDGET(gtk_builder_get_object(builder, "treeview_sale_details"));
	  store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(widget)));
	  gtk_list_store_clear (store);
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
  gchar *rut_ver, *giro;
  gint rut;

  treeview = GTK_TREE_VIEW(gtk_builder_get_object(builder, "treeview_clients"));
  selection = gtk_tree_view_get_selection(treeview);
  store = GTK_LIST_STORE(gtk_tree_view_get_model(treeview));

  if (gtk_tree_selection_get_selected (selection, NULL, &iter))
    {
      gtk_tree_model_get (GTK_TREE_MODEL (store), &iter,
                          0, &rut,
                          -1);

      q = g_strdup_printf ("select nombre, apell_p, apell_m, telefono, direccion, credito, dv, giro "
                           "FROM cliente where rut=%d", rut);
      res = EjecutarSQL(q);
      g_free(q);

      nombre = PQvaluebycol(res, 0, "nombre");
      apellido_materno = PQvaluebycol(res, 0, "apell_m");
      apellido_paterno = PQvaluebycol(res, 0, "apell_p");
      fono = PQvaluebycol(res, 0, "telefono");
      direccion = PQvaluebycol(res, 0, "direccion");
      credito = PQvaluebycol(res, 0, "credito");
      rut_ver = PQvaluebycol(res, 0, "dv");
      giro = PQvaluebycol(res, 0, "giro");

      widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_modclient_name"));
      gtk_entry_set_text (GTK_ENTRY (widget), nombre);

      widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_modclient_apell_p"));
      gtk_entry_set_text (GTK_ENTRY (widget), apellido_paterno);

      widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_modclient_apell_m"));
      gtk_entry_set_text (GTK_ENTRY (widget), apellido_materno);

      widget = GTK_WIDGET (gtk_builder_get_object(builder, "lbl_modclient_rut"));
      gtk_label_set_text (GTK_LABEL (widget), g_strdup_printf ("%d-%s", rut, rut_ver));

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
  gint rut;
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

  enable = !(enable);

  ChangeEnableCredit (enable, rut);

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
  gint rut;
  GtkTreeIter iter;

  treeview = GTK_TREE_VIEW(gtk_builder_get_object(builder, "treeview_clients"));
  store = GTK_LIST_STORE(gtk_tree_view_get_model(treeview));
  selection = gtk_tree_view_get_selection(treeview);

  if (gtk_tree_selection_get_selected (selection, NULL, &iter) && user_data->user_id == 1)
    {
      gtk_tree_model_get (GTK_TREE_MODEL (store), &iter,
                          0, &rut,
                          -1);

      if (ClientDelete (rut))
        {
          gtk_list_store_remove(store, &iter);
          widget = GTK_WIDGET (gtk_builder_get_object(builder, "statusbar"));
          statusbar_push (GTK_STATUSBAR(widget), "El Client fue eliminado con exito", 3000);
        }
      else
        ErrorMSG (GTK_WIDGET (treeview), "No se pudo elminar el cliente, compruebe que no tenga saldo deudor");
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
   rut = g_strsplit(gtk_label_get_text (GTK_LABEL (widget)), "-", 0);

   widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_modclient_addr"));
   direccion = g_strdup (gtk_entry_get_text (GTK_ENTRY (widget)));

   widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_modclient_phone"));
   fono = g_strdup (gtk_entry_get_text (GTK_ENTRY (widget)));

   widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_modclient_giro"));
   giro = g_strdup (gtk_entry_get_text (GTK_ENTRY (widget)));

   widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_modclient_limit_credit"));
   credito = atoi (g_strdup (gtk_entry_get_text (GTK_ENTRY (widget))));

   if (strcmp (nombre, "") == 0)
     AlertMSG (widget, "El Campo Nombre no puede estar vacio");
   else if (strcmp (paterno, "") == 0)
     AlertMSG (widget, "El Apellido Paterno no puede estar vacio");
   else if (strcmp (materno, "") == 0)
     AlertMSG (widget, "El Apellido Materno no puede estar vacio");
   else if (strcmp (rut[0], "") == 0)
     AlertMSG (widget, "El Campo rut no debe estar vacio");
   else if (strcmp (rut[1], "") == 0)
     AlertMSG (widget, "El Campo verificador del ru no puede estar vacio");
   else if (strcmp (direccion, "") == 0)
     AlertMSG (widget, "La direccion no puede estar vacia");
   else if (strcmp (fono, "") == 0)
     AlertMSG (widget, "El campo telefonico no puede estar vacio");
   else if (strcmp (giro, "") == 0)
     AlertMSG (widget, "El campo giro no puede estar vació");
   else if (credito == 0)
     AlertMSG (widget, "El campo credito no puede estar vacio");
   else
     {
       if (VerificarRut (rut[0], rut[1]))
         {
           q = g_strdup_printf ("UPDATE cliente SET nombre='%s', apell_p='%s', apell_m='%s', "
                                "direccion='%s', telefono='%s', credito=%d, giro='%s' WHERE rut=%d",
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
         AlertMSG (creditos->rut, "El Rut no es valido!!");
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
  gchar *rut2;

  rut2 = g_strdup(rut);
  res = EjecutarSQL (g_strdup_printf ("SELECT credito FROM cliente WHERE rut=%s", strtok(rut2,"-")));

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
  PrintTree(NULL, (gpointer) client_detail);
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

  uimanager = GTK_UI_MANAGER(g_object_get_data(G_OBJECT(button), "uimanager"));

  widget = gtk_ui_manager_get_widget(uimanager, "/popup");

  gtk_menu_popup (GTK_MENU(widget), NULL, NULL, NULL, NULL, 1, gtk_get_current_event_time());

  gtk_widget_show_all(widget);
}

void
admin_search_client(void)
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

  q = g_strdup_printf ("SELECT rut, nombre || ' ' || apell_p || ' ' || apell_m, telefono, credito_enable "
                       "FROM cliente WHERE lower(nombre) LIKE lower('%s%%') OR "
                       "lower(apell_p) LIKE lower('%s%%') OR lower(apell_m) LIKE lower('%s%%') OR "
                       "rut::varchar like ('%s%%')",
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
                              0, atoi(PQgetvalue (res, i, 0)),
                              1, PQgetvalue (res, i, 1),
                              2, atoi(PQvaluebycol (res, i, "telefono")),
                              3, strcmp (enable, "t") ? FALSE : TRUE,
                              4, ReturnClientCredit (atoi (PQvaluebycol (res, i, "rut"))),
                              5, DeudaTotalCliente (atoi(PQvaluebycol(res, i, "rut"))),
                              6, CreditoDisponible (atoi(PQvaluebycol(res, i, "rut"))),
                              -1);
        }
      else
        gtk_list_store_set (store, &iter,
                            0, atoi(PQgetvalue (res, i, 0)),
                            1, PQgetvalue (res, i, 1),
                            2, atoi (PQvaluebycol (res, i, "telefono")),
                            4, ReturnClientCredit (atoi (PQvaluebycol (res, i, "rut"))),
                            5, DeudaTotalCliente (atoi(PQvaluebycol(res, i, "rut"))),
                            6, CreditoDisponible (atoi(PQvaluebycol(res, i, "rut"))),
                            -1);
    }
}
