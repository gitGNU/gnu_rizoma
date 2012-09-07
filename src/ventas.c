/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4;
   c-indentation-style: gnu -*- */
/*ventas.c
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
#include<features.h>

#include<gtk/gtk.h>
#include<gdk/gdkkeysyms.h>

#include<math.h>
#include<stdlib.h>
#include<string.h>

#include<time.h>

#include"tipos.h"
#include"ventas.h"
#include"credito.h"
#include"postgres-functions.h"
#include"errors.h"
#include"manejo_productos.h"
#include"manejo_pagos.h"
#include"boleta.h"
#include"config_file.h"

#include"utils.h"
#include"encriptar.h"
#include"factura_more.h"
#include"rizoma_errors.h"
#include"proveedores.h"
#include"caja.h"
#include "vale.h"

GtkBuilder *builder;

GtkWidget *add_button;
GtkWidget *vuelto_button;

GtkWidget *calendar_window;
GtkWidget *button_cheque;

GtkWidget *buscador_window;

GtkWidget *label_found;

GtkWidget *tipos_window;

GtkWidget *canje_cantidad;

GtkWidget *window_seller;

gchar *tipo_venta;

gint monto_cheque = 0;

gboolean cheques = FALSE;

GtkWidget *canje_entry;

gboolean ventas = TRUE;

gint tipo_documento = -1;

gboolean mayorista = FALSE;

gboolean closing_tipos = FALSE;

gboolean block_discount = FALSE;


/**
 * Display the information of a product on the main sales window
 *
 * @param barcode the barcode of the product
 * @param mayorista TRUE if the product has a especial value for mayorist sales
 * @param marca the brand of the product
 * @param descripcion the description of the product
 * @param contenido the contents (amount) of the product
 * @param unidad the unit of the product (i.e. kg)
 * @param stock the current stock of the product
 * @param stock_day for how many days the current stock will be enough
 * @param precio the price of the product
 * @param precio_mayor the price of the producto for mayorist values
 * @param cantidad_mayor how many products must be saled to be considered as mayorist
 * @param codigo_corto the short code associated to the product
 */
void
FillProductSell (gchar *barcode,
                 gboolean mayorista,
                 gchar *marca,
                 gchar *descripcion,
                 gchar *contenido,
                 gchar *unidad,
                 gchar *stock,
                 gchar *stock_day,
                 gchar *precio,
                 gchar *precio_mayor,
                 gchar *cantidad_mayor,
                 gchar *codigo_corto)
{
  GtkWidget *widget;
  gchar *str_aux;

  //caja de producto
  widget = GTK_WIDGET(gtk_builder_get_object(builder, "barcode_entry"));
  gtk_entry_set_text(GTK_ENTRY(widget), barcode);

  gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "product_label")),
                      g_strdup_printf ("<span weight=\"ultrabold\" size=\"12000\">%s  %s  %s %s</span>", 
				       marca, descripcion, contenido, unidad));

  if (strtod (PUT (stock), (char **)NULL) <= GetMinStock (barcode))
    gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_stockday")),
                          g_strdup_printf("<span foreground=\"red\"><b>%.2f dia(s)</b></span>",
                                          strtod (PUT (stock_day), (char **)NULL)));
  else
    gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_stockday")),
                          g_strdup_printf ("<b>%.2f dia(s)</b>", strtod (PUT (stock_day), (char **)NULL)));

  //precio
  gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_precio")),
                        g_strdup_printf ("<span weight=\"ultrabold\" size=\"12000\">%s</span>",
                                         PutPoints (precio)));

  //precio de mayorista
  gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_mayor")),
                        g_strdup_printf ("<span weight=\"ultrabold\" size=\"12000\">%s</span>",
                                         PutPoints (precio_mayor)));

  gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_mayor_cantidad")),
                        g_strdup_printf ("<span weight=\"ultrabold\" size=\"12000\">%s</span>",
                                         PutPoints (cantidad_mayor)));

  gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_stock")),
                        g_strdup_printf ("<span weight=\"ultrabold\" size=\"12000\">%.2f</span>",
                                         strtod (PUT (stock), (char **)NULL)));

  str_aux = g_strdup(gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "cantidad_entry"))));
  str_aux = g_strdup_printf ("%.0f", strtod (PUT (str_aux), (char **)NULL) * atoi (precio));
  gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_subtotal")),
                        g_strdup_printf ("<span weight=\"ultrabold\" size=\"12000\">%s</span>", PutPoints(str_aux)));

  gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "codigo_corto")), 
			g_strdup_printf ("<span weight=\"ultrabold\" size=\"12000\">%s</span>", codigo_corto));
}

void
CanjearProducto (GtkWidget *widget, gpointer data)
{
  GtkWidget *entry = (GtkWidget *) data;

  if (entry == NULL)
    {
      gtk_widget_destroy (gtk_widget_get_toplevel (widget));

      gtk_widget_grab_focus (GTK_WIDGET (gtk_builder_get_object (builder, "barcode_entry")));
    }
  else
    {
      gchar *barcode = g_strdup (gtk_entry_get_text (GTK_ENTRY (canje_entry)));

      if ((GetDataByOne (g_strdup_printf ("SELECT barcode FROM producto WHERE barcode='%s'",
                                          barcode))) == NULL)
        {
          ErrorMSG (entry, "No existe el producto");
          return;
        }
      else
        {
          gdouble cantidad = strtod (PUT (g_strdup (gtk_entry_get_text (GTK_ENTRY (canje_cantidad)))),
                                     (char **)NULL);

          CanjearProduct (barcode, cantidad);

          CanjearProducto (widget, NULL);
        }
    }
}

void
CanjearProductoWin (GtkWidget *widget, gpointer data)
{
  GtkWidget *window;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *button;

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "Canjear Producto");
  gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER_ALWAYS);
  gtk_window_set_resizable (GTK_WINDOW (window), FALSE);
  gtk_window_present (GTK_WINDOW (window));
  gtk_widget_show (window);

  g_signal_connect (G_OBJECT (window), "destroy",
                    G_CALLBACK (CanjearProducto), NULL);

  vbox = gtk_vbox_new (FALSE, 3);
  gtk_container_add (GTK_CONTAINER (window), vbox);
  gtk_widget_show (vbox);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  label = gtk_label_new ("Código de Barras: ");
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
  gtk_widget_show (label);

  canje_entry = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (hbox), canje_entry, FALSE, FALSE, 3);
  gtk_widget_show (canje_entry);

  g_signal_connect (G_OBJECT (canje_entry), "activate",
                    G_CALLBACK (SearchSellProduct), (gpointer)FALSE);

  gtk_window_set_focus (GTK_WINDOW (window), canje_entry);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  label = gtk_label_new ("Cantidad: ");
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
  gtk_widget_show (label);

  canje_cantidad = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (hbox), canje_cantidad, FALSE, FALSE, 3);
  gtk_widget_show (canje_cantidad);

  g_signal_connect (G_OBJECT (canje_entry), "activate",
                    G_CALLBACK (SendCursorTo), (gpointer)canje_cantidad);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  button = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 3);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (CanjearProducto), NULL);

  button = gtk_button_new_from_stock (GTK_STOCK_OK);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 3);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (CanjearProducto), (gpointer)canje_entry);

  g_signal_connect (G_OBJECT (canje_cantidad), "activate",
                    G_CALLBACK (SendCursorTo), (gpointer)button);

}

void
SetChequeDate (GtkCalendar *calendar, gpointer data)
{
  GtkButton *button = (GtkButton *) data;
  guint day, month, year;

  time_t t;
  struct tm *current;

  if (calendar == NULL)
    {
      calendar = GTK_CALENDAR (gtk_calendar_new ());

      gtk_calendar_get_date (calendar, &year, &month, &day);

      gtk_button_set_label (button, g_strdup_printf ("%.2u/%.2u/%.4u", day, month+1, year));
    }
  else
    {
      time (&t);

      current = localtime (&t);

      gtk_calendar_get_date (calendar, &year, &month, &day);

      if (year >= (current->tm_year + 1900) && month >= current->tm_mon && day >= current->tm_mday)
        {
          gtk_button_set_label (button, g_strdup_printf ("%.2u/%.2u/%.4u", day, month+1, year));

          SetToggleMode (GTK_TOGGLE_BUTTON (data), NULL);
        }
    }
}

void
SelectChequeDate (GtkToggleButton *widget, gpointer data)
{
  GtkWidget *vbox;
  GtkCalendar *calendar;
  GtkRequisition req;
  gint h; //w;
  gint x, y;
  gint button_y, button_x;
  gboolean toggle = gtk_toggle_button_get_active (widget);

  if (toggle == TRUE)
    {
      gdk_window_get_origin (GTK_WIDGET (widget)->window, &x, &y);

      gtk_widget_size_request (GTK_WIDGET (widget), &req);
      h = req.height;
      //w = req.width;

      button_y = GTK_WIDGET (widget)->allocation.y;
      button_x = GTK_WIDGET (widget)->allocation.x;

      calendar_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
      gtk_window_set_screen (GTK_WINDOW (calendar_window),
                             gtk_widget_get_screen (GTK_WIDGET (widget)));

      gtk_container_set_border_width (GTK_CONTAINER (calendar_window), 5);
      gtk_window_set_type_hint (GTK_WINDOW (calendar_window), GDK_WINDOW_TYPE_HINT_DOCK);
      gtk_window_set_decorated (GTK_WINDOW (calendar_window), FALSE);
      gtk_window_set_resizable (GTK_WINDOW (calendar_window), FALSE);
      gtk_window_stick (GTK_WINDOW (calendar_window));
      gtk_window_set_title (GTK_WINDOW (calendar_window), "Calendario");

      vbox = gtk_vbox_new (FALSE, 3);
      gtk_container_add (GTK_CONTAINER (calendar_window), vbox);
      gtk_widget_show (vbox);

      calendar = GTK_CALENDAR (gtk_calendar_new ());
      gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (calendar), FALSE, FALSE, 0);
      gtk_widget_show (GTK_WIDGET (calendar));

      g_signal_connect (G_OBJECT (calendar), "day-selected-double-click",
                        G_CALLBACK (SetChequeDate), (gpointer) widget);

      gtk_widget_show (calendar_window);

      x = (x + button_x);
      y = (y + button_y) + h;

      gtk_window_move (GTK_WINDOW (calendar_window), x, y);
      gtk_window_present (GTK_WINDOW (calendar_window));

    }
  else if (toggle == FALSE)
    {
      gtk_widget_destroy (calendar_window);
    }
}

void
DatosCheque (void)
{
  GtkWidget *window;
  GtkWidget *label;
  GtkWidget *frame;
  GtkWidget *button;
  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *vbox2;

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "Ingreso de Cheque");
  gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER_ALWAYS);
  gtk_widget_show (window);
  gtk_window_present (GTK_WINDOW (window));
  gtk_window_set_resizable (GTK_WINDOW (window), FALSE);
  gtk_widget_show (window);

  vbox = gtk_vbox_new (FALSE, 3);
  gtk_container_add (GTK_CONTAINER (window), vbox);
  gtk_widget_show (vbox);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_end (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  button = gtk_button_new_with_mnemonic ("_Vender");
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);
  gtk_widget_show (button);

  frame = gtk_frame_new ("Datos Cheque");
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 3);
  gtk_widget_show (frame);

  vbox2 = gtk_vbox_new (FALSE, 3);
  gtk_container_add (GTK_CONTAINER (frame), vbox2);
  gtk_widget_show (vbox2);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  label = gtk_label_new ("Serie: ");
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
  gtk_widget_show (label);

  venta->cheque_serie = gtk_entry_new_with_max_length (10);
  gtk_box_pack_end (GTK_BOX (hbox), venta->cheque_serie, FALSE, FALSE, 3);
  gtk_widget_show (venta->cheque_serie);

  g_signal_connect (G_OBJECT (venta->venta_rut), "changed",
                    G_CALLBACK (SendCursorTo), (gpointer)venta->cheque_serie);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  label = gtk_label_new ("Número: ");
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
  gtk_widget_show (label);

  venta->cheque_numero = gtk_entry_new ();
  gtk_box_pack_end (GTK_BOX (hbox), venta->cheque_numero, FALSE, FALSE, 3);
  gtk_widget_show (venta->cheque_numero);

  g_signal_connect (G_OBJECT (venta->cheque_serie), "activate",
                    G_CALLBACK (SendCursorTo), (gpointer)venta->cheque_numero);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  label = gtk_label_new ("Banco: ");
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
  gtk_widget_show (label);

  venta->cheque_banco = gtk_entry_new_with_max_length (50);
  gtk_box_pack_end (GTK_BOX (hbox), venta->cheque_banco, FALSE, FALSE, 3);
  gtk_widget_show (venta->cheque_banco);

  g_signal_connect (G_OBJECT (venta->cheque_numero), "activate",
                    G_CALLBACK (SendCursorTo), (gpointer)venta->cheque_banco);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  label = gtk_label_new ("Plaza: ");
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
  gtk_widget_show (label);

  venta->cheque_plaza = gtk_entry_new_with_max_length (50);
  gtk_box_pack_end (GTK_BOX (hbox), venta->cheque_plaza, FALSE, FALSE, 3);
  gtk_widget_show (venta->cheque_plaza);

  g_signal_connect (G_OBJECT (venta->cheque_banco), "activate",
                    G_CALLBACK (SendCursorTo), (gpointer)venta->cheque_plaza);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  label = gtk_label_new ("Fecha: ");
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
  gtk_widget_show (label);

  button_cheque = gtk_toggle_button_new ();
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (button_cheque), FALSE);
  gtk_box_pack_end (GTK_BOX (hbox), button_cheque, FALSE, FALSE, 3);
  gtk_widget_show (button_cheque);

  g_signal_connect (G_OBJECT (venta->cheque_plaza), "activate",
                    G_CALLBACK (SendCursorTo), (gpointer)button_cheque);

  SetChequeDate (NULL, (gpointer)button_cheque);

  g_signal_connect (G_OBJECT (button_cheque), "toggled",
                    G_CALLBACK (SelectChequeDate), NULL);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  label = gtk_label_new ("Monto: ");
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
  gtk_widget_show (label);

  venta->cheque_monto = gtk_entry_new_with_max_length (50);
  gtk_box_pack_end (GTK_BOX (hbox), venta->cheque_monto, FALSE, FALSE, 3);
  gtk_widget_show (venta->cheque_monto);

  g_signal_connect (G_OBJECT (button_cheque), "toggled",
                    G_CALLBACK (SendCursorTo), (gpointer)venta->cheque_monto);

  g_signal_connect (G_OBJECT (venta->cheque_monto), "activate",
                    G_CALLBACK (SendCursorTo), (gpointer)button);

  venta->tipo_venta = CHEQUE;
}


void
CancelarTipo (GtkWidget *widget, gpointer data)
{

  if (closing_tipos == TRUE)
    return;

  closing_tipos = TRUE;

  gtk_widget_destroy (gtk_widget_get_toplevel (widget));

  if ((gboolean)data == TRUE)
    {
      //TiposVenta (NULL, NULL);
    }
  else
    {
      gtk_widget_grab_focus (GTK_WIDGET (gtk_builder_get_object (builder, "sencillo_entry")));

      venta->tipo_venta = SIMPLE;
    }
  closing_tipos = FALSE;
}

void
ventas_win ()
{
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;
  GtkTreeIter iter;
  GtkWidget *ventas_gui;
  GError *error = NULL;

  //Inicializacion de la estructura venta
  venta = (Venta *) g_malloc (sizeof (Venta));
  venta->header = NULL;
  venta->products = NULL;
  venta->window = NULL;
  Productos *fill = venta->header;

  //Inicialización de la estructura de cheques de restaurant
  pago_chk_rest = (PagoChequesRest *) g_malloc (sizeof (PagoChequesRest));
  pago_chk_rest->header = NULL;
  pago_chk_rest->cheques = NULL;
  //ChequesRestaurant *fill = pago_chk_rest->header;

  //Inicialización de pago mixto
  pago_mixto = (PagoMixto *) g_malloc (sizeof (PagoMixto));
  pago_mixto->check_rest1 = NULL;
  pago_mixto->check_rest2 = NULL;


  builder = gtk_builder_new ();

  gtk_builder_add_from_file (builder, DATADIR"/ui/rizoma-ventas.ui", &error);

  if (error != NULL)
    g_printerr ("%s: %s\n", G_STRFUNC, error->message);


  gtk_builder_add_from_file (builder, DATADIR"/ui/rizoma-common.ui", &error);

  if (error != NULL)
    g_printerr ("%s: %s\n", G_STRFUNC, error->message);

  gtk_builder_connect_signals (builder, NULL);

  // check if is enabled the print of invoices to show or not show the
  // make invoice button
  if (!(rizoma_get_value_boolean ("PRINT_FACTURA")))
    {
      GtkWidget *aux_widget;
      aux_widget = GTK_WIDGET(gtk_builder_get_object (builder, "btn_invoice"));
      gtk_widget_hide (aux_widget);
    }

  if (rizoma_get_value_boolean ("CAJA"))
    {
      gtk_widget_show (GTK_WIDGET (builder_get (builder, "btn_cash_box_close")));
    }


  if (rizoma_get_value_boolean ("TRASPASO"))
    {
      gtk_widget_show (GTK_WIDGET (builder_get (builder, "btn_traspaso_enviar")));
    }


  ventas_gui = GTK_WIDGET (gtk_builder_get_object (builder, "wnd_sell"));

  //Titulo
  gtk_window_set_title (GTK_WINDOW (ventas_gui), 
			g_strdup_printf ("POS Rizoma Comercio: Ventas - Conectado a [%s@%s]",
					 config_profile,
					 rizoma_get_value ("SERVER_HOST")));

  // check if the window must be set to fullscreen
  if (rizoma_get_value_boolean("FULLSCREEN"))
    gtk_window_maximize(GTK_WINDOW(ventas_gui));

  gtk_widget_show_all (ventas_gui);

  gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_seller_name")),
                        g_strdup_printf ("<span size=\"15000\">%s</span>", user_data->user));

  
  gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_ticket_number")),
                        g_strdup_printf ("<span size=\"15000\">%.6d</span>", get_last_sell_id ()));

  // El numerno de venta no se debe basar en el numero de ticket, puesto que aquel corresponde a los documentos que se han emitido
  /* gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_ticket_number")), */
  /*                       g_strdup_printf ("<b><big>%.6d</big></b>", get_ticket_number (SIMPLE))); */

  venta->store =  gtk_list_store_new (5,
                                      G_TYPE_STRING,
                                      G_TYPE_STRING,
				      G_TYPE_STRING,
				      G_TYPE_INT,
                                      G_TYPE_STRING);
  if (venta->header != NULL)
    {
      gint precio;
      do
        {

          if (fill->product->cantidad_mayorista > 0 && fill->product->precio_mayor > 0 && fill->product->cantidad > fill->product->cantidad_mayorista &&
              fill->product->mayorista == TRUE)
            precio = fill->product->precio_mayor;
          else
            precio = fill->product->precio;

          gtk_list_store_insert_after (venta->store, &iter, NULL);
          gtk_list_store_set (venta->store, &iter,
                              0, fill->product->codigo,
                              1, g_strdup_printf ("%s %s %d %s",
						  fill->product->producto,
						  fill->product->marca,
						  fill->product->contenido,
						  fill->product->unidad),
                              2, g_strdup_printf ("%.3f", fill->product->cantidad),
                              3, precio,
                              4, PutPoints (g_strdup_printf ("%.0f", fill->product->cantidad * precio)),
                              -1);
          fill = fill->next;
        }
      while (fill != venta->header);
    }

  venta->treeview_products = GTK_WIDGET (gtk_builder_get_object (builder, "sell_products_list"));
  gtk_tree_view_set_model (GTK_TREE_VIEW (venta->treeview_products), GTK_TREE_MODEL (venta->store));

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Código", renderer,
                                                     "text", 0,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (venta->treeview_products), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.0, "font", "15", NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Descripción", renderer,
                                                     "text", 1,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (venta->treeview_products), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.0, "font", "15", NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_min_width (column, 400);
  gtk_tree_view_column_set_expand (column, TRUE);

  /* renderer = gtk_cell_renderer_text_new (); */
  /* column = gtk_tree_view_column_new_with_attributes ("Marca", renderer, */
  /*                                                    "text", 2, */
  /*                                                    NULL); */
  /* gtk_tree_view_append_column (GTK_TREE_VIEW (venta->treeview_products), column); */
  /* gtk_tree_view_column_set_alignment (column, 0.5); */
  /* gtk_tree_view_column_set_resizable (column, FALSE); */

  /* renderer = gtk_cell_renderer_text_new (); */
  /* column = gtk_tree_view_column_new_with_attributes ("Cont.", renderer, */
  /*                                                    "text", 3, */
  /*                                                    NULL); */
  /* gtk_tree_view_append_column (GTK_TREE_VIEW (venta->treeview_products), column); */
  /* gtk_tree_view_column_set_alignment (column, 0.5); */
  /* gtk_tree_view_column_set_resizable (column, FALSE); */

  /* renderer = gtk_cell_renderer_text_new (); */
  /* column = gtk_tree_view_column_new_with_attributes ("Uni.", renderer, */
  /*                                                    "text", 4, */
  /*                                                    NULL); */
  /* gtk_tree_view_append_column (GTK_TREE_VIEW (venta->treeview_products), column); */
  /* g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL); */
  /* gtk_tree_view_column_set_resizable (column, FALSE); */

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Cant.", renderer,
                                                     "text", 2,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (venta->treeview_products), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, "font", "15", NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Precio Uni.", renderer,
                                                     "text", 3,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (venta->treeview_products), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, "font", "15", NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)3, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Sub Total", renderer,
                                                     "text", 4,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (venta->treeview_products), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, "font", "15", NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_max_width (column, 100);

  if (venta->header != NULL)
    CalcularVentas (venta->header);

  gtk_widget_grab_focus (GTK_WIDGET (gtk_builder_get_object (builder, "barcode_entry")));

  if (rizoma_get_value_boolean ("CAJA"))
    {
      if (check_caja())
        {
          open_caja (FALSE);
        }
      else
        {
          gtk_widget_show_all (GTK_WIDGET (builder_get (builder, "dialog_cash_box_opened")));
        }
    }

  /*conectando señales*/
  only_number_filer_on_container (GTK_CONTAINER (builder_get (builder, "wnd_mixed_pay_step1")));

  // Conectando la señal 'insert-text' para permitir solo números
  /* g_signal_connect (G_OBJECT (builder_get (builder, "entry_int_code_mixed_pay")), "insert-text", */
  /* 		    G_CALLBACK (only_numberi_filter), NULL); */

  /* g_signal_connect (G_OBJECT (builder_get (builder, "entry_int_amount_mixed_pay")), "insert-text", */
  /* 		    G_CALLBACK (only_numberi_filter), NULL); */

  /* g_signal_connect (G_OBJECT (builder_get (builder, "entry_int_amount_mixed_pay2")), "insert-text", */
  /* 		    G_CALLBACK (only_numberi_filter), NULL); */

  /* // TODO: crear (al igual que clean_container) una función que asigne este callback a todos los entry */
  /* // numéricos */
  /* g_signal_connect (G_OBJECT (builder_get (builder, "entry_int_code_detail_mp")), "insert-text", */
  /* 		    G_CALLBACK (only_numberi_filter), NULL); */

  /* g_signal_connect (G_OBJECT (builder_get (builder, "entry_int_exp_date_mp")), "insert-text", */
  /* 		    G_CALLBACK (only_numberi_filter), NULL); */

  /* g_signal_connect (G_OBJECT (builder_get (builder, "entry_int_amount_detail_mp")), "insert-text", */
  /* 		    G_CALLBACK (only_numberi_filter), NULL); */

  // Conectando la señal 'insert-text' para calcular diferencia con el monto total
  g_signal_connect (G_OBJECT (builder_get (builder, "entry_int_amount_mixed_pay")), "changed",
		    G_CALLBACK (calculate_amount), NULL);

  g_signal_connect (G_OBJECT (builder_get (builder, "entry_int_amount_mixed_pay2")), "changed",
		    G_CALLBACK (calculate_amount), NULL);

}

