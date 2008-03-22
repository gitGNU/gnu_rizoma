/* -*- Mode: C; tab-width: 4; ident-tabs-mode: nil; c-basic-offset: 4;
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
#include"dimentions.h"
#include"utils.h"
#include"encriptar.h"
#include"factura_more.h"
#include"rizoma_errors.h"

GtkBuilder *builder;

gint screen_width;
gint screen_height;

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

void
FillProductSell (gchar *barcode, gboolean mayorista, gchar *marca, gchar *contenido, gchar *unidad, gchar *stock, gchar *stock_day,
                 gchar *precio, gchar *precio_mayor, gchar *cantidad_mayor, gchar *codigo_corto)
{

  GtkWidget *widget;

  //caja de producto
  widget = GTK_WIDGET(gtk_builder_get_object(builder, "barcode_entry"));
  gtk_entry_set_text(GTK_ENTRY(widget), barcode);
  gtk_entry_set_text (GTK_ENTRY (gtk_builder_get_object (builder, "product_entry")),

  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "product_entry")),
                      g_strdup_printf ("%s  %s  %s  %s", codigo_corto, marca, contenido, unidad));

  if (atoi (stock) <= GetMinStock (barcode))
    gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_stockday")),
                          g_strdup_printf("<span foreground=\"red\"><b>%.2f dia(s)</b></span>",
                                          strtod (PUT (stock_day), (char **)NULL)));
  else
    gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_stockday")),
                          g_strdup_printf ("<b>%.2f dia(s)</b>", strtod (PUT (stock_day), (char **)NULL)));

  //precio
  gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_precio")),
                        g_strdup_printf ("<span weight=\"ultrabold\">%s</span>",
                                         PutPoints (precio)));

  gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_mayor")),
                        g_strdup_printf ("<span weight=\"ultrabold\">%s</span>",
                                         PutPoints (precio_mayor)));

  gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_mayor_cantidad")),
                        g_strdup_printf ("<span weight=\"ultrabold\">%s</span>",
                                         PutPoints (cantidad_mayor)));

  gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_stock")),
                        g_strdup_printf ("<span weight=\"ultrabold\">%.2f</span>",
                                         strtod (stock, (char **)NULL)));

  gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_subtotal")),
                        g_strdup_printf
                        ("<span weight=\"ultrabold\">%s</span>",
                         g_strdup_printf ("%.0f",
                                          strtod (g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "cantidad_entry")))), (char **)NULL) *
                                          atoi (precio))));

  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "codigo_corto")), codigo_corto);
}

void
CanjearProducto (GtkWidget *widget, gpointer data)
{
  GtkWidget *entry = (GtkWidget *) data;

  if (entry == NULL)
    {
      gtk_widget_set_sensitive (main_window, TRUE);

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
  //  GtkWidget *entry;
  GtkWidget *button;

  gtk_widget_set_sensitive (main_window, FALSE);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "Canjear Producto");
  gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER_ALWAYS);
  gtk_window_set_resizable (GTK_WINDOW (window), FALSE);
  //  gtk_window_set_transient_for (GTK_WINDOW (window), GTK_WINDOW (gtk_widget_get_toplevel (widget)));
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

  //  SetToggleMode (widget, NULL);
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

  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (Vender), (gpointer)window);

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
    TiposVenta (NULL, NULL);
  else
    {
      gtk_widget_grab_focus (GTK_WIDGET (gtk_builder_get_object (builder, "sencillo_entry")));

      venta->tipo_venta = SIMPLE;
    }
  closing_tipos = FALSE;
}

void
FillDatosVenta (GtkWidget *widget, gpointer data)
{
  GtkWidget *window;
  GtkWidget *hbox;
  GtkWidget *vbox;

  GtkWidget *vbox2;
  GtkWidget *frame;
  GtkWidget *button;
  GtkWidget *label;

  tipo_venta = (gchar *) data;
  if (widget != NULL)
    gtk_widget_destroy (gtk_widget_get_toplevel (widget));

  if (tipo_documento == FACTURA || tipo_documento == GUIA)
    {
      if (strcmp (tipo_venta, "cheque") == 0)
        venta->tipo_venta = CHEQUE;
      else if (strcmp (tipo_venta, "tarjeta") == 0)
        venta->tipo_venta = TARJETA;
      else if (strcmp (tipo_venta, "credito") == 0)
        venta->tipo_venta = CREDITO;

      gtk_widget_set_sensitive (venta->forma_pago, FALSE);

      switch (venta->tipo_venta)
        {
        case CHEQUE:
          gtk_label_set_markup (GTK_LABEL (venta->venta_pago),
                                g_strdup_printf ("<b>Cheque</b>"));
          gtk_widget_set_sensitive (venta->window, FALSE);
          DatosCheque ();
          break;
        case TARJETA:
          gtk_label_set_markup (GTK_LABEL (venta->venta_pago),
                                g_strdup_printf ("<b>Tarjeta</b>"));
          break;
        case CREDITO:
          gtk_label_set_markup (GTK_LABEL (venta->venta_pago),
                                g_strdup_printf ("<b>Credito</b>"));
          gtk_widget_set_sensitive (venta->window, TRUE);
          gtk_widget_set_sensitive (venta->forma_pago, TRUE);
          gtk_window_set_focus (GTK_WINDOW (venta->window), venta->forma_pago);
          gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "sencillo_entry")), FALSE);
          gtk_widget_set_sensitive (venta->sell_button, TRUE);

          break;
        }

      //      gtk_widget_set_sensitive (venta->window, TRUE);

      return;
    }

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "Tipo de Venta");
  gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER_ALWAYS);
  gtk_widget_show (window);
  gtk_window_present (GTK_WINDOW (window));
  //  gtk_window_set_transient_for (GTK_WINDOW (window), GTK_WINDOW (main_window));
  gtk_window_set_resizable (GTK_WINDOW (window), FALSE);
  gtk_widget_set_size_request (window, 220, -1);

  g_signal_connect (G_OBJECT (window), "destroy",
                    G_CALLBACK (CancelarTipo), (gpointer)TRUE);

  /*  g_signal_connect (G_OBJECT (window), "destroy",
      G_CALLBACK (CancelarTipo), (gpointer)TRUE);
  */
  vbox = gtk_vbox_new (FALSE, 3);
  gtk_container_add (GTK_CONTAINER (window), vbox);
  gtk_widget_show (vbox);

  frame = gtk_frame_new ("Datos del Cliente");
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 3);
  gtk_widget_show (frame);

  vbox2 = gtk_vbox_new (FALSE, 3);
  gtk_container_add (GTK_CONTAINER (frame), vbox2);
  gtk_widget_show (vbox2);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  label = gtk_label_new ("Rut: ");
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
  gtk_widget_show (label);

  venta->venta_rut = gtk_entry_new ();
  gtk_box_pack_end (GTK_BOX (hbox), venta->venta_rut, FALSE, FALSE, 3);
  gtk_widget_show (venta->venta_rut);

  gtk_entry_set_editable (GTK_ENTRY (venta->venta_rut), FALSE);

  g_signal_connect (G_OBJECT (venta->venta_rut), "activate",
                    G_CALLBACK (SelectClient), NULL);

  gtk_window_set_focus (GTK_WINDOW (window), venta->venta_rut);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  label = gtk_label_new ("Nombre: ");
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
  gtk_widget_show (label);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  venta->venta_nombre = gtk_label_new ("");
  gtk_box_pack_start (GTK_BOX (hbox), venta->venta_nombre, FALSE, FALSE, 3);
  gtk_widget_show (venta->venta_nombre);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  label = gtk_label_new ("Dirección: ");
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
  gtk_widget_show (label);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  venta->venta_direccion = gtk_label_new ("");
  gtk_box_pack_start (GTK_BOX (hbox), venta->venta_direccion, FALSE, FALSE, 3);
  gtk_widget_show (venta->venta_direccion);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  label = gtk_label_new ("Fono: ");
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
  gtk_widget_show (label);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  venta->venta_fono = gtk_label_new ("");
  gtk_box_pack_start (GTK_BOX (hbox), venta->venta_fono, FALSE, FALSE, 3);
  gtk_widget_show (venta->venta_fono);


  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_end (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  button = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 3);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (CancelarTipo), (gpointer)TRUE);

  button = gtk_button_new_with_label ("Vender");
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (Vender), (gpointer)window);

  if (strcmp (tipo_venta, "cheque") == 0)
    {
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
  else if (strcmp (tipo_venta, "tarjeta") == 0)
    {
      frame = gtk_frame_new ("Datos Tarjeta");
      gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 3);
      gtk_widget_show (frame);

      vbox2 = gtk_vbox_new (FALSE, 3);
      gtk_container_add (GTK_CONTAINER (frame), vbox2);
      gtk_widget_show (vbox2);

      hbox = gtk_hbox_new (FALSE, 3);
      gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 3);
      gtk_widget_show (hbox);

      label = gtk_label_new ("Institución: ");
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
      gtk_widget_show (label);

      venta->tarjeta_inst = gtk_entry_new_with_max_length (50);
      gtk_widget_set_size_request (venta->tarjeta_inst, 130, -1);
      gtk_box_pack_end (GTK_BOX (hbox), venta->tarjeta_inst, FALSE, FALSE, 3);
      gtk_widget_show (venta->tarjeta_inst);

      g_signal_connect (G_OBJECT (venta->venta_rut), "changed",
                        G_CALLBACK (SendCursorTo), (gpointer)venta->tarjeta_inst);

      hbox = gtk_hbox_new (FALSE, 3);
      gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 3);
      gtk_widget_show (hbox);

      label = gtk_label_new ("Nº de Tarjeta: ");
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
      gtk_widget_show (label);

      venta->tarjeta_numero = gtk_entry_new_with_max_length (20);
      gtk_widget_set_size_request (venta->tarjeta_numero, 130, -1);
      gtk_box_pack_end (GTK_BOX (hbox), venta->tarjeta_numero, FALSE, FALSE, 3);
      gtk_widget_show (venta->tarjeta_numero);

      g_signal_connect (G_OBJECT (venta->tarjeta_inst), "activate",
                        G_CALLBACK (SendCursorTo), (gpointer)venta->tarjeta_numero);

      hbox = gtk_hbox_new (FALSE, 3);
      gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 3);
      gtk_widget_show (hbox);

      label = gtk_label_new ("Fecha Venc: ");
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
      gtk_widget_show (label);

      venta->tarjeta_fecha = gtk_entry_new_with_max_length (7);
      gtk_widget_set_size_request (venta->tarjeta_fecha, 130, -1);
      gtk_box_pack_end (GTK_BOX (hbox), venta->tarjeta_fecha, FALSE, FALSE, 3);
      gtk_widget_show (venta->tarjeta_fecha);

      g_signal_connect (G_OBJECT (venta->tarjeta_numero), "activate",
                        G_CALLBACK (SendCursorTo), (gpointer)venta->tarjeta_fecha);

      g_signal_connect (G_OBJECT (venta->tarjeta_fecha), "activate",
                        G_CALLBACK (SendCursorTo), (gpointer)button);

      venta->tipo_venta = TARJETA;
    }
  else if (strcmp (tipo_venta, "credito") == 0)
    {
      venta->tipo_venta = CREDITO;

      g_signal_connect (G_OBJECT (venta->venta_rut), "changed",
                        G_CALLBACK (SendCursorTo), (gpointer)button);

    }
}

