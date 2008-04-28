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
#include"main.h"
#include"ventas.h"
#include"credito.h"
#include"administracion_productos.h"
#include"postgres-functions.h"
#include"manejo_productos.h"
#include"proveedores.h"
#include"errors.h"
#include"caja.h"
#include"dimentions.h"
#include"printing.h"
#include"utils.h"
#include"config_file.h"
#include"encriptar.h"
#include"rizoma_errors.h"

GtkBuilder *builder;

GtkWidget *see_button;
GtkWidget *add_button;
GtkWidget *clean_button;
GtkWidget *new_button;
GtkWidget *confirm_button;

GtkWidget *recv_button;

GtkWidget *entry_stock;

GtkWidget *ok_doc;

gboolean guias;

GtkWidget *add_guia;
GtkWidget *del_guia;
GtkWidget *ok_guia;

GtkWidget *ask_window;

gboolean ingreso_total = TRUE;
gboolean iva = TRUE;
gboolean perecible = TRUE;
gboolean fraccion = FALSE;

GtkWidget *combo_imp;
GtkWidget *combo_fami;

GtkWidget *button_elab;
GtkWidget *button_venc;

GtkWidget *calendar_win;

GtkWidget *pago_proveedor;
GtkWidget *pago_rut;
GtkWidget *pago_contacto;
GtkWidget *pago_direccion;
GtkWidget *pago_comuna;
GtkWidget *pago_fono;
GtkWidget *pago_email;
GtkWidget *pago_web;
GtkWidget *pago_factura;
GtkWidget *pago_emision;
GtkWidget *pago_monto;

GtkTreeStore *pagos_store;
GtkWidget *pagos_tree;

GtkWidget *pagos_calendar;

GtkWidget *label_found_compras;

GtkWidget *forma_pago_name;
GtkWidget *forma_pago_days;

GtkWidget *entry_plazo;

GtkWidget *label_canje;
GtkWidget *label_stock_pro;

GtkWidget *pago_plaza;
GtkWidget *pago_banco;
GtkWidget *pago_serie;
GtkWidget *pago_numero;
GtkWidget *pago_fecha;
GtkWidget *pago_monto;
GtkWidget *pago_otro;

GtkWidget *frame_cheque;
GtkWidget *frame_otro;

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

/*
 * This function will edit the cell
 * Can edit a cell of G_TYPE STRING or INT
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
        /* Our new number*/
        gint new_number;

        /* We get a string so we have to use atoi() */
        new_number = atoi (new_string);

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

void
Canjeables (GtkWidget *widget, gpointer data)
{
  gboolean dato = (gboolean)data;

  if (dato == FALSE)
    {
      gtk_widget_set_sensitive (main_window, TRUE);

      gtk_widget_destroy (gtk_widget_get_toplevel (widget));

      InsertarCompras ();

    }
  else if (dato == TRUE)
    {
      //gtk_widget_set_sensitive (main_window, TRUE);

      gtk_widget_destroy (gtk_widget_get_toplevel (widget));

      CalcularTotales ();

      DocumentoIngreso ();
    }
}

void
CanjeablesWindow (void)
{
  GtkWidget *window;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *scroll;
  GtkWidget *button;

  GtkTreeView *tree;
  GtkTreeStore *store;
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;
  GtkTreeIter iter;

  Productos *productos = compra->header;

  gtk_widget_set_sensitive (main_window, FALSE);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "Lista de productos a canjear");
  gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER_ALWAYS);
  gtk_window_present (GTK_WINDOW (window));
  gtk_window_set_resizable (GTK_WINDOW (window), FALSE);
  gtk_widget_show (window);

  vbox = gtk_vbox_new (FALSE, 3);
  gtk_container_add (GTK_CONTAINER (window), vbox);
  gtk_widget_show (vbox);

  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  gtk_widget_set_size_request (scroll, 640, 100);
  gtk_box_pack_start (GTK_BOX (vbox), scroll, FALSE, FALSE, 3);
  gtk_widget_show (scroll);

  store = gtk_tree_store_new (8,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_INT,
                              G_TYPE_STRING,
                              G_TYPE_INT,
                              G_TYPE_INT,
                              G_TYPE_BOOLEAN);

  do
    {
      /*      if (productos->product->tasa_canje > 1 && productos->product->stock_pro >= productos->product->tasa_canje)
              productos->product->cuanto += (productos->product->stock_pro / productos->product->tasa_canje);*/

      productos->product->cuanto = productos->product->stock_pro + (productos->product->stock_pro * ((double)1 / productos->product->tasa_canje));


      gtk_tree_store_append (store, &iter, NULL);
      gtk_tree_store_set (store, &iter,
                          0, productos->product->barcode,
                          1, productos->product->producto,
                          2, productos->product->marca,
                          3, productos->product->stock_pro,
                          4, g_strdup_printf ("%d:1", productos->product->tasa_canje),
                          5, (gint)(productos->product->stock_pro * ((gdouble)1 / productos->product->tasa_canje)),
                          6, productos->product->cuanto,
                          7, TRUE,
                          -1);

      productos->product->canjear = TRUE;
      productos->product->iter = iter;

      productos = productos->next;
    }
  while (productos != compra->header);


  tree = GTK_TREE_VIEW (gtk_tree_view_new_with_model (GTK_TREE_MODEL (store)));
  gtk_tree_view_set_rules_hint (tree, TRUE);
  gtk_container_add (GTK_CONTAINER (scroll), GTK_WIDGET (tree));
  gtk_widget_show (GTK_WIDGET (tree));

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Codigo de Barras", renderer,
                                                     "text", 0,
                                                     NULL);
  gtk_tree_view_append_column (tree, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Producto", renderer,
                                                     "text", 1,
                                                     NULL);
  gtk_tree_view_append_column (tree, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_min_width (column, 120);
  gtk_tree_view_column_set_max_width (column, 120);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Marca", renderer,
                                                     "text", 2,
                                                     NULL);
  gtk_tree_view_append_column (tree, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_min_width (column, 80);
  gtk_tree_view_column_set_max_width (column, 80);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Stock Pro", renderer,
                                                     "text", 3,
                                                     NULL);
  gtk_tree_view_append_column (tree, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_min_width (column, 50);
  gtk_tree_view_column_set_max_width (column, 50);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Tasa Cambio", renderer,
                                                     "text", 4,
                                                     NULL);
  gtk_tree_view_append_column (tree, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_min_width (column, 80);
  gtk_tree_view_column_set_max_width (column, 80);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Adic.", renderer,
                                                     "text", 5,
                                                     NULL);
  gtk_tree_view_append_column (tree, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  g_object_set_data (G_OBJECT (renderer), "column", (gint *)6);

  g_signal_connect (G_OBJECT (renderer), "edited",
                    G_CALLBACK (edit_cell), (gpointer)tree);

  column = gtk_tree_view_column_new_with_attributes ("Usar", renderer,
                                                     "text", 6,
                                                     "editable", 6,
                                                     NULL);
  gtk_tree_view_append_column (tree, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_min_width (column, 80);
  gtk_tree_view_column_set_max_width (column, 80);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_toggle_new ();
  g_object_set_data (G_OBJECT (renderer), "column", (gint *)7);

  g_signal_connect (G_OBJECT (renderer), "toggled",
                    G_CALLBACK (toggle_cell), (gpointer)tree);

  column = gtk_tree_view_column_new_with_attributes ("Canjear", renderer,
                                                     "active", 7,
                                                     NULL);
  gtk_tree_view_append_column (tree, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  button = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 3);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (Canjeables), (gpointer)FALSE);

  button = gtk_button_new_from_stock (GTK_STOCK_OK);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (Canjeables), (gpointer)TRUE);

}

void
CheckCanjeables (void)
{

  if (compra->header == NULL)
    return ;
  else
    {
      if (LookCanjeable (compra->header) == TRUE)
        CanjeablesWindow ();
      else
        DocumentoIngreso ();
      //CanjeablesWindow ();
    }
}

void
Pagar (GtkWidget *widget, gpointer data)
{
  GtkToggleButton *togglebutton = (GtkToggleButton *) data;
  GtkTreeIter iter;
  gchar *doc;
  gchar *descrip;
  gchar *monto;
  gchar *rut_proveedor = g_strdup (gtk_label_get_text (GTK_LABEL (pago_rut)));

  if (gtk_tree_selection_get_selected
      (gtk_tree_view_get_selection (GTK_TREE_VIEW (compra->tree_facturas)), NULL, &iter) == TRUE &&
      data != NULL)
    {
      /*      gtk_tree_model_get_iter_from_string
              (GTK_TREE_MODEL (compra->store_facturas), &iter,
              strtok (gtk_tree_model_get_string_from_iter (GTK_TREE_MODEL (compra->store_facturas),
              &iter), ":"));*/

      //�      gtk_tree_model_get_iter

      if (strcmp (rut_proveedor, "") != 0)
        gtk_tree_model_get (GTK_TREE_MODEL (compra->store_facturas), &iter,
                            2, &doc,
                            6, &monto,
                            -1);
      else
        gtk_tree_model_get (GTK_TREE_MODEL (compra->store_facturas), &iter,
                            1, &rut_proveedor,
                            2, &doc,
                            6, &monto,
                            -1);

      if (monto == NULL)
        return;

      monto = CutPoints (monto);

      doc = strtok (strchr (doc, ' '), "");

      /*if (cajita == TRUE)
        saldo_caja = ReturnSaldoCaja ();
      */

      if (gtk_toggle_button_get_active (togglebutton) == TRUE)
        {
          descrip = g_strdup_printf ("%s %s %s %s",
                                     gtk_entry_get_text (GTK_ENTRY (pago_banco)),
                                     gtk_entry_get_text (GTK_ENTRY (pago_serie)),
                                     gtk_entry_get_text (GTK_ENTRY (pago_fecha)),
                                     gtk_entry_get_text (GTK_ENTRY (pago_monto)));
        }
      else
        descrip = g_strdup_printf ("%s", gtk_entry_get_text (GTK_ENTRY (pago_otro)));

      if (PagarFactura (doc, rut_proveedor, descrip) == FALSE)
        ErrorMSG (pago_proveedor, "No se ingreso correctamente");
      else
        {
          ExitoMSG (pago_proveedor, "La factura se pago correctamente");

          gtk_label_set_text (GTK_LABEL (pago_emision), "");
          gtk_label_set_text (GTK_LABEL (pago_monto), "");
          gtk_label_set_text (GTK_LABEL (pago_factura), "");

          gtk_tree_store_clear (compra->store_facturas);
          gtk_tree_store_clear (pagos_store);

          ClearPagosData ();

          if (strcmp (rut_proveedor, "") != 0)
            FillPagarFacturas (rut_proveedor);
          else
            FillPagarFacturas (NULL);
        }

    }
  ClosePagarDocumentoWin (widget, NULL);
}

void
AskPagarCaja (void)
{
  GtkWidget *window;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *image;
  GtkWidget *label;
  GtkWidget *button;

  GtkTreeIter iter;
  gchar *doc, *monto;

  if (gtk_tree_selection_get_selected (gtk_tree_view_get_selection (GTK_TREE_VIEW (compra->tree_facturas)), NULL, &iter) != TRUE)
    return;
  else
    {
      gtk_tree_model_get (GTK_TREE_MODEL (compra->store_facturas), &iter,
                          0, &doc,
                          6, &monto,
                          -1);

      if (doc == NULL || monto == NULL)
        return;
    }

  gtk_widget_set_sensitive (main_window, FALSE);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER_ALWAYS);
  gtk_window_set_title (GTK_WINDOW (window), "Pagar con Caja");
  gtk_widget_show (window);

  vbox = gtk_vbox_new (FALSE, 3);
  gtk_widget_show (vbox);
  gtk_container_add (GTK_CONTAINER (window), vbox);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  image = gtk_image_new_from_stock (GTK_STOCK_DIALOG_QUESTION, GTK_ICON_SIZE_DIALOG);
  gtk_widget_show (image);
  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 3);

  label = gtk_label_new ("¿Desea cancelar la factura con dinero de caja?");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_end (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  button = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 3);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (Pagar), NULL);

  button = gtk_button_new_from_stock (GTK_STOCK_YES);
  gtk_widget_show (button);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);

  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (Pagar), (gpointer) TRUE);

  button = gtk_button_new_from_stock (GTK_STOCK_NO);
  gtk_widget_show (button);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);

  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (Pagar), (gpointer) FALSE);

  button = gtk_button_new_with_label ("Documento");
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (PagarDocumentoWin), NULL);
}

void
FillDetPagos (void)
{
  GtkTreeIter iter;
  PGresult *res;
  gchar *doc, *monto, *id;
  gchar *rut_proveedor;
  gchar *q;
  gint tuples, i;
  //  gchar *iter_string;

  if (gtk_tree_selection_get_selected
      (gtk_tree_view_get_selection (GTK_TREE_VIEW (compra->tree_facturas)), NULL, &iter) == TRUE)
    {
      gtk_tree_model_get (GTK_TREE_MODEL (compra->store_facturas), &iter,
                          0, &id,
                          1, &rut_proveedor,
                          2, &doc,
                          5, &monto,
                          -1);

      if (id == NULL)
        return;

      //hay que testear si funciona este split
      gchar **rut_split = g_strsplit (rut_proveedor, "-", 2);
      q = g_strdup_printf ("SELECT nombre, rut || '-' || dv, direccion, ciudad,"
                           "comuna, telefono, email, web, contacto, giro "
                           "FROM select_proveedor(%s)",
                           rut_split[0]);
      g_strfreev (rut_split); //libera el arreglo de strings
      res = EjecutarSQL (q);
      g_free (q); //libera el string

      gtk_entry_set_text (GTK_ENTRY (pago_proveedor), PQgetvalue (res, 0, 1));

      gtk_label_set_markup (GTK_LABEL (pago_rut),
                            g_strdup_printf ("<span weight=\"ultrabold\">%s</span>", rut_proveedor));

      gtk_label_set_markup (GTK_LABEL (pago_contacto),
                            g_strdup_printf ("<span weight=\"ultrabold\">%s</span>",
                                             PQgetvalue (res, 0, 9)));

      gtk_label_set_markup (GTK_LABEL (pago_direccion),
                            g_strdup_printf ("<span weight=\"ultrabold\">%s</span>",
                                             PQgetvalue (res, 0, 3)));

      gtk_label_set_markup (GTK_LABEL (pago_comuna),
                            g_strdup_printf ("<span weight=\"ultrabold\">%s</span>",
                                             PQgetvalue (res, 0, 5)));

      gtk_label_set_markup (GTK_LABEL (pago_fono),
                            g_strdup_printf ("<span weight=\"ultrabold\">%s</span>",
                                             PQgetvalue (res, 0, 6)));

      gtk_label_set_markup (GTK_LABEL (pago_email),
                            g_strdup_printf ("<span weight=\"ultrabold\">%s</span>",
                                             PQgetvalue (res, 0, 7)));

      gtk_label_set_markup (GTK_LABEL (pago_web),
                            g_strdup_printf ("<span weight=\"ultrabold\">%s</span>",
                                             PQgetvalue (res, 0, 8)));

      doc = strtok (strchr (doc, ' '), " ");

      gtk_tree_store_clear (pagos_store);

      if ((gtk_tree_model_iter_n_children (GTK_TREE_MODEL (compra->store_facturas), &iter)) == 0
          && monto == NULL)
        {
          if (gtk_tree_model_iter_has_child
              (GTK_TREE_MODEL (compra->store_facturas), &iter) == FALSE)
            { //TODO: es necesario revisar esta sentencia SQL y
              //simplificarla
              res = EjecutarSQL (g_strdup_printf
                                 ("SELECT t1.codigo, t1.descripcion, t1.marca, t1.contenido, t1.unidad, t2.cantidad, t2.precio, (t2.cantidad * t2.precio)::double precision AS total, t2.barcode, t2.id_compra, date_part('year', t2.fecha), date_part('month', t2.fecha), date_part('day', t2.fecha), (SELECT num_factura FROM factura_compra WHERE id=(SELECT id_factura FROM guias_compra WHERE numero=%s)) FROM producto AS t1, documentos_detalle AS t2 WHERE t2.id_compra=(SELECT id_compra FROM guias_compra WHERE numero=%s AND rut_proveedor='%s') AND t1.barcode=t2.barcode AND t2.numero=%s", doc, doc, rut_proveedor, doc));

              tuples = PQntuples (res);

              if (tuples == 0)
                return;

              for (i = 0; i < tuples; i++)
                {
                  gtk_tree_store_append (pagos_store, &iter, NULL);
                  gtk_tree_store_set (pagos_store, &iter,
                                      0, PQgetvalue (res, i, 0),
                                      1, g_strdup_printf
                                      ("%s %s %s %s", PQgetvalue (res, i, 1),
                                       PQgetvalue (res, i, 2), PQgetvalue (res, i, 3),
                                       PQgetvalue (res, i, 4)),
                                      2, PQgetvalue (res, i, 5),
                                      3, PQgetvalue (res, i, 6),
                                      4, PutPoints(g_strdup_printf ("%ld", lround (strtod (CUT(PQgetvalue (res, i, 7)), (char **)NULL)))),
                                      5, "Black",
                                      6, FALSE,
                                      -1);
                }

              gtk_label_set_markup (GTK_LABEL (pago_monto),
                                    g_strdup_printf ("<b>$ %s</b>", PQgetvalue (res, 0, 7)));

              gtk_label_set_markup (GTK_LABEL (pago_emision),
                                    g_strdup_printf
                                    ("<b>%.2d/%.2d/%.4d</b>", atoi (PQgetvalue (res, 0, 12)),
                                     atoi (PQgetvalue (res, 0, 11)), atoi (PQgetvalue (res, 0, 10))));

              gtk_label_set_markup (GTK_LABEL (pago_factura),
                                    g_strdup_printf ("<b>%s</b>", PQgetvalue (res, 0, 13)));
            }
          else
            {
              q = g_strdup_printf ("SELECT monto, date_part('day',fecha), "
                                   "date_part('month',fecha), "
                                   "date_part('year',fecha) FROM "
                                   "select_factura_compra_by_num_factura(%s)",
                                   doc);
              res = EjecutarSQL (q);
              g_free (q);
              tuples = PQntuples (res);

              if (tuples == 0)
                return;

              gtk_label_set_markup (GTK_LABEL (pago_monto),
                                    g_strdup_printf ("<b>$ %s</b>", PQgetvalue (res, 0, 0)));

              gtk_label_set_markup (GTK_LABEL (pago_emision),
                                    g_strdup_printf
                                    ("<b>%.2d/%.2d/%.4d</b>", atoi (PQgetvalue (res, 0, 1)),
                                     atoi (PQgetvalue (res, 0, 2)), atoi (PQgetvalue (res, 0, 3))));

              gtk_label_set_markup (GTK_LABEL (pago_factura),
                                    g_strdup_printf ("<b>%s</b>", doc));
            }
        }
      else
        {
          //TODO: hay que revisar esta sentencia y hacerla m�s sencilla
          res = EjecutarSQL (g_strdup_printf
                             ("SELECT t1.codigo, t1.descripcion, t1.marca, t1.contenido, t1.unidad, t2.cantidad, t2.precio, (t2.cantidad * t2.precio)::double precision AS total, t2.barcode, t2.id_compra, date_part('year', t2.fecha), date_part('month', t2.fecha), date_part('day', t2.fecha) FROM producto AS t1, documentos_detalle AS t2 WHERE t2.id_compra=(SELECT id_compra FROM factura_compra WHERE num_factura=%s AND rut_proveedor='%s' OR id=%s) AND t1.barcode=t2.barcode AND t2.numero=%s", doc, rut_proveedor, id, doc));

          tuples = PQntuples (res);

          if (tuples == 0)
            return;

          for (i = 0; i < tuples; i++)
            {
              gtk_tree_store_append (pagos_store, &iter, NULL);
              gtk_tree_store_set (pagos_store, &iter,
                                  0, PQgetvalue (res, i, 0),
                                  1, g_strdup_printf
                                  ("%s %s %s %s", PQgetvalue (res, i, 1),
                                   PQgetvalue (res, i, 2), PQgetvalue (res, i, 3),
                                   PQgetvalue (res, i, 4)),
                                  2, PQgetvalue (res, i, 5),
                                  3, PQgetvalue (res, i, 6),
                                  4, PutPoints(g_strdup_printf ("%ld", lround (strtod (CUT(PQgetvalue (res, i, 7)), (char **)NULL)))),
                                  5, "Black",
                                  6, FALSE,
                                  -1);
            }

          gtk_label_set_markup (GTK_LABEL (pago_monto),
                                g_strdup_printf ("<b>$ %s</b>", PQgetvalue (res, 0, 7)));

          gtk_label_set_markup (GTK_LABEL (pago_emision),
                                g_strdup_printf
                                ("<b>%.2d/%.2d/%.4d</b>", atoi (PQgetvalue (res, 0, 12)),
                                 atoi (PQgetvalue (res, 0, 11)), atoi (PQgetvalue (res, 0, 10))));

          gtk_label_set_markup (GTK_LABEL (pago_factura),
                                g_strdup_printf ("<b>%s</b>", doc));

        }

    }
}

