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
#include"boleta.h"
#include"config_file.h"

#include"utils.h"
#include"encriptar.h"
#include"factura_more.h"
#include"rizoma_errors.h"
#include"proveedores.h"
#include"caja.h"

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

  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "product_label")),
                      g_strdup_printf ("%s  %s  %s %s", marca, descripcion, contenido, unidad));

  if (g_ascii_strtod (stock, NULL) <= GetMinStock (barcode))
    gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_stockday")),
                          g_strdup_printf("<span foreground=\"red\"><b>%.2f dia(s)</b></span>",
                                          g_strtod (PUT (stock_day), NULL)));
  else
    gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_stockday")),
                          g_strdup_printf ("<b>%.2f dia(s)</b>", g_strtod (PUT (stock_day), NULL)));

  //precio
  gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_precio")),
                        g_strdup_printf ("<span weight=\"ultrabold\">%s</span>",
                                         PutPoints (precio)));

  //precio de mayorista
  gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_mayor")),
                        g_strdup_printf ("<span weight=\"ultrabold\">%s</span>",
                                         PutPoints (precio_mayor)));

  gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_mayor_cantidad")),
                        g_strdup_printf ("<span weight=\"ultrabold\">%s</span>",
                                         PutPoints (cantidad_mayor)));

  gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_stock")),
                        g_strdup_printf ("<span weight=\"ultrabold\">%.2f</span>",
                                         g_strtod (PUT (stock), NULL)));

  str_aux = g_strdup(gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "cantidad_entry"))));
  str_aux = g_strdup_printf ("%.0f", g_strtod (PUT (str_aux), NULL) * atoi (precio));
  gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_subtotal")),
                        g_strdup_printf ("<span weight=\"ultrabold\">%s</span>", str_aux));

  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "codigo_corto")), codigo_corto);
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
          gdouble cantidad = strtod (g_strdup (gtk_entry_get_text (GTK_ENTRY (canje_cantidad))),
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

  label = gtk_label_new ("CÃ³digo de Barras: ");
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
  gtk_widget_show (label);

  canje_entry = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (hbox), canje_entry, FALSE, FALSE, 3);
  gtk_widget_show (canje_entry);

  g_signal_connect (G_OBJECT (canje_entry), "activate",
                    G_CALLBACK (SearchBarcodeProduct), (gpointer)FALSE);

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
  gint h, w;
  gint x, y;
  gint button_y, button_x;
  gboolean toggle = gtk_toggle_button_get_active (widget);

  if (toggle == TRUE)
    {
      gdk_window_get_origin (GTK_WIDGET (widget)->window, &x, &y);

      gtk_widget_size_request (GTK_WIDGET (widget), &req);
      h = req.height;
      w = req.width;

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

  venta = (Venta *) g_malloc (sizeof (Venta));
  venta->header = NULL;
  venta->products = NULL;
  venta->window = NULL;
  Productos *fill = venta->header;

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

  // check if the window must be set to fullscreen
  if (rizoma_get_value_boolean("FULLSCREEN"))
    gtk_window_maximize(GTK_WINDOW(ventas_gui));

  gtk_widget_show_all (ventas_gui);

  gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_seller_name")),
                        g_strdup_printf ("<b><big>%s</big></b>", user_data->user));

  gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_ticket_number")),
                        g_strdup_printf ("<b><big>%.6d</big></b>", get_ticket_number (SIMPLE)));

  venta->store =  gtk_list_store_new (8,
                                      G_TYPE_STRING,
                                      G_TYPE_STRING,
                                      G_TYPE_STRING,
                                      G_TYPE_INT,
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
                              1, fill->product->producto,
                              2, fill->product->marca,
                              3, fill->product->contenido,
                              4, fill->product->unidad,
                              5, g_strdup_printf ("%.3f", fill->product->cantidad),
                              6, precio,
                              7, PutPoints (g_strdup_printf ("%.0f", fill->product->cantidad * precio)),
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
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Descripción", renderer,
                                                     "text", 1,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (venta->treeview_products), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Marca", renderer,
                                                     "text", 2,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (venta->treeview_products), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Cont.", renderer,
                                                     "text", 3,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (venta->treeview_products), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Uni.", renderer,
                                                     "text", 4,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (venta->treeview_products), column);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Cant.", renderer,
                                                     "text", 5,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (venta->treeview_products), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Precio Uni.", renderer,
                                                     "text", 6,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (venta->treeview_products), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Sub Total", renderer,
                                                     "text", 7,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (venta->treeview_products), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

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

}