gboolean
AgregarProducto (GtkButton *button, gpointer data)
{
  gchar *codigo = g_strdup (gtk_label_get_text (GTK_LABEL (gtk_builder_get_object (builder, "codigo_corto"))));
  gchar *barcode = g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "barcode_entry"))));
  guint32 total;
  gint tipo;
  gdouble stock = GetCurrentStock (barcode);
  gdouble cantidad;
  GtkTreeIter iter;
  GtkWidget *aux_widget;

  cantidad = strtod (PUT(g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "cantidad_entry"))))), (char **) NULL);

  if (cantidad <= 0)
    {
      aux_widget = GTK_WIDGET(gtk_builder_get_object (builder, "cantidad_entry"));
      AlertMSG (aux_widget, "Debe ingresar una cantidad mayor a 0");
      return FALSE;
    }

  aux_widget = GTK_WIDGET (gtk_builder_get_object (builder, "label_precio"));
  if (g_str_equal ("0", CutPoints (g_strdup (gtk_label_get_text (GTK_LABEL(aux_widget))))))
    {
      AlertMSG (GTK_WIDGET (gtk_builder_get_object (builder, "barcode_entry")), "No se pueden vender productos con precio 0");
      CleanEntryAndLabelData ();
      return FALSE;
    }
  else if (cantidad > stock)
    {
      aux_widget = GTK_WIDGET (gtk_builder_get_object (builder, "cantidad_entry"));
      AlertMSG (aux_widget, "Stock insuficiente, debe ingresar una cantidad igual o menor al stock disponible");

      return FALSE;
    }
  else if (strchr (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "cantidad_entry"))), ',') != NULL ||
           strchr (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "cantidad_entry"))), '.') != NULL)
    {
      if (VentaFraccion (barcode) == FALSE)
        {
          aux_widget = GTK_WIDGET (gtk_builder_get_object (builder, "cantidad_entry"));
          AlertMSG (aux_widget,
                    "Este producto no se puede vender por fracción de unidad");

          return FALSE;
        }
    }

  if (!(g_str_equal ("", codigo)))
    {
      /*
        Nos aseguramos de tener un subtotal
        es decir la cantidad por el precio
      */
      AumentarCantidad (NULL, FALSE);

      /*
        Agregamos el producto a la lista enlazada circular
      */

      if (venta->products != NULL)
        venta->product_check = BuscarPorCodigo (venta->header, codigo);
      else
        venta->product_check = NULL;

      if (venta->product_check == NULL)
        {
          gint precio;

          AgregarALista (codigo, barcode, cantidad);

          if (venta->products->product->cantidad_mayorista > 0 && venta->products->product->precio_mayor > 0 && venta->products->product->cantidad >= venta->products->product->cantidad_mayorista &&
              venta->products->product->mayorista == TRUE)
            precio = venta->products->product->precio_mayor;
          else
            precio = venta->products->product->precio;

          /*
            Agregamos el producto al TreeView
          */
          gtk_list_store_insert_after (venta->store, &iter, NULL);
          gtk_list_store_set (venta->store, &iter,
                              0, venta->products->product->codigo,
                              1, g_strdup_printf ("%s %s %d %s",
						  venta->products->product->producto,
						  venta->products->product->marca,
						  venta->products->product->contenido,
						  venta->products->product->unidad),
                              2, g_strdup_printf ("%.3f", venta->products->product->cantidad),
                              3, precio,
                              4, PutPoints (g_strdup_printf
                                            ("%.0f", venta->products->product->cantidad * precio)),
                              -1);

          venta->products->product->iter = iter;
        }
      else
        {
          gint precio;

          if ((venta->product_check->product->cantidad + cantidad) > stock)
            {
              AlertMSG (GTK_WIDGET (gtk_builder_get_object (builder, "cantidad_entry")),
                        "No puede vender más productos de los que tiene en stock");

              return FALSE;
            }

          venta->product_check->product->cantidad += cantidad;

          if (venta->product_check->product->cantidad_mayorista > 0 && venta->product_check->product->precio_mayor > 0 && venta->product_check->product->cantidad >= venta->product_check->product->cantidad_mayorista &&
              venta->product_check->product->mayorista == TRUE)
            precio = venta->products->product->precio_mayor;
          else
            precio = venta->product_check->product->precio;


          gtk_list_store_set (venta->store, &venta->product_check->product->iter,
                              2, g_strdup_printf ("%.3f", venta->product_check->product->cantidad),
                              3, precio,
                              4, PutPoints (g_strdup_printf
                                            ("%.0f", venta->product_check->product->cantidad *
                                             precio)),
                              -1);

        }

      //Se obtiene el tipo de mercaderia
      tipo = atoi (PQgetvalue (EjecutarSQL (g_strdup_printf ("SELECT tipo FROM producto WHERE barcode = %s", barcode)), 0, 0));
      //Se agrega a los datos del producto a vender
      venta->products->product->tipo = tipo;

      /*
        Eliminamos los datos del producto en las entradas
      */

      CleanEntryAndLabelData ();

      /*
        Seteamos el Sub-Total y el Total
      */
      total = llround (CalcularTotal (venta->header));

      gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_total")),
                            g_strdup_printf ("<span size=\"40000\">%s</span>",
                                             PutPoints (g_strdup_printf ("%u", total))));

      //      gtk_window_set_focus( GTK_WINDOW( main_window ), GTK_WIDGET (gtk_builder_get_object (builder, "label_total")));
    }

  return TRUE;
}


/*
 * Es llamada por las funciones SearchSellProduct
 *
 * Esta funcion limpia las etiquetas (label) de los datos del producto
 *
 */

void
CleanSellLabels (void)
{
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "codigo_corto")), "");
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "product_label")), "");
  gtk_entry_set_text (GTK_ENTRY (gtk_builder_get_object (builder, "barcode_entry")), "");
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_stockday")), "");
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_precio")), "");
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_stock")), "");
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_subtotal")), "");
  //gtk_entry_set_text (GTK_ENTRY (gtk_builder_get_object (builder, "cantidad_entry")), "1");
}

/*
 * Es llamada por las funciones SearchSellProduct
 *
 * Esta funcion limpia las etiquetas (label) de los datos del producto
 *
 */

void
CleanEntryAndLabelData (void)
{
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "product_label")), "");
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "codigo_corto")), "");
  gtk_entry_set_text (GTK_ENTRY (gtk_builder_get_object (builder, "cantidad_entry")), "1");
  gtk_entry_set_text (GTK_ENTRY (gtk_builder_get_object (builder, "barcode_entry")), "");
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_stockday")), "");
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_mayor_cantidad")), "");
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_precio")), "");
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_mayor")), "");
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_subtotal")), "");
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_stock")), "");

  gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "sell_add_button")), FALSE);

  gtk_widget_grab_focus (GTK_WIDGET (gtk_builder_get_object (builder, "barcode_entry")));
}



/**
 * Es llamada cuando el boton "button_delete_product" es presionado (signal click).
 *
 * Esta funcion elimina el producto de la treeView y llamas a la funcion
 * EliminarDeLIsta() y CalcularVentas()
 *
 * @param button the GtkButton that recieves the signal
 * @param data the pointer passed to the callback
 */
void
EliminarProducto (GtkButton *button, gpointer data)
{
  GtkTreeIter iter;
  GtkTreeSelection *selection;
  gchar *value;
  gint position;

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (venta->treeview_products));

  if (gtk_tree_selection_get_selected (selection, NULL, &iter) == TRUE)
    {
      gtk_tree_model_get (GTK_TREE_MODEL (venta->store), &iter,
                          0, &value,
                          -1);

      position = atoi (gtk_tree_model_get_string_from_iter (GTK_TREE_MODEL (venta->store), &iter));

      EliminarDeLista (value, position);

      gtk_list_store_remove (GTK_LIST_STORE (venta->store), &iter);

      CalcularVentas (venta->header);
    }

  select_back_deleted_row("sell_products_list", position);

}

/**
 * Callback connected to the button present in
 * rizoma-ventas.ui:tipo_venta_win_venta, the button sell_button
 *
 * @param button the GtkButton that recieves the signal
 * @param data the pointer passed to the callback
 */
void
on_sell_button_clicked (GtkButton *button, gpointer data)
{
  gchar *rut = NULL;
  gchar *cheque_date = NULL;
  gint monto = atoi (CutPoints (g_strdup (gtk_label_get_text
                                          (GTK_LABEL (gtk_builder_get_object (builder, "label_total"))))));
  gchar *discount = "0";
  gint maquina = atoi (rizoma_get_value ("MAQUINA"));
  gint vendedor = user_data->user_id;
  gint paga_con;
  gint ticket = 0;
  gboolean canceled;

  // && rizoma_get_value_int ("VENDEDOR") != 1) se agrega en todos estos if mientras no se complete la
  // la funcionalidad de pre-venta

  if ((tipo_documento != VENTA && tipo_documento != FACTURA) && rizoma_get_value_int ("VENDEDOR") != 1)
    paga_con = atoi (g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "sencillo_entry")))));

  if ((tipo_documento != VENTA && paga_con <= 0) && rizoma_get_value_int ("VENDEDOR") != 1)
    {
      GtkWidget *aux;
      aux = GTK_WIDGET(gtk_builder_get_object(builder, "wnd_sale_type"));
      g_assert (aux != NULL);
      gtk_widget_show_all(aux);
      return;
    }

  if ((tipo_documento != VENTA && paga_con < monto) && rizoma_get_value_int ("VENDEDOR") != 1)
    {
      ErrorMSG (GTK_WIDGET (gtk_builder_get_object (builder, "sencillo_entry")), "No esta pagando con el dinero suficiente");
      return;
    }

  if ((tipo_documento != VENTA) && rizoma_get_value_int ("VENDEDOR") != 1)
    discount = g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_discount_money"))));

  if (strlen (discount) == 0 )
    discount = "0";

  switch (tipo_documento)
    {
    case SIMPLE: case VENTA:
      if (monto >= 180)
        ticket = get_ticket_number (tipo_documento);
      else
        ticket = -1;
      break;
      /* case VENTA: */
      /*   ticket = -1; */
      /*   break; */
    default:
      g_printerr("Could not be found the document type %s", G_STRFUNC);
    }

  //  DiscountStock (venta->header);


  //TODO: Se descomenta cuando se termine la funcionalidad de pre-venta
  /* if (tipo_documento == VENTA) */
  /*   canceled = FALSE; */
  /* else */
  canceled = TRUE;

  SaveSell (monto, maquina, vendedor, CASH, rut, discount, ticket, tipo_documento,
            cheque_date, cheques, canceled);

  gtk_widget_hide (gtk_widget_get_toplevel (GTK_WIDGET (button)));
  gtk_widget_grab_focus (GTK_WIDGET (gtk_builder_get_object (builder, "barcode_entry")));

  gtk_list_store_clear (venta->store);

  CleanEntryAndLabelData ();

  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_total")), "");

  gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_ticket_number")),
                        g_strdup_printf ("<span size=\"15000\">%.6d</span>", get_last_sell_id ()));

  ListClean ();
}

/**
 * Al recibir un signal de "cantidad_entry", cambia el foco dependiendo de la opción
 * VENTA_DIRECTA especificada en el .rizoma
 *
 * @parametro entry tipo GtkEntry * que recive la señal
 * @parametro data tipo gpointer entrega el puntero para el procedimiento interno
 */
void
MoveFocus (GtkEntry *entry, gpointer data)
{ // Al presionar [ENTER] en el entry "Cantidad"
  if (atoi(rizoma_get_value("VENTA_DIRECTA")) == FALSE || // Si VENTA_DIRECTA = 0 mueve el foco al botón "Añadir"
      (atoi(rizoma_get_value("VENTA_DIRECTA")) == TRUE && // O Si VENTA_DIRECTA = 1 Y Es una venta fracción
       (VentaFraccion (g_strdup (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "barcode_entry")))))) )) 
    {
      GtkWidget *button;
      button = GTK_WIDGET (gtk_builder_get_object (builder, "sell_add_button"));
      gtk_widget_set_sensitive(button, TRUE);
      gtk_widget_grab_focus (button);
    }
  else // De lo contrario el foco regresa al entry "Código de barras"
    gtk_widget_grab_focus (GTK_WIDGET (gtk_builder_get_object (builder, "barcode_entry")));
}

void
AumentarCantidad (GtkEntry *entry, gpointer data)
{
  gdouble cantidad = strtod (PUT (g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "cantidad_entry"))))), (char **)NULL);
  gint precio = atoi (CutPoints (g_strdup (gtk_label_get_text (GTK_LABEL (gtk_builder_get_object (builder, "label_precio"))))));
  gint precio_mayor = atoi (CutPoints (g_strdup (gtk_label_get_text (GTK_LABEL (gtk_builder_get_object (builder, "label_mayor"))))));
  gdouble cantidad_mayor = strtod (PUT (g_strdup (gtk_label_get_text (GTK_LABEL (gtk_builder_get_object (builder, "label_mayor_cantidad"))))),
                                   (char **)NULL);
  guint32 subtotal;

  if (precio != 0 && ((mayorista == FALSE || cantidad < cantidad_mayor) ||
                      (mayorista == TRUE && (cantidad_mayor == 0 || precio_mayor == 0))))
    {
      subtotal = llround ((gdouble)cantidad * precio);

      gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_subtotal")),
                            g_strdup_printf ("<span weight=\"ultrabold\" size=\"12000\">%s</span>",
                                             PutPoints (g_strdup_printf ("%u", subtotal))));
    }
  else
    if (precio_mayor != 0 && mayorista == TRUE && cantidad >= cantidad_mayor)
      {
        subtotal = llround ((gdouble)cantidad * precio_mayor);

        gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_subtotal")),
                              g_strdup_printf ("<span weight=\"ultrabold\" size=\"12000\">%s</span>",
                                               PutPoints (g_strdup_printf ("%u", subtotal))));
      }
}

void
TipoVenta (GtkWidget *widget, gpointer data)
{
  GtkWindow *window;
  gchar *tipo_vendedor = rizoma_get_value ("VENDEDOR");
  gboolean sencillo_directo = rizoma_get_value_boolean ("SENCILLO_DIRECTO");

  /*Verifica si hay productos añadidos en la TreeView para vender*/

  if (venta->header == NULL)
    {
      ErrorMSG (GTK_WIDGET (gtk_builder_get_object (builder, "barcode_entry")), "No hay productos para vender");
      return;
    }

  /*De estar habilitada caja, se asegura que ésta se encuentre 
    abierta al momento de vender*/

  //TODO: Unificar esta comprobación en las funciones primarias 
  //      encargadas de hacer cualquier movimiento de caja

  if (rizoma_get_value_boolean ("CAJA"))
    if (check_caja()) // Se abre la caja en caso de que esté cerrada
      open_caja (TRUE);

  if (g_str_equal (tipo_vendedor, "1"))
    {
      //tipo_documento = VENTA;
      tipo_documento = SIMPLE;
      window = GTK_WINDOW (gtk_builder_get_object (builder, "vendedor_venta"));
      gtk_widget_grab_focus (GTK_WIDGET (gtk_builder_get_object (builder, "button_imprimir")));
      gtk_widget_show_all (GTK_WIDGET (window));
    }
  else
    {
      tipo_documento = SIMPLE;
      window = GTK_WINDOW (gtk_builder_get_object (builder, "tipo_venta_win_venta"));

      if (sencillo_directo == TRUE)
	gtk_widget_grab_focus (GTK_WIDGET (builder_get (builder, "sencillo_entry")));
      else
	gtk_widget_grab_focus (GTK_WIDGET (builder_get (builder, "entry_discount_money")));

      clean_container (GTK_CONTAINER (window));
      gtk_widget_show_all (GTK_WIDGET (window));
    }
  return;
}

void
CalcularVuelto (void)
{
  /* gchar *pago = g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "sencillo_entry")))); */
  gint paga_con = atoi (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "sencillo_entry"))));
  gint total = atoi (CutPoints (g_strdup (gtk_label_get_text (GTK_LABEL (gtk_builder_get_object (builder, "label_total"))))));
  gint resto;

  /* if (strcmp (pago, "c") == 0) */
  /*   PagoCheque (); */
  if (paga_con > 0)
    {
      if (paga_con < total)
        {
          gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "vuelto_label")),
                                "<span size=\"30000\">Monto Insuficiente</span>");
          gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "sell_button")), FALSE);

          return;
        }
      else
        {
          resto = paga_con - total;

          gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "vuelto_label")),
                                g_strdup_printf ("<span size=\"30000\">%s</span> "
                                                 "<span size=\"15000\">Vuelto</span>",
                                                 PutPoints (g_strdup_printf ("%d", resto))));

          gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "sell_button")), TRUE);
        }
    }
  else
    gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "vuelto_label")),
                          "<span size=\"30000\"> </span>");
}


gint
AddProduct (void)
{
  GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (venta->products_tree));
  GtkTreeIter iter;
  gint value;

  if (gtk_tree_selection_get_selected (selection, NULL, &iter) == TRUE)
    {
      gtk_tree_model_get (GTK_TREE_MODEL (venta->products_store), &iter,
                          0, &value,
                          -1);

      gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "codigo_corto")),
                          g_strdup_printf ("%d", value));

      SearchSellProduct (NULL, NULL);

      //      CloseProductsWindow ();
    }

  return 0;
}

void
LooksForClient (void)
{
  gint rut;
  gint total;

  if (SearchClient () == TRUE)
    {
      rut = atoi (g_strdup (gtk_entry_get_text (GTK_ENTRY (venta->entry_rut))));

      gtk_widget_set_sensitive (venta->client_label, TRUE);

      gtk_label_set_text (GTK_LABEL (venta->name_label),
                          ReturnClientName (rut));

      total = atoi (CutPoints (g_strdup (gtk_label_get_text (GTK_LABEL (gtk_builder_get_object (builder, "label_total"))))));

      gtk_widget_set_sensitive (venta->client_venta, TRUE);

      if (VentaPosible (rut, total) == TRUE)
        {
          gtk_label_set_text (GTK_LABEL (venta->client_vender),
                              "Si");
          gtk_widget_set_sensitive (venta->sell_button, TRUE);
        }
      else
        gtk_label_set_text (GTK_LABEL (venta->client_vender),
                            "No");

    }
  else
    {
      gtk_widget_set_sensitive (venta->client_label, FALSE);
      gtk_label_set_text (GTK_LABEL (venta->name_label),"");

      gtk_widget_set_sensitive (venta->client_venta, FALSE);
      gtk_label_set_text (GTK_LABEL (venta->client_vender), "");
    }
}

gboolean
SearchClient (void)
{
  gchar *rut = g_strdup (gtk_entry_get_text (GTK_ENTRY (venta->entry_rut)));

  if (RutExist (rut) == FALSE)
    return FALSE;
  else
    return TRUE;
}

gchar *
ModifieBarcode (gchar *barcode)
{
  gchar *alt;
  gchar lastchar;

  if (strchr (barcode, '-') == NULL)
    {
      lastchar = barcode[strlen (barcode)-1];

      alt = g_strndup (barcode, strlen (barcode) -1 );

      return strcat (alt, g_strdup_printf ("-%c", lastchar));
    }
  else
    return g_strdup (barcode);
}