void
TiposVenta (GtkWidget *widget, gpointer data)
{
  GtkWidget *vbox;
  GtkWidget *button;

  if (widget != NULL && strcmp (g_strdup (gtk_entry_get_text (GTK_ENTRY (widget))), "") != 0)
    SendCursorTo (widget, data);
  else
    {
      gtk_widget_set_sensitive (venta->window, FALSE);

      tipos_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
      gtk_window_set_title (GTK_WINDOW (tipos_window), "Tipo de Venta");
      gtk_window_set_position (GTK_WINDOW (tipos_window), GTK_WIN_POS_CENTER_ALWAYS);
      gtk_widget_show (tipos_window);
      gtk_window_present (GTK_WINDOW (tipos_window));
      //      gtk_window_set_transient_for (GTK_WINDOW (tipos_window), GTK_WINDOW (venta->window));
      gtk_window_set_resizable (GTK_WINDOW (tipos_window), FALSE);
      gtk_widget_set_size_request (tipos_window, 220, -1);

      g_signal_connect (G_OBJECT (venta->window), "destroy",
                        G_CALLBACK (CancelarTipo), (gboolean)FALSE);

      vbox = gtk_vbox_new (FALSE, 3);
      gtk_container_add (GTK_CONTAINER (tipos_window), vbox);
      gtk_widget_show (vbox);

      button = gtk_button_new_with_label ("Crédito");
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 3);
      gtk_widget_show (button);

      gtk_window_set_focus (GTK_WINDOW (tipos_window), button);

      g_signal_connect (G_OBJECT (button), "clicked",
                        G_CALLBACK (FillDatosVenta), (gpointer)"credito");

      button = gtk_button_new_with_label ("Cheque");
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 3);
      gtk_widget_show (button);

      g_signal_connect (G_OBJECT (button), "clicked",
                        G_CALLBACK (FillDatosVenta), (gpointer)"cheque");

      /*      button = gtk_button_new_with_label ("Tarjeta de Crédito");
              gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 3);
              gtk_widget_show (button);

              g_signal_connect (G_OBJECT (button), "clicked",
              G_CALLBACK (FillDatosVenta), (gpointer)("tarjeta"));

              button = gtk_button_new_with_label ("Red Compra");
              gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 3);
              gtk_widget_show (button);
      */
      button = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 3);
      gtk_widget_show (button);

      g_signal_connect (G_OBJECT (button), "clicked",
                        G_CALLBACK (CancelarTipo), (gpointer)FALSE);
    }
}

void
ventas_win ()
{
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;
  GtkTreeIter iter;
  GtkWidget *ventas_gui;
  gchar *fullscreen_opt = NULL;

  venta = (Venta *) g_malloc (sizeof (Venta));
  venta->header = NULL;
  venta->products = NULL;
  venta->window = NULL;
  Productos *fill = venta->header;

  builder = gtk_builder_new ();

  gtk_builder_add_from_file (builder, DATADIR"/ui/rizoma-ventas.ui", NULL);
  gtk_builder_connect_signals (builder, NULL);

  ventas_gui = GTK_WIDGET (gtk_builder_get_object (builder, "ventas_gui"));

  // check if the window must be set to fullscreen
  fullscreen_opt = rizoma_get_value("FULLSCREEN");
  if ((fullscreen_opt != NULL) && (g_str_equal(fullscreen_opt, "YES")))
      gtk_window_fullscreen(GTK_WINDOW(ventas_gui));

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
  /* gtk_tree_view_column_set_min_width (column, 75); */
  /* gtk_tree_view_column_set_max_width (column, 75); */
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Descripción", renderer,
                                                     "text", 1,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (venta->treeview_products), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);

  /* gtk_tree_view_column_set_min_width (column, 170); */
  /* gtk_tree_view_column_set_max_width (column, 170); */
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Marca", renderer,
                                                     "text", 2,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (venta->treeview_products), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  /* gtk_tree_view_column_set_min_width (column, 90); */
  /* gtk_tree_view_column_set_max_width (column, 90); */
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Cont.", renderer,
                                                     "text", 3,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (venta->treeview_products), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  /* gtk_tree_view_column_set_min_width (column, 35); */
  /* gtk_tree_view_column_set_max_width (column, 35); */
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Uni.", renderer,
                                                     "text", 4,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (venta->treeview_products), column);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  /* gtk_tree_view_column_set_min_width (column, 30); */
  /* gtk_tree_view_column_set_max_width (column, 30); */
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Cant.", renderer,
                                                     "text", 5,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (venta->treeview_products), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  /* gtk_tree_view_column_set_min_width (column, 50); */
  /* gtk_tree_view_column_set_max_width (column, 50); */
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Precio Uni.", renderer,
                                                     "text", 6,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (venta->treeview_products), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  /* gtk_tree_view_column_set_min_width (column, 70); */
  /* gtk_tree_view_column_set_max_width (column, 70); */
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Sub Total", renderer,
                                                     "text", 7,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (venta->treeview_products), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  /* gtk_tree_view_column_set_min_width (column, 70); */
  /* gtk_tree_view_column_set_max_width (column, 70); */
  gtk_tree_view_column_set_resizable (column, FALSE);

  if (venta->header != NULL)
    CalcularVentas (venta->header);

  gtk_widget_grab_focus (GTK_WIDGET (gtk_builder_get_object (builder, "barcode_entry")));
}

