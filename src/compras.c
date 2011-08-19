/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4;
   c-indentation-style: gnu -*- */
/* compras.c
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
#define _XOPEN_SOURCE 600

#include<gtk/gtk.h>
#include<gdk/gdkkeysyms.h>

#include<stdlib.h>
#include<string.h>
#include<ctype.h>
#include<math.h>
#include"tipos.h"

#include"compras.h"

#include"ventas.h"
#include"credito.h"
#include"administracion_productos.h"
#include"postgres-functions.h"
#include"manejo_productos.h"
#include"proveedores.h"
#include"errors.h"
#include"caja.h"

#include"printing.h"
#include"utils.h"
#include"config_file.h"
#include"encriptar.h"
#include"rizoma_errors.h"

GtkBuilder *builder;

GtkWidget *entry_plazo;

gint tipo_traspaso = 1;
gint calcular = 0;

// Representa el numero de filas que se han 
// borrado en el filtro de busqueda
gint numFilasBorradas = 0;

gchar *rut_proveedor_global = NULL;

/* Contienen las filas del treeview que son borradas */
GArray *rutBorrado = NULL, *nombreBorrado = NULL, 
  *descripcionBorrada = NULL, *lapRepBorrado = NULL;

void
toggle_cell (GtkCellRendererToggle *cellrenderertoggle,
             gchar *path_string, gpointer user_data)
{
  GtkWidget *tree = (GtkWidget *) user_data;
  GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree));
  GtkTreePath *path = gtk_tree_path_new_from_string (path_string);
  gboolean toggle_item;
  GtkTreeIter iter;
  gint column = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (cellrenderertoggle), "column"));
  gchar *barcode;
  Producto *producto;

  gtk_tree_model_get_iter (model, &iter, path);

  gtk_tree_model_get (model, &iter,
                      0, &barcode,
                      column, &toggle_item,
                      -1);

  producto = SearchProductByBarcode (barcode, TRUE);

  toggle_item ^= 1;

  producto->canjear = toggle_item;

  gtk_tree_store_set (GTK_TREE_STORE (model), &iter,
                      column, toggle_item,
                      -1);

  gtk_tree_path_free (path);
}


/**
 * This function will edit the cell
 * Can edit a cell of G_TYPE STRING or INT
 *
 * //TODO: About arguments - Completar
 * @param cellrenderertext
 * @param path_string
 * @param new_string
 * @param user_data the user data
 */
void
edit_cell (GtkCellRendererText *cellrenderertext,
           gchar *path_string, gchar *new_string, gpointer user_data)
{
  GtkWidget *tree = (GtkWidget *) user_data;

  /* We get back the model */
  GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree));

  /* We get the path to the cell from the string */
  GtkTreePath *path = gtk_tree_path_new_from_string (path_string);
  GtkTreeIter iter;

  gchar *barcode;
  Producto *producto;

  /* What's the column number? */
  gint column = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (cellrenderertext), "column"));

  /* We get the type of the column */
  GType column_type = gtk_tree_model_get_column_type (model, column);

  gtk_tree_model_get_iter (model, &iter, path);

  gtk_tree_model_get (model, &iter,
                      0, &barcode,
                      -1);

  producto = SearchProductByBarcode (barcode, TRUE);

  switch (column_type)
    {
    case G_TYPE_INT:
      {
        gint new_number;                // Our new number
        new_number = atoi (new_string); // We get a string so we have to use atoi()

        if (new_number > (producto->stock_pro + (producto->stock_pro * ((double)1 / producto->tasa_canje))))
          {
            AlertMSG (tree, "No Puede canjear mas del stock en pro");
            gtk_tree_path_free (path);
            return;
          }

        /* We set the new value */
        gtk_tree_store_set (GTK_TREE_STORE (model), &iter,
                            column, new_number,
                            -1);

        producto->cuanto = new_number;
      }
      break;
    }
  gtk_tree_path_free (path);
}


/**
 * Build "wnd_compras" - rizoma-compras's window
 * initiallizes and show all its components
 * @param void
 */
void
compras_win (void)
{
  GtkWidget *compras_gui;
  gchar *fullscreen_opt = NULL;

  GtkListStore *store;
  GtkTreeView *treeview;

  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;

  GtkTreeSelection *selection;

  GError *error = NULL;

  /*ComboBox - PrecioCompra*/
  /*GtkComboBox *combo;
    GtkTreeIter iter;
    GtkListStore *modelo;
    GtkCellRenderer *cell2;*/

  compra = (Compra *) g_malloc (sizeof (Compra));
  compra->header = NULL;
  compra->products_list = NULL;
  compra->header_compra = NULL;
  compra->products_compra = NULL;
  compra->current = NULL;

  compra->documentos = NULL;

  builder = gtk_builder_new ();

  gtk_builder_add_from_file (builder, DATADIR"/ui/rizoma-compras.ui", &error);

  if (error != NULL)
    g_printerr ("%s\n", error->message);

  gtk_builder_add_from_file (builder, DATADIR"/ui/rizoma-common.ui", &error);

  if (error != NULL)
    g_printerr ("%s\n", error->message);

  gtk_builder_connect_signals (builder, NULL);

  compras_gui = GTK_WIDGET (gtk_builder_get_object (builder, "wnd_compras"));

  if (rizoma_get_value_boolean ("TRASPASO"))
    {
      gtk_widget_show (GTK_WIDGET (builder_get (builder, "btn_traspaso_enviar")));
      gtk_widget_show (GTK_WIDGET (builder_get (builder, "btn_traspaso_recibir")));
      gtk_widget_show (GTK_WIDGET (builder_get (builder, "btn_traspaso")));

      gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_traspaso_enviar")), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_traspaso_recibir")), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_traspaso")), FALSE);
    }

  // check if the window must be set to fullscreen
  fullscreen_opt = rizoma_get_value ("FULLSCREEN");

  if ((fullscreen_opt != NULL) && (g_str_equal (fullscreen_opt, "YES")))
    gtk_window_fullscreen (GTK_WINDOW (compras_gui));

  /* History TreeView */
  store = gtk_list_store_new (7,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_DOUBLE,
                              G_TYPE_DOUBLE,  //Costo sin impuestos
			      G_TYPE_DOUBLE,  //Costo con impuestos
			      G_TYPE_DOUBLE); //Total con impuestos

  treeview = GTK_TREE_VIEW (gtk_builder_get_object (builder, "product_history_tree_view"));
  gtk_tree_view_set_model (GTK_TREE_VIEW (treeview), GTK_TREE_MODEL (store));

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Fecha", renderer,
                                                     "text", 0,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Id", renderer,
                                                     "text", 1,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Proveedor", renderer,
                                                     "text", 2,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Cantidad", renderer,
                                                     "text", 3,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)3, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("P.Unit S/Imp", renderer,
                                                     "text", 4,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)4, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("P.Unit C/Imp", renderer,
                                                     "text", 5,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)5, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Total", renderer,
                                                     "text", 6,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)6, NULL);


  /* End History TreeView */

  /* Buying Products TreeView */

  store = gtk_list_store_new (5,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_DOUBLE,
                              G_TYPE_STRING,
                              G_TYPE_STRING);

  if (compra->header_compra != NULL)
    {
      compra->products_compra = compra->header_compra;
      do
	{
	  compra->current = compra->products_compra->product;
	  AddToTree ();
	  compra->products_compra = compra->products_compra->next;
	} while (compra->products_compra != compra->header_compra);
    }

  treeview = GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_products_buy_list"));
  gtk_tree_view_set_model (GTK_TREE_VIEW (treeview), GTK_TREE_MODEL (store));

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Codigo Producto", renderer,
                                                     "text", 0,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Producto", renderer,
                                                     "text", 1,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Cant. Sol.", renderer,
                                                     "text", 2,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_resizable (column, FALSE);

  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)2, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("P. Unitario", renderer,
                                                     "text", 3,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Total", renderer,
                                                     "text", 4,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);


  /* End Buying Products TreeView*/


  /* Pending Requests */

  store = gtk_list_store_new (6,
                              G_TYPE_INT,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_BOOLEAN);

  treeview = GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_pending_requests"));
  gtk_tree_view_set_model (GTK_TREE_VIEW (treeview), GTK_TREE_MODEL (store));

  selection = gtk_tree_view_get_selection (treeview);

  gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);

  g_signal_connect (G_OBJECT (selection), "changed",
                    G_CALLBACK (IngresoDetalle), NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("ID", renderer,
                                                     "text", 0,
                                                     "foreground", 4,
                                                     "foreground-set", 5,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Fecha", renderer,
                                                     "text", 1,
                                                     "foreground", 4,
                                                     "foreground-set", 5,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Proveedor", renderer,
                                                     "text", 2,
                                                     "foreground", 4,
                                                     "foreground-set", 5,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Precio Total", renderer,
                                                     "text", 3,
                                                     "foreground", 4,
                                                     "foreground-set", 5,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);


  /* End Pending Requests */

  /* Pending Request Detail */

  store = gtk_list_store_new (8,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_DOUBLE,
                              G_TYPE_DOUBLE,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_BOOLEAN);

  treeview = GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_pending_request_detail"));
  gtk_tree_view_set_model (GTK_TREE_VIEW (treeview), GTK_TREE_MODEL (store));

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Codigo", renderer,
                                                     "text", 0,
                                                     "foreground", 6,
                                                     "foreground-set", 7,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Producto", renderer,
                                                     "text", 1,
                                                     "foreground", 6,
                                                     "foreground-set", 7,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Unit.", renderer,
                                                     "text", 2,
                                                     "foreground", 6,
                                                     "foreground-set", 7,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Cant. Sol.", renderer,
                                                     "text", 3,
                                                     "foreground", 6,
                                                     "foreground-set", 7,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)3, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Cant. Ing.", renderer,
                                                     "text", 4,
                                                     "foreground", 6,
                                                     "foreground-set", 7,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)4, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Sub-Total", renderer,
                                                     "text", 5,
                                                     "foreground", 6,
                                                     "foreground-set", 7,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  /* End Pending Request Detail */

  /* Guide -> Invoice */

  store = gtk_list_store_new (3,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_STRING);

  treeview = GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_pending_guide"));
  gtk_tree_view_set_model (GTK_TREE_VIEW (treeview), GTK_TREE_MODEL (store));

  g_signal_connect (G_OBJECT (gtk_tree_view_get_selection (treeview)), "changed",
                    G_CALLBACK (on_tree_selection_pending_guide_changed), NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("ID Guia", renderer,
                                                     "text", 0,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Nº Guia", renderer,
                                                     "text", 1,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Monto", renderer,
                                                     "text", 2,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  store = gtk_list_store_new (3,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_STRING);

  treeview = GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_guide_invoice"));
  gtk_tree_view_set_model (GTK_TREE_VIEW (treeview), GTK_TREE_MODEL (store));

  g_signal_connect (G_OBJECT (gtk_tree_view_get_model (treeview)), "row-changed",
                    G_CALLBACK (CalcularTotalesGuias), NULL);

  g_signal_connect (G_OBJECT (gtk_tree_view_get_model (treeview)), "row-deleted",
                    G_CALLBACK (CalcularTotalesGuias), NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("ID Guia", renderer,
                                                     "text", 0,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Nº Guia", renderer,
                                                     "text", 1,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Monto", renderer,
                                                     "text", 2,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  store = gtk_list_store_new (5,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_DOUBLE,
                              G_TYPE_DOUBLE);

  treeview = GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_guide_invoice_detail"));
  gtk_tree_view_set_model (GTK_TREE_VIEW (treeview), GTK_TREE_MODEL (store));

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Codigo", renderer,
                                                     "text", 0,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Producto", renderer,
                                                     "text", 1,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Unit.", renderer,
                                                     "text", 2,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Cant.", renderer,
                                                     "text", 3,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)3, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Sub-Total", renderer,
                                                     "text", 4,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)4, NULL);

  /* End Guide -> Invoice */

  /* Pay Invoices */

  store = gtk_list_store_new (7,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_STRING);

  treeview = GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_invoice_list"));
  gtk_tree_view_set_model (GTK_TREE_VIEW (treeview), GTK_TREE_MODEL (store));

  selection = gtk_tree_view_get_selection (treeview);

  gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);

  g_signal_connect (G_OBJECT (selection), "changed",
                    G_CALLBACK (on_tree_view_invoice_list_selection_changed), NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("ID", renderer,
                                                     "text", 0,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);


  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Rut", renderer,
                                                     "text", 1,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Numero", renderer,
                                                     "text", 2,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Compra", renderer,
                                                     "text", 3,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("F. Emision", renderer,
                                                     "text", 4,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Fecha Pagos", renderer,
                                                     "text", 5,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Monto", renderer,
                                                     "text", 6,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  store = gtk_list_store_new (5,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_DOUBLE,
                              G_TYPE_DOUBLE);

  treeview = GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_invoice_detail"));
  gtk_tree_view_set_model (GTK_TREE_VIEW (treeview), GTK_TREE_MODEL (store));

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Codigo", renderer,
                                                     "text", 0,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Producto", renderer,
                                                     "text", 1,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Unit.", renderer,
                                                     "text", 2,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Cant.", renderer,
                                                     "text", 3,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)3, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Sub-Total", renderer,
                                                     "text", 4,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)4, NULL);

  /*ComboBox - PrecioCompra*/
  /*modelo = gtk_list_store_new (1, G_TYPE_STRING);
    combo = (GtkComboBox *) gtk_builder_get_object (builder, "cmbPrecioCompra");
    cell2 = gtk_cell_renderer_text_new ();
    gtk_cell_layout_pack_start (GTK_CELL_LAYOUT(combo), cell2, TRUE);
    gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT(combo), cell2, "text", 0, NULL);*/

  /*Se agregan las opciones al combobox*/
  /*gtk_list_store_append (modelo, &iter);
    gtk_list_store_set (modelo, &iter, 0, "Precio neto", -1);

    gtk_list_store_append (modelo, &iter);
    gtk_list_store_set (modelo, &iter, 0, "Precio bruto", -1);

    gtk_combo_box_set_model (combo, (GtkTreeModel *)modelo);
    gtk_combo_box_set_active (combo, 0);*/

  /* End Pay Invoices */

  //mercaderia
  admini_box();

  //setup the providers tab
  proveedores_box();

  //Default disable entry and buttons
  //TODO: Ver como simplificar el set_sensitive
  gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "entry_buy_price")), FALSE);
  gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "entry_buy_gain")), FALSE);
  gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "entry_sell_price")), FALSE);
  gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "entry_buy_amount")), FALSE);
  gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "edit_product_button")), FALSE);
  gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "button_calculate")), FALSE);
  gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "button_add_product_list")), FALSE);
  gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "button_buy")), FALSE);
  //Compras - Mercadería
  //gtk_widget_set_sensitive (GTK_ENTRY (gtk_builder_get_object (builder, "btn_infomerca_save")), FALSE);
  //Compras - Mercadería - Editar Producto
  gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "entry_edit_prod_shortcode")), FALSE);
  gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "entry_edit_prod_barcode")), FALSE);


  //Restricciones a los entry 

  //Pestaña compras (barcode, costo, margen, precio y cantidad)
  gtk_entry_set_max_length (GTK_ENTRY (builder_get (builder, "entry_buy_barcode")), 25);
  gtk_entry_set_max_length (GTK_ENTRY (builder_get (builder, "entry_buy_price")), 9);
  gtk_entry_set_max_length (GTK_ENTRY (builder_get (builder, "entry_buy_gain")), 4);
  gtk_entry_set_max_length (GTK_ENTRY (builder_get (builder, "entry_sell_price")), 9);
  gtk_entry_set_max_length (GTK_ENTRY (builder_get (builder, "entry_buy_amount")), 6);

  //Pestaña mercaderías
  gtk_entry_set_max_length (GTK_ENTRY (builder_get (builder, "entry_infomerca_percentmargin")), 7);
  gtk_entry_set_max_length (GTK_ENTRY (builder_get (builder, "entry_informerca_price")), 9);
  gtk_entry_set_max_length (GTK_ENTRY (builder_get (builder, "entry_informerca_cantmayorist")), 4);
  gtk_entry_set_max_length (GTK_ENTRY (builder_get (builder, "entry_informerca_pricemayorist")), 9);
  gtk_entry_set_max_length (GTK_ENTRY (builder_get (builder, "entry_informerca_minstock")), 6);

  //Anulación de compras solo puede ser efectuada por el administrador
  if (user_data->user_id != 1)
    gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "btn_nullify_buy_pi")), FALSE);

  //Focus Control
  //TODO: Solucionar el foco de las pestañas (El foco debe cambiarse a los entrys principales)
  gtk_widget_set_can_focus (GTK_NOTEBOOK (builder_get (builder, "buy_notebook")), TRUE);
  gtk_widget_grab_focus (GTK_WIDGET (builder_get (builder, "entry_buy_barcode")));
  
  //Se oculta la pestaña "Ingreso Facturas" hasta que se termine la funcionalidad
  gtk_notebook_remove_page (GTK_NOTEBOOK (builder_get (builder, "buy_notebook")), 2);
  //Se oculta la opción para ingresar guias
  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "radiobutton8")), FALSE);

  gtk_widget_show_all (compras_gui);
} // compras_win (void)


void
SearchProductHistory (GtkEntry *entry, gchar *barcode)
{
  gdouble day_to_sell;
  PGresult *res;
  gchar *q;
  gchar *codigo;

  codigo = barcode;
  q = g_strdup_printf ("select barcode "
		       "from codigo_corto_to_barcode('%s')",
		       barcode);
  res = EjecutarSQL(q);

  /*Si entra aqui significa que se ingresó un codigo corto, y se pasa a codigo de barra*/
  if (PQntuples (res) == 1)
    {
      barcode = g_strdup (PQvaluebycol(res, 0, "barcode"));
      PQclear (res);
      gtk_entry_set_text (GTK_ENTRY (entry),barcode);
    }

  g_free(q);
  q = g_strdup_printf ("SELECT existe_producto(%s)", barcode);

  if (g_str_equal( GetDataByOne (q), "t"))
    {
      g_free(q);
      q = g_strdup_printf ("SELECT estado FROM producto where barcode = '%s'", barcode);
      res = EjecutarSQL(q);

      /*Para asegurarse de que el producto no haya sido inhabilitado (borrado) por el administrador*/
      if (strcmp (PQvaluebycol (res, 0, "estado"),"f") == 0)
	{
	  GtkWidget *aux_widget;
	  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "compras_gui"));
	  gchar *str = g_strdup_printf("El código %s no está disponible, fué invalidado por el administrador", codigo);
	  AlertMSG (aux_widget, str);

	  //Limipia todo antes de irse
	  gtk_entry_set_text (GTK_ENTRY (builder_get (builder, "entry_buy_barcode")), "");
	  g_free (str);
	  g_free (q);
	  g_free (codigo);
	  PQclear (res);
	  return;
	}

      ShowProductHistory ();

      q = g_strdup_printf ("select * from informacion_producto (%s, '')", barcode);
      res = EjecutarSQL (q);
      g_free (q);

      gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_stock")),
                            g_strdup_printf ("<span weight=\"ultrabold\">%.2f</span>", strtod (PUT(PQvaluebycol (res, 0, "stock")), (char **)NULL)));

      day_to_sell = atoi (PQvaluebycol (res, 0, "stock_day"));

      if (day_to_sell != 0)
        {
          gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_stock_further")),
                                g_strdup_printf ("<span weight=\"ultrabold\">%.2f dias"
                                                 "</span>", day_to_sell));
        }
      else
        {
          gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_stock_further")),
                                g_strdup_printf ("<span weight=\"ultrabold\">indefinidos dias</span>"));
        }

      gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_sell_price")),
                            g_strdup_printf ("<span weight=\"ultrabold\">%s/%s</span>",
                                             PutPoints (PQvaluebycol (res, 0, "precio")), PutPoints (PQvaluebycol (res, 0, "precio_mayor"))));

      gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_product_desc")),
                            g_strdup_printf ("<span weight=\"ultrabold\">%s</span>", PQvaluebycol (res, 0, "descripcion")));

      gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_mark")),
                            g_strdup_printf ("<span weight=\"ultrabold\">%s</span>", PQvaluebycol (res, 0, "marca")));

      gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_cont")),
                            g_strdup_printf ("<span weight=\"ultrabold\">%s</span>", PQvaluebycol (res, 0, "contenido")));

      gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_unit")),
                            g_strdup_printf ("<span weight=\"ultrabold\">%s</span>", PQvaluebycol (res, 0, "unidad")));

      gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_fifo")),
                            g_strdup_printf ("<span weight=\"ultrabold\">%s</span>",
					     PUT (PQvaluebycol (res, 0, "costo_promedio"))));

      gtk_label_set_markup (GTK_LABEL (builder_get (builder, "label_code")),
                            g_strdup_printf ("<span weight=\"ultrabold\">%s</span>",PQvaluebycol (res, 0, "codigo_corto")));

      //Enable entry sensitive
      //TODO: Buscar la forma de simplificar los sensitive
      gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "button_new_product")), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "entry_buy_barcode")), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "entry_buy_price")), TRUE);
      gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "entry_buy_gain")), TRUE);
      gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "entry_sell_price")), TRUE);
      gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "edit_product_button")), TRUE);
      gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "button_calculate")), TRUE);

      // Se habilita el botón de traspaso
      if (rizoma_get_value_boolean ("TRASPASO"))
	  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_traspaso")), TRUE);

      gtk_widget_grab_focus (GTK_WIDGET (builder_get (builder, "entry_buy_price")));
    } // if (g_str_equal( GetDataByOne (q), "t"))
  else
    {
      gtk_widget_grab_focus (GTK_WIDGET (builder_get (builder, "button_new_product")));
    }
} // SearchProductHistory (GtkEntry *entry, gchar *barcode)


/**
 * Callback connected to the button of the dialog used to modificate a
 * product in the 'Mercaderia' tab
 *
 * @param widget the widget that emited the signal
 * @param data the user data
 */
void
Save (GtkWidget *widget, gpointer data)
{
  GtkWidget *aux_widget;
  GtkTreeModel *model;
  GtkTreeIter iter;

  GtkWidget *combo_imp;
  gchar *barcode;
  gchar *codigo;
  gchar *description;
  gchar *marca;
  gchar *unidad;
  gchar *contenido;
  gchar *precio;
  gint otros;
  char *familia;
  gboolean iva;
  gboolean fraccion;
  gboolean perecible;

  GtkComboBox *combo_unit = GTK_COMBO_BOX (builder_get (builder, "cmb_box_edit_product_unit"));
  gint tab = gtk_notebook_get_current_page ( GTK_NOTEBOOK (builder_get (builder, "buy_notebook")));

  aux_widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_edit_prod_barcode"));
  barcode = g_strdup (gtk_entry_get_text (GTK_ENTRY (aux_widget)));

  aux_widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_edit_prod_shortcode"));
  codigo = g_strdup (gtk_entry_get_text (GTK_ENTRY (aux_widget)));

  aux_widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_edit_prod_desc"));
  description = g_strdup (gtk_entry_get_text (GTK_ENTRY (aux_widget)));

  aux_widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_edit_prod_brand"));
  marca = g_strdup (gtk_entry_get_text (GTK_ENTRY (aux_widget)));

  /* aux_widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_edit_prod_unit")); */
  /* unidad = g_strdup (gtk_entry_get_text (GTK_ENTRY (aux_widget))); */

  aux_widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_edit_prod_content"));
  contenido = g_strdup (gtk_entry_get_text (GTK_ENTRY (aux_widget)));

  aux_widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_edit_prod_price"));
  precio = g_strdup (gtk_entry_get_text (GTK_ENTRY (aux_widget)));

  aux_widget = GTK_WIDGET (gtk_builder_get_object(builder, "checkbtn_edit_prod_fraccionaria"));
  fraccion = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(aux_widget));

  aux_widget = GTK_WIDGET (gtk_builder_get_object(builder, "checkbtn_edit_prod_perecible"));
  perecible = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(aux_widget));

  aux_widget = GTK_WIDGET (gtk_builder_get_object(builder, "checkbtn_edit_prod_iva"));
  iva = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(aux_widget));

  combo_imp = GTK_WIDGET (gtk_builder_get_object(builder, "cmbbox_edit_prod_extratax"));

  if (gtk_combo_box_get_active (GTK_COMBO_BOX (combo_imp)) != -1)
    {
      model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo_imp));
      gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combo_imp), &iter);

      gtk_tree_model_get (model, &iter,
                          0, &otros,
                          -1);
    }
  else
    otros = -1;


  /* aux_widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_edit_prod_unit")); */
  /* unidad = g_strdup (gtk_entry_get_text (GTK_ENTRY (aux_widget))); */

  model = gtk_combo_box_get_model (combo_unit);
  gtk_combo_box_get_active_iter (combo_unit, &iter);

  gtk_tree_model_get (model, &iter,
		      1, &unidad,
		      -1);

  // TODO: Revisar el Otros, porque en la base de datos se guardan 0
  // TODO: Una vez que el entry solo pueda recibir valores numéricos se puede borrar esta condición
  if (HaveCharacters(contenido))
    {
      ErrorMSG (GTK_WIDGET (builder_get (builder, "entry_edit_prod_content")),"Debe ingresar un valor numérico");
      return;
    }

  SaveModifications (codigo, description, marca, unidad, contenido, precio,
                     iva, otros, barcode, familia, perecible, fraccion);

  if (tab == 0)
    {
      SearchProductHistory (GTK_ENTRY (gtk_builder_get_object (builder, "entry_buy_barcode")), barcode);
    }
  else if (tab == 3)
    {
      FillFields (NULL, NULL);
    }
  gtk_widget_hide (GTK_WIDGET (gtk_builder_get_object (builder, "wnd_mod_product")));
} //void Save (GtkWidget *widget, gpointer data)


/**
 * Es llamada por las funciones "AddToProductsList()" y "on_button_calculate_clicked()"
 *
 * Esta funcion se encarga de calcular la cifra restante entre: porcentaje de ganancia,
 * el precio de compra, tomando en cuenta si es bruto o neto y calculando sus impuestos
 * en el caso correspondiente y el precio final.
 *
 */
void
CalcularPrecioFinal (void)
{
  gchar *barcode = g_strdup (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_buy_barcode"))));
  gdouble ingresa = strtod (PUT (g_strdup (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_buy_price"))))), (char **)NULL);
  gdouble ganancia = (gdouble) atoi (g_strdup (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_buy_gain")))));
  gdouble precio_final = (gdouble) atoi (g_strdup (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_sell_price")))));
  gdouble precio;
  gdouble porcentaje;
  gdouble iva = GetIVA (barcode);
  gdouble otros = GetOtros (barcode);

  /*
    Obtener valores cmbPrecioCompra
    GtkComboBox *combo = (GtkComboBox *) gtk_builder_get_object (builder, "cmbPrecioCompra");
    GtkTreeIter iter;
    GtkTreeModel *model = gtk_combo_box_get_model (combo);
    gtk_combo_box_get_active_iter (combo, &iter);
    gchar *opcion;
    gtk_tree_model_get (model, &iter, 0, &opcion, -1);
  */

  if (iva != -1)
    iva = (gdouble)iva / 100 + 1;
  else
    iva = -1;

  // TODO: Revisión (Concenso de la variable)
  if (otros == 0)
    otros = -1;

  /*
    IVA = 1,19;
    Z = precio final
    Y = margen;
    X = Ingresa;

    Z     1
    X = ----- * ----
    Y+1    1,19
  */

  if ((ganancia == 0 && precio_final == 0 && ingresa != 0) ||
      (ganancia == 0 && precio_final == 0 && ingresa == 0) ||
      (ganancia != 0 && precio_final == 0 && ingresa == 0)  )
    {
      ErrorMSG (GTK_WIDGET (builder_get (builder, "entry_buy_price")),"Se requieren al menos 2 valores para efectuar el cálculo");
    }

  /* -- Initial price ("ingresa") is calculated here -- */
  else if (ingresa == 0 && ganancia >= 0 && precio_final != 0)
    {
      calcular = 1; // FLAG - Permite saber que se ha realizado un cálculo.

      if (otros == -1 && iva != -1 )
        precio = (gdouble) ((gdouble)(precio_final / iva) / (gdouble) (ganancia + 100)) * 100;

      else if (iva != -1 && otros != -1)
        {
          iva = (gdouble) iva - 1;
          otros = (gdouble) otros / 100;

          precio = (gdouble) precio_final / (gdouble)(iva + otros + 1);
          precio = (gdouble) precio / (gdouble)(ganancia / 100 + 1);
        }
      else if (iva == -1 && otros == -1)
        {
          precio = (gdouble) (precio_final / (gdouble) (ganancia + 100)) * 100;
        }

      gtk_entry_set_text (GTK_ENTRY (builder_get (builder, "entry_buy_price")),
                          g_strdup_printf ("%ld", lround (precio)));
    }

  /* -- Profit porcent ("ganancia") is calculated here -- */
  else if (ganancia == 0 && ingresa != 0 && precio_final != 0)
    {
      calcular = 1; // FLAG - Permite saber que se ha realizado un cálculo.

      if (otros == -1 && iva != -1 )
        porcentaje = (gdouble) ((precio_final / (gdouble)(iva * ingresa)) -1) * 100;

      else if (iva != -1 && otros != -1 )
        {
          iva = (gdouble) iva - 1;
          otros = (gdouble) otros / 100;

          precio = (gdouble) precio_final / (gdouble)(iva + otros + 1);
          ganancia = (gdouble) precio - ingresa;
          porcentaje = (gdouble)(ganancia / ingresa) * 100;
        }
      else if (iva == -1 && otros == -1 )
        porcentaje = (gdouble) ((precio_final / ingresa) - 1) * 100;

      gtk_entry_set_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_buy_gain")),
                          g_strdup_printf ("%ld", lround (porcentaje)));
    }

  /* -- Final price ("precio_final") is calculated here -- */
  else if (precio_final == 0 && ingresa != 0 && ganancia >= 0)
    {
      calcular = 1; // FLAG - Permite saber que se ha realizado un cÃ¡lculo.

      if (otros == -1 && iva != -1 )
        precio = (gdouble) ((gdouble)(ingresa * (gdouble)(ganancia + 100)) * iva) / 100;

      else if (iva != -1 && otros != -1)
        {
          iva = (gdouble) iva - 1;
          otros = (gdouble) otros / 100;

          precio = (gdouble) ingresa + (gdouble)((gdouble)(ingresa * ganancia ) / 100);
          precio = (gdouble)((gdouble)(precio * iva) +
                             (gdouble)(precio * otros) + (gdouble) precio);
        }
      else if (iva == -1 && otros == -1)
        precio = (gdouble)(ingresa * (gdouble)(ganancia + 100)) / 100;

      if (ganancia == 0)
        gtk_entry_set_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_buy_gain")), "0");

      gtk_entry_set_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_sell_price")),
                          g_strdup_printf ("%ld", lround (precio)));
    }
  else
    ErrorMSG (GTK_WIDGET (builder_get (builder, "entry_buy_price")), "Solo 2 campos deben ser llenados");

  if(calcular == 1)
    {
      //Inhabilitar la escritura en los entry de precio cuando el cálculo de ha realizado
      //TODO: Ver como simplificar los sensitive
      gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "entry_buy_price")), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "entry_buy_gain")), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "entry_sell_price")), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "button_calculate")), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "entry_buy_amount")), TRUE);
      gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "button_add_product_list")), TRUE);
    }
} // CalcularPrecioFinal (void)