void
parse_barcode (GtkEntry *entry, gpointer data)
{
  gboolean barcode_fragmentado = rizoma_get_value_boolean ("BARCODE_FRAGMENTADO");

  if (barcode_fragmentado == FALSE)
    {
      SearchSellProduct (entry, data);
      return;
    }

  // Especificaciones para fragmentar barcode
  gint bar_rango_producto_a = rizoma_get_value_int ("BAR_RANGO_PRODUCTO_A")-1;
  gint bar_rango_producto_b = rizoma_get_value_int ("BAR_RANGO_PRODUCTO_B")-1;
  gint bar_rango_cantidad_a = rizoma_get_value_int ("BAR_RANGO_CANTIDAD_A")-1;
  gint bar_rango_cantidad_b = rizoma_get_value_int ("BAR_RANGO_CANTIDAD_B")-1;
  gint bar_num_decimal = rizoma_get_value_int ("BAR_NUM_DECIMAL");

  gint largo_barcode = bar_rango_producto_b - bar_rango_producto_a + 1;
  gint largo_cantidad = bar_rango_cantidad_b - bar_rango_cantidad_a + 1;

  gchar *barcode = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));
  gchar barcode_split[largo_barcode+1]; //+1 '\0'
  gchar cantidad[largo_cantidad+2]; //+2 ',' y '\0'

  if (g_str_equal (barcode, "") || HaveCharacters (barcode) ||
      DataExist (g_strdup_printf ("SELECT barcode FROM producto WHERE barcode = %s", barcode)))
    {
      SearchSellProduct (entry, data);
    }
  else if (strlen(barcode) == 13)
    {
      //Divide el barcode
      //barcode_split = invested_strndup (g_strndup (barcode, 7), 1);
      strdup_string_range (barcode_split, barcode, bar_rango_producto_a, bar_rango_producto_b);
      strdup_string_range_with_decimal (cantidad, barcode, bar_rango_cantidad_a, bar_rango_cantidad_b, bar_num_decimal);
      
      //Divide Cantidad
      if (DataExist (g_strdup_printf ("SELECT barcode FROM producto WHERE barcode = %s", barcode_split)))
	{
	  //Setea el widget del barcode
	  gtk_entry_set_text (GTK_ENTRY (builder_get (builder, "barcode_entry")), barcode_split);
	  //Setea el widget cantidad
	  gtk_entry_set_text (GTK_ENTRY (builder_get (builder, "cantidad_entry")), cantidad);
	  //Llama a SearchSellProduct
	  SearchSellProduct (entry, data);
	}
      else
	SearchSellProduct (entry, data);
    }
  else
    SearchSellProduct (entry, data);
}


gint
SearchSellProduct (GtkEntry *entry, gpointer data)
{
  gchar *barcode;
  gchar *materia_prima;
  
  if (entry == NULL)
    barcode = g_strdup (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "barcode_entry"))));
  else
    barcode = g_strdup (gtk_entry_get_text (entry));
  
  materia_prima = g_strdup (PQvaluebycol (EjecutarSQL ("SELECT id FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'MATERIA PRIMA'"), 0, "id"));

  PGresult *res;
  gint venta_directa = atoi(rizoma_get_value("VENTA_DIRECTA"));
  gchar *q= NULL;

  if (strcmp (barcode, "") == 0)
    return 0;

  if (HaveCharacters (barcode) == TRUE || g_str_equal (barcode, ""))
    {
      BuscarProducto (NULL, NULL);
      return 0;
    }

  gtk_entry_set_text (entry, barcode);

  q = g_strdup_printf ("SELECT * FROM informacion_producto_venta ('%s')", barcode);
  res = EjecutarSQL(q);
  g_free(q);

  // TODO: Crear validaciones unificadas que permitan manejar con más claridad el control de los datos que se ingresan
  if (res == NULL)
    return -1;

  // Si la respuesta es de 0 filas (si no hay coincidencias en la búsqueda)
  if (PQntuples (res) == 0)
    {
      if (strcmp (barcode, "") != 0) // Si se ingresó algún código, significa que no existe el producto
        {
          AlertMSG (GTK_WIDGET (entry), g_strdup_printf
                    ("No existe un producto con el código de barras %s!!", barcode));

          if (ventas != FALSE)
            CleanSellLabels ();
        }
      else if (GetCurrentStock (barcode) == 0) //Nota: Creo que jamás entrará aquí
	{
	  AlertMSG (GTK_WIDGET (entry), "No hay mercadería en Stock.\nDebe ingresar mercadería");
	  
	  if (ventas != FALSE)
	    CleanSellLabels ();
	}
      else
	{
	  if (ventas != FALSE)
	    CleanSellLabels ();
	}
      return -1;
    }

  // Si el producto es "Materia Prima" no se puede comercializar
  if (g_str_equal (PQvaluebycol (res, 0, "tipo"), materia_prima) == TRUE)
    {
      GtkWidget *aux_widget;
      aux_widget = GTK_WIDGET (builder_get (builder, "ventas_gui"));
      gchar *str = g_strdup_printf("El código %s corresponde a una materia prima, no es comercializable", barcode);
      CleanSellLabels();
      AlertMSG (aux_widget, str);
      g_free (str);
      
      return -1;
    }

  // Si el estado es F significa que el producto fue eliminado
  if (strcmp (PQvaluebycol (res, 0, "estado"),"f") == 0)
    {
      GtkWidget *aux_widget;
      aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "ventas_gui"));
      gchar *str = g_strdup_printf("El código %s fué invalidado por el administrador", barcode);
      CleanSellLabels();
      AlertMSG (aux_widget, str);
      g_free (str);
      
      return -1;
    }

  //check if the product has stock
  if (strtod (PUT(PQvaluebycol(res, 0, "stock")), (char **)NULL) <= 0)
    {
      //the product has not stock, so display a message
      //and abort the operation
      GtkWidget *aux_widget;
      aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "barcode_entry"));
      gchar *str = g_strdup_printf("El producto %s no tiene stock", barcode);
      CleanSellLabels();
      AlertMSG (aux_widget, str);
      g_free (str);

      return -1;
    }

  mayorista = strcmp (PQvaluebycol( res, 0, "mayorista"), "t") == 0 ? TRUE : FALSE;

  FillProductSell (PQvaluebycol (res, 0, "barcode"), mayorista,  PQvaluebycol (res, 0, "marca"), PQvaluebycol (res, 0, "descripcion"),
                   PQvaluebycol (res, 0, "contenido"), PQvaluebycol (res, 0, "unidad"),
                   PQvaluebycol (res, 0, "stock"), PQvaluebycol (res, 0, "stock_day"),
                   PQvaluebycol (res, 0, "precio"), PQvaluebycol (res, 0, "precio_mayor"),
                   PQvaluebycol (res, 0, "cantidad_mayor"), PQvaluebycol (res, 0, "codigo_corto"));

  if (atoi (PQvaluebycol (res, 0, "precio")) != 0)
    {
      if (venta_directa == 1)
	{
	  if (VentaFraccion (PQvaluebycol (res, 0, "barcode")))
	    {
	      gtk_widget_grab_focus( GTK_WIDGET (gtk_builder_get_object (builder, "cantidad_entry")));
	      gtk_widget_set_sensitive (add_button, TRUE);
	    }
	  else
	    {
	      AgregarProducto( NULL, NULL );
	    }
	}
      else
	{
	  gtk_widget_grab_focus (GTK_WIDGET (gtk_builder_get_object (builder, "cantidad_entry")));
	  gtk_widget_set_sensitive( add_button, TRUE );
	}
    }
  else
    gtk_widget_grab_focus (GTK_WIDGET (gtk_builder_get_object (builder, "barcode_entry")));

  return 0;
}

void
CloseBuscarWindow (GtkWidget *widget, gpointer data)
{
  gboolean add = (gboolean) data;
  gboolean fraccion = VentaFraccion (g_strdup (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "barcode_entry")))));
  gint venta_directa = atoi(rizoma_get_value("VENTA_DIRECTA"));  

  gtk_widget_hide (GTK_WIDGET (gtk_builder_get_object (builder, "ventas_buscar")));

  if (add == TRUE && venta_directa == 0) 
    gtk_widget_grab_focus (GTK_WIDGET (gtk_builder_get_object (builder, "cantidad_entry")));
  else if (add == TRUE && (venta_directa == 1 && fraccion == TRUE))
    gtk_widget_grab_focus (GTK_WIDGET (gtk_builder_get_object (builder, "cantidad_entry")));
  else
    gtk_widget_grab_focus (GTK_WIDGET (gtk_builder_get_object (builder, "barcode_entry")));
}

void
BuscarProducto (GtkWidget *widget, gpointer data)
{
  GtkWidget *aux_widget;
  GtkWidget *entry;
  GtkWindow *window;

  GtkListStore *store;

  GtkTreeView *treeview = GTK_TREE_VIEW (gtk_builder_get_object (builder, "ventas_search_treeview"));

  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  aux_widget = GTK_WIDGET(gtk_builder_get_object (builder,
                                                  "ventas_buscar_entry"));
  entry = GTK_WIDGET(gtk_builder_get_object (builder, "barcode_entry"));

  gtk_entry_set_text (GTK_ENTRY (aux_widget),
                      gtk_entry_get_text (GTK_ENTRY (entry)));

  if (gtk_tree_view_get_model (treeview) == NULL )
    {
      store = gtk_list_store_new (10,
                                  G_TYPE_STRING,
                                  G_TYPE_STRING,
                                  G_TYPE_STRING,
                                  G_TYPE_STRING,
                                  G_TYPE_INT,
                                  G_TYPE_STRING,
                                  G_TYPE_STRING,
                                  G_TYPE_INT,
                                  G_TYPE_STRING,
                                  G_TYPE_BOOLEAN);

      gtk_tree_view_set_model (treeview, GTK_TREE_MODEL (store));

      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes ("Código", renderer,
                                                         "text", 0,
                                                         NULL);
      gtk_tree_view_append_column (treeview, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
      gtk_tree_view_column_set_min_width (column, 75);
      gtk_tree_view_column_set_max_width (column, 75);
      gtk_tree_view_column_set_resizable (column, FALSE);

      if (solo_venta != TRUE)
        {
          renderer = gtk_cell_renderer_text_new ();
          column = gtk_tree_view_column_new_with_attributes ("Código de Barras",
                                                             renderer,
                                                             "text", 1,
                                                             NULL);
          gtk_tree_view_append_column (treeview, column);
          gtk_tree_view_column_set_alignment (column, 0.5);
          g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
          gtk_tree_view_column_set_resizable (column, FALSE);
        }

      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes ("Descripción",
                                                         renderer,
                                                         "text", 2,
                                                         "foreground", 8,
                                                         "foreground-set", 9,
                                                         NULL);
      gtk_tree_view_append_column (treeview, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
      gtk_tree_view_column_set_min_width (column, 270);
      gtk_tree_view_column_set_max_width (column, 270);
      gtk_tree_view_column_set_resizable (column, FALSE);

      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes ("Marca", renderer,
                                                         "text", 3,
                                                         NULL);
      gtk_tree_view_append_column (treeview, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
      gtk_tree_view_column_set_min_width (column, 90);
      gtk_tree_view_column_set_max_width (column, 90);
      gtk_tree_view_column_set_resizable (column, FALSE);

      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes ("Cont.", renderer,
                                                         "text", 4,
                                                         NULL);
      gtk_tree_view_append_column (treeview, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
      gtk_tree_view_column_set_min_width (column, 40);
      gtk_tree_view_column_set_max_width (column, 40);
      gtk_tree_view_column_set_resizable (column, FALSE);

      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes ("Uni.", renderer,
                                                         "text", 5,
                                                         NULL);
      gtk_tree_view_append_column (treeview, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
      gtk_tree_view_column_set_min_width (column, 40);
      gtk_tree_view_column_set_max_width (column, 40);
      gtk_tree_view_column_set_resizable (column, FALSE);

      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes ("Stock", renderer,
                                                         "text", 6,
                                                         NULL);
      gtk_tree_view_append_column (treeview, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
      gtk_tree_view_column_set_min_width (column, 60);
      gtk_tree_view_column_set_max_width (column, 60);
      gtk_tree_view_column_set_resizable (column, FALSE);

      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes ("Precio", renderer,
                                                         "text", 7,
                                                         NULL);
      gtk_tree_view_append_column (treeview, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
      gtk_tree_view_column_set_min_width (column, 70);
      gtk_tree_view_column_set_max_width (column, 70);
      gtk_tree_view_column_set_resizable (column, FALSE);
    }

  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "ventas_buscar_entry"));
  gtk_entry_set_text(GTK_ENTRY(aux_widget), gtk_entry_get_text(GTK_ENTRY(entry)));

  window = GTK_WINDOW (gtk_builder_get_object (builder, "ventas_buscar"));
  gtk_widget_show_all (GTK_WIDGET (window));

  SearchAndFill ();

  return;
}

/**
 * Populate the search products window, takes as argument the content of
 * the 'ventas_buscar_entry' for the buscar_producto plpgsql function, and
 * the rows returned by the function are inserted into the TreeView model
 *
 */
void
SearchAndFill (void)
{
  PGresult *res=NULL;
  gint resultados, i;
  gchar *q;
  gchar *string;
  gchar *materia_prima;
  GtkTreeView *tree = GTK_TREE_VIEW (gtk_builder_get_object (builder, "ventas_search_treeview"));
  GtkTreeIter iter;
  GtkListStore *store;

  string = g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "ventas_buscar_entry"))));
  store = GTK_LIST_STORE (gtk_tree_view_get_model (tree));
  materia_prima = g_strdup (PQvaluebycol (EjecutarSQL ("SELECT id FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'MATERIA PRIMA'"), 0, "id"));

  if (!(g_str_equal (string, "")))
    {
      q = g_strdup_printf ("SELECT * FROM buscar_producto ('%s', "
                           "'{\"barcode\", \"codigo_corto\",\"marca\","
                           "\"descripcion\"}', true, true ) WHERE tipo_id != %s", string, materia_prima);
      res = EjecutarSQL (q);
      resultados = PQntuples (res);
    }
  else
    resultados = 0;

  gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "ventas_label_found")),
                        g_strdup_printf ("<b>%d producto(s)</b>", resultados));

  gtk_list_store_clear (store);

  if (resultados == 0)
    {
      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
                          2, "No se encontró descripción",
                          8, "Red",
                          9, TRUE,
                          -1);
    }
  else
    {
      for (i = 0; i < resultados; i++)
        {
          gtk_list_store_append (store, &iter);
          gtk_list_store_set (store, &iter,
                              0, PQvaluebycol (res, i, "codigo_corto"),
                              1, PQvaluebycol (res, i, "barcode"),
                              2, PQvaluebycol (res, i, "descripcion"),
                              3, PQvaluebycol (res, i, "marca"),
                              4, atoi (PQvaluebycol (res, i, "contenido")),
                              5, PQvaluebycol (res, i, "unidad"),
                              6, g_strdup_printf("%.3f", strtod (PUT (PQvaluebycol (res, i, "stock")), (char **)NULL)),
                              7, atoi (PQvaluebycol (res, i, "precio")),
                              -1);
        }
    }
  gtk_widget_grab_focus (GTK_WIDGET (tree));
  gtk_tree_selection_select_path (gtk_tree_view_get_selection (tree), gtk_tree_path_new_from_string ("0"));
}

void
FillSellData (GtkTreeView *treeview, GtkTreePath *arg1, GtkTreeViewColumn *arg2, gpointer data)
{
  GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (gtk_builder_get_object (builder, "ventas_search_treeview")));
  GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (gtk_builder_get_object (builder, "ventas_search_treeview"))));
  GtkTreeIter iter;
  gchar *barcode, *product, *codigo;
  gint precio;

  if (gtk_tree_selection_get_selected (selection, NULL, &iter) == TRUE)
    {
      gtk_tree_model_get (GTK_TREE_MODEL (store), &iter,
                          0, &codigo,
                          1, &barcode,
                          2, &product,
                          7, &precio,
                          -1);

      if (codigo == NULL)
        return;

      if (ventas == TRUE)
        {
	  gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "product_label")), 
				g_strdup_printf ("<span weight=\"ultrabold\" size=\"12000\">%s</span>", product));

          gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_precio")),
                                g_strdup_printf ("<span weight=\"ultrabold\" size=\"12000\">%s</span>",
                                                 PutPoints (g_strdup_printf ("%d", precio))));
          gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_subtotal")),
                                g_strdup_printf ("<span weight=\"ultrabold\" size=\"12000\">%s</span>",
                                                 PutPoints (g_strdup_printf ("%u", atoi (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "cantidad_entry")))) *
                                                                             atoi (CutPoints (g_strdup (gtk_label_get_text (GTK_LABEL (gtk_builder_get_object (builder, "label_precio"))))))))));

	  gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "codigo_corto")), 
				g_strdup_printf ("<span weight=\"ultrabold\" size=\"12000\">%s</span>", codigo));

          gtk_entry_set_text (GTK_ENTRY (gtk_builder_get_object (builder, "barcode_entry")), barcode);

          SearchSellProduct (GTK_ENTRY (builder_get (builder, "barcode_entry")), (gpointer)TRUE);
        }
      CloseBuscarWindow (NULL, (gpointer)TRUE);
    }
}

void
Descuento (GtkWidget *widget, gpointer data)
{
  gchar *widget_name = g_strdup (gtk_buildable_get_name (GTK_BUILDABLE (widget)));

  gint amount = atoi (gtk_entry_get_text (GTK_ENTRY (widget)));
  gint total;
  gint money_discount = 0;
  gint percent_discount = 0;

  if (block_discount) return;

  block_discount = TRUE;

  if (amount <= 0)
    {
      gtk_entry_set_text (GTK_ENTRY (builder_get (builder, "entry_discount_money")), "");
      gtk_entry_set_text (GTK_ENTRY (builder_get (builder, "entry_discount_percent")), "");

      CalcularVentas (venta->header);
      block_discount = FALSE;
      return;
    }


  CalcularVentas (venta->header);
  total = atoi (CutPoints (g_strdup (gtk_label_get_text (GTK_LABEL (builder_get (builder, "label_total"))))));

  if (g_str_equal (widget_name, "entry_discount_money"))
    {
      if (amount > total)
        {
          block_discount = FALSE;
          return;
        }

      money_discount = amount;
      percent_discount = lround ( (gdouble) (money_discount * 100) / total);

      gtk_entry_set_text (GTK_ENTRY (builder_get (builder, "entry_discount_percent")),
                          g_strdup_printf ("%d", percent_discount));
    }
  else if (g_str_equal (widget_name, "entry_discount_percent"))
    {
      if (amount > 100)
        {
          block_discount = FALSE;
          return;
        }

      money_discount = lround ( (gdouble) (total * amount) / 100);
      gtk_entry_set_text (GTK_ENTRY (builder_get (builder, "entry_discount_money")),
                          g_strdup_printf ("%d", money_discount));
    }

  block_discount = FALSE;

  gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_total")),
                        g_strdup_printf ("<span size=\"40000\">%s</span>", PutPoints (g_strdup_printf ("%u", total - money_discount))));
}

gboolean
CalcularVentas (Productos *header)
{
  guint32 total = llround (CalcularTotal (header));

  gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_total")),
                        g_strdup_printf ("<span size=\"40000\">%s</span>",
                                         PutPoints (g_strdup_printf ("%u", total))));
  return TRUE;
}


void
CloseCancelWindow (void)
{
  gtk_widget_destroy (venta->cancel_window);
  venta->cancel_window = NULL;

  gtk_widget_grab_focus (GTK_WIDGET (gtk_builder_get_object (builder, "barcode_entry")));

}

void
FindCancelSell (GtkWidget *widget, gpointer data)
{
  gchar *text = g_strdup (gtk_entry_get_text (GTK_ENTRY ( data)));
  PGresult *res;
  gint tuples, i;
  GtkTreeIter iter;

  gchar *q = NULL;

  q = g_strdup_printf("SELECT venta.id, monto, users.usuario FROM venta "
                      "INNER JOIN users ON users.id = venta.vendedor "
                      "WHERE venta.id = %s AND canceled = FALSE",
                      text);
  res = EjecutarSQL (q);
  g_free(q);

  tuples = PQntuples (res);

  gtk_list_store_clear (venta->cancel_store);

  for (i = 0; i < tuples; i++)
    {
      gtk_list_store_append (venta->cancel_store, &iter);
      gtk_list_store_set (venta->cancel_store, &iter,
                          0, PQvaluebycol (res, i, "id"),
                          1, PQvaluebycol (res, i, "usuario"),
                          2, PQvaluebycol (res, i, "monto"),
                          -1);
    }
}

void
CancelSell (GtkWidget *widget, gpointer data)
{
  GtkWidget *window_kill = gtk_widget_get_toplevel (widget);
  gint cancel = (gboolean)data;
  gchar *id_venta;

  GtkTreeIter iter;
  GtkTreeSelection *selection;

  //PGresult *res;

  if (cancel == 1)
    {
      selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (venta->cancel_tree));

      if (gtk_tree_selection_get_selected (selection, NULL, &iter) == TRUE)
        {
          gtk_tree_model_get (GTK_TREE_MODEL (venta->cancel_store), &iter,
                              0, &id_venta,
                              -1);
          gchar *q;
          q = g_strdup_printf ("select cancelar_venta(%s)", id_venta);
          EjecutarSQL (q);
          g_free(id_venta);
          g_free(q);
          gtk_list_store_remove(venta->cancel_store,&iter);
        }
    }

  gtk_widget_set_sensitive (venta->cancel_window, TRUE);
  gtk_widget_destroy (window_kill);
}

