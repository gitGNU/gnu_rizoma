/*ventas.c
 *
 *    Copyright (C) 2004 Rizoma Tecnologia Limitada <jonathan@rizoma.cl>
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
#include<gdk/gdkkeysyms.h>

#include<stdlib.h>
#include<string.h>
#include<math.h>

#include<time.h>

#include"tipos.h"
#include"main.h"
#include"ventas.h"
#include"ventas_stats.h"
#include"credito.h"
#include"compras.h"
#include"postgres-functions.h"
#include"errors.h"
#include"manejo_productos.h"
#include"administracion_productos.h"
#include"boleta.h"
#include"config_file.h"

#include"dimentions.h"

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
CanjearProducto (GtkWidget *widget, gpointer data)
{
  GtkWidget *entry = (GtkWidget *) data;

  if (entry == NULL)
    {
      gtk_widget_set_sensitive (main_window, TRUE);

      gtk_widget_destroy (gtk_widget_get_toplevel (widget));

      gtk_window_set_focus (GTK_WINDOW (main_window), venta->barcode_entry);
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
      gtk_widget_set_sensitive (venta->window, TRUE);
      gtk_window_set_focus (GTK_WINDOW (venta->window), NULL);
      gtk_window_set_focus (GTK_WINDOW (venta->window), venta->entry_paga);

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
	  gtk_widget_set_sensitive (venta->entry_paga, FALSE);
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
ventas_box (MainBox *module_box)
{
  Productos *fill = venta->header;

  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;
  GtkTreeIter iter;

  GtkWidget *scroll;

  GtkWidget *button;
  GtkWidget *label;
  GtkWidget *vbox;
  //  GtkWidget *vbox2;
  GtkWidget *hbox;
  GtkWidget *table;

  gchar *tipo_vendedor = rizoma_get_value ("VENDEDOR");

  if (module_box->new_box != NULL)
    gtk_widget_destroy (GTK_WIDGET (module_box->new_box));

  if (accel != NULL)
    {
      gtk_window_remove_accel_group (GTK_WINDOW (main_window), accel);

      accel = NULL;
    }

  /*
    Aqui creamos la caja que va a la derecha del menu
  */
  module_box->new_box = gtk_hbox_new (FALSE, 0);
  if (solo_venta == FALSE)
    gtk_widget_set_size_request (module_box->new_box, MODULE_BOX_WIDTH, MODULE_BOX_HEIGHT);
  else
    gtk_widget_set_size_request (module_box->new_box, MODULE_LITTLE_BOX_WIDTH,
				 MODULE_LITTLE_BOX_HEIGHT);
  gtk_widget_show (module_box->new_box);
  gtk_box_pack_start (GTK_BOX (module_box->main_box), module_box->new_box, FALSE, FALSE, 0);

  /*
    Creamos y agregamos el Accel Group
  */

  accel = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (main_window), accel);

  /*
    Estamos listos para crear shortcuts
  */

  vbox = gtk_vbox_new (FALSE, 3);
  if (solo_venta == FALSE)
    gtk_widget_set_size_request (vbox, MODULE_BOX_WIDTH-5, -1);
  else
    gtk_widget_set_size_request (vbox, MODULE_LITTLE_BOX_WIDTH - 5, -1);
  gtk_widget_show (vbox);
  gtk_box_pack_start (GTK_BOX (module_box->new_box), vbox, FALSE, FALSE, 0);

  label = gtk_label_new ("Ventas");
  gtk_label_set_markup (GTK_LABEL (label),
			"<span size=\"xx-large\"><b>Ventas</b></span>");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 3);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  label = gtk_label_new ("");
  gtk_label_set_markup (GTK_LABEL (label),
			g_strdup_printf ("<b><big>Vendedor: </big></b>"));
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);

  venta->vendedor = gtk_label_new ("");
  gtk_label_set_markup (GTK_LABEL (venta->vendedor),
			g_strdup_printf ("<b><big>%s</big></b>", user_data->user));
  gtk_widget_show (venta->vendedor);
  gtk_box_pack_start (GTK_BOX (hbox), venta->vendedor, FALSE, FALSE, 3);

  venta->boleta = gtk_label_new ("");
  gtk_label_set_markup (GTK_LABEL (venta->boleta),
			g_strdup_printf ("<b><big>%.6d</big></b>", get_ticket_number (SIMPLE)));
  gtk_widget_show (venta->boleta);
  gtk_box_pack_end (GTK_BOX (hbox), venta->boleta, FALSE, FALSE, 3);

  label = gtk_label_new ("");
  gtk_label_set_markup (GTK_LABEL (label),
			g_strdup_printf ("<b><big>Boleta Nº </big></b>"));
  gtk_widget_show (label);
  gtk_box_pack_end (GTK_BOX (hbox), label, FALSE, FALSE, 3);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  label = gtk_label_new ("Código de Barras: ");
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);

  venta->barcode_entry = gtk_entry_new ();
  gtk_widget_show (venta->barcode_entry);
  gtk_widget_set_size_request (GTK_WIDGET (venta->barcode_entry), 140, -1);
  gtk_box_pack_start (GTK_BOX (hbox), venta->barcode_entry, FALSE, FALSE, 3);

  g_signal_connect (G_OBJECT (venta->barcode_entry), "activate",
		    G_CALLBACK (SearchBarcodeProduct), (gpointer)TRUE);

  gtk_window_set_focus (GTK_WINDOW (main_window), venta->barcode_entry);

  button = gtk_button_new_with_label ("Buscar");
  gtk_widget_show (button);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 3);

  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (BuscarProducto), (gpointer)venta->barcode_entry);

  gtk_widget_add_accelerator (button, "clicked", accel,
			      GDK_F5, GDK_LOCK_MASK, GTK_ACCEL_VISIBLE);

  label = gtk_label_new ("Código Simple: ");
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);

  venta->codigo_entry = gtk_entry_new ();
  gtk_widget_show (venta->codigo_entry);
  gtk_widget_set_size_request (GTK_WIDGET (venta->codigo_entry), 120, -1);
  gtk_box_pack_start (GTK_BOX (hbox), venta->codigo_entry, FALSE, FALSE, 3);

  g_signal_connect (G_OBJECT (venta->codigo_entry), "activate",
		    G_CALLBACK (SearchProductByCode), NULL);

  /*label = gtk_label_new ("");
    gtk_label_set_markup (GTK_LABEL (label),
    g_strdup_printf ("<b><big>Boleta N </big></b>"));
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);

    venta->boleta = gtk_label_new ("");
    gtk_label_set_markup (GTK_LABEL (venta->boleta),
    g_strdup_printf ("<b><big>%.6d</big></b>", get_ticket_number ()));
    gtk_widget_show (venta->boleta);
    gtk_box_pack_start (GTK_BOX (hbox), venta->boleta, FALSE, FALSE, 3);
  */
  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  label = gtk_label_new ("Producto: ");
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);

  venta->product_entry = gtk_entry_new ();
  gtk_widget_show (venta->product_entry);
  gtk_widget_set_size_request (GTK_WIDGET (venta->product_entry), 280, -1);
  gtk_box_pack_start (GTK_BOX (hbox), venta->product_entry, FALSE, FALSE, 3);

  label = gtk_label_new ("Cantidad: ");
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);

  venta->cantidad_entry = gtk_entry_new ();
  gtk_widget_show (venta->cantidad_entry);
  gtk_widget_set_size_request (venta->cantidad_entry, 50, -1);
  gtk_box_pack_start (GTK_BOX (hbox), venta->cantidad_entry, FALSE, FALSE, 3);

  gtk_entry_set_text (GTK_ENTRY (venta->cantidad_entry), "1");

  gtk_editable_select_region (GTK_EDITABLE (venta->cantidad_entry),
			      0, GTK_ENTRY (venta->cantidad_entry)->text_length);

  g_signal_connect (G_OBJECT (venta->cantidad_entry), "changed",
		    G_CALLBACK (AumentarCantidad), (gpointer) FALSE);

  g_signal_connect(G_OBJECT(venta->cantidad_entry), "activate",
		   G_CALLBACK(MoveFocus), NULL);
  label = gtk_label_new ("");
  gtk_label_set_markup (GTK_LABEL (label), "<b>Stock para: </b>");
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
  gtk_widget_show (label);

  venta->stockday = gtk_label_new ("");
  gtk_box_pack_start (GTK_BOX (hbox), venta->stockday, FALSE, FALSE, 3);
  gtk_widget_show (venta->stockday);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  table = gtk_table_new (2, 2, FALSE);
  gtk_box_pack_start (GTK_BOX (hbox), table, FALSE, FALSE, 3);
  gtk_widget_show (table);

  label = gtk_label_new ("Precio Unitario: ");
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
  gtk_table_attach_defaults (GTK_TABLE (table), label,
			     0, 1,
			     0, 1);
  gtk_widget_show (label);
  //gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);

  venta->precio_label = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (venta->precio_label), 1, 0);
  gtk_widget_set_size_request (venta->precio_label, 50, -1);
  gtk_table_attach_defaults (GTK_TABLE (table), venta->precio_label,
			     1, 2,
			     0, 1);
  gtk_widget_show (venta->precio_label);
  //gtk_box_pack_start (GTK_BOX (hbox), venta->precio_label, FALSE, FALSE, 3);

  label = gtk_label_new ("Precio Mayor: ");
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
  gtk_table_attach_defaults (GTK_TABLE (table), label,
			     0, 1,
			     1, 2);
  gtk_widget_show (label);

  venta->mayor_label = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (venta->mayor_label), 1, 0);
  gtk_table_attach_defaults (GTK_TABLE (table), venta->mayor_label,
			     1, 2,
			     1, 2);
  gtk_widget_show (venta->mayor_label);

  table = gtk_table_new (2, 2, FALSE);
  gtk_box_pack_start (GTK_BOX (hbox), table, FALSE, FALSE, 3);
  gtk_widget_show (table);

  label = gtk_label_new ("Stock: ");
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
  gtk_table_attach_defaults (GTK_TABLE (table), label,
			     0, 1,
			     0, 1);
  gtk_widget_show (label);
  //  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);

  venta->stock_label = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (venta->stock_label), 1, 0);
  gtk_widget_set_size_request (venta->stock_label, 90, -1);
  gtk_table_attach_defaults (GTK_TABLE (table), venta->stock_label,
			     1, 2,
			     0, 1);
  gtk_widget_show (venta->stock_label);
  //  gtk_box_pack_start (GTK_BOX (hbox), venta->stock_label, FALSE, FALSE, 3);

  label = gtk_label_new ("Cant. Mayorista: ");
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
  gtk_table_attach_defaults (GTK_TABLE (table), label,
			     0, 1,
			     1, 2);
  gtk_widget_show (label);

  venta->mayor_cantidad = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (venta->mayor_cantidad), 1, 0);
  gtk_table_attach_defaults (GTK_TABLE (table), venta->mayor_cantidad,
			     1, 2,
			     1, 2);
  gtk_widget_show (venta->mayor_cantidad);

  label = gtk_label_new ("Total: ");
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);

  venta->subtotal_label = gtk_label_new ("\t\t");
  gtk_misc_set_alignment (GTK_MISC (venta->subtotal_label), 1, 0);
  gtk_widget_set_size_request (venta->precio_label, 50, -1);
  gtk_widget_show (venta->subtotal_label);
  gtk_box_pack_start (GTK_BOX (hbox), venta->subtotal_label, FALSE, FALSE, 3);

  add_button = gtk_button_new_with_label ("Agregar");
  gtk_widget_show (add_button);
  gtk_box_pack_end (GTK_BOX (hbox), add_button, FALSE, FALSE, 3);

  g_signal_connect (G_OBJECT (add_button), "clicked",
		    G_CALLBACK (AgregarProducto), NULL);

  button = gtk_button_new_from_stock (GTK_STOCK_CLEAR);
  gtk_widget_show (button);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);

  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (CleanEntryAndLabelData), NULL);


  /* button = gtk_button_new_with_label ("Buscar");
     gtk_widget_show (button);
     gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);

     g_signal_connect (G_OBJECT (button), "clicked",
     G_CALLBACK (BuscarProducto), (gpointer)venta->barcode_entry);

     gtk_widget_add_accelerator (button, "clicked", accel,
     GDK_F5, GDK_LOCK_MASK, GTK_ACCEL_VISIBLE);
  */
  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

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
      do {

	if (fill->product->cantidad_mayorista > 0 && fill->product->precio_mayor > 0 && fill->product->cantidad > fill->product->cantidad_mayorista &&
	    fill->product->mayorista == TRUE)
	  precio = fill->product->precio_mayor;
	else
	  precio = fill->product->precio;

	//gtk_list_store_append (venta->store, &iter);
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

  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_show (scroll);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_AUTOMATIC);
  if (solo_venta == FALSE)
    gtk_widget_set_size_request (scroll, MODULE_BOX_WIDTH-5, 250);
  else
    gtk_widget_set_size_request (scroll, MODULE_LITTLE_BOX_WIDTH - 5, 200);

  gtk_box_pack_start (GTK_BOX (hbox), scroll, FALSE, FALSE, 3);

  venta->treeview_products = gtk_tree_view_new_with_model (GTK_TREE_MODEL (venta->store));
  gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (venta->treeview_products), TRUE);
  gtk_widget_set_size_request (GTK_WIDGET (venta->treeview_products), -1, 200);
  gtk_widget_show (venta->treeview_products);
  gtk_container_add (GTK_CONTAINER (scroll), venta->treeview_products);

  GTK_WIDGET_UNSET_FLAGS (venta->treeview_products, GTK_CAN_FOCUS);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Código", renderer,
						     "text", 0,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (venta->treeview_products), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_min_width (column, 75);
  gtk_tree_view_column_set_max_width (column, 75);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Descripción", renderer,
						     "text", 1,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (venta->treeview_products), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
  gtk_tree_view_column_set_min_width (column, 170);
  gtk_tree_view_column_set_max_width (column, 170);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Marca", renderer,
						     "text", 2,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (venta->treeview_products), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  gtk_tree_view_column_set_min_width (column, 90);
  gtk_tree_view_column_set_max_width (column, 90);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Cont.", renderer,
						     "text", 3,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (venta->treeview_products), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  gtk_tree_view_column_set_min_width (column, 35);
  gtk_tree_view_column_set_max_width (column, 35);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Uni.", renderer,
						     "text", 4,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (venta->treeview_products), column);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_min_width (column, 30);
  gtk_tree_view_column_set_max_width (column, 30);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Cant.", renderer,
						     "text", 5,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (venta->treeview_products), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_min_width (column, 50);
  gtk_tree_view_column_set_max_width (column, 50);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Precio Uni.", renderer,
						     "text", 6,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (venta->treeview_products), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_min_width (column, 70);
  gtk_tree_view_column_set_max_width (column, 70);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Sub Total", renderer,
						     "text", 7,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (venta->treeview_products), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_min_width (column, 70);
  gtk_tree_view_column_set_max_width (column, 70);
  gtk_tree_view_column_set_resizable (column, FALSE);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  button = gtk_button_new_with_label ("Eliminar Producto");
  gtk_widget_show (button);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 3);

  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (EliminarProducto), NULL);

  button = gtk_button_new_with_label ("Canjear Producto");
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 3);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (CanjearProductoWin), NULL);

  button = gtk_button_new_with_mnemonic ("_Vender");
  gtk_widget_show (button);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);

  if (strcmp (tipo_vendedor, "1") == 0)
    {
      g_signal_connect (G_OBJECT (button), "clicked",
			G_CALLBACK (TipoVenta), (gpointer)VENTA);
    }
  else
    {
      g_signal_connect (G_OBJECT (button), "clicked",
			G_CALLBACK (TipoVenta), (gpointer)SIMPLE);
    }
  /*  if (user_data->level == 0)
      g_signal_connect (G_OBJECT (button), "clicked",
      G_CALLBACK (TipoVenta), (gpointer)SIMPLE);
      else if (user_data->level == 1)
      g_signal_connect (G_OBJECT (button), "clicked",
      G_CALLBACK (TipoVenta), (gpointer)VENTA);
  */

  gtk_widget_add_accelerator (button, "clicked", accel,
			      GDK_F9, GDK_LOCK_MASK, GTK_ACCEL_VISIBLE);

  button = gtk_button_new_with_mnemonic ("_Facturar");
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (TipoVenta), (gpointer)FACTURA);

  gtk_widget_add_accelerator (button, "clicked", accel,
			      GDK_F10, GDK_LOCK_MASK, GTK_ACCEL_VISIBLE);

  button = gtk_button_new_with_mnemonic ("_Guia");
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (TipoVenta), (gpointer)GUIA);

  gtk_widget_add_accelerator (button, "clicked", accel,
			      GDK_F11, GDK_LOCK_MASK, GTK_ACCEL_VISIBLE);

  if (user_data->level == 0)
    {
      button = gtk_button_new_with_mnemonic ("Canc_elar Ventas");
      gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);
      gtk_widget_show (button);

      g_signal_connect (G_OBJECT (button), "clicked",
			G_CALLBACK (CancelWindow), NULL);

      gtk_widget_add_accelerator (button, "clicked", accel,
				  GDK_F8, GDK_LOCK_MASK, GTK_ACCEL_VISIBLE);

    }

  button = gtk_button_new_with_mnemonic ("C_ambiar Vendedor");
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (WindowChangeSeller), NULL);

  gtk_widget_add_accelerator (button, "clicked", accel,
			      GDK_F10, GDK_LOCK_MASK, GTK_ACCEL_VISIBLE);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_widget_set_size_request (hbox, MODULE_BOX_WIDTH, -1);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  /*
    vbox2 = gtk_vbox_new (FALSE, 3);
    gtk_widget_show (vbox2);
    gtk_box_pack_start (GTK_BOX (hbox), vbox2, FALSE, FALSE, 3);

    label = gtk_label_new ("Sub-Total: ");
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (vbox2), label, FALSE, FALSE, 3);

    gtk_label_set_markup (GTK_LABEL (label),
    g_strdup_printf ("<span size=\"30000\">%s</span>",
    gtk_label_get_text (GTK_LABEL (label))));

    venta->sub_total_label = gtk_label_new ("");
    gtk_widget_show (venta->sub_total_label);
    gtk_box_pack_start (GTK_BOX (vbox2), venta->sub_total_label, FALSE, FALSE, 3);

    vbox2 = gtk_vbox_new (FALSE, 3);
    gtk_widget_show (vbox2);
    gtk_box_pack_start (GTK_BOX (hbox), vbox2, FALSE, FALSE, 3);

    label = gtk_label_new ("Dcto. en Pesos:");
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (vbox2), label, FALSE, FALSE, 3);

    venta->discount_entry = gtk_entry_new ();
    gtk_entry_set_text (GTK_ENTRY (venta->discount_entry), "0");
    gtk_widget_show (venta->discount_entry);
    gtk_widget_set_size_request (venta->discount_entry, 90, -1);
    gtk_box_pack_start (GTK_BOX (vbox2), venta->discount_entry, FALSE, FALSE, 3);

    g_signal_connect (G_OBJECT (venta->discount_entry), "activate",
    G_CALLBACK (Descuento), NULL);

    button = gtk_button_new_with_label ("Descontar");
    gtk_widget_show (button);
    gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 3);

    g_signal_connect (G_OBJECT (button), "clicked",
    G_CALLBACK (Descuento), NULL);

    box = gtk_hbox_new (FALSE, 3);
    gtk_widget_show (box);
    gtk_box_pack_start (GTK_BOX (vbox2), box, FALSE, FALSE, 3);

    venta->discount_label = gtk_label_new ("0");
    gtk_widget_show (venta->discount_label);
    gtk_box_pack_start (GTK_BOX (box), venta->discount_label, FALSE, FALSE, 3);

    label = gtk_label_new ("%");
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 3);

    vbox2 = gtk_vbox_new (FALSE, 3);
    gtk_widget_show (vbox2);
    gtk_box_pack_end (GTK_BOX (hbox), vbox2, FALSE, FALSE, 3);
  */

  label = gtk_label_new ("Total: ");
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
  gtk_widget_show (label);

  gtk_label_set_markup (GTK_LABEL (label),
			g_strdup_printf ("<span size=\"30000\">%s</span>",
					 gtk_label_get_text (GTK_LABEL (label))));

  venta->total_label = gtk_label_new ("");
  gtk_widget_show (venta->total_label);
  gtk_box_pack_end (GTK_BOX (hbox), venta->total_label, FALSE, FALSE, 3);

  if (venta->header != NULL)
    CalcularVentas (venta->header);

  gtk_window_set_focus (GTK_WINDOW (main_window), venta->barcode_entry);
}