gboolean
SearchProductByCode (void)
{
  gchar *codigo = g_strdup (gtk_label_get_text (GTK_LABEL (gtk_builder_get_object (builder, "codigo_corto"))));
  PGresult *res;
  gint venta_directa = atoi(rizoma_get_value("VENTA_DIRECTA"));

  res = EjecutarSQL (g_strdup_printf ("SELECT * FROM informacion_producto (0, '%s')", codigo));

  if (res != NULL && PQntuples (res) != 0)
    {
      mayorista = strcmp (PQvaluebycol (res, 0, "mayorista"), "t") == 0 ? TRUE : FALSE;

      FillProductSell (PQvaluebycol (res, 0, "barcode"), mayorista,  PQvaluebycol (res, 0, "marca"), PQvaluebycol (res, 0, "contenido"),
                       PQvaluebycol (res, 0, "unidad"),PQvaluebycol (res, 0, "stock"), PQvaluebycol (res, 0, "stock_day"),
                       PQvaluebycol (res, 0, "precio"), PQvaluebycol (res, 0, "precio_mayor"), PQvaluebycol (res, 0, "cantidad_mayor"),
                       PQvaluebycol (res, 0, "codigo_corto"));

      if (PQvaluebycol (res, 0, "precio") != 0)
        {
          if( venta_directa == 1 ) {
            if( VentaFraccion( PQvaluebycol( res, 0, "cantidad_mayor" ) ) ) {
              gtk_widget_grab_focus (GTK_WIDGET (gtk_builder_get_object (builder, "cantidad_entry")));
              gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "sell_add_button")), TRUE);
            } else {
              AgregarProducto( NULL, NULL );
            }
          } else {
            gtk_widget_grab_focus (GTK_WIDGET (gtk_builder_get_object (builder, "cantidad_entry")));
            gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "sell_add_button")), TRUE );
          }
        }
      else
        gtk_widget_grab_focus (GTK_WIDGET (gtk_builder_get_object (builder, "barcode_entry")));

      return TRUE;
    }
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

  cantidad = strtod (PUT(g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "cantidad_entry"))))),
                     (char **)NULL);

  if (cantidad <= 0 && VentaFraccion (barcode) == FALSE)
    {
      /*AlertMSG (gtk_builder_get_object (builder, "cantidad_entry"), "No puede vender una cantidad 0 o menor");

        return FALSE;*/

    }
  if ((strcmp ("0", CutPoints (g_strdup (gtk_label_get_text (GTK_LABEL (gtk_builder_get_object (builder, "label_precio"))))))) == 0)
    {
      AlertMSG (GTK_WIDGET (gtk_builder_get_object (builder, "barcode_entry")), "No se pueden vender productos con precio 0");

      CleanEntryAndLabelData ();

      return FALSE;
    }
  else if (cantidad > stock)
    {
      AlertMSG (GTK_WIDGET (gtk_builder_get_object (builder, "cantidad_entry")), "No puede vender mas productos de los que tiene en stock");

      gtk_window_set_focus (GTK_WINDOW (main_window), GTK_WIDGET (gtk_builder_get_object (builder, "cantidad_entry")));

      return FALSE;
    }
  else if (strchr (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "cantidad_entry"))), ',') != NULL ||
           strchr (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "cantidad_entry"))), '.') != NULL)
    {
      if (VentaFraccion (barcode) == FALSE)
        {
          AlertMSG (GTK_WIDGET (gtk_builder_get_object (builder, "cantidad_entry")),
                    "Este producto no se puede vender por fracción de producto");
          gtk_window_set_focus (GTK_WINDOW (main_window), GTK_WIDGET (gtk_builder_get_object (builder, "cantidad_entry")));
          return FALSE;
        }
    }



  if ((strcmp ("", codigo)) != 0)
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
                        "No puede vender mas productos de los que tiene en stock");

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

void
CleanSellLabels (void)
{
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "codigo_corto")), "");
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "product_entry")), "");
  gtk_entry_set_text (GTK_ENTRY (gtk_builder_get_object (builder, "barcode_entry")), "");
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_stockday")), "");
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_precio")), "");
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_stock")), "");
}

void
CleanEntryAndLabelData (void)
{
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "product_entry")), "");
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

void
EliminarProducto (GtkButton *button, gpointer data)
{
  GtkTreeIter iter;
  GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW
                                                             (venta->treeview_products));
  gchar *value;
  gint position;

  if (gtk_tree_selection_get_selected (selection, NULL, &iter) == TRUE)
    {
      gtk_tree_model_get (GTK_TREE_MODEL (venta->store), &iter,
                          0, &value,
                          -1);

      position = atoi (gtk_tree_model_get_string_from_iter (GTK_TREE_MODEL (venta->store), &iter));

      EliminarDeLista (value, position);

      gtk_list_store_remove (GTK_LIST_STORE (venta->store), &iter);

      CalcularVentas (venta->header);

      /*
        gtk_label_set_markup (GTK_LABEL (venta->sub_total_label),
        g_strdup_printf ("<span size=\"40000\">%s</span>",
        PutPoints (g_strdup_printf ("%d", CalcularTotal (venta->header)))));

        gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_total")),
        g_strdup_printf ("<span size=\"40000\">%d</span>", CalcularTotal ()));
      */

      //      gtk_entry_set_text (GTK_ENTRY (gtk_builder_get_object (builder, "discount_entry")), "0");
      //      Descuento ();
    }
}