void
AskCancelSell (GtkWidget *widget, gpointer data)
{
  GtkWidget *window;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *button;
  GtkWidget *label;

  gtk_widget_set_sensitive (venta->cancel_window, FALSE);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER_ALWAYS);
  gtk_window_present (GTK_WINDOW (window));
  gtk_window_set_resizable (GTK_WINDOW (window), FALSE);
  gtk_widget_show (window);

  g_signal_connect (G_OBJECT (window), "destroy",
                    G_CALLBACK (CancelSell), (gpointer)FALSE);

  vbox = gtk_vbox_new (FALSE, 3);
  gtk_container_add (GTK_CONTAINER (window), vbox);
  gtk_widget_show (vbox);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  label = gtk_label_new ("¿Desea cancelar la venta?");
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
  gtk_widget_show (label);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_end (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  button = gtk_button_new_from_stock (GTK_STOCK_NO);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 3);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (CancelSell), (gpointer)0);

  button = gtk_button_new_from_stock (GTK_STOCK_YES);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 3);
  gtk_widget_show (button);

  gtk_window_set_focus (GTK_WINDOW (window), button);

  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (CancelSell), (gpointer)1);

}
/*
 * El callback asociado al calendario que se encuentra en la venta
 * para cancelar ventas
 */
void
CancelSellDaySelected(GtkCalendar *calendar,gpointer data)
{
  GtkTreeIter iter;
  PGresult *res;
  gint tuples, i;
  guint year,month,day;

  gtk_calendar_get_date(calendar,&year,&month,&day);


  gchar *q = NULL;
  q = g_strdup_printf("SELECT venta.id, users.usuario,monto FROM venta "
                      "INNER JOIN users ON users.id = venta.vendedor "
                      "WHERE date_trunc('day',fecha) = '%.4d-%.2d-%.2d' "
                      "AND canceled = FALSE",
                      year,month+1,day);

  res = EjecutarSQL (q);
  g_free(q);

  tuples = PQntuples (res);

  gtk_list_store_clear (venta->cancel_store);

  for (i = 0; i < tuples; i++)
    {
      gtk_list_store_append (venta->cancel_store, &iter);
      gtk_list_store_set (venta->cancel_store, &iter,
                          0, PQvaluebycol (res, i, "id"),
                          1, PQvaluebycol (res, i, "usuario"),
                          2, PQvaluebycol (res, i, "monto"),
                          -1);
    }
}

void
CancelSellViewDetails(GtkTreeView *tree_view, gpointer user_data)
{
  gchar *q = NULL;
  int i,tuples;
  gchar *id_venta=NULL;
  PGresult *res;
  GtkTreeIter iter_detail,iter;
  GtkTreeSelection *selection;

  gtk_list_store_clear(venta->cancel_store_details);

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (venta->cancel_tree));

  if (gtk_tree_selection_get_selected (selection, NULL, &iter) == TRUE)
    {
      gtk_tree_model_get (GTK_TREE_MODEL (venta->cancel_store), &iter,
                          0, &id_venta,
                          -1);

      q = g_strdup_printf("SELECT producto.descripcion|| ' ' ||producto.marca, "
                          "cantidad, (venta_detalle.precio * cantidad) "
                          "FROM venta_detalle inner join producto on "
                          "venta_detalle.barcode = producto.barcode "
                          "WHERE id_venta=%s",
                          id_venta);

      res = EjecutarSQL (q);
      g_free(q);

      tuples = PQntuples (res);

      for (i = 0; i < tuples; i++)
        {
          gtk_list_store_append (venta->cancel_store_details, &iter_detail);
          gtk_list_store_set (venta->cancel_store_details, &iter_detail,
                              0, PQgetvalue (res, i, 0),
                              1, PQgetvalue (res, i, 1),
                              2, PQgetvalue (res, i, 2),
                              -1);
        }
    }
}


/**
 * This function shows (and hide) the corresponding widgets
 * as the selection made.
 *
 * @param: GtkToggleButton *togglebutton
 * @param: gpointer user_data
 */
void
change_pay2_mode (GtkToggleButton *togglebutton, gpointer user_data)
{
  //The "Efectivo" toggle button
  if (gtk_toggle_button_get_active (togglebutton))
    {
      gtk_widget_grab_focus (GTK_WIDGET (builder_get (builder, "entry_int_amount_mixed_pay2")));
      gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "entry_rut_mixed_pay_2")), FALSE);
      gtk_entry_set_text (GTK_ENTRY (builder_get (builder, "entry_rut_mixed_pay_2")), "");
      gtk_label_set_text (GTK_LABEL (builder_get (builder, "lbl_name_mixed_pay_2")), "");
    }
  else
    {
      gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "entry_rut_mixed_pay_2")), TRUE);
      gtk_widget_grab_focus (GTK_WIDGET (builder_get (builder, "entry_rut_mixed_pay_2")));
    }
}


/*
 * callback asociado a boton 'cancelar venta' para la costruccion de
 * la ventana para cancelar una venta
 */
void
CancelWindow (GtkWidget *widget, gpointer data)
{

  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;

  GtkWidget *scroll;
  GtkWidget *hbox;
  GtkWidget *vbox;

  GtkWidget *button;
  GtkWidget *entry;
  GtkWidget *calendario;

  venta->cancel_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  gtk_window_set_title (GTK_WINDOW (venta->cancel_window), "Cancelar Venta");

  gtk_window_set_position (GTK_WINDOW (venta->cancel_window), GTK_WIN_POS_CENTER_ALWAYS);
  gtk_window_present (GTK_WINDOW (venta->cancel_window));
  gtk_window_set_resizable (GTK_WINDOW (venta->cancel_window), FALSE);

  g_signal_connect (G_OBJECT (venta->cancel_window), "destroy",
                    G_CALLBACK (CloseCancelWindow), NULL);


  vbox = gtk_vbox_new (FALSE, 3);
  gtk_container_add (GTK_CONTAINER (venta->cancel_window), vbox);

  hbox = gtk_hbox_new (FALSE, 3);//principal
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
  GtkWidget *vbox_izquierda = gtk_vbox_new(FALSE,3);
  gtk_box_pack_start(GTK_BOX(hbox),vbox_izquierda,FALSE,FALSE,3);

  GtkWidget *hbox_busqueda = gtk_hbox_new(FALSE,3);
  gtk_box_pack_start(GTK_BOX(vbox_izquierda),hbox_busqueda,FALSE,FALSE,3);

  //entry de busqueda
  entry = gtk_entry_new ();
  gtk_window_set_focus (GTK_WINDOW (venta->cancel_window), entry);
  gtk_box_pack_start (GTK_BOX (hbox_busqueda), entry, FALSE, FALSE, 3);

  g_signal_connect (G_OBJECT (entry), "activate",
                    G_CALLBACK (FindCancelSell), (gpointer) entry);

  //boton de busqueda
  button = gtk_button_new_from_stock (GTK_STOCK_FIND);
  gtk_box_pack_end (GTK_BOX (hbox_busqueda), button, FALSE, FALSE, 3);

  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (FindCancelSell), (gpointer) entry);

  /* calendario */
  GtkWidget *expansor = gtk_expander_new("Calendario");
  calendario = gtk_calendar_new();
  g_signal_connect(G_OBJECT(calendario),"day-selected-double-click",
                   G_CALLBACK (CancelSellDaySelected),NULL);
  gtk_container_add(GTK_CONTAINER(expansor),calendario);
  gtk_box_pack_start(GTK_BOX(vbox_izquierda),expansor,FALSE,FALSE,0);
  /* fin calendario */

  GtkWidget *vbox_derecha = gtk_vbox_new(FALSE,3);
  gtk_box_pack_start(GTK_BOX(hbox),vbox_derecha,FALSE,FALSE,0);

  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_size_request (scroll, -1, 150);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (vbox_izquierda), scroll, FALSE, FALSE, 0);

  //tree de ventas
  venta->cancel_store = gtk_list_store_new (3,
                                            G_TYPE_STRING,
                                            G_TYPE_STRING,
                                            G_TYPE_STRING);

  venta->cancel_tree = gtk_tree_view_new_with_model (GTK_TREE_MODEL (venta->cancel_store));
  gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (venta->cancel_tree), TRUE);
  gtk_container_add (GTK_CONTAINER (scroll), venta->cancel_tree);

  g_signal_connect (G_OBJECT (venta->cancel_tree), "row-activated",
                    G_CALLBACK (CancelSellViewDetails), NULL);

  //ventas-ID
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("ID Venta", renderer,
                                                     "text", 0,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (venta->cancel_tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  //ventas-Vendedor
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Vendedor", renderer,
                                                     "text", 1,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (venta->cancel_tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  //total de la venta
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Total", renderer,
                                                     "text", 2,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (venta->cancel_tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  /////////////////////////
  //detalles de la venta //
  /////////////////////////
  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_size_request (scroll, -1, 100);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (vbox_izquierda), scroll, FALSE, FALSE, 0);

  venta->cancel_store_details = gtk_list_store_new (3,
                                                    G_TYPE_STRING,
                                                    G_TYPE_STRING,
                                                    G_TYPE_STRING);

  //tree de ventas
  venta->cancel_tree_details = gtk_tree_view_new_with_model (GTK_TREE_MODEL (venta->cancel_store_details));
  gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (venta->cancel_tree_details), TRUE);
  gtk_container_add (GTK_CONTAINER (scroll), venta->cancel_tree_details);

  //  g_signal_connect (G_OBJECT (venta->cancel_tree), "row-activated",
  //    G_CALLBACK (AskCancelSell), NULL);
  ////////////////////////

  //descripcion-marca
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Descripcion", renderer,
                                                     "text", 0,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (venta->cancel_tree_details), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  //cantidad
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Cant.", renderer,
                                                     "text", 1,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (venta->cancel_tree_details), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  //subtotal = (precio*cantidad)
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("SubTotal", renderer,
                                                     "text", 2,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (venta->cancel_tree_details), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  //boton para cancelar una venta
  button = gtk_button_new_with_mnemonic ("_Cancelar Venta");
  gtk_box_pack_end (GTK_BOX (vbox), button, FALSE, FALSE, 0);

  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (AskCancelSell), NULL);

  gtk_widget_show_all(venta->cancel_window);
}

void
CloseWindowChangeSeller (GtkWidget *widget, gpointer data)
{
  GtkWidget *aux_widget;

  aux_widget = GTK_WIDGET (gtk_builder_get_object (builder, "window_change_seller"));
  gtk_widget_hide (aux_widget);

  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_user_vendedor"));
  gtk_entry_set_text(GTK_ENTRY(aux_widget), "");
  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_pass_vendedor"));
  gtk_entry_set_text(GTK_ENTRY(aux_widget), "");

  aux_widget = GTK_WIDGET (gtk_builder_get_object (builder, "barcode_entry"));
  gtk_widget_grab_focus (aux_widget);
}

void
ChangeSeller (GtkWidget *widget, gpointer data)
{
  GtkEntry *entry;
  gchar *user;
  gchar *passwd;

  entry = GTK_ENTRY(gtk_builder_get_object(builder, "entry_user_vendedor"));
  user = g_strdup (gtk_entry_get_text (entry));
  entry = GTK_ENTRY(gtk_builder_get_object(builder, "entry_pass_vendedor"));
  passwd = g_strdup (gtk_entry_get_text (entry));

  if (g_str_equal (user, "") || g_str_equal (passwd, ""))
    {
      ErrorMSG (widget, g_strdup_printf
                ("Debe ingresar usuario y contraseña"));
      return;
    }

  if (AcceptPassword (passwd, user))
    {
      user_data->user_id = ReturnUserId (user);
      user_data->user = g_strdup (user);
      user_data->level = ReturnUserLevel (user);

      gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_seller_name")),
                            g_strdup_printf ("<span size=\"15000\">%s</span>", user_data->user));

      CloseWindowChangeSeller (widget, NULL);
    }
  else
    {
      ErrorMSG (widget, g_strdup_printf
                ("Usuario o contraseña incorrecta"));
    }
}

/**
 * Es llamada cuando el boton "btn_change_seller" es presionado (signal click).
 *
 * Esta funcion visualiza la ventana "window_change_seller" y deja el foco en
 * el "entry_id_vendedor"
 *
 */

void
WindowChangeSeller ()
{
  GtkWindow *window;

  window = GTK_WINDOW (gtk_builder_get_object (builder, "window_change_seller"));
  gtk_widget_show_all (GTK_WIDGET (window));
  gtk_widget_grab_focus (GTK_WIDGET (builder_get (builder, "entry_user_vendedor")));
}


/**
 * Funcion Principal
 *
 * Carga los valores de rizoma-login.ui y visualiza la ventana del
 * login "login_window", ademas de cargar el archivo de configuracion "/.rizoma"
 *
 */

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

  login_window = GTK_WINDOW(gtk_builder_get_object (builder, "login_window"));

  profiles = g_key_file_get_groups (key_file, NULL);

  g_key_file_free (key_file);

  model = gtk_list_store_new (1,
                              G_TYPE_STRING);

  combo = (GtkComboBox *) gtk_builder_get_object (builder, "combo_profile");

  cell = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT(combo), cell, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT(combo), cell,
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
          g_free (*profiles);
        }
    } while (*profiles++ != NULL);

  gtk_combo_box_set_model (combo, (GtkTreeModel *)model);
  gtk_combo_box_set_active (combo, 0);

  gtk_widget_show_all ((GtkWidget *)login_window);

  gtk_main();

  return 0;
}


/**
 * Connected to accept button of the login window
 *
 * This funcion is in charge of validate the user/password and then
 * call the initialization functions
 *
 * @param widget the widget that emits the signal
 * @param data the user data
 */
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

      log_register_access (user_data, TRUE);

      gtk_widget_destroy (GTK_WIDGET(gtk_builder_get_object (builder,"login_window")));
      g_object_unref ((gpointer) builder);
      builder = NULL;

      ventas_win();

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

/**
 * Callback connected to delete-event of the sales window and the exit
 * button in the same window.
 *
 * Checks if must be closed the caja, if must be closed then calls the
 * propers functions, otherwise raise the confirmation dialog.
 *
 * Note: do not use the parameters of this function, because they are
 * not secure.
 *
 * @param widget the widget that emits the signal
 * @param event the event
 * @param data the user data
 *
 * @return TRUE to stop other handlers of being invoked
 */
gboolean
on_delete_ventas_gui (GtkWidget *widget, GdkEvent *event, gpointer data)
{
  GtkWidget *window;

  window = GTK_WIDGET (gtk_builder_get_object (builder, "quit_message"));
  gtk_dialog_set_default_response (GTK_DIALOG(window), GTK_RESPONSE_YES);
  gtk_widget_show_all (window);

  return TRUE;
}

/**
 * Callback connected to response signal emited by the confirmation
 * dialog of rizoma-ventas
 *
 * This function must contain all the necesary finalization code, like
 * the exit time of the sales man
 *
 * @param dialog the dialog that emits the signal
 * @param response_id the response
 * @param data the user data
 */
void
exit_response (GtkDialog *dialog, gint response_id, gpointer data)
{
  if (response_id == GTK_RESPONSE_YES)
    {
      Asistencia (user_data->user_id, FALSE);
      log_register_access (user_data, FALSE);
      gtk_main_quit();
    }
  else
    if (response_id == GTK_RESPONSE_NO)
      gtk_widget_hide (GTK_WIDGET (dialog));
}

//related with the credit sale
void
on_btn_credit_sale_clicked (GtkButton *button, gpointer data)
{
  GtkWidget *widget;
  gint tipo_documento;
  gint rut;
  gchar *dv;
  gchar *str_rut;
  gchar **str_splited;
  gint vendedor;
  gint tipo_vendedor;
  gint ticket;
  gint monto;
  gint maquina;
  gchar *discount;
  gboolean canceled;

  if (g_str_equal (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_credit_rut"))), "") ||
      gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_credit_rut"))) == NULL)
    {
      search_client (GTK_WIDGET (builder_get (builder, "entry_credit_rut")), NULL);
      return;
    }

  str_splited = parse_rut (g_strdup (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_credit_rut")))));

  rut = atoi(str_splited[0]);
  dv = g_strdup(str_splited[1]);
  g_free (str_splited);
  str_rut = g_strdup_printf("%d-%s",rut,dv);

  tipo_vendedor = rizoma_get_value_int ("VENDEDOR");

  //amount
  widget = GTK_WIDGET(gtk_builder_get_object(builder, "label_total"));
  monto = atoi (CutPoints (g_strdup (gtk_label_get_text (GTK_LABEL (widget)))));
  //maquina
  maquina = rizoma_get_value_int ("MAQUINA");

  //salesman
  vendedor = user_data->user_id;

  if (!(RutExist(str_rut)))
    {
      gchar *motivo;

      widget = gtk_widget_get_ancestor(GTK_WIDGET(button),GTK_TYPE_WINDOW);

      motivo = g_strdup_printf("El rut %d no existe", rut);
      AlertMSG(widget, motivo);
      g_free (motivo);

      return;
    }

  if (LimiteCredito (str_rut) < (DeudaTotalCliente (rut) + monto))
    {
      gtk_widget_show (GTK_WIDGET (builder_get (builder, "msg_credit_out")));
      return;
    }

  /* if (tipo_vendedor == 1) */
  /*   tipo_documento = VENTA; */
  /* else */
  tipo_documento = SIMPLE;

  //if (tipo_documento != VENTA)
  if (tipo_vendedor != 1)
    {
      GtkWidget *wid;
      wid = GTK_WIDGET (gtk_builder_get_object (builder,"entry_discount_percent"));
      if (g_str_equal (gtk_entry_get_text (GTK_ENTRY (wid)),""))
        discount = "0";
      else
        discount = g_strdup(gtk_entry_get_text (GTK_ENTRY (wid)));
    }
  else
    discount = "0";

  switch (tipo_documento)
    {
    case SIMPLE: case VENTA:
      if (monto >= 180)
        ticket = get_ticket_number (tipo_documento);
      else
        ticket = -1;
      break;
    /* case VENTA: */
    /*   ticket = -1; */
    /*   break; */
    default:
      g_printerr("The document type could not be matched on %s", G_STRFUNC);
      ticket = -1;
      break;
    }

  // TODO: Queda temporalmente comentado hasta que de implemente completamente
  // la funcionalidad de pre-venta
  /* if (tipo_documento == VENTA) */
  /*   canceled = FALSE; */
  /* else */ 
  canceled = TRUE;

  SaveSell (monto, maquina, vendedor, CREDITO, str_rut, discount, ticket, tipo_documento,
            NULL, FALSE, canceled);

  //clean the interface
  gtk_widget_hide (gtk_widget_get_toplevel (GTK_WIDGET (button)));
  gtk_widget_hide (GTK_WIDGET(gtk_builder_get_object(builder, "wnd_sale_type")));
  gtk_widget_hide (GTK_WIDGET(gtk_builder_get_object(builder, "tipo_venta_win_venta")));

  gtk_widget_grab_focus (GTK_WIDGET (gtk_builder_get_object (builder, "barcode_entry")));

  gtk_list_store_clear (venta->store);

  CleanEntryAndLabelData();
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_total")), "");

  gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_ticket_number")),
			g_strdup_printf ("<span size=\"15000\">%.6d</span>", get_last_sell_id ()));

  clean_credit_data();

  ListClean ();
}

void
on_btn_credit_clicked (GtkButton *button, gpointer data)
{
  if (venta->header == NULL)
    {
      ErrorMSG (GTK_WIDGET (gtk_builder_get_object (builder, "barcode_entry")), "No hay productos para vender");
      return;
    }

  gtk_widget_show_all (GTK_WIDGET (builder_get (builder, "wnd_sale_credit")));

  gtk_widget_hide (GTK_WIDGET (builder_get (builder, "wnd_sale_type")));
}

void
on_btn_client_ok_clicked (GtkButton *button, gpointer data)
{
  GtkWidget *aux;
  GtkListStore *store;
  GtkTreeSelection *selection;
  GtkTreeIter iter;
  gchar *rut, *dv;
  PGresult *res;
  gchar *q;

  aux = GTK_WIDGET(gtk_builder_get_object(builder, "treeview_clients"));
  store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(aux)));
  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(aux));

  if (!(gtk_tree_selection_get_selected(selection, NULL, &iter)))
    {
      aux = GTK_WIDGET(gtk_builder_get_object(builder, "wnd_client_search"));
      AlertMSG(aux, "Debe seleccionar un cliente");
      return;
    }

  gtk_tree_model_get(GTK_TREE_MODEL(store), &iter,
                     0, &rut,
                     -1);
  aux = GTK_WIDGET(gtk_builder_get_object(builder, "wnd_client_search"));
  gtk_widget_hide(aux);

  dv = invested_strndup (rut, strlen (rut)-1);
  rut = g_strndup (rut, strlen (rut)-1);

  q = g_strdup_printf("SELECT nombre || ' ' || apell_p, direccion, telefono, giro from cliente where rut = %s", rut);
  res = EjecutarSQL(q);
  g_free (q);
  rut = g_strdup_printf ("%s-%s", rut, dv);

  if (gtk_widget_get_visible (GTK_WIDGET (builder_get (builder,"wnd_mixed_pay_step1"))))
    {
      gtk_entry_set_text (GTK_ENTRY (builder_get (builder, "entry_rut_mixed_pay")), rut);
      gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_name_mixed_pay")),
			    g_strdup_printf ("<b>%s</b>", PQgetvalue (res, 0, 0)));

      gtk_widget_grab_focus (GTK_WIDGET (builder_get (builder, "entry_int_amount_mixed_pay")));
    }
  else if (gtk_widget_get_visible (GTK_WIDGET (builder_get (builder,"wnd_mixed_pay_step2"))))
    {
      gtk_entry_set_text (GTK_ENTRY (builder_get (builder, "entry_rut_mixed_pay_2")), rut);
      gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_name_mixed_pay_2")),
			    g_strdup_printf ("<b>%s</b>", PQgetvalue (res, 0, 0)));

      gtk_widget_grab_focus (GTK_WIDGET (builder_get (builder, "entry_int_amount_mixed_pay2")));
    }
  else if (gtk_widget_get_visible (GTK_WIDGET(builder_get (builder,"wnd_sale_credit"))))
    {
      aux = GTK_WIDGET (gtk_builder_get_object(builder, "entry_credit_rut"));
      gtk_entry_set_text (GTK_ENTRY(aux), rut);

      aux = GTK_WIDGET(gtk_builder_get_object(builder, "btn_credit_sale"));
      gtk_widget_grab_focus(aux);

      fill_credit_data(rut, PQgetvalue(res, 0, 0),
		       PQvaluebycol (res, 0, "direccion"),
		       PQvaluebycol (res, 0, "telefono"));
    }
  else if (gtk_widget_get_visible (GTK_WIDGET (builder_get (builder,"wnd_sale_invoice"))))
    {
      aux = GTK_WIDGET (builder_get (builder, "entry_invoice_rut"));
      gtk_entry_set_text (GTK_ENTRY(aux), rut);

      aux = GTK_WIDGET (builder_get (builder, "lbl_invoice_name"));
      gtk_label_set_text(GTK_LABEL(aux), PQgetvalue (res, 0, 0));

      aux = GTK_WIDGET (builder_get (builder, "lbl_invoice_giro"));
      gtk_label_set_text(GTK_LABEL(aux), PQvaluebycol(res, 0, "giro"));
  
      aux = GTK_WIDGET (builder_get (builder, "lbl_direccion_factura"));
      gtk_label_set_text(GTK_LABEL(aux), PQvaluebycol(res, 0, "giro"));

      aux = GTK_WIDGET (builder_get (builder, "lbl_telefono_factura"));
      gtk_label_set_text(GTK_LABEL(aux), PQvaluebycol(res, 0, "giro"));

      aux = GTK_WIDGET (builder_get (builder, "btn_make_invoice"));
      gtk_widget_set_sensitive(aux, TRUE);

      aux = GTK_WIDGET (builder_get (builder, "btn_make_guide"));
      gtk_widget_set_sensitive (aux, TRUE);
    }
}