void
ClearPagosData (void)
{
  gtk_entry_set_text (GTK_ENTRY (pago_proveedor), "");
  gtk_label_set_text (GTK_LABEL (pago_factura), "");

  gtk_label_set_text (GTK_LABEL (pago_rut), "");
  gtk_label_set_text (GTK_LABEL (pago_contacto), "");
  gtk_label_set_text (GTK_LABEL (pago_direccion), "");
  gtk_label_set_text (GTK_LABEL (pago_comuna), "");
  gtk_label_set_text (GTK_LABEL (pago_fono), "");
  gtk_label_set_text (GTK_LABEL (pago_email), "");
  gtk_label_set_text (GTK_LABEL (pago_web), "");
  gtk_label_set_text (GTK_LABEL (pago_emision), "");
  gtk_label_set_text (GTK_LABEL (pago_monto), "");

  gtk_tree_store_clear (compra->store_facturas);
  gtk_tree_store_clear (pagos_store);

  //  gtk_window_set_focus (GTK_WINDOW (main_window), pago_proveedor);
}

void
compras_win ()
{
  GtkWidget *compras_gui;
  gchar *fullscreen_opt = NULL;

  GtkListStore *store;
  GtkTreeView *treeview;

  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;

  GtkTreeSelection *selection;

  GError *error = NULL;

  compra = (Compra *) g_malloc (sizeof (Compra));
  compra->header = NULL;
  compra->products_list = NULL;
  compra->header_compra = NULL;
  compra->products_compra = NULL;
  compra->current = NULL;


  builder = gtk_builder_new ();

  gtk_builder_add_from_file (builder, DATADIR"/ui/rizoma-compras.ui", &error);
  gtk_builder_connect_signals (builder, NULL);


  if (error != NULL) {
    g_printerr ("%s\n", error->message);
  }

  compras_gui = GTK_WIDGET (gtk_builder_get_object (builder, "wnd_compras"));

  // check if the window must be set to fullscreen
  fullscreen_opt = rizoma_get_value ("FULLSCREEN");
  if ((fullscreen_opt != NULL) && (g_str_equal (fullscreen_opt, "YES")))
    gtk_window_fullscreen (GTK_WINDOW (compras_gui));




  /* History TreeView */
  store = gtk_list_store_new (5,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_DOUBLE,
                              G_TYPE_INT);

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

  //  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)3, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Precio", renderer,
                                                     "text", 4,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);


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

      do {
        compra->current = compra->products_compra->product;

        AddToTree ();

        compra->products_compra = compra->products_compra->next;
      } while (compra->products_compra != compra->header_compra);

      gtk_widget_set_sensitive (confirm_button, TRUE);

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

  //gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)2, NULL);

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

  g_signal_connect (G_OBJECT (treeview), "row-activated",
                    G_CALLBACK (AskIngreso), NULL);

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));

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

  g_signal_connect (G_OBJECT (treeview), "row-activated",
                    G_CALLBACK (IngresoParcial), NULL);

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

  gtk_widget_show_all (compras_gui);
}

void
SearchProductHistory (GtkEntry *entry, gchar *barcode)
{
  gdouble day_to_sell;

  PGresult *res;

  gchar *q = g_strdup_printf ("select barcode "
                              "from codigo_corto_to_barcode('%s')",
                              barcode);
  res = EjecutarSQL(q);

  if (PQntuples (res) == 1)
    {
      barcode = g_strdup (PQvaluebycol( res, 0, "barcode"));
      PQclear (res);
      gtk_entry_set_text (GTK_ENTRY (entry),barcode);
    }

  g_free(q);

  q = g_strdup_printf ("SELECT existe_producto(%s)", barcode);

  if (g_str_equal( GetDataByOne (q), "t"))
    {
      /* gtk_widget_set_sensitive (see_button, TRUE); */

      //ActiveBuy (NULL, NULL);

      /* gtk_widget_set_sensitive (new_button, FALSE); */

      /* gtk_widget_set_sensitive (ingreso_entry, TRUE); */
      /* gtk_widget_set_sensitive (ganancia_entry, TRUE); */
      /* gtk_widget_set_sensitive (precio_final_entry, TRUE); */
      /* gtk_widget_set_sensitive (compra->cantidad_entry, TRUE); */

      ShowProductHistory ();

      gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_stock")),
                            g_strdup_printf ("<span weight=\"ultrabold\">%.2f</span>",
                                             GetCurrentStock (barcode)));
      if ((day_to_sell = GetDayToSell (barcode)) != 0)
        {
          gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_stock_further")),
                                g_strdup_printf ("<span weight=\"ultrabold\">%.2f dias"
                                                 "</span>",
                                                 day_to_sell));
        }
      else
        {
          gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_stock_further")),
                                g_strdup_printf ("<span weight=\"ultrabold\">"
                                                 "indefinidos dias</span>"));
        }

      gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_sell_price")),
                            g_strdup_printf ("<span weight=\"ultrabold\">%s</span>",
                                             PutPoints (GetCurrentPrice (barcode))));
      g_free (q);
      q = g_strdup_printf ("SELECT descripcion FROM select_producto(%s)", barcode);
      gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_product_desc")),
                            g_strdup_printf ("<span weight=\"ultrabold\">%s</span>",
                                             GetDataByOne (q)));
      g_free(q);
      q = g_strdup_printf ("SELECT marca, costo_promedio, canje, stock_pro "
                           "FROM select_producto(%s)",
                           barcode);
      res = EjecutarSQL(q);
      if (g_str_equal (PQvaluebycol(res, 0, "canje"), "t"))
        {
          gtk_label_set_markup (GTK_LABEL (label_canje), "Stock Pro: ");
          gtk_label_set_markup (GTK_LABEL (label_stock_pro),
                                g_strdup_printf ("<span weight=\"ultrabold\">%.3f</span>",
                                                 strtod (PUT (PQvaluebycol(res, 0, "stock_pro")),
                                                         (char **)NULL)));
        }

      gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_mark")),
                            g_strdup_printf ("<span weight=\"ultrabold\">%s</span>",
                                             PQvaluebycol(res, 0, "marca")));

      gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_unit")),
                            g_strdup_printf ("<span weight=\"ultrabold\">%s</span>",
                                             GetUnit (barcode)));

      gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_fifo")),
                            g_strdup_printf ("<span weight=\"ultrabold\">%s</span>",
                                             PutPoints (PQvaluebycol(res, 0, "costo_promedio"))));

      gtk_widget_grab_focus (GTK_WIDGET (gtk_builder_get_object (builder, "entry_buy_price")));
    }
  else
    {
      /* gtk_widget_set_sensitive (see_button, FALSE); */
      /* gtk_widget_set_sensitive (add_button, FALSE); */

      /* gtk_widget_set_sensitive (new_button, TRUE); */
      /* gtk_window_set_focus (GTK_WINDOW (main_window), new_button); */

      /* gtk_widget_set_sensitive (ingreso_entry, FALSE); */
      /* gtk_widget_set_sensitive (ganancia_entry, FALSE); */
      /* gtk_widget_set_sensitive (precio_final_entry, FALSE); */
      /* gtk_widget_set_sensitive (compra->cantidad_entry, FALSE); */

      /*
        AlertMSG ("El codigo de barras ingresado no esta registrado.\n"
        "Si realmente quiere comprar este producto haga click en \"Aregar Producto\"");
      */
    }
}

void
CloseProductDescription (void)
{
  gtk_widget_destroy (compra->see_window);

  compra->see_window = NULL;
  gtk_widget_set_sensitive (main_window, TRUE);
}

void
ShowProductDescription (void)
{
  gchar *q;
  gchar *barcode;
  gchar *codigo;
  gchar *description;
  gchar *marca;
  gchar *unidad;
  gchar *contenido;
  gchar *precio;
  gchar *active_char;

  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *label;
  GtkWidget *button;

  gint active;

  PGresult *res;
  PGresult *res_aux;
  gint tuples, i;
  GSList *group;

  barcode = g_strdup (gtk_entry_get_text (GTK_ENTRY (compra->barcode_history_entry)));
  q = g_strdup_printf("SELECT codigo_corto, descripcion, marca, unidad, contenido, precio "
                      "FROM select_producto(%s)",
                      barcode);
  res_aux = EjecutarSQL(q);
  g_free(q);

  codigo = PQvaluebycol (res_aux, 0, "codigo_corto");
  description = PQvaluebycol (res_aux, 0, "descripcion");
  marca = PQvaluebycol (res_aux, 0, "marca");
  unidad = PQvaluebycol (res_aux, 0, "unidad");
  contenido = PQvaluebycol (res_aux, 0, "contenido");
  precio = PQvaluebycol (res_aux, 0, "precio");

  gtk_widget_set_sensitive (main_window, FALSE);

  compra->see_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_position (GTK_WINDOW (compra->see_window),
                           GTK_WIN_POS_CENTER_ALWAYS);
  gtk_window_set_title (GTK_WINDOW (compra->see_window), "Descripcion Producto");

  gtk_widget_show (compra->see_window);
  gtk_window_present (GTK_WINDOW (compra->see_window));
  //#  gtk_window_set_transient_for (GTK_WINDOW (compra->see_window), GTK_WINDOW (main_window));
  g_signal_connect (G_OBJECT (compra->see_window), "destroy",
                    G_CALLBACK (CloseProductDescription), NULL);

  vbox = gtk_vbox_new (FALSE, 3);
  gtk_widget_show (vbox);
  gtk_container_add (GTK_CONTAINER (compra->see_window), vbox);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_end (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  button = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
  gtk_widget_show (button);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 3);

  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (CloseProductDescription), NULL);

  compra->see_button = gtk_button_new_from_stock (GTK_STOCK_SAVE);
  gtk_widget_show (compra->see_button);
  gtk_box_pack_end (GTK_BOX (hbox), compra->see_button, FALSE, FALSE, 3);

  g_signal_connect (G_OBJECT (compra->see_button), "clicked",
                    G_CALLBACK (Save), NULL);

  gtk_widget_set_sensitive (compra->see_button, FALSE);

  label = gtk_label_new ("Detalle del Producto");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 3);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  label = gtk_label_new ("Codigo: ");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);

  compra->see_codigo = gtk_entry_new_with_max_length (10);
  gtk_entry_set_text (GTK_ENTRY (compra->see_codigo), codigo);
  gtk_widget_show (compra->see_codigo);
  gtk_box_pack_end (GTK_BOX (hbox), compra->see_codigo, FALSE, FALSE, 3);

  g_signal_connect (G_OBJECT (compra->see_codigo), "changed",
                    G_CALLBACK (ChangeSave), NULL);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  label = gtk_label_new ("Codigo de Barras: ");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);

  compra->see_barcode = gtk_entry_new_with_max_length (14);
  gtk_entry_set_text (GTK_ENTRY (compra->see_barcode), barcode);
  gtk_widget_show (compra->see_barcode);
  gtk_box_pack_end (GTK_BOX (hbox), compra->see_barcode, FALSE, FALSE, 3);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  label = gtk_label_new ("Descripcion: ");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);

  compra->see_description = gtk_entry_new_with_max_length (35);
  gtk_entry_set_text (GTK_ENTRY (compra->see_description), description);
  gtk_widget_show (compra->see_description);
  gtk_box_pack_end (GTK_BOX (hbox), compra->see_description, FALSE, FALSE, 3);

  g_signal_connect (G_OBJECT (compra->see_description), "changed",
                    G_CALLBACK (ChangeSave), NULL);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  label = gtk_label_new ("Marca: ");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);

  compra->see_marca = gtk_entry_new_with_max_length (35);
  gtk_entry_set_text (GTK_ENTRY (compra->see_marca), marca);
  gtk_widget_show (compra->see_marca);
  gtk_box_pack_end (GTK_BOX (hbox), compra->see_marca, FALSE, FALSE, 3);

  g_signal_connect (G_OBJECT (compra->see_marca), "changed",
                    G_CALLBACK (ChangeSave), NULL);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  label = gtk_label_new ("Unidad: ");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);

  compra->see_unidad = gtk_entry_new_with_max_length (10);
  gtk_entry_set_text (GTK_ENTRY (compra->see_unidad), unidad);
  gtk_widget_show (compra->see_unidad);
  gtk_box_pack_end (GTK_BOX (hbox), compra->see_unidad, FALSE, FALSE, 3);

  g_signal_connect (G_OBJECT (compra->see_unidad), "changed",
                    G_CALLBACK (ChangeSave), NULL);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  label = gtk_label_new ("Contenido: ");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);

  compra->see_contenido = gtk_entry_new_with_max_length (10);
  gtk_entry_set_text (GTK_ENTRY (compra->see_contenido), contenido);
  gtk_widget_show (compra->see_contenido);
  gtk_box_pack_end (GTK_BOX (hbox), compra->see_contenido, FALSE, FALSE, 3);

  g_signal_connect (G_OBJECT (compra->see_contenido), "changed",
                    G_CALLBACK (ChangeSave), NULL);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  label = gtk_label_new ("Precio: ");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);

  compra->see_precio = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (compra->see_precio), precio);
  gtk_widget_show (compra->see_precio);
  gtk_box_pack_end (GTK_BOX (hbox), compra->see_precio, FALSE, FALSE, 3);

  g_signal_connect (G_OBJECT (compra->see_precio), "changed",
                    G_CALLBACK (ChangeSave), NULL);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  label = gtk_label_new ("Venta Fraccionaria: ");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);

  button = gtk_radio_button_new_with_label (NULL, "No");
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);
  gtk_widget_show (button);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), FALSE);

  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
  button = gtk_radio_button_new_with_label (group, "Si");
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);
  gtk_widget_show (button);

  fraccion = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));

  if (VentaFraccion (barcode) == TRUE)
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
      fraccion = TRUE;
    }
  else
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), FALSE);
      fraccion = FALSE;
    }

  g_signal_connect (G_OBJECT (button), "toggled",
                    G_CALLBACK (ToggleSelect), (gpointer)"3");

  g_signal_connect (G_OBJECT (button), "toggled",
                    G_CALLBACK (ChangeSave), NULL);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  label = gtk_label_new ("Producto Perecible: ");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);

  button = gtk_radio_button_new_with_label (NULL, "No");
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);
  gtk_widget_show (button);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);

  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
  button = gtk_radio_button_new_with_label (group, "Si");
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "toggled",
                    G_CALLBACK (ToggleSelect), (gpointer)"2");

  if (strcmp (GetPerecible (barcode), "Perecible") == 0)
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
      perecible = TRUE;
    }
  else
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), FALSE);
      perecible = FALSE;
    }
  g_signal_connect (G_OBJECT (button), "toggled",
                    G_CALLBACK (ChangeSave), NULL);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  label = gtk_label_new ("Incluye IVA: ");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);

  button = gtk_radio_button_new_with_label (NULL, "No");
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);
  gtk_widget_show (button);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);


  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
  button = gtk_radio_button_new_with_label (group, "Si");
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "toggled",
                    G_CALLBACK (ToggleSelect), (gpointer)"1");

  if (GetIVA (barcode) != -1)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  else
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), FALSE);

  g_signal_connect (G_OBJECT (button), "toggled",
                    G_CALLBACK (ChangeSave), NULL);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  label = gtk_label_new ("Impuestos Adicionales: ");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);


  res = EjecutarSQL ("SELECT id, descripcion, monto FROM select_otros_impuestos()");

  tuples = PQntuples (res);

  combo_imp = gtk_combo_box_new_text ();
  gtk_box_pack_end (GTK_BOX (hbox), combo_imp, FALSE, FALSE, 3);
  gtk_widget_show (combo_imp);

  gtk_combo_box_append_text (GTK_COMBO_BOX (combo_imp), "Ninguno");

  active_char = GetOtrosName (barcode);

  for (i = 0; i < tuples; i++)
    {
      gtk_combo_box_append_text (GTK_COMBO_BOX (combo_imp),
                                 g_strdup_printf ("%s", PQvaluebycol (res, i, "descripcion")));

      if (active_char != NULL)
        {
          if (strcmp (PQvaluebycol (res, i, "descripcion"), active_char) == 0 &&
              strcmp (PQvaluebycol (res, i, "descripcion"), ""))
            active = i;
        }
      else
        active = -1;

    }

  //active = GetOtrosIndex (barcode);


  gtk_combo_box_set_active (GTK_COMBO_BOX (combo_imp), active != -1 ? active+1 : 0);

  g_signal_connect (G_OBJECT (combo_imp), "changed",
                    G_CALLBACK (ChangeSave), NULL);

}