gboolean
SearchProductByCode (void)
{
  gchar *codigo = g_strdup (gtk_entry_get_text (GTK_ENTRY (venta->codigo_entry)));
  gint precio;
  gdouble stockday;
  gdouble stock;
  PGresult *res;
  gint venta_directa = atoi(rizoma_get_value("VENTA_DIRECTA"));
      
  res = EjecutarSQL (g_strdup_printf ("SELECT codigo_corto, descripcion, marca, contenido, unidad, stock, precio, "
				      "precio_mayor, cantidad_mayor, mayorista, barcode FROM producto WHERE codigo_corto='%s' "
				      "AND stock!=0", codigo));
  if (res != NULL && PQntuples (res) != 0)
    {
      mayorista = strcmp (PQvaluebycol (res, 0, "mayorista"), "t") == 0 ? TRUE : FALSE;
	
      gtk_entry_set_text (GTK_ENTRY (venta->product_entry),
			  g_strdup_printf ("%s  %s  %s  %s", PQvaluebycol (res, 0, "descripcion"),
					   PQvaluebycol (res, 0, "marca"), PQvaluebycol (res, 0, "contenido"),
					   PQvaluebycol (res, 0, "unidad")));

      stock = strtod (PUT (PQvaluebycol (res, 0, "stock")), (char **)NULL);

      stockday = GetDayToSell (PQvaluebycol (res, 0, "barcode"));

      if (stock <= GetMinStock (PQvaluebycol (res, 0, "barcode")))
	gtk_label_set_markup (GTK_LABEL (venta->stockday),
			      g_strdup_printf
			      ("<span foreground=\"red\"><b>%.2f dia(s)</b></span>", stockday));
      else
	gtk_label_set_markup (GTK_LABEL (venta->stockday),
			      g_strdup_printf ("<b>%.2f dia(s)</b>", stockday));

      gtk_label_set_markup (GTK_LABEL (venta->precio_label),
			    g_strdup_printf ("<span weight=\"ultrabold\">%s</span>",
					     PutPoints (PQvaluebycol (res, 0, "precio"))));

      gtk_label_set_markup (GTK_LABEL (venta->mayor_label),
			    g_strdup_printf ("<span weight=\"ultrabold\">%s</span>",
					     PutPoints (PQvaluebycol (res, 0, "precio_mayor"))));

      gtk_label_set_markup (GTK_LABEL (venta->mayor_cantidad),
			    g_strdup_printf ("<span weight=\"ultrabold\">%s</span>",
					     PutPoints (PQvaluebycol (res, 0, "cantidad_mayor"))));

      gtk_label_set_markup (GTK_LABEL (venta->stock_label),
			    g_strdup_printf ("<span weight=\"ultrabold\">%.2f</span>",
					     stock));

      precio = atoi (PQvaluebycol (res, 0, "precio"));

      gtk_label_set_markup (GTK_LABEL (venta->subtotal_label),
			    g_strdup_printf
			    ("<span weight=\"ultrabold\">%s</span>",
			     g_strdup_printf ("%.0f",
					      strtod (g_strdup (gtk_entry_get_text (GTK_ENTRY (venta->cantidad_entry))), (char **)NULL) *
					      precio)));

      gtk_entry_set_text (GTK_ENTRY (venta->codigo_entry), PQvaluebycol (res, 0, "codigo_corto"));
      gtk_entry_set_text (GTK_ENTRY (venta->barcode_entry), PQvaluebycol (res, 0, "barcode"));
      if (precio != 0)
	{
	  if( venta_directa == 1 ) {
	    if( VentaFraccion( PQvaluebycol( res, 0, "cantidad_mayor" ) ) ) {
	      gtk_window_set_focus( GTK_WINDOW( main_window ), venta->cantidad_entry );
	      gtk_widget_set_sensitive (add_button, TRUE);
	    } else {
	      AgregarProducto( NULL, NULL );
	    }
	  } else {
	    gtk_window_set_focus( GTK_WINDOW( main_window ), venta->cantidad_entry );
	    gtk_widget_set_sensitive( add_button, TRUE );
	  }
	}
      else
	gtk_window_set_focus (GTK_WINDOW (main_window), venta->barcode_entry);

      return TRUE;
    }
  else
    {
      if (strcmp (codigo, "") != 0)
	{
	  AlertMSG (venta->barcode_entry, g_strdup_printf
		    ("No existe un producto con el código %s!!", codigo));

	  /*gtk_entry_set_text (GTK_ENTRY (venta->codigo_entry), "");
	    gtk_entry_set_text (GTK_ENTRY (venta->product_entry), "");
	    gtk_entry_set_text (GTK_ENTRY (venta->barcode_entry), "");
	    gtk_label_set_text (GTK_LABEL (venta->stockday), "");
	    gtk_label_set_text (GTK_LABEL (venta->precio_label), "");
	    gtk_label_set_text (GTK_LABEL (venta->stock_label), "");*/
	  CleanSellLabels ();
	}
      /*      else if (GetCurrentStock (GetDataByOne (g_strdup_printf ("SELECT barcode FROM producto "
	      "WHERE codigo='%s'", codigo))) == 0)
	      {
	      AlertMSG ("No ahi mercaderia en Stock.\nDebe ingresar mercaderia");
      */
      /*gtk_entry_set_text (GTK_ENTRY (venta->codigo_entry), "");
	gtk_entry_set_text (GTK_ENTRY (venta->product_entry), "");
	gtk_entry_set_text (GTK_ENTRY (venta->barcode_entry), "");
	gtk_label_set_text (GTK_LABLE (venta->stockday));
	gtk_label_set_text (GTK_LABEL (venta->precio_label), "");
	gtk_label_set_text (GTK_LABEL (venta->stock_label), "");*/
      /*  CleanSellLabels ();
	  }*/
      else
	{
	  /*gtk_entry_set_text (GTK_ENTRY (venta->codigo_entry), "");
	    gtk_entry_set_text (GTK_ENTRY (venta->product_entry), "");
	    gtk_entry_set_text (GTK_ENTRY (venta->barcode_entry), "");
	    gtk_label_set_text (GTK_LABEL (venta->stockday), "");
	    s  gtk_label_set_text (GTK_LABEL (venta->precio_label), "");
	    gtk_label_set_text (GTK_LABEL (venta->subtotal_label), "");
	    gtk_label_set_text (GTK_LABEL (venta->stock_label), "");*/
	  CleanSellLabels ();

	  //	  WindowProductSelect ();
	}

      return FALSE;
    }
}