void
AddToProductsList (void)
{
  gchar *barcode = g_strdup (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_buy_barcode"))));
  gdouble cantidad;
  gdouble precio_compra = strtod (PUT(g_strdup (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_buy_price"))))), (char **)NULL);
  gint margen = atoi (g_strdup (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_buy_gain")))));
  gint precio = atoi (g_strdup (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_sell_price")))));
  Producto *check;

  if (g_str_equal (barcode, ""))
    {
      ErrorMSG (GTK_WIDGET (builder_get (builder, "entry_buy_barcode")),
		"Debe seleccionar una mercadería");
      return;
    }

  GtkListStore *store_history, *store_buy;

  store_buy = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (gtk_builder_get_object( builder, "tree_view_products_buy_list"))));

  cantidad = strtod (PUT (g_strdup (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_buy_amount"))))), (char **)NULL);

  if (precio_compra != 0 && (strcmp (GetCurrentPrice (barcode), "0") == 0 || precio != 0)
      && strcmp (barcode, "") != 0) //&& margen >= 0)
    {
      if (compra->header_compra != NULL)
        check = SearchProductByBarcode (barcode, FALSE);
      else
        check = NULL;

      if (check == NULL)
        {
          if (CompraAgregarALista (barcode, cantidad, precio, precio_compra, margen, FALSE))
            {
              AddToTree ();
            }
          else
            {
              return;
            }
        }
      else
        {
          check->cantidad += cantidad;

          gtk_list_store_set (store_buy, &check->iter,
                              2, check->cantidad,
                              4, PutPoints (g_strdup_printf ("%li", lround ((gdouble)check->cantidad * check->precio_compra))),
                              -1);
        }

      gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_total_buy")),
                            g_strdup_printf ("<span size=\"xx-large\"><b>%s</b></span>",
                                             PutPoints (g_strdup_printf
                                                        ("%li", lround (CalcularTotalCompra (compra->header_compra))))));
      store_history = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (gtk_builder_get_object( builder, "product_history_tree_view"))));
      gtk_list_store_clear (store_history);

      CleanStatusProduct ();

      //TODO: Se setea el sensitive cada vez que se agrega un producto a la lista, se debe evaluar
      gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "button_buy")), TRUE);

      gtk_widget_grab_focus (GTK_WIDGET (gtk_builder_get_object (builder, "entry_buy_barcode")));
    }
  else
    {
      CalcularPrecioFinal ();
      AddToProductsList ();
    }
} // void AddToProductsList (void)


void
AddToTree (void)
{
  GtkTreeIter iter;
  GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (gtk_builder_get_object( builder, "tree_view_products_buy_list"))));

  gtk_list_store_insert_after (store, &iter, NULL);
  gtk_list_store_set (store, &iter,
                      0, compra->current->codigo,
                      1, g_strdup_printf ("%s %s %d %s", compra->current->producto,
                                          compra->current->marca, compra->current->contenido,
                                          compra->current->unidad),
                      2, compra->current->cantidad,
                      3, PutPoints (g_strdup_printf ("%.2f", compra->current->precio_compra)),
                      4, PutPoints (g_strdup_printf ("%li", lround ((gdouble) compra->current->cantidad * compra->current->precio_compra))),
                      -1);

  compra->current->iter = iter;
}


void
MoveCursor (GtkTreeView *treeview, GtkTreePath *arg1, GtkTreeViewColumn *arg2,
            gpointer data)
{
  GtkWidget *window = (GtkWidget *) data;

  gtk_window_set_focus (GTK_WINDOW (window), compra->nota_entry);
}

void
CloseBuyWindow (void)
{
  gtk_widget_destroy (compra->buy_window);
  compra->buy_window = NULL;
}


void
BuyWindow (void)
{
  /*  PGresult *res;
      gint tuples, i;
  */
  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *label;
  GtkWidget *button;
  GtkWidget *button_add;

  GtkWidget *scroll;
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;
  GtkTreeSelection *selection;
  GtkTreeIter iter;

  gtk_widget_set_sensitive (main_window, FALSE);

  compra->buy_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_widget_set_size_request (compra->buy_window, 300, -1);
  gtk_window_set_resizable (GTK_WINDOW (compra->buy_window), FALSE);
  gtk_window_set_position (GTK_WINDOW (compra->buy_window), GTK_WIN_POS_CENTER_ALWAYS);
  gtk_window_set_title (GTK_WINDOW (compra->buy_window), "Datos Proveedor");
  //  gtk_window_set_transient_for (GTK_WINDOW (compra->buy_window), GTK_WINDOW (main_window));
  gtk_widget_show (compra->buy_window);
  gtk_window_present (GTK_WINDOW (compra->buy_window));

  g_signal_connect (G_OBJECT (compra->buy_window), "destroy",
                    G_CALLBACK (CloseBuyWindow), NULL);

  vbox = gtk_vbox_new (FALSE, 3);
  gtk_widget_show (vbox);
  gtk_container_add (GTK_CONTAINER (compra->buy_window), vbox);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  label = gtk_label_new ("Rut Proveedor    :");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);

  compra->rut_label = gtk_label_new ("");
  gtk_widget_show (compra->rut_label);
  gtk_box_pack_start (GTK_BOX (hbox), compra->rut_label, FALSE, FALSE, 3);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  label = gtk_label_new ("Nombre Proveedor :");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);

  compra->nombre_label = gtk_label_new ("");
  gtk_widget_show (compra->nombre_label);
  gtk_box_pack_start (GTK_BOX (hbox),compra->nombre_label, FALSE, FALSE, 3);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  label = gtk_label_new ("Nota Pedido");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);

  compra->nota_entry = gtk_entry_new ();
  gtk_widget_show (compra->nota_entry);
  gtk_box_pack_end (GTK_BOX (hbox), compra->nota_entry, FALSE, FALSE, 3);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  button_add = gtk_button_new_with_mnemonic ("Agregar _Proveedor");
  gtk_widget_show (button_add);
  gtk_box_pack_end (GTK_BOX (hbox), button_add, FALSE, FALSE, 3);

  g_signal_connect (G_OBJECT (button_add), "clicked",
                    G_CALLBACK (AddProveedorWindow), NULL);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  label = gtk_label_new ("Plazo de pago: ");
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
  gtk_widget_show (label);

  entry_plazo = gtk_entry_new_with_max_length (3);
  gtk_box_pack_start (GTK_BOX (hbox), entry_plazo, FALSE, FALSE, 3);
  gtk_widget_show (entry_plazo);

  g_signal_connect (G_OBJECT (compra->nota_entry), "activate",
                    G_CALLBACK (SendCursorTo), (gpointer) entry_plazo);

  label = gtk_label_new (" dias.");
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
  gtk_widget_show (label);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_end (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  button = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
  gtk_widget_show (button);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 3);

  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (CloseBuyWindow), NULL);

  button = gtk_button_new_from_stock (GTK_STOCK_OK);
  gtk_widget_show (button);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);

  g_signal_connect (G_OBJECT (entry_plazo), "activate",
                    G_CALLBACK (SendCursorTo), (gpointer)button);

  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (Comprar), NULL);

  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_show (scroll);
  gtk_widget_set_size_request (scroll, 280, 200);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (vbox), scroll, FALSE, FALSE, 3);

  compra->store_prov = gtk_list_store_new (2,
                                           G_TYPE_STRING,
                                           G_TYPE_STRING);

  compra->tree_prov = gtk_tree_view_new_with_model (GTK_TREE_MODEL (compra->store_prov));
  gtk_container_add (GTK_CONTAINER (scroll), compra->tree_prov);
  gtk_widget_show (compra->tree_prov);

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (compra->tree_prov));

  gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);

  g_signal_connect (G_OBJECT (selection), "changed",
                    G_CALLBACK (Seleccionado), NULL);

  g_signal_connect (G_OBJECT (compra->tree_prov), "row-activated",
                    G_CALLBACK (MoveCursor), (gpointer) compra->buy_window);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Rut", renderer,
                                                     "text", 0,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (compra->tree_prov), column);
  gtk_tree_view_column_set_min_width (column, 75);
  gtk_tree_view_column_set_max_width (column, 75);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Proveedor", renderer,
                                                     "text", 1,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (compra->tree_prov), column);
  gtk_tree_view_column_set_min_width (column, 200);
  gtk_tree_view_column_set_max_width (column, 200);
  gtk_tree_view_column_set_resizable (column, FALSE);

  FillProveedores ();

  if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (compra->store_prov), &iter) == FALSE)
    gtk_window_set_focus (GTK_WINDOW (compra->buy_window), button_add);

  gtk_tree_selection_select_path (selection,
                                  gtk_tree_path_new_from_string ("0"));

  gtk_window_set_focus (GTK_WINDOW (compra->buy_window), compra->tree_prov);
} // void BuyWindow (void)


void
AddFoundProduct (void)
{
  GtkTreeView *treeview = GTK_TREE_VIEW (builder_get (builder, "treeview_buscador"));
  GtkTreeIter iter;
  GtkTreeModel *model = gtk_tree_view_get_model (treeview);
  gchar *barcode;

  if (gtk_tree_selection_get_selected (gtk_tree_view_get_selection (treeview), NULL, &iter))
    {
      gtk_tree_model_get (model, &iter,
                          1, &barcode,
                          -1);

      gtk_entry_set_text (GTK_ENTRY (builder_get (builder, "entry_buy_barcode")), barcode);
      gtk_widget_hide (GTK_WIDGET (builder_get (builder, "wnd_buscador")));

      SearchProductHistory (GTK_ENTRY (builder_get (builder, "entry_buy_barcode")), barcode);
    }
}


void
SearchName (void)
{
  gchar *search_string = g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_buscar"))));
  gchar *q;
  PGresult *res;
  gint i, resultado;

  GtkTreeView *treeview = GTK_TREE_VIEW (gtk_builder_get_object (builder, "treeview_buscador"));
  GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model (treeview));
  GtkTreeIter iter;

  q = g_strdup_printf ("SELECT codigo_corto, barcode, descripcion, marca, "
                       "contenido, unidad, stock FROM buscar_productos('%s%%')",
                       search_string);
  res = EjecutarSQL (q);
  g_free (q);

  resultado = PQntuples (res);

  gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_found")),
                        g_strdup_printf ("<b>%d producto(s)</b>", resultado));

  gtk_list_store_clear (store);

  for (i = 0; i < resultado; i++)
    {
      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
                          0, PQvaluebycol (res, i, "codigo_corto"),
                          1, PQvaluebycol (res, i, "barcode"),
                          2, PQvaluebycol (res, i, "descripcion"),
                          3, PQvaluebycol (res, i, "marca"),
                          4, PQvaluebycol (res, i, "contenido"),
                          5, PQvaluebycol (res, i, "unidad"),
                          6, atoi (PQvaluebycol (res, i, "stock")),
                          -1);
    }

  if (resultado > 0)
    {
      gtk_tree_selection_select_path (gtk_tree_view_get_selection (treeview), gtk_tree_path_new_from_string ("0"));
      gtk_widget_grab_focus (GTK_WIDGET (treeview));
    }
}


void
SearchByName (void)
{
  GtkWindow *window;

  GtkListStore *store;
  GtkTreeView *treeview;

  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  gchar *string = g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_buy_barcode"))));

  treeview = GTK_TREE_VIEW (gtk_builder_get_object (builder, "treeview_buscador"));

  if (gtk_tree_view_get_model (treeview) == NULL )
    {
      store = gtk_list_store_new (7,
                                  G_TYPE_STRING,
                                  G_TYPE_STRING,
                                  G_TYPE_STRING,
                                  G_TYPE_STRING,
                                  G_TYPE_STRING,
                                  G_TYPE_STRING,
                                  G_TYPE_INT);

      gtk_tree_view_set_model (treeview, GTK_TREE_MODEL (store));

      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes ("Codigo", renderer,
                                                         "text", 0,
                                                         NULL);
      gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      gtk_tree_view_column_set_resizable (column, FALSE);

      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes ("Codigo de Barras", renderer,
                                                         "text", 1,
                                                         NULL);
      gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      gtk_tree_view_column_set_resizable (column, FALSE);

      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes ("Descripción del Producto", renderer,
                                                         "text", 2,
                                                         NULL);
      gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      gtk_tree_view_column_set_min_width (column, 400);
      //      gtk_tree_view_column_set_max_width (column, 300);
      gtk_tree_view_column_set_resizable (column, FALSE);

      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes ("Marca", renderer,
                                                         "text", 3,
                                                         NULL);
      gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      gtk_tree_view_column_set_min_width (column, 100);
      gtk_tree_view_column_set_max_width (column, 100);
      gtk_tree_view_column_set_resizable (column, FALSE);

      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes ("Cant.", renderer,
                                                         "text", 4,
                                                         NULL);
      gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
      gtk_tree_view_column_set_resizable (column, FALSE);

      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes ("Unid.", renderer,
                                                         "text", 5,
                                                         NULL);
      gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
      gtk_tree_view_column_set_resizable (column, FALSE);

      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes ("Stock", renderer,
                                                         "text", 6,
                                                         NULL);
      gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
      gtk_tree_view_column_set_resizable (column, FALSE);
    }

  window = GTK_WINDOW (gtk_builder_get_object (builder, "wnd_buscador"));
  gtk_widget_show_all (GTK_WIDGET (window));
  gtk_widget_grab_focus (GTK_WIDGET (gtk_builder_get_object (builder, "entry_buscar")));

  if ( ! g_str_equal (string, ""))
    {
      gtk_entry_set_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_buscar")), string);
      SearchName();
    }
} // void SearchByName (void)


void
Comprar (GtkWidget *widget, gpointer data)
{
  gchar *rut = g_strdup (gtk_label_get_text (GTK_LABEL (compra->rut_label)));
  gchar *nota = g_strdup (gtk_entry_get_text (GTK_ENTRY (compra->nota_entry)));
  gint dias_pago = atoi (g_strdup (gtk_entry_get_text (GTK_ENTRY (entry_plazo))));


  if (strcmp (rut, "") == 0)
    {
      ErrorMSG (compra->tree_prov, "Debe Seleccionar un Proveedor");
    }
  else
    {
      AgregarCompra (rut, nota, dias_pago);

      CloseBuyWindow ();

      ClearAllCompraData ();

      CleanStatusProduct ();

      compra->header_compra = NULL;
      compra->products_compra = NULL;
    }

  //Se libera la variable global
  if (rut_proveedor_global != NULL)
    {
      g_free (rut_proveedor_global);
      rut_proveedor_global = NULL;
    }

  //Se vuelve a habilitar el boton de compra sugerida
  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_suggest_buy")), TRUE);
  
  gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "button_buy")), FALSE);
}


void
ShowProductHistory (void)
{
  PGresult *res;
  GtkTreeIter iter;

  GtkListStore *store;

  gchar *barcode = g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object( builder, "entry_buy_barcode" ))));
  gint i, tuples;
  gdouble precio = 0;
  gdouble cantidad = 0;

  res = EjecutarSQL
    (g_strdup_printf
     ("SELECT (SELECT nombre FROM proveedor WHERE rut=c.rut_proveedor) AS proveedor, "
      "(SELECT precio FROM producto WHERE barcode = %s) AS precio_venta, "
      "date_part('day', c.fecha) AS dia, date_part('month', c.fecha) AS mes, date_part('year', c.fecha) AS ano, "
      "c.id, cd.iva, cd.otros_impuestos, cd.precio, cd.cantidad "
      "FROM compra c INNER JOIN compra_detalle cd "
      "ON c.id = cd.id_compra "
      "INNER JOIN producto p "
      "ON cd.barcode_product = p.barcode "
      "WHERE p.barcode='%s' "
      "AND cd.anulado='f' "
      "ORDER BY c.fecha DESC", barcode, barcode));

  tuples = PQntuples (res);

  store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (gtk_builder_get_object( builder, "product_history_tree_view"))));

  gtk_list_store_clear (store);

  for (i = 0; i < tuples; i++)
    {
      if (strcmp (PQvaluebycol (res, i, "iva"), "0") != 0)
        {
          precio = (strtod (PUT(PQvaluebycol (res, i, "precio")), (char **)NULL) + strtod (PUT(PQvaluebycol (res, i, "iva")), (char **)NULL)/
		    strtod (PUT(PQvaluebycol (res, i, "cantidad")), (char **)NULL));
        }
      if (strcmp (PQvaluebycol (res, i, "otros_impuestos"), "0") != 0)
        {
          precio += (strtod (PUT(PQvaluebycol (res, i, "otros_impuestos")), (char **)NULL) /
		     strtod (PUT(PQvaluebycol (res, i, "cantidad")), (char **)NULL));
        }

      cantidad = strtod (PUT(PQvaluebycol (res, i, "cantidad")), (char **)NULL);

      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
                          0, g_strdup_printf ("%.2d/%.2d/%s", atoi (PQvaluebycol (res, i, "dia")),
                                              atoi (PQvaluebycol (res, i, "mes")), PQvaluebycol (res, i, "ano")),
                          1, PQvaluebycol (res, i, "id"),
                          2, PQvaluebycol (res, i, "proveedor"),
                          3, cantidad,
                          4, strtod (PUT (PQvaluebycol (res, i, "precio")), (char **)NULL),
			  5, precio,
			  6, (cantidad * precio),
                          -1);

    }

  // Coloca el último precio de compra y el precio de venta del producto en los
  // entry correspondientes.
  if (tuples > 0)
    {
      gtk_entry_set_text (GTK_ENTRY (builder_get (builder, "entry_buy_price")), PUT (PQvaluebycol (res, 0, "precio")));
      gtk_entry_set_text (GTK_ENTRY (builder_get (builder, "entry_sell_price")), PQvaluebycol (res, 0, "precio_venta"));
    }
}


void
ClearAllCompraData (void)
{
  GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_products_buy_list"))));

  gtk_list_store_clear (store);

  CleanStatusProduct ();

  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_total_buy")), "\t\t\t");

}


void
InsertarCompras (void)
{
  GtkTreeView *treeview = GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_pending_requests"));
  GtkTreeSelection *selection = gtk_tree_view_get_selection (treeview);
  GtkTreeIter iter;
  GtkListStore *store_pending_request = GTK_LIST_STORE (gtk_tree_view_get_model (treeview));
  GtkListStore *store_pending_request_detail = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_pending_request_detail"))));

  PGresult *res;
  gint tuples, i, id_compra;

  res = EjecutarSQL( "SELECT * FROM get_compras()" );

  gtk_list_store_clear (store_pending_request);
  gtk_list_store_clear (store_pending_request_detail);

  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_pending_net_total")), "");
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_pending_task_total")), "");
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_pending_other_task_total")), "");
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_pending_total")), "");

  tuples = PQntuples (res);

  for (i = 0; i < tuples; i++)
    {
      id_compra = atoi (PQvaluebycol(res, i, "id_compra"));

      gtk_list_store_append (store_pending_request, &iter);
      gtk_list_store_set (store_pending_request, &iter,
                          0, id_compra,
                          1, g_strdup_printf ("%.2d/%.2d/%s", atoi (PQvaluebycol (res, i, "dia")),
                                              atoi (PQvaluebycol (res, i, "mes")), PQvaluebycol (res, i, "ano")),
                          2, PQvaluebycol(res, i, "nombre"),
                          3, PutPoints (g_strdup_printf ("%.0f", strtod (CUT(PQvaluebycol (res, i, "precio")), (char **)NULL))),
                          4, ReturnIncompletProducts (id_compra) ? "Red" : "Black",
                          5, TRUE,
                          -1);

      if (i == 0) gtk_tree_selection_select_iter (selection, &iter);
    }
}


void
IngresoDetalle (GtkTreeSelection *selection, gpointer data)
{
  gint i, id, tuples;
  gboolean color;
  GtkTreeIter iter;

  GtkListStore *store_pending_request = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_pending_requests"))));
  GtkListStore *store_pending_request_detail = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_pending_request_detail"))));

  PGresult *res;

  gdouble sol, ing;
  gdouble cantidad;

  if (compra->header != NULL)
    CompraListClean ();

  if (gtk_tree_selection_get_selected (selection, NULL, &iter) == TRUE)
    {
      gtk_tree_model_get (GTK_TREE_MODEL (store_pending_request), &iter,
                          0, &id,
                          -1);

      gtk_list_store_clear (store_pending_request_detail);

      res = EjecutarSQL ( g_strdup_printf( "SELECT * FROM get_detalle_compra( %d ) ", id ) );

      if (res == NULL)
        return;

      tuples = PQntuples (res);

      if (tuples == 0)
        return;

      for (i = 0; i < tuples; i++)
        {
          sol = strtod (PUT (PQvaluebycol(res, i, "cantidad")), (char **)NULL);
          ing = strtod (PUT (PQvaluebycol(res, i, "cantidad_ingresada")), (char **)NULL);

          if (ing<sol && ing >0)
            color = TRUE;
          else
            color = FALSE;

          gtk_list_store_append (store_pending_request_detail, &iter);
          gtk_list_store_set (store_pending_request_detail, &iter,
                              0, PQvaluebycol(res, i, "codigo_corto"),
                              1, g_strdup_printf ("%s %s %s %s", PQvaluebycol(res, i, "descripcion"),
                                                  PQvaluebycol(res, i, "marca"), PQvaluebycol(res, i, "contenido"),
                                                  PQvaluebycol(res, i, "unidad")),
                              2, PQvaluebycol(res, i, "precio"),
                              3, strtod (PUT(PQvaluebycol(res, i, "cantidad")), (char **)NULL),
                              4, strtod (PUT(PQvaluebycol(res, i, "cantidad_ingresada")), (char **)NULL),
                              5, PutPoints (PQvaluebycol(res, i, "costo_ingresado")),
                              6, color ? "Red" : "Black",
                              7, TRUE,
                              -1);

          if (ing != 0)
            cantidad = (gdouble) sol - ing;
          else
            cantidad = (gdouble) sol;

          CompraAgregarALista (PQvaluebycol(res, i, "barcode"), cantidad, atoi (PQvaluebycol(res, i, "precio_venta")),
                               strtod (PUT((PQvaluebycol(res, i, "precio"))), (char **)NULL), atoi (PQvaluebycol(res, i, "margen")), TRUE);
        }

      CalcularTotales ();
    }
} // void IngresoDetalle (GtkTreeSelection *selection, gpointer data)


void
IngresarCompra (gboolean invoice, gint n_document, gchar *monto, GDate *date)
{
  Productos *products = compra->header;
  gint id, doc;
  gchar *rut_proveedor;
  gchar *q;
  gint total_doc = atoi (monto);
  GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_pending_requests")));
  GtkListStore *store_pending_request = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_pending_requests"))));
  GtkTreeIter iter;

  if (gtk_tree_selection_get_selected (selection, NULL, &iter) == FALSE) return;

  gtk_tree_model_get (GTK_TREE_MODEL (store_pending_request), &iter,
                      0, &id,
                      -1);


  q = g_strdup_printf ("SELECT proveedor FROM get_proveedor_compra( %d )", id);
  rut_proveedor = GetDataByOne (q);
  g_free (q);

  if (invoice == TRUE)
    {
      doc = IngresarFactura (n_document, id, rut_proveedor, total_doc, g_date_get_day (date), g_date_get_month (date), g_date_get_year (date), 0);
    }
  else
    {
      doc = IngresarGuia (n_document, id, total_doc, g_date_get_day (date), g_date_get_month (date), g_date_get_year (date));
    }

  if (products != NULL)
    {
      do
	{
	  IngresarProducto (products->product, id);

	  IngresarDetalleDocumento (products->product, id, doc, invoice);

	  products = products->next;
	} while (products != compra->header);
    }

  CompraIngresada ();

  InsertarCompras ();
}


/**
 * Es llamada por la funcion "AddProveedor()".
 *
 * Esta funcion agrega al proveedor a la lista (tree view) de la busqueda de proveedores.
 *
 */
void
FillProveedores (void)
{
  PGresult *res;
  gint tuples, i;

  GtkTreeIter iter;

  /*consulta de sql que llama a la funcion select_proveedor() que retorna
    los proveedores */

  if (rut_proveedor_global != NULL)
    res = EjecutarSQL (g_strdup_printf ("SELECT rut, nombre FROM select_proveedor() "
					"WHERE rut = %s "
					"ORDER BY nombre ASC", rut_proveedor_global));
  else
  res = EjecutarSQL ("SELECT rut, nombre FROM select_proveedor() "
                     "ORDER BY nombre ASC");

  if (res == NULL)
    return;

  tuples = PQntuples (res);

  gtk_list_store_clear (compra->store_prov);

  /* aqui se agregan los proveedores al tree view*/
  for (i = 0; i < tuples; i++)
    {
      gtk_list_store_append (compra->store_prov, &iter);
      gtk_list_store_set (compra->store_prov, &iter,
                          0, PQvaluebycol(res, i, "rut"),
                          1, PQvaluebycol(res, i, "nombre"),
                          -1);
    }
}


/**
 * Es llamada por la funcion "AddProveedor()".
 *
 * Esta funcion cierra la ventana de "AddProveedorWindow()".
 *
 * @param button the button
 * @param user_data the user data
 *
 */
void
CloseAddProveedorWindow (GtkWidget *button, gpointer data)
{
  GtkWidget *window = (GtkWidget *)data;

  gtk_widget_destroy (window);

  gtk_window_set_focus (GTK_WINDOW (compra->buy_window), compra->tree_prov);
}


/**
 * Es llamada cuando el boton aceptar de "AddProveedorWindow()" es presionado (signal click).
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
AddProveedor (GtkWidget *widget, gpointer data)
{
  gchar *rut = g_strdup (gtk_entry_get_text (GTK_ENTRY (compra->rut_add)));
  gchar *rut_ver = g_strdup (gtk_entry_get_text (GTK_ENTRY (compra->rut_ver)));
  gchar *nombre = g_strdup (gtk_entry_get_text (GTK_ENTRY (compra->nombre_add)));
  gchar *direccion = g_strdup (gtk_entry_get_text (GTK_ENTRY (compra->direccion_add)));
  gchar *ciudad = g_strdup (gtk_entry_get_text (GTK_ENTRY (compra->ciudad_add)));
  gchar *comuna = g_strdup (gtk_entry_get_text (GTK_ENTRY (compra->comuna_add)));
  gchar *telefono = g_strdup (gtk_entry_get_text (GTK_ENTRY (compra->telefono_add)));
  gchar *email = g_strdup (gtk_entry_get_text (GTK_ENTRY (compra->email_add)));
  gchar *web = g_strdup (gtk_entry_get_text (GTK_ENTRY (compra->web_add)));
  gchar *contacto = g_strdup (gtk_entry_get_text (GTK_ENTRY (compra->contacto_add)));
  gchar *giro = g_strdup (gtk_entry_get_text (GTK_ENTRY (compra->giro_add)));

  if (strcmp (rut, "") == 0)
    {
      ErrorMSG (compra->rut_add, "Debe Escribir el rut completo");
      return;
    }
  else if ((GetDataByOne
            (g_strdup_printf ("SELECT * FROM proveedor WHERE rut='%s'", rut))) != NULL)
    {
      ErrorMSG (compra->rut_add, "Ya existe un proveedor con el mismo rut");
      return;
    }
  else if (g_str_equal (rut_ver, ""))
    {
      ErrorMSG (compra->rut_ver, "Debe ingresar el dígito verificador del rut");
      return;
    }
  else if (g_str_equal (nombre, ""))
    {
      ErrorMSG (compra->nombre_add, "Debe escribir el nombre del proveedor");
      return;
    }
  else if (g_str_equal (direccion, ""))
    {
      ErrorMSG (compra->direccion_add, "Debe escribir la direccion");
      return;
    }
  else if (g_str_equal (comuna, ""))
    {
      ErrorMSG (compra->comuna_add, "Debe escribir la comuna");
      return;
    }
  else if (g_str_equal (telefono, ""))
    {
      ErrorMSG (compra->telefono_add, "Debe escribir el telefono");
      return;
    }

  else if (g_str_equal (giro, ""))
    {
      ErrorMSG (compra->giro_add, "Debe escribir el giro");
      return;
    }

  if (VerificarRut (rut, rut_ver) != TRUE)
    {
      ErrorMSG (compra->rut_ver, "El rut no es valido!");
      return;
    }

  if (atoi(telefono) == 0)
    {
      ErrorMSG (compra->telefono_add, "Debe ingresar solo números en el campo telefono");
      return;
    }

  CloseAddProveedorWindow (NULL, data);


  AddProveedorToDB (g_strdup_printf ("%s-%s", rut, rut_ver),
                    nombre,
                    direccion,
                    ciudad,
                    comuna,
                    telefono,
                    email,
                    web,
                    contacto,
                    giro);

  FillProveedores ();
} // void AddProveedor (GtkWidget *widget, gpointer data)


/**
 * Es llamada por la funcion "AddProveedor()".
 *
 * Esta funcion visualiza la ventana para agregar un proveedor y agrega dos señales
 *
 * @param widget the widget that emited the signal
 * @param data the user data
 *
 */
void
AddProveedorWindow (GtkWidget *widget, gpointer user_data)
{
  GtkWidget *window;

  GtkWidget *hbox;
  GtkWidget *vbox;

  GtkWidget *button;
  GtkWidget *label;

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER_ALWAYS);
  //  gtk_window_set_transient_for (GTK_WINDOW (window), GTK_WINDOW (compra->buy_window));
  gtk_widget_show (window);
  gtk_window_present (GTK_WINDOW (window));

  vbox = gtk_vbox_new (FALSE, 3);
  gtk_widget_show (vbox);
  gtk_container_add (GTK_CONTAINER (window), vbox);

  /*
    Cajas
  */

  /*
    Rut Proveedor
  */

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  label = gtk_label_new ("Rut: ");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);

  compra->rut_ver = gtk_entry_new_with_max_length (1);
  gtk_widget_set_size_request (compra->rut_ver, 20, -1);
  gtk_widget_show (compra->rut_ver);
  gtk_box_pack_end (GTK_BOX (hbox), compra->rut_ver, FALSE, FALSE, 3);

  label = gtk_label_new ("-");
  gtk_widget_show (label);
  gtk_box_pack_end (GTK_BOX (hbox), label, FALSE, FALSE, 3);

  compra->rut_add = gtk_entry_new_with_max_length (10);
  gtk_widget_set_size_request (compra->rut_add, 75, -1);
  gtk_widget_show (compra->rut_add);
  gtk_box_pack_end (GTK_BOX (hbox), compra->rut_add, FALSE, FALSE, 3);

  gtk_window_set_focus (GTK_WINDOW (window), compra->rut_add);

  /*
    Nombre
  */

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  label = gtk_label_new ("*Nombre: ");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);

  compra->nombre_add = gtk_entry_new_with_max_length (200);
  gtk_widget_show (compra->nombre_add);
  gtk_box_pack_end (GTK_BOX (hbox), compra->nombre_add, FALSE, FALSE, 3);

  /*
    Direccion
  */

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  label = gtk_label_new ("*Direccion: ");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);

  compra->direccion_add = gtk_entry_new_with_max_length (200);
  gtk_widget_show (compra->direccion_add);
  gtk_box_pack_end (GTK_BOX (hbox), compra->direccion_add, FALSE, FALSE, 3);

  /*
    Ciudad
  */

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  label = gtk_label_new ("*Ciudad: ");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);

  compra->ciudad_add = gtk_entry_new_with_max_length (100);
  gtk_widget_show (compra->ciudad_add);
  gtk_box_pack_end (GTK_BOX (hbox), compra->ciudad_add, FALSE, FALSE, 3);


  /*
    Comuna
  */

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  label = gtk_label_new ("*Comuna: ");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);

  compra->comuna_add = gtk_entry_new_with_max_length (20);
  gtk_widget_show (compra->comuna_add);
  gtk_box_pack_end (GTK_BOX (hbox), compra->comuna_add, FALSE, FALSE, 3);

  /*
    Telefono
  */

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  label = gtk_label_new ("*Telefono: ");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);

  compra->telefono_add = gtk_entry_new_with_max_length (20);
  gtk_widget_show (compra->telefono_add);
  gtk_box_pack_end (GTK_BOX (hbox), compra->telefono_add, FALSE, FALSE, 3);

  /*
    E-Mail
  */

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  label = gtk_label_new ("E-Mail: ");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);

  compra->email_add = gtk_entry_new_with_max_length (300);
  gtk_widget_show (compra->email_add);
  gtk_box_pack_end (GTK_BOX (hbox), compra->email_add, FALSE, FALSE, 3);

  /*
    Web
  */

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  label = gtk_label_new ("Web: ");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);

  compra->web_add = gtk_entry_new_with_max_length (300);
  gtk_widget_show (compra->web_add);
  gtk_box_pack_end (GTK_BOX (hbox), compra->web_add, FALSE, FALSE, 3);

  /*
    Contacto
  */

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  label = gtk_label_new ("Contacto: ");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);

  compra->contacto_add = gtk_entry_new_with_max_length (200);
  gtk_widget_show (compra->contacto_add);
  gtk_box_pack_end (GTK_BOX (hbox), compra->contacto_add, FALSE, FALSE, 3);

  /*
    Giro
  */

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  label = gtk_label_new ("*Giro: ");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);

  compra->giro_add = gtk_entry_new_with_max_length (200);
  gtk_widget_show (compra->giro_add);
  gtk_box_pack_end (GTK_BOX (hbox), compra->giro_add, FALSE, FALSE, 3);

  /*
    Fin Cajas
  */

  /*
    Mensaje Inferior
  */
  label = gtk_label_new ("Los datos con * son obligatorios");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 3);

  /*
    Fin
  */

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_end (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  button = gtk_button_new_from_stock (GTK_STOCK_ADD);
  gtk_widget_show (button);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);

  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (AddProveedor), (gpointer)window);

  button = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
  gtk_widget_show (button);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 3);

  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (CloseAddProveedorWindow), (gpointer)window);
} // void AddProveedorWindow (GtkWidget *widget, gpointer user_data)