void
on_treeview_clients_row_activated (GtkTreeView *treeview, GtkTreePath *arg1, GtkTreeViewColumn *arg2, gpointer data)
{
  on_btn_client_ok_clicked (NULL, NULL);
}


void
on_treeview_emisor_row_activated (GtkTreeView *treeview, GtkTreePath *arg1, GtkTreeViewColumn *arg2, gpointer data)
{
  on_btn_ok_srch_emisor_clicked (NULL, NULL);
}


/**
 * Es llamada cuando se presiona el boton "btn_cancel_sale_credit" (signal clicked)
 *
 * Esta funcion cierra la ventana "wnd_sale_invoice" y llama a la funcion
 * "clean_credit_data" que limpia los datos de la venatan.
 *
 * @param button the button
 * @param user_data the user data
 */
void
on_btn_cancel_sale_credit_clicked (GtkButton *button, gpointer data)
{
  GtkWidget *widget;

  clean_credit_data();
  widget = GTK_WIDGET(gtk_builder_get_object(builder, "wnd_sale_credit"));
  gtk_widget_hide(widget);
}

/**
 * Clean the information displayed in the credit client dialog
 *
 */
void
clean_credit_data ()
{
  GtkWidget *widget;
  widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_credit_rut"));
  gtk_entry_set_text(GTK_ENTRY(widget), "");
  gtk_widget_grab_focus(widget);

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_client_name"));
  gtk_label_set_text(GTK_LABEL(widget), "");

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_client_addr"));
  gtk_label_set_text(GTK_LABEL(widget), "");

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_client_phone"));
  gtk_label_set_text(GTK_LABEL(widget), "");

  return;
}

/**
 * Es llamada cuando se presiona el boton "btn_cancel_invoice" (signal clicked)
 *
 * Esta funcion cierra la ventana "wnd_sale_invoice" y limpia la caja de
 * texto "entry_invoice_rut".
 *
 * @param button the button
 * @param user_data the user data
 */

void
on_btn_cancel_invoice_clicked (GtkButton *button, gpointer data)
{
  GtkWidget *widget;

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_invoice_rut"));
  gtk_entry_set_text(GTK_ENTRY(widget), "");
  //gtk_list_store_clear(GTK_LIST_STORE(gtk_entry_completion_get_model(gtk_entry_get_completion(GTK_ENTRY(widget)))));

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "wnd_sale_invoice"));
  clean_container (GTK_CONTAINER (widget));
  gtk_widget_hide(widget);
  set_cliente_facturacion (FALSE);
}

/**
 * Es llamada cuando se presiona el boton "btn_close_win" (signal clicked)
 * de la ventana "tipo_venta_win_venta"
 *
 * Esta funcion recalcula el total de la venta seteando el resultado en el
 * texto del label "label_total" de la ventana "wnd_sell".
 *
 * NOTA: Esto es realizado evitar la permanencia (grÃ¡fica) de la cifra de
 * venta total con descuento en caso de ser calculada y no realizar la venta
 *
 * @param button the button
 * @param user_data the user data
 */

void
on_btn_cancel_tipo_venta_win_venta (GtkButton *button, gpointer data)
{
  guint32 total;
  //Seteamos el Sub-Total y el Total
  total = llround (CalcularTotal (venta->header));

  gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_total")),
			g_strdup_printf ("<span size=\"40000\">%s</span>",
					 PutPoints (g_strdup_printf ("%u", total))));
}

/**
 * Callback conected to the btn_sale_invoice button.
 *
 * This function setup the window and the show it
 *
 * @param button the button
 * @param user_data the user data
 */
void
on_btn_invoice_clicked (GtkButton *button, gpointer user_data)
{
  GtkWidget *widget;
  GtkEntryCompletion *completion;
  GtkTreeModel *model;
  GtkTreeIter iter;
  PGresult *res;
  gint i;

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_invoice_rut"));

  //only setup the completion if it wasn't configured previously
  if (gtk_entry_get_completion(GTK_ENTRY(widget))== NULL)
    {
      completion = gtk_entry_completion_new();

      gtk_entry_set_completion(GTK_ENTRY(widget), completion);

      model = GTK_TREE_MODEL(gtk_list_store_new(1, G_TYPE_STRING));

      gtk_entry_completion_set_model(completion, model);
      gtk_entry_completion_set_text_column(completion, 0);
      gtk_entry_completion_set_minimum_key_length(completion, 3);
    }
  else
    model = gtk_entry_completion_get_model(gtk_entry_get_completion(GTK_ENTRY(widget)));

  res = EjecutarSQL("select rut || '-' || dv from cliente order by rut asc");

  gtk_list_store_clear (GTK_LIST_STORE(model));

  for (i=0 ; i < PQntuples(res) ; i++)
    {
      gtk_list_store_append(GTK_LIST_STORE(model), &iter);
      gtk_list_store_set(GTK_LIST_STORE(model), &iter,
                         0, PQgetvalue(res, i, 0),
                         -1);
    }

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "wnd_sale_invoice"));

  gtk_widget_show_all(widget);
  gtk_widget_grab_focus (GTK_WIDGET (builder_get (builder, "entry_invoice_rut")));
}

/**
 * Conected to the internal entry of the GtkComboBoxEntry that displays the rut
 * of the clients when is being sold with invoice
 *
 * @param editable The entry
 * @param user_data
 */
void
on_entry_invoice_rut_activate (GtkEntry *entry, gpointer user_data)
{
  /*
  GtkWidget *widget;
  PGresult *res;
  gchar *rut;
  gchar *q;
  gchar **rut_split;
  rut = g_strdup(gtk_entry_get_text(entry));

  rut_split = g_strsplit(rut, "-", 2);

  if (!(RutExist(rut)))
    {
      //the user with that rut does not exist so it is neccesary create a new client
      gtk_window_set_modal(GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(entry))), FALSE);
      AddClient(NULL, NULL); //raise the window to add a proveedor
      return;
    }

  q = g_strdup_printf("SELECT nombre, giro, direccion, telefono "
		      "FROM cliente WHERE rut = %s", rut_split[0]);

  res = EjecutarSQL(q);
  g_free (q);
  g_strfreev (rut_split);

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_invoice_name"));
  gtk_label_set_text(GTK_LABEL(widget), PQvaluebycol(res, 0, "nombre"));

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_invoice_giro"));
  gtk_label_set_text(GTK_LABEL(widget), PQvaluebycol(res, 0, "giro"));
  
  widget = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_direccion_factura"));
  gtk_label_set_text(GTK_LABEL(widget), PQvaluebycol(res, 0, "giro"));

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_telefono_factura"));
  gtk_label_set_text(GTK_LABEL(widget), PQvaluebycol(res, 0, "giro"));

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "btn_make_invoice"));
  gtk_widget_set_sensitive(widget, TRUE);

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "btn_make_guide"));
  gtk_widget_set_sensitive(widget, TRUE);
  */
  
  set_cliente_facturacion (TRUE);
  search_client (GTK_WIDGET (entry), NULL);
}


/**
 * This callback is connected to the btn_make_invoice button.
 *
 * This function must take care of make the sale process using a invoice
 *
 * @param button the button
 * @param data the user data
 */
void
on_btn_make_invoice_clicked (GtkButton *button, gpointer data)
{
  GtkWidget *widget;
  gchar *str_rut;
  gint rut;
  gint vendedor;
  //gint tipo_vendedor;
  gint ticket;
  gint monto;
  gint maquina;

  gchar *button_name;
  gint tipo_documento;

  button_name = g_strdup (gtk_buildable_get_name (GTK_BUILDABLE (button)));

  //tipo_vendedor = rizoma_get_value_int ("VENDEDOR");

  //amount
  widget = GTK_WIDGET (builder_get (builder, "label_total"));
  monto = atoi (CutPoints (g_strdup (gtk_label_get_text (GTK_LABEL (widget)))));

  //maquina
  maquina = rizoma_get_value_int ("MAQUINA");

  //salesman
  vendedor = user_data->user_id;

  //rut
  widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_invoice_rut"));
  str_rut = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));

  rut = atoi(strtok(g_strdup(str_rut),"-"));

  if (!(RutExist(str_rut)))
    {
      gchar *motivo;

      widget = gtk_widget_get_ancestor(GTK_WIDGET(button),GTK_TYPE_WINDOW);

      motivo = g_strdup_printf("El rut %s no existe", str_rut);
      AlertMSG(widget, motivo);
      g_free (motivo);

      return;
    }

  if (LimiteCredito (str_rut) < (DeudaTotalCliente (rut) + monto))
    {
      widget = gtk_widget_get_ancestor(GTK_WIDGET(button),GTK_TYPE_WINDOW);
      ErrorMSG (widget, "El cliente sobrepasa su limite de Credito");
      return;
    }


  if (g_str_equal (button_name, "btn_make_invoice")) {
    tipo_documento = FACTURA;
    ticket = get_ticket_number (FACTURA);
  }
  else if (g_str_equal (button_name, "btn_make_guide")) {
    tipo_documento = GUIA;
    ticket = get_ticket_number (GUIA);
  }

  SaveSell (monto, maquina, vendedor, CREDITO, str_rut, "0", ticket, tipo_documento,
            NULL, FALSE, TRUE);

  //clean the interface
  //restart the invoice dialog
  widget = GTK_WIDGET(gtk_builder_get_object (builder, "entry_invoice_rut"));
  gtk_entry_set_text (GTK_ENTRY(widget), "");
  gtk_widget_grab_focus (widget);

  gtk_list_store_clear(GTK_LIST_STORE(gtk_entry_completion_get_model(gtk_entry_get_completion(GTK_ENTRY(widget)))));

  widget = GTK_WIDGET(gtk_builder_get_object (builder, "lbl_entry_name"));
  gtk_label_set_text(GTK_LABEL(widget), "");

  widget = GTK_WIDGET(gtk_builder_get_object (builder, "lbl_entry_giro"));
  gtk_label_set_text(GTK_LABEL(widget), "");

  widget = GTK_WIDGET(gtk_builder_get_object (builder, "wnd_sale_invoice"));
  clean_container (GTK_CONTAINER (widget));
  gtk_widget_hide (widget);

  //clean the main window
  widget = GTK_WIDGET (gtk_builder_get_object (builder, "barcode_entry"));
  gtk_widget_grab_focus (widget);

  gtk_list_store_clear (venta->store);

  CleanEntryAndLabelData();

  widget = GTK_WIDGET (gtk_builder_get_object (builder, "label_total"));
  gtk_label_set_text (GTK_LABEL(widget), "");

  ListClean ();
  set_cliente_facturacion (FALSE);
}


/**
 * This function enters the sale when the amount
 * entered is enough, else show the 'wnd_mixed_pay_step2'
 * to complete. (signal-clicked)
 *
 * @param: GtkButton *button
 * @param: gpointer data
 */
void
on_btn_accept_mixed_pay_clicked (GtkButton *button, gpointer data)
{
  gint total;
  gint paga_con;
  gchar *nombre_cliente, *rut_cliente, *paga_con_txt, *cod_chk_rest_gen;

  gint maquina, vendedor, rut;
  gint tipo_documento, ticket;
  gchar **str_splited;
  gboolean cheque_restaurant;
  gboolean general;

  //Widgets de la segunda ventana
  GtkWidget *f_type_pay, *f_name, *f_rut, *f_amount, *f_code, *type_sell,
    *s_name, *s_rut, *s_amount, *w_cr; //*diferencia

  //Se obtienen todos los widget
  f_type_pay = GTK_WIDGET (builder_get (builder, "lbl_mixed_pay_type"));
  f_name = GTK_WIDGET (builder_get (builder, "lbl_mixed_pay_client_name"));
  f_rut = GTK_WIDGET (builder_get (builder, "lbl_mixed_pay_client_rut"));
  f_amount = GTK_WIDGET (builder_get (builder, "lbl_mixed_pay_first_amount"));
  type_sell = GTK_WIDGET (builder_get (builder, "radio_btn_effective_pay"));
  s_name = GTK_WIDGET (builder_get (builder, "lbl_name_mixed_pay_2"));
  s_rut = GTK_WIDGET (builder_get (builder, "entry_rut_mixed_pay_2"));
  s_amount = GTK_WIDGET (builder_get (builder, "entry_int_amount_mixed_pay2"));
  //diferencia = GTK_WIDGET (builder_get (builder, "lbl_diff_amount2"));
  w_cr = GTK_WIDGET (builder_get (builder, "radio_btn_cheques"));
  f_code = GTK_WIDGET (builder_get (builder, "entry_int_code_mixed_pay"));

  //Se recogen los datos de la primera ventana
  nombre_cliente = g_strdup (gtk_label_get_text (GTK_LABEL (builder_get (builder, "lbl_name_mixed_pay"))));
  rut_cliente = g_strdup (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_rut_mixed_pay"))));
  
  str_splited = parse_rut (rut_cliente);
  rut = atoi (str_splited[0]);
  g_free (str_splited);

  //Se ve si se seleccionó el cheque de restaurant o el credito
  cheque_restaurant = (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w_cr)) == TRUE) ? TRUE : FALSE;
  //Se ve si se se paga a cheques de restaurant en el modo general o a detalle
  general = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (builder_get (builder, "radio_btn_general")));

  //Se recogen el monto total a pagar y el monto con el que se paga en la primera ventana
  //total = atoi (CutPoints (g_strdup (gtk_label_get_text (GTK_LABEL (gtk_builder_get_object (builder, "label_total"))))));
  total = llround (CalcularTotal (venta->header));
  if (general)
    paga_con = atoi (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_int_amount_mixed_pay"))));
  else if (pago_chk_rest->header != NULL)
    paga_con = lround (calcular_total_cheques (pago_chk_rest->header));
  else
    {
      ErrorMSG (GTK_WIDGET (builder_get (builder, "entry_rut_mixed_pay")),
		"Debe agregar cheques a la lista");
      return;
    }

  /*Realiza algunas verificaciones antes de continuar con los pagos*/

  //Si la venta es a crédito, verifica que tenga el saldo suficiente
  if (cheque_restaurant == FALSE)
    {
      if (LimiteCredito (rut_cliente) < (DeudaTotalCliente (rut) + paga_con))
	{
	  gtk_widget_show (GTK_WIDGET (builder_get (builder, "msg_credit_out")));
	  return;
	}
    }
  else if (cheque_restaurant == TRUE && general == TRUE)
    {
      cod_chk_rest_gen = g_strdup (gtk_entry_get_text (GTK_ENTRY (f_code)));

      if (g_str_equal (cod_chk_rest_gen, ""))
	{
	  ErrorMSG (f_code, "Debe ingresar el código de transacción del cheque de restaurant");
	  return;
	}
      else if (DataExist (g_strdup_printf ("SELECT codigo FROM cheque_rest WHERE codigo = '%s'", cod_chk_rest_gen)))
	{
	  ErrorMSG (f_code, g_strdup_printf ("El cheque de codigo %s ya fué ingresado, ingrese uno nuevo", cod_chk_rest_gen));
	  return;
	}

      //Se crea un cheque
      add_chk_rest_to_list (cod_chk_rest_gen, "", paga_con);
    }

  paga_con_txt = PutPoints (g_strdup_printf ("%d", paga_con));
  /* Si el total es mayor al primer pago, se pide que complete la compra con otra forma de pago */
  if (total > paga_con)
    {
      //El pago no afecto debe ser menor o igual al total de los productos afectos
      if (cheque_restaurant == TRUE)
	{
	  gint monto_solo_afecto = lround (CalcularSoloAfecto (venta->header));
	  if (paga_con > monto_solo_afecto)
	    {
	      if (general == TRUE)
		{
		  limpiar_lista (); //Se tiene que volver a crear el cheque, por lo tanto de saca de la lista.
		  ErrorMSG (GTK_WIDGET (builder_get (builder, "entry_int_amount_mixed_pay")),
			    g_strdup_printf ("El pago con cheque de restaurant debe ser menor o igual a %d", monto_solo_afecto));
		}
	      else
		ErrorMSG (GTK_WIDGET (builder_get (builder, "entry_int_code_detail_mp")),
			  g_strdup_printf ("El pago con cheque de restaurant debe ser menor o igual a %d", monto_solo_afecto));
	      return;
	    }
	}

      //Se limpia el treeview de los cheques seleccionados
      gtk_list_store_clear (pago_chk_rest->store);

      //Se llena la estructura de pago mixto
      pago_mixto->tipo_pago1 = (cheque_restaurant) ? CHEQUE_RESTAURANT : CREDITO;
      pago_mixto->check_rest1 = (cheque_restaurant) ? pago_chk_rest : NULL;
      pago_mixto->rut_credito1 = (cheque_restaurant) ? NULL : rut_cliente;
      pago_mixto->monto_pago1 = paga_con;

      //Se rellenan y limpian los widgets según corresponda
      gtk_label_set_markup (GTK_LABEL (f_type_pay), g_strdup_printf ("<b>%s</b>", (cheque_restaurant) ? "CHEQUE RESTAURANT" : "CREDITO"));
      gtk_label_set_markup (GTK_LABEL (f_name), g_strdup_printf ("<b>%s</b>", nombre_cliente));
      gtk_label_set_markup (GTK_LABEL (f_rut), g_strdup_printf ("<b>%s</b>", rut_cliente));
      gtk_label_set_markup (GTK_LABEL (f_amount), g_strdup_printf ("<b>%s</b>", paga_con_txt));
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (type_sell), TRUE);
      gtk_entry_set_text (GTK_ENTRY (s_rut), "");
      gtk_label_set_text (GTK_LABEL (s_name), "");
      gtk_entry_set_text (GTK_ENTRY (s_amount), "");
      
      gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "lbl_diff_amount2")),
			    g_strdup_printf ("<span size=\"30000\"> %s </span> "
					     "<span size=\"15000\" color =\"red\">Faltante</span>",
					     PutPoints (g_strdup_printf ("%d", total-paga_con))));

      gtk_widget_set_sensitive (s_rut, FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_accept_mixed_pay2")), FALSE);

      gtk_widget_hide (GTK_WIDGET (builder_get (builder, "wnd_mixed_pay_step1")));
      gtk_widget_show (GTK_WIDGET (builder_get (builder, "wnd_mixed_pay_step2")));
      gtk_window_set_position (GTK_WINDOW (builder_get (builder, "wnd_mixed_pay_step2")), 
			       GTK_WIN_POS_CENTER_ALWAYS);
      gtk_widget_grab_focus (s_amount);
    }
  else
    {
      maquina = rizoma_get_value_int ("MAQUINA");
      vendedor = user_data->user_id;

      tipo_documento = SIMPLE;
      switch (tipo_documento)
	{
	case SIMPLE: case VENTA:
	  if (total >= 180)
	    ticket = get_ticket_number (tipo_documento);
	  else
	    ticket = -1;
	  break;
	default:
	  g_printerr("The document type could not be matched on %s", G_STRFUNC);
	  ticket = -1;
	  break;
	}
      
      //Si el pago es solamente con cheque de restaurant
      if (cheque_restaurant == TRUE)
	{
	  gint monto_solo_no_afecto = lround (CalcularSoloNoAfecto (venta->header));
	  gint monto_solo_afecto = lround (CalcularSoloAfecto (venta->header));
	  if (monto_solo_no_afecto > 0)
	    {
	      if (general == TRUE)
		{
		  limpiar_lista (); //Se tiene que volver a crear el cheque, por lo tanto de saca de la lista.
		  ErrorMSG (GTK_WIDGET (builder_get (builder, "entry_int_amount_mixed_pay")),
			    g_strdup_printf ("El pago con cheque de restaurant debe ser menor o igual a %d", monto_solo_afecto));
		}
	      else
		ErrorMSG (GTK_WIDGET (builder_get (builder, "entry_int_code_detail_mp")),
			  g_strdup_printf ("El pago con cheque de restaurant debe ser menor o igual a %d", monto_solo_afecto));
	      return;
	    }
	}

      //printf ("%s", (cheque_restaurant) ? "cheque_restaurant" : "credito");

      //Se registra la venta
      
      SaveSell (total, maquina, vendedor, (cheque_restaurant) ? CHEQUE_RESTAURANT : CREDITO, rut_cliente, "0", ticket, tipo_documento,
		NULL, FALSE, TRUE);
      
      //Se cierra y limpia todo
      ListClean ();     //Limpia la estructura de productos de venta
      limpiar_lista (); //Limpia la estructura de cheques 
      
      //Se quitan los productos de la lista de venta
      gtk_list_store_clear (venta->store);
      gtk_list_store_clear (pago_chk_rest->store);

      //Se limpian los label
      CleanEntryAndLabelData();
      gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_total")), "");

      //Se actualiza el numero de ticket de venta
      gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_ticket_number")),
			    g_strdup_printf ("<span size=\"15000\">%.6d</span>", get_last_sell_id ()));
      
      //Se cierra la ventana y se cambia el foco al entry de barcode
      gtk_widget_hide (GTK_WIDGET (builder_get (builder, "wnd_mixed_pay_step1")));
      gtk_widget_grab_focus (GTK_WIDGET (gtk_builder_get_object (builder, "barcode_entry")));
    }
}


