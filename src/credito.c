/*credito.c
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
#include<stdlib.h>

#include"tipos.h"
#include"credito.h"
#include"postgres-functions.h"
#include"errors.h"
#include"main.h"
#include"printing.h"
#include"dimentions.h"

GtkWidget *modificar_window;

void
search_client (GtkWidget *widget, gpointer data)
{
  GtkListStore *store = (GtkListStore *) data;
  GtkTreeIter iter;
  gchar *string = g_strdup (gtk_entry_get_text (GTK_ENTRY (creditos->search_entry)));
  PGresult *res;
  gint tuples, i;
  gchar *enable;

  res = EjecutarSQL 
    (g_strdup_printf ("SELECT * FROM cliente WHERE lower(nombre) LIKE lower('%s%%') OR "
		      "lower(apellido_paterno) LIKE lower('%s%%') OR lower(apellido_materno) LIKE lower('%s%%')", 
		      string, string, string));

  tuples = PQntuples (res);

  gtk_list_store_clear (store);
  
  for (i = 0; i < tuples; i++)
    {
      gtk_list_store_append (store, &iter);
  
      if (user_data->user_id == 1)
	{
	  enable = PQgetvalue (res, i, 10);
	  gtk_list_store_set (store, &iter,
			      0, atoi (PQgetvalue (res, i, 4)),
			      1, g_strdup_printf ("%s %s %s", PQgetvalue (res, i, 1),
						  PQgetvalue (res, i, 2), PQgetvalue (res, i, 3)),
			      2, atoi (PQgetvalue (res, i, 7)),
			      3, strcmp (enable, "t") ? FALSE : TRUE,
			      -1);
	}
      else
	gtk_list_store_set (store, &iter,
			    0, atoi (PQgetvalue (res, i, 4)),
			    1, g_strdup_printf ("%s %s %s", PQgetvalue (res, i, 1),
						PQgetvalue (res, i, 2), PQgetvalue (res, i, 3)),
			    2, atoi (PQgetvalue (res, i, 7)),
			    -1);
    }
}

void
//creditos_box (MainBox *module_box)
creditos_box (GtkWidget *main_box)
{ 
  GtkWidget *scroll;

  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkWidget *vbox;
  GtkWidget *vbox2;
  GtkWidget *hbox;

  GtkWidget *label;
  GtkWidget *button;

  Print *client_list = (Print *) malloc (sizeof (Print));
  Print *client_detail = (Print *) malloc (sizeof (Print));
  client_detail->son = (Print *) malloc (sizeof (Print));
  /*  if (module_box->new_box != NULL)
    gtk_widget_destroy (GTK_WIDGET (module_box->new_box));
  */
  /*  module_box->new_box = gtk_hbox_new (FALSE, 3);
  gtk_widget_set_size_request (module_box->new_box, MODULE_BOX_WIDTH, MODULE_BOX_HEIGHT);
  gtk_widget_show (module_box->new_box);
  gtk_box_pack_start (GTK_BOX (module_box->main_box), module_box->new_box, FALSE, FALSE, 3);*/

  vbox = gtk_vbox_new (FALSE, 3);
  gtk_widget_show (vbox);
  gtk_box_pack_start (GTK_BOX (main_box), vbox, FALSE, FALSE, 3);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
  
  creditos->store = gtk_list_store_new (4,
					G_TYPE_INT,
					G_TYPE_STRING,
					G_TYPE_INT,
					G_TYPE_BOOLEAN);
  //  FillClientStore (creditos->store);  

  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_show (scroll);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (hbox), scroll, FALSE, FALSE, 3);
  

  creditos->tree = gtk_tree_view_new_with_model (GTK_TREE_MODEL (creditos->store));
  gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (creditos->tree), TRUE);
  gtk_widget_show (creditos->tree);
  if (solo_venta != TRUE)
    gtk_widget_set_size_request (creditos->tree, (MODULE_BOX_WIDTH - 180), 200);
  else
    gtk_widget_set_size_request (creditos->tree, (MODULE_LITTLE_BOX_WIDTH - 150), 150);
  gtk_container_add (GTK_CONTAINER (scroll), creditos->tree);
  
  creditos->selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (creditos->tree));

  gtk_tree_selection_set_mode (creditos->selection, GTK_SELECTION_SINGLE);

  g_signal_connect (G_OBJECT (creditos->selection), "changed",
		    G_CALLBACK (DatosDeudor), NULL);
  
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Código", renderer,
						     "text", 0,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (creditos->tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_min_width (column, 80);
  gtk_tree_view_column_set_max_width (column, 80);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Nombre", renderer,
						     "text", 1,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (creditos->tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  gtk_tree_view_column_set_min_width (column, 260);
  gtk_tree_view_column_set_max_width (column, 260);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Teléfono", renderer,
						     "text", 2,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (creditos->tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_min_width (column, 70);
  gtk_tree_view_column_set_max_width (column, 70);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_toggle_new ();
  column = gtk_tree_view_column_new_with_attributes ("Crédito", renderer,
						     "active", 3,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (creditos->tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_min_width (column, 10);
  gtk_tree_view_column_set_max_width (column, 10);
  gtk_tree_view_column_set_resizable (column, FALSE);

  client_list->tree = GTK_TREE_VIEW (creditos->tree);
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

  g_signal_connect (G_OBJECT (renderer), "toggled",
		    G_CALLBACK (ToggleClientCredit), NULL);

  g_signal_connect (G_OBJECT (creditos->tree), "row-activated",
		    G_CALLBACK (ClientStatus), NULL);
  
  vbox2 = gtk_vbox_new (FALSE, 3);
  gtk_widget_show (vbox2);
  gtk_box_pack_end (GTK_BOX (hbox), vbox2, FALSE, FALSE, 3);
  
  label = gtk_label_new ("Buscar Persona");
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_CENTER);
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (vbox2), label, FALSE, FALSE, 3);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 3);

  creditos->search_entry = gtk_entry_new ();
  gtk_widget_set_size_request (creditos->search_entry, 140, -1);
  gtk_widget_show (creditos->search_entry);
  gtk_box_pack_start (GTK_BOX (hbox), creditos->search_entry, FALSE, FALSE, 3);

  g_signal_connect (G_OBJECT (creditos->search_entry), "activate",
		    G_CALLBACK (search_client), (gpointer)creditos->store);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 3);
  
  button = gtk_button_new_from_stock (GTK_STOCK_FIND);
  gtk_widget_show (button);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);

  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (search_client), (gpointer)creditos->store);

  if (user_data->user_id == 1)
    {
      hbox = gtk_hbox_new (FALSE, 3);
      gtk_widget_show (hbox);
      gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 3);      

      button = gtk_button_new_from_stock (GTK_STOCK_DELETE);
      gtk_widget_show (button);
      gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);

      g_signal_connect (G_OBJECT (button), "clicked",
			G_CALLBACK (EliminarCliente), NULL);
      
  
      button = gtk_button_new_with_label ("Modificar");
      gtk_widget_show (button);
      gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);
      
      g_signal_connect (G_OBJECT (button), "clicked",
			G_CALLBACK (ModificarCliente), NULL);
      
      hbox = gtk_hbox_new (FALSE, 3);
      gtk_widget_show (hbox);
      gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 3);      

      button = gtk_button_new_from_stock (GTK_STOCK_ADD);
      gtk_widget_show (button);
      gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);
      
      g_signal_connect (G_OBJECT (button), "clicked",
			G_CALLBACK (AddClient), (gpointer)creditos->store);

      button = gtk_button_new_from_stock (GTK_STOCK_PRINT);
      gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);
      gtk_widget_show (button);
      
      g_signal_connect (G_OBJECT (button), "clicked",
			G_CALLBACK (PrintTree), (gpointer)client_list);
    }

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  
  creditos->ventas = gtk_list_store_new (5,
					 G_TYPE_INT,
					 G_TYPE_INT,
					 G_TYPE_STRING,
					 G_TYPE_STRING,
					 G_TYPE_STRING);
  
  //FillVentasDeudas ();

  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_show (scroll);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_AUTOMATIC);
  if (solo_venta != TRUE)
    gtk_widget_set_size_request (scroll, 400, 130);
  else
    gtk_widget_set_size_request (scroll, 400, 80);
  gtk_box_pack_start (GTK_BOX (hbox), scroll, FALSE, FALSE, 3);

  creditos->tree_ventas = gtk_tree_view_new_with_model (GTK_TREE_MODEL (creditos->ventas));
  gtk_widget_show (creditos->tree_ventas);
  gtk_container_add (GTK_CONTAINER (scroll), creditos->tree_ventas);

  creditos->selection_ventas = gtk_tree_view_get_selection (GTK_TREE_VIEW (creditos->tree_ventas));

  g_signal_connect (G_OBJECT (creditos->selection_ventas), "changed",
		    G_CALLBACK (ChangeDetalle), NULL);
  
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("ID Venta", renderer,
						     "text", 0,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (creditos->tree_ventas), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);
  
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Maquina", renderer,
						     "text", 1,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (creditos->tree_ventas), column);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);
  
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Vendedor", renderer,
						     "text", 2,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (creditos->tree_ventas), column);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_alignment (column, 0.5);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Fecha", renderer,
						     "text", 3,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (creditos->tree_ventas), column);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_alignment (column, 0.5);
  gtk_tree_view_column_set_min_width (column, 120);
  gtk_tree_view_column_set_max_width (column, 120);  
  gtk_tree_view_column_set_resizable (column, FALSE);
  
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Monto", renderer,
						     "text", 4,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (creditos->tree_ventas), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  vbox2 = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox2);
  gtk_box_pack_start (GTK_BOX (hbox), vbox2, FALSE, FALSE, 3);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 3);

  label = gtk_label_new ("Deuda: ");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);

  creditos->deuda = gtk_label_new ("");
  gtk_widget_show (creditos->deuda);
  gtk_box_pack_end (GTK_BOX (hbox), creditos->deuda, FALSE, FALSE, 3);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 3);

  label = gtk_label_new ("Abonado: ");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);

  creditos->abono = gtk_label_new ("");
  gtk_widget_show (creditos->abono);
  gtk_box_pack_end (GTK_BOX (hbox), creditos->abono, FALSE, FALSE, 3);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 3);

  label = gtk_label_new ("Deuda Total del Cliente: ");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);

  creditos->deuda_total = gtk_label_new ("");
  gtk_widget_show (creditos->deuda_total);
  gtk_box_pack_start (GTK_BOX (hbox), creditos->deuda_total, FALSE, FALSE, 3);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 3);

  button = gtk_button_new_with_label ("Abono");
  gtk_widget_show (button);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 3);

  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (AbonarWindow), NULL);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 3);

  button = gtk_button_new_from_stock (GTK_STOCK_PRINT);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 3);
  gtk_widget_show (button);
  
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  creditos->detalle = gtk_list_store_new (6,
					  G_TYPE_STRING,
					  G_TYPE_STRING,
					  G_TYPE_STRING,
					  G_TYPE_INT,
					  G_TYPE_STRING,
					  G_TYPE_STRING);

  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_show (scroll);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_AUTOMATIC);
  if (solo_venta != TRUE)
    gtk_widget_set_size_request (scroll, (MODULE_BOX_WIDTH - 20), 150);
  else
    gtk_widget_set_size_request (scroll, (MODULE_LITTLE_BOX_WIDTH - 20), 100);
  gtk_box_pack_start (GTK_BOX (hbox), scroll, FALSE, FALSE, 3);

  creditos->tree_detalle = gtk_tree_view_new_with_model (GTK_TREE_MODEL (creditos->detalle));
  gtk_widget_show (creditos->tree_detalle);
  gtk_container_add (GTK_CONTAINER (scroll), creditos->tree_detalle);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Código", renderer,
						     "text", 0,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (creditos->tree_detalle), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_min_width (column, 100);
  gtk_tree_view_column_set_max_width (column, 100);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Producto", renderer,
						     "text", 1,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (creditos->tree_detalle), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_min_width (column, 200);
  gtk_tree_view_column_set_max_width (column, 200);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Marca", renderer,
						     "text", 2,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (creditos->tree_detalle), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_min_width (column, 130);
  gtk_tree_view_column_set_max_width (column, 130);
  

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Cantidad", renderer,
						     "text", 3,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (creditos->tree_detalle), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);
  
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Unitario", renderer,
						     "text", 4,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (creditos->tree_detalle), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Total", renderer,
						     "text", 5,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (creditos->tree_detalle), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);
  
  /* We fill the struct to print the last two tree views */

  client_detail->tree = GTK_TREE_VIEW (creditos->tree_ventas);
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

  client_detail->son->tree = GTK_TREE_VIEW (creditos->tree_detalle);
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

  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (PrintTwoTree), (gpointer)client_detail);

  
}