void
Seleccionado (GtkTreeSelection *selection, gpointer data)
{
  GtkTreeIter iter;  gchar *value;
  PGresult *res;
  gchar *q;

  if (gtk_tree_selection_get_selected (selection, NULL, &iter) == TRUE)
    {
      gtk_tree_model_get (GTK_TREE_MODEL (compra->store_prov), &iter,
                          0, &value,
                          -1);
      q = g_strdup_printf ("SELECT rut, nombre FROM select_proveedor (%s)", value);
      res = EjecutarSQL (q);
      g_free (q);

      gtk_label_set_text (GTK_LABEL (compra->rut_label),
                          PQvaluebycol( res, 0, "rut"));
      gtk_label_set_text (GTK_LABEL (compra->nombre_label),
                          PQvaluebycol( res, 0, "nombre"));
    }
}


void
on_btn_nullify_buy_clicked (void)
{
  GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_pending_requests")));
  GtkListStore *store_pending_request = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_pending_requests"))));
  GtkTreeIter iter;
  gint id_compra;

  if (gtk_tree_selection_get_selected (selection, NULL, &iter) == TRUE)
    {
      gtk_tree_model_get (GTK_TREE_MODEL (store_pending_request), &iter,
                          0, &id_compra,
                          -1);
      AnularCompraDB (id_compra);
      InsertarCompras ();
    }
}


void
on_btn_nullify_product_clicked (void)
{
  PGresult *res;

  GtkTreeSelection *selection1 = gtk_tree_view_get_selection (GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_pending_requests")));
  GtkListStore *store_pending_request = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_pending_requests"))));

  GtkTreeSelection *selection2 = gtk_tree_view_get_selection (GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_pending_request_detail")));
  GtkListStore *store_pending_request_detail = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_pending_request_detail"))));

  GtkTreeIter iter1, iter2;
  gint id_compra;
  gchar *codigo_producto;
  gchar *q;

  if (gtk_tree_selection_get_selected (selection1, NULL, &iter1) &&
      gtk_tree_selection_get_selected (selection2, NULL, &iter2))
    {
      gtk_tree_model_get (GTK_TREE_MODEL (store_pending_request), &iter1,
                          0, &id_compra,
                          -1);
      gtk_tree_model_get (GTK_TREE_MODEL (store_pending_request_detail), &iter2,
                          0, &codigo_producto,
                          -1);

      //TODO: pasar esto a funciones de la base pero por el momento funciona
      q = g_strdup_printf ("UPDATE compra_detalle SET anulado='t' WHERE barcode_product=(SELECT barcode FROM producto WHERE codigo_corto='%s') AND id_compra=%d", codigo_producto, id_compra);
      res = EjecutarSQL (q);
      g_free (q);
      q = g_strdup_printf ("UPDATE compra SET anulada='t' WHERE id NOT IN (SELECT id_compra FROM compra_detalle WHERE id_compra=%d AND anulado='f') AND id=%d", id_compra, id_compra);
      res = EjecutarSQL (q);
      g_free (q);

      InsertarCompras ();
    }
}


void
CleanStatusProduct (void)
{
  GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (gtk_builder_get_object (builder, "product_history_tree_view"))));

  gtk_list_store_clear (store);

  gtk_widget_show (GTK_WIDGET (builder_get (builder, "entry_sell_price")));
  gtk_widget_show (GTK_WIDGET (builder_get (builder, "entry_buy_price")));
  gtk_widget_show (GTK_WIDGET (builder_get (builder, "entry_buy_gain")));
  gtk_widget_show (GTK_WIDGET (builder_get (builder, "label28")));
  gtk_widget_show (GTK_WIDGET (builder_get (builder, "label29")));
  gtk_widget_show (GTK_WIDGET (builder_get (builder, "label1111")));
  //gtk_widget_show (GTK_WIDGET (builder_get (builder, "cmbPrecioCompra")));

  gtk_widget_show (GTK_WIDGET (builder_get (builder, "button_calculate")));

  gtk_widget_show (GTK_WIDGET (builder_get (builder, "button_buy")));

  gtk_widget_show (GTK_WIDGET (builder_get (builder, "label126")));
  gtk_widget_show (GTK_WIDGET (builder_get (builder, "entry_buy_amount")));
  gtk_widget_show (GTK_BUTTON (builder_get (builder, "button_add_product_list")));

  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_product_desc")), "");
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_mark")), "");
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_unit")), "");
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_stock")), "");
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_stock_further")), "");
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_sell_price")), "");
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_fifo")), "");
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_code")), "");
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_cont")), "");

  gtk_entry_set_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_buy_price")), "");
  gtk_entry_set_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_buy_gain")), "");
  gtk_entry_set_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_sell_price")), "");
  gtk_entry_set_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_buy_amount")), "1");

  gtk_entry_set_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_buy_barcode")), "");

  //Habilitar la escritura en el campo del código de barra cuando se limpie todo
  //TODO: Ver como simplificar el set_sensitive
  gtk_widget_set_sensitive (GTK_ENTRY (gtk_builder_get_object (builder, "entry_buy_barcode")), TRUE);
  gtk_widget_set_sensitive (GTK_ENTRY (gtk_builder_get_object (builder, "entry_buy_price")), FALSE);
  gtk_widget_set_sensitive (GTK_ENTRY (gtk_builder_get_object (builder, "entry_buy_gain")), FALSE);
  gtk_widget_set_sensitive (GTK_ENTRY (gtk_builder_get_object (builder, "entry_sell_price")), FALSE);
  gtk_widget_set_sensitive (GTK_ENTRY (gtk_builder_get_object (builder, "entry_buy_amount")), FALSE);
  gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "button_new_product")), TRUE);
  gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "edit_product_button")), FALSE);
  gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "button_calculate")), FALSE);
  gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "button_add_product_list")), FALSE);

  if (rizoma_get_value_boolean ("TRASPASO"))
    gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_traspaso")), FALSE);

  gtk_widget_grab_focus (GTK_ENTRY (builder_get (builder, "entry_buy_barcode")));

  calcular = 0;
}


gboolean
CheckDocumentData (GtkWidget *wnd, gboolean invoice, gchar *rut_proveedor, gint id, gchar *n_documento, gchar *monto, GDate *date)
{
  PGresult *res;
  GDate *date_buy = g_date_new ();

  res = EjecutarSQL (g_strdup_printf ("SELECT date_part('day', fecha), "
                                      "date_part('month', fecha), "
                                      "date_part('year', fecha) "
                                      "FROM compra WHERE id=%d", id));

  g_date_set_dmy (date_buy, atoi (PQgetvalue( res, 0, 0)), atoi (PQgetvalue( res, 0, 1)), atoi (PQgetvalue( res, 0, 2)));

  if (invoice)
    {
      if (g_date_compare (date_buy, date) > 0)
        {
          ErrorMSG (wnd, "La fecha de emision del documento no puede ser menor a la fecha de compra");
        }
    }
  else
    {
      if (g_date_compare (date_buy, date) > 0 )
        {
          ErrorMSG (wnd, "La fecha de emision del documento no puede ser menor a la fecha de compra");
        }
    }

  if (invoice)
    {
      if (strcmp (n_documento, "") == 0 || atoi (n_documento) <= 0)
        {
          ErrorMSG (wnd, "Debe Obligatoriamente ingresar el numero del documento");
          return FALSE;
        }
      else if (strcmp (monto, "") == 0 || atoi (monto) <= 0)
        {
          ErrorMSG (wnd, "El Monto del documento debe ser ingresado");
          return FALSE;
        }
      else
        {
          if (DataExist (g_strdup_printf ("SELECT num_factura FROM factura_compra WHERE rut_proveedor='%s' AND num_factura=%s", rut_proveedor, n_documento)) == TRUE)
            {
              ErrorMSG (wnd, g_strdup_printf ("Ya existe la factura %s ingresada de este proveedor", n_documento));
              return FALSE;
            }
          return TRUE;
        }
    }
  else
    {
      if (strcmp (n_documento, "") == 0 || atoi (n_documento) <= 0)
        {
          ErrorMSG (wnd, "Debe Obligatoriamente ingresar el numero del documento");
          return FALSE;
        }
      else if (strcmp (monto, "") == 0 || atoi (monto) <= 0)
        {
          ErrorMSG (wnd, "El Monto del documento debe ser ingresado");
          return FALSE;
        }
      else
        {
          if (DataExist (g_strdup_printf ("SELECT numero FROM guias_compra WHERE rut_proveedor='%s' AND numero=%s", rut_proveedor, n_documento)) == TRUE)
            {
              ErrorMSG (wnd, g_strdup_printf ("Ya existe la guia %s ingresada de este proveedor", n_documento));
              return FALSE;
            }
          return TRUE;
        }
    }
}


void
FillPagarFacturas (gchar *rut_proveedor)
{
  gchar *q;
  gint tuples, i;
  gint total_amount = 0;
  GtkListStore *store_invoice = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (builder_get (builder, "tree_view_invoice_list"))));
  GtkListStore *store_invoice_detail = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (builder_get (builder, "tree_view_invoice_detail"))));
  GtkTreeIter iter;

  PGresult *res;

  if (rut_proveedor == NULL) return;

  q = g_strdup_printf ("SELECT fc.id, fc.num_factura, fc.monto, "
		       "date_part ('day', fc.fecha) as dia, date_part('month', fc.fecha) as mes, date_part('year', fc.fecha) as ano, "
		       "fc.id_compra, date_part ('day', fecha_pago) AS pay_day, date_part ('month', fecha_pago) AS pay_month, date_part ('year', fecha_pago) AS pay_year, "
		       "fp.nombre, fc.id, fc.rut_proveedor "
		       "FROM formas_pago fp "
		       "INNER JOIN factura_compra fc "
		       "ON fp.id = fc.forma_pago "
		       "INNER JOIN compra c "
		       "ON fc.id_compra = c.id "
		       "WHERE fc.rut_proveedor='%s' "
		       "AND fc.pagada='f' "
		       "AND c.anulada_pi = false "
		       "ORDER BY pay_year, pay_month, pay_day ASC", rut_proveedor);

  res = EjecutarSQL (q);
  g_free (q);

  if (res == NULL) return;

  tuples = PQntuples (res);

  gtk_list_store_clear (store_invoice);
  gtk_list_store_clear (store_invoice_detail);

  for (i = 0; i < tuples; i++)
    {
      gtk_list_store_append (store_invoice, &iter);
      gtk_list_store_set (store_invoice, &iter,
                          0, PQvaluebycol (res, i, "id"),
                          1, PQvaluebycol (res, i, "rut_proveedor"),
                          2, PQvaluebycol (res, i, "num_factura"),
                          3, PQvaluebycol (res, i, "id_compra"),
                          4, g_strdup_printf ("%.2d/%.2d/%.4d", atoi (PQvaluebycol (res, i, "dia")),
                                              atoi (PQvaluebycol (res, i, "mes")), atoi (PQvaluebycol (res, i, "ano"))),
                          5, g_strdup_printf ("%.2d/%.2d/%.4d", atoi (PQvaluebycol (res, i, "pay_day")),
                                              atoi (PQvaluebycol (res, i, "pay_month")), atoi (PQvaluebycol (res, i, "pay_year"))),
                          6, PQvaluebycol (res, i, "monto"),
                          -1);
      total_amount += atoi (PQvaluebycol (res, i, "monto"));
    }

  gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_invoice_total_amount")),
                        g_strdup_printf ("<b>$ %s</b>", PUT(g_strdup_printf("%d", total_amount))));
}


void
FillGuias (gchar *rut_proveedor)
{
  GtkTreeView *treeview = GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_pending_guide"));
  GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model (treeview));
  GtkTreeIter iter;
  PGresult *res;
  gint tuples, i;
  gchar *q;

  q = g_strdup_printf ("SELECT id, numero, (SELECT SUM ((precio * cantidad) + iva + otros_impuestos) FROM compra_detalle WHERE compra_detalle.id_compra=guias_compra.id_compra) as monto "
                       "FROM guias_compra WHERE rut_proveedor='%s'", rut_proveedor);
  res = EjecutarSQL (q);

  tuples = PQntuples (res);

  gtk_list_store_clear (store);

  for (i = 0; i < tuples; i++)
    {
      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
                          0, PQvaluebycol (res, i, "id"),
                          1, PQvaluebycol (res, i, "numero"),
                          2, PQvaluebycol (res, i, "monto"),
                          -1);
    }
}


void
FillDetGuias (GtkTreeSelection *selection, gpointer data)
{
  gint tuples, i;
  gchar *guia;
  GtkTreeIter iter;
  PGresult *res;
  gchar *rut_proveedor = g_strdup (gtk_label_get_text (GTK_LABEL (compra->fact_rut)));

  if (gtk_tree_selection_get_selected (selection, NULL, &iter) == TRUE)
    {

      gtk_tree_model_get (GTK_TREE_MODEL (compra->store_guias), &iter,
                          0, &guia,
                          -1);

      res =
	EjecutarSQL
	(g_strdup_printf ("SELECT t1.codigo, t1.descripcion,  t2.cantidad, t2.precio, t2.cantidad * t2.precio AS "
			  "total, t2.barcode, t2.id_compra, t1.marca, t1.contenido, t1.unidad FROM producto AS "
			  "t1, documentos_detalle AS t2 WHERE t2.id_compra=(SELECT id_compra FROM guias_compra "
			  "WHERE numero=%s AND rut_proveedor='%s') AND t1.barcode=t2.barcode AND t2.numero=%s",
			  guia, rut_proveedor, guia));

      tuples = PQntuples (res);

      gtk_tree_store_clear (compra->store_det_guias);

      for (i = 0; i < tuples; i++)
        {
          gtk_tree_store_append (compra->store_det_guias, &iter, NULL);
          gtk_tree_store_set (compra->store_det_guias, &iter,
                              0, PQgetvalue (res, i, 0),
                              1, g_strdup_printf ("%s %s %s %s", PQgetvalue (res, i, 1),
                                                  PQgetvalue (res, i, 7), PQgetvalue (res, i, 8),
                                                  PQgetvalue (res, i, 9)),
                              2, PQgetvalue (res, i, 2),
                              3, PQgetvalue (res, i, 3),
                              4, PutPoints (g_strdup_printf ("%d", atoi (PQgetvalue (res, i, 4)))),
                              5, CheckProductIntegrity (PQgetvalue (res, i, 6), PQgetvalue (res, i, 5)) ? "Black" : "Red",
                              6, TRUE,
                              -1);
        }
    } // if (gtk_tree_selection_get_selected (selection, NULL, &iter) == TRUE)
}


void
FoundProveedor (GtkWidget *widget, gpointer data)
{
  gchar *string = g_strdup (gtk_entry_get_text (GTK_ENTRY (data)));
  PGresult *res;
  gint tuples, i;
  GtkTreeIter iter;

  res = EjecutarSQL (g_strdup_printf
                     ("SELECT * FROM proveedor WHERE lower(nombre) LIKE lower('%%%s%%') "
                      "OR rut LIKE '%%%s%%'", string, string));

  tuples = PQntuples (res);

  gtk_list_store_clear (compra->store_prov);

  for (i = 0; i < tuples; i++)
    {
      gtk_list_store_append (compra->store_prov, &iter);
      gtk_list_store_set (compra->store_prov, &iter,
                          0, PQvaluebycol (res, i, "rut"),
                          1, PQvaluebycol (res, i, "nombre"),
                          -1);
    }
}


void
FillProveedorData (gchar *rut, gboolean guias)
{
  PGresult *res;

  res = EjecutarSQL (g_strdup_printf ("SELECT * FROM select_proveedor('%s')", rut));

  if (guias == TRUE)
    {
      ClearFactData ();

      gtk_entry_set_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_guide_invoice_provider")), PQvaluebycol (res, 0, "nombre"));

      gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_guide_invoice_rut")),
                            g_strdup_printf ("<span weight=\"ultrabold\">%s</span>", rut));

      gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_guide_invoice_contact")),
                            g_strdup_printf ("<span weight=\"ultrabold\">%s</span>",
                                             PQvaluebycol (res, 0, "contacto")));

      gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_guide_invoice_address")),
                            g_strdup_printf ("<span weight=\"ultrabold\">%s</span>",
                                             PQvaluebycol (res, 0, "direccion")));

      gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_guide_invoice_comuna")),
                            g_strdup_printf ("<span weight=\"ultrabold\">%s</span>",
                                             PQvaluebycol (res, 0, "comuna")));

      gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_guide_invoice_fono")),
                            g_strdup_printf ("<span weight=\"ultrabold\">%s</span>",
                                             PQvaluebycol (res, 0, "telefono")));

      gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_guide_invoice_mail")),
                            g_strdup_printf ("<span weight=\"ultrabold\">%s</span>",
                                             PQvaluebycol (res, 0, "email")));

      gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_guide_invoice_web")),
                            g_strdup_printf ("<span weight=\"ultrabold\">%s</span>",
                                             PQvaluebycol (res, 0, "web")));

      FillGuias (rut);

    }
  else if (guias == FALSE)
    {
      gtk_entry_set_text (GTK_ENTRY (builder_get (builder, "entry_invoice_provider")), PQvaluebycol (res, 0, "nombre"));

      gtk_label_set_markup (GTK_LABEL (builder_get (builder, "label_invoice_rut")),
                            g_strdup_printf ("<span weight=\"ultrabold\">%s</span>", rut));

      gtk_label_set_markup (GTK_LABEL (builder_get (builder, "label_invoice_contact")),
                            g_strdup_printf ("<span weight=\"ultrabold\">%s</span>",
                                             PQvaluebycol (res, 0, "contacto")));

      gtk_label_set_markup (GTK_LABEL (builder_get (builder, "label_invoice_address")),
                            g_strdup_printf ("<span weight=\"ultrabold\">%s</span>",
                                             PQvaluebycol (res, 0, "direccion")));

      gtk_label_set_markup (GTK_LABEL (builder_get (builder, "label_invoice_comuna")),
                            g_strdup_printf ("<span weight=\"ultrabold\">%s</span>",
                                             PQvaluebycol (res, 0, "comuna")));

      gtk_label_set_markup (GTK_LABEL (builder_get (builder, "label_invoice_fono")),
                            g_strdup_printf ("<span weight=\"ultrabold\">%s</span>",
                                             PQvaluebycol (res, 0, "telefono")));

      gtk_label_set_markup (GTK_LABEL (builder_get (builder, "label_invoice_mail")),
                            g_strdup_printf ("<span weight=\"ultrabold\">%s</span>",
                                             PQvaluebycol (res, 0, "email")));

      gtk_label_set_markup (GTK_LABEL (builder_get (builder, "label_invoice_web")),
                            g_strdup_printf ("<span weight=\"ultrabold\">%s</span>",
                                             PQvaluebycol (res, 0, "web")));

      FillPagarFacturas (rut);
    }

  if (guias == TRUE)
    gtk_widget_grab_focus (GTK_WIDGET (gtk_builder_get_object (builder, "entry_guide_invoice_n_invoice")));
  else
    gtk_widget_grab_focus (GTK_WIDGET (builder_get (builder, "entry_invoice_provider")));
}


void
CalcularTotales (void)
{
  Productos *products = compra->header;

  gdouble total_neto = 0, total_iva = 0, total_otros = 0, total = 0;
  gdouble iva, otros;

  do
    {
      iva = GetIVA (products->product->barcode);
      otros = GetOtros (products->product->barcode);

      if (products->product->canjear != TRUE)
	total = (gdouble)products->product->precio_compra * products->product->cantidad;
      else
	total = (gdouble)products->product->precio_compra * (products->product->cantidad - products->product->cuanto);

      total_neto += total;

      if (iva != -1)
	total_iva += lround (total * (gdouble) iva / 100);

      if (otros != -1)
	total_otros += lround (total * (gdouble) otros / 100);

      products = products->next;
    } while (products != compra->header);

  total = total_neto + total_otros + total_iva;

  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_pending_net_total")),
                      PutPoints (g_strdup_printf ("%li", lround (total_neto))));
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_pending_task_total")),
                      PutPoints (g_strdup_printf ("%li", lround (total_iva))));
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_pending_other_task_total")),
                      PutPoints (g_strdup_printf ("%lu", lround (total_otros))));
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_pending_total")),
                      PutPoints (g_strdup_printf ("%li", lround (total))));
}


void
CalcularTotalesGuias (void)
{
  gint total_neto = 0, total_iva = 0, total_otros = 0, total = 0;
  gchar *guia;
  PGresult *res;
  gchar *rut_proveedor = g_strdup (gtk_label_get_text (GTK_LABEL (gtk_builder_get_object (builder, "label_guide_invoice_rut"))));

  GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_guide_invoice")));
  GtkTreeIter iter;

  if (gtk_tree_model_get_iter_first (model, &iter) == TRUE)
    {
      gtk_tree_model_get (model, &iter,
                          1, &guia,
                          -1);
      if (guia != NULL)
        {
          while (1)
            {
              res = EjecutarSQL (g_strdup_printf ("SELECT SUM (t1.precio * t3.cantidad) AS neto, SUM (t3.iva) AS iva, SUM (t3.otros) AS otros, SUM ((t1.precio * t3.cantidad) + t3.iva + t3.otros) AS total  FROM compra_detalle AS t1, guias_compra AS t2, guias_compra_detalle AS t3 WHERE t1.id_compra=(SELECT id_compra FROM guias_compra WHERE numero=%s AND rut_proveedor='%s') AND t2.numero=%s AND t1.barcode_product=t3.barcode and t3.id_guias_compra=t2.id",
                                                  guia, rut_proveedor, guia));

              total_neto += atoi (PQvaluebycol (res, 0, "neto"));

              total_iva += atoi (PQvaluebycol (res, 0, "iva"));

              total_otros += atoi (PQvaluebycol (res, 0, "otros"));

              total += atoi (PQvaluebycol (res, 0, "total"));

              if (gtk_tree_model_iter_next (model, &iter) != TRUE)
                break;
              else
                gtk_tree_model_get (model, &iter,
                                    0, &guia,
                                    -1);

            }

          gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_guide_invoice_neto")),
                              PutPoints (g_strdup_printf ("%d", total_neto)));
          gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_guide_invoice_task")),
                              PutPoints (g_strdup_printf ("%d", total_iva)));
          gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_guide_invoice_other_tasks")),
                              PutPoints (g_strdup_printf ("%d", total_otros)));

          gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_guide_invoice_total")),
                              PutPoints (g_strdup_printf ("%d", total)));

          CheckMontoGuias ();
        }
    }
  else
    {
      gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_guide_invoice_neto")), "");
      gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_guide_invoice_task")), "");
      gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_guide_invoice_other_tasks")), "");
      gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_guide_invoice_total")), "");
    }
}


void
CheckMontoGuias (void)
{
  gint total_guias = atoi (CutPoints (g_strdup (gtk_label_get_text (GTK_LABEL (gtk_builder_get_object (builder, "label_guide_invoice_total"))))));
  gint total_fact = atoi (CutPoints (g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_guide_invoice_amount"))))));

  if (total_guias == 0)
    return;

  if (total_guias < total_fact)
    {
      gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "btn_guide_invoice_ok")), FALSE);
    }
  else if (total_guias > total_fact)
    {
      gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "btn_guide_invoice_ok")), FALSE);
    }
  else if (total_guias == total_fact)
    {
      gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "btn_guide_invoice_ok")), TRUE);
      gtk_widget_grab_focus (GTK_WIDGET (gtk_builder_get_object (builder, "btn_guide_invoice_ok")));
    }
}


void
CheckMontoIngreso (GtkWidget *btn_ok, gint total, gint total_doc)
{
  if (total < total_doc)
    {
      gtk_widget_set_sensitive (btn_ok, FALSE);
    }
  else if (total > total_doc)
    {
      gtk_widget_set_sensitive (btn_ok, FALSE);
    }
  else if (total == total_doc)
    {
      gtk_widget_set_sensitive (btn_ok, TRUE);
      gtk_widget_grab_focus (btn_ok);
    }
}


void
AskIngreso (void)
{
  GtkWindow *wnd_ingress = GTK_WINDOW (gtk_builder_get_object (builder, "wnd_ingress_buy"));

  gtk_widget_grab_focus (GTK_WIDGET (builder_get (builder, "button_ok_ingress")));
  gtk_widget_show_all (GTK_WIDGET (wnd_ingress));
}


void
SetElabVencDate (GtkCalendar *calendar, gpointer data)
{
  GtkButton *button = (GtkButton *) data;
  guint day, month, year;

  gtk_calendar_get_date (calendar, &year, &month, &day);

  gtk_button_set_label (button, g_strdup_printf ("%.2u/%.2u/%.4u", day, month+1, year));

  SetToggleMode (GTK_TOGGLE_BUTTON (button), NULL);
}


void
AskElabVenc (GtkWidget *wnd, gboolean invoice)
{
  GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_pending_requests")));
  GtkListStore *store_pending_request = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_pending_requests"))));
  GtkTreeIter iter;
  gint id;
  gchar *rut_proveedor;

  GList *list = gtk_container_get_children (GTK_CONTAINER (wnd));
  gint nth;
  gchar *widget_name = NULL;

  GtkEntry *entry_n = NULL;
  GtkEntry *entry_amount = NULL;
  GtkEntry *entry_date = NULL;

  gchar *n_documento = NULL;
  gchar *monto = NULL;

  GDate *date = g_date_new ();

  while (list != NULL)
    {
      if (GTK_IS_ENTRY (list->data))
        {
          widget_name = g_strdup (gtk_buildable_get_name (GTK_WIDGET (list->data)));

          if (validate_string ("n$", widget_name))
            {
              entry_n = GTK_ENTRY (list->data);
            }
          else if (validate_string ("amount$", widget_name))
            {
              entry_amount = GTK_ENTRY (list->data);
            }
          else if (validate_string ("date$", widget_name))
            {
              entry_date = GTK_ENTRY (list->data);
            }
          g_free (widget_name);
        }
      else if (GTK_IS_CONTAINER (list->data) && !GTK_IS_TREE_VIEW (list->data))
        {
          nth = g_list_index (list, list->data);
          list = g_list_concat (list, gtk_container_get_children (GTK_CONTAINER(list->data)));
          list = g_list_nth (list, nth);
        }

      list = g_list_next (list);
    }
  g_list_free (list);

  n_documento = g_strdup (gtk_entry_get_text (entry_n));
  monto = g_strdup (gtk_entry_get_text (entry_amount));

  if (entry_amount == NULL || entry_date == NULL || entry_n == NULL)
    {
      gtk_widget_hide (wnd);
      ErrorMSG (GTK_WIDGET (entry_date), "Hubo un fallo obteniendo los datos, contacte a su proveedor");
      return;
    }

  g_date_set_parse (date, gtk_entry_get_text (entry_date));
  if (!g_date_valid (date))
    {
      ErrorMSG (GTK_WIDGET (entry_date), "Debe ingresar una fecha de emision para el documento");
      return;
    }

  if (gtk_tree_selection_get_selected (selection, NULL, &iter) == FALSE)
    return;

  gtk_tree_model_get (GTK_TREE_MODEL (store_pending_request), &iter,
                      0, &id,
                      -1);

  rut_proveedor = GetDataByOne (g_strdup_printf ("SELECT rut_proveedor FROM compra WHERE id=%d", id));

  if (validate_string ("[a-zA-Z ]", monto) || validate_string ("[a-zA-Z ]", n_documento))
    {
      ErrorMSG (GTK_WIDGET (entry_amount), "El formulario contiene caracteres invalidos.");
      return;
    }

  if (CheckDocumentData (wnd, invoice, rut_proveedor, id, n_documento, monto, date) == FALSE) return;

  IngresarCompra (invoice, atoi (n_documento), monto, date);

  gtk_widget_hide (wnd);
}


int
main (int argc, char **argv)
{
  GtkWindow *login_window;
  GError *err = NULL;

  GtkComboBox *combo;
  GtkListStore *model;
  GtkTreeIter iter;
  GtkCellRenderer *cell;

  GKeyFile *key_file;
  gchar **profiles;

  key_file = rizoma_open_config();

  if (key_file == NULL)
    {
      g_error ("Cannot open config file\n");
      return -1;
    }

  gtk_init (&argc, &argv);

  builder = gtk_builder_new ();

  gtk_builder_add_from_file (builder, DATADIR"/ui/rizoma-login.ui", &err);

  if (err)
    {
      g_error ("ERROR: %s\n", err->message);
      return -1;
    }

  gtk_builder_connect_signals (builder, NULL);

  profiles = g_key_file_get_groups (key_file, NULL);
  g_key_file_free (key_file);

  model = gtk_list_store_new (1,
                              G_TYPE_STRING);

  combo = (GtkComboBox *) gtk_builder_get_object (builder, "combo_profile");

  cell = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start ((GtkCellLayout *)combo, cell, TRUE);
  gtk_cell_layout_set_attributes ((GtkCellLayout *)combo, cell,
                                  "text", 0,
                                  NULL);
  do
    {
      if (*profiles != NULL)
        {
          gtk_list_store_append (model, &iter);
          gtk_list_store_set (model, &iter,
                              0, *profiles,
                              -1
                              );
        }
    } while (*profiles++ != NULL);

  gtk_combo_box_set_model (combo, (GtkTreeModel *)model);
  gtk_combo_box_set_active (combo, 0);

  gtk_widget_show_all (GTK_WIDGET (builder_get (builder, "login_window")));

  gtk_main ();

  return 0;
}


void
check_passwd (GtkWidget *widget, gpointer data)
{
  GtkTreeIter iter;
  GtkComboBox *combo = (GtkComboBox *) gtk_builder_get_object (builder, "combo_profile");
  GtkTreeModel *model = gtk_combo_box_get_model (combo);
  gchar *group_name;

  gchar *passwd = g_strdup (gtk_entry_get_text ( (GtkEntry *) gtk_builder_get_object (builder,"passwd_entry")));
  gchar *user = g_strdup (gtk_entry_get_text ( (GtkEntry *) gtk_builder_get_object (builder,"user_entry")));

  gtk_combo_box_get_active_iter (combo, &iter);
  gtk_tree_model_get (model, &iter,
                      0, &group_name,
                      -1);

  rizoma_set_profile (group_name);

  switch (AcceptPassword (passwd, user))
    {
    case TRUE:

      user_data = (User *) g_malloc (sizeof (User));

      user_data->user_id = ReturnUserId (user);
      user_data->level = ReturnUserLevel (user);
      user_data->user = user;

      Asistencia (user_data->user_id, TRUE);

      gtk_widget_destroy (GTK_WIDGET(gtk_builder_get_object (builder,"login_window")));
      g_object_unref ((gpointer) builder);
      builder = NULL;

      compras_win ();

      break;
    case FALSE:
      gtk_entry_set_text ((GtkEntry *) gtk_builder_get_object (builder,"user_entry"), "");
      gtk_entry_set_text ((GtkEntry *) gtk_builder_get_object (builder,"passwd_entry"), "");
      rizoma_error_window ((GtkWidget *) gtk_builder_get_object (builder,"user_entry"));
      break;
    default:
      break;
    }
}