void
ChangeSave (void)
{
  gtk_widget_set_sensitive (compra->see_button, TRUE);
}

void
Save (GtkWidget *widget, gpointer data)
{
  GtkTreeModel *model;
  GtkTreeIter iter;

  gchar *barcode = g_strdup (gtk_entry_get_text (GTK_ENTRY (compra->see_barcode)));

  gchar *codigo = g_strdup (gtk_entry_get_text (GTK_ENTRY (compra->see_codigo)));
  gchar *description = g_strdup (gtk_entry_get_text (GTK_ENTRY (compra->see_description)));
  gchar *marca = g_strdup (gtk_entry_get_text (GTK_ENTRY (compra->see_marca)));
  gchar *unidad = g_strdup (gtk_entry_get_text (GTK_ENTRY (compra->see_unidad)));
  gchar *contenido = g_strdup (gtk_entry_get_text (GTK_ENTRY (compra->see_contenido)));
  gchar *precio = g_strdup (gtk_entry_get_text (GTK_ENTRY (compra->see_precio)));
  gchar *otros, *familia;

  if (gtk_combo_box_get_active (GTK_COMBO_BOX (combo_imp)) != -1)
    {
      model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo_imp));
      gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combo_imp), &iter);

      gtk_tree_model_get (model, &iter,
                          0, &otros,
                          -1);
    }
  else
    otros = "";
  /*
    if (gtk_combo_box_get_active (GTK_COMBO_BOX (combo_fami)) != -1)
    {
    model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo_fami));
    gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combo_fami), &iter);

    gtk_tree_model_get (model, &iter,
    0, &familia,
    -1);
    }
  */
  SaveModifications (codigo, description, marca, unidad, contenido, precio,
                     iva, otros, barcode, familia, perecible, fraccion);

  CloseProductDescription ();
}

void
CalcularPrecioFinal (void)
{
  gchar *barcode = g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_buy_barcode"))));
  gdouble ingresa = strtod (PUT (g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_buy_price"))))), (char **)NULL);
  gdouble ganancia = (gdouble) atoi (g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_buy_gain")))));
  gdouble precio_final = (gdouble) atoi (g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_sell_price")))));
  gdouble precio;
  gdouble porcentaje;
  gdouble iva = GetIVA (barcode);
  gdouble otros = GetOtros (barcode);

  if (iva != -1)
    iva = (gdouble)iva / 100 + 1;
  else
    iva = -1;


  /*
    IVA = 1,19;
    Z = precio final
    Y = margen;
    X = Ingresa;

    Z      1
    X = ----- * ----
    Y+1    1,19
  */

  if (ganancia == 0 && precio_final == 0 && ingresa != 0)
    {
    }
  else if (ganancia == 0 && precio_final == 0 && ingresa == 0)
    {
    }
  else if (ingresa == 0 && ganancia >= 0 && precio_final != 0)
    {
      if (otros == -1 && iva != -1)
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
      gtk_entry_set_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_buy_price")),
                          g_strdup_printf ("%ld", lround (precio)));
    }
  else if (ganancia == 0 && ingresa != 0 && precio_final != 0)
    {
      if (otros == -1 && iva != -1)
        porcentaje = (gdouble) ((precio_final / (gdouble)(iva * ingresa)) -1) * 100;
      else if (iva != -1 && otros != -1)
        {
          iva = (gdouble) iva - 1;
          otros = (gdouble) otros / 100;

          precio = (gdouble) precio_final / (gdouble)(iva + otros + 1);
          ganancia = (gdouble) precio - ingresa;
          porcentaje = (gdouble)(ganancia / ingresa) * 100;

        }
      else if (iva == -1 && otros == -1)
        porcentaje = (gdouble) ((precio_final / ingresa) - 1) * 100;


      gtk_entry_set_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_buy_gain")),
                          g_strdup_printf ("%ld", lround (porcentaje)));
    }
  else if (precio_final == 0 && ingresa != 0 && ganancia >= 0)
    {
      if (otros == -1 && iva != -1)
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
    ErrorMSG (GTK_WIDGET (gtk_builder_get_object (builder, "entry_buy_price")), "Solamente 2 campos deben ser llenados");

}

void
AddToProductsList (void)
{
  gchar *barcode = g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_buy_barcode"))));
  gdouble cantidad;
  gdouble precio_compra = strtod (PUT(g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_buy_price"))))), (char **)NULL);
  gint margen = atoi (g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_buy_gain")))));
  gint precio = atoi (g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_sell_price")))));
  Producto *check;

  GtkListStore *store_history, *store_buy;

  store_buy = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (gtk_builder_get_object( builder, "tree_view_products_buy_list"))));

  cantidad = strtod (PUT(g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_buy_amount"))))),
                     (char **)NULL);

  if (precio_compra != 0 && (strcmp (GetCurrentPrice (barcode), "0") == 0 || precio != 0)
      && strcmp (barcode, "") != 0) //&& margen >= 0)
    {

      if (compra->header_compra != NULL)
        check = SearchProductByBarcode (barcode, FALSE);
      else
        check = NULL;

      if (check == NULL)
        {
          CompraAgregarALista (barcode, cantidad, precio, precio_compra, margen, FALSE);
          AddToTree ();
        }
      else
        {
          check->cantidad += cantidad;

          gtk_list_store_set (store_buy, &check->iter,
                              2, check->cantidad,
                              4, PutPoints (g_strdup_printf ("%.2f", check->cantidad *
                                                             check->precio_compra)),
                              -1);


        }


      gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_total_buy")),
                            g_strdup_printf ("<span size=\"xx-large\"><b>%s</b></span>",
                                             PutPoints (g_strdup_printf
                                                        ("%.2f",
                                                         CalcularTotalCompra
                                                         (compra->header_compra)))));
      store_history = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (gtk_builder_get_object( builder, "product_history_tree_view"))));
      gtk_list_store_clear (store_history);

      CleanStatusProduct ();

      gtk_widget_grab_focus (GTK_WIDGET (gtk_builder_get_object (builder, "entry_buy_barcode")));
    }
  else if (precio == 0 && strcmp (GetCurrentPrice (barcode), "0") != 0)
    {
      AskForCurrentPrice (barcode);
    }
  else
    {
      CalcularPrecioFinal ();
      AddToProductsList ();
    }
}

void
CloseAddWindow (GtkWidget *widget, gpointer data)
{
  if (compra->new_window != NULL)
    {
      if (widget != NULL && data == NULL)
        gtk_entry_set_text (GTK_ENTRY (compra->barcode_history_entry), "");

      gtk_widget_destroy (compra->new_window);
      compra->new_window = NULL;

      gtk_widget_set_sensitive (main_window, TRUE);
    }
}

void
AddNewProduct(void)
{
  GtkWidget *label;
  GtkWidget *button;

  GtkWidget *vbox;
  GtkWidget *hbox;

  GSList *group;

  PGresult *res;
  gint tuples, i;

  gchar *barcode = g_strdup (gtk_entry_get_text (GTK_ENTRY (compra->barcode_history_entry)));

  perecible = TRUE;

  gtk_widget_set_sensitive (main_window, FALSE);

  compra->new_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (compra->new_window), "Nuevo Producto");
  gtk_window_set_position (GTK_WINDOW (compra->new_window), GTK_WIN_POS_CENTER_ALWAYS);
  //  gtk_window_set_transient_for (GTK_WINDOW (compra->new_window), GTK_WINDOW (main_window));
  gtk_widget_show (compra->new_window);
  gtk_window_present (GTK_WINDOW (compra->new_window));
  g_signal_connect (G_OBJECT (compra->new_window), "destroy",
                    G_CALLBACK (CloseAddWindow), (gpointer)TRUE);

  vbox = gtk_vbox_new (FALSE, 3);
  gtk_widget_show (vbox);
  gtk_container_add (GTK_CONTAINER (compra->new_window), vbox);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_end (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  button = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
  gtk_widget_show (button);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 3);

  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (CloseAddWindow), (gpointer)TRUE);

  button = gtk_button_new_from_stock (GTK_STOCK_ADD);
  gtk_widget_show (button);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);

  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (AddNew), NULL);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  label = gtk_label_new ("Codigo Simple: ");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);

  compra->new_codigo = gtk_entry_new_with_max_length (10);
  gtk_widget_show (compra->new_codigo);
  gtk_box_pack_end (GTK_BOX (hbox), compra->new_codigo, FALSE, FALSE, 3);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  label = gtk_label_new ("Codigo de Barras: ");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);

  compra->new_barcode = gtk_entry_new_with_max_length (14);
  gtk_widget_show (compra->new_barcode);
  gtk_box_pack_end (GTK_BOX (hbox), compra->new_barcode, FALSE, FALSE, 3);

  gtk_entry_set_text (GTK_ENTRY (compra->new_barcode), barcode);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  label = gtk_label_new ("Descripcion: ");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);

  compra->new_description = gtk_entry_new_with_max_length (35);
  gtk_widget_show (compra->new_description);
  gtk_box_pack_end (GTK_BOX (hbox), compra->new_description, FALSE, FALSE, 3);

  if (strlen (barcode) > 0 && strlen (barcode) <= 6)
    {
      gtk_entry_set_text (GTK_ENTRY (compra->new_codigo), barcode);
      gtk_window_set_focus (GTK_WINDOW (compra->new_window), compra->new_description);
    }
  else
    gtk_window_set_focus (GTK_WINDOW (compra->new_window), compra->new_codigo);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  label = gtk_label_new ("Marca: ");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);

  compra->new_marca = gtk_entry_new_with_max_length (35);
  gtk_widget_show (compra->new_marca);
  gtk_box_pack_end (GTK_BOX (hbox), compra->new_marca, FALSE, FALSE, 3);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  label = gtk_label_new ("Contenido: ");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);

  compra->new_contenido = gtk_entry_new_with_max_length (10);
  gtk_widget_show (compra->new_contenido);
  gtk_box_pack_end (GTK_BOX (hbox), compra->new_contenido, FALSE, FALSE, 3);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  label = gtk_label_new ("Unidad: ");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);

  compra->new_unidad = gtk_entry_new_with_max_length (10);
  gtk_widget_show (compra->new_unidad);
  gtk_box_pack_end (GTK_BOX (hbox), compra->new_unidad, FALSE, FALSE, 3);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  label = gtk_label_new ("Venta Fraccionaria: ");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);

  button = gtk_radio_button_new_with_label (NULL, "No");
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);
  gtk_widget_show (button);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), FALSE);

  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
  button = gtk_radio_button_new_with_label (group, "Si");
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);
  gtk_widget_show (button);

  fraccion = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));

  g_signal_connect (G_OBJECT (button), "toggled",
                    G_CALLBACK (ToggleSelect), (gpointer)"3");

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  label = gtk_label_new ("Producto Perecible: ");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);

  button = gtk_radio_button_new_with_label (NULL, "No");
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);
  gtk_widget_show (button);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);

  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
  button = gtk_radio_button_new_with_label (group, "Si");
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);
  gtk_widget_show (button);

  perecible = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));

  g_signal_connect (G_OBJECT (button), "toggled",
                    G_CALLBACK (ToggleSelect), (gpointer)"2");

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  label = gtk_label_new ("Incluye IVA: ");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);

  button = gtk_radio_button_new_with_label (NULL, "No");
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);
  gtk_widget_show (button);

  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
  button = gtk_radio_button_new_with_label (group, "Si");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "toggled",
                    G_CALLBACK (ToggleSelect), (gpointer)"1");

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  label = gtk_label_new ("Impuestos Adicionales: ");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);


  res = EjecutarSQL ("SELECT descripcion FROM select_otros_impuestos()");

  tuples = PQntuples (res);

  combo_imp = gtk_combo_box_new_text ();
  gtk_box_pack_end (GTK_BOX (hbox), combo_imp, FALSE, FALSE, 3);
  gtk_widget_show (combo_imp);

  gtk_combo_box_append_text (GTK_COMBO_BOX (combo_imp), "Ninguno");

  for (i = 0; i < tuples; i++)
    gtk_combo_box_append_text (GTK_COMBO_BOX (combo_imp),
                               PQvaluebycol (res, i, "descripcion"));

  gtk_combo_box_set_active (GTK_COMBO_BOX (combo_imp), 0);

}

void
AddNew (GtkWidget *widget, gpointer data)
{
  GtkTreeModel *model;
  GtkTreeIter iter;

  gchar *codigo = g_strdup (gtk_entry_get_text (GTK_ENTRY (compra->new_codigo)));
  gchar *barcode = g_strdup (gtk_entry_get_text (GTK_ENTRY (compra->new_barcode)));
  gchar *description = g_strdup (gtk_entry_get_text (GTK_ENTRY (compra->new_description)));
  gchar *marca = g_strdup (gtk_entry_get_text (GTK_ENTRY (compra->new_marca)));
  gchar *contenido = g_strdup (gtk_entry_get_text (GTK_ENTRY (compra->new_contenido)));
  gchar *unidad = g_strdup (gtk_entry_get_text (GTK_ENTRY (compra->new_unidad)));
  gchar *otros, *familia;

  if (strcmp (codigo, "") == 0)
    ErrorMSG (compra->new_codigo, "Debe ingresar un codigo corto");
  else if (strcmp (barcode, "") == 0)
    ErrorMSG (compra->new_barcode, "Debe Ingresar un Codigo de Barras");
  else if (strcmp (description, "") == 0)
    ErrorMSG (compra->new_description, "Debe Ingresar una Descripcion");
  else if (strcmp (marca, "") == 0)
    ErrorMSG (compra->new_marca, "Debe Ingresar al Marca del producto");
  else if (strcmp (contenido, "") == 0)
    ErrorMSG (compra->new_contenido, "Debe Ingresar el Contenido del producto");
  else if (strcmp (unidad, "") == 0)
    ErrorMSG (compra->new_unidad, "Debe Ingresar la Unidad del producto");
  else
    {
      if (DataExist (g_strdup_printf ("SELECT codigo_corto FROM select_producto('%s')", codigo)))
        {
          ErrorMSG (compra->new_codigo, "Ya existe un producto con el mismo codigo corto");
          return;
        }

      if (gtk_combo_box_get_active (GTK_COMBO_BOX (combo_imp)) == 0)
        otros = "";
      else
        {
          model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo_imp));
          gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combo_imp), &iter);

          gtk_tree_model_get (model, &iter,
                              0, &otros,
                              -1);
        }

      AddNewProductToDB (codigo, barcode, description, marca,
                         CUT (contenido), unidad, iva, otros, familia, perecible, fraccion);

      //      SearchProductHistory (NULL, "");

      CloseAddWindow (NULL, NULL);

      gtk_widget_grab_focus (GTK_WIDGET (gtk_builder_get_object (builder, "entry_buy_price")));
    }

  return;
}

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
                      4, PutPoints (g_strdup_printf ("%.2f", compra->current->cantidad *
                                                     compra->current->precio_compra)),
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

}