void
AddClient (GtkWidget *widget, gpointer data)
{
  GtkWidget *button;
  GtkWidget *frame;

  GtkWidget *label;
  GtkWidget *box;
  GtkWidget *box2;
  GtkWidget *hbox;
  GtkWidget *vbox;

  creditos->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_position (GTK_WINDOW (creditos->window), GTK_WIN_POS_CENTER_ALWAYS);
  gtk_widget_show (creditos->window);

  g_signal_connect (G_OBJECT (creditos->window), "destroy",
		   G_CALLBACK (CloseAddClientWindow), NULL);

  vbox = gtk_vbox_new (FALSE, 3);
  gtk_widget_show (vbox);
  gtk_container_add (GTK_CONTAINER (creditos->window), vbox);

  frame = gtk_frame_new ("Datos del Nuevo Cliente");
  gtk_widget_show (frame);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 3);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_end (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  button = gtk_button_new_from_stock (GTK_STOCK_ADD);
  gtk_widget_show (button);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);

  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (AgregarClienteABD), (gpointer)data);

  button = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
  gtk_widget_show (button);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);

  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (CloseAddClientWindow), NULL);

  vbox = gtk_vbox_new (FALSE, 3);
  gtk_widget_show (vbox);
  gtk_container_add (GTK_CONTAINER (frame), vbox);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  box = gtk_vbox_new (TRUE, 2);
  gtk_widget_show (box);
  label = gtk_label_new ("Nombres");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
  creditos->entry_nombres = gtk_entry_new_with_max_length (100);
  gtk_widget_set_size_request (creditos->entry_nombres, 100, -1);
  gtk_box_pack_start (GTK_BOX (box), creditos->entry_nombres, FALSE, FALSE, 0);
  gtk_widget_show (creditos->entry_nombres);

  gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);

  gtk_window_set_focus (GTK_WINDOW (creditos->window), creditos->entry_nombres);

  box = gtk_vbox_new (TRUE, 2);
  gtk_widget_show (box);
  label = gtk_label_new ("Apellido Paterno");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
  creditos->paterno = gtk_entry_new_with_max_length (60);
  gtk_widget_set_size_request (creditos->paterno, 100, -1);
  gtk_box_pack_start (GTK_BOX (box), creditos->paterno, FALSE, FALSE, 0);
  gtk_widget_show (creditos->paterno);

  gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);

  box = gtk_vbox_new (TRUE, 2);
  gtk_widget_show (box);
  label = gtk_label_new ("Apellido Materno");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
  creditos->materno = gtk_entry_new_with_max_length (60);
  gtk_widget_set_size_request (creditos->materno, 100, -1);
  gtk_box_pack_start (GTK_BOX (box), creditos->materno, FALSE, FALSE, 0);
  gtk_widget_show (creditos->materno);

  gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  box = gtk_vbox_new (TRUE, 2);
  gtk_widget_show (box);
  label = gtk_label_new ("Rut");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
  
  box2 = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (box2);
  gtk_box_pack_start (GTK_BOX (box), box2, FALSE, FALSE, 0);
  creditos->rut = gtk_entry_new_with_max_length (8);
  gtk_widget_set_size_request (creditos->rut, 72, -1);
  gtk_box_pack_start (GTK_BOX (box2), creditos->rut, FALSE, FALSE, 0);
  gtk_widget_show (creditos->rut);
  label = gtk_label_new (" - ");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (box2), label, FALSE, FALSE, 0);
  creditos->rut_ver = gtk_entry_new_with_max_length (1);
  gtk_widget_set_size_request ( creditos->rut_ver, 25, -1);
  gtk_widget_show (creditos->rut_ver);
  gtk_box_pack_start (GTK_BOX (box2), creditos->rut_ver, FALSE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);

  box = gtk_vbox_new (TRUE, 2);
  gtk_widget_show (box);
  label = gtk_label_new ("Direccion");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
  creditos->direccion = gtk_entry_new_with_max_length (150);
  gtk_widget_set_size_request (creditos->direccion, 150, -1);
  gtk_box_pack_end (GTK_BOX (box), creditos->direccion, FALSE, FALSE, 0);
  gtk_widget_show (creditos->direccion);

  gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);
    
  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  box = gtk_vbox_new (TRUE, 2);
  gtk_widget_show (box);
  label = gtk_label_new ("Teléfono");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
  creditos->fono = gtk_entry_new_with_max_length (60);
  gtk_widget_set_size_request (creditos->fono, 90, -1);
  gtk_box_pack_start (GTK_BOX (box), creditos->fono, FALSE, FALSE, 0);
  gtk_widget_show (creditos->fono);

  gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);

  box = gtk_vbox_new (TRUE, 2);
  gtk_widget_show (box);
  label = gtk_label_new ("Giro");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
  creditos->giro = gtk_entry_new_with_max_length (60);
  gtk_widget_set_size_request (creditos->giro, 90, -1);
  gtk_box_pack_start (GTK_BOX (box), creditos->giro, FALSE, FALSE, 0);
  gtk_widget_show (creditos->giro);

  gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);


  box = gtk_vbox_new (TRUE, 2);
  gtk_widget_show (box);
  label = gtk_label_new ("Limite de Crédito");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
  creditos->credito = gtk_entry_new_with_max_length (20);
  gtk_widget_set_size_request (creditos->credito, 90, -1);
  gtk_box_pack_start (GTK_BOX (box), creditos->credito, FALSE, FALSE, 0);
  gtk_widget_show (creditos->credito);

  gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);
}