void
on_entry_buy_barcode_activate (GtkEntry *entry, gpointer user_data)
{
  gchar *barcode = g_strdup (gtk_entry_get_text (entry));

  if (HaveCharacters (barcode) == TRUE || g_str_equal (barcode, ""))
    {
      SearchByName ();
    }
  else
    {
      SearchProductHistory (entry, barcode);
    }
}


void
on_buy_notebook_switch_page (GtkNotebook *notebook, GtkNotebookPage *page, guint page_num, gpointer user_data)
{
  GtkWidget *widget;

  switch (page_num)
    {
    case 0:
      gtk_widget_grab_focus (GTK_WIDGET (builder_get (builder, "entry_buy_barcode")));
      break;
    case 1:
      InsertarCompras ();
      break;
    case 2:
      clean_container (GTK_CONTAINER (gtk_builder_get_object (builder, "vbox_guide_invoice")));
      break;
    case 4: //mercaderia tab
      widget = GTK_WIDGET(gtk_builder_get_object(builder, "find_product_entry"));
      gtk_editable_select_region(GTK_EDITABLE(widget), 0, -1);
      gtk_widget_grab_focus(widget);
    default:
      break;
    }
}


/**
 * Es llamada cuando el boton "button_calculate" es presionado (signal click).
 *
 * Esta funcion verifica si se han ingresado valores a los entry
 * "entry_buy_price", "entry_buy_gain", "entry_sell_price", si es así llama a
 * la funcion CalcularPrecioFinal.
 *
 * @param button the button
 * @param user_data the user data
 */
void
on_button_calculate_clicked (GtkButton *button, gpointer data)
{
  gdouble ingresa = strtod (PUT (g_strdup (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_buy_price"))))), (char **)NULL);
  gdouble ganancia = (gdouble) atoi (g_strdup (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_buy_gain")))));
  gdouble precio_final = (gdouble) atoi (g_strdup (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_sell_price")))));

  if (ingresa == 0 && ganancia == 0 && precio_final == 0)
    {
      ErrorMSG (GTK_WIDGET (gtk_builder_get_object (builder, "entry_buy_price")),
                "Ingrese los valores a calcular");
      return;
    }
  else
    {
      CalcularPrecioFinal();
    }
}


/**
 * Es llamada cuando el boton "button_add_product_list" es presionado (signal click).
 *
 * Esta funcion llama a la funcion  AddToProductsList.
 *
 * @param button the button
 * @param user_data the user data
 */
void
on_button_add_product_list_clicked (GtkButton *button, gpointer data)
{
  gint unidades = NULL;
  unidades = atoi (g_strdup (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_buy_amount")))));

  if (unidades == NULL || unidades < 0)
    {
      ErrorMSG (GTK_WIDGET (gtk_builder_get_object (builder, "entry_buy_amount")),
                "No se ha especificado una cantidad válida de unidades");
      return;
    }
  else
    {
      calcular = 0;
      AddToProductsList ();
    }
}

/**
 * Es llamada por la función "on_button_new_product_clicked"
 * cuando ROPA=0 en el .rizoma.
 *
 * Visualiza la ventana "wnd_new_product" y carga los
 * respectivos datos de ésta: los entry's, botones, y combo box'
 *
 * @param void
 */
void
create_new_product (void)
{
  PGresult *res, *res2;
  gchar *barcode = g_strdup (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_buy_barcode"))));
  GtkComboBox *combo = GTK_COMBO_BOX (builder_get (builder, "cmbbox_new_product_imp_others"));
  GtkComboBox *cmb_unit;
  GtkListStore *combo_store = GTK_LIST_STORE (gtk_combo_box_get_model (combo));
  GtkListStore *modelo;
  GtkCellRenderer *cell, *cell2;

  cmb_unit = GTK_WIDGET (gtk_builder_get_object(builder, "cmb_box_new_product_unit"));
  modelo = GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(cmb_unit)));

  clean_container (GTK_CONTAINER (builder_get (builder, "wnd_new_product")));

  gtk_entry_set_text (GTK_ENTRY (builder_get (builder, "entry_new_product_barcode")), barcode);

  if (strlen (barcode) > 0 && strlen (barcode) <= 6)
    {
      gtk_entry_set_text (GTK_ENTRY (builder_get (builder, "entry_new_product_code")), barcode);
      gtk_widget_grab_focus (GTK_WIDGET (builder_get (builder, "entry_new_product_desc")));
    }
  else
    {
      gtk_widget_grab_focus (GTK_WIDGET (builder_get (builder, "entry_new_product_code")));
    }

  if (combo_store == NULL)
    {
      res = EjecutarSQL ("SELECT * FROM select_otros_impuestos()");
      if (res != NULL)
        {

          GtkTreeIter iter;
          gint i, tuples;

          tuples = PQntuples (res);

          combo_store = gtk_list_store_new (3,
                                            G_TYPE_INT,    //0 id
                                            G_TYPE_STRING, //1 descripcion
                                            G_TYPE_DOUBLE);//2 monto

          gtk_combo_box_set_model (combo, GTK_TREE_MODEL(combo_store));

          cell = gtk_cell_renderer_text_new ();
          gtk_cell_layout_pack_start (GTK_CELL_LAYOUT(combo), cell, TRUE);
          gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT(combo), cell,
                                          "text", 1,
                                          NULL);
          for (i = 0; i < tuples; i++)
            {
              gtk_list_store_append (combo_store, &iter);
              gtk_list_store_set (combo_store, &iter,
                                  0, atoi (PQvaluebycol(res, i, "id")),
                                  1, PQvaluebycol (res, i, "descripcion"),
                                  2, strtod (PUT(PQvaluebycol(res, i, "monto")), (char **)NULL),
                                  -1);
            }
        }
    }

  if(modelo == NULL)
    {
      GtkTreeIter iter;
      gint i, tuples;

      modelo = gtk_list_store_new (2,
				   G_TYPE_INT,
				   G_TYPE_STRING);

      gtk_combo_box_set_model(GTK_COMBO_BOX(cmb_unit), GTK_TREE_MODEL(modelo));

      cell2 = gtk_cell_renderer_text_new();
      gtk_cell_layout_pack_start (GTK_CELL_LAYOUT(cmb_unit), cell2, TRUE);
      gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(cmb_unit), cell2,
                                     "text", 1,
                                     NULL);

      res2 = EjecutarSQL ("SELECT * FROM unidad_producto");
      tuples = PQntuples (res2);

      if(res2 != NULL)
	{
	  for (i=0 ; i < tuples ; i++)
	    {
	      gtk_list_store_append(modelo, &iter);
	      gtk_list_store_set(modelo, &iter,
	      			 0, atoi(PQvaluebycol(res2, i, "id")),
	      			 1, PQvaluebycol(res2, i, "descripcion"),
	      			 -1);
	    }
	}
    }

  //Seleccion radiobutton
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (builder_get (builder,"radio_btn_fractional_no")), TRUE);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (builder_get (builder,"radio_btn_task_yes")), TRUE);

  //Ventana creacion de productos
  gtk_entry_set_max_length (GTK_ENTRY (builder_get (builder, "entry_new_product_barcode")), 18);
  gtk_entry_set_max_length (GTK_ENTRY (builder_get (builder, "entry_new_product_code")), 16);
  gtk_entry_set_max_length (GTK_ENTRY (builder_get (builder, "entry_new_product_brand")), 20);
  gtk_entry_set_max_length (GTK_ENTRY (builder_get (builder, "entry_new_product_desc")), 25);
  gtk_entry_set_max_length (GTK_ENTRY (builder_get (builder, "entry_new_product_cont")), 10);

  //Seleccion Combobox
  gtk_combo_box_set_active (combo, 0);
  gtk_combo_box_set_active (cmb_unit, 0);
  gtk_widget_show_all (GTK_WIDGET (builder_get (builder, "wnd_new_product")));
}


/**
 * This callback is triggered when a cell on 
 * 'treeview_sizes' is edited. (editable-event)
 * @param cell
 * @param path_string
 * @param new_size
 * @param data -> A GtkListStore
 */
void
on_sizes_cell_renderer_edited (GtkCellRendererText *cell, gchar *path_string, gchar *new_size, gpointer data)
{
  GtkTreeModel *model = GTK_TREE_MODEL (data);
  GtkTreePath *path = gtk_tree_path_new_from_string (path_string);
  GtkTreeIter iter;
  
  /* obtiene el iter para poder obtener y setear 
     datos del treeview */
  gtk_tree_model_get_iter (model, &iter, path);

  //Se asegura que la talla agregada contenga 2 dígitos
  if (strlen (new_size) != 2)
    {
      ErrorMSG (GTK_WIDGET (builder_get (builder, "treeview_colors")),
		"La talla debe contener 2 dígitos");
      return;
    }
  
  // Se setean los datos modificados en el treeview
  gtk_list_store_set (GTK_LIST_STORE (model), &iter,
		      0, FALSE,    // False indica que es un dato agregado a mano
		      1, new_size, // Se le inserta la nueva talla
		      -1);

}


/**
 * This callback is triggeres by 'btn_create_codes' button
 * 
 * Get all the pieces of code and creates the corresponding
 * codes from them.
 *
 * @param button the button
 * @param user_data the user data
 */
void
on_btn_create_codes_clicked (GtkButton *button, gpointer data)
{
  GtkTreeView *treeview;
  GtkTreeModel *model;
  GtkListStore *store;
  GtkTreeIter iter;
  gboolean valid;

  gint i, j, partida_color, partida_size, num_codes;
  gboolean existente, base_existente;
  gint num_sizes = get_treeview_length (GTK_TREE_VIEW (builder_get (builder, "treeview_sizes")));
  gint num_colors = get_treeview_length (GTK_TREE_VIEW (builder_get (builder, "treeview_colors")));
  gchar *sizes[num_sizes];
  gchar *colors[num_colors];
  gint new_colors, new_sizes;
  new_colors = new_sizes = num_codes = 0;
  partida_color = partida_size = 0;

  /*Gets all new sizes*/
  treeview = GTK_TREE_VIEW (builder_get (builder, "treeview_sizes"));
  model = gtk_tree_view_get_model (treeview);
  store = GTK_LIST_STORE (model);
  valid = gtk_tree_model_get_iter_first (model, &iter);
  i = 0;
  while (valid)
    {
      gtk_tree_model_get (model, &iter,
			  0, &existente,
			  1, &sizes[i],
			  -1);

      if (existente == FALSE)
	new_sizes++;
      
      i++;
      valid = gtk_tree_model_iter_next (store, &iter); /* Me da TRUE si itera a la siguiente */
    }  
    
  /*Gets all new colors*/
  treeview = GTK_TREE_VIEW (builder_get (builder, "treeview_colors"));
  model = gtk_tree_view_get_model (treeview);
  store = GTK_LIST_STORE (model);
  valid = gtk_tree_model_get_iter_first (model, &iter);
  i = 0;
  while (valid)
    {
      gtk_tree_model_get (model, &iter,
			  0, &existente,
			  1, &colors[i],
			  -1);

      if (existente == FALSE)
	new_colors++;

      i++;
      valid = gtk_tree_model_iter_next (store, &iter); /* Me da TRUE si itera a la siguiente */
    }

  /*-Creating codes -*/

  /*get fragments*/
  gchar *code_base = g_strdup_printf("");
  GObject *clothes_base_code[5];
  clothes_base_code[0] = builder_get (builder, "entry_clothes_depto");
  clothes_base_code[1] = builder_get (builder, "entry_clothes_temp");
  clothes_base_code[2] = builder_get (builder, "entry_clothes_year");
  clothes_base_code[3] = builder_get (builder, "entry_clothes_sub_depto");
  clothes_base_code[4] = builder_get (builder, "entry_clothes_id");

  for (i=0; i<5; i++)
    code_base = g_strdup_printf ("%s%s", code_base, gtk_entry_get_text (GTK_ENTRY (clothes_base_code[i])));
  
  /*codigos + tamaños*/
  gchar *code_sizes[num_sizes];
  for (i=0; i<num_sizes; i++)
    {
      code_sizes[i] = g_strdup_printf ("%s%s", code_base, sizes[i]);
    }
 
  // (si hay nuevas tallas o colores y hay a lo menos 1 de ambos) o
  // (si el codigo base es nuevo y existe a lo menos 1 color y talla)
  base_existente = DataExist (g_strdup_printf ("SELECT * FROM clothes_code WHERE codigo_corto like '%s%%'",code_base));

  if ((new_sizes > 0 || new_colors > 0 && (num_sizes > 0 && num_colors > 0)) ||
      (!base_existente && (num_sizes > 0 && num_colors > 0)))
    {
      //Solo se deshabilita si no tiene texto en el
      if (g_str_equal (gtk_entry_get_text 
		       (GTK_ENTRY (builder_get (builder, "entry_new_clothes_brand"))), ""))
	gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "entry_new_clothes_brand")), TRUE);

      //Solo se deshabilita si no tiene texto en el
      if (g_str_equal (gtk_entry_get_text 
		       (GTK_ENTRY (builder_get (builder, "entry_new_clothes_desc"))), ""))
	gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "entry_new_clothes_desc")), TRUE);

      gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_add_new_clothes")), TRUE);
    }
  else
    {
      if (!base_existente)
	ErrorMSG (GTK_WIDGET (builder_get (builder, "btn_add_size")), 
		  "Debe ingresar a lo menos una talla y un color");
      else
	ErrorMSG (GTK_WIDGET (builder_get (builder, "btn_add_size")), 
		  "Debe ingresar a lo menos una talla o un color nuevo");
      return;
    }

  /*Se rellena el treeview de codigos cortos*/
  treeview = GTK_TREE_VIEW (builder_get (builder, "treeview_shortcodes"));
  model = gtk_tree_view_get_model (treeview);
  store = GTK_LIST_STORE (model);

  /*Quitamos solamente los codigos nuevos*/
  num_codes = get_treeview_length (treeview);
  i=0;
  valid = gtk_tree_model_get_iter_first (model, &iter);
  while (valid && i < num_codes)
    {
      gtk_tree_model_get (model, &iter,
			  0, &existente,
			  -1);

      if (existente == FALSE)
	gtk_list_store_remove (store, &iter);
      else
	valid = gtk_tree_model_iter_next (store, &iter); /* Me da TRUE si itera a la siguiente */
      i++;
    }

  if (base_existente == FALSE)
    {
      store = GTK_LIST_STORE (model);
      for (i=0; i<num_sizes; i++)
	{
	  for (j=0; j<num_colors; j++)
	    {
	      gtk_list_store_append (store, &iter);
	      gtk_list_store_set (store, &iter,
				  0, FALSE,
				  1, g_strdup_printf ("%s%s", code_sizes[i], colors[j]),
				  -1);	      
	    }
	}
    }
  else /*Agregamos nuevos codigos*/
    {
      if (new_sizes > 0 && new_colors == 0)
	{
	  partida_size = num_sizes - new_sizes;
	  store = GTK_LIST_STORE (model);
	  for (i=partida_size; i<num_sizes; i++)
	    {
	      for (j=0; j<num_colors; j++)
		{
		  gtk_list_store_append (store, &iter);
		  gtk_list_store_set (store, &iter,
				      0, FALSE,
				      1, g_strdup_printf ("%s%s", code_sizes[i], colors[j]),
				      -1);	      
		}
	    }
	}
      else if (new_colors > 0 && new_sizes == 0)
	{
	  partida_color = num_colors - new_colors;
	  store = GTK_LIST_STORE (model);
	  for (i=0; i<num_sizes; i++)
	    {
	      for (j=partida_color; j<num_colors; j++)
		{
		  gtk_list_store_append (store, &iter);
		  gtk_list_store_set (store, &iter,
				      0, FALSE,
				      1, g_strdup_printf ("%s%s", code_sizes[i], colors[j]),
				      -1);	      
		}
	    }
	}
      else if (new_colors > 0 && new_sizes > 0)
	{
	  partida_size = num_sizes - new_sizes;;
	  partida_color = num_colors - new_colors;
	  store = GTK_LIST_STORE (model);
	  for (i=partida_size; i<num_sizes; i++)
	    {
	      for (j=partida_color; j<num_colors; j++)
		{
		  gtk_list_store_append (store, &iter);
		  gtk_list_store_set (store, &iter,
				      0, FALSE,
				      1, g_strdup_printf ("%s%s", code_sizes[i], colors[j]),
				      -1);	      
		}
	    }
	}


    }
}


/**
 * This callback is triggeres by 'btn_add_clothes' button
 * 
 * Create clothes from information on 'wnd_new_clothing'
 *
 * @param button the button
 * @param user_data the user data
 */
void
on_btn_add_new_clothes_clicked (GtkButton *button, gpointer data)
{
  GtkTreeView *treeview;
  GtkTreeModel *model;
  GtkListStore *store;
  GtkTreeIter iter;
  gboolean valid;
  
  gboolean registrado; // Para saber si es un codigo previamente registrado
  gchar *marca, *descripcion, *sub_depto_code;
  gint i, fila, largo;
  i = fila = 0;

  sub_depto_code = gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_clothes_sub_depto")));
  marca = gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_new_clothes_brand")));
  descripcion = gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_new_clothes_desc")));
  
  //Se comprueba que tengan datos
  if (g_str_equal (marca, ""))
    {
      ErrorMSG (GTK_WIDGET (builder_get (builder, "entry_new_clothes_brand")),
			    "Debe ingresar una marca");
      return;
    }
  else if (g_str_equal (descripcion, ""))
    {
      ErrorMSG (GTK_WIDGET (builder_get (builder, "entry_new_clothes_desc")),
			    "Debe ingresar una descripción");
      return;
    }

  /* Se registra la información del subdepartamento 
     (se asocia 'descripcion') al fragmento sub_depto */
  registrar_nuevo_sub_depto (sub_depto_code, descripcion);
  
  // Se obtienen todos los colores
  fila = 0;
  treeview = GTK_TREE_VIEW (builder_get (builder, "treeview_colors"));
  
  largo = get_treeview_length (treeview);
  gchar *color, *codigo_color;
  gchar *colores[largo], *codigos_colores[largo];

  model = gtk_tree_view_get_model (treeview);
  store = GTK_LIST_STORE (model);
  valid = gtk_tree_model_get_iter_first (model, &iter);
  while (valid)
    {
      gtk_tree_model_get (model, &iter,
			  0, &registrado,
			  1, &codigo_color,
			  2, &color,
			  -1);

      if (registrado == FALSE)
	{
	  codigos_colores[fila] = g_strdup (codigo_color);
	  colores[fila] = g_strdup (color);
	  fila++;
	}

      valid = gtk_tree_model_iter_next (store, &iter); /* Me da TRUE si itera a la siguiente */
    }

  // Se registran los colores nuevos
  for (i=0; i<fila; i++)
    registrar_nuevo_color (g_strdup_printf ("%s", codigos_colores[i]),
			   g_strdup_printf ("%s", colores[i]));

  
  // Se obtienen todos los códigos generados 
  fila = 0;
  treeview = GTK_TREE_VIEW (builder_get (builder, "treeview_shortcodes"));

  largo = get_treeview_length (treeview);
  gchar *codigos[largo]; // Para guardar todos los codigos nuevos
  gchar *code;           // Para guardar el codigo del treeview

  model = gtk_tree_view_get_model (treeview);
  store = GTK_LIST_STORE (model);
  valid = gtk_tree_model_get_iter_first (model, &iter);
  while (valid)
    {
      gtk_tree_model_get (model, &iter,
			  0, &registrado,
			  1, &code,
			  -1);

      if (registrado == FALSE)
	{
	  codigos[fila] = g_strdup (code);
	  fila++;
	}
      valid = gtk_tree_model_iter_next (store, &iter); /* Me da TRUE si itera a la siguiente */
    }

  // Se registran todos los codigos nuevos que se han generado
  for (i=0; i<fila; i++)
    registrar_nuevo_codigo (g_strdup_printf ("%s", codigos[i]));

  // Se registran los productos
  for (i=0; i<fila; i++)
    AddNewProductToDB (codigos[i], "0", descripcion, marca, "1",
		       "UN", TRUE, 0, "", FALSE, FALSE);
  
  clean_container (GTK_CONTAINER (gtk_builder_get_object (builder, "wnd_new_clothing")));
  gtk_widget_hide (GTK_WIDGET (gtk_builder_get_object (builder, "wnd_new_clothing")));
}


/**
 * This callback is triggered when a cell on 
 * 'treeview_sizes' is edited. (editable-event)
 * @param cell
 * @param path_string
 * @param new_size
 * @param data -> A GtkListStore
 */
void
on_code_color_cell_renderer_edited (GtkCellRendererText *cell, gchar *path_string, gchar *new_c_color, gpointer data)
{
  GtkTreeModel *model = GTK_TREE_MODEL (data);
  GtkTreePath *path = gtk_tree_path_new_from_string (path_string);
  GtkTreeIter iter;
  
  /* obtiene el iter para poder obtener y setear 
     datos del treeview */
  gtk_tree_model_get_iter (model, &iter, path);

  //Se asegura que el código del color contenga 2 dígitos
  if (strlen (new_c_color) != 2)
    {
      ErrorMSG (GTK_WIDGET (builder_get (builder, "treeview_colors")),
		"El código del color debe contener 2 dígitos");
      return;
    }

  // Se setean los datos modificados en el treeview
  gtk_list_store_set (GTK_LIST_STORE (model), &iter,
		      0, FALSE,    // False indica que es un dato agregado a mano
		      1, new_c_color, // Se le inserta la nueva talla
		      -1);

}

/**
 * This callback is triggered when a cell on 
 * 'treeview_sizes' is edited. (editable-event)
 * @param cell
 * @param path_string
 * @param new_size
 * @param data -> A GtkListStore
 */
void
on_color_cell_renderer_edited (GtkCellRendererText *cell, gchar *path_string, gchar *new_color, gpointer data)
{
  GtkTreeModel *model = GTK_TREE_MODEL (data);
  GtkTreePath *path = gtk_tree_path_new_from_string (path_string);
  GtkTreeIter iter;
  
  /* obtiene el iter para poder obtener y setear 
     datos del treeview */
  gtk_tree_model_get_iter (model, &iter, path);
  
  // Se setean los datos modificados en el treeview
  gtk_list_store_set (GTK_LIST_STORE (model), &iter,
		      0, FALSE,    // False indica que es un dato agregado a mano
		      2, new_color, // Se le inserta la nueva talla
		      -1);

}

/**
 * Es llamada por la función "on_button_new_product_clicked"
 * cuando ROPA=1 en el .rizoma.
 *
 * Visualiza la ventana "wnd_new_clothing" y carga los
 * respectivos datos de ésta: los entry's, botones, y combo box'
 *
 * @param void
 */
void
create_new_clothing (void)
{
  GtkListStore *store;
  GtkTreeView *treeview;
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;
  GtkTreeSelection *selection;

  treeview = GTK_TREE_VIEW (gtk_builder_get_object (builder, "treeview_sizes"));
  store = GTK_LIST_STORE(gtk_tree_view_get_model(treeview));
  if (store == NULL)
    {
      //Tallas
      store = gtk_list_store_new (2,
				  G_TYPE_BOOLEAN, //Para saber si se rellenó desde la BD o se agregó a mano
				  G_TYPE_STRING); //Tallas

      gtk_tree_view_set_model (GTK_TREE_VIEW (treeview), GTK_TREE_MODEL (store));

      renderer = gtk_cell_renderer_text_new ();
      g_object_set (renderer,
		    "editable", TRUE,
		    NULL);
      g_signal_connect (G_OBJECT (renderer), "edited",
			G_CALLBACK (on_sizes_cell_renderer_edited), (gpointer)store);
      column = gtk_tree_view_column_new_with_attributes ("Talla", renderer,
							 "text", 1,
							 NULL);
      gtk_tree_view_append_column (treeview, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
      gtk_tree_view_column_set_resizable (column, FALSE);
    }

  treeview = GTK_TREE_VIEW (gtk_builder_get_object (builder, "treeview_colors"));
  store = GTK_LIST_STORE(gtk_tree_view_get_model(treeview));
  if (store == NULL)
    {
      //Colores
      store = gtk_list_store_new (3,
				  G_TYPE_BOOLEAN, //Para saber si se rellenó desde la BD o se agregó a mano
				  G_TYPE_STRING,  //CODIGO
				  G_TYPE_STRING); //NOMBRE

      gtk_tree_view_set_model (GTK_TREE_VIEW (treeview), GTK_TREE_MODEL (store));

      renderer = gtk_cell_renderer_text_new ();
      g_object_set (renderer,
		    "editable", TRUE,
		    NULL);
      g_signal_connect (G_OBJECT (renderer), "edited",
			G_CALLBACK (on_code_color_cell_renderer_edited), (gpointer)store);
      column = gtk_tree_view_column_new_with_attributes ("Codigo", renderer,
							 "text", 1,
							 NULL);
      gtk_tree_view_append_column (treeview, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
      gtk_tree_view_column_set_resizable (column, FALSE);

      renderer = gtk_cell_renderer_text_new ();
      g_object_set (renderer,
		    "editable", TRUE,
		    NULL);
      g_signal_connect (G_OBJECT (renderer), "edited",
			G_CALLBACK (on_color_cell_renderer_edited), (gpointer)store);
      column = gtk_tree_view_column_new_with_attributes ("Nombre", renderer,
							 "text", 2,
							 NULL);
      gtk_tree_view_append_column (treeview, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
      gtk_tree_view_column_set_resizable (column, FALSE);
    }

  treeview = GTK_TREE_VIEW (gtk_builder_get_object (builder, "treeview_shortcodes"));
  store = GTK_LIST_STORE(gtk_tree_view_get_model(treeview));
  if (store == NULL)
    {
      //Clothes Code
      store = gtk_list_store_new (2,
				  G_TYPE_BOOLEAN, //Para saber si se rellenó desde la BD o se agregó a mano
				  G_TYPE_STRING); //Clothes Code

      gtk_tree_view_set_model (GTK_TREE_VIEW (treeview), GTK_TREE_MODEL (store));

      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes ("Codigo", renderer,
							 "text", 1,
							 NULL);
      gtk_tree_view_append_column (treeview, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
      gtk_tree_view_column_set_resizable (column, FALSE);

      //Barcodes
      store = gtk_list_store_new (2,
				  G_TYPE_BOOLEAN, //Para saber si se rellenó desde la BD o se agregó a mano
				  G_TYPE_STRING); //BARCODE

      treeview = GTK_TREE_VIEW (gtk_builder_get_object (builder, "treeview_barcodes"));
      gtk_tree_view_set_model (GTK_TREE_VIEW (treeview), GTK_TREE_MODEL (store));

      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes ("Barcode", renderer,
							 "text", 1,
							 NULL);
      gtk_tree_view_append_column (treeview, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
      gtk_tree_view_column_set_resizable (column, FALSE);
    }

  //Limitaciones para los entry
  gtk_entry_set_max_length (GTK_ENTRY (builder_get (builder, "entry_clothes_depto")), 2);
  gtk_entry_set_max_length (GTK_ENTRY (builder_get (builder, "entry_clothes_temp")), 3);
  gtk_entry_set_max_length (GTK_ENTRY (builder_get (builder, "entry_clothes_year")), 2);
  gtk_entry_set_max_length (GTK_ENTRY (builder_get (builder, "entry_clothes_sub_depto")), 3);
  gtk_entry_set_max_length (GTK_ENTRY (builder_get (builder, "entry_clothes_id")), 2);

  //Botones deshabilitados
  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_add_size")), FALSE);
  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_del_size")), FALSE);
  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_add_color")), FALSE);
  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_del_color")), FALSE);
  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_add_new_clothes")), FALSE);

  //Habilitar los widget de construcción de código
  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "entry_clothes_depto")), TRUE);
  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "entry_clothes_temp")), TRUE);
  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "entry_clothes_year")), TRUE);
  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "entry_clothes_sub_depto")), TRUE);
  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "entry_clothes_id")), TRUE);
  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_accept_clothes")), TRUE);

  //widget's deshabilitados
  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "entry_new_clothes_brand")), FALSE);
  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "entry_new_clothes_desc")), FALSE);
  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_create_codes")), FALSE);

  gtk_widget_grab_focus (GTK_WIDGET (builder_get (builder, "entry_clothes_depto")));
  gtk_widget_show_all (GTK_WIDGET (builder_get (builder, "wnd_new_clothing")));
}


/**
 * Esta funcion es llamada cuando el boton "button_new_product" es presionado
 * (signal click).
 *
 * visualiza la ventana "wnd_new_product" o "wnd_new_clothing" 
 * dependindo de la opción "ROPA" en el .rizoma.
 *
 * @param button the button
 * @param user_data the user data
 */
void
on_button_new_product_clicked (GtkButton *button, gpointer data)
{
  if (g_str_equal(rizoma_get_value ("ROPA"),"0"))
    create_new_product ();
  else
    create_new_clothing ();
}


/**
 * Esta funcion es llamada cuando el boton "btn_accept_clothes" es presionado
 * (signal click).
 *
 * Revisa la existencia del código en la base de datos y rellena los campos
 * correspondientes con la información adquirida, de no existir habilita los
 * campos para la creación de nuevos productos.
 *
 * @param button the button
 * @param user_data the user data
 */