/**
 * This function enters the sale when the amount
 * entered is enough, recording as MIXED pay
 *
 * @param: GtkButton *button
 * @param: gpointer data
 */
void
on_btn_accept_mixed_pay2_clicked (GtkButton *button, gpointer data)
{
  gint total, paga_con, primer_pago;
  
  gchar *rut_cuenta1, *rut_cuenta2;
  gchar **str_splited;
  gint rut1, rut2;
  gchar *dv1; //*dv2;

  gint maquina, vendedor;
  gint tipo_documento, ticket;
  gint tipo_pago1, tipo_pago2;

  //gboolean afecto_impuesto1, afecto_impuesto2;

  //total = atoi (CutPoints (g_strdup (gtk_label_get_text (GTK_LABEL (gtk_builder_get_object (builder, "label_total"))))));
  total = llround (CalcularTotal (venta->header));
  pago_mixto->total_a_pagar = total;

  /*-- Primer Pago --*/
  tipo_pago1 = pago_mixto->tipo_pago1;
  //afecto_impuesto1 = (tipo_pago1 == CREDITO) ? TRUE : FALSE;
  primer_pago = pago_mixto->monto_pago1;

  //Primer rut
  if (tipo_pago1 == CREDITO)
    rut_cuenta1 = pago_mixto->rut_credito1;
  else
    rut_cuenta1 = pago_mixto->check_rest1->rut_emisor;

  str_splited = parse_rut (rut_cuenta1);
  rut1 = atoi (str_splited[0]);
  dv1 = g_strdup (str_splited[1]);
  g_free (str_splited);

  printf ("%s, %d, %s\n", rut_cuenta1, rut1, dv1);
  printf ("%d\n", primer_pago);

  /*-- Segundo Pago --*/
  paga_con = atoi (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_int_amount_mixed_pay2"))));

  //Si el pago es a credito obtiene el 2do rut
  if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (builder_get (builder, "radio_btn_effective_pay"))))
    {
      //Se asegura se tener seleccionado un cliente - TODO: agregar condición al calcular
      if (g_str_equal (gtk_label_get_text (GTK_LABEL (builder_get (builder, "lbl_name_mixed_pay_2"))), ""))
	{
	  ErrorMSG (GTK_WIDGET (builder_get (builder, "entry_rut_mixed_pay_2")), "Debe seleccionar un cliente");
	  return;
	}
      
      //Obtiene el segundo rut
      rut_cuenta2 = g_strdup (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_rut_mixed_pay_2"))));

      //Obtiene el rut sin dígito verificador
      str_splited = parse_rut (rut_cuenta2);
      rut2 = atoi (str_splited[0]);
      //dv2 = g_strdup (str_splited[1]);
      g_free (str_splited);

      //verifica que el cliente tenga cupo disponible
      if (LimiteCredito (rut_cuenta2) < (DeudaTotalCliente (rut2) + paga_con))
	{
	  gtk_widget_show (GTK_WIDGET (builder_get (builder, "msg_credit_out")));
	  return;
	}

      tipo_pago2 = CREDITO;
      //afecto_impuesto2 = TRUE;
    }
  else //quiere decir que el segundo pago es en efectivo
    {
      rut_cuenta2 = g_strdup_printf ("0");
      rut2 = 0;
      //dv2 = g_strdup_printf ("0");
      tipo_pago2 = CASH;
      //afecto_impuesto2 = TRUE;
    }

  //Obtiene datos de la máquina y el vendedor
  maquina = rizoma_get_value_int ("MAQUINA");
  vendedor = user_data->user_id;

  // TODO: ese monto mínimo para imprimir la boleta debe ser editable
  tipo_documento = SIMPLE;
  switch (tipo_documento)
    {
    case SIMPLE: case VENTA:
      if (total >= 180)
  	ticket = get_ticket_number (tipo_documento);
      else
  	ticket = -1;
      break;
    default:
      g_printerr("The document type could not be matched on %s", G_STRFUNC);
      ticket = -1;
      break;
    }

  //Se guarda en la estructura pago_mixto, los datos del segundo pago
  pago_mixto->tipo_pago2 = tipo_pago2;
  pago_mixto->check_rest2 = NULL;
  pago_mixto->rut_credito2 = (tipo_pago2 == CREDITO) ? rut_cuenta2 : NULL;
  pago_mixto->monto_pago2 = paga_con;

  /* Solo el cheque de restaurant se registra tal cual como se ingresa
     (siendo igual o mayor al monto para liquidar la venta) 
     puesto que solo con éste no se da vuelto */

  if (pago_mixto->tipo_pago2 != CHEQUE_RESTAURANT)
    pago_mixto->monto_pago2 = pago_mixto->total_a_pagar - pago_mixto->monto_pago1;

  //El pago no afecto debe ser menor o igual al total de los productos afectos
  if (pago_mixto->tipo_pago2 == CHEQUE_RESTAURANT)
    {
      gint monto_solo_afecto = lround (CalcularSoloAfecto (venta->header));
      if (pago_mixto->monto_pago2 > monto_solo_afecto)
	{
	  limpiar_lista ();
	  ErrorMSG (GTK_WIDGET (builder_get (builder, "entry_int_amount_mixed_pay2")),
		    g_strdup_printf ("El pago con cheque de restaurant debe ser menor o igual a %d", monto_solo_afecto));
	  return;
	}
    }

  //Se registra la venta -- TODO: solo se le esta pasando el primer cliente, ver bien el asunto cuando es mixto
  SaveSell (total, maquina, vendedor, MIXTO, rut_cuenta1, "0", ticket, tipo_documento,
  	    NULL, FALSE, TRUE);
      
  //Se cierra y limpia todo
  CleanEntryAndLabelData();
  ListClean (); //Se debe ejecutar antes de vaciar la lista de venta (la estructura)
  limpiar_lista (); // vacía la lista de cheques de restaurant
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_total")), "");

  gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_ticket_number")),
			g_strdup_printf ("<span size=\"15000\">%.6d</span>", get_last_sell_id ()));
      
  gtk_widget_grab_focus (GTK_WIDGET (gtk_builder_get_object (builder, "barcode_entry")));
  gtk_widget_hide (GTK_WIDGET (builder_get (builder, "wnd_mixed_pay_step2")));

  //Se quitan los productos de la lista de venta (de la estructura)
  gtk_list_store_clear (venta->store);
}


/**
 * This function calculate the amount entered vs
 * the total and shows the difference in the 
 * corresponding label.
 * Also enable the corresponding widgets.
 *
 * @param: GtkEditable *editable
 * @param: gchar *new_text
 * @param: gint new_text_length
 * @param: gint *position
 * @param: gpointer user_data
 */
void
calculate_amount (GtkEditable *editable,
		  gchar *new_text,
		  gint new_text_length,
		  gint *position,
		  gpointer user_data)
{
  gchar *num;
  gint paga_con;
  gint total = atoi (CutPoints (g_strdup (gtk_label_get_text (GTK_LABEL (gtk_builder_get_object (builder, "label_total"))))));
  gint resto;
  gboolean primer_pago = gtk_widget_get_visible (GTK_WIDGET (builder_get (builder, "wnd_mixed_pay_step1")));
  gboolean segundo_pago = gtk_widget_get_visible (GTK_WIDGET (builder_get (builder, "wnd_mixed_pay_step2")));
  
  //Ve de donde sacar los montos de pago y el monto a completar
  if (primer_pago == TRUE && pago_chk_rest->header != NULL) //Si es la primera ventana, y se vende con cheques resto
    paga_con = lround (calcular_total_cheques (pago_chk_rest->header));
  else if (primer_pago == TRUE) //Se es la primera ventana y se vende a credito
    {
      num = g_strdup_printf ("%s%s", gtk_entry_get_text (GTK_ENTRY (editable)), new_text);
      paga_con = atoi (num);
    }
  else if (segundo_pago == TRUE) //Si es la segunda ventana
    {
      num = g_strdup_printf ("%s%s", gtk_entry_get_text (GTK_ENTRY (editable)), new_text);
      paga_con = atoi (num);
      total = total - atoi (CutPoints (g_strdup 
				       (gtk_label_get_text (GTK_LABEL (gtk_builder_get_object (builder, "lbl_mixed_pay_first_amount"))))));
    }

  printf ("paga con: %d\n"
	  "total: %d\n", paga_con, total);

  if (paga_con < total)
    {
      resto = total - paga_con;
      gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, (!segundo_pago) ? "lbl_diff_amount" : "lbl_diff_amount2")),
			    g_strdup_printf ("<span size=\"30000\"> %s </span> "
					     "<span size=\"15000\" color =\"red\">Faltante</span>",
					     PutPoints (g_strdup_printf ("%d", resto))));
    }
  else
    {
      resto = paga_con - total;
      gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, (!segundo_pago) ? "lbl_diff_amount" : "lbl_diff_amount2")),
			    g_strdup_printf ("<span size=\"30000\"> %s </span> "
					     "<span size=\"15000\" color =\"#04B404\">Sobrante</span>",
					     PutPoints (g_strdup_printf ("%d", resto))));

    }

  if (!segundo_pago)
    {
      if (resto == total ||
	  g_str_equal (gtk_label_get_text (GTK_LABEL (builder_get (builder, "lbl_name_mixed_pay"))), ""))
	gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_accept_mixed_pay")), FALSE);
      else
	gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_accept_mixed_pay")), TRUE);
    }
  else 
    {
      if (paga_con < total)
	gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_accept_mixed_pay2")), FALSE);
      else
	gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_accept_mixed_pay2")), TRUE);
    }
}

/**
 * This function show the "wnd_mixed_pay_step1"
 * window, clean their widgets, and grab focus 
 * on "entry_rut_mixed_pay" entry.
 */
void
mixed_pay_window (void)
{
  //GtkListStore *store;
  //GtkTreeView *treeview;
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;
  
  // Se inicializan en ventas_win ()
  /* pago_chk_rest = (PagoChequesRest *) g_malloc (sizeof (PagoChequesRest)); */
  /* pago_chk_rest->header = NULL; */
  /* pago_chk_rest->cheques = NULL; */

  // Se inicializa al momento de rellenar el treeview.
  //ChequesRestaurant *fill = pago_chk_rest->header;

  gint total = atoi (CutPoints (g_strdup (gtk_label_get_text (GTK_LABEL (gtk_builder_get_object (builder, "label_total"))))));

  if (venta->header == NULL)
    {
      ErrorMSG (GTK_WIDGET (gtk_builder_get_object (builder, "barcode_entry")), "No hay productos para vender");
      return;
    }
    
  gtk_entry_set_text (GTK_ENTRY (builder_get (builder, "entry_rut_mixed_pay")), "");
  gtk_label_set_text (GTK_LABEL (builder_get (builder, "lbl_name_mixed_pay")), "");
  gtk_entry_set_text (GTK_ENTRY (builder_get (builder, "entry_int_amount_mixed_pay")), "");
  gtk_label_set_text (GTK_LABEL (builder_get (builder, "lbl_diff_amount")), "");
  gtk_entry_set_text (GTK_ENTRY (builder_get (builder, "entry_int_code_mixed_pay")), "");
  gtk_label_set_text (GTK_LABEL (builder_get (builder, "lbl_subtotal_mixed_pay")), "");

  gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "lbl_diff_amount")),
			g_strdup_printf ("<span size=\"30000\"> %s </span> "
					 "<span size=\"15000\" color =\"red\">Faltante</span>",
					 PutPoints (g_strdup_printf ("%d", total))));
  
  //Selecciones por defecto
  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "radio_btn_cheques")), TRUE);
  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "radio_btn_general")), TRUE);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (builder_get (builder, "radio_btn_cheques")), TRUE);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (builder_get (builder, "radio_btn_general")), TRUE);

  gtk_widget_grab_focus (GTK_WIDGET (builder_get (builder, "entry_rut_mixed_pay")));

  //Oculto por defecto
  gtk_widget_hide (GTK_WIDGET (builder_get (builder, "hbox3_detail_mixed_pay")));
  gtk_widget_hide (GTK_WIDGET (builder_get (builder, "scrolledwindow1_mixed_pay")));
  gtk_widget_hide (GTK_WIDGET (builder_get (builder, "hbox4_mixed_pay")));

  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_accept_mixed_pay")), FALSE);
  
  pago_chk_rest->treeview = GTK_TREE_VIEW (gtk_builder_get_object (builder, "treeview_mixed_pay"));
  //Se inicializa el treeview - gtk_tree_view_get_model (pago_chk_rest->treeview) == NULL
  if (gtk_tree_view_get_model (pago_chk_rest->treeview) == NULL)
    {
      pago_chk_rest->treeview = GTK_TREE_VIEW (gtk_builder_get_object (builder, "treeview_mixed_pay"));

      // TreeView Providers
      pago_chk_rest->store = gtk_list_store_new (3,
						 G_TYPE_STRING, //Codigo
						 G_TYPE_STRING, //fecha Vencimiento
						 G_TYPE_INT,    //Monto
						 -1);

      gtk_tree_view_set_model (pago_chk_rest->treeview, GTK_TREE_MODEL (pago_chk_rest->store));

      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes ("Codigo", renderer,
							 "text", 0,
							 NULL);
      gtk_tree_view_append_column (pago_chk_rest->treeview, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
      gtk_tree_view_column_set_sort_column_id (column, 0);
      gtk_tree_view_column_set_resizable (column, FALSE);
      gtk_tree_view_column_set_min_width (column, 250);

      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes ("Fecha Vencimiento", renderer,
							 "text", 1,
							 NULL);
      gtk_tree_view_append_column (pago_chk_rest->treeview, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
      gtk_tree_view_column_set_sort_column_id (column, 1);
      gtk_tree_view_column_set_resizable (column, FALSE);
      gtk_tree_view_column_set_min_width (column, 250);

      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes ("Monto", renderer,
							 "text", 2,
							 NULL);
      gtk_tree_view_append_column (pago_chk_rest->treeview, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
      gtk_tree_view_column_set_sort_column_id (column, 2);
      gtk_tree_view_column_set_resizable (column, FALSE);
      gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)2, NULL);
    }

  limpiar_lista (); //Limpia la estructura de cheques
  gtk_list_store_clear (pago_chk_rest->store); //limpia la lista de cheques  
  gtk_widget_show (GTK_WIDGET (builder_get (builder, "wnd_mixed_pay_step1")));  
  gtk_window_set_position (GTK_WINDOW (builder_get (builder, "wnd_mixed_pay_step1")),
			   GTK_WIN_POS_CENTER_ALWAYS);
}


/**
 * This function shows (and hide) the corresponding widgets
 * as the selection made.
 *
 * @param: GtkToggleButton *togglebutton
 * @param: gpointer user_data
 */
void
change_ingress_mode (GtkToggleButton *togglebutton, gpointer user_data)
{
  //The general toggle button
  if (gtk_toggle_button_get_active (togglebutton))
    {
      gtk_widget_grab_focus (GTK_WIDGET (builder_get (builder, "entry_int_code_mixed_pay")));

      gtk_widget_hide (GTK_WIDGET (builder_get (builder, "hbox3_detail_mixed_pay")));
      gtk_widget_hide (GTK_WIDGET (builder_get (builder, "scrolledwindow1_mixed_pay")));
      gtk_widget_hide (GTK_WIDGET (builder_get (builder, "hbox4_mixed_pay")));

      gtk_widget_show (GTK_WIDGET (builder_get (builder, "label1_cod_mixed_pay")));
      gtk_widget_show (GTK_WIDGET (builder_get (builder, "label_amount_mixed_pay")));
      gtk_widget_show (GTK_WIDGET (builder_get (builder, "entry_int_code_mixed_pay")));      
      gtk_widget_show (GTK_WIDGET (builder_get (builder, "entry_int_amount_mixed_pay")));

      gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "radio_btn_credito")), TRUE);
    }
  else
    {
      gtk_widget_grab_focus (GTK_WIDGET (builder_get (builder, "entry_int_code_detail_mp")));
      gtk_entry_set_text (GTK_ENTRY (builder_get (builder, "entry_int_amount_mixed_pay")), "");

      gtk_widget_show (GTK_WIDGET (builder_get (builder, "hbox3_detail_mixed_pay")));
      gtk_widget_show (GTK_WIDGET (builder_get (builder, "scrolledwindow1_mixed_pay")));
      gtk_widget_show (GTK_WIDGET (builder_get (builder, "hbox4_mixed_pay")));

      gtk_widget_hide (GTK_WIDGET (builder_get (builder, "label1_cod_mixed_pay")));
      gtk_widget_hide (GTK_WIDGET (builder_get (builder, "label_amount_mixed_pay")));
      gtk_widget_hide (GTK_WIDGET (builder_get (builder, "entry_int_code_mixed_pay")));
      gtk_widget_hide (GTK_WIDGET (builder_get (builder, "entry_int_amount_mixed_pay")));

      gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "radio_btn_credito")), FALSE);
      calculate_amount (NULL, NULL, 0, NULL, NULL);
    }
}


/**
 * This function shows (and hide) the corresponding widgets
 * as the selection made.
 *
 * @param: GtkToggleButton *togglebutton
 * @param: gpointer user_data
 */
void
change_pay_mode (GtkToggleButton *togglebutton, gpointer user_data)
{
  //The "cheque restaurant" toggle button
  if (gtk_toggle_button_get_active (togglebutton))
    {
      gtk_widget_grab_focus (GTK_WIDGET (builder_get (builder, "entry_rut_mixed_pay")));
      gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "label1_cod_mixed_pay")), TRUE);
      gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "entry_int_code_mixed_pay")), TRUE);
      gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "radio_btn_detail")), TRUE);
      gtk_entry_set_text (GTK_ENTRY (builder_get (builder, "entry_rut_mixed_pay")), "");
      gtk_label_set_text (GTK_LABEL (builder_get (builder, "lbl_name_mixed_pay")), "");
    }
  else
    {
      gtk_widget_grab_focus (GTK_WIDGET (builder_get (builder, "entry_rut_mixed_pay")));
      gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "label1_cod_mixed_pay")), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "entry_int_code_mixed_pay")), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "radio_btn_detail")), FALSE);
      gtk_entry_set_text (GTK_ENTRY (builder_get (builder, "entry_rut_mixed_pay")), "");
      gtk_label_set_text (GTK_LABEL (builder_get (builder, "lbl_name_mixed_pay")), "");
    }
}


/**
 * This function add the restaurant check to treeview and
 * adds it to the "pago_chk_rest" list.
 *
 * @param: GtkButton *button
 * @param: gpointer user_data
 */
void
on_btn_add_chk_rest_clicked (GtkButton *button, gpointer user_data)
{
  GtkTreeIter iter;
  GtkWidget *codigo_w, *fecha_venc_w, *monto_w;
  gchar *codigo, *fecha_venc, *monto, *fecha_valida;
  gint subtotal;

  codigo_w = GTK_WIDGET (builder_get (builder, "entry_int_code_detail_mp"));
  fecha_venc_w = GTK_WIDGET (builder_get (builder, "entry_int_exp_date_mp"));
  monto_w = GTK_WIDGET (builder_get (builder, "entry_int_amount_detail_mp"));

  codigo = g_strdup (gtk_entry_get_text (GTK_ENTRY (codigo_w)));
  fecha_venc = g_strdup (gtk_entry_get_text (GTK_ENTRY (fecha_venc_w)));
  monto = g_strdup (gtk_entry_get_text (GTK_ENTRY (monto_w)));
  fecha_valida = GetDataByOne (g_strdup_printf ("SELECT * FROM is_valid_date('%s','DDMMYY')", fecha_venc));

  if (g_str_equal (codigo, ""))
    ErrorMSG (codigo_w, "Debe ingresar un código");
  else if (DataExist (g_strdup_printf ("SELECT codigo FROM cheque_rest WHERE codigo = '%s'", codigo)))
    ErrorMSG (codigo_w, g_strdup_printf ("El cheque de codigo %s ya fué ingresado, ingrese uno nuevo", codigo));
  else if (g_str_equal (fecha_venc, ""))
    ErrorMSG (fecha_venc_w, "Debe ingresar la fecha de vencimiento");
  else if (g_str_equal (fecha_valida, "f"))
    ErrorMSG (fecha_venc_w, "Debe ingresar una fecha válida");
  else if (g_str_equal (monto, ""))
    ErrorMSG (monto_w, "Debe el monto del cheque de restaurant");
  else
    {
      //Busca si ya se ha ingresado un cheque con ese codigo a la lista
      if (pago_chk_rest->cheques != NULL)
        pago_chk_rest->cheques_check = buscar_por_codigo (pago_chk_rest->header, codigo);
      else
        pago_chk_rest->cheques_check = NULL;

      if (pago_chk_rest->cheques_check == NULL)
	{
	  add_chk_rest_to_list (codigo, fecha_venc, atoi(monto));

	  gtk_list_store_insert_after (pago_chk_rest->store, &iter, NULL);
          gtk_list_store_set (pago_chk_rest->store, &iter,
                              0, pago_chk_rest->cheques->cheque->codigo,
                              1, pago_chk_rest->cheques->cheque->fecha_vencimiento,
                              2, pago_chk_rest->cheques->cheque->monto,
                              -1);
	  
          pago_chk_rest->cheques->cheque->iter = iter;
	  clean_container (GTK_CONTAINER (builder_get (builder, "hbox3_detail_mixed_pay")));
	  gtk_widget_grab_focus (codigo_w);

	  //Recalcular Sub TOTAL
	  subtotal = lround (calcular_total_cheques (pago_chk_rest->header));
	  printf ("Pase por aquí en agregar a la lista\n");
	  gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_subtotal_mixed_pay")), 
				g_strdup_printf ("<span size=\"23000\">%d</span>", subtotal));
	  calculate_amount (NULL, NULL, 0, NULL, NULL);

	  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "radio_btn_general")), FALSE);
	}
      else
	ErrorMSG (codigo_w, g_strdup_printf ("El cheque de código %s ya está en la lista, ingrese uno nuevo", codigo));
    }
}