gboolean
SearchProductByCode (void)
{
  gchar *codigo = g_strdup (gtk_label_get_text (GTK_LABEL (gtk_builder_get_object (builder, "codigo_corto"))));
  gdouble stock = 0;
  PGresult *res;
  gint venta_directa = atoi(rizoma_get_value("VENTA_DIRECTA"));

  res = EjecutarSQL (g_strdup_printf ("SELECT * FROM informacion_producto_venta (0, '%s')", codigo));

  if (res != NULL && PQntuples (res) != 0)
    {
      if (PQntuples(res) > 1)
        g_printerr("%s: the plpgsql function informacion_producto_venta(0,'%s') returned more than 1 tuple",
                   G_STRFUNC, codigo);

      stock = g_strtod (PUT (PQvaluebycol (res, 0, "stock")), NULL);


      // TODO: Crear una busqueda de productos que tengan estado t para barcode y codigo corto (para todo el programa)
      if (strcmp (PQvaluebycol (res, 0, "estado"),"f") == 0)
	{
	  GtkWidget *aux_widget;
          aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "ventas_gui"));
          gchar *str = g_strdup_printf("El código %s fué invalidado por el administrador", codigo);
	  CleanSellLabels();
          AlertMSG (aux_widget, str);
          g_free (str);

          return FALSE;
	}
      else if (stock <= 0)
        {
          GtkWidget *aux_widget;
          aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "ventas_gui"));
          gchar *str = g_strdup_printf("El producto %s no tiene stock", codigo);
	  CleanSellLabels();
          AlertMSG (aux_widget, str);
          g_free (str);

          return FALSE;
        }

      mayorista = strcmp (PQvaluebycol (res, 0, "mayorista"), "t") == 0 ? TRUE : FALSE;

      FillProductSell (PQvaluebycol (res, 0, "barcode"), mayorista,  PQvaluebycol (res, 0, "marca"), PQvaluebycol (res, 0, "descripcion"),
                       PQvaluebycol (res, 0, "contenido"), PQvaluebycol (res, 0, "unidad"),PQvaluebycol (res, 0, "stock"),
                       PQvaluebycol (res, 0, "stock_day"), PQvaluebycol (res, 0, "precio"), PQvaluebycol (res, 0, "precio_mayor"),
                       PQvaluebycol (res, 0, "cantidad_mayor"), PQvaluebycol (res, 0, "codigo_corto"));

      if (PQvaluebycol (res, 0, "precio") != 0)
        {
          if (venta_directa == 1)
	    {
	      if (VentaFraccion (PQvaluebycol (res, 0, "barcode")))
		{
		  gtk_widget_grab_focus (GTK_WIDGET (gtk_builder_get_object (builder, "cantidad_entry")));
		  gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "sell_add_button")), TRUE);
		}
	      else
		{
		  AgregarProducto( NULL, NULL );
		}
          }
	  else
	    {
	      gtk_widget_grab_focus (GTK_WIDGET (gtk_builder_get_object (builder, "cantidad_entry")));
	      gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "sell_add_button")), TRUE );
	    }
        }
      else
        gtk_widget_grab_focus (GTK_WIDGET (gtk_builder_get_object (builder, "barcode_entry")));

      return TRUE;
    } // if (res != NULL && PQntuples (res) != 0)
  else
    {
      if (strcmp (codigo, "") != 0)
        {
          AlertMSG (GTK_WIDGET (gtk_builder_get_object (builder, "barcode_entry")), g_strdup_printf
                    ("No existe un producto con el código %s!!", codigo));
          CleanSellLabels ();
        }
      else
        {
          CleanSellLabels ();
        }
      return FALSE;
    }
}

gboolean
AgregarProducto (GtkButton *button, gpointer data)
{
  gchar *codigo = g_strdup (gtk_label_get_text (GTK_LABEL (gtk_builder_get_object (builder, "codigo_corto"))));
  gchar *barcode = g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "barcode_entry"))));
  guint32 total;
  gdouble stock = GetCurrentStock (barcode);
  gdouble cantidad;
  GtkTreeIter iter;
  GtkWidget *aux_widget;

  cantidad = g_strtod (PUT(g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "cantidad_entry"))))), NULL);

  if (cantidad <= 0)
    {
      aux_widget = GTK_WIDGET(gtk_builder_get_object (builder, "cantidad_entry"));
      AlertMSG (aux_widget, "No puede vender una cantidad 0 o menor");
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
      AlertMSG (aux_widget, "No puede vender más productos de los que tiene en stock");

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
                              1, venta->products->product->producto,
                              2, venta->products->product->marca,
                              3, venta->products->product->contenido,
                              4, venta->products->product->unidad,
                              5, g_strdup_printf ("%.3f", venta->products->product->cantidad),
                              6, precio,
                              7, PutPoints (g_strdup_printf
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
                              5, g_strdup_printf ("%.3f", venta->product_check->product->cantidad),
                              6, precio,
                              7, PutPoints (g_strdup_printf
                                            ("%.0f", venta->product_check->product->cantidad *
                                             precio)),
                              -1);

        }
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
 * Es llamada por las funciones SearchBarcodeProduct y SearchProductByCode.
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
}