void
on_btn_accept_clothes_clicked (GtkButton *button, gpointer data)
{ 
  //Variables para el treeview
  GtkListStore *store;
  GtkTreeIter iter;

  PGresult *res;
  gchar *clothes_code = g_strdup_printf ("");
  gchar *q;
  gchar *depto, *sub_depto;
  gint i, num_res = 0;
  GObject *clothes_base_code[6];
  GObject *clothes_next_widget[6];

  //Fracciones del codigo de ropa
  clothes_base_code[0] = builder_get (builder, "entry_clothes_depto");
  clothes_base_code[1] = builder_get (builder, "entry_clothes_temp");
  clothes_base_code[2] = builder_get (builder, "entry_clothes_year");
  clothes_base_code[3] = builder_get (builder, "entry_clothes_sub_depto");
  clothes_base_code[4] = builder_get (builder, "entry_clothes_id");
  clothes_base_code[5] = builder_get (builder, "btn_accept_clothes");

  //Los widgets del 2 paso
  clothes_next_widget[0] = builder_get (builder, "btn_add_size");
  clothes_next_widget[1] = builder_get (builder, "btn_del_size");
  clothes_next_widget[2] = builder_get (builder, "btn_add_color");
  clothes_next_widget[3] = builder_get (builder, "btn_del_color");
  clothes_next_widget[4] = builder_get (builder, "btn_create_codes");
  //clothes_next_widget[4] = builder_get (builder, "entry_new_clothes_brand");
  //clothes_next_widget[5] = builder_get (builder, "entry_new_clothes_desc");

  //Valida los campos
  for (i = 0; i < 5; i++)
    {
      if (i==0 || i==2 || i==4)
	{
	  if (strlen (gtk_entry_get_text (GTK_ENTRY (clothes_base_code[i]))) != 2)
	    {
	      ErrorMSG (GTK_WIDGET (clothes_base_code[i]),
			g_strdup_printf ("El campo %s debe contener 2 dígitos", (i==0) ?
					 "departamento":(i==2) ? "año":"id"));
	      return;
	    }
	}
      else
	{
	  if (strlen (gtk_entry_get_text (GTK_ENTRY (clothes_base_code[i]))) != 3)
	    {
	      ErrorMSG (GTK_WIDGET (clothes_base_code[i]),
			g_strdup_printf ("El campo %s debe contener 3 dígitos", (i==1) ?
					 "temporada":"sub departamento"));
	      return;
	    }
	}
    }

  //Deshabilita la escritura en los entry (fracciones de 'clothes code') y el boton
  for (i = 0; i < 6; i++)
    gtk_widget_set_sensitive (GTK_WIDGET (clothes_base_code[i]), FALSE);

  //Concadena todas las partes del "clothes code"
  for (i = 0; i < 5; i++)
    clothes_code = g_strdup_printf ("%s%s", clothes_code, gtk_entry_get_text (GTK_ENTRY (clothes_base_code[i])));

  //Habilita la escritura en los widgets que sigen
  for (i = 0; i < 5; i++)
    gtk_widget_set_sensitive (GTK_WIDGET (clothes_next_widget[i]), TRUE);


  //Deja el foco en el botón para agregar tallas
  gtk_widget_grab_focus (GTK_WIDGET (clothes_next_widget[0]));

  /* == Se rellena el treeview de talla == */
  depto = gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_clothes_depto")));
  q = g_strdup_printf ("SELECT DISTINCT talla "
  		       "FROM clothes_code "
		       "WHERE depto = '%s'", 
		       depto);
  res = EjecutarSQL(q);
  num_res = PQntuples (res);
  
  //Si encuentra tallas correspondientes al departamento
  if (num_res > 0)
    {
      //Rellena el treeview con las tallas correspondientes al departamento
      store = GTK_LIST_STORE (gtk_tree_view_get_model 
			      (GTK_TREE_VIEW (gtk_builder_get_object (builder, "treeview_sizes"))));
      for (i=0; i<num_res; i++)
	{
	  gtk_list_store_append (store, &iter);
	  gtk_list_store_set (store, &iter,
			      0, TRUE,
			      1, g_strdup (PQvaluebycol(res, i, "talla")),
			      -1);
	}
    }

  /* == Se rellena el treeview de colores == */
  q = g_strdup_printf ("SELECT codigo, nombre "
  		       "FROM color");
  res = EjecutarSQL(q);
  num_res = PQntuples (res);
  
  //Si encuentra colores pre-existentes
  if (num_res > 0)
    {
      //Rellena el treeview todos los colores disponibles
      store = GTK_LIST_STORE (gtk_tree_view_get_model 
			      (GTK_TREE_VIEW (gtk_builder_get_object (builder, "treeview_colors"))));
      for (i=0; i<num_res; i++)
	{
	  gtk_list_store_append (store, &iter);
	  gtk_list_store_set (store, &iter,
			      0, TRUE,
			      1, g_strdup (PQvaluebycol(res, i, "codigo")),
			      2, g_strdup (PQvaluebycol(res, i, "nombre")),
			      -1);
	}
    }

  /* == Se rellena el treeview de codigo == */
  q = g_strdup_printf ("SELECT codigo_corto "
  		       "FROM producto "
		       "WHERE codigo_corto like '%s%%' "
		       "AND length (codigo_corto) = 16",
  		       clothes_code);
  res = EjecutarSQL(q);
  num_res = PQntuples (res);
  
  //Si encuentra el codigo base
  if (num_res > 0)
    {
      //Rellena el treeview con los codigos ya existentes
      store = GTK_LIST_STORE (gtk_tree_view_get_model 
			      (GTK_TREE_VIEW (gtk_builder_get_object (builder, "treeview_shortcodes"))));
      for (i=0; i<num_res; i++)
	{
	  gtk_list_store_append (store, &iter);
	  gtk_list_store_set (store, &iter,
			      0, TRUE,
			      1, g_strdup (PQvaluebycol(res, i, "codigo_corto")),
			      -1);
	}
    }

  /* == Se ingresa la descripcion correspondiente al sub_depto == */
  sub_depto = gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_clothes_sub_depto")));
  q = g_strdup_printf ("SELECT nombre "
  		       "FROM sub_depto "
		       "WHERE codigo = '%s'", 
		       sub_depto);

  res = EjecutarSQL(q);
  num_res = PQntuples (res);

  //Si encuentra la descripcion del sub_depto
  if (num_res > 0)
    gtk_entry_set_text (GTK_ENTRY (builder_get (builder, "entry_new_clothes_desc")),
			g_strdup (PQvaluebycol(res, 0, "nombre")));
}


/**
 * Is called by 'btn_add_size' button when is pressed (signal-clicked)
 *
 * make a new row on 'treeview_sizes'
 */
void
on_btn_add_size_clicked ()
{
  GtkTreeView *treeview;
  GtkListStore *store;
  GtkTreeIter iter;

  treeview = GTK_TREE_VIEW (builder_get (builder, "treeview_sizes"));
  store = GTK_LIST_STORE (gtk_tree_view_get_model (treeview));

  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter,
		      0, FALSE,
		      1, "Ingrese Talla",
		      -1);
}


/**
 * Is called by 'btn_del_size' button when is pressed (signal-clicked)
 *
 * delete the selected row on 'treeview_sizes'
 */
void
on_btn_del_size_clicked ()
{
  GtkTreeView *treeview;
  GtkListStore *store;
  GtkTreeSelection *selection;
  GtkTreeIter iter;
  gint position;
  gboolean datoExistente;
  gchar *nombreTalla;

  gboolean base_existente;
  gint i = 0;
  gchar *code_base = g_strdup_printf("");
  GObject *clothes_base_code[5];
  clothes_base_code[0] = builder_get (builder, "entry_clothes_depto");
  clothes_base_code[1] = builder_get (builder, "entry_clothes_temp");
  clothes_base_code[2] = builder_get (builder, "entry_clothes_year");
  clothes_base_code[3] = builder_get (builder, "entry_clothes_sub_depto");
  clothes_base_code[4] = builder_get (builder, "entry_clothes_id");

  for (i=0; i<5; i++)
    code_base = g_strdup_printf ("%s%s", code_base, gtk_entry_get_text (GTK_ENTRY (clothes_base_code[i])));

  base_existente = DataExist (g_strdup_printf ("SELECT * FROM clothes_code WHERE codigo_corto like '%s%%'",code_base));

  treeview = GTK_TREE_VIEW (builder_get (builder, "treeview_sizes"));
  selection = gtk_tree_view_get_selection (treeview);
  store = GTK_LIST_STORE (gtk_tree_view_get_model (treeview));

  if (gtk_tree_selection_get_selected (selection, NULL, &iter))
    {
      // Comprueba que no sea una fila de una talla pre-existente
      // (Que no se haya agregado a través de una consulta sql)
      gtk_tree_model_get (GTK_TREE_MODEL (store), &iter,
                          0, &datoExistente,
			  1, &nombreTalla,
                          -1);

      if (datoExistente == FALSE || (datoExistente == TRUE && base_existente == FALSE))
	{
	  position = atoi (gtk_tree_model_get_string_from_iter(GTK_TREE_MODEL(store), &iter));
	  gtk_list_store_remove (store, &iter);
	  select_back_deleted_row("treeview_sizes", position);
	}
      else
	ErrorMSG (GTK_WIDGET (builder_get (builder, "btn_del_size")), 
		  g_strdup_printf ("No puede eliminar la talla %s, pues existen productos con ella", nombreTalla));
    }
}


/**
 * Is called by 'btn_add_color' button when is pressed (signal-clicked)
 *
 * make a new row on 'treeview_color'
 */
void
on_btn_add_color_clicked ()
{
  GtkTreeView *treeview;
  GtkListStore *store;
  GtkTreeIter iter;

  treeview = GTK_TREE_VIEW (builder_get (builder, "treeview_colors"));
  store = GTK_LIST_STORE (gtk_tree_view_get_model (treeview));

  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter,
		      0, FALSE,
		      1, "Cod",
		      2, "Color",
		      -1);
}


/**
 * Is called by 'btn_del_color' button when is pressed (signal-clicked)
 *
 * delete the selected row on 'treeview_colors'
 */
void
on_btn_del_color_clicked ()
{
  GtkTreeView *treeview;
  GtkListStore *store;
  GtkTreeSelection *selection;
  GtkTreeIter iter;
  gint position;
  gboolean datoExistente;
  gchar *codColor;
  gchar *nombreColor;

  gboolean base_existente;
  gint i = 0;
  gchar *code_base = g_strdup_printf("");
  GObject *clothes_base_code[5];
  clothes_base_code[0] = builder_get (builder, "entry_clothes_depto");
  clothes_base_code[1] = builder_get (builder, "entry_clothes_temp");
  clothes_base_code[2] = builder_get (builder, "entry_clothes_year");
  clothes_base_code[3] = builder_get (builder, "entry_clothes_sub_depto");
  clothes_base_code[4] = builder_get (builder, "entry_clothes_id");

  for (i=0; i<5; i++)
    code_base = g_strdup_printf ("%s%s", code_base, gtk_entry_get_text (GTK_ENTRY (clothes_base_code[i])));

  base_existente = DataExist (g_strdup_printf ("SELECT * FROM clothes_code WHERE codigo_corto like '%s%%'",code_base));

  treeview = GTK_TREE_VIEW (builder_get (builder, "treeview_colors"));
  selection = gtk_tree_view_get_selection (treeview);
  store = GTK_LIST_STORE (gtk_tree_view_get_model (treeview));

  if (gtk_tree_selection_get_selected (selection, NULL, &iter))
    {
      // Comprueba que no sea una fila de una talla pre-existente
      // (Que no se haya agregado a través de una consulta sql)
      gtk_tree_model_get (GTK_TREE_MODEL (store), &iter,
                          0, &datoExistente,
			  1, &codColor,
			  2, &nombreColor,
                          -1);

      if (datoExistente == FALSE || (datoExistente == TRUE && base_existente == FALSE))
	{
	  position = atoi (gtk_tree_model_get_string_from_iter(GTK_TREE_MODEL(store), &iter));
	  gtk_list_store_remove (store, &iter);
	  select_back_deleted_row("treeview_colors", position);
	}
      else
	ErrorMSG (GTK_WIDGET (builder_get (builder, "btn_del_color")), 
		  g_strdup_printf ("No puede eliminar el color %s, pues existen productos con él", nombreColor));
    }
}


/**
 * Esta funcion es llamada cuando se activa el delete-event
 *
 * Esconde los dos botones de la venta.
 *
 */
void
on_wnd_new_unit_delete_event(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
  gtk_widget_hide(GTK_WIDGET (builder_get (builder, "btn_save_unidad")));
  gtk_widget_hide(GTK_WIDGET (builder_get (builder, "btn_save_unidad2")));
}

/**
 * Esta funcion es llamada cuando el boton "btn_save_unidad" es presionado
 * (signal click).
 *
 * Esta funcion agregar la nueva unidad que se ingreso en "entry_unidad" a la
 * base datos
 *
 */
void
Save_new_unit (GtkButton *button, gpointer user_data)
{
  GtkTreeIter iter;
  GtkComboBox *combo;
  GtkWindow *parent, *win;
  GtkButton *btn;
  GtkListStore *store;
  GtkCellRenderer *cell;
  PGresult *res2;
  gint i, tuples;
  GtkTreeModel *model;
  gboolean valid;
  gchar *unidad = g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_unidad"))));
  gchar *tipo = gtk_buildable_get_name(button);


  win = GTK_WINDOW (gtk_builder_get_object(builder, "wnd_new_unit"));

  if(strcmp(tipo, "btn_save_unidad") == 0)
    {
      parent = GTK_WINDOW (gtk_builder_get_object(builder, "wnd_new_product"));
      gtk_window_set_transient_for(win, parent);
      combo = GTK_COMBO_BOX (gtk_builder_get_object(builder, "cmb_box_new_product_unit"));
      btn = GTK_BUTTON (gtk_builder_get_object(builder, "btn_new_unidad"));
      gtk_widget_hide(GTK_WIDGET (gtk_builder_get_object(builder, "btn_save_unidad")));
    }
  else
    {
      printf("lalaaalala entre \n");
      parent = GTK_WINDOW (gtk_builder_get_object(builder, "wnd_mod_product"));
      gtk_window_set_transient_for(win, parent);
      combo = GTK_COMBO_BOX (gtk_builder_get_object(builder, "cmb_box_edit_product_unit"));
      btn = GTK_BUTTON (gtk_builder_get_object(builder, "btn_new_unidad"));
      gtk_widget_hide(GTK_WIDGET (gtk_builder_get_object(builder, "btn_save_unidad2")));
    }

  gchar *q = g_strdup_printf("SELECT * FROM unidad_producto where descripcion = upper('%s')", unidad);
  res2 = EjecutarSQL(q);
  tuples = PQntuples (res2);

  if(tuples > 0)
    {
      gtk_widget_hide(GTK_WIDGET(win));
      gtk_entry_set_text(GTK_ENTRY (gtk_builder_get_object (builder, "entry_unidad")), "");
      ErrorMSG (GTK_WIDGET (btn), "La unidad que ingreso ya existe.");
    }
  else
    {
      q = g_strdup_printf("INSERT INTO unidad_producto VALUES (DEFAULT, upper('%s'))", unidad);

      res2 = EjecutarSQL(q);

      res2 = EjecutarSQL ("SELECT * FROM unidad_producto");
      tuples = PQntuples (res2);

      /* metodo parcial para limpiar un combo box  */
      /* TODO: debe haber un metodo mas eficiente */
      i=0;
      do
	{
	  model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));

	  if(valid=gtk_tree_model_get_iter_first (model, &iter) == 1 && i<tuples-1)
	    {
	      store = GTK_LIST_STORE(gtk_combo_box_get_model(combo));

	      gtk_list_store_remove(store, &iter);
	      i++;
	    }
	  else
	    valid = 0;
	}
      while(valid == 1);

      /* Ahora repoblar el combo box */
      res2 = EjecutarSQL ("SELECT * FROM unidad_producto");
      tuples = PQntuples (res2);

      if(res2 != NULL)
	{
	  for (i=0 ; i < tuples ; i++)
	    {
	      gtk_list_store_append(GTK_LIST_STORE(model), &iter);
	      gtk_list_store_set(GTK_LIST_STORE(model), &iter,
				 0, atoi(PQvaluebycol(res2, i, "id")),
				 1, PQvaluebycol(res2, i, "descripcion"),
				 -1);
	    }
	}

      model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));

      gtk_combo_box_set_active(combo, tuples-1);
      gtk_widget_hide(GTK_WIDGET(win));
      gtk_entry_set_text(GTK_ENTRY (gtk_builder_get_object (builder, "entry_unidad")), "");
    }
}

void
on_button_buy_clicked (GtkButton *button, gpointer data)
{
  if (compra->header_compra != NULL)
    BuyWindow ();
}


void
on_button_ok_ingress_clicked (GtkButton *button, gpointer data)
{
  GtkWindow *wnd_ingress = GTK_WINDOW (gtk_builder_get_object (builder, "wnd_ingress_buy"));
  gboolean complete = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (gtk_builder_get_object (builder, "radiobutton_ingress_complete")));
  gboolean factura = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (gtk_builder_get_object (builder, "radiobutton_ingress_document")));

  gchar str_date[256];
  GtkEntry *entry;
  GDate *date = g_date_new ();

  gtk_widget_hide_all (GTK_WIDGET (wnd_ingress));

  if (complete)
    {
      if (factura)
        {
          GtkWindow *wnd_invoice = GTK_WINDOW (gtk_builder_get_object (builder, "wnd_ingress_invoice"));
          clean_container (GTK_CONTAINER (wnd_invoice));

          entry = GTK_ENTRY (builder_get (builder, "entry_ingress_factura_date"));

          /* Suggested amount */
          gtk_entry_set_text (GTK_ENTRY (builder_get (builder, "entry_ingress_factura_amount")),
                              CutPoints (g_strdup (gtk_label_get_text (GTK_LABEL (gtk_builder_get_object (builder, "label_pending_total"))))));
          gtk_editable_select_region (GTK_EDITABLE (builder_get (builder, "entry_ingress_factura_amount")), 0, -1);

          gtk_widget_show_all (GTK_WIDGET (wnd_invoice));
        }
      else
        {
          GtkWindow *wnd_guide = GTK_WINDOW (gtk_builder_get_object (builder, "wnd_ingress_guide"));
          clean_container (GTK_CONTAINER (wnd_guide));

          entry = GTK_ENTRY (builder_get (builder, "entry_ingress_guide_date"));

          /* Suggested amount */
          gtk_entry_set_text (GTK_ENTRY (builder_get (builder, "entry_ingress_guide_amount")),
                              CutPoints (g_strdup (gtk_label_get_text (GTK_LABEL (gtk_builder_get_object (builder, "label_pending_total"))))));
          gtk_editable_select_region (GTK_EDITABLE (builder_get (builder, "entry_ingress_guide_amount")), 0, -1);

          gtk_widget_show_all (GTK_WIDGET (wnd_guide));
        }
    }
  else
    {
      if (factura)
        {
          GtkListStore *store;
          GtkTreeView *treeview = GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_partial_invoice"));
          GtkTreeViewColumn *column;
          GtkCellRenderer *renderer;

          if (gtk_tree_view_get_model (treeview) == NULL)
            {
              store = gtk_list_store_new (8,
                                          G_TYPE_STRING,
                                          G_TYPE_STRING,
                                          G_TYPE_STRING,
                                          G_TYPE_DOUBLE,
                                          G_TYPE_DOUBLE,
                                          G_TYPE_STRING,
                                          G_TYPE_STRING,
                                          G_TYPE_BOOLEAN);

              gtk_tree_view_set_model (GTK_TREE_VIEW (treeview), GTK_TREE_MODEL (store));

              renderer = gtk_cell_renderer_text_new ();
              column = gtk_tree_view_column_new_with_attributes ("Codigo", renderer,
                                                                 "text", 0,
                                                                 "foreground", 6,
                                                                 "foreground-set", 7,
                                                                 NULL);
              gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
              gtk_tree_view_column_set_alignment (column, 0.5);
              g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
              gtk_tree_view_column_set_resizable (column, FALSE);

              renderer = gtk_cell_renderer_text_new ();
              column = gtk_tree_view_column_new_with_attributes ("Producto", renderer,
                                                                 "text", 1,
                                                                 "foreground", 6,
                                                                 "foreground-set", 7,
                                                                 NULL);
              gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
              gtk_tree_view_column_set_alignment (column, 0.5);
              gtk_tree_view_column_set_resizable (column, FALSE);

              renderer = gtk_cell_renderer_text_new ();
              column = gtk_tree_view_column_new_with_attributes ("Unit.", renderer,
                                                                 "text", 2,
                                                                 "foreground", 6,
                                                                 "foreground-set", 7,
                                                                 NULL);
              gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
              gtk_tree_view_column_set_alignment (column, 0.5);
              gtk_tree_view_column_set_resizable (column, FALSE);

              renderer = gtk_cell_renderer_text_new ();
              column = gtk_tree_view_column_new_with_attributes ("Cant. Sol.", renderer,
                                                                 "text", 3,
                                                                 "foreground", 6,
                                                                 "foreground-set", 7,
                                                                 NULL);
              gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
              gtk_tree_view_column_set_alignment (column, 0.5);
              g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
              gtk_tree_view_column_set_resizable (column, FALSE);

              gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)3, NULL);

              renderer = gtk_cell_renderer_text_new ();
              g_object_set (renderer,
                            "editable", TRUE,
                            NULL);
              g_signal_connect (G_OBJECT (renderer), "edited",
                                G_CALLBACK (on_partial_cell_renderer_edited), (gpointer)store);

              column = gtk_tree_view_column_new_with_attributes ("Cant. Ing.", renderer,
                                                                 "text", 4,
                                                                 "foreground", 6,
                                                                 "foreground-set", 7,
                                                                 NULL);
              gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
              gtk_tree_view_column_set_alignment (column, 0.5);
              g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
              gtk_tree_view_column_set_resizable (column, FALSE);

              gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)4, NULL);

              renderer = gtk_cell_renderer_text_new ();
              column = gtk_tree_view_column_new_with_attributes ("Sub-Total", renderer,
                                                                 "text", 5,
                                                                 "foreground", 6,
                                                                 "foreground-set", 7,
                                                                 NULL);
              gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
              gtk_tree_view_column_set_alignment (column, 0.5);
              g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
              gtk_tree_view_column_set_resizable (column, FALSE);
            } // if (gtk_tree_view_get_model (treeview) == NULL)

          clean_container (GTK_CONTAINER (builder_get (builder, "wnd_ingress_partial_invoice")));

          entry = GTK_ENTRY (builder_get (builder, "entry_ingress_partial_invoice_date"));

          /* Suggested amount */
          gtk_entry_set_text (GTK_ENTRY (builder_get (builder, "entry_ingress_partial_invoice_amount")),
                              CutPoints (g_strdup (gtk_label_get_text (GTK_LABEL (gtk_builder_get_object (builder, "label_pending_total"))))));
          gtk_editable_select_region (GTK_EDITABLE (builder_get (builder, "entry_ingress_partial_invoice_amount")), 0, -1);


          FillPartialTree (treeview);
          gtk_widget_show_all (GTK_WIDGET (builder_get (builder, "wnd_ingress_partial_invoice")));
        } // if (factura)
      else
        {
          GtkListStore *store;
          GtkTreeView *treeview = GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_partial_guide"));
          GtkTreeViewColumn *column;
          GtkCellRenderer *renderer;

          if (gtk_tree_view_get_model (treeview) == NULL)
            {
              store = gtk_list_store_new (8,
                                          G_TYPE_STRING,
                                          G_TYPE_STRING,
                                          G_TYPE_STRING,
                                          G_TYPE_DOUBLE,
                                          G_TYPE_DOUBLE,
                                          G_TYPE_STRING,
                                          G_TYPE_STRING,
                                          G_TYPE_BOOLEAN);

              gtk_tree_view_set_model (GTK_TREE_VIEW (treeview), GTK_TREE_MODEL (store));

              renderer = gtk_cell_renderer_text_new ();
              column = gtk_tree_view_column_new_with_attributes ("Codigo", renderer,
                                                                 "text", 0,
                                                                 "foreground", 6,
                                                                 "foreground-set", 7,
                                                                 NULL);
              gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
              gtk_tree_view_column_set_alignment (column, 0.5);
              g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
              gtk_tree_view_column_set_resizable (column, FALSE);

              renderer = gtk_cell_renderer_text_new ();
              column = gtk_tree_view_column_new_with_attributes ("Producto", renderer,
                                                                 "text", 1,
                                                                 "foreground", 6,
                                                                 "foreground-set", 7,
                                                                 NULL);
              gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
              gtk_tree_view_column_set_alignment (column, 0.5);
              gtk_tree_view_column_set_resizable (column, FALSE);

              renderer = gtk_cell_renderer_text_new ();
              column = gtk_tree_view_column_new_with_attributes ("Unit.", renderer,
                                                                 "text", 2,
                                                                 "foreground", 6,
                                                                 "foreground-set", 7,
                                                                 NULL);
              gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
              gtk_tree_view_column_set_alignment (column, 0.5);
              gtk_tree_view_column_set_resizable (column, FALSE);

              renderer = gtk_cell_renderer_text_new ();
              column = gtk_tree_view_column_new_with_attributes ("Cant. Sol.", renderer,
                                                                 "text", 3,
                                                                 "foreground", 6,
                                                                 "foreground-set", 7,
                                                                 NULL);
              gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
              gtk_tree_view_column_set_alignment (column, 0.5);
              g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
              gtk_tree_view_column_set_resizable (column, FALSE);

              gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)3, NULL);

              renderer = gtk_cell_renderer_text_new ();
              g_object_set (renderer,
                            "editable", TRUE,
                            NULL);
              g_signal_connect (G_OBJECT (renderer), "edited",
                                G_CALLBACK (on_partial_cell_renderer_edited), (gpointer)store);

              column = gtk_tree_view_column_new_with_attributes ("Cant. Ing.", renderer,
                                                                 "text", 4,
                                                                 "foreground", 6,
                                                                 "foreground-set", 7,
                                                                 NULL);
              gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
              gtk_tree_view_column_set_alignment (column, 0.5);
              g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
              gtk_tree_view_column_set_resizable (column, FALSE);

              gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)4, NULL);

              renderer = gtk_cell_renderer_text_new ();
              column = gtk_tree_view_column_new_with_attributes ("Sub-Total", renderer,
                                                                 "text", 5,
                                                                 "foreground", 6,
                                                                 "foreground-set", 7,
                                                                 NULL);
              gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
              gtk_tree_view_column_set_alignment (column, 0.5);
              g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
              gtk_tree_view_column_set_resizable (column, FALSE);
            } // if (gtk_tree_view_get_model (treeview) == NULL)

          clean_container (GTK_CONTAINER (builder_get (builder, "wnd_ingress_partial_guide")));

          entry = GTK_ENTRY (builder_get (builder, "entry_ingress_partial_guide_date"));

          /* Suggested amount */
          gtk_entry_set_text (GTK_ENTRY (builder_get (builder, "entry_ingress_partial_guide_amount")),
                              CutPoints (g_strdup (gtk_label_get_text (GTK_LABEL (gtk_builder_get_object (builder, "label_pending_total"))))));
          gtk_editable_select_region (GTK_EDITABLE (builder_get (builder, "entry_ingress_partial_guide_amount")), 0, -1);

          FillPartialTree (treeview);
          gtk_widget_show_all (GTK_WIDGET (builder_get (builder, "wnd_ingress_partial_guide")));
        } // else => if (factura)
    } // else => if (complete)

  g_date_set_time_t (date, time (NULL));

  if (g_date_strftime (str_date, sizeof (str_date), "%x", date) == 0)
    strcpy (str_date, "---");

  gtk_entry_set_text (entry, str_date);
} // void on_button_ok_ingress_clicked (GtkButton *button, gpointer data)


void
on_quit_message_response (GtkDialog *dialog, gint response_id, gpointer user_data)
{
  if (response_id == GTK_RESPONSE_YES)
    {
      gtk_main_quit();
    }
  else
    if (response_id == GTK_RESPONSE_NO)
      gtk_widget_hide (GTK_WIDGET (dialog));
}


gboolean
on_wnd_compras_delete_event (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
  GtkWidget *window;
  window = GTK_WIDGET (gtk_builder_get_object (builder, "quit_message"));
  gtk_widget_show_all (window);
  return TRUE;
}


void
on_entry_ingress_factura_amount_activate (GtkWidget *btn_ok)
{
  gint total = atoi (CutPoints(g_strdup (gtk_label_get_text (GTK_LABEL (gtk_builder_get_object (builder, "label_pending_total"))))));
  gint total_doc = atoi (g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_ingress_factura_amount")))));

  CheckMontoIngreso (btn_ok, total, total_doc);
}


void
on_btn_ok_ingress_invoice_clicked (GtkWidget *widget, gpointer data)
{
  GtkWidget *wnd = GTK_WIDGET (gtk_builder_get_object (builder, "wnd_ingress_invoice"));

  AskElabVenc (wnd, TRUE);
  gtk_widget_set_sensitive( GTK_WIDGET (gtk_builder_get_object (builder, "btn_ok_ingress_invoice")), FALSE);
}


void
on_entry_ingress_guide_amount_activate (GtkWidget *btn_ok)
{
  gint total = atoi (CutPoints(g_strdup (gtk_label_get_text (GTK_LABEL (gtk_builder_get_object (builder, "label_pending_total"))))));
  gint total_doc = atoi (g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_ingress_guide_amount")))));

  CheckMontoIngreso (btn_ok, total, total_doc);
}


void
on_btn_ok_ingress_guide_clicked (GtkWidget *widget, gpointer data)
{
  GtkWidget *wnd = GTK_WIDGET (gtk_builder_get_object (builder, "wnd_ingress_guide"));

  AskElabVenc (wnd, FALSE);
}


void
on_entry_srch_provider_activate (GtkEntry *entry)
{
  GtkListStore *store;
  GtkTreeIter iter;
  PGresult *res;
  gint tuples, i;
  gchar *str_schr = g_strdup (gtk_entry_get_text (entry));
  gchar *str_axu;
  gchar *q;

  q = g_strdup_printf ("SELECT rut, dv, nombre "
                       "FROM buscar_proveedor ('%%%s%%')", str_schr);
  g_free (str_schr);

  res = EjecutarSQL (q);
  g_free (q);

  tuples = PQntuples (res);

  store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_srch_provider"))));

  gtk_list_store_clear (store);

  for (i = 0; i < tuples; i++)
    {
      str_axu = g_strconcat(PQvaluebycol (res, i, "rut"),"-",
                            PQvaluebycol (res, i, "dv"), NULL);

      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
                          0, PQvaluebycol (res, i, "nombre"),
                          1, str_axu,
                          -1);
      g_free (str_axu);
    }
}


void
wnd_search_provider (GtkEntry *entry, gpointer user_data)
{
  GtkWindow *window;
  GtkTreeView *tree = GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_srch_provider"));
  GtkListStore *store;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  gchar *srch_provider = g_strdup (gtk_entry_get_text (entry));
  gchar *str_schr = g_strdup (gtk_entry_get_text (entry));


  if (gtk_tree_view_get_model (tree) == NULL )
    {
      store = gtk_list_store_new (2,
                                  G_TYPE_STRING,
                                  G_TYPE_STRING);

      gtk_tree_view_set_model (GTK_TREE_VIEW (tree), GTK_TREE_MODEL (store));

      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes ("Proveedor", renderer,
                                                         "text", 0,
                                                         NULL);
      gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
      gtk_tree_view_column_set_resizable (column, FALSE);

      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes ("Rut Proveedor", renderer,
                                                         "text", 1,
                                                         NULL);
      gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
      gtk_tree_view_column_set_resizable (column, FALSE);
    }

  window = GTK_WINDOW (gtk_builder_get_object (builder, "wnd_srch_provider"));
  gtk_entry_set_text (GTK_ENTRY (gtk_builder_get_object(builder, "entry_srch_provider")), str_schr);
  on_entry_srch_provider_activate(entry);

  gtk_widget_show_all (GTK_WIDGET (window));
}


void
on_entry_guide_invoice_provider_activate (GtkEntry *entry, gpointer data)
{
  wnd_search_provider (entry, data);
}


void
on_btn_find_srch_provider_clicked (GtkEntry *entry, gpointer data)
{
  on_entry_srch_provider_activate (entry);
}


void
on_btn_ok_srch_provider_clicked (GtkTreeView *tree)
{
  GtkTreeSelection *selection = gtk_tree_view_get_selection (tree);
  GtkTreeModel *model = gtk_tree_view_get_model (tree);
  GtkTreeIter iter;
  gchar *str;
  gchar **strs;
  gint tab = gtk_notebook_get_current_page ( GTK_NOTEBOOK (builder_get (builder, "buy_notebook")));
  gboolean guide;

  if (gtk_tree_selection_get_selected (selection, NULL, &iter) == TRUE)
    {
      gtk_tree_model_get (model, &iter,
                          1, &str,
                          -1);

      strs = g_strsplit (str, "-", 2);

      if (gtk_widget_get_visible (GTK_WIDGET (builder_get (builder, "wnd_nullify_buy"))))
	{
	  gtk_entry_set_text (GTK_WIDGET (builder_get (builder, "entry_nullify_buy_provider")), *strs);
	  on_btn_nullify_buy_search_clicked (NULL, NULL);
	}
      else
	{
	  //guide = tab == 2 ? TRUE : FALSE; //Ex page 2 "Ingreso Factura" is disabled
	  FillProveedorData (*strs, FALSE);
	}

      gtk_widget_hide (GTK_WIDGET (builder_get (builder, "wnd_srch_provider")));
    }
}


void
on_tree_view_srch_provider_row_activated (GtkTreeView *tree)
{
  on_btn_ok_srch_provider_clicked (tree);
}


void
on_btn_guide_invoice_clicked (GtkButton *button, gpointer data)
{
  GtkTreeView *tree_pending_guide = GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_pending_guide"));
  GtkTreeView *tree_guide_invoice = GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_guide_invoice"));
  GtkTreeSelection *selection = gtk_tree_view_get_selection (tree_pending_guide);
  GtkTreeModel *model = gtk_tree_view_get_model (tree_pending_guide);
  GtkTreeIter iter;
  gchar *id;
  gchar *n_guide;
  gchar *monto;

  if (gtk_tree_selection_get_selected (selection, NULL, &iter) == TRUE)
    {
      gtk_tree_model_get (model, &iter,
                          0, &id,
                          1, &n_guide,
                          2, &monto,
                          -1);
      gtk_list_store_remove (GTK_LIST_STORE (model), &iter);


      model = gtk_tree_view_get_model (tree_guide_invoice);

      gtk_list_store_append (GTK_LIST_STORE (model), &iter);
      gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                          0, id,
                          1, n_guide,
                          2, monto,
                          -1);
    }
}