void
GetBuyData (void)
{

}

void
CloseSearchByName (void)
{
  gtk_widget_destroy (compra->find_win);

  compra->find_win = NULL;

  gtk_widget_set_sensitive (main_window, TRUE);
  gtk_window_set_focus (GTK_WINDOW (main_window), compra->barcode_history_entry);
}

void
AddFoundProduct (void)
{
  GtkTreeView *treeview = GTK_TREE_VIEW (gtk_builder_get_object (builder, "treeview_buscador"));
  GtkTreeIter iter;
  GtkTreeModel *model = gtk_tree_view_get_model (treeview);
  gchar *barcode;

  if (gtk_tree_selection_get_selected (gtk_tree_view_get_selection (treeview), NULL, &iter))
    {
      gtk_tree_model_get (GTK_TREE_MODEL (model), &iter,
                          1, &barcode,
                          -1);

      gtk_entry_set_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_buy_barcode")), barcode);
      gtk_widget_hide (GTK_WIDGET (gtk_builder_get_object (builder, "wnd_buscador")));
      gtk_widget_grab_focus (GTK_WIDGET (gtk_builder_get_object (builder, "entry_buy_price")));
    }
}

void
SearchName ()
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

  if (resultado != 0) gtk_widget_grab_focus (GTK_WIDGET (gtk_builder_get_object (builder, "entry_buscar")));
}

void
SearchByName()
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
      column = gtk_tree_view_column_new_with_attributes ("Codigo Simple", renderer,
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
      column = gtk_tree_view_column_new_with_attributes ("Descripcion del Producto", renderer,
                                                         "text", 2,
                                                         NULL);
      gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      gtk_tree_view_column_set_min_width (column, 200);
      gtk_tree_view_column_set_max_width (column, 200);
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

  if ( ! g_str_equal (string, ""))
    {
      gtk_entry_set_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_buscar")), string);
      SearchName();
    }
}

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
}

void
ShowProductHistory (void)
{
  PGresult *res;
  GtkTreeIter iter;

  GtkListStore *store;

  gchar *barcode = g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object( builder, "entry_buy_barcode" ))));
  gint i, tuples, precio = 0;


  res = EjecutarSQL
    (g_strdup_printf
     ("SELECT (SELECT nombre FROM proveedor WHERE rut=t1.rut_proveedor) as proveedor, t2.precio, t2.cantidad, date_part('day', t1.fecha) as dia, "
      "date_part('month', t1.fecha) as mes, date_part('year', t1.fecha) as ano, t1.id, t2.iva, t2.otros_impuestos "
      "FROM compra AS t1, compra_detalle AS t2, producto "
      "WHERE producto.barcode='%s' AND t2.barcode_product=producto.barcode AND t1.id=t2.id_compra AND t2.anulado='f' ORDER BY t1.fecha DESC", barcode));

  tuples = PQntuples (res);

  store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (gtk_builder_get_object( builder, "product_history_tree_view"))));

  gtk_list_store_clear (store);

  for (i = 0; i < tuples; i++)
    {
      if (strcmp (PQvaluebycol (res, i, "iva"), "0") != 0)
        {
          precio = lround ((double) (atoi(PQvaluebycol (res, i, "precio")) + lround((double)atoi(PQvaluebycol (res, i, "iva"))/
                                                                                    strtod (PUT (PQvaluebycol (res, i, "cantidad")), (char **)NULL))));
        }
      if (strcmp (PQvaluebycol (res, i, "otros_impuestos"), "0") != 0)
        {
          precio += lround ((double) lround ((double)atoi(PQvaluebycol (res, i, "otros_impuestos")) /
                                             strtod (PUT (PQvaluebycol (res, i, "cantidad")), (char **)NULL)));
        }

      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
                          0, g_strdup_printf ("%.2d/%.2d/%s", atoi (PQvaluebycol (res, i, "dia")),
                                              atoi (PQvaluebycol (res, i, "mes")), PQvaluebycol (res, i, "ano")),
                          1, PQvaluebycol (res, i, "id"),
                          2, PQvaluebycol (res, i, "proveedor"),
                          3, strtod (PUT (PQvaluebycol (res, i, "cantidad")), (char **)NULL),
                          4, precio,
                          -1);
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
  GtkTreeIter iter;

  GtkListStore *store_pending_request = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_pending_requests"))));
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
    }
}

void
IngresoDetalle (GtkTreeSelection *selection1, gpointer data)
{
  gint i, id, tuples;
  gboolean color;
  GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_pending_requests")));
  GtkTreeIter iter;

  GtkListStore *store_pending_request = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_pending_requests"))));
  GtkListStore *store_pending_request_detail = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_pending_requests_detail"))));

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
}

void
IngresarCompra (void)
{
  Productos *products = compra->header;
  gint id, doc;
  gchar *rut_proveedor;
  gchar *q;
  GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (compra->ingreso_tree));
  GtkTreeIter iter;

  if (gtk_tree_selection_get_selected (selection, NULL, &iter) == FALSE)
    return;

  gtk_tree_model_get (GTK_TREE_MODEL (compra->ingreso_store), &iter,
                      0, &id,
                      -1);

  if (products != NULL)
    {
      do {

        IngresarProducto (products->product, id);

        IngresarDetalleDocumento (products->product, id,
                                  atoi (gtk_entry_get_text (GTK_ENTRY (compra->n_documento))),
                                  compra->factura);

        products = products->next;
      }
      while (products != compra->header);

      //      SetProductosIngresados ();

    }
  //TODO: usar funcion plpgsql
  q = g_strdup_printf ("SELECT proveedor FROM get_proveedor_compra( %d )", id);
  rut_proveedor = GetDataByOne (q);
  g_free (q);

  if (compra->factura == TRUE)
    doc = IngresarFactura
      (g_strdup (gtk_entry_get_text (GTK_ENTRY (compra->n_documento))),
       id, rut_proveedor,
       atoi (CutPoints (g_strdup (gtk_entry_get_text (GTK_ENTRY (compra->monto_documento))))),
       g_strdup (gtk_entry_get_text (GTK_ENTRY (compra->fecha_emision_d))),
       g_strdup (gtk_entry_get_text (GTK_ENTRY (compra->fecha_emision_m))),
       g_strdup (gtk_entry_get_text (GTK_ENTRY (compra->fecha_emision_y))), 0);
  else
    doc = IngresarGuia
      (g_strdup (gtk_entry_get_text (GTK_ENTRY (compra->n_documento))),
       id, atoi (CutPoints (g_strdup (gtk_entry_get_text (GTK_ENTRY (compra->fact_monto))))),
       g_strdup (gtk_entry_get_text (GTK_ENTRY (compra->fecha_emision_d))),
       g_strdup (gtk_entry_get_text (GTK_ENTRY (compra->fecha_emision_m))),
       g_strdup (gtk_entry_get_text (GTK_ENTRY (compra->fecha_emision_y))));



  CloseDocumentoIngreso ();

  CompraIngresada ();

  InsertarCompras ();

  gtk_window_set_focus (GTK_WINDOW (main_window), compra->ingreso_tree);

}

void
CloseSelectProveedores (GtkWidget *widget, gpointer data)
{
  GtkWidget *window = gtk_widget_get_toplevel (widget);
  gboolean cancel = (gboolean) data;

  gtk_widget_destroy (window);

  if (cancel == TRUE)
    gtk_window_set_focus (GTK_WINDOW (main_window), compra->fact_proveedor);
}

void
FillProveedores ()
{
  PGresult *res;
  gint tuples, i;

  GtkTreeIter iter;

  res = EjecutarSQL ("SELECT rut, nombre FROM select_proveedor() "
                     "ORDER BY nombre ASC");

  if (res == NULL)
    return;

  tuples = PQntuples (res);

  gtk_list_store_clear (compra->store_prov);

  for (i = 0; i < tuples; i++)
    {
      gtk_list_store_append (compra->store_prov, &iter);
      gtk_list_store_set (compra->store_prov, &iter,
                          0, PQvaluebycol(res, i, "rut"),
                          1, PQvaluebycol(res, i, "nombre"),
                          -1);
    }
}

void
CloseAddProveedorWindow (GtkWidget *button, gpointer data)
{
  GtkWidget *window = (GtkWidget *)data;

  gtk_widget_destroy (window);

  gtk_window_set_focus (GTK_WINDOW (compra->buy_window), compra->tree_prov);
}

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
            (g_strdup_printf ("SELECT * FROM proveedor WHERE rut='%s-%s'", rut, rut_ver))) != NULL)
    {
      ErrorMSG (compra->rut_add, "Ya existe un proveedor con el mismo rut");
      return;
    }
  else if (g_str_equal (rut_ver, ""))
    {
      ErrorMSG (compra->rut_ver, "Debe ingresar el digito verificador del rut");
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
}

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

}

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
ActiveBuy (GtkWidget *widget, gpointer data)
{
  /* gchar *precio = g_strdup (gtk_entry_get_text (GTK_ENTRY (ingreso_entry))); */
  /* gchar *barcode_history = g_strdup (gtk_entry_get_text (GTK_ENTRY (compra->barcode_history_entry))); */

  /* if (strcmp (precio, "") != 0 && strcmp (barcode_history, "") != 0) */
  /*   gtk_widget_set_sensitive (add_button, TRUE); */
  /* else */
  /*   gtk_widget_set_sensitive (add_button, FALSE); */

}

void
Ingresar (GtkCellRendererToggle *cellrenderertoggle, gchar *path_str, gpointer data)
{
  GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
  GtkTreeIter iter;
  gchar *codigo;
  //  Productos *producto;

  gtk_tree_model_get_iter (GTK_TREE_MODEL (compra->compra_store), &iter, path);
  gtk_tree_model_get (GTK_TREE_MODEL (compra->compra_store), &iter,
                      0, &codigo,
                      -1);

  /*
    if (ingresar == FALSE)
    ingresar = TRUE;
    else
    ingresar = FALSE;

    gtk_list_store_set (GTK_LIST_STORE (compra->compra_store), &iter,
    0, ingresar,
    -1);

    producto = BuscarPorCodigo (codigo);

    producto->product->ingresar = ingresar;
  */

}

void
CloseIngresoParcial (GtkWidget *widget, gpointer data)
{
  GtkWidget *window = (GtkWidget *)data;

  gtk_widget_destroy (window);

  gtk_widget_set_sensitive (main_window, TRUE);
}

void
IngresoParcial (void)
{
  GtkWidget *window;
  GtkWidget *vbox;
  GtkWidget *vbox2;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *button;

  gtk_widget_set_sensitive (main_window, FALSE);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER_ALWAYS);
  gtk_window_set_resizable (GTK_WINDOW (window), FALSE);
  gtk_widget_set_size_request (window, 230, 100);
  //  gtk_window_set_transient_for (GTK_WINDOW (window), GTK_WINDOW (main_window));
  gtk_widget_show (window);
  gtk_window_present (GTK_WINDOW (window));

  vbox = gtk_vbox_new (FALSE, 3);
  gtk_widget_show (vbox);
  gtk_container_add (GTK_CONTAINER (window), vbox);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  vbox2 = gtk_vbox_new (FALSE, 3);
  gtk_widget_show (vbox2);
  gtk_box_pack_start (GTK_BOX (hbox), vbox2, FALSE, FALSE, 3);

  label = gtk_label_new ("Stock a Ingresar: ");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (vbox2), label, FALSE, FALSE, 3);

  entry_stock = gtk_entry_new ();
  gtk_widget_show (entry_stock);
  gtk_box_pack_start (GTK_BOX (vbox2), entry_stock, FALSE, FALSE, 3);

  g_signal_connect (G_OBJECT (entry_stock), "changed",
                    G_CALLBACK (ChangeIngreso), NULL);

  gtk_window_set_focus (GTK_WINDOW (window), entry_stock);

  vbox2 = gtk_vbox_new (FALSE, 3);
  gtk_widget_show (vbox2);
  gtk_box_pack_start (GTK_BOX (hbox), vbox2, FALSE, FALSE, 3);

  button = gtk_button_new_with_label ("  Total  ");
  gtk_widget_show (button);
  gtk_box_pack_end (GTK_BOX (vbox2), button, FALSE, FALSE, 3);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  button = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
  gtk_widget_show (button);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 3);

  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (CloseIngresoParcial), (gpointer)window);

  button = gtk_button_new_from_stock (GTK_STOCK_OK);
  gtk_widget_show (button);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);

  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (CambiarStock), (gpointer)window);

  g_signal_connect (G_OBJECT (entry_stock), "activate",
                    G_CALLBACK (SendCursorTo), (gpointer)button);
}

void
ChangeIngreso (GtkEntry *entry, gpointer data)
{
  gchar *valor = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry_stock)));
  gint len = strlen (valor);

  if (len == 0)
    return;

  if (isdigit (valor[len-1]) == 0)
    gtk_entry_set_text (GTK_ENTRY (entry_stock), g_strndup (valor, len-1));
}

void
CambiarStock (GtkWidget *widget, gpointer data)
{
  //  gint new_stock = atoi (gtk_entry_get_text (GTK_ENTRY (entry_stock)));
  gdouble new_stock = strtod (PUT ((gchar *)gtk_entry_get_text (GTK_ENTRY (entry_stock))), (char **)NULL);
  gchar *codigo;
  GtkWidget *window = (GtkWidget *) data;
  GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (compra->compra_tree));
  GtkTreeIter iter;
  Productos *products;

  if (gtk_tree_selection_get_selected (selection, NULL, &iter) == TRUE &&
      new_stock != 0)
    {
      gtk_tree_model_get (GTK_TREE_MODEL (compra->compra_store), &iter,
                          0, &codigo,
                          -1);

      products = BuscarPorCodigo (compra->header, codigo);

      if (new_stock > products->product->cantidad)
        {
          ErrorMSG (entry_stock, "No se puede ingresar mas stock del pedido");
          products->product->ingresar = FALSE;
        }
      else
        products->product->cantidad = (gdouble) new_stock;

      CloseIngresoParcial (NULL, window);

      CalcularTotales ();

      ingreso_total = FALSE;

      AskIngreso (GTK_WINDOW (main_window));

    }
  else
    {
      ErrorMSG (entry_stock, "No se puede hacer un ingreso parcial de 0");
    }

}

void
AnularCompra (void)
{
  //  PGresult *res;

  GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (compra->ingreso_tree));
  GtkTreeIter iter;
  gint id_compra;

  if (gtk_tree_selection_get_selected (selection, NULL, &iter) == TRUE)
    {
      gtk_tree_model_get (GTK_TREE_MODEL (compra->ingreso_store), &iter,
                          0, &id_compra,
                          -1);

      AnularCompraDB (id_compra);

      InsertarCompras ();

    }
}

void
AnularProducto (void)
{

  PGresult *res;

  GtkTreeSelection *selection1 = gtk_tree_view_get_selection (GTK_TREE_VIEW (compra->ingreso_tree));
  GtkTreeSelection *selection2 = gtk_tree_view_get_selection (GTK_TREE_VIEW (compra->compra_tree));
  GtkTreeIter iter1, iter2;
  gint id_compra;
  gchar *codigo_producto;
  gchar *q;

  if (gtk_tree_selection_get_selected (selection1, NULL, &iter1) &&
      gtk_tree_selection_get_selected (selection2, NULL, &iter2))
    {
      gtk_tree_model_get (GTK_TREE_MODEL (compra->ingreso_store), &iter1,
                          0, &id_compra,
                          -1);
      gtk_tree_model_get (GTK_TREE_MODEL (compra->compra_store), &iter2,
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

  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_product_desc")), "");
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_mark")), "");
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_unit")), "");
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_stock")), "");
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_stock_further")), "");
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_sell_price")), "");
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_fifo")), "");

  gtk_entry_set_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_buy_price")), "");
  gtk_entry_set_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_buy_gain")), "");
  gtk_entry_set_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_sell_price")), "");
  gtk_entry_set_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_buy_amount")), "1");

  gtk_entry_set_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_buy_barcode")), "");

  gtk_widget_grab_focus (GTK_WIDGET (gtk_builder_get_object (builder, "entry_buy_barcode")));
}

void
CloseAskForCurrentPrice (GtkWidget *widget, gpointer data)
{
  GtkWidget *window = (GtkWidget *)data;

  gtk_widget_destroy (window);

  gtk_widget_set_sensitive (main_window, TRUE);
}

void
AcceptCurrentPrice (GtkWidget *widget, gpointer data)
{
  /* gchar *barcode = g_strdup (gtk_entry_get_text (GTK_ENTRY (compra->barcode_history_entry))); */

  /* gtk_entry_set_text (GTK_ENTRY (precio_final_entry), GetCurrentPrice (barcode)); */

  /* AddToProductsList (); */

  /* CloseAskForCurrentPrice (NULL, data); */
}

void
AskForCurrentPrice (gchar *barcode)
{
  GtkWidget *window;

  GtkWidget *hbox;
  GtkWidget *vbox;

  GtkWidget *label;
  GtkWidget *image;

  GtkWidget *button;

  gtk_widget_set_sensitive (main_window, FALSE);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "Confirmar Precio");
  //  gtk_window_set_transient_for (GTK_WINDOW (window), GTK_WINDOW (main_window));
  gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER_ALWAYS);
  gtk_widget_show (window);

  vbox = gtk_vbox_new (FALSE, 3);
  gtk_container_add (GTK_CONTAINER (window), vbox);
  gtk_widget_show (vbox);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  image = gtk_image_new_from_stock (GTK_STOCK_DIALOG_QUESTION, GTK_ICON_SIZE_DIALOG);
  gtk_widget_show (image);
  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 3);

  label = gtk_label_new (g_strdup_printf ("¿Desea mantener el $%s como precio actual?",
                                          GetCurrentPrice (barcode)));
  gtk_box_pack_end (GTK_BOX (hbox), label, FALSE, FALSE, 3);
  gtk_widget_show (label);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_end (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  button = gtk_button_new_from_stock (GTK_STOCK_NO);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 3);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (CloseAskForCurrentPrice), (gpointer)window);

  button = gtk_button_new_from_stock (GTK_STOCK_YES);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (AcceptCurrentPrice), (gpointer)window);

  gtk_window_set_focus (GTK_WINDOW (window), button);

}