/**
 * This function add the restaurant check to treeview and
 * adds it to the "pago_chk_rest" list.
 *
 * @param: GtkButton *button
 * @param: gpointer user_data
 */
void
on_btn_del_chk_rest_clicked (GtkButton *button, gpointer user_data)
{
  GtkTreeIter iter;
  GtkTreeSelection *selection;
  gchar *codigo;
  //gint position;
  gint subtotal;

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (pago_chk_rest->treeview));

  if (gtk_tree_selection_get_selected (selection, NULL, &iter) == TRUE)
    {
      gtk_tree_model_get (GTK_TREE_MODEL (pago_chk_rest->store), &iter,
                          0, &codigo,
                          -1);

      //Esta línea hace que haya un violación de segmento al momento de agregar un cheque a la lista o_Ó!! WTF!
      //position = atoi (gtk_tree_model_get_string_from_iter (GTK_TREE_MODEL (pago_chk_rest->store), &iter));

      del_chk_rest_from_list (codigo);
      gtk_list_store_remove (GTK_LIST_STORE (pago_chk_rest->store), &iter);
      subtotal = lround (calcular_total_cheques (pago_chk_rest->header));
      printf ("Pase por aquí en quitar de la lista\n");
      gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_subtotal_mixed_pay")), 
			    g_strdup_printf ("<span size=\"23000\">%d</span>", subtotal));
      calculate_amount (NULL, NULL, 0, NULL, NULL);

      //Si subtotal = 0, significa que no hay cheques en la lista
      if (subtotal == 0)
	gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "radio_btn_general")), TRUE);
    }
  //select_back_deleted_row ("treeview_mixed_pay", position);
}


/**
 * Callback connected the key-press-event in the main window.
 *
 * This function must be simple, because can lead to a performance
 * issues. It currently only handles the global keys that are
 * associated to nothing.
 *
 * @param widget the widget that emits the signal
 * @param event the event
 * @param data the user data
 *
 * @return TRUE on key captured, FALSE to let the key pass.
 */
gboolean
on_ventas_gui_key_press_event(GtkWidget   *widget,
                              GdkEventKey *event,
                              gpointer     data)
{
  switch (event->keyval)
    {
    case GDK_F2:
      if (atoi(rizoma_get_value("VENTA_DIRECTA"))) // Si VENTA_DIRECTA = 1
	  gtk_widget_grab_focus (GTK_WIDGET (gtk_builder_get_object (builder, "cantidad_entry")));
      break;

    case GDK_F5:
      if (user_data->user_id == 1)
        nullify_sale_win ();
      break;

    case GDK_F6:
      if (user_data->user_id == 1)
        VentanaEgreso(0);
      break;

    case GDK_F7:
      if (user_data->user_id == 1)
        VentanaIngreso (0);
      break;

    case GDK_F8:
      mixed_pay_window ();
      break;

      //if the key pressed is not in use let it pass
    case GDK_F10:
      on_btn_credit_clicked (NULL, NULL);
      break;
    default:
      return FALSE;
    }

  return TRUE;
}

/**
 * This function loads the nullify sale dialog.
 *
 * Here must stay all the configuration of the dialog that is needed
 * when the dialog will be showed to the user.
 */
void
nullify_sale_win (void)
{
  GtkWidget *widget;

  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  GtkTreeView *treeview_sales;
  GtkTreeSelection *selection_sales;
  GtkTreeView *treeview_details;

  GtkListStore *store_sales;
  GtkListStore *store_details;

  // Comprueba que caja está habilitada
  // Se deben poder hacer enulaciones de venatas sin caja habilitada
  /*if (rizoma_get_value_boolean ("CAJA") == 0)
    {      
      widget = GTK_WIDGET(gtk_builder_get_object(builder, "barcode_entry"));
      AlertMSG (widget, "Debe habilitar caja para realizar anulaciones de venta");
      return;
      }*/

  treeview_sales = GTK_TREE_VIEW(gtk_builder_get_object(builder, "treeview_nullify_sale"));
  store_sales = GTK_LIST_STORE(gtk_tree_view_get_model(treeview_sales));

  if (store_sales == NULL)
    {
      store_sales = gtk_list_store_new (4,
                                        G_TYPE_INT,    //id
                                        G_TYPE_STRING, //date
                                        G_TYPE_STRING, //salesman
                                        G_TYPE_INT);   //total amount

      gtk_tree_view_set_model(treeview_sales, GTK_TREE_MODEL(store_sales));

      selection_sales = gtk_tree_view_get_selection (treeview_sales);
      gtk_tree_selection_set_mode (selection_sales, GTK_SELECTION_SINGLE);
      g_signal_connect (G_OBJECT(selection_sales), "changed",
                        G_CALLBACK(on_selection_nullify_sales_change), NULL);

      //ID
      renderer = gtk_cell_renderer_text_new();
      column = gtk_tree_view_column_new_with_attributes("ID", renderer,
                                                        "text", 0,
                                                        NULL);
      gtk_tree_view_append_column (treeview_sales, column);

      //Date
      renderer = gtk_cell_renderer_text_new();
      column = gtk_tree_view_column_new_with_attributes("Fecha", renderer,
                                                        "text", 1,
                                                        NULL);
      gtk_tree_view_append_column (treeview_sales, column);

      //Salesman
      renderer = gtk_cell_renderer_text_new();
      column = gtk_tree_view_column_new_with_attributes("Vendedor", renderer,
                                                        "text", 2,
                                                        NULL);
      gtk_tree_view_append_column (treeview_sales, column);

      //Total amount
      renderer = gtk_cell_renderer_text_new();
      column = gtk_tree_view_column_new_with_attributes("Monto Total", renderer,
                                                        "text", 3,
                                                        NULL);
      gtk_tree_view_append_column (treeview_sales, column);
    }

  gtk_list_store_clear(store_sales);

  treeview_details = GTK_TREE_VIEW(gtk_builder_get_object(builder, "treeview_nullify_sale_details"));

  store_details = GTK_LIST_STORE(gtk_tree_view_get_model(treeview_details));

  if (store_details == NULL)
    {
      store_details = gtk_list_store_new (7,
                                          G_TYPE_INT,    //barcode
                                          G_TYPE_STRING, //description
                                          G_TYPE_DOUBLE, //cantity
                                          G_TYPE_INT,    //price
                                          G_TYPE_INT,    //subtotal
                                          G_TYPE_INT,    //id (detail)
                                          G_TYPE_INT);   //id_venta

      gtk_tree_view_set_model(treeview_details, GTK_TREE_MODEL(store_details));
      gtk_tree_selection_set_mode (gtk_tree_view_get_selection (treeview_details), GTK_SELECTION_NONE);

      //barcode
      renderer = gtk_cell_renderer_text_new();
      column = gtk_tree_view_column_new_with_attributes("Cod. Barras", renderer,
                                                        "text", 0,
                                                        NULL);
      gtk_tree_view_append_column (treeview_details, column);

      //description
      renderer = gtk_cell_renderer_text_new();
      column = gtk_tree_view_column_new_with_attributes("Descripcion", renderer,
                                                        "text", 1,
                                                        NULL);
      gtk_tree_view_append_column (treeview_details, column);

      //cantity
      renderer = gtk_cell_renderer_text_new();
      column = gtk_tree_view_column_new_with_attributes("Cantidad", renderer,
                                                        "text", 2,
                                                        NULL);
      gtk_tree_view_append_column (treeview_details, column);
      gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)2, NULL);

      //price
      renderer = gtk_cell_renderer_text_new();
      column = gtk_tree_view_column_new_with_attributes("Precio", renderer,
                                                        "text", 3,
                                                        NULL);
      gtk_tree_view_append_column (treeview_details, column);

      //subtotal
      renderer = gtk_cell_renderer_text_new();
      column = gtk_tree_view_column_new_with_attributes("Subtotal", renderer,
                                                        "text", 4,
                                                        NULL);
      gtk_tree_view_append_column (treeview_details, column);
    }

  gtk_list_store_clear(store_details);

  widget = GTK_WIDGET (gtk_builder_get_object(builder, "wnd_nullify_sale"));
  gtk_widget_show_all (widget);
}

/**
 * Callback connected to the search button of the nullify sale.
 *
 * This function fills the treeview of sale accordgin with the filters
 * applied by the user.
 *
 * @param button the button that emited the signal
 * @param data the user data
 */
void
on_btn_nullify_search_clicked (GtkButton *button, gpointer data)
{
  GtkTreeView *treeview;
  GtkListStore *store_sales;
  GtkTreeIter iter;
  GtkEntry *entry;
  gchar *sale_id;
  gchar *barcode;
  gchar *date;
  gchar *amount;
  gchar *q;
  gchar *condition = "";
  PGresult *res;
  gint i;
  gint tuples;
  GDate *gdate;
  gchar **date_splited;
  gchar str_date[256];

  entry = GTK_ENTRY(gtk_builder_get_object(builder, "entry_nullify_id"));

  if (g_str_equal(gtk_entry_get_text(entry), ""))
    sale_id = NULL;
  else
    sale_id = g_strdup(gtk_entry_get_text(entry));

  entry = GTK_ENTRY(gtk_builder_get_object(builder, "entry_nullify_barcode"));

  if (g_str_equal(gtk_entry_get_text(entry), ""))
    barcode = NULL;
  else
    barcode = g_strdup(gtk_entry_get_text(entry));

  entry = GTK_ENTRY(gtk_builder_get_object(builder, "entry_nullify_date"));

  if (g_str_equal(gtk_entry_get_text(entry), ""))
    date = NULL;
  else
    date = g_strdup(gtk_entry_get_text(entry));

  entry = GTK_ENTRY(gtk_builder_get_object(builder, "entry_nullify_amount"));

  if (g_str_equal(gtk_entry_get_text(entry), ""))
    amount = NULL;
  else
    amount = g_strdup(gtk_entry_get_text(entry));

  q = g_strdup ("SELECT id, date_trunc('day',fecha) as fecha, (select usuario from users where id=venta.vendedor) as usuario, monto FROM venta"
                " WHERE id not in (select id_sale from venta_anulada where id_sale=venta.id) ");

  if (sale_id != NULL)
    {
      if (atoi(sale_id) == 0)
        {
          AlertMSG(GTK_WIDGET(entry), "Debe ingresar un N° de Venta mayor a 0\nSi no sabe el numero de venta deje el campo vacío");
          return;
        }
      else
        condition = g_strconcat (condition, " AND id=", sale_id, NULL);
    }

  if (date != NULL)
    {
      GDate *date_aux;
      gchar *str_date;
      date_aux = g_date_new();

      g_date_set_parse (date_aux, date);
      str_date = g_strdup_printf("%d-%d-%d",
                                 g_date_get_year(date_aux),
                                 g_date_get_month(date_aux),
                                 g_date_get_day(date_aux));

      condition = g_strconcat (condition, " AND date_trunc('day', fecha)='", str_date, "'", NULL);
      g_date_free (date_aux);
      g_free (str_date);
    }

  if (barcode != NULL)
    {
      if (atoi(barcode) == 0)
        {
          AlertMSG(GTK_WIDGET(entry), "No puede ingresar un codigo de barras 0, o con caracteres\n"
                   "Si no sabe el código de barras deje el campo vacío");
          return;
        }
      else
        condition = g_strconcat(condition, " AND id in (select id_venta from venta_detalle where barcode=", barcode, ")", NULL);
    }

  if (amount != NULL)
    {
      if (atoi(amount) == 0)
        {
          AlertMSG(GTK_WIDGET(entry), "No puede ingresar un monto 0\nSi no sabe el monto puede dejar el campo vacio");
          return;
        }
      else
        condition = g_strconcat(condition, "AND monto=", amount, NULL);
    }

  if (!(g_str_equal(condition, "")))
    {
      q = g_strconcat (q, condition, NULL);
      g_free (condition);
    }

  res = EjecutarSQL(q);
  g_free (q);

  tuples = PQntuples (res);

  treeview = GTK_TREE_VIEW(gtk_builder_get_object(builder, "treeview_nullify_sale_details"));
  gtk_list_store_clear (GTK_LIST_STORE(gtk_tree_view_get_model(treeview)));

  treeview = GTK_TREE_VIEW(gtk_builder_get_object(builder, "treeview_nullify_sale"));
  store_sales = GTK_LIST_STORE(gtk_tree_view_get_model(treeview));
  gtk_list_store_clear (store_sales);

  for (i=0 ; i < tuples ; i++)
    {

      date_splited = g_strsplit(PQvaluebycol(res, i, "fecha"), "-", -1);

      gdate = g_date_new();

      g_date_set_year(gdate, atoi(date_splited[0]));
      g_date_set_month(gdate, atoi(date_splited[1]));
      g_date_set_day(gdate, atoi(date_splited[2]));

      g_date_strftime (str_date, sizeof(str_date), "%x", gdate);

      g_strfreev(date_splited);
      g_date_free(gdate);

      gtk_list_store_append (store_sales, &iter);
      gtk_list_store_set(store_sales, &iter,
                         0, atoi(PQvaluebycol(res, i, "id")),
                         1, str_date,
                         2, PQvaluebycol(res, i, "usuario"),
                         3, atoi(PQvaluebycol(res, i, "monto")),
                         -1);
    }
}

/**
 * Callback connected to 'changed' signal of the treeview that
 * displays the sales in the nullify sale dialog.
 *
 * This function load the details of the selected sale in the treeview details.
 *
 * @param treeselection the tree selection that emited the signal
 * @param data the user data
 */
void
on_selection_nullify_sales_change (GtkTreeSelection *treeselection, gpointer data)
{
  GtkListStore *store_sales;
  GtkListStore *store_details;
  GtkTreeView *treeview_sales;
  GtkTreeView *treeview_details;
  GtkTreeIter iter;

  PGresult *res;
  gint i;
  gint tuples;
  gchar *q;
  gint sale_id;

  treeview_sales = gtk_tree_selection_get_tree_view(treeselection);
  store_sales = GTK_LIST_STORE(gtk_tree_view_get_model(treeview_sales));

  treeview_details = GTK_TREE_VIEW(gtk_builder_get_object(builder, "treeview_nullify_sale_details"));
  store_details = GTK_LIST_STORE(gtk_tree_view_get_model(treeview_details));

  gtk_list_store_clear(store_details);

  if (!(gtk_tree_selection_get_selected(treeselection, NULL, &iter))) return;

  gtk_tree_model_get (GTK_TREE_MODEL(store_sales), &iter,
                      0, &sale_id,
                      -1);

  q = g_strdup_printf("select id, id_venta, barcode, cantidad, precio, (select descripcion from producto where barcode=venta_detalle.barcode) as descripcion, "
                      "(cantidad*precio) as subtotal from venta_detalle where id_venta=%d",
                      sale_id);

  res = EjecutarSQL(q);
  g_free (q);

  tuples = PQntuples(res);

  for (i=0 ; i<tuples ; i++)
    {
      gtk_list_store_append (store_details, &iter);
      gtk_list_store_set (store_details, &iter,
                          0, atoi(PQvaluebycol(res, i, "barcode")),
                          1, PQvaluebycol(res, i, "descripcion"),
                          2, strtod(PUT(PQvaluebycol(res, i, "cantidad")), (char **)NULL),
                          3, atoi(PQvaluebycol(res, i, "precio")),
                          4, atoi(PQvaluebycol(res, i, "subtotal")),
                          5, atoi(PQvaluebycol(res, i, "id")),
                          6, atoi(PQvaluebycol(res, i, "id_venta")),
                          -1);
    }
}

/**
 * Callback connected to cancel button in the nullify sale dialog.
 *
 * @param button the button that emited the signal
 * @param data the user data
 */
void
on_btn_nullify_cancel_clicked (GtkButton *button, gpointer data)
{
  close_nullify_sale_dialog();
}

/**
 * Callback connected to the delete-event of the nullify sale dialog.
 *
 * @param widget the widget that emited the signal
 * @param event the event wich triggered the signal
 * @param data the user data
 *
 * @return TRUE stop other handlers from being invoked
 */
gboolean
on_wnd_nullify_sale_delete_event (GtkWidget *widget, GdkEvent *event, gpointer data)
{
  close_nullify_sale_dialog();
  return TRUE;
}

/**
 * Closes the nullify sale dialog and reset the data contained
 *
 */
void
close_nullify_sale_dialog(void)
{
  GtkWidget *widget;
  GtkListStore *store;

  widget = GTK_WIDGET (gtk_builder_get_object(builder, "wnd_nullify_sale"));
  gtk_widget_hide(widget);

  widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_nullify_id"));
  gtk_entry_set_text(GTK_ENTRY(widget), "");
  gtk_widget_grab_focus(widget);

  widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_nullify_barcode"));
  gtk_entry_set_text(GTK_ENTRY(widget), "");

  widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_nullify_amount"));
  gtk_entry_set_text(GTK_ENTRY(widget), "");

  widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_nullify_date"));
  gtk_entry_set_text(GTK_ENTRY(widget), "");

  widget = GTK_WIDGET (gtk_builder_get_object(builder, "treeview_nullify_sale"));
  store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(widget)));
  gtk_list_store_clear(store);

  widget = GTK_WIDGET (gtk_builder_get_object(builder, "treeview_nullify_sale_details"));
  store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(widget)));
  gtk_list_store_clear(store);
}

/**
 * Callback connected to the accept button of the nullify sale dialog
 *
 * This function get the selected sale and nullify it.
 *
 * @param button the button that emited the signal
 * @param data the use data
 */
void
on_btn_nullify_ok_clicked (GtkButton *button, gpointer data)
{
  GtkTreeView *treeview = GTK_TREE_VIEW (builder_get (builder, "treeview_nullify_sale"));
  GtkTreeSelection *selection = gtk_tree_view_get_selection (treeview);
  GtkTreeModel *model = GTK_TREE_MODEL (gtk_tree_view_get_model (treeview));

  GtkListStore *sell = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (builder_get (builder, "sell_products_list"))));

  GtkTreeIter iter;
  gint sale_id;
  gint monto;
  gint tipo_venta, tipo_pago1, tipo_pago2;

  //gboolean is_credit_sell;
  
  PGresult *res;
  gchar *q;
  gint tuples, i;

  guint32 total;

  if (! gtk_tree_selection_get_selected (selection, NULL, &iter)) return;

  gtk_tree_model_get (model, &iter,
                      0, &sale_id,
		      3, &monto,
                      -1);

  //Se ve el tipo de venta, de ser mixta se obtienen los tipos y montos de cada pago
  tipo_venta = atoi (PQvaluebycol (EjecutarSQL (g_strdup_printf ("SELECT tipo_venta FROM venta WHERE id=%d", sale_id)), 0, "tipo_venta"));
  if (tipo_venta == MIXTO)
    {
      tipo_pago1 = atoi (PQvaluebycol (EjecutarSQL (g_strdup_printf ("SELECT tipo_pago1 FROM pago_mixto WHERE id_sale=%d", sale_id)), 0, "tipo_pago1"));
      tipo_pago2 = atoi (PQvaluebycol (EjecutarSQL (g_strdup_printf ("SELECT tipo_pago2 FROM pago_mixto WHERE id_sale=%d", sale_id)), 0, "tipo_pago2"));
      if (tipo_pago1 == CASH)
	monto = atoi (PQvaluebycol (EjecutarSQL (g_strdup_printf ("SELECT monto1 FROM pago_mixto WHERE id_sale=%d", sale_id)), 0, "monto1"));
      else if (tipo_pago2 == CASH)
	monto = atoi (PQvaluebycol (EjecutarSQL (g_strdup_printf ("SELECT monto2 FROM pago_mixto WHERE id_sale=%d", sale_id)), 0, "monto2"));
    }

  /*Se comprueba que el monto de la venta sea menor al dinero en caja*/ //TODO: Si no hay suficiente dinero, que devuelva lo que tiene
  if ( (tipo_venta == CASH || (tipo_venta == MIXTO && (tipo_pago1 == CASH || tipo_pago2 == CASH)))
       && rizoma_get_value_boolean ("CAJA") == TRUE //Si la caja esta habilitada se preocupa de ver el monto en caja
       && monto > ArqueoCaja()) //Entra aquí si la venta fué en efectivo y no hay dinero para devolver
    {
      ErrorMSG (GTK_WIDGET (gtk_builder_get_object (builder, "treeview_nullify_sale")), 
		"No existe monto suficiente en caja para anular esta venta");
      return;
    }

  if (nullify_sale (sale_id) == 0)
    {
      gtk_list_store_clear (sell);
      CleanEntryAndLabelData ();
      ListClean ();

      /*Se llama a get_sale_detail con 'FALSE' para que entregue la venta tal cual como se hizo*/
      q = g_strdup_printf ("SELECT * FROM get_sale_detail (%d, FALSE)", sale_id);
      res = EjecutarSQL (q);
      g_free (q);

      if (res != NULL && PQntuples (res) != 0)
        {
          tuples = PQntuples (res);

          for (i = 0; i < tuples; i++)
            {
              AgregarALista (NULL, PQvaluebycol (res, i, "barcode"), strtod (PUT (PQvaluebycol (res, i, "amount")), (char **)NULL));

              venta->products->product->precio = atoi (PQvaluebycol (res, i, "price"));

              gtk_list_store_insert_after (sell, &iter, NULL);
              gtk_list_store_set (sell, &iter,
                                  0, venta->products->product->codigo,
				  1, g_strdup_printf ("%s %s %d %s",
						      venta->products->product->producto,
						      venta->products->product->marca,
						      venta->products->product->contenido,
						      venta->products->product->unidad),
                                  2, g_strdup_printf ("%.3f", venta->products->product->cantidad),
                                  3, venta->products->product->precio,
                                  4, PutPoints (g_strdup_printf
                                                ("%.0f", venta->products->product->cantidad * venta->products->product->precio)),
                                  -1);

              venta->products->product->iter = iter;
            }

          total = llround (CalcularTotal (venta->header));

          gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_total")),
                                g_strdup_printf ("<span size=\"40000\">%s</span>",
                                                 PutPoints (g_strdup_printf ("%u", total))));
        }
    }
  else if (rizoma_get_value_boolean("CAJA") == 0) // Si entra en este else if significa que nunca se ha habilitado caja.
    {
      // TODO: la ventana aparece atrás, se debe lograr que se sobreponga a la ventana wnd_sell
      // Si es que no hay datos en caja, y CAJA = 0 en el .rizoma, entonces tiene que aparecer una ventana que diga:
      AlertMSG (GTK_WIDGET (gtk_builder_get_object (builder, "wnd_sell")),"Para realizar una devolución es necesario habilitar e iniciar caja con anterioridad, asegúrese de habilitar caja en el .rizoma y vuelva a ejecutar rizoma-ventas");
    }
  close_nullify_sale_dialog ();
}