gint
Vender (GtkButton *button, gpointer data)
{
  gboolean cheque = FALSE;
  gboolean tarjeta = FALSE;
  gchar *rut = NULL;
  gchar *cheque_date = NULL;
  gint monto = atoi (CutPoints (g_strdup (gtk_label_get_text
                                          (GTK_LABEL (gtk_builder_get_object (builder, "label_total"))))));
  gchar *discount = "0";
  gint maquina = atoi (rizoma_get_value ("MAQUINA"));
  gint vendedor = user_data->user_id;
  gint paga_con;
  gint ticket;
  gboolean canceled;

  if (tipo_documento != VENTA && tipo_documento != FACTURA)
    paga_con = atoi (g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "sencillo_entry")))));

  if (data != NULL)
    {
      rut = g_strdup (gtk_entry_get_text (GTK_ENTRY (venta->venta_rut)));

      if (strcmp (rut, "") == 0)
        {
          ErrorMSG (venta->venta_rut, "Debe ingresar un cliente");
          return 0;
        }

      if (strcmp (tipo_venta, "cheque") == 0)
        {
          venta->tipo_venta = CHEQUE;
          cheque = TRUE;
          tarjeta = FALSE;
        }
      else if (strcmp (tipo_venta, "tarjeta") == 0)
        {
          venta->tipo_venta = tarjeta;
          cheque = FALSE;
          tarjeta = TRUE;
        }
    }

  if (venta->tipo_venta == CHEQUE)
    {
      if (strcmp (gtk_entry_get_text (GTK_ENTRY (venta->cheque_serie)), "") == 0)
        {
          ErrorMSG (venta->cheque_serie, "Debe Ingresar la serie del cheque");
          return 0;
        }
      else if (strcmp (gtk_entry_get_text (GTK_ENTRY (venta->cheque_numero)), "") == 0)
        {
          ErrorMSG (venta->cheque_numero, "Debe Ingresar el Nmero del Cheque");
          return 0;
        }
      else if (strcmp (gtk_entry_get_text (GTK_ENTRY (venta->cheque_banco)), "") == 0)
        {
          ErrorMSG (venta->cheque_banco, "Debe Ingresar el Banco al que pertenece el Cheque");
          return 0;
        }
      else if (strcmp (gtk_entry_get_text (GTK_ENTRY (venta->cheque_plaza)), "") == 0)
        {
          ErrorMSG (venta->cheque_plaza, "Debe Ingresar la Plaza del cheque");
          return 0;
        }

      cheque_date = g_strdup (gtk_button_get_label (GTK_BUTTON (button_cheque)));

      monto_cheque += atoi (g_strdup (gtk_entry_get_text (GTK_ENTRY (venta->cheque_monto))));

    }
  else if (venta->tipo_venta == TARJETA)
    {
      if (strcmp (gtk_entry_get_text (GTK_ENTRY (venta->tarjeta_inst)), "") == 0)
        {
          ErrorMSG (venta->tarjeta_inst, "Debe Ingresar la Institución emisora de la Tarjeta");
          return 0;
        }
      else if (strcmp (gtk_entry_get_text (GTK_ENTRY (venta->tarjeta_numero)), "") == 0)
        {
          ErrorMSG (venta->tarjeta_numero, "Debe Ingresar el número de la tarjeta");
          return 0;
        }
      else if (strcmp (gtk_entry_get_text (GTK_ENTRY (venta->tarjeta_fecha)), "") == 0)
        {
          ErrorMSG (venta->tarjeta_fecha, "Debe Ingresar la fecha de Vencimiento de la tarjeta");
          return 0;
        }
    }
  else if (venta->tipo_venta == CREDITO)
    {
      rut = g_strdup (gtk_entry_get_text (GTK_ENTRY (venta->venta_rut)));

      if (strcmp (rut, "") == 0)
        {
          ErrorMSG (venta->venta_rut, "En la venta a crédito el RUT no puede estar vacio");
          return 0;
        }
      if (RutExist (rut) == FALSE)
        {
          ErrorMSG (venta->venta_rut, "No existe un cliente con ese Rut");
          return 0;
        }

      if (LimiteCredito (rut) < (DeudaTotalCliente (atoi (rut)) + monto))
        {
          ErrorMSG (venta->venta_rut, "El cliente sobrepasa su limite de Credito");
          return 0;
        }
    }
  else if (tipo_documento != VENTA && tipo_documento != FACTURA && paga_con < monto)
    {
      ErrorMSG (GTK_WIDGET (gtk_builder_get_object (builder, "sencillo_entry")), "No esta pagando con el dinero suficiente");
      return 0;
    }

  if (tipo_documento == FACTURA)
    {
      rut = g_strdup (gtk_entry_get_text (GTK_ENTRY (venta->venta_rut)));

      if (strcmp (rut, "") == 0)
        {
          ErrorMSG (venta->venta_rut, "En la venta con factura el RUT es un dato obligatorio");
          return 0;
        }
      if (RutExist (rut) == FALSE)
        {
          ErrorMSG (venta->venta_rut, g_strdup_printf ("No existe un cliente con el rut: %s",
                                                       rut));
          return 0;
        }
    }

  if (tipo_documento == GUIA)
    {
      rut = g_strdup (gtk_entry_get_text (GTK_ENTRY (venta->venta_rut)));

      if (strcmp (rut, "") == 0)
        {
          ErrorMSG (venta->venta_rut, "En la venta con guia el RUT es un dato obligatorio");
          return 0;
        }
      if (RutExist (rut) == FALSE)
        {
          ErrorMSG (venta->venta_rut, g_strdup_printf ("No existe un cliente con el rut: %s",
                                                       rut));
          return 0;
        }
    }

  if (tipo_documento != VENTA)
    discount = g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "discount_entry"))));

  switch (tipo_documento)
    {
    case SIMPLE:
      if (monto >= 180)
        ticket = get_ticket_number (tipo_documento);
      else
        ticket = -1;
      break;
    case FACTURA:
      ticket = get_ticket_number (tipo_documento);
      break;
    case GUIA:
      ticket = get_ticket_number (tipo_documento);
      break;
    case VENTA:
      ticket = -1;
      break;
    }

  //  DiscountStock (venta->header);

  if (tipo_documento == VENTA)
    canceled = FALSE;
  else
    canceled = TRUE;

  SaveSell (monto, maquina, vendedor, venta->tipo_venta, rut, discount, ticket, tipo_documento,
            cheque_date, cheques, canceled);


  if (data != NULL && (venta->tipo_venta == TARJETA || venta->tipo_venta == CREDITO ||
                       (cheque == TRUE && monto_cheque >= monto) || venta->tipo_venta == FACTURA))
    //gtk_widget_destroy ((GtkWidget *)data);
    CancelarTipo ((GtkWidget *)data, (gpointer)FALSE);
  /* else if (tipos_window != NULL && monto_cheque >= monto)
     gtk_widget_destroy (tipos_window);*/
  else if (monto > monto_cheque && venta->tipo_venta == CHEQUE)
    {
      if (data != NULL)
        //gtk_widget_destroy ((GtkWidget *)data);
        CancelarTipo ((GtkWidget *)data, (gpointer)FALSE);

      FillDatosVenta (NULL, (gpointer)tipo_venta);

      cheques = TRUE;

      return 0;
    }
  else if (monto_cheque >= monto && venta->tipo_venta == CHEQUE)
    {
      cheques = FALSE;
      monto_cheque = 0;
    }

  /*
    if (cheque == TRUE)
    CloseChequeWindow ();
  */

  CloseSellWindow (NULL, NULL);

  gtk_list_store_clear (venta->store);

  CleanEntryAndLabelData ();

  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_total")), "");

  gtk_widget_grab_focus (GTK_WIDGET (gtk_builder_get_object (builder, "barcode_entry")));

  if (monto >= 180 && ticket != -1)
    gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_ticket_number")),
                          g_strdup_printf ("<b><big>%.6d</big></b>", ticket+1));
  else
    gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_ticket_number")),
                          g_strdup_printf ("<b><big>%.6d</big></b>", get_ticket_number (SIMPLE)));

  //  PrintDocument (tipo_documento, rut, monto, ticket, venta->products);

  ListClean ();

  WindowChangeSeller();

  return 0;
}

void
MoveFocus (GtkEntry *entry, gpointer data)
{
	GtkWidget *button;
	button = GTK_WIDGET (gtk_builder_get_object (builder, "sell_add_button"));
	gtk_widget_set_sensitive(button, TRUE);
	gtk_widget_grab_focus (button);
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
  gchar *tipo_vendedor = rizoma_get_value("VENDEDOR");

  if (venta->header == NULL)
    {
      ErrorMSG (GTK_WIDGET (gtk_builder_get_object (builder, "barcode_entry")), "No hay productos para vender");
      return;
    }

  if (strcmp (tipo_vendedor, "1") == 0)
    {
      tipo_documento = SIMPLE;
    }
  else
    {
      window = GTK_WINDOW (gtk_builder_get_object (builder, "tipo_venta_win_venta"));
      gtk_entry_set_text (GTK_ENTRY (gtk_builder_get_object (builder, "discount_entry")), "0");
      gtk_entry_set_text (GTK_ENTRY (gtk_builder_get_object (builder, "sencillo_entry")), "");
      gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "vuelto_label")), "");
      gtk_widget_show_all (GTK_WIDGET (window));
      return;
    }
}

void
CloseSellWindow (GtkWidget *widget, gpointer user_data)
{
  //  gchar *name_window = (gchar *) user_data;
  /*
    gtk_widget_destroy (venta->window);
    venta->window = NULL;

    gtk_widget_set_sensitive (main_window, TRUE);
  */
  gtk_widget_hide (GTK_WIDGET (gtk_builder_get_object (builder, "tipo_venta_win_venta")));

  gtk_widget_grab_focus (GTK_WIDGET (gtk_builder_get_object (builder, "barcode_entry")));

  CalcularVentas (venta->header);

}