gboolean
AgregarProducto (GtkButton *button, gpointer data)
{
  gchar *codigo = g_strdup (gtk_entry_get_text (GTK_ENTRY (venta->codigo_entry)));
  gchar *barcode = g_strdup (gtk_entry_get_text (GTK_ENTRY (venta->barcode_entry)));
  guint32 total;
  gdouble stock = GetCurrentStock (barcode);
  gdouble cantidad;
  GtkTreeIter iter;

  cantidad = strtod (PUT(g_strdup (gtk_entry_get_text (GTK_ENTRY (venta->cantidad_entry)))),
		     (char **)NULL);

  if (cantidad <= 0 && VentaFraccion (barcode) == FALSE)
    {
      /*AlertMSG (venta->cantidad_entry, "No puede vender una cantidad 0 o menor");

	return FALSE;*/

    }
  if ((strcmp ("0", CutPoints (g_strdup (gtk_label_get_text (GTK_LABEL (venta->precio_label)))))) == 0)
    {
      AlertMSG (venta->barcode_entry, "No se pueden vender productos con precio 0");

      CleanEntryAndLabelData ();

      return FALSE;
    }
  else if (cantidad > stock)
    {
      AlertMSG (venta->cantidad_entry, "No puede vender mas productos de los que tiene en stock");

      gtk_window_set_focus (GTK_WINDOW (main_window), venta->cantidad_entry);

      return FALSE;
    }
  else if (strchr (gtk_entry_get_text (GTK_ENTRY (venta->cantidad_entry)), ',') != NULL ||
	   strchr (gtk_entry_get_text (GTK_ENTRY (venta->cantidad_entry)), '.') != NULL)
    {
      if (VentaFraccion (barcode) == FALSE)
	{
	  AlertMSG (venta->cantidad_entry,
		    "Este producto no se puede vender por fracción de producto");
	  gtk_window_set_focus (GTK_WINDOW (main_window), venta->cantidad_entry);
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
	      AlertMSG (venta->cantidad_entry,
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

      /*
	gtk_label_set_markup (GTK_LABEL (venta->sub_total_label),
	g_strdup_printf ("<span size=\"40000\">%s</span>",
	PutPoints (g_strdup_printf ("%d", total))));
      */
      gtk_label_set_markup (GTK_LABEL (venta->total_label),
			    g_strdup_printf ("<span size=\"40000\">%s</span>",
					     PutPoints (g_strdup_printf ("%lu", total))));

      gtk_window_set_focus( GTK_WINDOW( main_window ), venta->barcode_entry );
    }

  return TRUE;
}

void
CleanSellLabels (void)
{
  gtk_entry_set_text (GTK_ENTRY (venta->codigo_entry), "");
  gtk_entry_set_text (GTK_ENTRY (venta->product_entry), "");
  gtk_entry_set_text (GTK_ENTRY (venta->barcode_entry), "");
  gtk_label_set_text (GTK_LABEL (venta->stockday), "");
  gtk_label_set_text (GTK_LABEL (venta->precio_label), "");
  gtk_label_set_text (GTK_LABEL (venta->stock_label), "");
}

void
CleanEntryAndLabelData (void)
{
  gtk_entry_set_text (GTK_ENTRY (venta->product_entry), "");
  gtk_entry_set_text (GTK_ENTRY (venta->codigo_entry), "");
  gtk_entry_set_text (GTK_ENTRY (venta->cantidad_entry), "1");
  gtk_entry_set_text (GTK_ENTRY (venta->barcode_entry), "");
  gtk_label_set_text (GTK_LABEL (venta->stockday), "");
  gtk_label_set_text (GTK_LABEL (venta->mayor_cantidad), "");
  gtk_label_set_text (GTK_LABEL (venta->precio_label), "");
  gtk_label_set_text (GTK_LABEL (venta->mayor_label), "");
  gtk_label_set_text (GTK_LABEL (venta->subtotal_label), "");
  //  gtk_label_set_text (GTK_LABEL (venta->sub_total_label), "");
  gtk_label_set_text (GTK_LABEL (venta->stock_label), "");

  gtk_widget_set_sensitive (add_button, FALSE);

  //  gtk_entry_set_text (GTK_ENTRY (venta->discount_entry), "0");
  //  gtk_label_set_text (GTK_LABEL (venta->discount_label), "0");

  gtk_window_set_focus (GTK_WINDOW (main_window), venta->barcode_entry);
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

	gtk_label_set_markup (GTK_LABEL (venta->total_label),
	g_strdup_printf ("<span size=\"40000\">%d</span>", CalcularTotal ()));
      */

      //      gtk_entry_set_text (GTK_ENTRY (venta->discount_entry), "0");
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
					  (GTK_LABEL (venta->total_label)))));
  gchar *discount = "0";
  gint maquina = atoi (rizoma_get_value ("MAQUINA"));
  gint vendedor = user_data->user_id;
  gint paga_con;
  gint ticket;
  gboolean canceled;

  if (tipo_documento != VENTA && tipo_documento != FACTURA)
    paga_con = atoi (g_strdup (gtk_entry_get_text (GTK_ENTRY (venta->entry_paga))));

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
      ErrorMSG (venta->entry_paga, "No esta pagando con el dinero suficiente");
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
    discount = g_strdup (gtk_entry_get_text (GTK_ENTRY (venta->discount_entry)));

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

  CloseSellWindow ();

  gtk_list_store_clear (venta->store);

  CleanEntryAndLabelData ();

  gtk_label_set_text (GTK_LABEL (venta->total_label), "");

  gtk_widget_set_sensitive (main_window, TRUE);

  gtk_window_set_focus (GTK_WINDOW (main_window), venta->barcode_entry);

  if (monto >= 180 && ticket != -1)
    gtk_label_set_markup (GTK_LABEL (venta->boleta),
			  g_strdup_printf ("<b><big>%.6d</big></b>", ticket+1));
  else
    gtk_label_set_markup (GTK_LABEL (venta->boleta),
			  g_strdup_printf ("<b><big>%.6d</big></b>", get_ticket_number (SIMPLE)));

  PrintDocument (tipo_documento, rut, monto, ticket, venta->products);

  ListClean ();

  WindowChangeSeller();

  return 0;
}