void
on_btn_invoice_guide_clicked (GtkButton *button, gpointer date)
{
  GtkTreeView *tree_pending_guide = GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_pending_guide"));
  GtkTreeView *tree_guide_invoice = GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_guide_invoice"));
  GtkTreeSelection *selection = gtk_tree_view_get_selection (tree_guide_invoice);
  GtkTreeModel *model = gtk_tree_view_get_model (tree_guide_invoice);
  GtkTreeIter iter;
  gchar *id;
  gchar *n_guide;
  gchar *monto;

  if (gtk_tree_selection_get_selected (selection, NULL, &iter) == TRUE)
    {
      gtk_tree_model_get (model, &iter,
                          0, &id,
                          1, &n_guide,
                          2, &monto,
                          -1);
      gtk_list_store_remove (GTK_LIST_STORE (model), &iter);

      model = gtk_tree_view_get_model (tree_pending_guide);

      gtk_list_store_append (GTK_LIST_STORE (model), &iter);
      gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                          0, id,
                          1, n_guide,
                          2, monto,
                          -1);
    }
}


void
on_tree_selection_pending_guide_changed (GtkTreeSelection *selection, gpointer user_data)
{
  GtkTreeView *tree = GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_guide_invoice_detail"));
  GtkTreeModel *model = gtk_tree_view_get_model (gtk_tree_selection_get_tree_view (selection));
  GtkTreeIter iter;
  gchar *n_guide;
  gchar *rut_provider;

  PGresult *res;
  gint tuples, i;
  gchar *q;

  if (gtk_tree_selection_get_selected (selection, NULL, &iter) == TRUE)
    {
      gtk_tree_model_get (model, &iter,
                          1, &n_guide,
                          -1);
      rut_provider = g_strdup (gtk_label_get_text (GTK_LABEL (gtk_builder_get_object (builder, "label_guide_invoice_rut"))));

      q = g_strdup_printf ("SELECT * FROM get_guide_detail(%s, %s)", n_guide, rut_provider);
      res = EjecutarSQL (q);
      g_free (q);

      if (res == NULL) return;

      tuples = PQntuples (res);

      model = gtk_tree_view_get_model (tree);
      gtk_list_store_clear (GTK_LIST_STORE (model));

      for (i = 0; i < tuples; i++)
        {
          gtk_list_store_append (GTK_LIST_STORE (model), &iter);
          gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                              0, PQvaluebycol (res, i, "codigo_corto"),
                              1, g_strdup_printf ("%s %s %s %s", PQvaluebycol (res, i, "descripcion"),
                                                  PQvaluebycol (res, i, "marca"), PQvaluebycol (res, i, "contenido"),
                                                  PQvaluebycol (res, i, "unidad")),
                              2, PQvaluebycol(res, i, "precio"),
                              3, strtod (PUT(PQvaluebycol(res, i, "cantidad")), (char **)NULL),
                              4, (gdouble) (strtod (PUT(PQvaluebycol(res, i, "precio")), (char **)NULL) * strtod (PUT(PQvaluebycol(res, i, "cantidad")), (char **)NULL)),
                              -1);
        }
    }
}


void
on_btn_guide_invoice_ok_clicked (void)
{
  PGresult *res;

  GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_guide_invoice")));
  GtkTreeIter iter;

  gchar *id;
  gchar *guia;
  gint factura;

  gchar *rut = g_strdup (gtk_label_get_text (GTK_LABEL (gtk_builder_get_object (builder, "label_guide_invoice_rut"))));
  gchar *monto = g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_guide_invoice_amount"))));

  gint n_fact = atoi (g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_guide_invoice_n_invoice")))));

  gchar *date_str = g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_guide_invoice_date"))));
  GDate *date = g_date_new ();
  GDate *date_guide = g_date_new ();

  g_date_set_parse (date, date_str);

  if (!g_date_valid (date))
    {
      ErrorMSG (GTK_WIDGET (builder_get (builder, "entry_guide_invoice_date")),
                "Debe ingresar una fecha valida");
      return;
    }

  gtk_tree_model_get_iter_first (model, &iter);

  gtk_tree_model_get (model, &iter,
                      1, &guia,
                      -1);

  res = EjecutarSQL (g_strdup_printf
                     ("SELECT date_part('day', fecha), date_part('month', fecha), "
                      "date_part('year', fecha) FROM compra WHERE id=(SELECT id_compra FROM "
                      "guias_compra WHERE numero=%s AND rut_proveedor='%s')", guia, rut));

  if (res == NULL || PQntuples (res) == 0)
    return;

  g_date_set_dmy (date_guide, atoi (PQgetvalue( res, 0, 0)), atoi (PQgetvalue( res, 0, 1)), atoi (PQgetvalue( res, 0, 2)));

  if (g_date_compare (date_guide, date) > 0)
    {
      ErrorMSG (GTK_WIDGET (builder_get (builder, "entry_guide_invoice_date")),
                "La fecha de emision del documento no puede ser menor al de la guia");
    }

  if (strcmp (rut, "") == 0)
    {
      ErrorMSG (compra->tree_prov, "Debe Seleccionar un proveedor");
      return;
    }
  else if (n_fact == 0)
    {
      ErrorMSG (compra->n_factura, "Debe Ingresar el numero de la factura");
      return;
    }
  else if (strcmp (monto, "") == 0)
    {
      ErrorMSG (compra->fact_monto, "Debe Ingresar el Monto de la Factura");
      return;
    }

  factura = IngresarFactura (n_fact, 0, rut, atoi (monto), g_date_get_day (date), g_date_get_month (date), g_date_get_year (date), atoi (guia));


  gtk_tree_model_get_iter_first (model, &iter);

  do {
    gtk_tree_model_get (model, &iter,
                        0, &id,
                        -1);

    AsignarFactAGuia (atoi (id), factura);
  }  while ((gtk_tree_model_iter_next (model, &iter)) != FALSE);

  ClearFactData ();
}


void
ClearFactData (void)
{
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_guide_invoice_address")), "");
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_guide_invoice_mail")), "");
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_guide_invoice_comuna")), "");
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_guide_invoice_contact")), "");
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_guide_invoice_fono")), "");
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_guide_invoice_rut")), "");
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_guide_invoice_date_emit")), "");
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_guide_invoice_web")), "");

  gtk_entry_set_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_guide_invoice_provider")), "");
  gtk_entry_set_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_guide_invoice_n_invoice")), "");
  gtk_entry_set_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_guide_invoice_amount")), "");

  gtk_list_store_clear (GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (builder_get (builder, "tree_view_pending_guide")))));
  gtk_list_store_clear (GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (builder_get (builder, "tree_view_guide_invoice")))));
  gtk_list_store_clear (GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (builder_get (builder, "tree_view_guide_invoice_detail")))));
}


void
on_entry_invoice_provider_activate (GtkEntry *entry, gpointer data)
{

  //on_entry_srch_provider_activate(entry);
  wnd_search_provider (entry, data);
}


void
on_tree_view_invoice_list_selection_changed (GtkTreeSelection *selection, gpointer data)
{
  gint i, tuples;
  gchar *id_invoice;
  gchar *q;
  GtkTreeView *tree = GTK_TREE_VIEW (builder_get (builder, "tree_view_invoice_detail"));
  GtkTreeModel *model = gtk_tree_view_get_model (gtk_tree_selection_get_tree_view (selection));
  GtkTreeIter iter;

  PGresult *res;

  if (gtk_tree_selection_get_selected (selection, NULL, &iter) == TRUE)
    {
      gtk_tree_model_get (model, &iter,
                          0, &id_invoice,
                          -1);

      q = g_strdup_printf ("SELECT * FROM get_invoice_detail(%s)", id_invoice);
      res = EjecutarSQL (q);
      g_free (q);

      if (res == NULL) return;

      tuples = PQntuples (res);

      model = gtk_tree_view_get_model (tree);
      gtk_list_store_clear (GTK_LIST_STORE (model));

      for (i = 0; i < tuples; i++)
        {
          gtk_list_store_append (GTK_LIST_STORE (model), &iter);
          gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                              0, PQvaluebycol (res, i, "codigo_corto"),
                              1, g_strdup_printf ("%s %s %s %s", PQvaluebycol (res, i, "descripcion"),
                                                  PQvaluebycol (res, i, "marca"), PQvaluebycol (res, i, "contenido"),
                                                  PQvaluebycol (res, i, "unidad")),
                              2, PQvaluebycol(res, i, "precio"),
                              3, strtod (PUT(PQvaluebycol(res, i, "cantidad")), (char **)NULL),
                              4, (gdouble) (strtod (PUT(PQvaluebycol(res, i, "precio")), (char **)NULL) * strtod (PUT(PQvaluebycol(res, i, "cantidad")), (char **)NULL)),
                              -1);
        }
    }
}


void
on_btn_pay_invoice_clicked (void)
{
  GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (builder_get (builder, "tree_view_invoice_list")));
  GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (builder_get (builder, "tree_view_invoice_list")));
  GtkTreeIter iter;
  gchar *id_invoice;
  gchar *rut_provider = g_strdup (gtk_label_get_text (GTK_LABEL (builder_get (builder, "label_invoice_rut"))));
  gint position;

  if (gtk_tree_selection_get_selected (selection, NULL, &iter) == TRUE)
    {
      gtk_tree_model_get (model, &iter,
                          0, &id_invoice,
                          -1);

      position = atoi (gtk_tree_model_get_string_from_iter(model, &iter));
      
      if (PagarFactura (atoi (id_invoice)) == FALSE)
        {
          ErrorMSG (GTK_WIDGET (builder_get (builder, "entry_invoice_provider")), "No se ingreso correctamente");
        }
      else
        {
          FillPagarFacturas (rut_provider);
	  select_back_deleted_row("tree_view_invoice_list", position);
        }
    }
}


void
FillPartialTree (GtkTreeView *tree)
{
  gint i, id, tuples;
  gboolean color;
  GtkTreeIter iter;

  GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model (tree));
  GtkListStore *store_pending_request = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (builder_get (builder, "tree_view_pending_requests"))));
  GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (builder_get (builder, "tree_view_pending_requests")));

  PGresult *res;

  gdouble sol, ing;
  gdouble cantidad;

  if (compra->header != NULL)
    CompraListClean ();

  if (gtk_tree_selection_get_selected (selection, NULL, &iter) == TRUE)
    {
      gtk_tree_model_get (GTK_TREE_MODEL (store_pending_request), &iter,
                          0, &id,
                          -1);

      gtk_list_store_clear (store);

      res = EjecutarSQL ( g_strdup_printf( "SELECT * FROM get_detalle_compra( %d ) ", id ) );

      if (res == NULL)
        return;

      tuples = PQntuples (res);

      if (tuples == 0)
        return;

      for (i = 0; i < tuples; i++)
        {
          sol = strtod (PUT (PQvaluebycol(res, i, "cantidad")), (char **)NULL);
          ing = strtod (PUT (PQvaluebycol(res, i, "cantidad_ingresada")), (char **)NULL);

          if (ing<sol && ing >0)
            color = TRUE;
          else
            color = FALSE;

          gtk_list_store_append (store, &iter);
          gtk_list_store_set (store, &iter,
                              0, PQvaluebycol(res, i, "codigo_corto"),
                              1, g_strdup_printf ("%s %s %s %s", PQvaluebycol(res, i, "descripcion"),
                                                  PQvaluebycol(res, i, "marca"), PQvaluebycol(res, i, "contenido"),
                                                  PQvaluebycol(res, i, "unidad")),
                              2, PQvaluebycol(res, i, "precio"),
                              3, strtod (PUT(PQvaluebycol(res, i, "cantidad")), (char **)NULL),
                              4, strtod (PUT(PQvaluebycol(res, i, "cantidad_ingresada")), (char **)NULL),
                              5, PutPoints (PQvaluebycol(res, i, "costo_ingresado")),
                              6, color ? "Red" : "Black",
                              7, TRUE,
                              -1);

          if (ing != 0)
            cantidad = (gdouble) sol - ing;
          else
            cantidad = (gdouble) sol;

          CompraAgregarALista (PQvaluebycol(res, i, "barcode"), cantidad, atoi (PQvaluebycol(res, i, "precio_venta")),
                               strtod (PUT((PQvaluebycol(res, i, "precio"))), (char **)NULL), atoi (PQvaluebycol(res, i, "margen")), TRUE);
        }
    } // if (gtk_tree_selection_get_selected (selection, NULL, &iter) == TRUE)
} // void FillPartialTree (GtkTreeView *tree)


void
on_partial_cell_renderer_edited (GtkCellRendererText *cell, gchar *path_string, gchar *new_amount, gpointer data)
{
  GtkTreeModel *model = GTK_TREE_MODEL (data);
  GtkTreePath *path = gtk_tree_path_new_from_string (path_string);
  GtkTreeIter iter;
  gdouble new_stock = strtod (PUT (new_amount), (char **)NULL);
  gchar *codigo;
  Productos *products;

  gtk_tree_model_get_iter (model, &iter, path);
  gtk_tree_path_free (path);

  gtk_tree_model_get (model, &iter,
                      0, &codigo,
                      -1);

  products = BuscarPorCodigo (compra->header, codigo);

  if (new_stock > 0 && new_stock <= products->product->cantidad)
    {
      gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                          4, new_stock,
                          -1);
      products->product->cantidad = new_stock;
      CalcularTotales ();

      if (GTK_WIDGET_VISIBLE (GTK_WIDGET (builder_get (builder, "wnd_ingress_partial_invoice"))))
        {
          gtk_entry_set_text (GTK_ENTRY (builder_get (builder, "entry_ingress_partial_invoice_amount")),
                              CutPoints (g_strdup (gtk_label_get_text (GTK_LABEL (gtk_builder_get_object (builder, "label_pending_total"))))));
          gtk_editable_select_region (GTK_EDITABLE (builder_get (builder, "entry_ingress_partial_invoice_amount")), 0, -1);
        }
      else if (GTK_WIDGET_VISIBLE (GTK_WIDGET (builder_get (builder, "wnd_ingress_partial_guide"))))
        {
          gtk_entry_set_text (GTK_ENTRY (builder_get (builder, "entry_ingress_partial_guide_amount")),
                              CutPoints (g_strdup (gtk_label_get_text (GTK_LABEL (gtk_builder_get_object (builder, "label_pending_total"))))));
          gtk_editable_select_region (GTK_EDITABLE (builder_get (builder, "entry_ingress_partial_guide_amount")), 0, -1);
        }

    }
  else
    {
      ErrorMSG (NULL, "El stock a ingresa debe ser mayor a 0 y menor a la cantidad solicitada");
    }
}


void
on_btn_ok_ingress_partial_guide_clicked (GtkWidget *widget, gpointer data)
{
  GtkWidget *wnd = GTK_WIDGET (gtk_builder_get_object (builder, "wnd_ingress_partial_guide"));

  AskElabVenc (wnd, FALSE);
}


void
on_btn_ok_ingress_partial_invoice_clicked (GtkWidget *widget, gpointer data)
{
  GtkWidget *wnd = GTK_WIDGET (gtk_builder_get_object (builder, "wnd_ingress_partial_invoice"));

  AskElabVenc (wnd, TRUE);
}


void
on_entry_ingress_partial_invoice_amount_activate (GtkWidget *btn_ok)
{
  gint total = atoi (CutPoints(g_strdup (gtk_label_get_text (GTK_LABEL (gtk_builder_get_object (builder, "label_pending_total"))))));
  gint total_doc = atoi (g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_ingress_partial_invoice_amount")))));

  CheckMontoIngreso (btn_ok, total, total_doc);
}


void
on_entry_ingress_partial_guide_amount_activate (GtkWidget *btn_ok)
{
  gint total = atoi (CutPoints(g_strdup (gtk_label_get_text (GTK_LABEL (gtk_builder_get_object (builder, "label_pending_total"))))));
  gint total_doc = atoi (g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_ingress_partial_guide_amount")))));

  CheckMontoIngreso (btn_ok, total, total_doc);
}

/**
 * Esta funcion es llamada cuando el boton "btn_add_new_product" es
 * presionado (signal click).
 *
 * Esta funcion obtiene todos los datos del producto que se llenaron en la
 * ventana "wnd_new_product", para ingresar un nuevo producto a la base de datos
 *
 */
void
on_btn_add_new_product_clicked (GtkButton *button, gpointer data)
{
  GtkTreeModel *model;
  GtkTreeIter iter;

  GtkEntry *entry_code = GTK_ENTRY (builder_get (builder, "entry_new_product_code"));
  GtkEntry *entry_barcode = GTK_ENTRY (builder_get (builder, "entry_new_product_barcode"));
  GtkEntry *entry_desc = GTK_ENTRY (builder_get (builder, "entry_new_product_desc"));
  GtkEntry *entry_brand = GTK_ENTRY (builder_get (builder, "entry_new_product_brand"));
  GtkEntry *entry_cont = GTK_ENTRY (builder_get (builder, "entry_new_product_cont"));

  GtkComboBox *combo = GTK_COMBO_BOX (builder_get (builder, "cmbbox_new_product_imp_others"));
  GtkComboBox *combo_unit = GTK_COMBO_BOX (builder_get (builder, "cmb_box_new_product_unit"));

  gboolean iva = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (builder_get (builder, "radio_btn_task_yes")));
  gboolean fraccion = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (builder_get (builder,
										    "radio_btn_fractional_yes")));
  gchar *codigo = g_strdup (gtk_entry_get_text (entry_code));
  gchar *barcode = g_strdup (gtk_entry_get_text (entry_barcode));
  gchar *description = g_strdup (gtk_entry_get_text (entry_desc));
  gchar *marca = g_strdup (gtk_entry_get_text (entry_brand));
  gchar *contenido = g_strdup (gtk_entry_get_text (entry_cont));
  gchar *familia;
  gchar *unidad;
  gint otros;

  if (strcmp (codigo, "") == 0)
    ErrorMSG (GTK_WIDGET (entry_code), "Debe ingresar un codigo corto");
  else if (strlen (codigo) > 16)
    ErrorMSG (GTK_WIDGET (entry_code), "Código corto debe ser menor a 16 caracteres");
  else if (strcmp (barcode, "") == 0)
    ErrorMSG (GTK_WIDGET (entry_barcode), "Debe Ingresar un Codigo de Barras");
  else if (HaveCharacters(barcode))
    ErrorMSG (GTK_WIDGET (entry_barcode), "Código de barras debe ser un valor numérico");
  else if (strlen (barcode) > 18)
    ErrorMSG (GTK_WIDGET (entry_barcode), "Código de barras debe ser menor a 18 caracteres");
  else if (strcmp (description, "") == 0)
    ErrorMSG (GTK_WIDGET (entry_desc), "Debe Ingresar una Descripción");
  else if (strcmp (marca, "") == 0)
    ErrorMSG (GTK_WIDGET (entry_brand), "Debe Ingresar al Marca del producto");
  else if (strcmp (contenido, "") == 0)
    ErrorMSG (GTK_WIDGET (entry_cont), "Debe Ingresar el Contenido del producto");
  else if (HaveCharacters(contenido))
    ErrorMSG (GTK_WIDGET (entry_cont), "Contenido debe ser un valor numérico");
  else
    {
      if (DataExist (g_strdup_printf ("SELECT codigo_corto FROM informacion_producto_venta(NULL, '%s')", codigo)))
        {
          ErrorMSG (GTK_WIDGET (entry_code), "Ya existe un producto con el mismo codigo corto");
          return;
        }
      if (DataExist (g_strdup_printf ("SELECT barcode FROM informacion_producto_venta(%s, '')", barcode)))
        {
          ErrorMSG (GTK_WIDGET (entry_code), "Ya existe un producto con el mismo codigo de barras");
          return;
        }

      if (gtk_combo_box_get_active (combo) == 0)
        otros = 0;
      else
        {
          model = gtk_combo_box_get_model (combo);
          gtk_combo_box_get_active_iter (combo, &iter);
          gtk_tree_model_get (model, &iter,
                              0, &otros,
                              -1);
        }
      model = gtk_combo_box_get_model (combo_unit);
      gtk_combo_box_get_active_iter (combo_unit, &iter);

      gtk_tree_model_get (model, &iter,
			  1, &unidad,
			  -1);

      AddNewProductToDB (codigo, barcode, description, marca, CUT (contenido),
			 unidad, iva, otros, familia, FALSE, fraccion);

      gtk_widget_hide (GTK_WIDGET (builder_get (builder, "wnd_new_product")));

      SearchProductHistory (GTK_ENTRY (gtk_builder_get_object (builder, "entry_buy_barcode")), barcode);
    }
  return;
} // void on_btn_add_new_product_clicked (GtkButton *button, gpointer data)


/**
 * Es llamada cuando el boton "btn_get_request" es presionado (signal click).
 *
 * Esta funcion llama a a funcion "AskIngreso()".
 *
 */
void
on_btn_get_request_clicked (void)
{
  AskIngreso();
}


/**
 * Es llamada cuando el boton "btn_remove_buy_product" es presionado (signal click).
 *
 * Esta funcion elimina el producto seleccionado del treeView y ademas llama
 * a la funcion "DropBuyProduct()".
 *
 */
void
on_btn_remove_buy_product_clicked (void)
{
  GtkTreeView *treeview = GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_products_buy_list"));
  GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (builder_get (builder, "tree_view_products_buy_list")));
  GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (builder_get (builder, "tree_view_products_buy_list"))));
  GtkTreeIter iter;
  gchar *short_code;
  gint position;

  if (get_treeview_length(treeview) == 0)
    return;

  if (gtk_tree_selection_get_selected (selection, NULL, &iter))
    {
      gtk_tree_model_get (GTK_TREE_MODEL (store), &iter,
                          0, &short_code,
                          -1);

      DropBuyProduct (short_code);
      position = atoi (gtk_tree_model_get_string_from_iter(GTK_TREE_MODEL(store), &iter));
      gtk_list_store_remove (store, &iter);

      gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_total_buy")),
                            g_strdup_printf ("<span size=\"xx-large\"><b>%s</b></span>",
                                             PutPoints (g_strdup_printf ("%li", lround (CalcularTotalCompra (compra->header_compra))))));
      tipo_traspaso = 1;
    }

  select_back_deleted_row("tree_view_products_buy_list", position);

  if (get_treeview_length(treeview) == 0)
    {
      gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "btn_suggest_buy")), TRUE);
      gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "button_buy")), FALSE);
      
      //Se libera la variable global
      if (rut_proveedor_global != NULL)
	{
	  g_free (rut_proveedor_global);
	  rut_proveedor_global = NULL;
	}
    }
}


/**
 * Es llamada cuando el boton "button_add_product_list_trans" es presionado (signal click).
 *
 * Esta funcion agrega el prudcto a la lista (list) de productos y lo agrega
 * al treeView
 *
 */
void
AddToProductsListTraspaso (void)
{
  gchar *barcode = g_strdup (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_buy_barcode"))));
  gdouble cantidad;
  gdouble precio_compra = strtod (PUT (PQgetvalue (get_product_information (barcode, "", "costo_promedio"), 0, 0)), (char **)NULL);
  gint margen = 1;
  gint precio = atoi (PQgetvalue (get_product_information (barcode, "", "precio"), 0, 0));
  Producto *check;

  if (g_str_equal (barcode, ""))
    return;

  GtkListStore *store_history, *store_buy;

  store_buy = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (gtk_builder_get_object( builder, "tree_view_products_buy_list"))));

  cantidad = strtod (PUT (g_strdup (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_buy_amount_trans"))))), (char **)NULL);


  if (precio_compra != 0 && (strcmp (GetCurrentPrice (barcode), "0") == 0 || precio != 0)
      && strcmp (barcode, "") != 0)
    {

      if (compra->header_compra != NULL)
        check = SearchProductByBarcode (barcode, FALSE);
      else
        check = NULL;

      if (check == NULL)
        {
          if (CompraAgregarALista (barcode, cantidad, precio, precio_compra, margen, FALSE))
            {
              AddToTree ();
            }
          else
            {
              return;
            }
        }
      else
        {
          check->cantidad += cantidad;

          gtk_list_store_set (store_buy, &check->iter,
                              2, check->cantidad,
                              4, PutPoints (g_strdup_printf ("%li", lround ((gdouble)check->cantidad * check->precio_compra))),
                              -1);
        }

      if (strtod (PUT(gtk_label_get_text (GTK_LABEL (gtk_builder_get_object (builder, "label_stock")))), (char**)NULL) == 0)
        {
          gtk_widget_hide (GTK_WIDGET (builder_get (builder, "btn_traspaso_enviar")));
          tipo_traspaso = 0;
        }

      gtk_label_set_text (GTK_LABEL (builder_get (builder, "lbl_info"))," ");
      gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_total_buy")),
                            g_strdup_printf ("<span size=\"xx-large\"><b>%s</b></span>",
                                             PutPoints (g_strdup_printf
                                                        ("%li", lround (CalcularTotalCompra (compra->header_compra))))));
      store_history = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (gtk_builder_get_object( builder, "product_history_tree_view"))));
      gtk_list_store_clear (store_history);

      CleanStatusProduct ();

      gtk_widget_grab_focus (GTK_WIDGET (gtk_builder_get_object (builder, "entry_buy_barcode")));

    }
  else
    {
      ErrorMSG (GTK_WIDGET (gtk_builder_get_object (builder, "wnd_cant_traspaso")), "El Producto debe tener precio mayor a 0");
    }

  gtk_widget_hide (GTK_WIDGET (builder_get (builder, "entry_sell_price")));
  gtk_widget_hide (GTK_WIDGET (builder_get (builder, "entry_buy_price")));
  gtk_widget_hide (GTK_WIDGET (builder_get (builder, "entry_buy_gain")));
  gtk_widget_hide (GTK_WIDGET (builder_get (builder, "label28")));
  gtk_widget_hide (GTK_WIDGET (builder_get (builder, "label29")));
  gtk_widget_hide (GTK_WIDGET (builder_get (builder, "label1111")));
  //gtk_widget_hide (GTK_WIDGET (builder_get (builder, "cmbPrecioCompra")));
  gtk_widget_hide (GTK_WIDGET (builder_get (builder, "button_calculate")));

  gtk_widget_hide (GTK_WIDGET (builder_get (builder, "label126")));
  gtk_widget_hide (GTK_WIDGET (builder_get (builder, "entry_buy_amount")));
  gtk_widget_hide (GTK_WIDGET (builder_get (builder, "button_add_product_list")));

  gtk_widget_hide (GTK_WIDGET (gtk_builder_get_object (builder, "wnd_cant_traspaso")));

  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_traspaso_recibir")), TRUE);
  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_traspaso_enviar")), TRUE);
}


/**
 * Es llamada cuando el boton "btn_traspaso" es presionado (signal click).
 *
 * Esta funcion visualiza la ventana "wnd_cant_traspaso" y carga los
 * respectivos datos de esta, ademas desaparecer la opcion de comprar(entrys,labels,buttons)
 *
 */
gboolean
on_wnd_cant_traspaso (GtkWidget *widget, gpointer user_data)
{
  GtkWidget *window;

  gint barcode =atoi (g_strdup (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_buy_barcode")))));

  if (barcode != 0)
    {
      if (atoi(gtk_label_get_text (GTK_LABEL (gtk_builder_get_object (builder, "label_stock")))) == 0
          && tipo_traspaso == 0)
        {
          gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "lbl_info")),
                                g_strdup_printf ("<span size=\"xx-large\"><b>%s</b></span>",
                                                 "Solo podra recibir un traspaso"));
        }

      if (atoi(gtk_label_get_text (GTK_LABEL (gtk_builder_get_object (builder, "label_stock")))) > 0
          && tipo_traspaso == 0)
        {
          gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "lbl_info")),
                                g_strdup_printf ("<span size=\"xx-large\"><b>%s</b></span>",
                                                 "Solo podra recibir un traspaso"));
        }

      if (atoi(gtk_label_get_text (GTK_LABEL (gtk_builder_get_object (builder, "label_stock")))) == 0
          && tipo_traspaso == 1)
        {
          gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "lbl_info")),
                              "Este producto tiene 0 Stock, solo se puede recibir un traspaso");
        }

      if (atoi(gtk_label_get_text (GTK_LABEL (gtk_builder_get_object (builder, "label_stock")))) > 0
          && tipo_traspaso == 1)
        {
          gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "lbl_info")),
                              " ");
        }

      gtk_widget_hide (GTK_WIDGET (builder_get (builder, "entry_sell_price")));
      gtk_widget_hide (GTK_WIDGET (builder_get (builder, "entry_buy_price")));
      gtk_widget_hide (GTK_WIDGET (builder_get (builder, "entry_buy_gain")));
      gtk_widget_hide (GTK_WIDGET (builder_get (builder, "label28")));
      gtk_widget_hide (GTK_WIDGET (builder_get (builder, "label29")));
      gtk_widget_hide (GTK_WIDGET (builder_get (builder, "label1111")));
      //gtk_widget_hide (GTK_WIDGET (builder_get (builder, "cmbPrecioCompra")));
      gtk_widget_hide (GTK_WIDGET (builder_get (builder, "button_calculate")));

      gtk_widget_hide (GTK_WIDGET (builder_get (builder, "label126")));
      gtk_widget_hide (GTK_WIDGET (builder_get (builder, "entry_buy_amount")));
      gtk_widget_hide (GTK_WIDGET (builder_get (builder, "button_add_product_list")));

      gtk_widget_hide (GTK_WIDGET (builder_get (builder, "button_buy")));


      gtk_entry_set_text (GTK_ENTRY (builder_get (builder, "entry_buy_amount_trans")), "1");

      window = GTK_WIDGET (gtk_builder_get_object (builder, "wnd_cant_traspaso"));
      gtk_widget_show_all (window);
      return TRUE;
    }
  else
    {
      ErrorMSG (GTK_WIDGET (gtk_builder_get_object (builder, "barcode_entry")), "Ingrese un producto");
      gtk_widget_grab_focus (GTK_WIDGET (gtk_builder_get_object (builder, "entry_buy_barcode")));
      return FALSE;
    }
}


/**
 * Es llamada por la funcion "realizar_traspaso_Env()".
 *
 * Esta funcion carga las etiquetas (labels) de la ventana
 * "traspaso_enviar_win", ademas del combobox de destino, con los respectivos valores
 *
 */
void
DatosEnviar (void)
{
  GtkWidget *combo;
  GtkTreeIter iter;
  GtkListStore *modelo;
  gint tuples,i;
  PGresult *res;
  gint venta_id;

  gtk_widget_grab_focus (GTK_WIDGET (gtk_builder_get_object (builder, "comboboxDestino")));
  gdouble total = CalcularTotalCompra (compra->header_compra);
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_monto_total")),
                      PUT(g_strdup_printf ("%.2f",total)));
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_origen")),(gchar *)ReturnNegocio());
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "labelID")),g_strdup_printf ("%d",InsertIdTraspaso()+1));
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_vendedor")),"admin");


  res = EjecutarSQL (g_strdup_printf ("SELECT id,nombre FROM bodega "
                                      "WHERE nombre!=(SELECT nombre FROM negocio) AND estado = true"));
  tuples = PQntuples (res);

  combo = GTK_WIDGET (gtk_builder_get_object(builder, "comboboxDestino"));
  modelo = GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(combo)));


  if (modelo == NULL)
    {
      GtkCellRenderer *cell;
      modelo = gtk_list_store_new (2,
				   G_TYPE_INT,
				   G_TYPE_STRING);

      gtk_combo_box_set_model(GTK_COMBO_BOX(combo), GTK_TREE_MODEL(modelo));

      cell = gtk_cell_renderer_text_new();
      gtk_cell_layout_pack_start (GTK_CELL_LAYOUT(combo), cell, TRUE);
      gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo), cell,
                                     "text", 1,
                                     NULL);
    }

  gtk_list_store_clear(modelo);

  for (i=0 ; i < tuples ; i++)
    {
      gtk_list_store_append(modelo, &iter);
      gtk_list_store_set(modelo, &iter,
                         0, atoi(PQvaluebycol(res, i, "id")),
                         1, PQvaluebycol(res, i, "nombre"),
                         -1);
    }
}


/**
 * Es llamada por la funcion "realizar_traspaso_Rec()".
 *
 * Esta funcion carga las etiquetas (labels) de la ventana
 * "traspaso_recibir_win", ademas del combobox de origen, con los respectivos valores
 *
 */