void
CloseAddClientWindow (void)
{
  gtk_widget_destroy (creditos->window);
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
  gchar *nombre = g_strdup (gtk_entry_get_text (GTK_ENTRY (creditos->entry_nombres)));
  gchar *paterno = g_strdup (gtk_entry_get_text (GTK_ENTRY (creditos->paterno)));
  gchar *materno = g_strdup (gtk_entry_get_text (GTK_ENTRY (creditos->materno)));
  gchar *rut = g_strdup (gtk_entry_get_text (GTK_ENTRY (creditos->rut)));
  gchar *ver = g_strdup (gtk_entry_get_text (GTK_ENTRY (creditos->rut_ver)));
  gchar *direccion = g_strdup (gtk_entry_get_text (GTK_ENTRY (creditos->direccion)));
  gchar *fono = g_strdup (gtk_entry_get_text (GTK_ENTRY (creditos->fono)));
  gchar *giro = g_strdup (gtk_entry_get_text (GTK_ENTRY (creditos->giro)));
  gint credito = atoi (g_strdup (gtk_entry_get_text (GTK_ENTRY (creditos->credito))));

  if (strcmp (nombre, "") == 0)
    AlertMSG (creditos->entry_nombres, "El Campo Nombre no puede estar vacio");
  else if (strcmp (paterno, "") == 0)
    AlertMSG (creditos->paterno, "El Apellido Paterno no puede estar vacio");
  else if (strcmp (materno, "") == 0)
    AlertMSG (creditos->materno, "El Apellido Materno no puede estar vacio");
  else if (strcmp (rut, "") == 0)
    AlertMSG (creditos->rut, "El Campo rut no debe estar vacio");
  else if (strcmp (ver, "") == 0)
    AlertMSG (creditos->rut_ver, "El Campo verificador del ru no puede estar vacio");
  else if (strcmp (direccion, "") == 0)
    AlertMSG (creditos->direccion, "La direccion no puede estar vacia");
  else if (strcmp (fono, "") == 0)
    AlertMSG (creditos->fono, "El campo telefonico no puede estar vacio");
  else if (strcmp (giro, "") == 0)
    AlertMSG (creditos->giro, "El campo giro no puede estar vacio");
  else if (RutExist (rut) == TRUE)
    AlertMSG (creditos->rut, "Ya existe alguien con el mismo rut");
  else if (credito == 0)
    AlertMSG (creditos->credito, "El campo credito no puede estar vacio");
  else
    {
      if (VerificarRut (rut, ver) == TRUE)
	{
	  InsertClient (nombre, paterno, materno, rut, ver, direccion, fono, credito, giro);
	  FillClientStore ((GtkListStore *)data);
	  CloseAddClientWindow ();	  
	}
      else
	AlertMSG (creditos->rut, "El Rut no es valido!!");
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
			      -1);
	}
      else
	gtk_list_store_set (store, &iter,
			    0, atoi (PQgetvalue (res, i, 4)),
			    1, g_strdup_printf ("%s %s %s", PQgetvalue (res, i, 1),
						PQgetvalue (res, i, 2), PQgetvalue (res, i, 3)),
			    2, atoi (PQgetvalue (res, i, 7)),
			    -1);
    }
  return 0;
}