void
CloseDocumentoIngreso (void)
{
  gtk_widget_destroy (compra->documentos);
  compra->documentos = NULL;

  gtk_widget_set_sensitive (main_window, TRUE);

  gtk_window_set_focus (GTK_WINDOW (main_window), compra->ingreso_tree);
}

void
DocumentoIngreso (void)
{
  //  Productos *products = compra->header;

  GtkWidget *label;

  GtkWidget *vbox;
  GtkWidget *hbox;
  //  GtkWidget *scroll;
  GtkWidget *button;

  /*GtkTreeIter iter;
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;*/

  if (compra->documentos != NULL)
    return;

  gtk_widget_set_sensitive (main_window, FALSE);

  compra->documentos = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (compra->documentos), "Documento de Ingreso");
  gtk_window_set_position (GTK_WINDOW (compra->documentos), GTK_WIN_POS_CENTER_ALWAYS);
  //  gtk_window_set_transient_for (GTK_WINDOW (compra->documentos), GTK_WINDOW (main_window));
  gtk_window_present (GTK_WINDOW (compra->documentos));
  gtk_widget_show (compra->documentos);

  g_signal_connect (G_OBJECT (compra->documentos), "destroy",
                    G_CALLBACK (CloseDocumentoIngreso), NULL);

  vbox = gtk_vbox_new (FALSE, 3);
  gtk_container_add (GTK_CONTAINER (compra->documentos), vbox);
  gtk_widget_show (vbox);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  label = gtk_label_new ("La mercadería ingresa con: ");
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
  gtk_widget_show (label);

  /*
    Necesitamos saber si tenemos puesta la pregunta del numero de documento
  */
  compra->n_doc = FALSE;

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  button = gtk_button_new_with_label ("Factura");
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 3);
  gtk_widget_show (button);

  gtk_window_set_focus (GTK_WINDOW (compra->documentos), button);

  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (DocumentoFactura), (gpointer)vbox);

  button = gtk_button_new_with_label ("Guia de Despacho");
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (DocumentoGuia), (gpointer)vbox);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  compra->error_documento = gtk_label_new ("");
  gtk_box_pack_start (GTK_BOX (hbox), compra->error_documento, FALSE, FALSE, 3);
  gtk_widget_show (compra->error_documento);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_end (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  button = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 3);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (CloseDocumentoIngreso), NULL);

  ok_doc = gtk_button_new_from_stock (GTK_STOCK_OK);
  gtk_box_pack_end (GTK_BOX (hbox), ok_doc, FALSE, FALSE, 3);
  gtk_widget_show (ok_doc);

  g_signal_connect (G_OBJECT (ok_doc), "clicked",
                    G_CALLBACK (AskElabVenc), (gpointer)compra->header);

  gtk_widget_set_sensitive (ok_doc, FALSE);

  //  AskIngreso (GTK_WINDOW (compra->documentos));
}

void
DocumentoFactura (GtkWidget *widget, gpointer data)
{
  GtkWidget *vbox = (GtkWidget *)data;

  GtkWidget *hbox;
  GtkWidget *label;
  //  GtkWidget *button;

  //  Productos *products = compra->header;

  if (compra->n_doc == FALSE)
    compra->n_doc = TRUE;
  else
    return;

  compra->factura = TRUE;

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  label = gtk_label_new ("Numero de la Factura: ");
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
  gtk_widget_show (label);

  compra->n_documento = gtk_entry_new ();
  gtk_box_pack_end (GTK_BOX (hbox), compra->n_documento, FALSE, FALSE, 3);
  gtk_widget_show (compra->n_documento);

  gtk_window_set_focus (GTK_WINDOW (compra->documentos), compra->n_documento);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  label = gtk_label_new ("Fecha de Emision: ");
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
  gtk_widget_show (label);

  compra->fecha_emision_y = gtk_entry_new_with_max_length (2);
  gtk_widget_set_size_request (compra->fecha_emision_y, 25, -1);
  gtk_box_pack_end (GTK_BOX (hbox), compra->fecha_emision_y, FALSE, FALSE, 3);
  gtk_widget_show (compra->fecha_emision_y);

  compra->fecha_emision_m = gtk_entry_new_with_max_length (2);
  gtk_widget_set_size_request (compra->fecha_emision_m, 25, -1);
  gtk_box_pack_end (GTK_BOX (hbox), compra->fecha_emision_m, FALSE, FALSE, 3);
  gtk_widget_show (compra->fecha_emision_m);

  g_signal_connect (G_OBJECT (compra->fecha_emision_m), "activate",
                    G_CALLBACK (SendCursorTo), (gpointer)compra->fecha_emision_y);

  compra->fecha_emision_d = gtk_entry_new_with_max_length (2);
  gtk_widget_set_size_request (compra->fecha_emision_d, 25, -1);
  gtk_box_pack_end (GTK_BOX (hbox), compra->fecha_emision_d, FALSE, FALSE, 3);
  gtk_widget_show (compra->fecha_emision_d);

  g_signal_connect (G_OBJECT (compra->fecha_emision_d), "activate",
                    G_CALLBACK (SendCursorTo), (gpointer)compra->fecha_emision_m);

  g_signal_connect (G_OBJECT (compra->n_documento), "activate",
                    G_CALLBACK (SendCursorTo), (gpointer)compra->fecha_emision_d);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  label = gtk_label_new ("Monto de la Factura: ");
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
  gtk_widget_show (label);

  compra->monto_documento = gtk_entry_new ();
  gtk_box_pack_end (GTK_BOX (hbox), compra->monto_documento, FALSE, FALSE, 3);
  gtk_widget_show (compra->monto_documento);

  g_signal_connect (G_OBJECT (compra->fecha_emision_y), "activate",
                    G_CALLBACK (SendCursorTo), (gpointer)compra->monto_documento);

  g_signal_connect (G_OBJECT (compra->monto_documento), "activate",
                    G_CALLBACK (CheckMontoIngreso), NULL);

  gtk_entry_set_text (GTK_ENTRY (compra->monto_documento),
                      gtk_label_get_text (GTK_LABEL (compra->total)));
}

void
DocumentoGuia (GtkWidget *widget, gpointer data)
{
  GtkWidget *vbox = (GtkWidget *)data;

  GtkWidget *hbox;
  GtkWidget *label;

  //  Productos *products = compra->header;

  if (compra->n_doc == FALSE)
    compra->n_doc = TRUE;
  else
    return;

  compra->factura = FALSE;

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  label = gtk_label_new ("Numero de la Guia: ");
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
  gtk_widget_show (label);

  compra->n_documento = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (hbox), compra->n_documento, FALSE, FALSE, 3);
  gtk_widget_show (compra->n_documento);

  gtk_window_set_focus (GTK_WINDOW (compra->documentos), compra->n_documento);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  label = gtk_label_new ("Fecha de Emision: ");
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
  gtk_widget_show (label);

  compra->fecha_emision_y = gtk_entry_new_with_max_length (2);
  gtk_widget_set_size_request (compra->fecha_emision_y, 25, -1);
  gtk_box_pack_end (GTK_BOX (hbox), compra->fecha_emision_y, FALSE, FALSE, 3);
  gtk_widget_show (compra->fecha_emision_y);

  compra->fecha_emision_m = gtk_entry_new_with_max_length (2);
  gtk_widget_set_size_request (compra->fecha_emision_m, 25, -1);
  gtk_box_pack_end (GTK_BOX (hbox), compra->fecha_emision_m, FALSE, FALSE, 3);
  gtk_widget_show (compra->fecha_emision_m);

  g_signal_connect (G_OBJECT (compra->fecha_emision_m), "activate",
                    G_CALLBACK (SendCursorTo), (gpointer)compra->fecha_emision_y);

  compra->fecha_emision_d = gtk_entry_new_with_max_length (2);
  gtk_widget_set_size_request (compra->fecha_emision_d, 25, -1);
  gtk_box_pack_end (GTK_BOX (hbox), compra->fecha_emision_d, FALSE, FALSE, 3);
  gtk_widget_show (compra->fecha_emision_d);

  g_signal_connect (G_OBJECT (compra->n_documento), "activate",
                    G_CALLBACK (SendCursorTo), (gpointer)compra->fecha_emision_d);

  g_signal_connect (G_OBJECT (compra->fecha_emision_d), "activate",
                    G_CALLBACK (SendCursorTo), (gpointer)compra->fecha_emision_m);

  g_signal_connect (G_OBJECT (compra->fecha_emision_y), "toggle-overwrite",
                    G_CALLBACK (SendCursorTo), (gpointer) compra->fecha_emision_m);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  label = gtk_label_new ("Monto de la Guia: ");
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
  gtk_widget_show (label);

  compra->monto_documento = gtk_entry_new ();
  gtk_box_pack_end (GTK_BOX (hbox), compra->monto_documento, FALSE, FALSE, 3);
  gtk_widget_show (compra->monto_documento);

  g_signal_connect (G_OBJECT (compra->fecha_emision_y), "activate",
                    G_CALLBACK (SendCursorTo), (gpointer)compra->monto_documento);

  g_signal_connect (G_OBJECT (compra->monto_documento), "activate",
                    G_CALLBACK (CheckMontoIngreso), NULL);

  gtk_entry_set_text (GTK_ENTRY (compra->monto_documento),
                      gtk_label_get_text (GTK_LABEL (compra->total)));
}

gboolean
CheckDocumentData (gboolean factura, gchar *rut_proveedor, gint id)
{
  PGresult *res;

  gchar *n_documento = g_strdup (gtk_entry_get_text (GTK_ENTRY (compra->n_documento)));
  gchar *fecha_y = g_strdup (gtk_entry_get_text (GTK_ENTRY (compra->fecha_emision_y)));
  gchar *fecha_m = g_strdup (gtk_entry_get_text (GTK_ENTRY (compra->fecha_emision_m)));
  gchar *fecha_d = g_strdup (gtk_entry_get_text (GTK_ENTRY (compra->fecha_emision_d)));
  gchar *monto = g_strdup (gtk_entry_get_text (GTK_ENTRY (compra->monto_documento)));


  res = EjecutarSQL (g_strdup_printf ("SELECT date_part('day', fecha), "
                                      "date_part('month', fecha), "
                                      "date_part('year', fecha) "
                                      "FROM compra WHERE id=%d", id));

  if (atoi (fecha_y) < (atoi (PQgetvalue (res, 0, 2)) - 2000))
    {
      ErrorMSG (compra->fecha_emision_y,
                "La fecha de emision del documento no puede ser menor a\nla fecha de compra");
      return FALSE;
    }
  else
    {
      if (atoi (fecha_m) < atoi (PQgetvalue (res, 0, 1)))
        {
          ErrorMSG (compra->fecha_emision_m,
                    "La fecha de emision del documento no puede ser menor a\nla fecha de compra");
          return FALSE;
        }
      else if (atoi (fecha_m) == atoi (PQgetvalue (res, 0, 1)))
        {
          if (atoi (fecha_d) < atoi (PQgetvalue (res, 0, 0)))
            {
              ErrorMSG (compra->fecha_emision_d,
                        "La fecha de emision del documento no puede ser menor a\nla fecha de compra");
              return FALSE;
            }
        }
    }


  /*
    Un switch es mucho mas elegante para el caso, sin embargo debe buscarse
    una manera mas limpia de ejecutar el chequeo de datos.
  */

  switch (factura)
    {
    case TRUE:
      if (strcmp (n_documento, "") == 0)
        {
          ErrorMSG (compra->n_documento,
                    "Debe Obligatoriamente ingresar el numero del documento");
          return FALSE;
        }
      else if (strcmp (fecha_y, "") == 0)
        {
          ErrorMSG (compra->fecha_emision_y,
                    "Debe Obligatoriamente ingresar el a��±o de emision");
          return FALSE;
        }
      else if (strcmp (fecha_m, "") == 0)
        {
          ErrorMSG (compra->fecha_emision_m,
                    "Debe Obligatoriamente ingresar el mes de emision");
          return FALSE;
        }
      else if (strcmp (fecha_d, "") == 0)
        {
          ErrorMSG (compra->fecha_emision_d,
                    "Debe Obligatoriamente ingresar el dia de la emision");
          return FALSE;
        }
      else if (strcmp (monto, "") == 0)
        {
          ErrorMSG (compra->monto_documento, "El Monto del documento debe ser ingresado");
          return FALSE;
        }
      else
        {
          if (DataExist (g_strdup_printf ("SELECT num_factura FROM factura_compra WHERE rut_proveedor='%s' AND num_factura=%s", rut_proveedor, n_documento)) == TRUE)
            {
              ErrorMSG (compra->n_documento, g_strdup_printf ("Ya existe la factura %s ingresada de este proveedor", n_documento));
              return FALSE;
            }
          return TRUE;
        }
      break;
    case FALSE:
      if (strcmp (n_documento, "") == 0)
        {
          ErrorMSG (compra->n_documento, "Debe Obligatoriamente ingresar el numero del documento");
          return FALSE;
        }
      else if (strcmp (fecha_y, "") == 0)
        {
          ErrorMSG (compra->fecha_emision_y, "Debe Obligatoriamente ingresar el a��±o de emision");
          return FALSE;
        }
      else if (strcmp (fecha_m, "") == 0)
        {
          ErrorMSG (compra->fecha_emision_m, "Debe Obligatoriamente ingresar el mes de emision");
          return FALSE;
        }
      else if (strcmp (fecha_d, "") == 0)
        {
          ErrorMSG (compra->fecha_emision_d, "Debe Obligatoriamente ingresar el dia de la emision");
          return FALSE;
        }
      else if (strcmp (monto, "") == 0)
        {
          ErrorMSG (compra->monto_documento, "El Monto del documento debe ser ingresado");
          return FALSE;
        }
      else
        {
          if (DataExist (g_strdup_printf ("SELECT numero FROM guias_compra WHERE rut_proveedor='%s' AND numero=%s", rut_proveedor, n_documento)) == TRUE)
            {
              ErrorMSG (compra->n_documento, g_strdup_printf ("Ya existe la guia %s ingresada de este proveedor", n_documento));
              return FALSE;
            }
          return TRUE;
        }
      break;
    default:
      return FALSE;
    }
}