void
MoveFocus (GtkEntry *entry, gpointer data)
{
  gtk_window_set_focus(GTK_WINDOW(main_window), add_button);
}

void
AumentarCantidad (GtkEntry *entry, gpointer data)
{
  gdouble cantidad = g_strtod (PUT (g_strdup (gtk_entry_get_text (GTK_ENTRY (venta->cantidad_entry)))), (char **)NULL);
  gint precio = atoi (CutPoints (g_strdup (gtk_label_get_text (GTK_LABEL (venta->precio_label)))));
  gint precio_mayor = atoi (CutPoints (g_strdup (gtk_label_get_text (GTK_LABEL (venta->mayor_label)))));
  gdouble cantidad_mayor = strtod (PUT (g_strdup (gtk_label_get_text (GTK_LABEL (venta->mayor_cantidad)))),
				   (char **)NULL);
  guint32 subtotal;

  if (precio != 0 && ((mayorista == FALSE || cantidad < cantidad_mayor) ||
		      (mayorista == TRUE && (cantidad_mayor == 0 || precio_mayor == 0))))
    {
      subtotal = llround ((gdouble)cantidad * precio);

      gtk_label_set_markup (GTK_LABEL (venta->subtotal_label),
			    g_strdup_printf ("<span weight=\"ultrabold\">%s</span>",
					     PutPoints (g_strdup_printf ("%lu", subtotal))));
    }
  else
    if (precio_mayor != 0 && mayorista == TRUE && cantidad >= cantidad_mayor)
      {
	subtotal = llround ((gdouble)cantidad * precio_mayor);

	gtk_label_set_markup (GTK_LABEL (venta->subtotal_label),
			      g_strdup_printf ("<span weight=\"ultrabold\">%s</span>",
					       PutPoints (g_strdup_printf ("%u", subtotal))));
      }
}