/**
 * Es llamado por 'dialog_cash_box_opened', para abrir una nueva caja
 * o cerrar la apliación.
 */
void
on_dialog_cash_box_closed_response (GtkDialog *dialog, gint response_id, gpointer user_data)
{
  GtkWidget *aux_widget;

    switch (response_id)
    {
      /*Si quiere abrir una nueva caja y continuar*/
    case -8:
      open_caja (FALSE);
      break;
    case -9:
      aux_widget = GTK_WIDGET (gtk_builder_get_object (builder, "quit_message"));
      gtk_dialog_response (GTK_DIALOG(aux_widget), GTK_RESPONSE_YES);
      break;
    default:
      break;
    }

  gtk_widget_hide (GTK_WIDGET (dialog));
}

/**
 * Es llamado por 'dialog_cash_box_opened', para cerrar una caja o 
 * continuar con la que esta abierta.
 */
void
on_dialog_cash_box_opened_response (GtkDialog *dialog, gint response_id, gpointer user_data)
{
  switch (response_id)
    {
    case -8:
      CerrarCajaWin ();
      break;
    case -9:
      break;
    default:
      break;
    }

  gtk_widget_hide (GTK_WIDGET (dialog));
}


/**
 * Es llamada cuando el boton "btn_devolver" es presionado (signal click).
 *
 * Esta Funcion visualiza la ventana "wnd_devolver" (ventana para realizar
 * una devolucion de productos al proveedor).
 *
 * @param widget the widget that emits the signal
 * @param data the user data
 *
 */

void
on_btn_devolver_clicked (GtkWidget *widget, gpointer data)
{
  GtkWindow *window;

  //reviza si no hay productos
  if (venta->header == NULL)
    {
      ErrorMSG (GTK_WIDGET (gtk_builder_get_object (builder, "barcode_entry")), "No hay productos para devolver");
      return;
    }
  window = GTK_WINDOW (gtk_builder_get_object (builder, "wnd_devolver"));
  gtk_widget_grab_focus (GTK_WIDGET (builder_get (builder, "entry_proveedor")));
  clean_container (GTK_CONTAINER (window));
  gtk_widget_show_all (GTK_WIDGET (window));

}


/**
 * Es llamada cuando se presiona enter(signal actived) en el "entry_proveedor"
 * de la ventana "wnd_devolver).
 *
 * Esta Funcion visualiza la ventana "wnd_srch_provider"(que es para buscar
 * un proveedor) y ademas  carga el tree_view para luego visualizar la
 * busqueda de proveedores encontrados.
 *
 * @param entry the entry that emits the signal
 *
 */
void
on_entry_srch_provider_activate (GtkEntry *entry, gpointer user_data)
{
  GtkListStore *store;
  GtkTreeIter iter;
  PGresult *res;
  gint tuples, i;
  gchar *str_schr = g_strdup (gtk_entry_get_text (entry));
  gchar *str_axu;
  gchar *q;
  /*
    consulta a la BD, para obtener los rut's con su correspondiente digito
    verificador y los nombres  de los  proveedores
   */
  q = g_strdup_printf ("SELECT rut, dv, nombre "
                       "FROM buscar_proveedor ('%%%s%%')", str_schr);
  g_free (str_schr);

  res = EjecutarSQL (q);
  g_free (q);

  tuples = PQntuples (res);

  store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_srch_provider"))));

  gtk_list_store_clear (store);

  /*
    visualiza en treeView los proveedores
   */
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
    gtk_widget_grab_focus (GTK_WIDGET (GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_srch_provider"))));
    gtk_tree_selection_select_path (gtk_tree_view_get_selection (GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_srch_provider"))), gtk_tree_path_new_from_string ("0"));
}


/**
 * Es llamada cuando se presiona enter(signal actived) en el "entry_proveedor"
 * de la ventana "wnd_devolver".
 *
 * Esta Funcion visualiza la ventana "wnd_srch_provider"(que es para buscar
 * un proveedor) y ademas  carga el tree_view para luego visualizar la
 * busqueda de proveedores encontrados.
 *
 * @param entry the entry that emits the signal
 * @param data the user data
 *
 */
void
on_entry_provider_activate (GtkEntry *entry, gpointer user_data)
{
  GtkWindow *window;
  GtkTreeView *tree = GTK_TREE_VIEW (gtk_builder_get_object(builder, "tree_view_srch_provider"));
  GtkListStore *store;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  //gchar *srch_provider = g_strdup (gtk_entry_get_text (entry));
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
  on_entry_srch_provider_activate(entry, NULL);

  gtk_widget_show_all (GTK_WIDGET (window));
}


/**
 * Es llamada cuando se presiona enter (signal actived) en el "entry_emisor"
 * de la ventana "wnd_srch_emisor.
 *
 * Visualiza la ventana "wnd_srch_emisor" y realiza la
 * busqueda de emisores.
 *
 * @param entry the entry that emits the signal
 */
void
on_entry_srch_emisor_activate (GtkEntry *entry, gpointer user_data)
{
  GtkListStore *store;
  GtkTreeIter iter;
  PGresult *res;
  gint tuples, i;
  gchar *str_schr = g_strdup (gtk_entry_get_text (entry));
  gchar *str_axu;
  gchar *q;
  /*
    consulta a la BD, para obtener los rut's con su correspondiente digito
    verificador y los nombres  de los  proveedores
   */
  q = g_strdup_printf ("SELECT id, rut, dv, razon_social "
                       "FROM buscar_emisor ('%%%s%%')", str_schr);
  g_free (str_schr);
  res = EjecutarSQL (q);
  g_free (q);

  tuples = PQntuples (res);
  store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_srch_emisor"))));
  gtk_list_store_clear (store);

  /*
    visualiza en treeView los proveedores
   */
  for (i = 0; i < tuples; i++)
    {
      str_axu = g_strconcat (PQvaluebycol (res, i, "rut"),
                             PQvaluebycol (res, i, "dv"), NULL);

      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
			  0, atoi(PQvaluebycol (res, i, "id")),
                          1, PQvaluebycol (res, i, "razon_social"),
                          2, str_axu,
                          -1);
      g_free (str_axu);
    }

  gtk_widget_grab_focus (GTK_WIDGET (builder_get (builder, "entry_srch_emisor")));
  //gtk_widget_grab_focus (GTK_WIDGET (GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_srch_emisor"))));
  //gtk_tree_selection_select_path (gtk_tree_view_get_selection (GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_srch_emisor"))), gtk_tree_path_new_from_string ("0"));
}


/**
 * Es llamada cuando se presiona enter(signal actived) en el "entry_proveedor"
 * de la ventana "wnd_devolver".
 *
 * Esta Funcion visualiza la ventana "wnd_srch_provider"(que es para buscar
 * un proveedor) y ademas  carga el tree_view para luego visualizar la
 * busqueda de proveedores encontrados.
 *
 * @param entry the entry that emits the signal
 * @param data the user data
 *
 */
void
show_srch_emisor (GtkEntry *entry, gpointer user_data)
{
  GtkWindow *window;
  GtkTreeView *tree = GTK_TREE_VIEW (gtk_builder_get_object(builder, "tree_view_srch_emisor"));
  GtkListStore *store;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  //gchar *srch_provider = g_strdup (gtk_entry_get_text (entry));
  gchar *str_schr = g_strdup (gtk_entry_get_text (entry));


  if (gtk_tree_view_get_model (tree) == NULL )
    {
      store = gtk_list_store_new (3,
				  G_TYPE_INT,     //id
                                  G_TYPE_STRING,  //Razon Social
                                  G_TYPE_STRING); //Rut

      gtk_tree_view_set_model (GTK_TREE_VIEW (tree), GTK_TREE_MODEL (store));

      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes ("Emisor", renderer,
                                                         "text", 1,
                                                         NULL);
      gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
      gtk_tree_view_column_set_resizable (column, FALSE);

      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes ("Rut Emisor", renderer,
                                                         "text", 2,
                                                         NULL);
      gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
      gtk_tree_view_column_set_resizable (column, FALSE);
      gtk_tree_view_column_set_cell_data_func (column, renderer, control_rut, (gpointer)2, NULL);
    }

  window = GTK_WINDOW (gtk_builder_get_object (builder, "wnd_srch_emisor"));
  gtk_entry_set_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_srch_emisor")), str_schr);
  on_entry_srch_emisor_activate (entry, NULL);

  gtk_widget_show_all (GTK_WIDGET (window));
}


/**
 * This function enters the sale when the amount
 * entered is enough, else show the 'wnd_mixed_pay_step2'
 * to complete. (signal-clicked)
 *
 * @param: GtkButton *button
 * @param: gpointer data
 */
void
on_entry_rut_mixed_pay_activate (GtkEntry *entry, gpointer data)
{
  if (gtk_widget_get_visible (GTK_WIDGET (builder_get (builder,"wnd_mixed_pay_step1"))))
    {
      if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (builder_get (builder, "radio_btn_cheques"))))
	show_srch_emisor (entry, data);
      else
	search_client (GTK_WIDGET (entry), data);
    }
  else if (gtk_widget_get_visible (GTK_WIDGET (builder_get (builder,"wnd_mixed_pay_step2"))))
    search_client (GTK_WIDGET (entry), data);
}


void
on_btn_ok_srch_emisor_clicked (GtkButton *button, gpointer user_data)
{
  GtkWidget *aux;
  GtkListStore *store;
  GtkTreeSelection *selection;
  GtkTreeIter iter;
  gchar *rut, *dv, *razon_social;
  gint id;
  //PGresult *res;
  //gchar *q;

  aux = GTK_WIDGET(gtk_builder_get_object(builder, "tree_view_srch_emisor"));
  store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(aux)));
  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(aux));

  if (!(gtk_tree_selection_get_selected(selection, NULL, &iter)))
    {
      aux = GTK_WIDGET(gtk_builder_get_object(builder, "entry_srch_emisor"));
      AlertMSG (aux, "Debe seleccionar un cliente");
      return;
    }

  //Obtengo los datos de la fila seleccionada
  gtk_tree_model_get (GTK_TREE_MODEL(store), &iter,
		      0, &id,
		      1, &razon_social,
		      2, &rut,
		      -1);

  dv = invested_strndup (rut, strlen (rut)-1);
  rut = g_strndup (rut, strlen (rut)-1);
  rut = g_strconcat (rut, "-", dv, NULL);

  //Cierro la ventana se búsqueda del emisor
  aux = GTK_WIDGET(gtk_builder_get_object(builder, "wnd_srch_emisor"));
  gtk_widget_hide(aux);

  //Seteo la información obtenida en los entry's y labels correspondientes
  gtk_entry_set_text (GTK_ENTRY (builder_get (builder, "entry_rut_mixed_pay")), rut);

  gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_name_mixed_pay")),
			g_strdup_printf ("<b>%s</b>", razon_social));
  
  pago_chk_rest->id_emisor = id;
  pago_chk_rest->rut_emisor = g_strdup (rut);

  if (gtk_widget_get_visible (GTK_WIDGET (builder_get (builder,"entry_int_code_mixed_pay"))))
    gtk_widget_grab_focus (GTK_WIDGET (builder_get (builder, "entry_int_code_mixed_pay")));
  else
    gtk_widget_grab_focus (GTK_WIDGET (builder_get (builder, "entry_int_code_detail_mp")));
}


/**
 * Es llamada por la funcion on_btn_ok_srch_provider_clicked() y recibe el
 * parametro gchar rut.
 *
 * Esta funcion con el parametro de entrada rut realiza una consulta a la BD,
 * para obtener el nombre del proveedor y luego visualiza (set text) los
 * datos en los respctivas etiquetas (label)
 *
 * @param rut the rut of the proveedor
 *
 */
void
FillProveedorData (gchar *rut)
{
  PGresult *res;

  res = EjecutarSQL (g_strdup_printf ("SELECT nombre, rut FROM select_proveedor('%s')", rut));

  gtk_entry_set_text (GTK_ENTRY (builder_get (builder, "entry_proveedor")), PQvaluebycol (res, 0, "nombre"));

  gtk_label_set_markup (GTK_LABEL (builder_get (builder, "label_proveedor_rut")),
			g_strdup_printf ("<span weight=\"ultrabold\">%s</span>", rut));

  gtk_label_set_markup (GTK_LABEL (builder_get (builder, "label_proveedor_nom")),
			g_strdup_printf ("<span weight=\"ultrabold\">%s</span>",
					 PQvaluebycol (res, 0, "nombre")));

  gtk_widget_grab_focus (GTK_WIDGET (builder_get (builder, "btn_devolucion")));
}


/**
 * Es llamada cuando el boton "btn_devolucion" es presionado (signal click).
 *
 * Esta funcion llama a la funcion SaveDevolucion()(que registra en la BD la
 * devolucion(tablas devolucion y devolucion_detalle)) ademas de limpiar
 * treeview de los productos vendidos, la etiqueta(label) total, y el numero
 * de ventas.
 *
 * @param button the button
 * @param user_data the user data
 */
void
on_btn_devolucion_clicked (GtkButton *button, gpointer data)
{

  gint monto = atoi (CutPoints (g_strdup (gtk_label_get_text
                                          (GTK_LABEL (gtk_builder_get_object (builder, "label_total"))))));
  gint rut = atoi (CutPoints (g_strdup  (gtk_label_get_text
                                         (GTK_LABEL (gtk_builder_get_object (builder, "label_proveedor_rut"))))));

  if (g_str_equal(gtk_entry_get_text(GTK_ENTRY (gtk_builder_get_object (builder, "entry_proveedor"))), ""))
    {
      gtk_widget_grab_focus (GTK_WIDGET (builder_get (builder, "entry_proveedor")));
      AlertMSG (GTK_WIDGET (gtk_builder_get_object (builder, "wnd_devolver")),"No Ingreso un Proveedor");
      //gtk_widget_grab_focus (GTK_WIDGET (builder_get (builder, "entry_proveedor")));
      return;
    }

  if (!DataExist (g_strdup_printf ("SELECT rut FROM proveedor WHERE rut=%d",rut)))
    {
      gtk_widget_grab_focus (GTK_WIDGET (builder_get (builder, "entry_proveedor")));
      AlertMSG (GTK_WIDGET (gtk_builder_get_object (builder, "wnd_devolver")),"El rut no existe");
      //gtk_widget_grab_focus (GTK_WIDGET (builder_get (builder, "entry_proveedor")));
      return;
    }

  else
    {
      SaveDevolucion (monto,rut);

      gtk_widget_hide (gtk_widget_get_toplevel (GTK_WIDGET (button)));
      gtk_widget_grab_focus (GTK_WIDGET (gtk_builder_get_object (builder, "barcode_entry")));

      gtk_list_store_clear (venta->store);

      CleanEntryAndLabelData ();

      gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_total")), "");

      gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_ticket_number")),
			    g_strdup_printf ("<span size=\"15000\">%.6d</span>", get_last_sell_id ()));

      ListClean ();
    }
}


/**
 * Es llamada por la funcion realizar_traspaso_Env()
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
  //gint venta_id;

  gtk_widget_grab_focus (GTK_WIDGET (gtk_builder_get_object (builder, "comboboxDestino")));
  //gint total = atoi (CutPoints (g_strdup (gtk_label_get_text (GTK_LABEL (gtk_builder_get_object (builder, "label_total"))))));
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_monto_total")),
                      g_strdup_printf ("%f",TotalPrecioCompra(venta->header)));
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_origen")),(gchar *)ReturnNegocio());
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "labelID")),g_strdup_printf ("%d",InsertIdTraspaso()+1));
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_vendedor")),
                                 g_strdup (gtk_label_get_text ( GTK_LABEL  (gtk_builder_get_object (builder,"label_seller_name")))));

  res = EjecutarSQL (g_strdup_printf ("SELECT id,nombre FROM bodega "
                                      "WHERE nombre!=(SELECT nombre FROM negocio) AND estado=true"));
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

  gtk_widget_grab_focus (GTK_WIDGET (gtk_builder_get_object (builder, "comboboxDestino")));
  gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 0);
}


/**
 * Es llamada cuando se selecciona un proveedor del TreeView a traves de un
 * enter (signal row-actived) o presionando el boton de la ventana "wnd_devolver"
 * (signal clicked).
 *
 * Esta funcion extrae lo seleccionado en TreeView y lo carga en strs y se
 * envia en la funcion FillProveedorData().
 *
 * @param TreeView the tree that emits the signal
 *
 */
void
on_btn_ok_srch_provider_clicked (GtkTreeView *tree)
{
  GtkTreeSelection *selection = gtk_tree_view_get_selection (tree);
  GtkTreeModel *model = gtk_tree_view_get_model (tree);
  GtkTreeIter iter;
  gchar *str;
  gchar **strs;


  if (gtk_tree_selection_get_selected (selection, NULL, &iter) == TRUE)
    {
      gtk_tree_model_get (model, &iter,
                          1, &str,
                          -1);

      strs = g_strsplit (str, "-", 2);

      FillProveedorData (*strs);

      gtk_widget_hide (GTK_WIDGET (builder_get (builder, "wnd_srch_provider")));
    }
}


/**
 * Es llamada cuando el boton "btn_traspaso_enviar" es presionado (signal click).
 *
 * Esta funcion visualiza la ventana "traspaso_enviar_win" y ademas llama a
 * la funcion DatosEnviar().
 *
 * @param widget the widget that emits the signal
 * @param data the user data
 *
 */
void
realizar_traspaso_Env (GtkWidget *widget, gpointer data)
{
  GtkWindow *window;

  //gchar *tipo_vendedor = rizoma_get_value ("VENDEDOR");

  /*Comprueba que hallan productos añadidos en el treView para traspasar */

  if (venta->header == NULL)
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

      return;
    }
}

/**
 * Es llamada cuando el boton "traspaso_button" es presionado (signal click).
 *
 * Esta funcion visualiza la ventana "traspaso_enviar_win".
 *
 * @param widget the widget that emits the signal
 * @param data the user data
 *
 */
void
on_enviar_button_clicked (GtkButton *button, gpointer data)
{
  gint destino;
  GtkTreeIter iter;
  GtkWidget *combo;
  GtkTreeModel *model;
  gint active;
  gint vendedor = user_data->user_id;
  /*gint monto = atoi (CutPoints (g_strdup (gtk_label_get_text
    (GTK_LABEL (gtk_builder_get_object (builder, "label_total"))))));*/
  gint id_traspaso;
  gchar *nombre_origen, *nombre_destino;

  combo = GTK_WIDGET (gtk_builder_get_object(builder, "comboboxDestino"));
  active = gtk_combo_box_get_active (GTK_COMBO_BOX (combo));

  nombre_origen = g_strdup (gtk_label_get_text (GTK_LABEL (builder_get (builder, "label_origen"))));

  /* Verifica si se selecciono un destino del combobox*/
  if (active == -1)
    ErrorMSG (combo, "Debe Seleccionar un Destino");

  else
    {
      model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));
      gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combo), &iter);

      gtk_tree_model_get (model, &iter,
                          0, &destino,
			  1, &nombre_destino,
                          -1);

      id_traspaso = SaveTraspaso (TotalPrecioCompra(venta->header),
				  ReturnBodegaID(ReturnNegocio()),
				  vendedor,
				  destino,
				  TRUE);
      
      PrintValeTraspaso (venta->header, id_traspaso, TRUE, nombre_origen, nombre_destino);

      gtk_widget_hide (gtk_widget_get_toplevel (GTK_WIDGET (button)));

      gtk_widget_hide (GTK_WIDGET(gtk_builder_get_object(builder, "traspaso_enviar_win")));

      gtk_widget_grab_focus (GTK_WIDGET (gtk_builder_get_object (builder, "barcode_entry")));

      gtk_list_store_clear (venta->store);

      CleanEntryAndLabelData ();

      gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_total")), "");

      gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_ticket_number")),
			    g_strdup_printf ("<span size=\"15000\">%.6d</span>", get_last_sell_id ()));


      ListClean ();

    }
  return;
}