/* void */
/* CambiarTipoVenta (GtkToggleButton *button, */
/*   gpointer data) */
/* { */
/*   gboolean state = gtk_toggle_button_get_active (button); */

/*   gtk_widget_set_sensitive (venta->entry_rut, state); */
/*   gtk_widget_set_sensitive (venta->find_button, state); */

/*   gtk_widget_set_sensitive (venta->sell_button, FALSE); */

/*   if (state == TRUE) */
/*     { */
/*       gtk_widget_set_sensitive (gtk_builder_get_object (builder, "sencillo_entry"), FALSE); */
/*       //      gtk_widget_set_sensitive (vuelto_button, FALSE); */
/*     } */
/*   else */
/*     { */
/*       gtk_widget_set_sensitive (gtk_builder_get_object (builder, "sencillo_entry"), TRUE); */
/*       //      gtk_widget_set_sensitive (vuelto_button, TRUE); */
/*     } */

/*   venta->tipo_venta = state; */
/* } */

void
CalcularVuelto (void)
{
  gchar *pago = g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "sencillo_entry"))));
  gint paga_con = atoi (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "sencillo_entry"))));
  gint total = atoi (CutPoints (g_strdup (gtk_label_get_text (GTK_LABEL (gtk_builder_get_object (builder, "label_total"))))));
  gint resto;

  if (strcmp (pago, "c") == 0)
    PagoCheque ();
  else if (paga_con > 0)
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

void
SearchProductByName (void)
{
  //  gchar *producto = g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "product_entry"))));

  //  if (strcmp (producto, "") == 0)
  //    WindowProductSelect ();
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

void
CloseWindowClientSelection (GtkWidget *nothing, gpointer data)
{
  GtkWidget *widget = (GtkWidget *) data;

  gtk_widget_set_sensitive (gtk_widget_get_toplevel (widget), TRUE);

  gtk_widget_destroy (venta->clients_window);

  venta->clients_window = NULL;
}

void
WindowClientSelection (GtkWidget *widget, gpointer data)
{
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  GtkWidget *hbox;
  GtkWidget *vbox;

  GtkWidget *button;
  GtkWidget *scroll;

  venta->clients_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (venta->clients_window),
                        "Rizoma: Busqueda de Cliente");
  gtk_window_set_resizable (GTK_WINDOW (venta->clients_window), FALSE);
  gtk_window_set_position (GTK_WINDOW (venta->clients_window), GTK_WIN_POS_CENTER_ALWAYS);
  gtk_widget_show (venta->clients_window);
  gtk_window_present (GTK_WINDOW (venta->clients_window));
  //  gtk_window_set_transient_for (GTK_WINDOW (venta->clients_window), GTK_WINDOW (venta->window));
  g_signal_connect (G_OBJECT (venta->clients_window), "destroy",
                    G_CALLBACK (CloseWindowClientSelection), (gpointer)widget);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_container_add (GTK_CONTAINER (venta->clients_window), hbox);

  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_show (scroll);
  gtk_widget_set_size_request (scroll, 450, 200);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (hbox), scroll, FALSE, FALSE, 3);

  venta->clients_store = gtk_list_store_new (4,
                                             G_TYPE_INT,
                                             G_TYPE_STRING,
                                             G_TYPE_INT,
                                             G_TYPE_BOOLEAN,
                                             -1);
  FillClientStore (venta->clients_store);

  venta->clients_tree = gtk_tree_view_new_with_model (GTK_TREE_MODEL (venta->clients_store));
  gtk_widget_show (venta->clients_tree);
  gtk_container_add (GTK_CONTAINER (scroll), venta->clients_tree);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Código", renderer,
                                                     "text", 0,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (venta->clients_tree), column);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Nombre", renderer,
                                                     "text", 1,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (venta->clients_tree), column);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Teléfono", renderer,
                                                     "text", 2,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (venta->clients_tree), column);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_toggle_new ();
  column = gtk_tree_view_column_new_with_attributes ("Crédito", renderer,
                                                     "active", 3,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (venta->clients_tree), column);
  gtk_tree_view_column_set_resizable (column, FALSE);

  vbox = gtk_vbox_new (FALSE, 3);
  gtk_widget_show (vbox);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 3);

  button = gtk_button_new_with_label ("Seleccionar");
  gtk_widget_show (button);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 3);

  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (SeleccionarCliente), (gpointer)widget);

  button = gtk_button_new_from_stock (GTK_STOCK_ADD);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 3);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (AddClient), (gpointer)venta->clients_store);

  button = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
  gtk_widget_show (button);
  gtk_box_pack_end (GTK_BOX (vbox), button, FALSE, FALSE, 3);

  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (CloseWindowClientSelection), (gpointer)widget);

}