void
DatosRecibir (void)
{
  GtkWidget *combo;
  GtkTreeIter iter;
  GtkListStore *modelo;
  gint tuples,i;
  PGresult *res;
  gint venta_id;

  gtk_widget_grab_focus (GTK_WIDGET (gtk_builder_get_object (builder, "comboboxOrigen")));
  gdouble total = CalcularTotalCompra (compra->header_compra);
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_monto_total1")),
                      PUT(g_strdup_printf ("%.2f",total)));
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_destino")),(gchar *)ReturnNegocio());
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "labelID1")),g_strdup_printf ("%d",InsertIdTraspaso()+1));
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_vendedor1")),"admin");

  res = EjecutarSQL (g_strdup_printf ("SELECT id,nombre FROM bodega "
					      "WHERE nombre!=(SELECT nombre FROM negocio) AND estado = true"));
  tuples = PQntuples (res);

  combo = GTK_WIDGET (gtk_builder_get_object(builder, "comboboxOrigen"));
  modelo = GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(combo)));


  if (modelo == NULL)
    {
      GtkCellRenderer *cell;
      modelo = gtk_list_store_new (2,
				   G_TYPE_INT,
				   G_TYPE_STRING);

      gtk_combo_box_set_model(GTK_COMBO_BOX(combo), GTK_TREE_MODEL(modelo));

      cell = gtk_cell_renderer_text_new();
      gtk_cell_layout_pack_start (GTK_CELL_LAYOUT(combo), cell, TRUE);
      gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo), cell,
                                     "text", 1,
                                     NULL);
    }

  gtk_list_store_clear(modelo);

  for (i=0 ; i < tuples ; i++)
    {
      gtk_list_store_append(modelo, &iter);
      gtk_list_store_set(modelo, &iter,
                         0, atoi(PQvaluebycol(res, i, "id")),
                         1, PQvaluebycol(res, i, "nombre"),
                         -1);
    }
}


/**
 * Es llamada cuando el boton "btn_traspaso_enviar" es presionado (signal click).
 *
 * Esta funcion visualiza la ventana "traspaso_enviar_win" y ademas llama a
 * la funcion "DatosEnviar()".
 *
 * @param widget the widget that emits the signal
 * @param data the user data
 *
 */
void
realizar_traspaso_Env (GtkWidget *widget, gpointer data)
{
  GtkWindow *window;

  if (compra->header_compra == NULL)
    {
      ErrorMSG (GTK_WIDGET (gtk_builder_get_object (builder, "barcode_entry")), "No hay productos para traspasar");
      return;
    }

  else
    {
      window = GTK_WINDOW (gtk_builder_get_object (builder, "traspaso_enviar_win"));
      clean_container (GTK_CONTAINER (window));
      gtk_widget_show_all (GTK_WIDGET (window));
      DatosEnviar();
      gtk_widget_grab_focus (GTK_WIDGET (builder_get (builder, "comboboxDestino")));
      return;
    }
}


/**
 * Es llamada cuando el boton "btn_traspaso_recibir" es presionado (signal click).
 *
 * Esta funcion visualiza la ventana "traspaso_recibir_win" y ademas llama a
 * la funcion "DatosRecibir()".
 *
 * @param widget the widget that emits the signal
 * @param data the user data
 *
 */
void
realizar_traspaso_Rec (GtkWidget *widget, gpointer data)
{
  GtkWindow *window;

  if (compra->header_compra == NULL)
    {
      ErrorMSG (GTK_WIDGET (gtk_builder_get_object (builder, "barcode_entry")), "No hay productos para traspasar");
      return;
    }

  else
    {
      window = GTK_WINDOW (gtk_builder_get_object (builder, "traspaso_recibir_win"));
      clean_container (GTK_CONTAINER (window));
      gtk_widget_show_all (GTK_WIDGET (window));
      DatosRecibir();
      gtk_widget_grab_focus (GTK_WIDGET (builder_get (builder, "comboboxOrigen")));
      return;
    }
}


/**
 * Es llamada cuando el boton "traspaso_button" es presionado (signal click).
 *
 * Esta funcion visualiza la ventana "traspaso_enviar_win" y ademas vuelve a
 * visualizar la opcion "comprar"
 *
 * @param widget the widget that emits the signal
 * @param data the user data
 *
 */
void
on_enviar_button_clicked (GtkButton *button, gpointer data)
{
  gint * destino;
  GtkTreeIter iter;
  GtkWidget *combo;
  GtkTreeModel *model;
  gint active;
  gint vendedor = 1; //user_data->user_id;
  gdouble monto = CalcularTotalCompra (compra->header_compra);

  combo = GTK_WIDGET (gtk_builder_get_object(builder, "comboboxDestino"));
  active = gtk_combo_box_get_active (GTK_COMBO_BOX (combo));

  if (active == -1)
    {
      ErrorMSG (combo, "Debe Seleccionar un Destino");
    }
  else
    {
      model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));
      gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combo), &iter);

      gtk_tree_model_get (model, &iter,
			  0, &destino,
			  -1);

      SaveTraspasoCompras (monto,
			   ReturnBodegaID(ReturnNegocio()),
			   vendedor,
			   destino,
			   TRUE);

      gtk_widget_hide (gtk_widget_get_toplevel (GTK_WIDGET (button)));

      gtk_widget_hide (GTK_WIDGET(gtk_builder_get_object(builder, "traspaso_enviar_win")));

      gtk_widget_grab_focus (GTK_WIDGET (gtk_builder_get_object (builder, "entry_buy_barcode")));

      ClearAllCompraData ();

      CleanStatusProduct ();

      compra->header_compra = NULL;
      compra->products_compra = NULL;

      gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_total_buy")), "");

      gtk_widget_show (GTK_WIDGET (builder_get (builder, "entry_sell_price")));

      gtk_widget_show (GTK_WIDGET (builder_get (builder, "entry_buy_price")));
      gtk_widget_show (GTK_WIDGET (builder_get (builder, "entry_buy_gain")));
      gtk_widget_show (GTK_WIDGET (builder_get (builder, "label28")));
      gtk_widget_show (GTK_WIDGET (builder_get (builder, "label29")));
      gtk_widget_show (GTK_WIDGET (builder_get (builder, "label1111")));
      //gtk_widget_show (GTK_WIDGET (builder_get (builder, "cmbPrecioCompra")));
      gtk_widget_show (GTK_WIDGET (builder_get (builder, "button_calculate")));

      gtk_widget_show (GTK_WIDGET (builder_get (builder, "label126")));
      gtk_widget_show (GTK_WIDGET (builder_get (builder, "entry_buy_amount")));
      gtk_widget_show (GTK_WIDGET (builder_get (builder, "button_add_product_list")));

      gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_traspaso")), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_traspaso_recibir")), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_traspaso_enviar")), FALSE);
    } // else => if (active == -1)
} // void on_enviar_button_clicked (GtkButton *button, gpointer data)


/**
 * Es llamada cuando el boton "traspaso_button1" es presionado (signal click).
 *
 * Esta funcion visualiza la ventana "traspaso_recibir_win"  y ademas vuelve a
 * visualizar la opcion "comprar"
 *
 * @param widget the widget that emits the signal
 * @param data the user data
 *
 */
void
on_recibir_button_clicked (GtkButton *button, gpointer data)
{
  gint * origen;
  GtkTreeIter iter;
  GtkWidget *combo;
  GtkTreeModel *model;
  gint active;
  gint vendedor = 1; //user_data->user_id;
  gdouble monto = CalcularTotalCompra (compra->header_compra);

  combo = GTK_WIDGET (gtk_builder_get_object(builder, "comboboxOrigen"));
  active = gtk_combo_box_get_active (GTK_COMBO_BOX (combo));

  if (active == -1)
    {
      ErrorMSG (combo, "Debe Seleccionar un Origen");
    }
  else
    {
      model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));
      gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combo), &iter);

      gtk_tree_model_get (model, &iter,
			  0, &origen,
			  -1);

      SaveTraspasoCompras (monto,
			   origen,
			   vendedor,
			   ReturnBodegaID(ReturnNegocio()),
			   FALSE);

      gtk_widget_hide (gtk_widget_get_toplevel (GTK_WIDGET (button)));

      gtk_widget_hide (GTK_WIDGET(gtk_builder_get_object(builder, "traspaso_recibir_win")));

      gtk_widget_grab_focus (GTK_WIDGET (gtk_builder_get_object (builder, "entry_buy_barcode")));

      ClearAllCompraData ();

      CleanStatusProduct ();

      compra->header_compra = NULL;
      compra->products_compra = NULL;

      gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_total_buy")), "");

      gtk_widget_show (GTK_WIDGET (builder_get (builder, "entry_sell_price")));

      gtk_widget_show (GTK_WIDGET (builder_get (builder, "entry_buy_price")));
      gtk_widget_show (GTK_WIDGET (builder_get (builder, "entry_buy_gain")));
      gtk_widget_show (GTK_WIDGET (builder_get (builder, "label28")));
      gtk_widget_show (GTK_WIDGET (builder_get (builder, "label29")));
      gtk_widget_show (GTK_WIDGET (builder_get (builder, "label1111")));
      //gtk_widget_show (GTK_WIDGET (builder_get (builder, "cmbPrecioCompra")));
      gtk_widget_show (GTK_WIDGET (builder_get (builder, "button_calculate")));

      gtk_widget_show (GTK_WIDGET (builder_get (builder, "label126")));
      gtk_widget_show (GTK_WIDGET (builder_get (builder, "entry_buy_amount")));
      gtk_widget_show (GTK_WIDGET (builder_get (builder, "button_add_product_list")));

      gtk_widget_show (GTK_WIDGET (builder_get (builder, "btn_traspaso_enviar")));

      gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_traspaso")), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_traspaso_recibir")), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_traspaso_enviar")), FALSE);
    } // else => if (active == -1)
} // void on_recibir_button_clicked (GtkButton *button, gpointer data)


/**
 * Callback connected to the toggle button of the products_providers treeview
 *
 * This function enable or disable the product's selection.
 * @param toggle the togle button
 * @param path_str the path of the selected row
 * @param data the user data
 */

void
ToggleProductSelection (GtkCellRendererToggle *toggle, char *path_str, gpointer data)
{
  GtkWidget *treeview;
  GtkTreeModel *store;
  gboolean enable, valid;
  GtkTreePath *path;
  GtkTreeIter iter;
  gdouble subTotal = 0, precio = 0, sugerido = 0;

  treeview = GTK_WIDGET (gtk_builder_get_object (builder, "tree_view_products_providers"));
  store = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
  path = gtk_tree_path_new_from_string (path_str);

  gtk_tree_model_get_iter (store, &iter, path);
  gtk_tree_model_get (store, &iter,
                      0, &enable,
		      8, &sugerido,
                      -1);

  // Se cambia la seleccion del checkButton
  enable = !(enable);
  gtk_list_store_set (GTK_LIST_STORE(store), &iter,
                      0, enable,
		      8, (sugerido == 0) ? 1 : sugerido,
                      -1);

  // Se calcula el subtotal de todas las filas que fueron seleccionadas
  valid = gtk_tree_model_get_iter_first (store, &iter);
  while (valid)
    {
      gtk_tree_model_get (store, &iter,
			  0, &enable,
			  3, &precio,
			  8, &sugerido,
			  -1);
      if (enable == TRUE)
	subTotal = subTotal + (precio * ((sugerido > 0) ? sugerido : 1));

      // Itero a la siguiente fila --
      valid = gtk_tree_model_iter_next (store, &iter); /* Me da TRUE si itera a la siguiente */
    }

  gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "lbl_sum_selected")),
			g_strdup_printf ("<b>%u</b>", llround(subTotal)));

  if (subTotal > 0)
    {
      gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_buy_selected")), TRUE);
      gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_print_suggest_buy")), TRUE);
    }
  else
    {
      gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_buy_selected")), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_print_suggest_buy")), FALSE);
    }

  gtk_tree_path_free (path);
}


/**
 * Es llamada cuando se edita "Costo" en el tree_view_products_providers (signal edited).
 *
 * Esta funcion actualiza el campo "Costo" por el valor indicado por el usuario.
 *
 */
void
on_buy_price_edited (GtkCellRendererText *cell, gchar *path_string, gchar *costoT, gpointer data)
{
  GtkTreeModel *model = GTK_TREE_MODEL (data);
  GtkTreePath *path = gtk_tree_path_new_from_string (path_string);
  GtkTreeIter iter;
  gdouble costo = strtod (PUT (costoT), (char **)NULL);
  gdouble costoAnterior;
  gdouble subTotal, precio, cantidad;
  gboolean fraccion, enable;
  gchar *barcode;

  // Se inicializan en 0
  subTotal = precio = cantidad = 0;

  gtk_tree_model_get_iter (model, &iter, path);
  gtk_tree_path_free (path);

  gtk_tree_model_get (model, &iter,
		      0, &enable,
		      1, &barcode, // Se obtiene el codigo_corto desde el treeview
		      3, &costoAnterior, // Costo antes de la modificacion
		      4, &precio,
		      8, &cantidad,
		      -1);

  if (costo < 0) // costo no puede ser negativo
    costo = 0;

  gtk_list_store_set (GTK_LIST_STORE (model), &iter,
		      3, costo,
		      -1);

  // Recaclula subtotal
  if (enable == TRUE)
    {
      gchar *subTotalActualT = g_strdup (gtk_label_get_text (GTK_LABEL (gtk_builder_get_object (builder, "lbl_sum_selected"))));
      gdouble subTotalActual = strtod (PUT (subTotalActualT), (char **)NULL);

      // Se limpia subtotal restandole costo de la cantidad anterior
      // así se obtiene el subTotal sin contar este producto
      subTotal = subTotalActual - (costoAnterior * ((cantidad > 0) ? cantidad : 1));

      // Agrego al subtotal la cantidad al costo actual (la que se acaba de editar)
      subTotal = subTotal + (costo * ((cantidad > 0) ? cantidad : 1));

      gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "lbl_sum_selected")),
			    g_strdup_printf ("<b>%u</b>", llround(subTotal)));
    }
}

/**
 * Es llamada cuando se edita "Precio" en el tree_view_products_providers (signal edited).
 *
 * Esta funcion actualiza el campo "precio" por el valor indicado por el usuario.
 *
 */
void
on_sell_price_edited (GtkCellRendererText *cell, gchar *path_string, gchar *precioT, gpointer data)
{
  GtkTreeModel *model = GTK_TREE_MODEL (data);
  GtkTreePath *path = gtk_tree_path_new_from_string (path_string);
  GtkTreeIter iter;
  gdouble precio = strtod (PUT (precioT), (char **)NULL);
  gdouble precioAntes;
  gdouble subTotal, costo, cantidad;
  gboolean fraccion, enable;
  gchar *barcode;

  // Se inicializan en 0
  subTotal = costo = cantidad = 0;

  gtk_tree_model_get_iter (model, &iter, path);
  gtk_tree_path_free (path);

  gtk_tree_model_get (model, &iter,
		      0, &enable,
		      1, &barcode, // Se obtiene el codigo_corto desde el treeview
		      3, &costo,
		      4, &precioAntes, // precio antes de la modificacion
		      8, &cantidad,
		      -1);

  if (precio < 0) // precio no puede ser negativo
    precio = 0;

  gtk_list_store_set (GTK_LIST_STORE (model), &iter,
		      4, precio,
		      -1);
}


/**
 * Es llamada cuando se edita "sugerido" en el tree_view_products_providers (signal edited).
 *
 * Esta funcion actualiza el campo "sugerido" por el valor indicado por el usuario.
 *
 * TODO: Crear una función que actualice el valor modificado de cualquier treeview
 */

void
on_suggested_amount_edited (GtkCellRendererText *cell, gchar *path_string, gchar *sugeridoT, gpointer data)
{
  GtkTreeModel *model = GTK_TREE_MODEL (data);
  GtkTreePath *path = gtk_tree_path_new_from_string (path_string);
  GtkTreeIter iter;
  gdouble sugerido = strtod (PUT (sugeridoT), (char **)NULL);
  gdouble sugeridoAntes;
  gdouble subTotal = 0, precio = 0;
  gboolean fraccion, enable;
  gchar *barcode;

  gtk_tree_model_get_iter (model, &iter, path);
  gtk_tree_path_free (path);

  gtk_tree_model_get (model, &iter,
		      0, &enable,
		      1, &barcode, // Se obtiene el codigo_corto desde el treeview
		      3, &precio,
		      8, &sugeridoAntes, // Sugerido antes de la modificacion
		      -1);


  if (enable == FALSE)
    {
      AlertMSG (GTK_WIDGET (builder_get (builder, "tree_view_products_providers")), 
		"Debe seleccionar producto a editar");
      return;
    }
  else if (sugerido < 1)
    {
      AlertMSG (GTK_WIDGET (builder_get (builder, "tree_view_products_providers")), 
		"Debe ingresar una cantidad mayor a cero");
      return;
    }
  
  PGresult *res;
  // Se obtiene el barcode a partir del codigo_corto
  barcode = PQvaluebycol (EjecutarSQL
			  (g_strdup_printf ("SELECT barcode FROM codigo_corto_to_barcode('%s')", barcode)),
			  0, "barcode");

  gchar *q = g_strdup_printf ("SELECT fraccion FROM producto WHERE barcode = '%s'", barcode);
  res = EjecutarSQL (q);
  g_free (q);

  if (res == NULL)
    return;

  fraccion = (g_str_equal (PQvaluebycol (res, 0, "fraccion"), "t") == TRUE) ? TRUE : FALSE;

  if (sugerido < 0) // Sugerido no puede ser negativo
    sugerido = 0;

  gtk_list_store_set (GTK_LIST_STORE (model), &iter,
		      8, (fraccion == TRUE) ? sugerido : lround(sugerido),
		      -1);

  // Recaclula subtotal
  if (enable == TRUE)
    {
      gchar *subTotalActualT = g_strdup (gtk_label_get_text (GTK_LABEL (gtk_builder_get_object (builder, "lbl_sum_selected"))));
      gdouble subTotalActual = strtod (PUT (subTotalActualT), (char **)NULL);

      // Se limpia subtotal restandole costo de la cantidad anterior
      // así se obtiene el subTotal sin contar este producto
      subTotal = subTotalActual - (precio * ((sugeridoAntes > 0) ? sugeridoAntes : 1));

      // Agrego al subtotal la cantidad sugerida actual (la que se acaba de editar)
      subTotal = subTotal + (precio * ((sugerido > 0) ? sugerido : 1));

      gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "lbl_sum_selected")),
			    g_strdup_printf ("<b>%u</b>", llround(subTotal)));
    }

}


/**
 * Es llamada cuando el boton "btn_suggest_buy" es presionado (signal click).
 *
 * Esta funcion despliega una ventana, la cual permite realizar un proceso de compra
 * alternativo, basado en la selección de proveedores
 *
 */

void
on_btn_suggest_buy_clicked (GtkButton *button, gpointer user_data)
{
  GtkWidget *window;

  GtkListStore *store;
  GtkTreeView *treeview;
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;
  GtkTreeSelection *selection;

  Print *sugerido = (Print *) malloc (sizeof (Print));
  sugerido->son = (Print *) malloc (sizeof (Print));

  treeview = GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_providers"));
  if (gtk_tree_view_get_model (treeview) == NULL )
    {
      // TreeView Providers
      store = gtk_list_store_new (4,
				  G_TYPE_STRING, //Rut
				  G_TYPE_STRING, //Nombre
				  G_TYPE_DOUBLE, //Lapso Reposición
				  G_TYPE_STRING, //Descripción
				  -1);

      treeview = GTK_TREE_VIEW (builder_get (builder, "tree_view_providers"));
      gtk_tree_view_set_model (treeview, GTK_TREE_MODEL (store));
      selection = gtk_tree_view_get_selection (treeview);

      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes ("Rut", renderer,
							 "text", 0,
							 NULL);
      gtk_tree_view_append_column (treeview, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
      gtk_tree_view_column_set_sort_column_id (column, 0);
      gtk_tree_view_column_set_resizable (column, FALSE);

      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes ("Nombre", renderer,
							 "text", 1,
							 NULL);
      gtk_tree_view_append_column (treeview, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
      gtk_tree_view_column_set_sort_column_id (column, 1);
      gtk_tree_view_column_set_resizable (column, FALSE);

      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes ("Lap. rep.", renderer,
							 "text", 2,
							 NULL);
      gtk_tree_view_append_column (treeview, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
      gtk_tree_view_column_set_sort_column_id (column, 2);
      gtk_tree_view_column_set_resizable (column, FALSE);
      gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)2, NULL);

      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes ("Descripción", renderer,
							 "text", 3,
							 NULL);
      gtk_tree_view_append_column (treeview, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
      gtk_tree_view_column_set_sort_column_id (column, 3);
      gtk_tree_view_column_set_resizable (column, FALSE);

      sugerido->tree = treeview;
      sugerido->title = "Sugerido de compras";
      sugerido->name = "Sugerido";
      sugerido->date_string = NULL;
      sugerido->cols[0].name = "Rut";
      sugerido->cols[0].num = 0;
      sugerido->cols[1].name = "Nombre";
      sugerido->cols[1].num = 1;
      sugerido->cols[2].name = "Lapso Rep.";
      sugerido->cols[2].num = 2;
      sugerido->cols[3].name = "Giro";
      sugerido->cols[3].num = 3;
      sugerido->cols[4].name = NULL;

      // END TreeView Providers


      // TreeView Products Providers (merchandises of each providers)
      store = gtk_list_store_new (9,
				  G_TYPE_BOOLEAN, //Elegir (Para comprar)
				  G_TYPE_STRING,  //Código
				  G_TYPE_STRING,  //Descripción
				  G_TYPE_DOUBLE,  //Costo_promedio
				  G_TYPE_DOUBLE,  //Precio
				  G_TYPE_DOUBLE,  //Vta/Dia
				  G_TYPE_DOUBLE,  //Stock
				  G_TYPE_DOUBLE,  //Dias/Stock
				  G_TYPE_DOUBLE,  //Sugerido
				  -1);

      treeview = GTK_TREE_VIEW (builder_get (builder, "tree_view_products_providers"));
      gtk_tree_view_set_model (treeview, GTK_TREE_MODEL (store));
      selection = gtk_tree_view_get_selection (treeview);

      renderer = gtk_cell_renderer_toggle_new ();
      column = gtk_tree_view_column_new_with_attributes ("Elegir", renderer,
							 "active", 0,
							 NULL);
      gtk_tree_view_append_column (treeview, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
      //gtk_tree_view_column_set_sort_column_id (column, 0);
      gtk_tree_view_column_set_resizable (column, FALSE);

      g_signal_connect (G_OBJECT (renderer), "toggled",
			G_CALLBACK (ToggleProductSelection), NULL);

      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes ("Código", renderer,
							 "text", 1,
							 NULL);
      gtk_tree_view_append_column (treeview, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
      gtk_tree_view_column_set_sort_column_id (column, 1);
      gtk_tree_view_column_set_resizable (column, FALSE);

      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes ("Descripción Mercadería", renderer,
							 "text", 2,
							 NULL);
      gtk_tree_view_append_column (treeview, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
      gtk_tree_view_column_set_sort_column_id (column, 2);
      gtk_tree_view_column_set_min_width (column, 300);
      gtk_tree_view_column_set_max_width (column, 380);
      gtk_tree_view_column_set_resizable (column, TRUE);

      renderer = gtk_cell_renderer_text_new ();
      g_object_set (renderer, "editable", TRUE, NULL);
      g_signal_connect (G_OBJECT (renderer), "edited", G_CALLBACK (on_buy_price_edited), (gpointer)store);
      column = gtk_tree_view_column_new_with_attributes ("Costo", renderer,
							 "text", 3,
							 NULL);
      gtk_tree_view_append_column (treeview, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
      gtk_tree_view_column_set_sort_column_id (column, 3);
      gtk_tree_view_column_set_resizable (column, FALSE);
      gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)3, NULL);

      renderer = gtk_cell_renderer_text_new ();
      g_object_set (renderer, "editable", TRUE, NULL);
      g_signal_connect (G_OBJECT (renderer), "edited", G_CALLBACK (on_sell_price_edited), (gpointer)store);
      column = gtk_tree_view_column_new_with_attributes ("Precio", renderer,
							 "text", 4,
							 NULL);
      gtk_tree_view_append_column (treeview, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
      gtk_tree_view_column_set_sort_column_id (column, 4);
      gtk_tree_view_column_set_resizable (column, FALSE);
      gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)4, NULL);


      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes ("Vta/Dia", renderer,
							 "text", 5,
							 NULL);
      gtk_tree_view_append_column (treeview, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
      gtk_tree_view_column_set_sort_column_id (column, 5);
      gtk_tree_view_column_set_resizable (column, FALSE);
      gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)5, NULL);

      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes ("Stock", renderer,
							 "text", 6,
							 NULL);
      gtk_tree_view_append_column (treeview, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
      gtk_tree_view_column_set_sort_column_id (column, 6);
      gtk_tree_view_column_set_resizable (column, FALSE);
      gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)6, NULL);


      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes ("Dias/Stock", renderer,
							 "text", 7,
							 NULL);
      gtk_tree_view_append_column (treeview, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
      gtk_tree_view_column_set_sort_column_id (column, 7);
      gtk_tree_view_column_set_resizable (column, FALSE);
      gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)7, NULL);

      renderer = gtk_cell_renderer_text_new ();
      g_object_set (renderer, "editable", TRUE, NULL);
      g_signal_connect (G_OBJECT (renderer), "edited", G_CALLBACK (on_suggested_amount_edited), (gpointer)store);
      column = gtk_tree_view_column_new_with_attributes ("Sugerido", renderer,
							 "text", 8,
							 NULL);
      gtk_tree_view_append_column (treeview, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
      gtk_tree_view_column_set_sort_column_id (column, 8);
      gtk_tree_view_column_set_resizable (column, FALSE);
      gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)8, NULL);
      // END TreeView Products Providers      

      sugerido->son->tree = treeview;
      sugerido->son->cols[0].name = "Selection";
      sugerido->son->cols[0].num = 0;
      sugerido->son->cols[1].name = "Codigo Corto";
      sugerido->son->cols[1].num = 1;
      sugerido->son->cols[2].name = "Descripción";
      sugerido->son->cols[2].num = 2;
      sugerido->son->cols[3].name = "Costo Promedio";
      sugerido->son->cols[3].num = 3;
      sugerido->son->cols[4].name = "Precio";
      sugerido->son->cols[4].num = 4;
      sugerido->son->cols[5].name = "Venta/Dia";
      sugerido->son->cols[5].num = 5;
      sugerido->son->cols[6].name = "Stock";
      sugerido->son->cols[6].num = 6;
      sugerido->son->cols[7].name = "Dias/Stock";
      sugerido->son->cols[7].num = 7;
      sugerido->son->cols[8].name = "Cantidad";
      sugerido->son->cols[8].num = 8;
      sugerido->son->cols[9].name = NULL;

      g_signal_connect (builder_get (builder, "btn_print_suggest_buy"), "clicked",
			G_CALLBACK (SelectivePrintTwoTree), (gpointer)sugerido);

    }

  window = GTK_WIDGET (gtk_builder_get_object (builder, "wnd_suggest_buy"));
  clean_container (GTK_CONTAINER (window));
  
  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_buy_selected")), FALSE);
  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_print_suggest_buy")), FALSE);

  gtk_widget_show_all (GTK_WIDGET (window));

  // Rellenando TreeView
  GtkTreeIter iter;
  gint i, tuples;
  PGresult *res;

  gchar *q = "SELECT * FROM proveedor";
  res = EjecutarSQL (q);
  tuples = PQntuples (res);
  store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (builder_get (builder, "tree_view_providers"))));

  if (res == NULL) return;

  for (i = 0; i < tuples; i++)
    {
      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
			  0, PQvaluebycol (res, i, "rut"),
                          1, PQvaluebycol (res, i, "nombre"),
			  2, strtod (PUT (PQvaluebycol (res, i, "lapso_reposicion")), (char **)NULL),
			  3, PQvaluebycol (res, i, "giro"),
			  -1);
    }  
}


/**
 * Callback connected to providers treeview "activated signal"
 *
 * This function show on providers_products treeview
 * the products by selected provider.
 *
 * @param tree_view: the object on which the signal is emitted
 * @param path: the GtkTreePath for the activated row
 * @param column: the GtkTreeViewColumn in which the activation occurred
 * @param user_data: user data set when the signal handler was connected.
 */

void
AskProductProvider (GtkTreeView *tree_view, GtkTreePath *path_parameter,
		    GtkTreeViewColumn *column, gpointer user_data)
{
  GtkTreeView *treeview;
  GtkTreePath *path;
  GtkTreeModel *store;
  GtkTreeIter iter;
  gchar *rut;
  gdouble lapso_rep;

  treeview = tree_view;
  path = path_parameter;

  store = gtk_tree_view_get_model(GTK_TREE_VIEW(tree_view));

  gtk_tree_model_get_iter (store, &iter, path);
  gtk_tree_model_get (store, &iter,
                      0, &rut,
		      2, &lapso_rep,
                      -1);
  PGresult *res;
  gint i, tuples;

  res = getProductsByProvider (rut);

  // servirá para elegir el proveedor al confirmar la compra
  rut_proveedor_global = g_strdup_printf (rut);

  if (res == NULL) return;

  tuples = PQntuples (res);

  if (tuples <= 0)
    {
      ErrorMSG (GTK_WIDGET (builder_get (builder, "tree_view_providers")),
		"No existen mercaderías asociadas a este proveedor");
    }
  
  gdouble dias_stock, ventas_dia, sugerido;
  gboolean fraccion;

  //Se obtiene el treeview de productos del proveedor
  store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (builder_get (builder, "tree_view_products_providers"))));
  gtk_list_store_clear (store);
  gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "lbl_sum_selected")),
			g_strdup_printf ("<b>0</b>"));

  for (i = 0; i < tuples; i++)
    {
      sugerido = 0;
      dias_stock = strtod (PUT(PQvaluebycol (res, i, "stock_day")), (char **)NULL);
      ventas_dia = strtod (PUT(PQvaluebycol (res, i, "ventas_dia")), (char **)NULL);
      fraccion = (g_str_equal (PQvaluebycol (res, i, "fraccion"), "t")) ? TRUE : FALSE;

      if (dias_stock <= (lapso_rep + 1))
	sugerido = ((lapso_rep + 1) * ventas_dia);

      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
			  //0, Toggle Button (Elegir)
                          1, PQvaluebycol (res, i, "codigo_corto"),
			  2, g_strdup_printf ("%s %s %s %s", 
					      PQvaluebycol (res, i, "descripcion"),
					      PQvaluebycol (res, i, "marca"),
					      PQvaluebycol (res, i, "contenido"),
					      PQvaluebycol (res, i, "unidad")),
			  3, strtod (PUT(PQvaluebycol (res, i, "precio_compra")), (char **)NULL),
			  4, strtod (PUT(PQvaluebycol (res, i, "precio")), (char **)NULL),
			  5, ventas_dia,
                          6, strtod (PUT(PQvaluebycol (res, i, "stock")), (char **)NULL),
			  7, dias_stock,
			  8, (fraccion == TRUE) ? sugerido : lround (sugerido),
			  -1);
    }
}


/**
 * Callback connected to the entry_filter_providers when is edited
 * (changed signal)
 *
 * This function filters the content (on tree_view_providers) according 
 * to parameters obtained from the entry "entry_filter_providers".
 *
 * @param GtkEditable *editable
 * @param gpointer user_data
 */