gint
TipoVenta (GtkWidget *widget, gpointer data)
{
  GtkWidget *button;
  GtkWidget *vbox2;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *frame;
  GtkWidget *label;

  gchar *tipo_vendedor = rizoma_get_value("VENDEDOR");

  if (venta->header == NULL)
    {
      ErrorMSG (venta->barcode_entry, "No hay productos para vender");
      return 0;
    }

  if (venta->window != NULL)
    return -1;

  gtk_widget_set_sensitive (main_window, FALSE);

  tipo_documento = (gint) data;

  venta->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  if (tipo_documento == SIMPLE)
    gtk_window_set_title (GTK_WINDOW (venta->window), "Tipo de Venta: Boleta");
  else if (tipo_documento == FACTURA)
    gtk_window_set_title (GTK_WINDOW (venta->window), "Tipo de Venta: Factura");

  gtk_window_set_position (GTK_WINDOW (venta->window), GTK_WIN_POS_CENTER_ALWAYS);
  gtk_widget_show (venta->window);
  gtk_window_present (GTK_WINDOW (venta->window));
  gtk_window_set_resizable (GTK_WINDOW (venta->window), FALSE);
  gtk_widget_set_size_request (venta->window, 220, -1);

  g_signal_connect (G_OBJECT (venta->window), "destroy",
		    G_CALLBACK (CloseSellWindow), NULL);

  vbox2 = gtk_vbox_new (FALSE, 3);
  gtk_widget_show (vbox2);
  gtk_container_add (GTK_CONTAINER (venta->window), vbox2);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_end (GTK_BOX (vbox2), hbox, FALSE, FALSE, 3);

  if (strcmp (tipo_vendedor, "1") == 0)
    {
      venta->sell_button = gtk_button_new_with_mnemonic ("_Imprimir");
      gtk_box_pack_end (GTK_BOX (hbox), venta->sell_button, FALSE, FALSE, 3);
      gtk_widget_show (venta->sell_button);

      SendCursorTo (NULL, venta->sell_button);

      g_signal_connect (G_OBJECT (venta->sell_button), "clicked",
			G_CALLBACK (Vender), NULL);
    }
  else
    {
      venta->sell_button = gtk_button_new_with_mnemonic ("_Vender");
      gtk_widget_show (venta->sell_button);
      gtk_box_pack_end (GTK_BOX (hbox), venta->sell_button, FALSE, FALSE, 3);

      gtk_widget_set_sensitive (venta->sell_button, FALSE);

      g_signal_connect (G_OBJECT (venta->sell_button), "clicked",
			G_CALLBACK (Vender), NULL);
    }

  /*  if (user_data->level == 0)
      {
      venta->sell_button = gtk_button_new_with_mnemonic ("_Vender");
      gtk_widget_show (venta->sell_button);
      gtk_box_pack_end (GTK_BOX (hbox), venta->sell_button, FALSE, FALSE, 3);

      gtk_widget_set_sensitive (venta->sell_button, FALSE);

      g_signal_connect (G_OBJECT (venta->sell_button), "clicked",
      G_CALLBACK (Vender), NULL);
      }
      else
      {
      venta->sell_button = gtk_button_new_with_mnemonic ("_Imprimir");
      gtk_box_pack_end (GTK_BOX (hbox), venta->sell_button, FALSE, FALSE, 3);
      gtk_widget_show (venta->sell_button);

      SendCursorTo (NULL, venta->sell_button);

      g_signal_connect (G_OBJECT (venta->sell_button), "clicked",
      G_CALLBACK (Vender), NULL);
      }
  */
  button = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
  gtk_widget_show (button);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);

  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (CloseSellWindow), NULL);

  venta->tipo_venta = CASH;

  if (tipo_documento == FACTURA)
    {
      frame = gtk_frame_new ("Datos Cliente");
      gtk_box_pack_start (GTK_BOX (vbox2), frame, FALSE, FALSE, 3);
      gtk_widget_show (frame);


      vbox = gtk_vbox_new (FALSE, 3);
      gtk_container_add (GTK_CONTAINER (frame), vbox);
      gtk_widget_show (vbox);

      hbox = gtk_hbox_new (FALSE, 3);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
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

      gtk_window_set_focus (GTK_WINDOW (venta->window), venta->venta_rut);

      hbox = gtk_hbox_new (FALSE, 3);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
      gtk_widget_show (hbox);

      label = gtk_label_new ("Nombre: ");
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
      gtk_widget_show (label);

      hbox = gtk_hbox_new (FALSE, 3);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
      gtk_widget_show (hbox);

      venta->venta_nombre = gtk_label_new ("");
      gtk_box_pack_start (GTK_BOX (hbox), venta->venta_nombre, FALSE, FALSE, 3);
      gtk_widget_show (venta->venta_nombre);

      hbox = gtk_hbox_new (FALSE, 3);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
      gtk_widget_show (hbox);

      label = gtk_label_new ("Dirección: ");
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
      gtk_widget_show (label);

      hbox = gtk_hbox_new (FALSE, 3);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
      gtk_widget_show (hbox);

      venta->venta_direccion = gtk_label_new ("");
      gtk_box_pack_start (GTK_BOX (hbox), venta->venta_direccion, FALSE, FALSE, 3);
      gtk_widget_show (venta->venta_direccion);

      hbox = gtk_hbox_new (FALSE, 3);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
      gtk_widget_show (hbox);

      label = gtk_label_new ("Fono: ");
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
      gtk_widget_show (label);

      hbox = gtk_hbox_new (FALSE, 3);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
      gtk_widget_show (hbox);

      venta->venta_fono = gtk_label_new ("");
      gtk_box_pack_start (GTK_BOX (hbox), venta->venta_fono, FALSE, FALSE, 3);
      gtk_widget_show (venta->venta_fono);

      hbox = gtk_hbox_new (FALSE, 3);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
      gtk_widget_show (hbox);

      label = gtk_label_new ("Giro: ");
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
      gtk_widget_show (label);

      hbox = gtk_hbox_new (FALSE, 3);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
      gtk_widget_show (hbox);

      venta->factura_giro = gtk_label_new ("");
      gtk_box_pack_start (GTK_BOX (hbox), venta->factura_giro, FALSE, FALSE, 3);
      gtk_widget_show (venta->factura_giro);

      frame = gtk_frame_new ("Pago");
      gtk_widget_show (frame);
      gtk_box_pack_start (GTK_BOX (vbox2), frame, FALSE, FALSE, 3);

      gtk_widget_set_size_request (frame, 215, -1);

      vbox = gtk_vbox_new (FALSE, 3);
      gtk_widget_show (vbox);
      gtk_container_add (GTK_CONTAINER (frame), vbox);

      hbox = gtk_hbox_new (FALSE, 3);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
      gtk_widget_show (hbox);

      label = gtk_label_new ("Forma de pago:");
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
      gtk_widget_show (label);

      venta->venta_pago = gtk_label_new ("");
      gtk_label_set_markup (GTK_LABEL (venta->venta_pago), "<b>Contado</b>");
      gtk_box_pack_end (GTK_BOX (hbox), venta->venta_pago, FALSE, FALSE, 3);
      gtk_widget_show (venta->venta_pago);

      hbox = gtk_hbox_new (FALSE, 3);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
      gtk_widget_show (hbox);

      label = gtk_label_new ("Dias de Pago: ");
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
      gtk_widget_show (label);

      venta->forma_pago = gtk_entry_new ();
      gtk_widget_set_size_request (venta->forma_pago, 100, -1);
      gtk_box_pack_end (GTK_BOX (hbox), venta->forma_pago, FALSE, FALSE, 3);
      gtk_widget_show (venta->forma_pago);

      gtk_widget_set_sensitive (venta->forma_pago, FALSE);

      g_signal_connect (G_OBJECT (venta->forma_pago), "activate",
			G_CALLBACK (SendCursorTo), (gpointer)venta->sell_button);


      hbox = gtk_hbox_new (FALSE, 3);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
      gtk_widget_show (hbox);

      label = gtk_label_new ("Descuento %: ");
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
      gtk_widget_show (label);

      venta->discount_entry = gtk_entry_new ();
      gtk_widget_set_size_request (venta->discount_entry, 100, -1);
      gtk_box_pack_end (GTK_BOX (hbox), venta->discount_entry, FALSE, FALSE, 3);
      gtk_widget_show (venta->discount_entry);

      gtk_entry_set_text (GTK_ENTRY (venta->discount_entry), "0");

      g_signal_connect (G_OBJECT (venta->discount_entry), "activate",
			G_CALLBACK (Descuento), (gpointer) FALSE);
    }
  else if (tipo_documento == GUIA)
    {
      frame = gtk_frame_new ("Datos Cliente");
      gtk_box_pack_start (GTK_BOX (vbox2), frame, FALSE, FALSE, 3);
      gtk_widget_show (frame);

      vbox = gtk_vbox_new (FALSE, 3);
      gtk_container_add (GTK_CONTAINER (frame), vbox);
      gtk_widget_show (vbox);

      hbox = gtk_hbox_new (FALSE, 3);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
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

      gtk_window_set_focus (GTK_WINDOW (venta->window), venta->venta_rut);

      hbox = gtk_hbox_new (FALSE, 3);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
      gtk_widget_show (hbox);

      label = gtk_label_new ("Nombre: ");
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
      gtk_widget_show (label);

      hbox = gtk_hbox_new (FALSE, 3);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
      gtk_widget_show (hbox);

      venta->venta_nombre = gtk_label_new ("");
      gtk_box_pack_start (GTK_BOX (hbox), venta->venta_nombre, FALSE, FALSE, 3);
      gtk_widget_show (venta->venta_nombre);

      hbox = gtk_hbox_new (FALSE, 3);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
      gtk_widget_show (hbox);

      label = gtk_label_new ("Dirección: ");
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
      gtk_widget_show (label);

      hbox = gtk_hbox_new (FALSE, 3);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
      gtk_widget_show (hbox);

      venta->venta_direccion = gtk_label_new ("");
      gtk_box_pack_start (GTK_BOX (hbox), venta->venta_direccion, FALSE, FALSE, 3);
      gtk_widget_show (venta->venta_direccion);

      hbox = gtk_hbox_new (FALSE, 3);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
      gtk_widget_show (hbox);

      label = gtk_label_new ("Fono: ");
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
      gtk_widget_show (label);

      hbox = gtk_hbox_new (FALSE, 3);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
      gtk_widget_show (hbox);

      venta->venta_fono = gtk_label_new ("");
      gtk_box_pack_start (GTK_BOX (hbox), venta->venta_fono, FALSE, FALSE, 3);
      gtk_widget_show (venta->venta_fono);

      hbox = gtk_hbox_new (FALSE, 3);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
      gtk_widget_show (hbox);

      label = gtk_label_new ("Giro: ");
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
      gtk_widget_show (label);

      hbox = gtk_hbox_new (FALSE, 3);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
      gtk_widget_show (hbox);

      venta->factura_giro = gtk_label_new ("");
      gtk_box_pack_start (GTK_BOX (hbox), venta->factura_giro, FALSE, FALSE, 3);
      gtk_widget_show (venta->factura_giro);

      frame = gtk_frame_new ("Pago");
      gtk_widget_show (frame);
      gtk_box_pack_start (GTK_BOX (vbox2), frame, FALSE, FALSE, 3);

      gtk_widget_set_size_request (frame, 215, -1);

      vbox = gtk_vbox_new (FALSE, 3);
      gtk_widget_show (vbox);
      gtk_container_add (GTK_CONTAINER (frame), vbox);

      hbox = gtk_hbox_new (FALSE, 3);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
      gtk_widget_show (hbox);

      label = gtk_label_new ("Forma de pago:");
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
      gtk_widget_show (label);

      venta->venta_pago = gtk_label_new ("");
      gtk_label_set_markup (GTK_LABEL (venta->venta_pago), "<b>Contado</b>");
      gtk_box_pack_end (GTK_BOX (hbox), venta->venta_pago, FALSE, FALSE, 3);
      gtk_widget_show (venta->venta_pago);

      hbox = gtk_hbox_new (FALSE, 3);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
      gtk_widget_show (hbox);

      label = gtk_label_new ("Dias de Pago: ");
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
      gtk_widget_show (label);

      venta->forma_pago = gtk_entry_new ();
      gtk_widget_set_size_request (venta->forma_pago, 100, -1);
      gtk_box_pack_end (GTK_BOX (hbox), venta->forma_pago, FALSE, FALSE, 3);
      gtk_widget_show (venta->forma_pago);

      gtk_widget_set_sensitive (venta->forma_pago, FALSE);

      g_signal_connect (G_OBJECT (venta->forma_pago), "activate",
			G_CALLBACK (SendCursorTo), (gpointer)venta->sell_button);


      hbox = gtk_hbox_new (FALSE, 3);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
      gtk_widget_show (hbox);

      label = gtk_label_new ("Descuento %: ");
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
      gtk_widget_show (label);

      venta->discount_entry = gtk_entry_new ();
      gtk_widget_set_size_request (venta->discount_entry, 100, -1);
      gtk_box_pack_end (GTK_BOX (hbox), venta->discount_entry, FALSE, FALSE, 3);
      gtk_widget_show (venta->discount_entry);

      gtk_entry_set_text (GTK_ENTRY (venta->discount_entry), "0");

      g_signal_connect (G_OBJECT (venta->discount_entry), "activate",
			G_CALLBACK (Descuento), (gpointer) FALSE);
    }
  else if (tipo_documento == VENTA)
    {

    }
  else
    {
      frame = gtk_frame_new ("Descuentos");
      gtk_widget_show (frame);
      gtk_box_pack_start (GTK_BOX (vbox2), frame, FALSE, FALSE, 3);

      gtk_widget_set_size_request (frame, 215, -1);

      vbox = gtk_vbox_new (FALSE, 3);
      gtk_widget_show (vbox);
      gtk_container_add (GTK_CONTAINER (frame), vbox);

      hbox = gtk_hbox_new (FALSE, 3);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
      gtk_widget_show (hbox);

      label = gtk_label_new ("Ajuste Sencillo: ");
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
      gtk_widget_show (label);

      hbox = gtk_hbox_new (FALSE, 3);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
      gtk_widget_show (hbox);

      venta->sencillo = gtk_entry_new ();
      gtk_widget_set_size_request (venta->sencillo, 100, -1);
      gtk_box_pack_end (GTK_BOX (hbox), venta->sencillo, FALSE, FALSE, 3);
      gtk_widget_show (venta->sencillo);

      gtk_entry_set_text (GTK_ENTRY (venta->sencillo), "0");

      if (tipo_documento != FACTURA)
	gtk_window_set_focus (GTK_WINDOW (venta->window), venta->sencillo);

      gtk_editable_select_region (GTK_EDITABLE (venta->sencillo), 0, -1);

      g_signal_connect (G_OBJECT (venta->sencillo), "activate",
			G_CALLBACK (Descuento), (gpointer) TRUE);

      hbox = gtk_hbox_new (FALSE, 3);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
      gtk_widget_show (hbox);

      label = gtk_label_new ("Descuento %: ");
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
      gtk_widget_show (label);

      hbox = gtk_hbox_new (FALSE, 3);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
      gtk_widget_show (hbox);

      venta->discount_entry = gtk_entry_new ();
      gtk_widget_set_size_request (venta->discount_entry, 100, -1);
      gtk_box_pack_end (GTK_BOX (hbox), venta->discount_entry, FALSE, FALSE, 3);
      gtk_widget_show (venta->discount_entry);

      gtk_entry_set_text (GTK_ENTRY (venta->discount_entry), "0");

      g_signal_connect (G_OBJECT (venta->discount_entry), "activate",
			G_CALLBACK (Descuento), (gpointer) FALSE);
    }

  //  if (user_data->level == 0)
  if (strcmp (tipo_vendedor, "0") == 0)
    {
      frame = gtk_frame_new ("Pago Contado");
      gtk_widget_show (frame);
      gtk_box_pack_start (GTK_BOX (vbox2), frame, FALSE, FALSE, 3);

      gtk_widget_set_size_request (frame, 215, -1);

      vbox = gtk_vbox_new (FALSE, 3);
      gtk_widget_show (vbox);
      gtk_container_add (GTK_CONTAINER (frame), vbox);

      hbox = gtk_hbox_new (FALSE, 3);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
      gtk_widget_show (hbox);

      label = gtk_label_new ("Efectivo");
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
      gtk_widget_show (label);

      /*
	label = gtk_label_new ("Cheque");
	gtk_box_pack_end (GTK_BOX (hbox), label, FALSE, FALSE, 3);
	gtk_widget_show (label);

	hbox = gtk_hbox_new (FALSE, 3);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
      */

      venta->entry_paga = gtk_entry_new ();
      gtk_entry_set_alignment (GTK_ENTRY (venta->entry_paga), 1);
      gtk_widget_set_size_request (venta->entry_paga, 130, -1);
      gtk_box_pack_start (GTK_BOX (hbox), venta->entry_paga, FALSE, FALSE, 3);
      gtk_widget_show (venta->entry_paga);

      if (tipo_documento == SIMPLE)
	g_signal_connect (G_OBJECT (venta->discount_entry), "activate",
			  G_CALLBACK (SendCursorTo), (gpointer)venta->entry_paga);

      if (tipo_documento != FACTURA && tipo_documento != GUIA && tipo_documento != VENTA)
	g_signal_connect (G_OBJECT (venta->sencillo), "activate",
			  G_CALLBACK (SendCursorTo), (gpointer)venta->entry_paga);

      if (tipo_documento == FACTURA || tipo_documento == GUIA)
	g_signal_connect (G_OBJECT (venta->forma_pago), "activate",
			  G_CALLBACK (SendCursorTo), (gpointer)venta->entry_paga);

      g_signal_connect (G_OBJECT (venta->entry_paga), "changed",
			G_CALLBACK (CalcularVuelto), NULL);

      g_signal_connect (G_OBJECT (venta->entry_paga), "activate",
			G_CALLBACK (TiposVenta), (gpointer) venta->sell_button);


      /*  button = gtk_button_new_with_mnemonic ("C_heque");
	  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);
	  gtk_widget_show (button);

	  g_signal_connect (G_OBJECT (button), "clicked",
	  G_CALLBACK (PagoCheque), NULL);
      */
      venta->label_vuelto = gtk_label_new ("");
      gtk_label_set_markup (GTK_LABEL (venta->label_vuelto),
			    "<span size=\"30000\"> </span>");
      gtk_widget_show (venta->label_vuelto);
      gtk_box_pack_start (GTK_BOX (vbox), venta->label_vuelto, FALSE, FALSE, 3);
    }

  return 0;
}