void
SeleccionarCliente (GtkWidget *nothing, gpointer data)
{
  GtkWidget *widget = (GtkWidget *) data;
  GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (venta->clients_tree));
  GtkTreeIter iter;
  gint rut;

  if (gtk_tree_selection_get_selected (selection, NULL, &iter))
    {
      gtk_tree_model_get (GTK_TREE_MODEL (venta->clients_store), &iter,
                          0, &rut,
                          -1);

      gtk_entry_set_text (GTK_ENTRY (venta->entry_rut),
                          g_strdup_printf ("%d", rut));

      CloseWindowClientSelection (NULL, (gpointer)widget);
    }
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



  //  if (ventas != FALSE)
  q = g_strdup_printf ("SELECT * FROM informacion_producto (%s,'')", barcode);
  /* else */
  /*   q = g_strdup_printf ("SELECT codigo_corto, descripcion, marca, contenido, unidad, stock, " */
  /*  "precio, mayorista FROM producto " */
  /*  "WHERE barcode='%s' AND stock!=0 AND canje='t'", */
  /*  barcode); */

  res = EjecutarSQL(q);

  g_free(q);

  if (res == NULL)
    return -1;

  else
    if (PQntuples (res) == 0)
      {
        if (strcmp (barcode, "") != 0)
          {
            AlertMSG (widget, g_strdup_printf
                      ("No existe un producto con el código de barras %s!!", barcode));

            if (ventas != FALSE)
              CleanSellLabels ();
          }
        else
          if (GetCurrentStock (barcode) == 0)
            {
              AlertMSG (widget, "No ahi mercadería en Stock.\nDebe ingresar mercadería");

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

  mayorista = strcmp (PQvaluebycol( res, 0, "mayorista"), "t") == 0 ? TRUE : FALSE;

  FillProductSell (barcode, mayorista,  PQvaluebycol (res, 0, "marca"), PQvaluebycol (res, 0, "contenido"), PQvaluebycol (res, 0, "unidad"),
                   PQvaluebycol (res, 0, "stock"), PQvaluebycol (res, 0, "stock_day"), PQvaluebycol (res, 0, "precio"), PQvaluebycol (res, 0, "precio_mayor"),
                   PQvaluebycol (res, 0, "cantidad_mayor"), PQvaluebycol (res, 0, "codigo_corto"));

  if (atoi (PQvaluebycol (res, 0, "precio")) != 0)
    {
      if( venta_directa == 1 ) {
        if( VentaFraccion( PQvaluebycol( res, 0, "cantidad_mayor" ) ) ) {
          gtk_widget_grab_focus( GTK_WIDGET (gtk_builder_get_object (builder, "cantidad_entry")));
          gtk_widget_set_sensitive (add_button, TRUE);
        } else {
          AgregarProducto( NULL, NULL );
        }
      } else {
        gtk_widget_grab_focus (GTK_WIDGET (gtk_builder_get_object (builder, "cantidad_entry")));
        gtk_widget_set_sensitive( add_button, TRUE );
      }
    }
  else
    gtk_widget_grab_focus (GTK_WIDGET (gtk_builder_get_object (builder, "barcode_entry")));
  /* } */

  return 0;
}

void
CloseBuscarWindow (GtkWidget *widget, gpointer data)
{
  gboolean add = (gboolean) data;
  gint venta_directa = atoi(rizoma_get_value("VENTA_DIRECTA"));

  gtk_widget_hide (GTK_WIDGET (gtk_builder_get_object (builder, "ventas_buscar")));

  if (add == TRUE && venta_directa == 0)
    gtk_widget_grab_focus (GTK_WIDGET (gtk_builder_get_object (builder, "cantidad_entry")));
  else
    gtk_widget_grab_focus (GTK_WIDGET (gtk_builder_get_object (builder, "barcode_entry")));

  /* { */
  /*   gtk_widget_set_sensitive (main_window, TRUE); */

  /*   gtk_window_set_focus (main_window, canje_cantidad); */
  /* } */
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

  if (gtk_tree_view_get_model (GTK_TREE_VIEW(treeview)) == NULL )
    {
      store = gtk_list_store_new (10,
                                  G_TYPE_STRING,
                                  G_TYPE_STRING,
                                  G_TYPE_STRING,
                                  G_TYPE_STRING,
                                  G_TYPE_INT,
                                  G_TYPE_STRING,
                                  G_TYPE_INT,
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
      g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
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
  window = GTK_WINDOW (gtk_builder_get_object (builder, "ventas_buscar"));
  gtk_widget_show_all (GTK_WIDGET (window));

  SearchAndFill ();

  return;

  /* gtk_widget_set_sensitive (gtk_widget_get_toplevel (entry), FALSE); */

  /* buscador_window = gtk_window_new (GTK_WINDOW_TOPLEVEL); */
  /* gtk_window_set_position (GTK_WINDOW (buscador_window), GTK_WIN_POS_CENTER_ALWAYS); */
  /* if (solo_venta != TRUE) */
  /*   gtk_widget_set_size_request (buscador_window, 800, 300); */
  /* else */
  /*   gtk_widget_set_size_request (buscador_window, 640, 300); */
  /* g_signal_connect (G_OBJECT (buscador_window), "destroy", */
  /*     G_CALLBACK (CloseBuscarWindow), (gpointer)FALSE); */

  /* gtk_window_present (GTK_WINDOW (buscador_window)); */


  /* accel_search = gtk_accel_group_new (); */
  /* gtk_window_add_accel_group (GTK_WINDOW (buscador_window), accel_search); */

  /* vbox = gtk_vbox_new (FALSE, 3); */
  /* gtk_widget_show (vbox); */
  /* gtk_container_add (GTK_CONTAINER (buscador_window), vbox); */

  /* hbox = gtk_hbox_new (FALSE, 3); */
  /* gtk_widget_show (hbox); */
  /* gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3); */

  /* label = gtk_label_new ("Producto a buscar: "); */
  /* gtk_widget_show (label); */
  /* gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3); */

  /* venta->buscar_entry = gtk_entry_new (); */
  /* gtk_widget_show (venta->buscar_entry); */
  /* gtk_box_pack_start (GTK_BOX (hbox), venta->buscar_entry, FALSE, FALSE, 3); */

  /* if (strcmp (string, "") != 0) */
  /*   gtk_entry_set_text (GTK_ENTRY (venta->buscar_entry), string); */

  /* g_signal_connect (G_OBJECT (venta->buscar_entry), "activate", */
  /*     G_CALLBACK (SearchAndFill), NULL); */

  /* gtk_window_set_focus (GTK_WINDOW (buscador_window), venta->buscar_entry); */

  /* button = gtk_button_new_from_stock (GTK_STOCK_FIND); */
  /* gtk_widget_show (button); */
  /* gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 3); */

  /* g_signal_connect (G_OBJECT (button), "clicked", */
  /*     G_CALLBACK (SearchAndFill), NULL); */

  /* button = gtk_button_new_from_stock (GTK_STOCK_ADD); */
  /* gtk_widget_show (button); */
  /* gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3); */

  /* g_signal_connect (G_OBJECT (button), "clicked", */
  /*     G_CALLBACK (FillSellData), NULL); */

  /* scroll = gtk_scrolled_window_new (NULL, NULL); */
  /* gtk_widget_show (scroll); */
  /* gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll), */
  /*   GTK_POLICY_AUTOMATIC, */
  /*   GTK_POLICY_AUTOMATIC); */
  /* gtk_widget_set_size_request (scroll, 320, 200); */

  /* gtk_box_pack_start (GTK_BOX (vbox), scroll, FALSE, FALSE, 3); */

  /* venta->search_store = gtk_list_store_new (10, */
  /*     G_TYPE_STRING, */
  /*     G_TYPE_STRING, */
  /*     G_TYPE_STRING, */
  /*     G_TYPE_STRING, */
  /*     G_TYPE_INT, */
  /*     G_TYPE_STRING, */
  /*     G_TYPE_INT, */
  /*     G_TYPE_INT, */
  /*     G_TYPE_STRING, */
  /*     G_TYPE_BOOLEAN); */

  /* treeview = gtk_tree_view_new_with_model (GTK_TREE_MODEL (venta->search_store)); */
  /* gtk_widget_show (treeview); */
  /* gtk_container_add (GTK_CONTAINER (scroll), treeview); */

  /* venta->search_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview)); */

  /* g_signal_connect (G_OBJECT (treeview), "row-activated", */
  /*     G_CALLBACK (FillSellData), NULL); */

  /* renderer = gtk_cell_renderer_text_new (); */
  /* column = gtk_tree_view_column_new_with_attributes ("Código", renderer, */
  /*      "text", 0, */
  /*      NULL); */
  /* gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column); */
  /* gtk_tree_view_column_set_alignment (column, 0.5); */
  /* g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL); */
  /* gtk_tree_view_column_set_min_width (column, 75); */
  /* gtk_tree_view_column_set_max_width (column, 75); */
  /* gtk_tree_view_column_set_resizable (column, FALSE); */

  /* if (solo_venta != TRUE) */
  /*   { */
  /*     renderer = gtk_cell_renderer_text_new (); */
  /*     column = gtk_tree_view_column_new_with_attributes ("Código de Barras", renderer, */
  /*  "text", 1, */
  /*  NULL); */
  /*     gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column); */
  /*     gtk_tree_view_column_set_alignment (column, 0.5); */
  /*     g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL); */
  /*     gtk_tree_view_column_set_resizable (column, FALSE); */
  /*   } */

  /* renderer = gtk_cell_renderer_text_new (); */
  /* column = gtk_tree_view_column_new_with_attributes ("Descripción", renderer, */
  /*      "text", 2, */
  /*      "foreground", 8, */
  /*      "foreground-set", 9, */
  /*      NULL); */
  /* gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column); */
  /* gtk_tree_view_column_set_alignment (column, 0.5); */
  /* g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL); */
  /* gtk_tree_view_column_set_min_width (column, 270); */
  /* gtk_tree_view_column_set_max_width (column, 270); */
  /* gtk_tree_view_column_set_resizable (column, FALSE); */

  /* renderer = gtk_cell_renderer_text_new (); */
  /* column = gtk_tree_view_column_new_with_attributes ("Marca", renderer, */
  /*      "text", 3, */
  /*      NULL); */
  /* gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column); */
  /* gtk_tree_view_column_set_alignment (column, 0.5); */
  /* g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL); */
  /* gtk_tree_view_column_set_min_width (column, 90); */
  /* gtk_tree_view_column_set_max_width (column, 90); */
  /* gtk_tree_view_column_set_resizable (column, FALSE); */

  /* renderer = gtk_cell_renderer_text_new (); */
  /* column = gtk_tree_view_column_new_with_attributes ("Cont.", renderer, */
  /*      "text", 4, */
  /*      NULL); */
  /* gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column); */
  /* gtk_tree_view_column_set_alignment (column, 0.5); */
  /* g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL); */
  /* gtk_tree_view_column_set_min_width (column, 40); */
  /* gtk_tree_view_column_set_max_width (column, 40); */
  /* gtk_tree_view_column_set_resizable (column, FALSE); */

  /* renderer = gtk_cell_renderer_text_new (); */
  /* column = gtk_tree_view_column_new_with_attributes ("Uni.", renderer, */
  /*      "text", 5, */
  /*      NULL); */
  /* gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column); */
  /* gtk_tree_view_column_set_alignment (column, 0.5); */
  /* g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL); */
  /* gtk_tree_view_column_set_min_width (column, 40); */
  /* gtk_tree_view_column_set_max_width (column, 40); */
  /* gtk_tree_view_column_set_resizable (column, FALSE); */

  /* renderer = gtk_cell_renderer_text_new (); */
  /* column = gtk_tree_view_column_new_with_attributes ("Stock", renderer, */
  /*      "text", 6, */
  /*      NULL); */
  /* gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column); */
  /* gtk_tree_view_column_set_alignment (column, 0.5); */
  /* g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL); */
  /* gtk_tree_view_column_set_min_width (column, 60); */
  /* gtk_tree_view_column_set_max_width (column, 60); */
  /* gtk_tree_view_column_set_resizable (column, FALSE); */

  /* renderer = gtk_cell_renderer_text_new (); */
  /* column = gtk_tree_view_column_new_with_attributes ("Precio", renderer, */
  /*      "text", 7, */
  /*      NULL); */
  /* gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column); */
  /* gtk_tree_view_column_set_alignment (column, 0.5); */
  /* g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL); */
  /* gtk_tree_view_column_set_min_width (column, 70); */
  /* gtk_tree_view_column_set_max_width (column, 70); */
  /* gtk_tree_view_column_set_resizable (column, FALSE); */



  /* SearchAndFill (); */

}

void
SearchAndFill (void)
{
  PGresult *res;
  gint resultados, i;
  gchar *string = g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "barcode_entry"))));

  GtkTreeIter iter;
  GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (gtk_builder_get_object (builder, "ventas_search_treeview"))));

  if (strcmp (string, "") != 0)
    res = EjecutarSQL
      (g_strdup_printf
       ("SELECT * FROM buscar_producto ( '%s', '{\"barcode\", \"codigo_corto\",\"marca\",\"descripcion\"}', true, true )", string));

  resultados = PQntuples (res);

  gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "ventas_label_found")),
                        g_strdup_printf ("<b>%d producto(s)</b>", resultados));

  gtk_list_store_clear (store);


  if (resultados == 0)
    {
      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
                          2, "No se encontró descripción",
                          8, "White",
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
                              6, atoi (PQvaluebycol (res, i, "stock")),
                              7, atoi (PQvaluebycol (res, i, "precio")),
                              -1);
        }
      //gtk_window_set_focus (GTK_WINDOW (gtk_widget_get_toplevel (venta->buscar_entry)),
      //GTK_WIDGET (gtk_tree_selection_get_tree_view (venta->search_selection)));
    }

  //gtk_tree_selection_select_path (venta->search_selection, gtk_tree_path_new_from_string ("0"));

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

          gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "product_entry")), product);

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
      /*   else */
      /* { */
      /*   gtk_entry_set_text (GTK_ENTRY (canje_entry), barcode); */

      /*   SearchBarcodeProduct (canje_entry, (gpointer)FALSE); */
      /* } */
      CloseBuscarWindow (NULL, (gpointer)TRUE);
    }
}