/*
 * Es llamada por las funciones SearchBarcodeProduct y SearchProductByCode.
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
                        g_strdup_printf ("<b><big>%.6d</big></b>", get_ticket_number (SIMPLE)));

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
       (VentaFraccion (gtk_entry_get_text (GTK_WIDGET (gtk_builder_get_object (builder, "barcode_entry"))))) )) 
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
  gdouble cantidad = g_strtod (PUT (g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "cantidad_entry"))))), (gchar **)NULL);
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
                            g_strdup_printf ("<span weight=\"ultrabold\">%s</span>",
                                             PutPoints (g_strdup_printf ("%u", subtotal))));
    }
  else
    if (precio_mayor != 0 && mayorista == TRUE && cantidad >= cantidad_mayor)
      {
        subtotal = llround ((gdouble)cantidad * precio_mayor);

        gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_subtotal")),
                              g_strdup_printf ("<span weight=\"ultrabold\">%s</span>",
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

      SearchProductByCode ();

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

gint
SearchBarcodeProduct (GtkWidget *widget, gpointer data)
{
  gchar *barcode = g_strdup (gtk_entry_get_text (GTK_ENTRY (widget)));

  if (barcode == NULL)
    {
      GtkWidget *widgetEntry;
      widgetEntry = GTK_WIDGET(gtk_builder_get_object(builder, "barcode_entry"));
      barcode = g_strdup (gtk_entry_get_text (GTK_ENTRY (widgetEntry)));
    }

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

  if (strlen (barcode) <= 5)
    {
      gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "codigo_corto")), barcode);

      SearchProductByCode ();

      return 0;
    }

  gtk_entry_set_text (GTK_ENTRY (widget), barcode);

  q = g_strdup_printf ("SELECT * FROM informacion_producto_venta (%s,'')", barcode);
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
          AlertMSG (widget, g_strdup_printf
                    ("No existe un producto con el código de barras %s!!", barcode));

          if (ventas != FALSE)
            CleanSellLabels ();
        }
      else // TODO: Verificar utilidad de los else
        if (GetCurrentStock (barcode) == 0)
          {
            AlertMSG (widget, "No hay mercadería en Stock.\nDebe ingresar mercadería");

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
  if (g_ascii_strtod(PQvaluebycol(res, 0, "stock"), NULL) <= 0)
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

  FillProductSell (barcode, mayorista,  PQvaluebycol (res, 0, "marca"), PQvaluebycol (res, 0, "descripcion"),
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
  gboolean fraccion = VentaFraccion (gtk_entry_get_text (GTK_WIDGET (gtk_builder_get_object (builder, "barcode_entry"))));
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
  GtkTreeView *tree = GTK_TREE_VIEW (gtk_builder_get_object (builder, "ventas_search_treeview"));
  GtkTreeIter iter;
  GtkListStore *store;

  string = g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "ventas_buscar_entry"))));
  store = GTK_LIST_STORE (gtk_tree_view_get_model (tree));

  if (!(g_str_equal (string, "")))
    {
      q = g_strdup_printf ("SELECT * FROM buscar_producto ('%s', "
                           "'{\"barcode\", \"codigo_corto\",\"marca\","
                           "\"descripcion\"}', true, true )", string);
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
                              6, g_strdup_printf("%.3f", g_ascii_strtod (PQvaluebycol (res, i, "stock"), NULL)),
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
          gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "product_label")), product);

          gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_precio")),
                                g_strdup_printf ("<span weight=\"ultrabold\">%s</span>",
                                                 PutPoints (g_strdup_printf ("%d", precio))));
          gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_subtotal")),
                                g_strdup_printf ("<span weight=\"ultrabold\">%s</span>",
                                                 PutPoints (g_strdup_printf ("%u", atoi (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "cantidad_entry")))) *
                                                                             atoi (CutPoints (g_strdup (gtk_label_get_text (GTK_LABEL (gtk_builder_get_object (builder, "label_precio"))))))))));

          gtk_entry_set_text (GTK_ENTRY (gtk_builder_get_object (builder, "barcode_entry")), barcode);

          SearchBarcodeProduct (GTK_WIDGET (gtk_builder_get_object (builder, "barcode_entry")), (gpointer)TRUE);
        }
      CloseBuscarWindow (NULL, (gpointer)TRUE);
    }
}

void
Descuento (GtkWidget *widget, gpointer data)
{
  const gchar *widget_name = gtk_buildable_get_name (widget);

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

  PGresult *res;

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
          res = EjecutarSQL (q);
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

  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_id_vendedor"));
  gtk_entry_set_text(GTK_ENTRY(aux_widget), "");

  aux_widget = GTK_WIDGET (gtk_builder_get_object (builder, "barcode_entry"));
  gtk_widget_grab_focus (aux_widget);
}

void
ChangeSeller (GtkWidget *widget, gpointer data)
{
  GtkEntry *entry = GTK_ENTRY(gtk_builder_get_object(builder, "entry_id_vendedor"));
  gint id = atoi (gtk_entry_get_text (entry));
  gchar *user_name;

  user_name = ReturnUsername (id);

  if (user_name != NULL)
    {
      user_data->user_id = id;
      user_data->user = ReturnUsername (id);
      user_data->level = ReturnUserLevel (user_name);

      gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_seller_name")),
                            g_strdup_printf ("<b><big>%s</big></b>", user_data->user));

      CloseWindowChangeSeller (widget, NULL);
    }
  else
    {
      ErrorMSG (widget, g_strdup_printf
                ("No existe un usuario con el indentificador %d", id));
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
  gtk_widget_grab_focus (GTK_WIDGET (builder_get (builder, "entry_id_vendedor")));
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
                        g_strdup_printf ("<b><big>%.6d</big></b>", get_ticket_number (SIMPLE)));

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
  gchar *rut;
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

  aux = GTK_WIDGET(gtk_builder_get_object(builder, "entry_credit_rut"));
  gtk_entry_set_text(GTK_ENTRY(aux), rut);

  aux = GTK_WIDGET(gtk_builder_get_object(builder, "btn_credit_sale"));
  gtk_widget_grab_focus(aux);

  q = g_strdup_printf("SELECT nombre || ' ' || apell_p, direccion, telefono from cliente where rut = %s",
                      strtok(g_strdup(rut),"-"));
  res = EjecutarSQL(q);
  g_free (q);

  fill_credit_data(rut, PQgetvalue(res, 0, 0),
                   PQvaluebycol(res, 0, "direccion"),
                   PQvaluebycol(res, 0, "telefono"));
}

void
on_treeview_clients_row_activated (GtkTreeView *treeview, GtkTreePath *arg1, GtkTreeViewColumn *arg2, gpointer data)
{
  on_btn_client_ok_clicked (NULL, NULL);
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
  gtk_list_store_clear(GTK_LIST_STORE(gtk_entry_completion_get_model(gtk_entry_get_completion(GTK_ENTRY(widget)))));

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "wnd_sale_invoice"));
  gtk_widget_hide(widget);
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

  q = g_strdup_printf("select nombre, giro from cliente where rut = %s", rut_split[0]);

  res = EjecutarSQL(q);
  g_free (q);
  g_strfreev (rut_split);

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_invoice_name"));
  gtk_label_set_text(GTK_LABEL(widget), PQvaluebycol(res, 0, "nombre"));

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_invoice_giro"));
  gtk_label_set_text(GTK_LABEL(widget), PQvaluebycol(res, 0, "giro"));

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "btn_make_invoice"));
  gtk_widget_set_sensitive(widget, TRUE);
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
  gint tipo_vendedor;
  gint ticket;
  gint monto;
  gint maquina;

  tipo_vendedor = rizoma_get_value_int ("VENDEDOR");

  //amount
  widget = GTK_WIDGET(gtk_builder_get_object(builder, "label_total"));
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

  ticket = get_ticket_number (FACTURA);

  SaveSell (monto, maquina, vendedor, CASH, str_rut, "0", ticket, FACTURA,
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
  gtk_widget_hide (widget);

  //clean the main window
  widget = GTK_WIDGET (gtk_builder_get_object (builder, "barcode_entry"));
  gtk_widget_grab_focus (widget);

  gtk_list_store_clear (venta->store);

  CleanEntryAndLabelData();

  widget = GTK_WIDGET (gtk_builder_get_object (builder, "label_total"));
  gtk_label_set_text (GTK_LABEL(widget), "");

  ListClean ();

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
  if (rizoma_get_value_boolean ("CAJA") == 0)
    {      
      widget = GTK_WIDGET(gtk_builder_get_object(builder, "barcode_entry"));
      AlertMSG (widget, "Debe habilitar caja para realizar anulaciones de venta");
      return;
    }

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

      //date
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
                                          G_TYPE_FLOAT,  //cantity
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
                          2, g_ascii_strtod(PQvaluebycol(res, i, "cantidad"), NULL),
                          3, atoi(PQvaluebycol(res, i, "precio")),
                          4, atoi (PQvaluebycol(res, i, "subtotal")),
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

  PGresult *res;
  gchar *q;
  gint tuples, i;

  guint32 total;

  if (! gtk_tree_selection_get_selected (selection, NULL, &iter)) return;

  gtk_tree_model_get (model, &iter,
                      0, &sale_id,
		      3, &monto,
                      -1);

  /*Se comprueba que el monto de la venta sea menor al dinero en caja*/
  if (monto > ArqueoCaja())
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

      q = g_strdup_printf ("SELECT * FROM get_sale_detail (%d)", sale_id);
      res = EjecutarSQL (q);
      g_free (q);

      if (res != NULL && PQntuples (res) != 0)
        {
          tuples = PQntuples (res);

          for (i = 0; i < tuples; i++)
            {
              AgregarALista (NULL, PQvaluebycol (res, i, "barcode"), g_strtod (PUT (PQvaluebycol (res, i, "amount")), NULL));

              venta->products->product->precio = atoi (PQvaluebycol (res, i, "price"));

              gtk_list_store_insert_after (sell, &iter, NULL);
              gtk_list_store_set (sell, &iter,
                                  0, venta->products->product->codigo,
                                  1, venta->products->product->producto,
                                  2, venta->products->product->marca,
                                  3, venta->products->product->contenido,
                                  4, venta->products->product->unidad,
                                  5, g_strdup_printf ("%.3f", venta->products->product->cantidad),
                                  6, venta->products->product->precio,
                                  7, PutPoints (g_strdup_printf
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
on_entry_srch_provider_activate (GtkEntry *entry)
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
  GtkTreeView *tree = GTK_TREE_VIEW (gtk_builder_get_object(builder, "tree_view_srch_provider"));;
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
                            g_strdup_printf ("<b><big>%.6d</big></b>", get_ticket_number (SIMPLE)));

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
  gint venta_id;

  gtk_widget_grab_focus (GTK_WIDGET (gtk_builder_get_object (builder, "comboboxDestino")));
  gint total = atoi (CutPoints (g_strdup (gtk_label_get_text (GTK_LABEL (gtk_builder_get_object (builder, "label_total"))))));
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_monto_total")),
                      g_strdup_printf ("%d",TotalPrecioCompra(venta->header)));
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
}

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

  gchar *tipo_vendedor = rizoma_get_value ("VENDEDOR");

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
  gint * destino;
  GtkTreeIter iter;
  GtkWidget *combo;
  GtkTreeModel *model;
  gint active;
  gint vendedor = user_data->user_id;
  gint monto = atoi (CutPoints (g_strdup (gtk_label_get_text
                                          (GTK_LABEL (gtk_builder_get_object (builder, "label_total"))))));


  combo = GTK_WIDGET (gtk_builder_get_object(builder, "comboboxDestino"));
  active = gtk_combo_box_get_active (GTK_COMBO_BOX (combo));

  /* Verifica si se selecciono un destino del combobox*/
  if (active == -1)
    ErrorMSG (combo, "Debe Seleccionar un Destino");

  else
    {
      model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));
      gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combo), &iter);

      gtk_tree_model_get (model, &iter,
                          0, &destino,
                          -1);
      SaveTraspaso (TotalPrecioCompra(venta->header),
                    ReturnBodegaID(ReturnNegocio()),
                    vendedor,
                    destino,
                    TRUE);

      gtk_widget_hide (gtk_widget_get_toplevel (GTK_WIDGET (button)));

      gtk_widget_hide (GTK_WIDGET(gtk_builder_get_object(builder, "traspaso_enviar_win")));

      gtk_widget_grab_focus (GTK_WIDGET (gtk_builder_get_object (builder, "barcode_entry")));

      gtk_list_store_clear (venta->store);

      CleanEntryAndLabelData ();

      gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_total")), "");

      gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_ticket_number")),
                            g_strdup_printf ("<b><big>%.6d</big></b>", get_ticket_number (SIMPLE)));

      ListClean ();

    }
  return;
}