void
CloseSellWindow (void)
{
  gtk_widget_destroy (venta->window);
  venta->window = NULL;

  gtk_widget_set_sensitive (main_window, TRUE);

  gtk_window_set_focus (GTK_WINDOW (main_window), venta->barcode_entry);

  CalcularVentas (venta->header);

}

/* void */
/* CambiarTipoVenta (GtkToggleButton *button, */
/* 		  gpointer data) */
/* { */
/*   gboolean state = gtk_toggle_button_get_active (button); */

/*   gtk_widget_set_sensitive (venta->entry_rut, state); */
/*   gtk_widget_set_sensitive (venta->find_button, state); */

/*   gtk_widget_set_sensitive (venta->sell_button, FALSE); */

/*   if (state == TRUE) */
/*     { */
/*       gtk_widget_set_sensitive (venta->entry_paga, FALSE); */
/*       //      gtk_widget_set_sensitive (vuelto_button, FALSE); */
/*     } */
/*   else */
/*     { */
/*       gtk_widget_set_sensitive (venta->entry_paga, TRUE); */
/*       //      gtk_widget_set_sensitive (vuelto_button, TRUE); */
/*     } */

/*   venta->tipo_venta = state; */
/* } */

void
CalcularVuelto (void)
{
  gchar *pago = g_strdup (gtk_entry_get_text (GTK_ENTRY (venta->entry_paga)));
  gint paga_con = atoi (gtk_entry_get_text (GTK_ENTRY (venta->entry_paga)));
  gint total = atoi (CutPoints (g_strdup (gtk_label_get_text (GTK_LABEL (venta->total_label)))));
  gint resto;

  if (strcmp (pago, "c") == 0)
    PagoCheque ();
  else if (paga_con > 0)
    {
      if (paga_con < total)
	{
	  //	  gtk_label_set_text (GTK_LABEL (venta->label_vuelto), "No es dinero suficiente");
	  gtk_label_set_markup (GTK_LABEL (venta->label_vuelto),
				"<span size=\"30000\">Monto Insuficiente</span>");

	  gtk_widget_set_sensitive (venta->sell_button, FALSE);

	  return;
	}
      else
	{
	  resto = paga_con - total;

	  gtk_label_set_markup (GTK_LABEL (venta->label_vuelto),
				g_strdup_printf ("<span size=\"30000\">%s</span> "
						 "<span size=\"15000\">Vuelto</span>",
						 PutPoints (g_strdup_printf ("%d", resto))));

	  gtk_widget_set_sensitive (venta->sell_button, TRUE);
	}
    }
  else
    gtk_label_set_markup (GTK_LABEL (venta->label_vuelto),
			  "<span size=\"30000\"> </span>");
}