void
FillPagarFacturas (gchar *rut_proveedor)
{
  gint tuples, tuples2, i, j;
  gint year = 0, month = 0, day = 0;
  gint monto_fecha = 0;
  GtkTreeIter fecha_iter, factura_iter, guia_iter;

  PGresult *res, *res2;

  if (rut_proveedor != NULL)
    res = EjecutarSQL
      (g_strdup_printf
       ("SELECT t1.id, t1.num_factura, t1.monto, date_part ('day', t1.fecha), date_part('month', t1.fecha), date_part('year', t1.fecha), t1.id_compra, date_part ('day', fecha_pago) AS pay_day, date_part ('month', fecha_pago) AS pay_month, date_part ('year', fecha_pago) AS pay_year, t1.forma_pago, t1.id, t1.rut_proveedor FROM factura_compra AS t1 WHERE t1.rut_proveedor='%s' AND t1.pagada='f' ORDER BY pay_year, pay_month, pay_day ASC", rut_proveedor));
  else
    res = EjecutarSQL
      ("SELECT t1.id, t1.num_factura, t1.monto, date_part ('day', t1.fecha), date_part('month', t1.fecha), date_part('year', t1.fecha), t1.id_compra, date_part ('day', fecha_pago) AS pay_day, date_part ('month', fecha_pago) AS pay_month, date_part ('year', fecha_pago) AS pay_year, t1.forma_pago, t1.id, t1.rut_proveedor FROM factura_compra AS t1 WHERE pagada='f' ORDER BY pay_year, pay_month, pay_day ASC");

  if (res == NULL)
    return;

  tuples = PQntuples (res);

  gtk_tree_store_clear (compra->store_facturas);

  for (i = 0; i < tuples; i++)
    {
      if (atoi (PQgetvalue (res, i, 7)) > day || atoi (PQgetvalue (res, i, 8)) > month
          || atoi (PQgetvalue (res, i, 9)) > year)
        {
          if (gtk_tree_store_iter_is_valid (compra->store_facturas, &fecha_iter) != FALSE)
            {
              gtk_tree_store_set (compra->store_facturas, &fecha_iter,
                                  6, PutPoints (g_strdup_printf ("%d", monto_fecha)),
                                  -1);
              monto_fecha = 0;
            }

          day = atoi (PQgetvalue (res, i, 7));
          month = atoi (PQgetvalue (res, i, 8));
          year = atoi (PQgetvalue (res, i, 9));

          gtk_tree_store_append (compra->store_facturas, &fecha_iter, NULL);
          gtk_tree_store_set (compra->store_facturas, &fecha_iter,
                              5, g_strdup_printf
                              ("%.2d/%.2d/%.4d", atoi (PQgetvalue (res, i, 7)),
                               atoi (PQgetvalue (res, i, 8)), atoi (PQgetvalue (res, i, 9))),
                              -1);
        }

      gtk_tree_store_append (compra->store_facturas, &factura_iter, &fecha_iter);
      gtk_tree_store_set (compra->store_facturas, &factura_iter,
                          0, PQgetvalue (res, i, 11),
                          1, PQgetvalue (res, i, 12),
                          2, g_strdup_printf ("F. %s", PQgetvalue (res, i, 1)),
                          3, strcmp (PQgetvalue (res, i, 6), "0") == 0 ? "" : PQgetvalue (res, i, 6),
                          4, g_strdup_printf
                          ("%.2d/%.2d/%.4d", atoi (PQgetvalue (res, i, 3)),
                           atoi (PQgetvalue (res, i, 4)), atoi (PQgetvalue (res, i, 5))),
                          5, atoi (PQgetvalue (res, i, 10)) != -1 ? g_strdup_printf
                          ("%.2d/%.2d/%.4d", atoi (PQgetvalue (res, i, 7)), atoi (PQgetvalue (res, i, 8)),
                           atoi (PQgetvalue (res, i, 9))) : "Contado",
                          6, PutPoints (PQgetvalue (res, i, 2)),
                          -1);

      monto_fecha += atoi (PQgetvalue (res, i, 2));


      res2 = EjecutarSQL
        (g_strdup_printf
         ("SELECT numero, id_compra, id, date_part ('day', fecha_emicion), "
          "date_part ('month', fecha_emicion), date_part ('year', fecha_emicion), rut_proveedor FROM "
          "guias_compra WHERE id_factura=%s", PQgetvalue (res, i, 0)));

      tuples2 = PQntuples (res2);

      if (tuples2 != 0)
        {
          for (j = 0; j < tuples2; j++)
            {
              gtk_tree_store_append (compra->store_facturas, &guia_iter, &factura_iter);
              gtk_tree_store_set (compra->store_facturas, &guia_iter,
                                  0, PQgetvalue (res2, j, 0),
                                  1, PQgetvalue (res2, j, 6),
                                  2, g_strdup_printf ("Guia %s", PQgetvalue (res2, j, 0)),
                                  3, PQgetvalue (res2, j, 1),
                                  4, g_strdup_printf
                                  ("%.2d/%.2d/%.4d", atoi (PQgetvalue (res2, j, 3)),
                                   atoi (PQgetvalue (res2, j, 4)), atoi (PQgetvalue (res2, j, 5))),
                                  -1);
            }
        }
    }
  /*
   * No sacar este trozo de codigo
   */
  if (gtk_tree_store_iter_is_valid (compra->store_facturas, &fecha_iter) != FALSE)
    {
      gtk_tree_store_set (compra->store_facturas, &fecha_iter,
                          6, PutPoints (g_strdup_printf ("%d", monto_fecha)),
                          -1);
      monto_fecha = 0;
    }
}

void
FillGuias (gchar *proveedor)
{
  gint tuples, i;

  GtkTreeIter iter;

  PGresult *res;

  res = EjecutarSQL (g_strdup_printf ("SELECT t1.numero, t1.id_compra, date_part ('day', t1.fecha_emicion), date_part ('month', t1.fecha_emicion), date_part ('year', t1.fecha_emicion), (SELECT SUM (t2.cantidad * t2.precio) FROM documentos_detalle AS t2 WHERE t2.numero=t1.numero AND t2.id_compra=t1.id_compra), (SELECT nombre FROM formas_pago WHERE id=(SELECT forma_pago FROM compra WHERE id=t1.id_compra)) FROM guias_compra AS t1, products_buy_history AS t3 WHERE t1.rut_proveedor='%s' AND t3.id_compra=t1.id_compra AND t1.id_factura=0 GROUP BY t1.numero, t1.id_compra, t1.fecha_emicion", proveedor));

  tuples = PQntuples (res);

  gtk_tree_store_clear (compra->store_guias);

  for (i = 0; i < tuples; i++)
    {
      gtk_tree_store_append (compra->store_guias, &iter, NULL);
      gtk_tree_store_set (compra->store_guias, &iter,
                          0, PQgetvalue (res, i, 0),
                          1, PQgetvalue (res, i, 1),
                          2, PQgetvalue (res, i, 6),
                          3, g_strdup_printf
                          ("%.2d/%.2d/%s", atoi (PQgetvalue (res, i, 2)),
                           atoi (PQgetvalue (res, i, 3)), PQgetvalue (res, i, 4)),
                          4, PutPoints (PQgetvalue (res, i, 5)),
                          5, CheckCompraIntegrity (PQgetvalue (res, i, 1)) ? "Black" : "Red",
                          6, TRUE,
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
        (g_strdup_printf
         ("SELECT t1.codigo, t1.descripcion,  t2.cantidad, t2.precio, t2.cantidad * t2.precio AS "
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
    }

}

void
AddGuia (GtkWidget *widget, gpointer data)
{
  GtkTreeIter iter, iter2, iter3;
  gchar *guia_add;
  gchar *guia_first;
  gchar *rut = g_strdup (gtk_label_get_text (GTK_LABEL (compra->fact_rut)));
  gchar *fact = g_strdup (gtk_entry_get_text (GTK_ENTRY (compra->n_factura)));
  PGresult *res;

  if (gtk_tree_selection_get_selected
      (gtk_tree_view_get_selection (GTK_TREE_VIEW (compra->tree_guias)), NULL, &iter) == TRUE &&
      strcmp (fact, "") != 0)
    {
      gtk_tree_model_get (GTK_TREE_MODEL (compra->store_guias), &iter,
                          0, &guia_add,
                          -1);

      if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (compra->store_new_guias), &iter3) == TRUE)
        {
          gtk_tree_model_get (GTK_TREE_MODEL (compra->store_new_guias), &iter3,
                              0, &guia_first,
                              -1);


          res = EjecutarSQL
            (g_strdup_printf
             ("SELECT id_compra=(SELECT id_compra FROM guias_compra WHERE numero=%s AND rut_proveedor='%s') FROM guias_compra WHERE numero=%s AND rut_proveedor='%s'", guia_first, rut, guia_add, rut));

          if (strcmp (PQgetvalue (res, 0, 0), "t") != 0)
            {
              gtk_label_set_markup
                (GTK_LABEL (compra->guias_error),
                 "<span foreground=\"red\">No se pueden ingresar guias de distinta compra</span>");
              return;
            }

        }

      gtk_tree_store_append (compra->store_new_guias, &iter2, NULL);
      gtk_tree_store_set (compra->store_new_guias, &iter2,
                          0, guia_add,
                          -1);

      gtk_tree_store_remove (compra->store_guias, &iter);
      gtk_tree_store_clear (compra->store_det_guias);

      CalcularTotalesGuias ();
    }
}

void
DelGuia (GtkWidget *widget, gpointer data)
{
  GtkTreeIter iter;
  gchar *string;
  PGresult *res;
  gchar *rut_proveedor = g_strdup (gtk_label_get_text (GTK_LABEL (compra->fact_rut)));

  if (gtk_tree_selection_get_selected (gtk_tree_view_get_selection
                                       (GTK_TREE_VIEW (compra->tree_new_guias)), NULL, &iter) == TRUE)
    {
      gtk_tree_model_get (GTK_TREE_MODEL (compra->store_new_guias), &iter,
                          0, &string,
                          -1);

      gtk_tree_store_remove (compra->store_new_guias, &iter);

      res = EjecutarSQL (g_strdup_printf ("SELECT t1.numero, t1.id_compra, date_part ('day', t1.fecha_emicion), date_part ('month', t1.fecha_emicion), date_part ('year', t1.fecha_emicion), (SELECT SUM (t2.cantidad * t2.precio) FROM documentos_detalle AS t2 WHERE t2.numero=t1.numero AND t2.id_compra=t1.id_compra), (SELECT nombre FROM formas_pago WHERE id=(SELECT forma_pago FROM compra WHERE id=t1.id_compra)) FROM guias_compra AS t1 WHERE numero=%s AND rut_proveedor='%s'", string, rut_proveedor));

      gtk_tree_store_append (compra->store_guias, &iter, NULL);
      gtk_tree_store_set (compra->store_guias, &iter,
                          0, PQgetvalue (res, 0, 0),
                          1, PQgetvalue (res, 0, 1),
                          2, PQgetvalue (res, 0, 6),
                          3, g_strdup_printf ("%.2d/%.2d/%s", atoi (PQgetvalue (res, 0, 2)),
                                              atoi (PQgetvalue (res, 0, 3)), PQgetvalue (res, 0, 4)),
                          4, PQgetvalue (res, 0, 5),
                          5, CheckCompraIntegrity (PQgetvalue (res, 0, 1)) ? "Black" : "Red",
                          6, TRUE,
                          -1);

      CalcularTotalesGuias ();
    }
}

void
CloseSelectProveedor (GtkWidget *widget, gpointer data)
{
  gboolean cancel = (gboolean) data;
  gtk_widget_destroy (compra->win_proveedor);

  compra->win_proveedor = NULL;

  gtk_widget_set_sensitive (main_window, TRUE);

  if (cancel == TRUE)
    {
      if (guias == TRUE)
        gtk_window_set_focus (GTK_WINDOW (main_window), compra->fact_proveedor);
      else
        gtk_window_set_focus (GTK_WINDOW (main_window), pago_proveedor);
    }
}

void
SelectProveedor (GtkWidget *widget, gpointer data)
{
  GtkWidget *proveedor;

  GtkWidget *scroll;
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;

  GtkWidget *vbox;
  GtkWidget *hbox;

  GtkWidget *button;
  GtkWidget *label;

  guias = (gboolean) data;

  gtk_widget_set_sensitive (main_window, FALSE);

  compra->win_proveedor = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (compra->win_proveedor), "Seleccionar Proveedor");
  gtk_window_set_position (GTK_WINDOW (compra->win_proveedor), GTK_WIN_POS_CENTER_ALWAYS);
  //  gtk_window_set_transient_for (GTK_WINDOW (compra->win_proveedor), GTK_WINDOW (main_window));
  gtk_window_present (GTK_WINDOW (compra->win_proveedor));
  gtk_widget_show (compra->win_proveedor);

  g_signal_connect (G_OBJECT (compra->win_proveedor), "destroy",
                    G_CALLBACK (CloseSelectProveedor), (gpointer)TRUE);

  vbox = gtk_vbox_new (FALSE, 3);
  gtk_container_add (GTK_CONTAINER (compra->win_proveedor), vbox);
  gtk_widget_show (vbox);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  label = gtk_label_new ("Buscar: ");
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
  gtk_widget_show (label);

  proveedor = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (hbox), proveedor, FALSE, FALSE, 3);
  gtk_widget_show (proveedor);

  g_signal_connect (G_OBJECT (proveedor), "activate",
                    G_CALLBACK (FoundProveedor), (gpointer) proveedor);

  gtk_window_set_focus (GTK_WINDOW (compra->win_proveedor), proveedor);

  button = gtk_button_new_from_stock (GTK_STOCK_FIND);
  gtk_box_pack_end (GTK_BOX (hbox), button,  FALSE, FALSE, 3);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (FoundProveedor), (gpointer) proveedor);

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

  g_signal_connect (G_OBJECT (compra->tree_prov), "row-activated",
                    G_CALLBACK (FillProveedorData), data);

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

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_end (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  button = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 3);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (CloseSelectProveedor), (gpointer)TRUE);

  button = gtk_button_new_from_stock (GTK_STOCK_OK);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (FillProveedorData), data);

  FoundProveedor (NULL, widget);
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
FillProveedorData (GtkWidget *widget, gpointer data)
{
  PGresult *res;

  GtkTreeIter iter;
  gchar *rut;

  if (gtk_tree_selection_get_selected
      (gtk_tree_view_get_selection (GTK_TREE_VIEW (compra->tree_prov)), NULL, &iter) == TRUE)
    {
      gtk_tree_model_get (GTK_TREE_MODEL (compra->store_prov), &iter,
                          0, &rut,
                          -1);

      res = EjecutarSQL (g_strdup_printf ("SELECT * FROM proveedor WHERE rut='%s'", rut));

      if (guias == TRUE)
        {
          ClearFactData ();

          gtk_entry_set_text (GTK_ENTRY (compra->fact_proveedor), PQvaluebycol (res, 0, "nombre"));

          gtk_label_set_markup (GTK_LABEL (compra->fact_rut),
                                g_strdup_printf ("<span weight=\"ultrabold\">%s</span>", rut));

          gtk_label_set_markup (GTK_LABEL (compra->fact_contacto),
                                g_strdup_printf ("<span weight=\"ultrabold\">%s</span>",
                                                 PQvaluebycol (res, 0, "contacto")));

          gtk_label_set_markup (GTK_LABEL (compra->fact_direccion),
                                g_strdup_printf ("<span weight=\"ultrabold\">%s</span>",
                                                 PQvaluebycol (res, 0, "direccion")));

          gtk_label_set_markup (GTK_LABEL (compra->fact_comuna),
                                g_strdup_printf ("<span weight=\"ultrabold\">%s</span>",
                                                 PQvaluebycol (res, 0, "comuna")));

          gtk_label_set_markup (GTK_LABEL (compra->fact_fono),
                                g_strdup_printf ("<span weight=\"ultrabold\">%s</span>",
                                                 PQvaluebycol (res, 0, "telefono")));

          gtk_label_set_markup (GTK_LABEL (compra->fact_email),
                                g_strdup_printf ("<span weight=\"ultrabold\">%s</span>",
                                                 PQvaluebycol (res, 0, "email")));

          gtk_label_set_markup (GTK_LABEL (compra->fact_web),
                                g_strdup_printf ("<span weight=\"ultrabold\">%s</span>",
                                                 PQvaluebycol (res, 0, "web")));

          FillGuias (rut);

          gtk_widget_set_sensitive (add_guia, TRUE);
          gtk_widget_set_sensitive (del_guia, TRUE);
        }
      else if (guias == FALSE)
        {
          ClearPagosData ();

          gtk_entry_set_text (GTK_ENTRY (pago_proveedor), PQvaluebycol (res, 0, "nombre"));

          gtk_label_set_markup (GTK_LABEL (pago_rut),
                                g_strdup_printf ("<span weight=\"ultrabold\">%s</span>", rut));

          gtk_label_set_markup (GTK_LABEL (pago_contacto),
                                g_strdup_printf ("<span weight=\"ultrabold\">%s</span>",
                                                 PQvaluebycol (res, 0, "contacto")));

          gtk_label_set_markup (GTK_LABEL (pago_direccion),
                                g_strdup_printf ("<span weight=\"ultrabold\">%s</span>",
                                                 PQvaluebycol (res, 0, "direccion")));

          gtk_label_set_markup (GTK_LABEL (pago_comuna),
                                g_strdup_printf ("<span weight=\"ultrabold\">%s</span>",
                                                 PQvaluebycol (res, 0, "comuna")));

          gtk_label_set_markup (GTK_LABEL (pago_fono),
                                g_strdup_printf ("<span weight=\"ultrabold\">%s</span>",
                                                 PQvaluebycol (res, 0, "telefono")));

          gtk_label_set_markup (GTK_LABEL (pago_email),
                                g_strdup_printf ("<span weight=\"ultrabold\">%s</span>",
                                                 PQvaluebycol (res, 0, "email")));

          gtk_label_set_markup (GTK_LABEL (pago_web),
                                g_strdup_printf ("<span weight=\"ultrabold\">%s</span>",
                                                 PQvaluebycol (res, 0, "web")));

          FillPagarFacturas (rut);

        }

      CloseSelectProveedor (NULL, (gpointer)FALSE);

      if (guias == TRUE)
        gtk_window_set_focus (GTK_WINDOW (main_window), compra->n_factura);
      else
        gtk_window_set_focus (GTK_WINDOW (main_window), compra->tree_facturas);
    }
}