void
DatosDeudor (void)
{
  GtkTreeIter iter;
  gint rut;
  gint deuda, abono;

  if (gtk_tree_selection_get_selected (creditos->selection, NULL, &iter) == TRUE)
    {
      gtk_tree_model_get (GTK_TREE_MODEL (creditos->store), &iter, 
			  0, &rut, 
			  -1);

      deuda = DeudaTotalCliente (rut);

      abono = GetResto (rut);

      gtk_label_set_text (GTK_LABEL (creditos->deuda),
			  PutPoints (g_strdup_printf ("%d", deuda)));

      gtk_label_set_text (GTK_LABEL (creditos->abono),
			  PutPoints (g_strdup_printf ("%d", abono)));
    
      gtk_label_set_text (GTK_LABEL (creditos->deuda_total),
			  PutPoints (g_strdup_printf ("%d", deuda - abono)));
      
      gtk_list_store_clear (creditos->detalle);

      FillVentasDeudas (rut);
    }
}

void
FillVentasDeudas (gint rut)
{
  gint i;
  gint tuples;
  GtkTreeIter iter;
  PGresult *res;

  res = SearchDeudasCliente (rut);

  tuples = PQntuples (res);

  gtk_list_store_clear (creditos->ventas);

  for (i = 0; i < tuples; i++)
    {
      gtk_list_store_append (creditos->ventas, &iter);
      gtk_list_store_set (creditos->ventas, &iter,
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
ChangeDetalle (void)
{
  GtkTreeIter iter;
  gint id_venta, tuples;
  gint i;
  PGresult *res;

  if (gtk_tree_selection_get_selected (creditos->selection_ventas, NULL, &iter) == TRUE)
    {
      gtk_tree_model_get (GTK_TREE_MODEL (creditos->ventas), &iter, 
			  0, &id_venta, 
			  -1);

      gtk_list_store_clear (creditos->detalle);

      res = EjecutarSQL (g_strdup_printf 
			 ("SELECT t2.codigo, t2.descripcion, t2.marca, t1.cantidad, t1.precio "
			  "FROM venta_detalle AS t1, productos AS t2 WHERE "
			  "t1.id_venta=%d AND t2.barcode=t1.barcode", id_venta));

      tuples = PQntuples (res);

      for (i = 0; i < tuples; i++)
	{
	  gtk_list_store_append (creditos->detalle, &iter);
	  gtk_list_store_set (creditos->detalle, &iter,
			      0, PQgetvalue (res, i, 0),
			      1, PQgetvalue (res, i, 1),
			      2, PQgetvalue (res, i, 2),
			      3, atoi (PQgetvalue (res, i, 3)),
			      4, PQgetvalue (res, i, 4),
			      5, PutPoints (g_strdup_printf ("%d", atoi (PQgetvalue (res, i, 3)) *
							     atoi (PQgetvalue (res, i, 4)))),
			      -1);
	}      
    }
}

void
KillAbonarWindow (void)
{
  gtk_widget_destroy (creditos->abonar_window);
}

gint
AbonarWindow (void)
{
  GtkTreeIter iter;
  GtkWidget *frame;
  GtkWidget *label;
  GtkWidget *button;
  GtkWidget *sell_button;

  GtkWidget *hbox;
  GtkWidget *vbox;

  if (gtk_tree_selection_get_selected (creditos->selection, NULL, &iter) == FALSE)
    return 0;

  creditos->abonar_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_widget_set_size_request (creditos->abonar_window, -1, 120);
  gtk_window_set_position (GTK_WINDOW (creditos->abonar_window), GTK_WIN_POS_CENTER_ALWAYS);
  gtk_widget_show (creditos->abonar_window);
  gtk_window_present (GTK_WINDOW (creditos->abonar_window));
  gtk_window_set_title (GTK_WINDOW (creditos->abonar_window), "Abonado de Dinero");

  g_signal_connect (G_OBJECT (creditos->abonar_window), "destroy",
		    G_CALLBACK (KillAbonarWindow), NULL);

  vbox = gtk_vbox_new (FALSE, 3);
  gtk_widget_show (vbox);
  gtk_container_add (GTK_CONTAINER (creditos->abonar_window), vbox);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_end (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  sell_button = gtk_button_new_with_label ("Abonar");
  gtk_widget_show (sell_button);
  gtk_box_pack_end (GTK_BOX (hbox), sell_button, FALSE, FALSE, 3);

  g_signal_connect (G_OBJECT (sell_button), "clicked",
		    G_CALLBACK (Abonar), NULL);

  button = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
  gtk_widget_show (button);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);

  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (KillAbonarWindow), NULL);

  frame = gtk_frame_new ("Abonar");
  gtk_widget_show (frame);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 3);

  vbox = gtk_vbox_new (FALSE, 3);
  gtk_widget_show (vbox);
  gtk_container_add (GTK_CONTAINER (frame), vbox);

  label = gtk_label_new ("Ingrese la cantidad de Dinero");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 3);

  creditos->entry_plata = gtk_entry_new ();
  gtk_widget_show (creditos->entry_plata);
  gtk_box_pack_start (GTK_BOX (vbox), creditos->entry_plata, FALSE, FALSE, 3);

  g_signal_connect (G_OBJECT (creditos->entry_plata), "activate",
		    G_CALLBACK (SendCursorTo), (gpointer) sell_button);
  
  return 0;
}