void
SearchProductByName (void)
{
  //  gchar *producto = g_strdup (gtk_entry_get_text (GTK_ENTRY (venta->product_entry)));

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

      gtk_entry_set_text (GTK_ENTRY (venta->codigo_entry),
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

      total = atoi (CutPoints (g_strdup (gtk_label_get_text (GTK_LABEL (venta->total_label)))));

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
  gint precio;
  gdouble stockday;
  gdouble stock;
  PGresult *res;
  gint venta_directa = atoi(rizoma_get_value("VENTA_DIRECTA"));

  ventas = (gboolean) data;

  if (strcmp (barcode, "") == 0)
    return 0;

  if (HaveCharacters (barcode) == TRUE || strcmp (barcode, "") == 0)
    {
      //  WindowProductSelect ();
      BuscarProducto (NULL, (gpointer)widget);

      return 0;
    }

  if (strlen (barcode) <= 5)
    {
      gtk_entry_set_text (GTK_ENTRY (venta->codigo_entry), barcode);

      SearchProductByCode ();

      return 0;
    }

  gtk_entry_set_text (GTK_ENTRY (widget), barcode);

  gchar *q= NULL;

  if (ventas != FALSE)
    q = g_strdup_printf ("SELECT codigo_corto, descripcion, marca, contenido, unidad, stock, "
			 "precio, precio_mayor, cantidad_mayor, mayorista "
			 "FROM producto "
			 "WHERE barcode='%s' AND stock!=0",
			 barcode);
  else
    q = g_strdup_printf ("SELECT codigo_corto, descripcion, marca, contenido, unidad, stock, "
			 "precio, mayorista FROM producto "
			 "WHERE barcode='%s' AND stock!=0 AND canje='t'",
			 barcode);

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

  if (ventas != FALSE)
    {
      mayorista = strcmp (PQvaluebycol( res, 0, "mayorista"), "t") == 0 ? TRUE : FALSE;

      //caja de producto
      gtk_entry_set_text (GTK_ENTRY (venta->product_entry),
			  g_strdup_printf ("%s  %s  %s  %s",PQvaluebycol (res, 0, "codigo_corto"),
					   PQvaluebycol (res, 0, "marca"), PQvaluebycol (res, 0, "contenido"),
					   PQvaluebycol (res, 0, "unidad")));

      stock = strtod (PUT (PQvaluebycol (res, 0, "stock")), (char **)NULL);

      stockday = GetDayToSell (barcode);

      if (stock <= GetMinStock (barcode))
	gtk_label_set_markup (GTK_LABEL (venta->stockday),
			      g_strdup_printf("<span foreground=\"red\"><b>%.2f dia(s)</b></span>",
					      stockday));
      else
	gtk_label_set_markup (GTK_LABEL (venta->stockday),
			      g_strdup_printf ("<b>%.2f dia(s)</b>", stockday));

      //precio
      gtk_label_set_markup (GTK_LABEL (venta->precio_label),
			    g_strdup_printf ("<span weight=\"ultrabold\">%s</span>",
					     PutPoints (PQvaluebycol (res, 0, "precio"))));

      gtk_label_set_markup (GTK_LABEL (venta->mayor_label),
			    g_strdup_printf ("<span weight=\"ultrabold\">%s</span>",
					     PutPoints (PQvaluebycol (res, 0, "precio_mayor"))));

      gtk_label_set_markup (GTK_LABEL (venta->mayor_cantidad),
			    g_strdup_printf ("<span weight=\"ultrabold\">%s</span>",
					     PutPoints (PQvaluebycol (res, 0, "cantidad_mayor"))));

      gtk_label_set_markup (GTK_LABEL (venta->stock_label),
			    g_strdup_printf ("<span weight=\"ultrabold\">%.2f</span>",
					     stock));

      precio = atoi (PQvaluebycol (res, 0, "precio"));

      gtk_label_set_markup (GTK_LABEL (venta->subtotal_label),
			    g_strdup_printf
			    ("<span weight=\"ultrabold\">%s</span>",
			     g_strdup_printf ("%.0f",
					      strtod (g_strdup (gtk_entry_get_text (GTK_ENTRY (venta->cantidad_entry))), (char **)NULL) *
					      precio)));

      gtk_entry_set_text (GTK_ENTRY (venta->codigo_entry), PQvaluebycol (res, 0, "codigo_corto"));

      if (precio != 0)
	{
	  if( venta_directa == 1 ) {
	    if( VentaFraccion( PQvaluebycol( res, 0, "cantidad_mayor" ) ) ) {
	      gtk_window_set_focus( GTK_WINDOW( main_window ), venta->cantidad_entry );
	      gtk_widget_set_sensitive (add_button, TRUE);
	    } else {
	      AgregarProducto( NULL, NULL );
	    }
	  } else {
	    gtk_window_set_focus( GTK_WINDOW( main_window ), venta->cantidad_entry );
	    gtk_widget_set_sensitive( add_button, TRUE );
	  }
	}
      else
	gtk_window_set_focus (GTK_WINDOW (main_window), venta->barcode_entry);
    }

  return 0;
}

void
CloseBuscarWindow (GtkWidget *widget, gpointer data)
{
  gboolean add = (gboolean) data;
  gint venta_directa = atoi(rizoma_get_value("VENTA_DIRECTA"));

  gtk_widget_destroy (buscador_window);

  if (ventas == TRUE)
    {
      gtk_widget_set_sensitive (main_window, TRUE);
      if (add == TRUE && venta_directa == 0)
	gtk_window_set_focus (GTK_WINDOW (main_window), venta->cantidad_entry);
      else
	gtk_window_set_focus (GTK_WINDOW (main_window), venta->barcode_entry);
    }
  else
    {
      gtk_widget_set_sensitive (gtk_widget_get_toplevel (canje_entry), TRUE);

      gtk_window_set_focus (GTK_WINDOW (gtk_widget_get_toplevel (widget)),
			    canje_cantidad);
    }
  buscador_window = NULL;
}

void
BuscarProducto (GtkWidget *widget, gpointer data)
{
  GtkWidget *vbox;
  GtkWidget *hbox;

  GtkWidget *label;
  GtkWidget *button;

  GtkWidget *entry = (GtkWidget *)data;
  gchar *string = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));

  GtkWidget *treeview;
  GtkWidget *scroll;
  GtkAccelGroup *accel_search;

  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  gtk_widget_set_sensitive (gtk_widget_get_toplevel (entry), FALSE);

  buscador_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_position (GTK_WINDOW (buscador_window), GTK_WIN_POS_CENTER_ALWAYS);
  if (solo_venta != TRUE)
    gtk_widget_set_size_request (buscador_window, 800, 300);
  else
    gtk_widget_set_size_request (buscador_window, 640, 300);
  g_signal_connect (G_OBJECT (buscador_window), "destroy",
		    G_CALLBACK (CloseBuscarWindow), (gpointer)FALSE);

  gtk_window_present (GTK_WINDOW (buscador_window));


  accel_search = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (buscador_window), accel_search);

  vbox = gtk_vbox_new (FALSE, 3);
  gtk_widget_show (vbox);
  gtk_container_add (GTK_CONTAINER (buscador_window), vbox);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  label = gtk_label_new ("Producto a buscar: ");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);

  venta->buscar_entry = gtk_entry_new ();
  gtk_widget_show (venta->buscar_entry);
  gtk_box_pack_start (GTK_BOX (hbox), venta->buscar_entry, FALSE, FALSE, 3);

  if (strcmp (string, "") != 0)
    gtk_entry_set_text (GTK_ENTRY (venta->buscar_entry), string);

  g_signal_connect (G_OBJECT (venta->buscar_entry), "activate",
		    G_CALLBACK (SearchAndFill), NULL);

  gtk_window_set_focus (GTK_WINDOW (buscador_window), venta->buscar_entry);

  button = gtk_button_new_from_stock (GTK_STOCK_FIND);
  gtk_widget_show (button);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 3);

  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (SearchAndFill), NULL);

  button = gtk_button_new_from_stock (GTK_STOCK_ADD);
  gtk_widget_show (button);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);

  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (FillSellData), NULL);

  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_show (scroll);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_AUTOMATIC);
  gtk_widget_set_size_request (scroll, 320, 200);

  gtk_box_pack_start (GTK_BOX (vbox), scroll, FALSE, FALSE, 3);

  venta->search_store = gtk_list_store_new (10,
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

  treeview = gtk_tree_view_new_with_model (GTK_TREE_MODEL (venta->search_store));
  gtk_widget_show (treeview);
  gtk_container_add (GTK_CONTAINER (scroll), treeview);

  venta->search_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));

  g_signal_connect (G_OBJECT (treeview), "row-activated",
		    G_CALLBACK (FillSellData), NULL);
	
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Código", renderer,
						     "text", 0,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_min_width (column, 75);
  gtk_tree_view_column_set_max_width (column, 75);
  gtk_tree_view_column_set_resizable (column, FALSE);

  if (solo_venta != TRUE)
    {
      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes ("Código de Barras", renderer,
							 "text", 1,
							 NULL);
      gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
      gtk_tree_view_column_set_resizable (column, FALSE);
    }

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Descripción", renderer,
						     "text", 2,
						     "foreground", 8,
						     "foreground-set", 9,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
  gtk_tree_view_column_set_min_width (column, 270);
  gtk_tree_view_column_set_max_width (column, 270);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Marca", renderer,
						     "text", 3,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
  gtk_tree_view_column_set_min_width (column, 90);
  gtk_tree_view_column_set_max_width (column, 90);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Cont.", renderer,
						     "text", 4,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_min_width (column, 40);
  gtk_tree_view_column_set_max_width (column, 40);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Uni.", renderer,
						     "text", 5,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_min_width (column, 40);
  gtk_tree_view_column_set_max_width (column, 40);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Stock", renderer,
						     "text", 6,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_min_width (column, 60);
  gtk_tree_view_column_set_max_width (column, 60);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Precio", renderer,
						     "text", 7,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_min_width (column, 70);
  gtk_tree_view_column_set_max_width (column, 70);
  gtk_tree_view_column_set_resizable (column, FALSE);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_end (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  button = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
  gtk_widget_show (button);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 3);

  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (CloseBuscarWindow), (gpointer)FALSE);

  gtk_widget_add_accelerator (button, "clicked", accel_search,
			      GDK_Escape, GDK_LOCK_MASK, GTK_ACCEL_VISIBLE);

  label = gtk_label_new ("");
  gtk_label_set_markup (GTK_LABEL (label),
			"<b>Se han encontrado:</b> ");
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
  gtk_widget_show (label);

  label_found = gtk_label_new ("");
  gtk_box_pack_start (GTK_BOX (hbox), label_found, FALSE, FALSE, 3);
  gtk_widget_show (label_found);

  SearchAndFill ();

}