void
Descuento (GtkWidget *widget, gpointer data)
{
  //gboolean money_discount = (gboolean) data;
  gint money;
  gdouble discount = strtod (CUT(g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "discount_entry"))))), (char **)NULL);
  gint total;
  gint plata;

  if (tipo_documento != FACTURA && tipo_documento != GUIA)
    money = atoi (g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "sencillo_entry")))));

  CalcularVentas (venta->header);

  total = atoi (CutPoints (g_strdup (gtk_label_get_text (GTK_LABEL (gtk_builder_get_object (builder, "label_total"))))));

  /*  if (money_discount == TRUE)
    {
      porcentaje = (gdouble)(100 * money) / total;

      gtk_entry_set_text (GTK_ENTRY (gtk_builder_get_object (builder, "discount_entry")),
                          g_strdup_printf ("%.2f", porcentaje));

      gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_total")),
                            g_strdup_printf ("<span size=\"40000\">%s</span>",
                                             PutPoints (g_strdup_printf ("%u", total - money))));
    }
  else if (money_discount == FALSE)
  {*/
      plata = lround ((gdouble)(total * discount) / 100);

      if (tipo_documento != FACTURA)
        gtk_entry_set_text (GTK_ENTRY (gtk_builder_get_object (builder, "sencillo_entry")),
                            g_strdup_printf ("%d", plata));

      gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_total")),
                            g_strdup_printf ("<span size=\"40000\">%s</span>",
                                             PutPoints (g_strdup_printf ("%u", total - plata))));
      //}
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
CloseChequeWindow (void)
{

  gtk_widget_destroy (venta->cheque_window);

}

void
PagoCheque (void)
{
  GtkWidget *vbox;
  GtkWidget *vbox2;
  GtkWidget *hbox;

  GtkWidget *label;
  GtkWidget *button;

  GtkWidget *frame;

  venta->cheque_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_position (GTK_WINDOW (venta->cheque_window), GTK_WIN_POS_CENTER_ALWAYS);
  gtk_window_set_title (GTK_WINDOW (venta->cheque_window), "Pago con Cheque al Día");
  //  gtk_window_set_transient_for (GTK_WINDOW (venta->cheque_window), GTK_WINDOW (venta->window));
  gtk_window_present (GTK_WINDOW (venta->cheque_window));
  gtk_widget_show (venta->cheque_window);

  g_signal_connect (G_OBJECT (venta->cheque_window), "destroy",
                    G_CALLBACK (CloseChequeWindow), NULL);

  vbox = gtk_vbox_new (FALSE, 3);
  gtk_container_add (GTK_CONTAINER (venta->cheque_window), vbox);
  gtk_widget_show (vbox);

  frame = gtk_frame_new ("Datos Emisor");
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 3);
  gtk_widget_show (frame);

  vbox2 = gtk_vbox_new (FALSE, 3);
  gtk_container_add (GTK_CONTAINER (frame), vbox2);
  gtk_widget_show (vbox2);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  label = gtk_label_new ("Rut: ");
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
  gtk_widget_show (label);

  venta->cheque_rut = gtk_entry_new ();
  gtk_box_pack_end (GTK_BOX (hbox), venta->cheque_rut, FALSE, FALSE, 3);
  gtk_widget_show (venta->cheque_rut);

  g_signal_connect (G_OBJECT (venta->cheque_rut), "activate",
                    G_CALLBACK (SelectClient), NULL);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  label = gtk_label_new ("Nombre: ");
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
  gtk_widget_show (label);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  venta->cheque_nombre = gtk_label_new ("");
  gtk_box_pack_start (GTK_BOX (hbox), venta->cheque_nombre, FALSE, FALSE, 3);
  gtk_widget_show (venta->cheque_nombre);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  label = gtk_label_new ("Dirección: ");
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
  gtk_widget_show (label);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  venta->cheque_direccion = gtk_label_new ("");
  gtk_box_pack_start (GTK_BOX (hbox), venta->cheque_direccion, FALSE, FALSE, 3);
  gtk_widget_show (venta->cheque_direccion);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  label = gtk_label_new ("Fono: ");
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
  gtk_widget_show (label);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  venta->cheque_fono = gtk_label_new ("");
  gtk_box_pack_start (GTK_BOX (hbox), venta->cheque_fono, FALSE, FALSE, 3);
  gtk_widget_show (venta->cheque_fono);


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

  venta->cheque_serie = gtk_entry_new ();
  gtk_box_pack_end (GTK_BOX (hbox), venta->cheque_serie, FALSE, FALSE, 3);
  gtk_widget_show (venta->cheque_serie);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  label = gtk_label_new ("Número: ");
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
  gtk_widget_show (label);

  venta->cheque_numero = gtk_entry_new ();
  gtk_box_pack_end (GTK_BOX (hbox), venta->cheque_numero, FALSE, FALSE, 3);
  gtk_widget_show (venta->cheque_numero);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  label = gtk_label_new ("Banco: ");
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
  gtk_widget_show (label);

  venta->cheque_banco = gtk_entry_new ();
  gtk_box_pack_end (GTK_BOX (hbox), venta->cheque_banco, FALSE, FALSE, 3);
  gtk_widget_show (venta->cheque_banco);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  label = gtk_label_new ("Plaza: ");
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
  gtk_widget_show (label);

  venta->cheque_plaza = gtk_entry_new ();
  gtk_box_pack_end (GTK_BOX (hbox), venta->cheque_plaza, FALSE, FALSE, 3);
  gtk_widget_show (venta->cheque_plaza);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_end (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  button = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 3);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (CloseChequeWindow), NULL);

  button = gtk_button_new_from_stock (GTK_STOCK_OK);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (Vender), (gpointer)TRUE);
}