void
Abonar (void)
{
  GtkTreeIter iter;
  gint abonar = atoi (g_strdup (gtk_entry_get_text (GTK_ENTRY (creditos->entry_plata))));
  gint rut;

  if (gtk_tree_selection_get_selected (creditos->selection, NULL, &iter) == TRUE)
    {
      gtk_tree_model_get (GTK_TREE_MODEL (creditos->store), &iter, 
			  0, &rut, 
			  -1);
      
      KillAbonarWindow ();
    
      CancelarDeudas (abonar, rut);
      
      DatosDeudor ();
    }
}

void
CloseModificarWindow (void)
{
  gtk_widget_destroy (modificar_window);
  
  modificar_window = NULL;
  gtk_widget_set_sensitive (main_window, TRUE);
}

void
ModificarCliente (void)
{
  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *box;
  GtkWidget *box2;

  GtkWidget *label;
  GtkWidget *button;

  GtkTreeIter iter;

  gchar *nombre, *apellido_materno, *apellido_paterno;
  gchar *fono, *direccion, *credito, *rut_ver, *giro;
  gint rut;

  if (gtk_tree_selection_get_selected (creditos->selection, NULL, &iter) == TRUE)
    {
      gtk_widget_set_sensitive (main_window, FALSE);
      
      gtk_tree_model_get (GTK_TREE_MODEL (creditos->store), &iter,
			  0, &rut,
			  -1);
      
      nombre = GetDataByOne (g_strdup_printf ("SELECT nombre FROM cliente WHERE rut=%d", rut));
      apellido_materno = g_strdup (GetDataByOne (g_strdup_printf ("SELECT apellido_materno FROM cliente WHERE rut=%d", rut)));
      apellido_paterno = g_strdup (GetDataByOne (g_strdup_printf ("SELECT apellido_paterno FROM cliente WHERE rut=%d", rut)));
      fono = g_strdup (GetDataByOne (g_strdup_printf ("SELECT telefono FROM cliente WHERE rut=%d", rut)));
      direccion = g_strdup (GetDataByOne (g_strdup_printf ("SELECT direccion FROM cliente WHERE rut=%d", rut)));
      credito = g_strdup (GetDataByOne (g_strdup_printf ("SELECT credito FROM cliente WHERE rut=%d", rut)));
      rut_ver = g_strdup (GetDataByOne (g_strdup_printf ("SELECT verificador FROM cliente WHERE rut=%d", rut)));
      giro = g_strdup (GetDataByOne (g_strdup_printf ("SELECT giro FROM cliente WHERE rut=%d", rut)));

      modificar_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
      gtk_window_set_position (GTK_WINDOW (modificar_window), GTK_WIN_POS_CENTER_ALWAYS);
      gtk_widget_show (modificar_window);
      
      g_signal_connect (G_OBJECT (modificar_window), "destroy",
			G_CALLBACK (CloseModificarWindow), NULL);
      
      vbox = gtk_vbox_new (FALSE, 3);
      gtk_widget_show (vbox);
      gtk_container_add (GTK_CONTAINER (modificar_window), vbox);
      
      hbox = gtk_hbox_new (FALSE, 3);
      gtk_widget_show (hbox);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
      
      box = gtk_vbox_new (TRUE, 2);
      gtk_widget_show (box);
      label = gtk_label_new ("Nombres");
      gtk_widget_show (label);
      gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
      creditos->entry_nombres = gtk_entry_new_with_max_length (100);
      gtk_entry_set_text (GTK_ENTRY (creditos->entry_nombres), nombre);
      gtk_widget_set_size_request (creditos->entry_nombres, 100, -1);
      gtk_box_pack_start (GTK_BOX (box), creditos->entry_nombres, FALSE, FALSE, 0);
      gtk_widget_show (creditos->entry_nombres);
      
      gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);
      
      box = gtk_vbox_new (TRUE, 2);
      gtk_widget_show (box);
      label = gtk_label_new ("Apellido Paterno");
      gtk_widget_show (label);
      gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
      creditos->paterno = gtk_entry_new_with_max_length (60);
      gtk_entry_set_text (GTK_ENTRY (creditos->paterno), apellido_paterno);
      gtk_widget_set_size_request (creditos->paterno, 100, -1);
      gtk_box_pack_start (GTK_BOX (box), creditos->paterno, FALSE, FALSE, 0);
      gtk_widget_show (creditos->paterno);
      
      gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);
      
      box = gtk_vbox_new (TRUE, 2);
      gtk_widget_show (box);
      label = gtk_label_new ("Apellido Materno");
      gtk_widget_show (label);
      gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
      creditos->materno = gtk_entry_new_with_max_length (60);
      gtk_entry_set_text (GTK_ENTRY (creditos->materno), apellido_materno);
      gtk_widget_set_size_request (creditos->materno, 100, -1);
      gtk_box_pack_start (GTK_BOX (box), creditos->materno, FALSE, FALSE, 0);
      gtk_widget_show (creditos->materno);
      
      gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);
      
      hbox = gtk_hbox_new (FALSE, 3);
      gtk_widget_show (hbox);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
      
      box = gtk_vbox_new (TRUE, 2);
      gtk_widget_show (box);
      label = gtk_label_new ("Rut");
      gtk_widget_show (label);
      gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
      
      box2 = gtk_hbox_new (FALSE, 3);
      gtk_widget_show (box2);
      gtk_box_pack_start (GTK_BOX (box), box2, FALSE, FALSE, 0);
      creditos->rut = gtk_entry_new_with_max_length (8);
      gtk_entry_set_text (GTK_ENTRY (creditos->rut), g_strdup_printf ("%d", rut));
      gtk_widget_set_size_request (creditos->rut, 72, -1);
      gtk_widget_set_sensitive (creditos->rut, FALSE);
      gtk_box_pack_start (GTK_BOX (box2), creditos->rut, FALSE, FALSE, 0);
      gtk_widget_show (creditos->rut);
      label = gtk_label_new (" - ");
      gtk_widget_set_sensitive (label, FALSE);
      gtk_widget_show (label);
      gtk_box_pack_start (GTK_BOX (box2), label, FALSE, FALSE, 0);
      creditos->rut_ver = gtk_entry_new_with_max_length (1);
      gtk_entry_set_text (GTK_ENTRY (creditos->rut_ver), rut_ver);
      gtk_widget_set_size_request (creditos->rut_ver, 25, -1);
      gtk_widget_set_sensitive (creditos->rut_ver, FALSE);
      gtk_widget_show (creditos->rut_ver);
      gtk_box_pack_start (GTK_BOX (box2), creditos->rut_ver, FALSE, FALSE, 0);

      gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);
      
      box = gtk_vbox_new (TRUE, 2);
      gtk_widget_show (box);
      label = gtk_label_new ("Direccion");
      gtk_widget_show (label);
      gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
      creditos->direccion = gtk_entry_new_with_max_length (150);
      gtk_entry_set_text (GTK_ENTRY (creditos->direccion), direccion);
      gtk_widget_set_size_request (creditos->direccion, 150, -1);
      gtk_box_pack_end (GTK_BOX (box), creditos->direccion, FALSE, FALSE, 0);
      gtk_widget_show (creditos->direccion);
      
      gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);
      
      hbox = gtk_hbox_new (FALSE, 3);
      gtk_widget_show (hbox);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
      
      box = gtk_vbox_new (TRUE, 2);
      gtk_widget_show (box);
      label = gtk_label_new ("Teléfono");
      gtk_widget_show (label);
      gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
      creditos->fono = gtk_entry_new_with_max_length (60);
      gtk_entry_set_text (GTK_ENTRY (creditos->fono), fono);
      gtk_widget_set_size_request (creditos->fono, 90, -1);
      gtk_box_pack_start (GTK_BOX (box), creditos->fono, FALSE, FALSE, 0);
      gtk_widget_show (creditos->fono);
      
      gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);

      box = gtk_vbox_new (TRUE, 2);
      gtk_widget_show (box);
      label = gtk_label_new ("Giro");
      gtk_widget_show (label);
      gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
      creditos->giro = gtk_entry_new_with_max_length (60);
      gtk_entry_set_text (GTK_ENTRY (creditos->giro), giro);
      gtk_widget_set_size_request (creditos->giro, 90, -1);
      gtk_box_pack_start (GTK_BOX (box), creditos->giro, FALSE, FALSE, 0);
      gtk_widget_show (creditos->giro);
      
      gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);

      box = gtk_vbox_new (TRUE, 2);
      gtk_widget_show (box);
      label = gtk_label_new ("Limite de Crédito");
      gtk_widget_show (label);
      gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
      creditos->credito = gtk_entry_new_with_max_length (20);
      gtk_entry_set_text (GTK_ENTRY (creditos->credito), credito);
      gtk_widget_set_size_request (creditos->credito, 90, -1);
      gtk_box_pack_start (GTK_BOX (box), creditos->credito, FALSE, FALSE, 0);
      gtk_widget_show (creditos->credito);
      
      gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);
      
      hbox = gtk_hbox_new (FALSE, 3);
      gtk_widget_show (hbox);
      gtk_box_pack_end (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
      
      button = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
      gtk_widget_show (button);
      gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 3);
      
      g_signal_connect (G_OBJECT (button), "clicked",
			G_CALLBACK (CloseModificarWindow), NULL);
      
      button = gtk_button_new_from_stock (GTK_STOCK_SAVE);
      gtk_widget_show (button);
      gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);

      g_signal_connect (G_OBJECT (button), "clicked",
			G_CALLBACK (ModificarClienteDB), NULL);
    }
}