void
SearchAndFill (void)
{
  PGresult *res;
  gint resultados, i;
  gchar *string = g_strdup (gtk_entry_get_text (GTK_ENTRY (venta->buscar_entry)));

  GtkTreeIter iter;

  if (strcmp (string, "") != 0)
    res = EjecutarSQL
      (g_strdup_printf
       ("SELECT * FROM producto WHERE stock>0 AND lower (descripcion) LIKE lower('%%%s%%') OR "
	"lower(marca) LIKE lower('%%%s%%') %s ORDER BY descripcion ASC", string, string,
	ventas == FALSE ? "WHERE canje='t'" : ""));
  else
    res = EjecutarSQL
      (g_strdup_printf
       ("SELECT * FROM producto WHERE stock>0 %s", ventas == FALSE ? "AND canje='t'": ""));

  resultados = PQntuples (res);

  gtk_label_set_markup (GTK_LABEL (label_found),
			g_strdup_printf ("<b>%d producto(s)</b>", resultados));

  gtk_list_store_clear (venta->search_store);


  if (resultados == 0)
    {
      gtk_list_store_append (venta->search_store, &iter);
      gtk_list_store_set (venta->search_store, &iter,
			  2, "No se encontró descripción",
			  8, "White",
			  9, TRUE,
			  -1);
    }
  else
    {
      for (i = 0; i < resultados; i++)
	{
	  gtk_list_store_append (venta->search_store, &iter);
	  gtk_list_store_set (venta->search_store, &iter,
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
      gtk_window_set_focus (GTK_WINDOW (gtk_widget_get_toplevel (venta->buscar_entry)),
			    GTK_WIDGET (gtk_tree_selection_get_tree_view (venta->search_selection)));
    }

  gtk_tree_selection_select_path (venta->search_selection, gtk_tree_path_new_from_string ("0"));

}

void
FillSellData (GtkTreeView *treeview, GtkTreePath *arg1, GtkTreeViewColumn *arg2, gpointer data)
{
  GtkTreeIter iter;
  gchar *barcode, *product, *codigo;
  gint precio;

  if (gtk_tree_selection_get_selected (venta->search_selection, NULL, &iter) == TRUE)
    {
      gtk_tree_model_get (GTK_TREE_MODEL (venta->search_store), &iter,
			  0, &codigo,
			  1, &barcode,
			  2, &product,
			  7, &precio,
			  -1);

      if (codigo == NULL)
	return;

      if (ventas == TRUE)
	{

	  gtk_entry_set_text (GTK_ENTRY (venta->product_entry), product);
	  gtk_label_set_markup (GTK_LABEL (venta->precio_label),
				g_strdup_printf ("<span weight=\"ultrabold\">%s</span>",
						 PutPoints (g_strdup_printf ("%d", precio))));
	  gtk_label_set_markup (GTK_LABEL (venta->subtotal_label),
				g_strdup_printf ("<span weight=\"ultrabold\">%s</span>",
						 PutPoints (g_strdup_printf ("%u", atoi (gtk_entry_get_text (GTK_ENTRY (venta->cantidad_entry))) *
									     atoi (CutPoints (g_strdup (gtk_label_get_text (GTK_LABEL (venta->precio_label)))))))));

	  gtk_entry_set_text (GTK_ENTRY (venta->codigo_entry), codigo);
	  gtk_entry_set_text (GTK_ENTRY (venta->barcode_entry), barcode);

	  SearchBarcodeProduct (venta->barcode_entry, (gpointer)TRUE);
	}
      else
	{
	  gtk_entry_set_text (GTK_ENTRY (canje_entry), barcode);

	  SearchBarcodeProduct (canje_entry, (gpointer)FALSE);
	}
      CloseBuscarWindow (NULL, (gpointer)TRUE);
    }
}

void
Descuento (GtkWidget *widget, gpointer data)
{
  gboolean money_discount = (gboolean) data;
  gint money;
  gdouble discount = strtod (CUT(g_strdup (gtk_entry_get_text (GTK_ENTRY (venta->discount_entry)))), (char **)NULL);
  gint total;
  gint plata;
  gdouble porcentaje;

  if (tipo_documento != FACTURA && tipo_documento != GUIA)
    money = atoi (g_strdup (gtk_entry_get_text (GTK_ENTRY (venta->sencillo))));

  CalcularVentas (venta->header);

  total = atoi (CutPoints (g_strdup (gtk_label_get_text (GTK_LABEL (venta->total_label)))));

  if (money_discount == TRUE)
    {
      porcentaje = (gdouble)(100 * money) / total;

      gtk_entry_set_text (GTK_ENTRY (venta->discount_entry),
			  g_strdup_printf ("%.2f", porcentaje));

      gtk_label_set_markup (GTK_LABEL (venta->total_label),
			    g_strdup_printf ("<span size=\"40000\">%s</span>",
					     PutPoints (g_strdup_printf ("%u", total - money))));
    }
  else if (money_discount == FALSE)
    {
      plata = lround ((gdouble)(total * discount) / 100);

      if (tipo_documento != FACTURA)
	gtk_entry_set_text (GTK_ENTRY (venta->sencillo),
			    g_strdup_printf ("%d", plata));

      gtk_label_set_markup (GTK_LABEL (venta->total_label),
			    g_strdup_printf ("<span size=\"40000\">%s</span>",
					     PutPoints (g_strdup_printf ("%u", total - plata))));
    }
}

gboolean
CalcularVentas (Productos *header)
{
  guint32 total = llround (CalcularTotal (header));

  gtk_label_set_markup (GTK_LABEL (venta->total_label),
			g_strdup_printf ("<span size=\"40000\">%s</span>",
					 PutPoints (g_strdup_printf ("%lu", total))));
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
	gtk_window_set_focus (GTK_WINDOW (venta->window), venta->discount_entry);
    }
}


void
CloseCancelWindow (void)
{
  gtk_widget_destroy (venta->cancel_window);
  venta->cancel_window = NULL;

  gtk_widget_set_sensitive (main_window, TRUE);

  gtk_window_set_focus (GTK_WINDOW (main_window), venta->barcode_entry);

}

void
FindCancelSell (GtkWidget *widget, gpointer data)
{
  gchar *text = NULL;
  PGresult *res;
  gint tuples, i;
  GtkTreeIter iter;

  text = gtk_entry_get_text (GTK_ENTRY (data));
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
  //		    G_CALLBACK (AskCancelSell), NULL);
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
  gtk_widget_destroy (window_seller);

  gtk_widget_set_sensitive (main_window, TRUE);

  gtk_window_set_focus (GTK_WINDOW (main_window), venta->barcode_entry);

}

void
ChangeSeller (GtkWidget *widget, gpointer data)
{
  GtkEntry *entry = (GtkEntry *) data;
  gint id = atoi (gtk_entry_get_text (entry));
  gchar *user_name;

  user_name = ReturnUsername (id);

  if (user_name != NULL)
    {
      user_data->user_id = id;
      user_data->user = ReturnUsername (id);
      user_data->level = ReturnUserLevel (user_name);

      gtk_label_set_markup (GTK_LABEL (venta->vendedor),
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
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *entry;
  GtkWidget *button;

  gtk_widget_set_sensitive (main_window, FALSE);

  window_seller = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window_seller), "Ingresar ID Vendedor");
  gtk_window_set_position (GTK_WINDOW (window_seller), GTK_WIN_POS_CENTER_ALWAYS);
  gtk_window_set_resizable (GTK_WINDOW (window_seller), FALSE);
  gtk_widget_show (window_seller);
  gtk_window_present (GTK_WINDOW (window_seller));

  g_signal_connect (G_OBJECT (window_seller), "destroy",
		    G_CALLBACK (CloseWindowChangeSeller), NULL);

  vbox = gtk_vbox_new (3, FALSE);
  gtk_container_add (GTK_CONTAINER (window_seller), vbox);
  gtk_widget_show (vbox);

  hbox = gtk_hbox_new (3, FALSE);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  label = gtk_label_new ("Ingrese el ID de vendedor: ");
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
  gtk_widget_show (label);

  entry = gtk_entry_new_with_max_length (3);
  gtk_box_pack_end (GTK_BOX (hbox), entry, FALSE, FALSE, 3);
  gtk_widget_show (entry);

  gtk_window_set_focus (GTK_WINDOW (window_seller), entry);

  g_signal_connect (G_OBJECT (entry), "changed",
		    G_CALLBACK (ChangeSeller), (gpointer) entry);

  g_signal_connect (G_OBJECT (entry), "activate",
		    G_CALLBACK (ChangeSeller), (gpointer) entry);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_end (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);


  button = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (CloseWindowChangeSeller), NULL);

  button = gtk_button_new_from_stock (GTK_STOCK_OK);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (ChangeSeller), (gpointer) entry);

}