void
SelectClient (GtkWidget *widget, gpointer data)
{
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkTreeIter iter;

  GtkWidget *hbox;
  GtkWidget *vbox;

  GtkWidget *button;
  GtkWidget *scroll;

  gtk_widget_set_sensitive (gtk_widget_get_toplevel (widget), FALSE);

  venta->clients_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (venta->clients_window),
                        "Rizoma: Búsqueda de Cliente");
  gtk_window_set_resizable (GTK_WINDOW (venta->clients_window), FALSE);
  gtk_window_set_position (GTK_WINDOW (venta->clients_window), GTK_WIN_POS_CENTER_ALWAYS);
  //gtk_window_set_transient_for (GTK_WINDOW (venta->clients_window), GTK_WINDOW
  //(venta->cheque_window));
  gtk_widget_show (venta->clients_window);
  gtk_window_present (GTK_WINDOW (venta->clients_window));

  g_signal_connect (G_OBJECT (venta->clients_window), "destroy",
                    G_CALLBACK (CloseWindowClientSelection), (gpointer)widget);

  vbox = gtk_vbox_new (FALSE, 3);
  gtk_widget_show (vbox);
  gtk_container_add (GTK_CONTAINER (venta->clients_window), vbox);

  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_show (scroll);
  gtk_widget_set_size_request (scroll, 450, 200);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (vbox), scroll, FALSE, FALSE, 3);

  venta->clients_store = gtk_list_store_new (4,
                                             G_TYPE_INT,
                                             G_TYPE_STRING,
                                             G_TYPE_INT,
                                             G_TYPE_BOOLEAN,
                                             -1);
  FillClientStore (venta->clients_store);

  venta->clients_tree = gtk_tree_view_new_with_model (GTK_TREE_MODEL (venta->clients_store));
  gtk_widget_show (venta->clients_tree);
  gtk_container_add (GTK_CONTAINER (scroll), venta->clients_tree);

  gtk_window_set_focus (GTK_WINDOW (venta->clients_window),
                        GTK_WIDGET (venta->clients_tree));

  g_signal_connect (G_OBJECT (venta->clients_tree), "row-activated",
                    G_CALLBACK (AddDataEmisor), (gpointer)widget);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Código", renderer,
                                                     "text", 0,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (venta->clients_tree), column);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Nombre", renderer,
                                                     "text", 1,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (venta->clients_tree), column);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Teléfono", renderer,
                                                     "text", 2,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (venta->clients_tree), column);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_toggle_new ();
  column = gtk_tree_view_column_new_with_attributes ("Crédito", renderer,
                                                     "active", 3,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (venta->clients_tree), column);
  gtk_tree_view_column_set_resizable (column, FALSE);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_end (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  button = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 3);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (CloseWindowClientSelection), (gpointer)widget);

  button = gtk_button_new_from_stock (GTK_STOCK_ADD);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (AddClient), (gpointer)venta->clients_store);

  if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (venta->clients_store), &iter) == FALSE)
    gtk_window_set_focus (GTK_WINDOW (venta->clients_window), button);

}

void
AddDataEmisor (GtkTreeView *treeview, GtkTreePath *path, GtkTreeViewColumn *column, gpointer data)
{
  GtkWidget *widget = (GtkWidget *)data;
  GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (venta->clients_tree));
  GtkTreeIter iter;
  gint code;
  PGresult *res;

  if (gtk_tree_selection_get_selected (selection, NULL, &iter) == TRUE)
    {
      CloseWindowClientSelection (NULL, widget);

      gtk_tree_model_get (GTK_TREE_MODEL (venta->clients_store), &iter,
                          0, &code,
                          -1);

      res = EjecutarSQL (g_strdup_printf ("SELECT * FROM cliente WHERE rut=%d", code));

      gtk_entry_set_text (GTK_ENTRY (venta->venta_rut),
                          g_strdup_printf ("%s-%s", PQgetvalue (res, 0, 4), PQgetvalue (res, 0, 5)));

      gtk_label_set_markup (GTK_LABEL (venta->venta_nombre),
                            g_strdup_printf ("<span weight=\"ultrabold\">%s %s %s</span>",
                                             PQgetvalue (res, 0, 1), PQgetvalue (res, 0, 2), PQgetvalue (res, 0, 3)));

      gtk_label_set_markup (GTK_LABEL (venta->venta_direccion),
                            g_strdup_printf ("<span weight=\"ultrabold\">%s</span>", PQgetvalue (res, 0, 6)));

      gtk_label_set_markup (GTK_LABEL (venta->venta_fono),
                            g_strdup_printf ("<span weight=\"ultrabold\">%s</span>", PQgetvalue (res, 0, 7)));

      if (tipo_documento == FACTURA)
        {
          gtk_label_set_markup (GTK_LABEL (venta->factura_giro),
                                g_strdup_printf ("<span weight=\"ultrabold\">%s</span>", PQgetvalue (res, 0, 11)));
        }

      if (tipo_documento == FACTURA || tipo_documento == GUIA)
        gtk_widget_grab_focus (GTK_WIDGET (gtk_builder_get_object (builder, "discount_entry")));
    }
}


void
CloseCancelWindow (void)
{
  gtk_widget_destroy (venta->cancel_window);
  venta->cancel_window = NULL;

  gtk_widget_set_sensitive (main_window, TRUE);

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

  gtk_widget_set_sensitive (main_window, FALSE);

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
  gtk_widget_hide (GTK_WIDGET (gtk_builder_get_object (builder, "window_change_seller")));

  gtk_widget_grab_focus (GTK_WIDGET (gtk_builder_get_object (builder, "barcode_entry")));
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
                ("No existe un usuario con el identificador %d", id));
    }
}

void
WindowChangeSeller ()
{
  GtkWindow *window;

  window = GTK_WINDOW (gtk_builder_get_object (builder, "window_change_seller"));
  gtk_widget_show_all (GTK_WIDGET (window));
}

int
main (int argc, char *argv[])
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

  if (!g_file_test(config_file,
                   G_FILE_TEST_EXISTS|G_FILE_TEST_IS_REGULAR))
    {
      perror (g_strdup_printf ("Opening %s", config_file));
      printf ("Para configurar su sistema debe ejecutar rizoma-config\n");
      exit (-1);
    }
  key_file = g_key_file_new ();
  g_key_file_load_from_file(key_file, config_file, G_KEY_FILE_KEEP_COMMENTS, NULL);

  gtk_init (&argc, &argv);

  screen_width = gdk_screen_width ();
  screen_height = gdk_screen_height ();

  if (screen_width == 640 && screen_height == 480)
    solo_venta = TRUE;
  else
    solo_venta = FALSE;

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

  gtk_main();

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

gboolean
on_delete_ventas_gui (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
  GtkWidget *window;
  window = GTK_WIDGET (gtk_builder_get_object (builder, "quit_message"));
  gtk_widget_show_all (window);
  return TRUE;
}

void
exit_response (GtkDialog *dialog, gint response_id, gpointer user_data)
{
  if (response_id == GTK_RESPONSE_YES)
    {
      //TODO: find out how to get the user id
      //Asistencia (user_data->user_id, FALSE);
      gtk_main_quit();
    }
  else
    if (response_id == GTK_RESPONSE_NO)
      gtk_widget_hide (GTK_WIDGET (dialog));
}