void
CloseClientStatus (void)
{
  gtk_widget_destroy (creditos->status_window);
  
  creditos->status_window = NULL;
}

void
ClientStatus (void)
{
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *button;
  GtkWidget *table;
  GtkWidget *label;

  GtkTreeIter iter;
  gint rut;
  
  if (gtk_tree_selection_get_selected (creditos->selection, NULL, &iter) == TRUE)
    if (creditos->status_window == NULL)
      {
	creditos->status_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (creditos->status_window),
			      "Estado actual del Cliente");
	gtk_window_set_resizable (GTK_WINDOW (creditos->status_window),
				  FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (creditos->status_window), 8);
	gtk_widget_set_size_request (creditos->status_window, 270, -1);
	g_signal_connect (G_OBJECT (creditos->status_window), "destroy",
			  G_CALLBACK (CloseClientStatus), NULL);

	gtk_widget_show (creditos->status_window);

	vbox = gtk_vbox_new (FALSE, 3);
	gtk_widget_show (vbox);
	gtk_container_add (GTK_CONTAINER (creditos->status_window), vbox);

	gtk_tree_model_get (GTK_TREE_MODEL (creditos->store), &iter,
			    0, &rut,
			    -1);

	table = gtk_table_new (8, 2, FALSE);
	gtk_widget_show (table);
	gtk_table_set_row_spacings (GTK_TABLE (table), 5);
	gtk_table_set_col_spacings (GTK_TABLE (table), 3);
	gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 3);

	label = gtk_label_new ("Nombre: ");
	gtk_widget_show (label);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
	gtk_table_attach_defaults (GTK_TABLE (table), label,
				   0, 1,
				   0, 1);

	label = gtk_label_new (ReturnClientName (rut));
	gtk_widget_show (label);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
	gtk_table_attach_defaults (GTK_TABLE (table), label,
				   1, 2,
				   0, 1);

	label = gtk_label_new ("Código: ");
	gtk_widget_show (label);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
	gtk_table_attach_defaults (GTK_TABLE (table), label,
				   0, 1,
				   1, 2);

	label = gtk_label_new (g_strdup_printf ("%d", rut));
	gtk_widget_show (label);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
	gtk_table_attach_defaults (GTK_TABLE (table), label,
				   1, 2,
				   1, 2);

	label = gtk_label_new ("Teléfono: ");
	gtk_widget_show (label);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
	gtk_table_attach_defaults (GTK_TABLE (table), label,
				   0, 1,
				   2, 3);

	label = gtk_label_new (ReturnClientPhone (rut));
	gtk_widget_show (label);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
	gtk_table_attach_defaults (GTK_TABLE (table), label,
				   1, 2,
				   2, 3);

	label = gtk_label_new ("Direccion: ");
	gtk_widget_show (label);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
	gtk_table_attach_defaults (GTK_TABLE (table), label,
				   0, 1,
				   3, 4);

	label = gtk_label_new (ReturnClientAdress (rut));
	gtk_widget_show (label);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
	gtk_table_attach_defaults (GTK_TABLE (table), label,
				   1, 2,
				   3, 4);

	label = gtk_label_new ("Estado del Crédito: ");
	gtk_widget_show (label);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
	gtk_table_attach_defaults (GTK_TABLE (table), label,
				   0, 1,
				   4, 5);

	label = gtk_label_new (ReturnClientStatusCredit (rut));
	gtk_widget_show (label);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
	gtk_table_attach_defaults (GTK_TABLE (table), label,
				   1, 2,
				   4, 5);

	label = gtk_label_new ("Cupo: ");
	gtk_widget_show (label);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
	gtk_table_attach_defaults (GTK_TABLE (table), label,
				   0, 1,
				   5, 6);

	label = gtk_label_new (g_strdup_printf ("$%s", ReturnClientCredit (rut)));
	gtk_widget_show (label);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
	gtk_table_attach_defaults (GTK_TABLE (table), label,
				   1, 2,
				   5, 6);

	label = gtk_label_new ("Saldo: ");
	gtk_widget_show (label);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
	gtk_table_attach_defaults (GTK_TABLE (table), label,
				   0, 1,
				   6, 7);

	label = gtk_label_new (g_strdup_printf ("$%d", CreditoDisponible (rut)));
	gtk_widget_show (label);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
	gtk_table_attach_defaults (GTK_TABLE (table), label,
				   1, 2,
				   6, 7);

	label = gtk_label_new ("Deuda: ");
	gtk_widget_show (label);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
	gtk_table_attach_defaults (GTK_TABLE (table), label,
				   0, 1,
				   7, 8);

	label = gtk_label_new (g_strdup_printf ("$%s", PutPoints (g_strdup_printf ("%d", DeudaTotalCliente (rut)))));
	gtk_widget_show (label);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
	gtk_table_attach_defaults (GTK_TABLE (table), label,
				   1, 2,
				   7, 8);

	hbox = gtk_hbox_new (FALSE, 3);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

	button = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
	gtk_widget_show (button);
	gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);

	g_signal_connect (G_OBJECT (button), "clicked",
			  G_CALLBACK (CloseClientStatus), NULL);
      }
}