void
AddFactura (void)
{
  PGresult *res;
  GtkTreeIter iter;
  gchar *guia;
  gint factura;
  gchar *rut = g_strdup (gtk_label_get_text (GTK_LABEL (compra->fact_rut)));
  gchar *n_fact = g_strdup (gtk_entry_get_text (GTK_ENTRY (compra->n_factura)));
  gchar *fecha_y = g_strdup (gtk_entry_get_text (GTK_ENTRY (compra->fecha_y)));
  gchar *fecha_m = g_strdup (gtk_entry_get_text (GTK_ENTRY (compra->fecha_m)));
  gchar *fecha_d = g_strdup (gtk_entry_get_text (GTK_ENTRY (compra->fecha_d)));
  gchar *monto = g_strdup (gtk_entry_get_text (GTK_ENTRY (compra->fact_monto)));

  gtk_tree_model_get_iter_first (GTK_TREE_MODEL (compra->store_new_guias), &iter);

  gtk_tree_model_get (GTK_TREE_MODEL (compra->store_new_guias), &iter,
                      0, &guia,
                      -1);

  res = EjecutarSQL (g_strdup_printf
                     ("SELECT date_part('day', fecha), date_part('month', fecha), "
                      "date_part('year', fecha) FROM compra WHERE id=(SELECT id_compra FROM "
                      "guias_compra WHERE numero=%s AND rut_proveedor='%s')", guia, rut));

  if (res == NULL || PQntuples (res) == 0)
    return;

  if (atoi (fecha_y) < (atoi (PQgetvalue (res, 0, 2)) - 2000))
    {
      ErrorMSG (compra->fecha_y, "La fecha de emision del documento no puede ser menor a\nla fecha de compra");
      return;
    }
  else
    {
      if (atoi (fecha_m) < atoi (PQgetvalue (res, 0, 1)))
        {
          ErrorMSG (compra->fecha_m, "La fecha de emision del documento no puede ser menor a\nla fecha de compra");
          return;
        }
      else if (atoi (fecha_m) == atoi (PQgetvalue (res, 0, 1)))
        {
          if (atoi (fecha_d) < atoi (PQgetvalue (res, 0, 0)))
            {
              ErrorMSG (compra->fecha_d, "La fecha de emision del documento no puede ser menor a\nla fecha de compra");
              return;
            }
        }
    }


  if (strcmp (rut, "") == 0)
    {
      ErrorMSG (compra->tree_prov, "Debe Seleccionar un proveedor");
      return;
    }
  else if (strcmp (n_fact, "") == 0)
    {
      ErrorMSG (compra->n_factura, "Debe Ingresar el numero de la factura");
      return;
    }
  else if (strcmp (fecha_y, "") == 0)
    {
      ErrorMSG (compra->fecha_y, "Debe Obligatoriamente ingresar el a��±o de emision");
      return;
    }
  else if (strcmp (fecha_m, "") == 0)
    {
      ErrorMSG (compra->fecha_m, "Debe Obligatoriamente ingresar el mes de emision");
      return;
    }
  else if (strcmp (fecha_d, "") == 0)
    {
      ErrorMSG (compra->fecha_d, "Debe Obligatoriamente ingresar el dia de la emision");
      return;
    }
  else if (strcmp (monto, "") == 0)
    {
      ErrorMSG (compra->fact_monto, "Debe Ingresar el Monto de la Factura");
      return;
    }

  factura = IngresarFactura (n_fact, 0, rut, atoi (monto), fecha_d, fecha_m, fecha_y, atoi (guia));


  gtk_tree_model_get_iter_first (GTK_TREE_MODEL (compra->store_new_guias), &iter);

  do {
    gtk_tree_model_get (GTK_TREE_MODEL (compra->store_new_guias), &iter,
                        0, &guia,
                        -1);

    AsignarFactAGuia (atoi (guia), factura);
  }  while ((gtk_tree_model_iter_next (GTK_TREE_MODEL (compra->store_new_guias), &iter)) != FALSE);

  ClearFactData ();

}

void
ClearFactData (void)
{
  gtk_widget_set_sensitive (add_guia, FALSE);
  gtk_widget_set_sensitive (del_guia, FALSE);
  gtk_widget_set_sensitive (ok_guia, FALSE);

  gtk_label_set_text (GTK_LABEL (compra->guias_error), "");

  gtk_entry_set_text (GTK_ENTRY (compra->fact_proveedor), "");

  gtk_label_set_text (GTK_LABEL (compra->fact_rut), "");
  gtk_label_set_text (GTK_LABEL (compra->fact_direccion), "");
  gtk_label_set_text (GTK_LABEL (compra->fact_comuna), "");
  gtk_label_set_text (GTK_LABEL (compra->fact_fono), "");
  gtk_label_set_text (GTK_LABEL (compra->fact_email), "");
  gtk_label_set_text (GTK_LABEL (compra->fact_web), "");
  gtk_label_set_text (GTK_LABEL (compra->fact_contacto), "");

  gtk_entry_set_text (GTK_ENTRY (compra->n_factura), "");

  gtk_entry_set_text (GTK_ENTRY (compra->fecha_y), "");
  gtk_entry_set_text (GTK_ENTRY (compra->fecha_m), "");
  gtk_entry_set_text (GTK_ENTRY (compra->fecha_d), "");
  gtk_entry_set_text (GTK_ENTRY (compra->fact_monto), "");

  gtk_tree_store_clear (compra->store_new_guias);
  gtk_tree_store_clear (compra->store_guias);
  gtk_tree_store_clear (compra->store_det_guias);

  gtk_label_set_text (GTK_LABEL (compra->fact_neto), "");
  gtk_label_set_text (GTK_LABEL (compra->fact_iva), "");
  gtk_label_set_text (GTK_LABEL (compra->fact_otros), "");
  gtk_label_set_text (GTK_LABEL (compra->fact_total), "");

  //  gtk_window_set_focus (GTK_WINDOW (main_window), compra->fact_proveedor);
}

void
RemoveBuyProduct (void)
{
  GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (compra->tree_list));
  GtkTreeIter iter;
  gchar *product;

  if (gtk_tree_selection_get_selected (selection, NULL, &iter) == TRUE)
    {
      gtk_tree_model_get (GTK_TREE_MODEL (compra->store_list), &iter,
                          0, &product,
                          -1);

      DropBuyProduct (product);

      gtk_list_store_clear (compra->store_list);

      if (compra->header_compra != NULL)
        {
          compra->products_compra = compra->header_compra;

          do {
            compra->current = compra->products_compra->product;

            AddToTree ();

            compra->products_compra = compra->products_compra->next;
          } while (compra->products_compra != compra->header_compra);

          gtk_widget_set_sensitive (confirm_button, TRUE);

        }

    }
}

void
CalcularTotales (void)
{
  Productos *products = compra->header;

  gdouble total_neto = 0, total_iva = 0, total_otros = 0, total = 0;
  gdouble iva, otros;

  do {

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
  //  gint tuples, i;
  gchar *guia;
  PGresult *res;
  gchar *rut_proveedor = g_strdup (gtk_label_get_text (GTK_LABEL (compra->fact_rut)));

  GtkTreeIter iter;

  if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (compra->store_new_guias), &iter) == TRUE)
    {
      gtk_tree_model_get (GTK_TREE_MODEL (compra->store_new_guias), &iter,
                          0, &guia,
                          -1);

      while (1)
        {
          res = EjecutarSQL (g_strdup_printf ("SELECT SUM (t1.precio * t2.cantidad) AS neto, SUM (t2.iva) AS iva, SUM (t2.otros) AS otros, SUM ((t1.precio * t2.cantidad) + t2.iva + t2.otros) AS total  FROM compra_detalle AS t1, documentos_detalle AS t2 WHERE t1.id_compra=(SELECT id_compra FROM guias_compra WHERE numero=%s AND rut_proveedor='%s') AND t2.numero=%s AND t1.barcode_product=t2.barcode",
                                              guia, rut_proveedor, guia));

          total_neto += atoi (PQgetvalue (res, 0, 0));

          total_iva += atoi (PQgetvalue (res, 0, 1));

          total_otros += atoi (PQgetvalue (res, 0, 2));

          if (gtk_tree_model_iter_next (GTK_TREE_MODEL (compra->store_new_guias), &iter) != TRUE)
            break;
          else
            gtk_tree_model_get (GTK_TREE_MODEL (compra->store_new_guias), &iter,
                                0, &guia,
                                -1);

        }

      gtk_label_set_text (GTK_LABEL (compra->fact_neto),
                          PutPoints (g_strdup_printf ("%d", total_neto)));

      gtk_label_set_text (GTK_LABEL (compra->fact_iva),
                          PutPoints (g_strdup_printf ("%d", total_iva)));

      gtk_label_set_text (GTK_LABEL (compra->fact_otros),
                          PutPoints (g_strdup_printf ("%d", total_otros)));

      total = total_neto + total_iva + total_otros;

      gtk_label_set_text (GTK_LABEL (compra->fact_total),
                          PutPoints (g_strdup_printf ("%d", total)));


      CheckMontoGuias ();
    }
  else
    {
      gtk_label_set_text (GTK_LABEL (compra->fact_neto), "");

      gtk_label_set_text (GTK_LABEL (compra->fact_iva), "");

      gtk_label_set_text (GTK_LABEL (compra->fact_otros), "");

      gtk_label_set_text (GTK_LABEL (compra->fact_total), "");
    }
}


void
ToggleSelect (GtkRadioButton *button, gpointer data)
{
  gint var = atoi ((char *)data);

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)) == TRUE)
    {
      if (var == 1)
        iva = TRUE;
      else if (var == 2)
        perecible = TRUE;
      else if (var == 3)
        fraccion = TRUE;
    }
  else
    {
      if (var == 1)
        iva = FALSE;
      else if (var == 2)
        perecible = FALSE;
      else if (var == 3)
        fraccion = FALSE;
    }
}

void
CheckMontoGuias (void)
{
  gint total_guias = atoi (CutPoints (g_strdup (gtk_label_get_text
                                                (GTK_LABEL (compra->fact_total)))));
  gint total_fact = atoi (g_strdup (gtk_entry_get_text (GTK_ENTRY (compra->fact_monto))));

  if (total_guias == 0)
    return;

  if (total_guias < total_fact)
    {
      gtk_label_set_markup
        (GTK_LABEL (compra->guias_error),
         "<span foreground=\"red\">El monto total de la(s) guia(s) es insuficiente</span>");
      gtk_widget_set_sensitive (ok_guia, FALSE);
    }
  else if (total_guias > total_fact)
    {
      gtk_label_set_markup
        (GTK_LABEL (compra->guias_error),
         "<span foreground=\"red\">El Monto de la(s) guia(s) es mayor alde la factura</span>");
      gtk_widget_set_sensitive (ok_guia, FALSE);
    }
  else if (total_guias == total_fact)
    {
      gtk_label_set_markup (GTK_LABEL (compra->guias_error),
                            "");
      gtk_widget_set_sensitive (ok_guia, TRUE);

      SendCursorTo (ok_guia, (gpointer)ok_guia);
    }
}

void
CheckMontoIngreso (void)
{
  gint total = atoi (g_strdup (gtk_label_get_text (GTK_LABEL (compra->total))));
  gint total_doc = atoi (g_strdup (gtk_entry_get_text (GTK_ENTRY (compra->monto_documento))));

  if (total < total_doc)
    {
      gtk_label_set_markup
        (GTK_LABEL (compra->error_documento),
         "<span foreground=\"red\">El monto total de la(s) guia(s) es insuficiente</span>");
      gtk_widget_set_sensitive (ok_doc, FALSE);
    }
  else if (total > total_doc)
    {
      gtk_label_set_markup (GTK_LABEL (compra->error_documento),
                            "<span foreground=\"red\">El Monto de la(s) guia(s) es mayor alde la factura</span>");
      gtk_widget_set_sensitive (ok_doc, FALSE);
    }
  else if (total == total_doc)
    {
      gtk_label_set_markup (GTK_LABEL (compra->error_documento),
                            "");
      gtk_widget_set_sensitive (ok_doc, TRUE);

      SendCursorTo (ok_doc, (gpointer)ok_doc);
    }

}

void
CloseAskIngreso (GtkWidget *widget, gpointer data)
{
  gboolean answer = ((gboolean) data);

  if (GTK_WIDGET_TOPLEVEL (widget) == TRUE)
    return;

  if (answer == TRUE)
    {
      CheckCanjeables ();
    }
  else if (answer == FALSE)
    gtk_widget_set_sensitive (main_window, TRUE);

  gtk_widget_destroy (ask_window);

  ask_window = NULL;
}

void
AskIngreso (GtkWindow *win_mother)
{
  GtkWidget *label = NULL;
  GtkWidget *image;
  GtkWidget *button;

  GtkWidget *vbox;
  GtkWidget *hbox;

  gtk_widget_set_sensitive (main_window, FALSE);

  ask_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (ask_window), "Ingresar");
  gtk_window_set_position (GTK_WINDOW (ask_window), GTK_WIN_POS_CENTER_ALWAYS);
  //  gtk_window_set_transient_for (GTK_WINDOW (ask_window), win_mother);
  gtk_window_present (GTK_WINDOW (ask_window));
  gtk_widget_set_size_request (ask_window, -1, -1);
  gtk_widget_show (ask_window);

  g_signal_connect (G_OBJECT (ask_window), "destroy",
                    G_CALLBACK (CloseAskIngreso), NULL);

  //  gtk_widget_set_sensitive (GTK_WIDGET (win_mother), FALSE);

  vbox = gtk_vbox_new (FALSE, 3);
  gtk_container_add (GTK_CONTAINER (ask_window), vbox);
  gtk_widget_show (vbox);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  image = gtk_image_new_from_stock (GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_DIALOG);
  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 3);
  gtk_widget_show (image);

  if (ingreso_total == TRUE)
    label = gtk_label_new ("¿Desea ingresar totalidad de la compra?");
  else if (ingreso_total == FALSE)
    label = gtk_label_new ("¿Desea ingresar parcialmente otra mercadería?");
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
  gtk_widget_show (label);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  if (ingreso_total == TRUE)
    {
      button = gtk_button_new_from_stock (GTK_STOCK_NO);
      gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 3);
      gtk_widget_show (button);

      g_signal_connect (G_OBJECT (button), "clicked",
                        G_CALLBACK (CloseAskIngreso), ((gpointer)FALSE));

      button = gtk_button_new_from_stock (GTK_STOCK_YES);
      gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);
      gtk_widget_show (button);

      g_signal_connect (G_OBJECT (button), "clicked",
                        G_CALLBACK (CloseAskIngreso), (gpointer)TRUE);

      gtk_window_set_focus (GTK_WINDOW (ask_window), button);
    }
  else if (ingreso_total == FALSE)
    {
      button = gtk_button_new_from_stock (GTK_STOCK_YES);
      gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 3);
      gtk_widget_show (button);

      g_signal_connect (G_OBJECT (button), "clicked",
                        G_CALLBACK (CloseAskIngreso), ((gpointer)FALSE));

      button = gtk_button_new_from_stock (GTK_STOCK_NO);
      gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);
      gtk_widget_show (button);

      g_signal_connect (G_OBJECT (button), "clicked",
                        G_CALLBACK (CloseAskIngreso), (gpointer)TRUE);

      gtk_window_set_focus (GTK_WINDOW (ask_window), button);
    }

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
ElabVencCal (GtkToggleButton *widget, gpointer data)
{
  GtkWidget *vbox;
  GtkCalendar *calendar;
  GtkRequisition req;
  gint h, w;
  gint x, y;
  gint button_y, button_x;
  gboolean toggle = gtk_toggle_button_get_active (widget);
  //  gboolean elab = (gboolean) data;

  if (toggle == TRUE)
    {
      gdk_window_get_origin (GTK_WIDGET (widget)->window, &x, &y);

      gtk_widget_size_request (GTK_WIDGET (widget), &req);
      h = req.height;
      w = req.width;

      button_y = GTK_WIDGET (widget)->allocation.y;
      button_x = GTK_WIDGET (widget)->allocation.x;

      calendar_win = gtk_window_new (GTK_WINDOW_TOPLEVEL);
      gtk_window_set_screen (GTK_WINDOW (calendar_win),
                             gtk_widget_get_screen (GTK_WIDGET (widget)));

      gtk_container_set_border_width (GTK_CONTAINER (calendar_win), 5);
      gtk_window_set_type_hint (GTK_WINDOW (calendar_win), GDK_WINDOW_TYPE_HINT_DOCK);
      gtk_window_set_decorated (GTK_WINDOW (calendar_win), FALSE);
      gtk_window_set_resizable (GTK_WINDOW (calendar_win), FALSE);
      gtk_window_stick (GTK_WINDOW (calendar_win));
      gtk_window_set_title (GTK_WINDOW (calendar_win), "Calendario");

      vbox = gtk_vbox_new (FALSE, 3);
      gtk_container_add (GTK_CONTAINER (calendar_win), vbox);
      gtk_widget_show (vbox);

      calendar = GTK_CALENDAR (gtk_calendar_new ());
      gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (calendar), FALSE, FALSE, 0);
      gtk_widget_show (GTK_WIDGET (calendar));

      g_signal_connect (G_OBJECT (calendar), "day-selected-double-click",
                        G_CALLBACK (SetElabVencDate), (gpointer) widget);

      gtk_widget_show (calendar_win);

      x = (x + button_x);
      y = (y + button_y) + h;

      gtk_window_move (GTK_WINDOW (calendar_win), x, y);
      gtk_window_present (GTK_WINDOW (calendar_win));

    }
  else if (toggle == FALSE)
    gtk_widget_destroy (calendar_win);

}


void
CloseElabVenc (GtkWidget *widget, gpointer data)
{
  GtkWidget *window = (GtkWidget *)data;

  gtk_widget_destroy (window);
}