void
on_entry_filter_providers_changed (GtkEditable *editable, gpointer user_data)
{
  GtkTreeView *treeview = GTK_TREE_VIEW (builder_get (builder, "tree_view_providers"));
  GtkTreeModel *model = gtk_tree_view_get_model (treeview);
  GtkTreeIter iter;
  GtkWidget *aux_widget;
  gboolean valid;
  gint fila_actual = 0; // Representa el numero de fila en el que se encuetra la iteración
  gint i; // Para la iteración del for

  gchar *rut, *nombre, *descripcion; // Datos que se obtienen del treeview
  gdouble lapRep; // Dato que se obtiene del treeview (lapso de reposición)
  gchar *patron, *textoFila, *textoFilaEliminada;

  /* Contienen las filas del treeview que son borradas */
  if (rutBorrado == NULL || nombreBorrado == NULL || 
      descripcionBorrada == NULL || lapRepBorrado == NULL )
    {
      rutBorrado = g_array_new (FALSE, FALSE, sizeof (gchar*));
      nombreBorrado = g_array_new (FALSE, FALSE, sizeof (gchar*));
      lapRepBorrado = g_array_new (FALSE, FALSE, sizeof (gdouble));
      descripcionBorrada = g_array_new (FALSE, FALSE, sizeof (gchar*));
    }

  /* Obtención del patron */
  aux_widget = GTK_WIDGET (gtk_builder_get_object (builder, "entry_filter_providers"));
  patron = g_strdup (gtk_entry_get_text (GTK_ENTRY (aux_widget))); //El texto obtenido será el patrón de filtrado

  valid = gtk_tree_model_get_iter_first (model, &iter);
  while (valid)
    {
      // Obtengo los valores del treeview --
      gtk_tree_model_get (model, &iter,
			  0, &rut,
                          1, &nombre,
			  2, &lapRep,
                          3, &descripcion,
                          -1);
      
      textoFila = g_strdup_printf ("%s %s %f %s", rut, nombre, lapRep, descripcion);
      //printf ("%s\n", textoFila);

      if (!g_regex_match_simple (patron, textoFila, G_REGEX_CASELESS, 0))
	{
	  /* Se almacenan los datos de las filas eliminadas */
	  g_array_append_val (rutBorrado, rut);
	  g_array_append_val (nombreBorrado, nombre);
	  g_array_append_val (lapRepBorrado, lapRep);
	  g_array_append_val (descripcionBorrada, descripcion);
	  
	  numFilasBorradas++;
	  
	  /* Se elimina la linea correspondiente y se "itera" si Ã©ste es la Ãºltima */
	  gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
	  if (fila_actual == get_treeview_length (treeview))
	    valid = gtk_tree_model_iter_next (model, &iter);
	}
      else
	{
	  valid = gtk_tree_model_iter_next (model, &iter);
	  fila_actual++;
	}
    }

  /* Para comprobar si el patron coincide con las filas eliminadas */  
  for (i = 0; i < numFilasBorradas; i++)
    {
      textoFilaEliminada = g_strdup_printf("%s %s %f %s",
					   g_array_index (rutBorrado, gchar*, i),
					   g_array_index (nombreBorrado, gchar*, i),
					   g_array_index (lapRepBorrado, gdouble, i),
					   g_array_index (descripcionBorrada, gchar*, i));
	      
      if (g_regex_match_simple (patron, textoFilaEliminada, G_REGEX_CASELESS, 0))
	{
	  /* Se reincorpora la fila borrada al treeview */
	  gtk_list_store_append (GTK_LIST_STORE (model), &iter);
	  gtk_list_store_set (GTK_LIST_STORE (model), &iter,
			      0, g_array_index (rutBorrado, gchar*, i),
			      1, g_array_index (nombreBorrado, gchar*, i),
			      2, g_array_index (lapRepBorrado, gdouble, i),
			      3, g_array_index (descripcionBorrada, gchar*, i),
			      -1);
	  
	  /* Se quita la fila reincorporada al treeview de los datos eliminados */
	  rutBorrado = g_array_remove_index (rutBorrado, i);
	  nombreBorrado = g_array_remove_index (nombreBorrado, i);
	  lapRepBorrado = g_array_remove_index (lapRepBorrado, i);
	  descripcionBorrada = g_array_remove_index (descripcionBorrada, i);
	  numFilasBorradas--;
	  i--;
	}
    }  
}


/**
 * Función temporal para facilitar la legibilidad de
 * "addSugestedBuy(void)"
 *
 * @param: void
 */

void
calcularPorcentajeGanancia (void)
{
  gchar *barcode = g_strdup (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_buy_barcode"))));
  gdouble ingresa = strtod (PUT (g_strdup (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_buy_price"))))), (char **)NULL);
  gdouble ganancia = (gdouble) atoi (g_strdup (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_buy_gain")))));
  gdouble precio_final = (gdouble) atoi (g_strdup (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_sell_price")))));
  gdouble precio;
  gdouble porcentaje;

  gdouble iva = GetIVA (barcode);
  gdouble otros = GetOtros (barcode);

  /*------ Se calcula el porcentaje de ganancia ------*/
  if (iva != -1)
    iva = (gdouble)iva / 100 + 1;
  else
    iva = -1;

  // TODO: Revisión (Concenso de la variable)
  if (otros == 0)
    otros = -1;

  /* -- Profit porcent ("ganancia") is calculated here -- */
  if (otros == -1 && iva != -1 )
    porcentaje = (gdouble) ((precio_final / (gdouble)(iva * ingresa)) -1) * 100;

  else if (iva != -1 && otros != -1 )
    {
      iva = (gdouble) iva - 1;
      otros = (gdouble) otros / 100;

      precio = (gdouble) precio_final / (gdouble)(iva + otros + 1);
      ganancia = (gdouble) precio - ingresa;
      porcentaje = (gdouble)(ganancia / ingresa) * 100;
    }
  else if (iva == -1 && otros == -1 )
    porcentaje = (gdouble) ((precio_final / ingresa) - 1) * 100;

  gtk_entry_set_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_buy_gain")),
		      g_strdup_printf ("%ld", lround (porcentaje)));
}


/**
 * Is called by "btn_buy_selected" (signal click).
 *
 * this function add the selected products to
 * "tree_view_products_buy_list"
 * 
 * @param: GtkButton *button: button from signal is triggered
 * @param: gpointer user_data: user data
 */

void
addSugestedBuy (GtkButton *button, gpointer user_data)
{
  /*Variables del treeview*/
  GtkTreeView *treeview = GTK_TREE_VIEW (builder_get (builder, "tree_view_products_providers"));
  GtkTreeModel *model = gtk_tree_view_get_model (treeview);
  GtkListStore *store;
  GtkTreeIter iter;
  gboolean valid;

  /*Variables de informacion*/
  gboolean enabled;
  gchar *codigo_corto; //Se obtienen desde el treeview
  gdouble costo, cantidad; //Se obtienen desde el treeview
  gchar *barcode, *precio; //Se obtienen a través de una consulta sql

  /*Variables consulta*/
  PGresult *res;
  gchar *q = NULL;

  store = GTK_LIST_STORE (gtk_tree_view_get_model (treeview));
  valid = gtk_tree_model_get_iter_first (model, &iter);
  while (valid)
    {
      // Obtengo los valores del treeview --
      gtk_tree_model_get (model, &iter,
			  0, &enabled,
			  1, &codigo_corto,
			  3, &costo,
			  8, &cantidad,
                          -1);

      if (enabled == TRUE)
	{
	  q = g_strdup_printf ("SELECT barcode, precio FROM producto where codigo_corto = '%s'", codigo_corto);
	  res = EjecutarSQL (q);
	  barcode = PQvaluebycol (res, 0, "barcode");
	  precio = PQvaluebycol (res, 0, "precio");

	  if (cantidad == 0)
	    cantidad = 1;

	  //Set Buy's Entries
	  gtk_entry_set_text (GTK_ENTRY (builder_get (builder, "entry_buy_barcode")), barcode);
	  gtk_entry_set_text (GTK_ENTRY (builder_get (builder, "entry_buy_price")), g_strdup_printf ("%f",costo));
	  gtk_entry_set_text (GTK_ENTRY (builder_get (builder, "entry_sell_price")), precio);
	  calcularPorcentajeGanancia (); // Calcula el porcentaje de ganancia y lo agraga al entry
	  gtk_entry_set_text (GTK_ENTRY (builder_get (builder, "entry_buy_amount")), g_strdup_printf ("%f",cantidad));

	  AddToProductsList(); // Agrega los productos a la lista de compra
	}
      valid = gtk_tree_model_iter_next (model, &iter);
    }    
  
  // Si se selecciona un producto
  if (q != NULL)
    {
      gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_suggest_buy")), FALSE);
      gtk_widget_hide (GTK_WIDGET (gtk_builder_get_object (builder, "wnd_suggest_buy")));
    }
  else
    {
      ErrorMSG (GTK_WIDGET (builder_get (builder, "tree_view_providers")),"Debe elegir a lo menos un producto a comprar");
    }

  /* Se librea la consulta */
  g_free (q);

  /* Se libera la memeria del GArray*/
  if (rutBorrado != NULL || nombreBorrado != NULL || lapRepBorrado != NULL || 
      descripcionBorrada != NULL)
    {
      g_array_free (rutBorrado, TRUE); rutBorrado = NULL;
      g_array_free (nombreBorrado, TRUE); nombreBorrado = NULL;
      g_array_free (lapRepBorrado, TRUE); lapRepBorrado = NULL;
      g_array_free (descripcionBorrada, TRUE); descripcionBorrada = NULL;
    }
}


/**
 * Is called by "btn_close_suggest_buy" (signal click).
 *
 * this function hide the "wnd_suggest_buy" windows and
 * free GArray struct of memory.
 * 
 * @param: GtkButton *button: button from signal is triggered
 * @param: gpointer user_data: user data
 */

void 
hide_wnd_suggest_buy (GtkButton *button, gpointer user_data)
{
  gtk_widget_hide (GTK_WIDGET (gtk_builder_get_object (builder, "wnd_suggest_buy")));

  if (rutBorrado != NULL || nombreBorrado != NULL || lapRepBorrado != NULL || 
      descripcionBorrada != NULL)
    {
      g_array_free (rutBorrado, TRUE); rutBorrado = NULL;
      g_array_free (nombreBorrado, TRUE); nombreBorrado = NULL;
      g_array_free (lapRepBorrado, TRUE); lapRepBorrado = NULL;
      g_array_free (descripcionBorrada, TRUE); descripcionBorrada = NULL;
    }

  //Se libera la variable global
  if (rut_proveedor_global != NULL)
    {
      g_free (rut_proveedor_global);
      rut_proveedor_global = NULL;
    }
}


/**
 * Callback when "change" event on 'treeview_nullify_buy_invoice' is triggered.
 *
 * This function shows (on the 'treeview_nullify_buy_invoice_details') 
 * the information correspondig to the row selected
 * on the 'treeview_nullify_buy_invoice'. 
 */
void
on_selection_nullify_buy_invoice_change (GtkTreeSelection *selection, gpointer data)
{
  GtkTreeView *treeview = GTK_TREE_VIEW (builder_get (builder, "treeview_nullify_buy_invoice_details"));
  GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model (treeview));
  gtk_list_store_clear (store);

  GtkTreeModel *model = gtk_tree_view_get_model (gtk_tree_selection_get_tree_view (selection));
  GtkTreeIter iter;

  gint i, tuples;
  gchar *q;
  PGresult *res;

  gint id_compra, id_factura_compra;

  if (gtk_tree_selection_get_selected (selection, NULL, &iter) == TRUE)
    {
      gtk_tree_model_get (model, &iter,
			  5, &id_compra,
			  6, &id_factura_compra,
			  -1);

      q = g_strdup_printf ("SELECT fcd.barcode, fcd.cantidad, fcd.precio, (fcd.cantidad * fcd.precio) AS subtotal, "
			   "(SELECT p.descripcion FROM producto p WHERE p.barcode = fcd.barcode) AS descripcion "
			   "FROM factura_compra_detalle fcd "
			   "INNER JOIN factura_compra fc "
			   "ON fc.id = fcd.id_factura_compra "
			   "WHERE fc.id = %d", id_factura_compra);
      
      res = EjecutarSQL (q);
      g_free (q);
      tuples = PQntuples (res);
      
      for (i = 0; i < tuples; i++)
	{
	  gtk_list_store_append (store, &iter);
	  gtk_list_store_set (store, &iter,
			      0, g_strdup (PQvaluebycol (res, i, "barcode")),
			      1, PQvaluebycol (res, i, "descripcion"),
			      2, strtod (PUT(g_strdup (PQvaluebycol (res, i, "cantidad"))), (char **)NULL),
			      3, PUT (g_strdup (PQvaluebycol (res, i, "precio"))),
			      4, PUT (g_strdup (PQvaluebycol (res, i, "subtotal"))),
			      5, id_compra,
			      6, id_factura_compra,
			      -1);
	}
      
      PQclear (res);
    }
}

/**
 * Callback when "change" event on 'treeview_nullify_buy' is triggered.
 *
 * This function shows (on the 'treeview_nullify_buy_invoice') 
 * the information correspondig to the row selected
 * on the 'treeview_nullify_buy'. 
 */
void 
on_selection_nullify_buy_change (GtkTreeSelection *selection, gpointer data)
{  
  GtkTreeView *treeview = GTK_TREE_VIEW (builder_get (builder, "treeview_nullify_buy_invoice"));
  GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model (treeview));
  gtk_list_store_clear (store);

  GtkTreeModel *model = gtk_tree_view_get_model (gtk_tree_selection_get_tree_view (selection));
  GtkTreeIter iter;

  gint i, tuples;
  gchar *q;
  PGresult *res;

  gint id_compra;
  gchar *nombre_proveedor, *rut_proveedor;

  if (gtk_tree_selection_get_selected (selection, NULL, &iter) == TRUE)
    {
      gtk_tree_model_get (model, &iter,
			  0, &id_compra,
			  3, &nombre_proveedor,
			  4, &rut_proveedor,
			  -1);

      gtk_label_set_text (GTK_LABEL (builder_get (builder, "lbl_nullify_buy_provider_name")), nombre_proveedor);
      gtk_label_set_text (GTK_LABEL (builder_get (builder, "lbl_nullify_buy_provider_rut")), rut_proveedor);

      q = g_strdup_printf ("SELECT fc.id, fc.num_factura, fc.pagada, fc.id_compra, "
			   "date_part ('year', fc.fecha) AS f_year, "
			   "date_part ('month', fc.fecha) AS f_month, "
			   "date_part ('day', fc.fecha) AS f_day, "

			   "date_part ('year', fc.fecha_pago) AS fp_year, "
			   "date_part ('month', fc.fecha_pago) AS fp_month, "
			   "date_part ('day', fc.fecha_pago) AS fp_day, "

			   "(SELECT SUM (cantidad * precio) "
		                   "FROM factura_compra_detalle fcd "
		                   "WHERE fcd.id_factura_compra = fc.id) AS monto "

			   "FROM factura_compra fc "
			   "WHERE fc.id_compra = %d", id_compra);
      
      res = EjecutarSQL (q);
      g_free (q);
      tuples = PQntuples (res);
      
      for (i = 0; i < tuples; i++)
	{
	  gtk_list_store_append (store, &iter);
	  gtk_list_store_set (store, &iter,
			      0, atoi (g_strdup (PQvaluebycol (res, i, "num_factura"))),
			      1, g_strdup_printf ("%s/%s/%s",
						  PQvaluebycol (res, i, "f_day"),
						  PQvaluebycol (res, i, "f_month"),
						  PQvaluebycol (res, i, "f_year")),
			      2, g_strdup_printf ("%s", (g_str_equal (PQvaluebycol (res, i, "pagada"), "t")) ? "Si" : "No"),
			      3, g_strdup_printf ("%s/%s/%s",
						  PQvaluebycol (res, i, "fp_day"),
						  PQvaluebycol (res, i, "fp_month"),
						  PQvaluebycol (res, i, "fp_year")),
			      4, PUT (g_strdup (PQvaluebycol (res, i, "monto"))),
			      5, id_compra,
			      6, atoi (g_strdup (PQvaluebycol (res, i, "id"))),
			      -1);
	}
      
      PQclear (res);
    }
}

/**
 * Is called by "btn_nullify_buy_ok" (signal click).
 *
 * this function nullify the buy selected
 * 
 * @param: GtkButton *button: button from signal is triggered
 * @param: gpointer user_data: user data
 */
void
on_btn_nullify_buy_ok_clicked (GtkButton *button, gpointer user_data)
{
  GtkTreeView *treeview = GTK_TREE_VIEW (builder_get (builder, "treeview_nullify_buy"));
  GtkTreeSelection *selection  = gtk_tree_view_get_selection (treeview);
  GtkTreeModel *model = gtk_tree_view_get_model (gtk_tree_selection_get_tree_view (selection));
  GtkTreeIter iter;  

  gint id_compra;

  gchar *q;
  PGresult *res;
  gint tuples, i;


  if (gtk_tree_selection_get_selected (selection, NULL, &iter) == FALSE)
    {
      AlertMSG (GTK_WIDGET (builder_get (builder, "treeview_nullify_buy")), 
		"Debe seleccionar la compra que desea anular");
      return;
    }

  gtk_tree_model_get (model, &iter,
		      0, &id_compra,
		      -1);

  q = g_strdup_printf ("SELECT * FROM nullify_buy (%d, %d)", id_compra, atoi (rizoma_get_value ("MAQUINA")));
  res = EjecutarSQL (q);
  g_free (q);
  tuples = PQntuples (res);
  
  for (i = 0; i < tuples; i++)
    {
      gtk_entry_set_text (GTK_ENTRY (builder_get (builder, "entry_buy_barcode")), PQvaluebycol (res, i, "barcode"));
      gtk_entry_set_text (GTK_ENTRY (builder_get (builder, "entry_buy_price")), PQvaluebycol (res, i, "costo"));
      gtk_entry_set_text (GTK_ENTRY (builder_get (builder, "entry_sell_price")), PQvaluebycol (res, i, "precio"));
      calcularPorcentajeGanancia (); // Calcula el porcentaje de ganancia y lo agraga al entry
      
      gtk_entry_set_text (GTK_ENTRY (builder_get (builder, "entry_buy_amount")), PQvaluebycol (res, i, "cantidad_anulada"));
      
      AddToProductsList(); // Agrega los productos a la lista de compra
    }

  gtk_widget_hide (GTK_WIDGET (builder_get (builder, "wnd_nullify_buy")));
}

/**
 * This function loads the nullify buy dialog.
 *
 * Here must stay all the configuration of the dialog that is needed
 * when the dialog will be showed to the user.
 */
void
nullify_buy_win(void)
{
  GtkWidget *widget;

  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  GtkTreeView *treeview_buy;
  GtkTreeSelection *selection_buy;
  GtkTreeView *treeview_invoice;
  GtkTreeSelection *selection_invoice;
  GtkTreeView *treeview_details;

  GtkListStore *store_buy;
  GtkListStore *store_invoice;
  GtkListStore *store_details;

  // TreeView Compras
  treeview_buy = GTK_TREE_VIEW(gtk_builder_get_object(builder, "treeview_nullify_buy"));
  store_buy = GTK_LIST_STORE(gtk_tree_view_get_model(treeview_buy));
  if (store_buy == NULL)
    {
      store_buy = gtk_list_store_new (5,
				      G_TYPE_INT,     //Id
				      G_TYPE_STRING,  //Date
				      G_TYPE_STRING,  //Total amount
				      G_TYPE_STRING,  //Nombre proveedor
				      G_TYPE_STRING); //Rut proveedor

      gtk_tree_view_set_model(treeview_buy, GTK_TREE_MODEL(store_buy));

      selection_buy = gtk_tree_view_get_selection (treeview_buy);
      gtk_tree_selection_set_mode (selection_buy, GTK_SELECTION_SINGLE);
      g_signal_connect (G_OBJECT (selection_buy), "changed",
			G_CALLBACK (on_selection_nullify_buy_change), NULL);

      //ID
      renderer = gtk_cell_renderer_text_new();
      column = gtk_tree_view_column_new_with_attributes("ID", renderer,
                                                        "text", 0,
                                                        NULL);
      gtk_tree_view_append_column (treeview_buy, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
      gtk_tree_view_column_set_sort_column_id (column, 0);
      gtk_tree_view_column_set_resizable (column, FALSE);

      //date
      renderer = gtk_cell_renderer_text_new();
      column = gtk_tree_view_column_new_with_attributes("Fecha", renderer,
                                                        "text", 1,
                                                        NULL);
      gtk_tree_view_append_column (treeview_buy, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
      gtk_tree_view_column_set_sort_column_id (column, 1);
      gtk_tree_view_column_set_resizable (column, FALSE);
      
      //Total amount
      renderer = gtk_cell_renderer_text_new();
      column = gtk_tree_view_column_new_with_attributes("Monto Total", renderer,
                                                        "text", 2,
                                                        NULL);
      gtk_tree_view_append_column (treeview_buy, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
      gtk_tree_view_column_set_sort_column_id (column, 2);
      gtk_tree_view_column_set_resizable (column, FALSE);
    }

  // TreeView Facturas
  treeview_invoice = GTK_TREE_VIEW(gtk_builder_get_object(builder, "treeview_nullify_buy_invoice"));
  store_invoice = GTK_LIST_STORE(gtk_tree_view_get_model(treeview_invoice));
  if (store_invoice == NULL)
    {
      store_invoice = gtk_list_store_new (7,
					  G_TYPE_INT,    // N° Factura
					  G_TYPE_STRING, // Fecha Ingreso
					  G_TYPE_STRING, // Pagada
					  G_TYPE_STRING, // Fecha Pago
					  G_TYPE_STRING, // Monto Total
					  G_TYPE_INT,    // ID Compra
					  G_TYPE_INT);   // ID Factura Compra

      gtk_tree_view_set_model(treeview_invoice, GTK_TREE_MODEL(store_invoice));

      selection_invoice = gtk_tree_view_get_selection (treeview_invoice);
      gtk_tree_selection_set_mode (selection_invoice, GTK_SELECTION_SINGLE);
      g_signal_connect (G_OBJECT(selection_invoice), "changed",
      G_CALLBACK (on_selection_nullify_buy_invoice_change), NULL);

      //ID
      renderer = gtk_cell_renderer_text_new();
      column = gtk_tree_view_column_new_with_attributes("N° Factura", renderer,
                                                        "text", 0,
                                                        NULL);
      gtk_tree_view_append_column (treeview_invoice, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
      gtk_tree_view_column_set_sort_column_id (column, 0);
      gtk_tree_view_column_set_resizable (column, FALSE);

      //Fecha Ingreso
      renderer = gtk_cell_renderer_text_new();
      column = gtk_tree_view_column_new_with_attributes("Fecha Ing.", renderer,
                                                        "text", 1,
                                                        NULL);
      gtk_tree_view_append_column (treeview_invoice, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
      gtk_tree_view_column_set_sort_column_id (column, 1);
      gtk_tree_view_column_set_resizable (column, FALSE);

      //Pagado
      renderer = gtk_cell_renderer_text_new();
      column = gtk_tree_view_column_new_with_attributes("Pagada", renderer,
                                                        "text", 2,
                                                        NULL);
      gtk_tree_view_append_column (treeview_invoice, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
      gtk_tree_view_column_set_sort_column_id (column, 2);
      gtk_tree_view_column_set_resizable (column, FALSE);

      //Fecha Pago
      renderer = gtk_cell_renderer_text_new();
      column = gtk_tree_view_column_new_with_attributes("Fecha Pago", renderer,
                                                        "text", 3,
                                                        NULL);
      gtk_tree_view_append_column (treeview_invoice, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
      gtk_tree_view_column_set_sort_column_id (column, 3);
      gtk_tree_view_column_set_resizable (column, FALSE);

      //Monto Total
      renderer = gtk_cell_renderer_text_new();
      column = gtk_tree_view_column_new_with_attributes("Monto Total", renderer,
                                                        "text", 4,
                                                        NULL);
      gtk_tree_view_append_column (treeview_invoice, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
      gtk_tree_view_column_set_sort_column_id (column, 4);
      gtk_tree_view_column_set_resizable (column, FALSE);
    }

  //TreeView Detalle Factura
  treeview_details = GTK_TREE_VIEW(gtk_builder_get_object(builder, "treeview_nullify_buy_invoice_details"));
  store_details = GTK_LIST_STORE(gtk_tree_view_get_model(treeview_details));
  if (store_details == NULL)
    {
      store_details = gtk_list_store_new (7,
                                          G_TYPE_STRING, //barcode
                                          G_TYPE_STRING, //description
                                          G_TYPE_DOUBLE, //cantity
                                          G_TYPE_STRING, //price
                                          G_TYPE_STRING, //subtotal
                                          G_TYPE_INT,    //Id_compra
                                          G_TYPE_INT);   //id_factura_compra

      gtk_tree_view_set_model(treeview_details, GTK_TREE_MODEL(store_details));
      gtk_tree_selection_set_mode (gtk_tree_view_get_selection (treeview_details), GTK_SELECTION_NONE);

      //barcode
      renderer = gtk_cell_renderer_text_new();
      column = gtk_tree_view_column_new_with_attributes("Cod. Barras", renderer,
                                                        "text", 0,
                                                        NULL);
      gtk_tree_view_append_column (treeview_details, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
      gtk_tree_view_column_set_sort_column_id (column, 0);
      gtk_tree_view_column_set_resizable (column, FALSE);

      //description
      renderer = gtk_cell_renderer_text_new();
      column = gtk_tree_view_column_new_with_attributes("Descripcion", renderer,
                                                        "text", 1,
                                                        NULL);
      gtk_tree_view_append_column (treeview_details, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
      gtk_tree_view_column_set_sort_column_id (column, 1);
      gtk_tree_view_column_set_resizable (column, FALSE);

      //cantity
      renderer = gtk_cell_renderer_text_new();
      column = gtk_tree_view_column_new_with_attributes("Cantidad", renderer,
                                                        "text", 2,
                                                        NULL);
      gtk_tree_view_append_column (treeview_details, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
      gtk_tree_view_column_set_sort_column_id (column, 2);
      gtk_tree_view_column_set_resizable (column, FALSE);
      gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)2, NULL);

      //price
      renderer = gtk_cell_renderer_text_new();
      column = gtk_tree_view_column_new_with_attributes("Precio", renderer,
                                                        "text", 3,
                                                        NULL);
      gtk_tree_view_append_column (treeview_details, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
      gtk_tree_view_column_set_sort_column_id (column, 3);
      gtk_tree_view_column_set_resizable (column, FALSE);

      //subtotal
      renderer = gtk_cell_renderer_text_new();
      column = gtk_tree_view_column_new_with_attributes("Subtotal", renderer,
                                                        "text", 4,
                                                        NULL);
      gtk_tree_view_append_column (treeview_details, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
      gtk_tree_view_column_set_sort_column_id (column, 4);
      gtk_tree_view_column_set_resizable (column, FALSE);
    }

  widget = GTK_WIDGET (gtk_builder_get_object(builder, "wnd_nullify_buy"));
  clean_container (GTK_CONTAINER (widget));
  gtk_widget_show_all (widget);
}


/**
 * Is called by "btn_nullify_buy_search" (signal click).
 *
 * this function shows the products that meet the search criteria
 * on treeview "treeview_nullify_buy"
 * 
 * @param: GtkButton *button: button from signal is triggered
 * @param: gpointer user_data: user data
 */
void
on_btn_nullify_buy_search_clicked (GtkButton *button, gpointer user_data)
{
  /*Treeview*/
  GtkTreeView *treeview;
  GtkListStore *store;
  GtkTreeIter iter;

  /*Limpiando Treeview*/
  treeview = GTK_TREE_VIEW (gtk_builder_get_object (builder, "treeview_nullify_buy"));
  store = GTK_LIST_STORE (gtk_tree_view_get_model (treeview));
  gtk_list_store_clear (store);

  /*Limpiando Labels*/
  gtk_label_set_text (GTK_LABEL (builder_get (builder, "lbl_nullify_buy_provider_name")), "");
  gtk_label_set_text (GTK_LABEL (builder_get (builder, "lbl_nullify_buy_provider_rut")), "");

  /*Fecha*/
  GDate *date_aux;
  date_aux = g_date_new();

  /*Obteniendo datos de los entry*/
  gchar *id_compra = g_strdup (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_nullify_buy_id"))));
  gchar *num_factura = g_strdup (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_nullify_buy_invoice"))));
  gchar *proveedor = g_strdup (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_nullify_buy_provider"))));
  gchar *str_date = g_strdup (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_nullify_buy_date"))));
  
  if (!g_str_equal (str_date, ""))
    {
      g_date_set_parse (date_aux, str_date);
      str_date = g_strdup_printf("%d-%d-%d",
				 g_date_get_year(date_aux),
				 g_date_get_month(date_aux),
				 g_date_get_day(date_aux));
    }
  
  gchar *q, *q2;
  PGresult *res;
  gint tuples, i;

  q = g_strdup_printf ("SELECT c.id, "
		       "date_part ('year', c.fecha) AS year, "
		       "date_part ('month', c.fecha) AS month, "
		       "date_part ('day', c.fecha) AS day, "
		       "date_part ('hour', c.fecha) AS hour, "
		       "date_part ('minute', c.fecha) AS minute, "
		       "c.rut_proveedor, "
		       "(SELECT nombre FROM proveedor WHERE rut = c.rut_proveedor) AS nombre_proveedor, "
		       "(SELECT SUM (cantidad * precio) "
		            "FROM factura_compra_detalle fcd "
		            "INNER JOIN factura_compra fc "
		            "ON fcd.id_factura_compra = fc.id "
		            "AND fc.id_compra = c.id) AS monto "
		       "FROM compra c INNER JOIN factura_compra fc "
		       "ON c.id = fc.id_compra "
		       "WHERE c.ingresada = 't' "
		       "AND c.anulada = 'f' "
		       "AND c.anulada_pi = 'f'");

  /*Comprobando datos*/
  if (!g_str_equal(id_compra, ""))
    q = g_strdup_printf (" %s AND c.id = %s", q, id_compra);

  if (!g_str_equal(num_factura, ""))
    q = g_strdup_printf (" %s AND fc.num_factura = %s", q, num_factura);

  if (!g_str_equal(proveedor, ""))
    {
      q2 = g_strdup_printf ("SELECT rut, dv, nombre "
			    "FROM buscar_proveedor ('%%%s%%')", proveedor);
      res = EjecutarSQL (q2);
      g_free (q2);
      tuples = PQntuples (res);

      if (tuples > 1)
	{
	  wnd_search_provider (GTK_ENTRY (builder_get (builder, "entry_nullify_buy_provider")), NULL);
	  return;
	}

      else if (tuples == 1)
	q = g_strdup_printf (" %s AND fc.rut_proveedor = %s", q, PQvaluebycol(res,0,"rut"));
      else if (tuples == 0)
	AlertMSG (GTK_WIDGET (builder_get (builder, "entry_nullify_buy_provider")), 
		  "No existen proveedores con el nombre o rut que ha especificado");
    }

  if (g_date_valid (date_aux))
    q = g_strdup_printf (" %s AND c.fecha BETWEEN '%s' AND '%s 23:59:59'", q, str_date, str_date);

  // Se ejecuta la consulta construida
  res = EjecutarSQL (q);
  g_free (q);  
  tuples = PQntuples (res);

  for (i = 0; i < tuples; i++)
    {
      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
			  0, atoi (g_strdup (PQvaluebycol (res, i, "id"))),
			  1, g_strdup_printf ("%s/%s/%s %s:%s",
					      PQvaluebycol (res, i, "day"),
					      PQvaluebycol (res, i, "month"),
					      PQvaluebycol (res, i, "year"),
					      PQvaluebycol (res, i, "hour"),
					      PQvaluebycol (res, i, "minute")),
			  2, PUT (g_strdup (PQvaluebycol (res, i, "monto"))),
			  3, PQvaluebycol (res, i, "nombre_proveedor"),
			  4, PQvaluebycol (res, i, "rut_proveedor"),
			  -1);
    }

  PQclear (res);
}


/**
 * This is a callback from button 'btn_nullify_buy_pi'
 * (signal clicked).
 *
 * Is called by 'f5' key too.
 *
 * Show the 'wnd_nullify_buy' window, to nullify purchases
 * that have already been entered.
 *
 * @param button the button
 * @param user_data the user data
 */
void 
on_btn_nullify_buy_pi_clicked (GtkButton *button, gpointer data)
{
  if (user_data->user_id == 1)
    nullify_buy_win ();
}