gboolean
VentaPosible (gint rut, gint total_venta)
{
  gint saldo = CreditoDisponible (rut);

  if (total_venta <= saldo)
    return TRUE;
  else
    return FALSE;
}

gint
ToggleClientCredit (GtkCellRendererToggle *toggle, char *path_str, gpointer data)
{
  gboolean enable;
  gint rut;
  GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
  GtkTreeIter iter;

  gtk_tree_model_get_iter (GTK_TREE_MODEL (creditos->store), &iter, path);
  gtk_tree_model_get (GTK_TREE_MODEL (creditos->store), &iter, 3, &enable, -1);
  gtk_tree_model_get (GTK_TREE_MODEL (creditos->store), &iter, 0, &rut, -1);

  if (enable == TRUE)
    enable = FALSE;
  else
    enable = TRUE;

  gtk_list_store_set (GTK_LIST_STORE (creditos->store), &iter, 3, enable);

  gtk_tree_path_free (path);

  ChangeEnableCredit (enable, rut);


  return 0;
}

void
EliminarCliente (void)
{
  gint rut;
  GtkTreeIter iter;

  if (gtk_tree_selection_get_selected (creditos->selection, NULL, &iter) == TRUE && user_data->user_id == 1)
    {
      gtk_tree_model_get (GTK_TREE_MODEL (creditos->store), &iter, 
			  0, &rut, 
			  -1);

      if (ClientDelete (rut) == TRUE)
	{
	  ExitoMSG (GTK_WIDGET (creditos->store), "El Client fue eliminado con exito");
	  FillClientStore (creditos->store);
	}
      else
	ErrorMSG (GTK_WIDGET (creditos->selection), "No se pudo elminar el cliente");
    }
}