void
DrawAsk (Productos *products)
{
  GtkWidget *window;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *button;
  GtkWidget *image;

  compra->products_list = products;

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_position (GTK_WINDOW (window),
                           GTK_WIN_POS_CENTER_ALWAYS);
  gtk_window_set_title (GTK_WINDOW (window), "Elaboración y Vencimiento del Producto");
  gtk_window_present (GTK_WINDOW (window));
  //  gtk_window_set_transient_for (GTK_WINDOW (window), GTK_WINDOW (main_window));
  gtk_widget_show (window);

  g_signal_connect (G_OBJECT (window), "destroy",
                    G_CALLBACK (CloseElabVenc), (gpointer)window);

  vbox = gtk_vbox_new (FALSE, 3);
  gtk_container_add (GTK_CONTAINER (window), vbox);
  gtk_widget_show (vbox);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  image = gtk_image_new_from_stock (GTK_STOCK_DIALOG_QUESTION, GTK_ICON_SIZE_DIALOG);
  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 3);
  gtk_widget_show (image);

  label = gtk_label_new ("");
  gtk_label_set_markup (GTK_LABEL (label),
                        g_strdup_printf ("El Producto \"<b>%s</b>\" de marca \"<b>%s</b>\" es un\n"
                                         "producto perecible por lo que debe ingresar fecha de\n"
                                         "vencimiento",
                                         products->product->producto, products->product->marca));

  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
  gtk_widget_show (label);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  /*  button_elab = gtk_toggle_button_new_with_label ("\t\t");
      gtk_box_pack_start (GTK_BOX (hbox), button_elab, FALSE, FALSE, 3);
      gtk_widget_show (button_elab);

      g_signal_connect (G_OBJECT (button_elab), "toggled",
      G_CALLBACK (ElabVencCal), (gpointer)TRUE);
  */
  button_venc = gtk_toggle_button_new_with_label ("\t\t");
  gtk_box_pack_end (GTK_BOX (hbox), button_venc, FALSE, FALSE, 3);
  gtk_widget_show (button_venc);

  g_signal_connect (G_OBJECT (button_venc), "toggled",
                    G_CALLBACK (ElabVencCal), (gpointer)TRUE);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_end (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  button = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 3);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (CloseElabVenc), (gpointer)window);

  button = gtk_button_new_from_stock (GTK_STOCK_OK);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (SetElabVenc), (gpointer)products);

}

void
SetElabVenc (GtkWidget *widget, gpointer data)
{
  Productos *products = (Productos *) data;
  //  gchar *elab = g_strdup (gtk_button_get_label (GTK_BUTTON (button_elab)));
  gchar *venc = g_strdup (gtk_button_get_label (GTK_BUTTON (button_venc)));

  //  if (strcmp (elab, "\t\t") == 0 || strcmp (venc, "") == 0)
  if (strcmp (venc, "") == 0)
    ErrorMSG (button_venc, "Debe Ingresar una Fecha de Vencimiento");
  else
    {
      /*      products->product->elab_day = atoi (g_strdup_printf ("%c%c", elab[0], elab[1]));
              products->product->elab_month = atoi (g_strdup_printf ("%c%c", elab[3], elab[4]));
              products->product->elab_year = atoi (g_strdup_printf ("%c%c%c%c", elab[6], elab[7],
              elab[8], elab[9]));
      */
      products->product->venc_day = atoi (g_strdup_printf ("%c%c", venc[0], venc[1]));
      products->product->venc_month= atoi (g_strdup_printf ("%c%c", venc[3], venc[4]));
      products->product->venc_year = atoi (g_strdup_printf ("%c%c%c%c", venc[6], venc[7],
                                                            venc[8], venc[9]));


      gtk_widget_destroy (gtk_widget_get_toplevel (widget));

      if (products->next != compra->header)
        {
          products = products->next;

          while (products->product->perecible == FALSE && products->next != compra->header)
            products = products->next;

          DrawAsk (products);
        }
      else
        IngresarCompra ();
    }
}

void
AskElabVenc (GtkWidget *widget, gpointer data)
{
  Productos *products = (Productos *) data;
  GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (compra->ingreso_tree));
  GtkTreeIter iter;
  gint id;
  gchar *rut_proveedor;

  gtk_widget_set_sensitive (main_window, FALSE);

  if (gtk_tree_selection_get_selected (selection, NULL, &iter) == FALSE)
    return;

  gtk_tree_model_get (GTK_TREE_MODEL (compra->ingreso_store), &iter,
                      0, &id,
                      -1);

  rut_proveedor = GetDataByOne (g_strdup_printf ("SELECT rut_proveedor FROM compra WHERE id=%d", id));

  /*
    Las facturas y las guias ingresan en tablas distintas pero con estructura similar,
    sin embargo es mejor separar para posteriores cambios.
    CheckDocumentData recive un argumento como factura si es TRUE checkea datos en modo factura
    si es FALSE checkea datos en modo guia de despacho
  */
  if (CheckDocumentData (compra->factura, rut_proveedor, id) == FALSE) return;

  //  CloseDocumentoIngreso ();

  while (products->product->perecible == FALSE)
    {
      products = products->next;
      if (products == compra->header)
        break;
    }

  if (products->product->perecible == TRUE)
    DrawAsk (products);
  else
    IngresarCompra ();

  gtk_widget_set_sensitive (main_window, TRUE);
}

void
InformeCompras (GtkWidget *box)
{
  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *button;
  GtkWidget *scroll;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  Print *compras_print = (Print *) malloc (sizeof (Print));

  vbox = gtk_vbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (box), vbox, FALSE, FALSE, 3);
  gtk_widget_show (vbox);

  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_size_request (scroll, 350, 430);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (vbox), scroll, FALSE, FALSE, 3);
  gtk_widget_show (scroll);

  compra->informe_store = gtk_tree_store_new (5,
                                              G_TYPE_STRING,
                                              G_TYPE_STRING,
                                              G_TYPE_STRING,
                                              G_TYPE_STRING,
                                              G_TYPE_STRING);


  compra->informe_tree = gtk_tree_view_new_with_model (GTK_TREE_MODEL (compra->informe_store));
  gtk_container_add (GTK_CONTAINER (scroll), compra->informe_tree);
  gtk_widget_show (compra->informe_tree);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("ID", renderer,
                                                     "text", 0,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (compra->informe_tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_max_width (column, 50);
  gtk_tree_view_column_set_min_width (column, 50);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Proveedor", renderer,
                                                     "text", 1,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (compra->informe_tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_max_width (column, 250);
  gtk_tree_view_column_set_min_width (column, 250);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Rut", renderer,
                                                     "text", 2,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (compra->informe_tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_max_width (column, 80);
  gtk_tree_view_column_set_min_width (column, 80);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Cantidad", renderer,
                                                     "text", 3,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (compra->informe_tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Monto", renderer,
                                                     "text", 4,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (compra->informe_tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (box), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);


  button = gtk_button_new_from_stock (GTK_STOCK_PRINT);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);
  gtk_widget_show (button);

  compras_print->tree = GTK_TREE_VIEW (compra->informe_tree);
  compras_print->title = "Compras";
  compras_print->name = "compras";
  compras_print->date_string = NULL;
  compras_print->cols[0].name = "ID";
  compras_print->cols[0].num = 0;
  compras_print->cols[1].name = "Proveedor";
  compras_print->cols[1].num = 1;
  compras_print->cols[2].name = "Rut";
  compras_print->cols[2].num = 2;
  compras_print->cols[3].name = "Cantidad";;
  compras_print->cols[3].num = 3;
  compras_print->cols[4].name = "Monto";
  compras_print->cols[4].num = 4;
  compras_print->cols[5].name = NULL;

  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (PrintTree), (gpointer)compras_print);


}

void
InformeComprasShow (void)
{
  GtkTreeIter father;
  GtkTreeIter son;
  PGresult *res;
  PGresult *res2;
  gchar *q2;
  gint tuples, i;
  gint jtuples, j;

  if (ventastats->selected_from_day == ventastats->selected_to_day &&
      ventastats->selected_from_month == ventastats->selected_to_month &&
      ventastats->selected_from_year == ventastats->selected_to_year)
    res = EjecutarSQL //TODO: hay que arreglar estas funciones, pero
      //son demasiado toxicas xD
      (g_strdup_printf
       ("SELECT id, (SELECT nombre FROM proveedor WHERE rut=compras.rut_proveedor), rut_proveedor, "
        "(SELECT SUM (cantidad*precio) FROM compra_detalle WHERE id_compra=compras.id) FROM compra "
        "WHERE date_part ('day', fecha)=%d AND date_part ('month', fecha)=%d AND "
        "date_part ('year', fecha)=%d ORDER BY fecha DESC", ventastats->selected_from_day,
        ventastats->selected_from_month, ventastats->selected_from_year));
  else //TODO: arreglar esta sentencia que esta refea xD
    res = EjecutarSQL
      (g_strdup_printf
       ("SELECT id, (SELECT nombre FROM proveedor WHERE rut=compras.rut_proveedor), rut_proveedor, "
        "(SELECT SUM (cantidad*precio) FROM compra_detalle WHERE id_compra=compras.id) FROM compra "
        "WHERE fecha>=to_timestamp ('%.2d %.2d %.4d', 'DD MM YYYY') AND "
        "fecha<=to_timestamp ('%.2d %.2d %.4d', 'DD MM YYYY') ORDER BY fecha DESC",
        ventastats->selected_from_day, ventastats->selected_from_month, ventastats->selected_from_year,
        ventastats->selected_to_day, ventastats->selected_to_month, ventastats->selected_to_year));

  tuples = PQntuples (res);

  gtk_tree_store_clear (GTK_TREE_STORE (compra->informe_store));

  for (i = 0; i < tuples; i++)
    {
      gtk_tree_store_append (GTK_TREE_STORE (compra->informe_store), &father, NULL);
      gtk_tree_store_set (GTK_TREE_STORE (compra->informe_store), &father,
                          0, PQgetvalue (res, i, 0),
                          1, PQgetvalue (res, i, 1),
                          2, PQgetvalue (res, i, 2),
                          4, PQgetvalue (res, i, 3),
                          -1);
      q2 = g_strdup_printf ("SELECT producto.descripcion, cantidad, precio, "
                            "precio*cantidad as subtotal "
                            "FROM compra_detalle "
                            "INNER JOIN producto "
                            "ON producto.barcode = compra_detalle.barcode_product "
                            " AND id_compra = %s",
                            PQgetvalue (res, i, 0));
      res2 = EjecutarSQL (q2);
      g_free (q2);

      jtuples = PQntuples (res2);

      for (j = 0; j < jtuples; j++)
        {
          gtk_tree_store_append (GTK_TREE_STORE (compra->informe_store), &son, &father);
          gtk_tree_store_set (GTK_TREE_STORE (compra->informe_store), &son,
                              1, PQvaluebycol (res2, j, "producto.descripcion"),
                              2, PQvaluebycol (res2, j, "cantidad"),
                              3, PQvaluebycol (res2, j, "precio"),
                              4, PQvaluebycol (res2, j, "subtotal"),
                              -1);
        }

    }

}

void
ClosePagarDocumentoWin (GtkWidget *widget, gpointer data)
{
  GtkWidget *window = gtk_widget_get_toplevel (widget);

  gtk_widget_destroy (window);
  gtk_widget_set_sensitive (main_window, TRUE);
}

void
PagarToggle (GtkToggleButton *togglebutton, gpointer data)
{
  gboolean state = gtk_toggle_button_get_active (togglebutton);

  if (state == FALSE)
    {
      gtk_widget_set_sensitive (frame_cheque, FALSE);
      gtk_widget_set_sensitive (frame_otro, TRUE);
    }
  else
    {
      gtk_widget_set_sensitive (frame_cheque, TRUE);
      gtk_widget_set_sensitive (frame_otro, FALSE);
    }
}

void
PagarDocumentoWin (GtkWidget *widget, gpointer data)
{
  GtkWidget *window;
  GtkWidget *vbox;
  GtkWidget *vbox2;
  GtkWidget *hbox;
  //  GtkWidget *frame;
  GtkWidget *button;
  GtkWidget *button_ok;
  GtkWidget *label;

  GtkTreeIter iter;
  gchar *monto, *doc;

  if (gtk_tree_selection_get_selected (gtk_tree_view_get_selection (GTK_TREE_VIEW (compra->tree_facturas)), NULL, &iter) != TRUE)
    return;
  else
    {
      gtk_tree_model_get (GTK_TREE_MODEL (compra->store_facturas), &iter,
                          0, &doc,
                          6, &monto,
                          -1);

      if (doc == NULL || monto == NULL)
        return;
    }

  gtk_widget_set_sensitive (main_window, FALSE);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER_ALWAYS);
  gtk_window_set_title (GTK_WINDOW (window), "Pago con Documentos");
  gtk_widget_show (window);

  vbox = gtk_vbox_new (FALSE, 3);
  gtk_widget_show (vbox);
  gtk_container_add (GTK_CONTAINER (window), vbox);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_end (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  button_ok = gtk_button_new_from_stock (GTK_STOCK_OK);
  gtk_box_pack_end (GTK_BOX (hbox), button_ok, FALSE, FALSE, 3);
  gtk_widget_show (button_ok);

  button = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 3);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (ClosePagarDocumentoWin), NULL);

  g_signal_connect (G_OBJECT (window), "destroy",
                    G_CALLBACK (ClosePagarDocumentoWin),NULL);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  button = gtk_check_button_new_with_label ("Cheque");
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 3);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button_ok), "clicked",
                    G_CALLBACK (Pagar), (gpointer)button);

  g_signal_connect (G_OBJECT (button), "toggled",
                    G_CALLBACK (PagarToggle), NULL);

  frame_cheque = gtk_frame_new ("Cheque");
  gtk_box_pack_start (GTK_BOX (vbox), frame_cheque, FALSE, FALSE, 3);
  gtk_widget_show (frame_cheque);

  gtk_widget_set_sensitive (frame_cheque, FALSE);

  vbox2 = gtk_vbox_new (FALSE, 3);
  gtk_container_add (GTK_CONTAINER (frame_cheque), vbox2);
  gtk_widget_show (vbox2);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  label = gtk_label_new ("Banco");
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
  gtk_widget_show (label);

  pago_banco = gtk_entry_new ();
  gtk_box_pack_end (GTK_BOX (hbox), pago_banco, FALSE, FALSE, 3);

  gtk_widget_show (pago_banco);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  label = gtk_label_new ("Serie");
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
  gtk_widget_show (label);

  pago_serie = gtk_entry_new ();
  gtk_box_pack_end (GTK_BOX (hbox), pago_serie, FALSE, FALSE, 3);
  gtk_widget_show (pago_serie);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  label = gtk_label_new ("Vencimiento");
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
  gtk_widget_show (label);

  pago_fecha = gtk_entry_new ();
  gtk_box_pack_end (GTK_BOX (hbox), pago_fecha, FALSE, FALSE, 3);
  gtk_widget_show (pago_fecha);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  label = gtk_label_new ("Monto");
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
  gtk_widget_show (label);

  pago_monto = gtk_entry_new ();
  gtk_box_pack_end (GTK_BOX (hbox), pago_monto, FALSE, FALSE, 3);
  gtk_widget_show (pago_monto);

  frame_otro = gtk_frame_new ("Otro");
  gtk_box_pack_end (GTK_BOX (vbox), frame_otro, FALSE, FALSE, 3);
  gtk_widget_show (frame_otro);

  vbox2 = gtk_vbox_new (FALSE, 3);
  gtk_container_add (GTK_CONTAINER (frame_otro), vbox2);
  gtk_widget_show (vbox2);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  label = gtk_label_new ("Especifique el medio de pago");
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
  gtk_widget_show (label);

  pago_otro = gtk_entry_new ();
  gtk_box_pack_end (GTK_BOX (hbox), pago_otro, FALSE, FALSE, 3);
  gtk_widget_show (pago_otro);

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

  char *config_file;
  GKeyFile *key_file;
  gchar **profiles;

  config_file = g_strconcat(g_getenv("HOME"),"/.rizoma", NULL);

  if (!g_file_test(config_file, G_FILE_TEST_EXISTS|G_FILE_TEST_IS_REGULAR))
    {
      perror (g_strdup_printf ("Opening %s", config_file));
      g_print ("Your system must be configured, please run rizoma-config to do the job\n");
      exit (-1);
    }

  key_file = g_key_file_new ();
  g_key_file_load_from_file(key_file, config_file, G_KEY_FILE_KEEP_COMMENTS, NULL);

  gtk_init (&argc, &argv);

  builder = gtk_builder_new ();

  gtk_builder_add_from_file (builder, DATADIR"/ui/rizoma-login.ui", &err);
  if (err) {
    g_error ("ERROR: %s\n", err->message);
    return -1;
  }
  gtk_builder_connect_signals (builder, NULL);

  login_window = GTK_WINDOW(gtk_builder_get_object (builder, "login_window"));

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

  gtk_widget_show_all ((GtkWidget *)login_window);

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
on_buy_notebook_switch_page (GtkNotebook *notebook, GtkNotebookPage *page, guint page_num, gpointer user_data) {

  switch (page_num)
    {
    case 0:
      break;
    case 1:
      InsertarCompras ();
      break;
    /* case 2: */
    /*   ClearFactData (); */
    /*   break; */
    /* case 3: */
    /*   ClearPagosData (); */
    /*   FillPagarFacturas (NULL); */
    /*   break; */
    /* case 4: */
    /*   ReturnProductsStore (ingreso->store); */
    /*   break; */
    /* case 5: */
    /*   ListarProveedores (); */
    /*   break; */
    default:
      break;
    }
}

void
on_button_calculate_clicked (GtkButton *button, gpointer data) {
  CalcularPrecioFinal();
}

void
on_button_add_product_list_clicked (GtkButton *button, gpointer data) {
  AddToProductsList ();
}

void
on_button_new_product_clicked (GtkButton *button, gpointer data) {
  AddNewProduct ();
}

void
on_button_buy_clicked (GtkButton *button, gpointer data) {
  BuyWindow ();
}