void
ModificarClienteDB (void)
{
  gchar *nombre = g_strdup (gtk_entry_get_text (GTK_ENTRY (creditos->entry_nombres)));
  gchar *paterno = g_strdup (gtk_entry_get_text (GTK_ENTRY (creditos->paterno)));
  gchar *materno = g_strdup (gtk_entry_get_text (GTK_ENTRY (creditos->materno)));
  gchar *rut = g_strdup (gtk_entry_get_text (GTK_ENTRY (creditos->rut)));
  gchar *ver = g_strdup (gtk_entry_get_text (GTK_ENTRY (creditos->rut_ver)));
  gchar *direccion = g_strdup (gtk_entry_get_text (GTK_ENTRY (creditos->direccion)));
  gchar *fono = g_strdup (gtk_entry_get_text (GTK_ENTRY (creditos->fono)));
  gchar *giro = g_strdup (gtk_entry_get_text (GTK_ENTRY (creditos->giro)));
  gint credito = atoi (g_strdup (gtk_entry_get_text (GTK_ENTRY (creditos->credito))));

  if (strcmp (nombre, "") == 0)
    AlertMSG (creditos->entry_nombres, "El Campo Nombre no puede estar vacio");
  else if (strcmp (paterno, "") == 0)
    AlertMSG (creditos->paterno, "El Apellido Paterno no puede estar vacio");
  else if (strcmp (materno, "") == 0)
    AlertMSG (creditos->materno, "El Apellido Materno no puede estar vacio");
  else if (strcmp (rut, "") == 0)
    AlertMSG (creditos->rut, "El Campo rut no debe estar vacio");
  else if (strcmp (ver, "") == 0)
    AlertMSG (creditos->rut_ver, "El Campo verificador del ru no puede estar vacio");
  else if (strcmp (direccion, "") == 0)
    AlertMSG (creditos->direccion, "La direccion no puede estar vacia");
  else if (strcmp (fono, "") == 0)
    AlertMSG (creditos->fono, "El campo telefonico no puede estar vacio");
  else if (strcmp (giro, "") == 0)
    AlertMSG (creditos->giro, "El campo giro no puede estar vaci�");
  else if (credito == 0)
    AlertMSG (creditos->credito, "El campo credito no puede estar vacio");
  else
    {
      if (VerificarRut (rut, ver) == TRUE)
	{
	  
	  if (EjecutarSQL (g_strdup_printf ("UPDATE clientes SET nombre='%s', apellido_paterno='%s', apellido_materno='%s', "
					    "direccion='%s', telefono='%s', credito=%d, giro='%s' WHERE rut=%d", 
					    nombre, paterno, materno, direccion, fono, credito, giro, atoi (rut))) != NULL)
	    ExitoMSG (GTK_WIDGET (creditos->store), "Se modificaron los datos con exito");
	    else
	    ErrorMSG (GTK_WIDGET (creditos->store), "No se puedieron modificar los datos");

	  CloseModificarWindow ();
	  FillClientStore (creditos->store);
	}
      else
	AlertMSG (creditos->rut, "El Rut no es valido!!");
    }
}

gint
LimiteCredito (gchar *rut)
{
  PGresult *res;
  
  res = EjecutarSQL (g_strdup_printf ("SELECT credito FROM cliente WHERE rut='%s'", rut));
  
  if (res != NULL)
    return atoi (PQgetvalue (res, 0, 0));
  else
    return -1;
}

