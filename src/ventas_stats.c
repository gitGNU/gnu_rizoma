/*ventas_stats.c
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
*    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include<gtk/gtk.h>

#include<stdlib.h>
#include<string.h>

#include"tipos.h"
#include"main.h"
#include"ventas_stats.h"
#include"postgres-functions.h"
#include"caja.h"
#include"credito.h"
#include"printing.h"

#include"dimentions.h"

GtkButton *button_from;
GtkButton *button_to;

GtkLabel *label_from;
GtkLabel *label_to;

GtkNotebook *tab;

GtkWidget *total_vendidos;
GtkWidget *total_costo;
GtkWidget *total_contrib;
GtkWidget *total_margen;

GtkWidget *n_ventas;
GtkWidget *venta_promedio;

GtkWidget *credito_n_ventas;
GtkWidget *credito_venta_promedio;


GtkWidget *total_n_ventas;
GtkWidget *total_venta_promedio;

GtkTreeStore *proveedores_store;
GtkWidget *proveedores_tree;

GtkWidget *total_productos;
GtkWidget *total_comprado;
GtkWidget *total_vendido;

void
FillProductsProveedor (GtkTreeSelection *selection, gpointer data)
{
  PGresult *res;
  GtkTreeView *tree = gtk_tree_selection_get_tree_view (selection);
  GtkTreeModel *model = gtk_tree_view_get_model (tree);
  GtkTreeIter iter;
  gchar *rut;
  gint tuples, i;
  gdouble total1 = 0, total2 = 0;

  if ((gtk_tree_selection_get_selected (selection, &model, &iter)) == FALSE)
    return;

  gtk_tree_model_get (model, &iter,
		      1, &rut,
		      -1);

  res = EjecutarSQL
    (g_strdup_printf
     ("SELECT descripcion, marca, contenido, unidad, (SELECT SUM(cantidad_ingresada) FROM products_buy_history WHERE barcode_product=barcode),vendidos FROM productos WHERE barcode IN (SELECT barcode_product FROM products_buy_history WHERE id_compra IN (SELECT id FROM compras WHERE rut_proveedor='%s') AND anulado='f' GROUP BY barcode_product)", rut));

  if (res == NULL)
    return;

  tuples = PQntuples (res);

  gtk_label_set_text (GTK_LABEL (total_productos), g_strdup_printf ("%d", tuples));

  gtk_tree_store_clear (proveedores_store);

  for (i = 0; i < tuples; i++)
    {
      total1 += strtod (PQgetvalue (res, i, 4), (char **)NULL);
      total2 += strtod (PQgetvalue (res, i, 5), (char **)NULL);

      gtk_tree_store_append (proveedores_store, &iter, NULL);
      gtk_tree_store_set (proveedores_store, &iter,
			  0, PQgetvalue (res, i, 0),
			  1, PQgetvalue (res, i, 1),
			  2, PQgetvalue (res, i, 2),
			  3, PQgetvalue (res, i, 3),
			  4, strtod (PQgetvalue (res, i, 4), (char **)NULL),
			  5, strtod (PQgetvalue (res, i, 5), (char **)NULL),
			  -1);

    }

  gtk_label_set_text (GTK_LABEL (total_comprado), g_strdup_printf ("%.2f", total1));
  gtk_label_set_text (GTK_LABEL (total_vendido), g_strdup_printf ("%.2f", total2));

  res = EjecutarSQL
    (g_strdup_printf ("SELECT nombre, direccion, telefono, web, giro, comuna, email FROM proveedores WHERE rut='%s'", rut));

  if (res == NULL)
    return;

  gtk_label_set_markup (GTK_LABEL (ventastats->proveedor_razon),
			g_strdup_printf ("<b>%s</b>", PQgetvalue (res, 0, 0)));
  gtk_label_set_markup (GTK_LABEL (ventastats->proveedor_direccion),
			g_strdup_printf ("<b>%s</b>", PQgetvalue (res, 0, 1)));
  gtk_label_set_markup (GTK_LABEL (ventastats->proveedor_fono),
			g_strdup_printf ("<b>%s</b>", PQgetvalue (res, 0, 2)));
  gtk_label_set_markup (GTK_LABEL (ventastats->proveedor_web),
			g_strdup_printf ("<b>%s</b>", PQgetvalue (res, 0, 3)));
  gtk_label_set_markup (GTK_LABEL (ventastats->proveedor_giro),
			g_strdup_printf ("<b>%s</b>", PQgetvalue (res, 0, 4)));
  gtk_label_set_markup (GTK_LABEL (ventastats->proveedor_comuna),
			g_strdup_printf ("<b>%s</b>", PQgetvalue (res, 0, 5)));
  gtk_label_set_markup (GTK_LABEL (ventastats->proveedor_email),
			g_strdup_printf ("<b>%s</b>", PQgetvalue (res, 0, 6)));

}

void
SetToggleMode (GtkToggleButton *widget, gpointer data)
{

  if (gtk_toggle_button_get_active (widget) == TRUE)
    gtk_toggle_button_set_active (widget, FALSE);
  else
    gtk_toggle_button_set_active (widget, TRUE);
}

void
DisplayCal (GtkToggleButton *widget, gpointer data)
{
  GtkWidget *window;
  GtkWidget *vbox;
  GtkCalendar *calendar;
  GtkRequisition req;
  gint h, w;
  gint x, y;
  gint button_y, button_x;
  gboolean toggle = gtk_toggle_button_get_active (widget);
  gboolean from = (gboolean) data;

  if (toggle == TRUE)
    {
      gdk_window_get_origin (GTK_WIDGET (widget)->window, &x, &y);

      gtk_widget_size_request (GTK_WIDGET (widget), &req);
      h = req.height;
      w = req.width;

      button_y = GTK_WIDGET (widget)->allocation.y;
      button_x = GTK_WIDGET (widget)->allocation.x;

      window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
      gtk_window_set_screen (GTK_WINDOW (window), gtk_widget_get_screen (GTK_WIDGET (widget)));

      gtk_container_set_border_width (GTK_CONTAINER (window), 5);
      gtk_window_set_type_hint (GTK_WINDOW (window), GDK_WINDOW_TYPE_HINT_DOCK);
      gtk_window_set_decorated (GTK_WINDOW (window), FALSE);
      gtk_window_set_resizable (GTK_WINDOW (window), FALSE);
      gtk_window_stick (GTK_WINDOW (window));
      gtk_window_set_title (GTK_WINDOW (window), "Calendario");

      vbox = gtk_vbox_new (FALSE, 3);
      gtk_container_add (GTK_CONTAINER (window), vbox);
      gtk_widget_show (vbox);

      calendar = GTK_CALENDAR (gtk_calendar_new ());
      gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (calendar), FALSE, FALSE, 0);
      gtk_widget_show (GTK_WIDGET (calendar));

      if (from == TRUE)
	{
	  if (strchr (g_strdup (gtk_button_get_label (button_from)), '/') != NULL)
	    {
	      gtk_calendar_select_day (calendar, ventastats->selected_from_day);
	      gtk_calendar_select_month (calendar, ventastats->selected_from_month - 1,
					 ventastats->selected_from_year);
	    }
	}
      else
	{
	  if (strchr (g_strdup (gtk_button_get_label (button_to)), '/') != NULL)
	    {
	      gtk_calendar_select_day (calendar, ventastats->selected_to_day);
	      gtk_calendar_select_month (calendar, ventastats->selected_to_month - 1,
					 ventastats->selected_to_year);
	    }
	}

      g_signal_connect (G_OBJECT (calendar), "day-selected-double-click",
			G_CALLBACK (SetDate), (gpointer) widget);

      gtk_widget_show (window);

      x = (x + button_x);
      y = (y + button_y) + h;

      gtk_window_move (GTK_WINDOW (window), x, y);
      gtk_window_present (GTK_WINDOW (window));

      if (from == TRUE)
	calendar_from = window;
      else
	calendar_to = window;

    }
  else if (toggle == FALSE)
    {
      if (from == TRUE)
	{
	  gtk_widget_destroy (calendar_from);
	  calendar_from = NULL;
	}
      else
	{
	  gtk_widget_destroy (calendar_to);
	  calendar_to = NULL;
	}

    }

  //  SetToggleMode (widget, NULL);
}

void
SetDate (GtkCalendar *calendar, gpointer data)
{
  GtkButton *button = (GtkButton *) data;
  guint day, month, year;
  gint page;
  gchar *label;

  label = gtk_button_get_label (button);

  if (calendar == NULL)
    {
      calendar = GTK_CALENDAR (gtk_calendar_new ());

      gtk_calendar_get_date (calendar, &year, &month, &day);

      gtk_button_set_label (button, g_strdup_printf ("%.2u/%.2u/%.4u", day, month+1, year));

    }
  else
    {
      gtk_calendar_get_date (calendar, &year, &month, &day);

      gtk_button_set_label (button, g_strdup_printf ("%.2u/%.2u/%.4u", day, month+1, year));
      GetSelectedDate ();

      SetToggleMode (GTK_TOGGLE_BUTTON (data), NULL);

      page = gtk_notebook_get_current_page (tab);

      g_signal_emit_by_name (G_OBJECT (tab), "switch-page", gtk_notebook_get_nth_page (tab, page),
			     page, 1);
    }
}

void
control_decimal (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model,
		 GtkTreeIter *iter, gpointer user_data)
{
  gint column = (gint) user_data;
  gchar  buf[20];
  GType column_type = gtk_tree_model_get_column_type (model, column);

  switch (column_type)
    {
    case G_TYPE_DOUBLE:
      {
	gdouble number;
	gtk_tree_model_get(model, iter, column, &number, -1);
	g_snprintf (buf, sizeof (buf), "%.3f", number);
	g_object_set(renderer, "text", buf, NULL);
      }
      break;
    case G_TYPE_INT:
      {
	gint number;
	gtk_tree_model_get(model, iter, column, &number, -1);
	g_snprintf (buf, sizeof (buf), "%d", number);
	g_object_set(renderer, "text", PutPoints (buf), NULL);
      }
      break;
    default:
      break;
    }

}

void
ventas_stats_box (MainBox *module_box)
{

  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  GtkWidget *label;
  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *hbox2;
  GtkWidget *vbox2;

  GtkWidget *table;

  GtkWidget *button;

  GtkWidget *scroll;

  GtkWidget *frame;

  Print *print = (Print *) malloc (sizeof (Print));

  Print *libro = (Print *) malloc (sizeof (Print));
  libro->son = (Print *) malloc (sizeof (Print));

  Print *proveedores_print = (Print *) malloc (sizeof (Print));
  proveedores_print->son = (Print *) malloc (sizeof (Print));

  if (module_box->new_box != NULL)
    gtk_widget_destroy (GTK_WIDGET (module_box->new_box));

  module_box->new_box = gtk_vbox_new (FALSE, 3);
  gtk_widget_show (module_box->new_box);
  gtk_box_pack_start (GTK_BOX (module_box->main_box), module_box->new_box, FALSE, FALSE, 3);

  /*
   * Debemos asegurar que las fechas estan en 0
   */
  ventastats->selected_from_day = 0;
  ventastats->selected_from_month = 0;
  ventastats->selected_from_year = 0;
  ventastats->selected_to_day = 0;
  ventastats->selected_to_month = 0;
  ventastats->selected_to_year = 0;


  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (module_box->new_box), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  button_from = GTK_BUTTON (gtk_toggle_button_new_with_label ("Inicio"));
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (button_from), FALSE);
  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (button_from), FALSE, FALSE, 3);
  gtk_widget_show (GTK_WIDGET (button_from));

  gtk_window_set_focus (GTK_WINDOW (main_window), GTK_WIDGET (button_from));

  //  SetDate (NULL, (gpointer)button_from);

  g_signal_connect (G_OBJECT (button_from), "toggled",
		    G_CALLBACK (DisplayCal), (gpointer) TRUE);

  button_to = GTK_BUTTON (gtk_toggle_button_new_with_label ("Fin"));
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (button_to), FALSE);
  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (button_to), FALSE, FALSE, 3);
  gtk_widget_show (GTK_WIDGET (button_to));

  //  SetDate (NULL, (gpointer)button_to);

  g_signal_connect (G_OBJECT (button_to), "toggled",
		    G_CALLBACK (DisplayCal), (gpointer) FALSE);

  tab = GTK_NOTEBOOK (gtk_notebook_new ());
  gtk_widget_set_size_request (GTK_WIDGET (tab), MODULE_BOX_WIDTH-10, MODULE_BOX_HEIGHT-30);
  gtk_box_pack_start (GTK_BOX (module_box->new_box), GTK_WIDGET (tab), FALSE, FALSE, 3);
  gtk_widget_show (GTK_WIDGET (tab));


  /*
    Ventas
  */
  vbox = gtk_vbox_new (FALSE, 3);
  gtk_widget_show (vbox);

  gtk_notebook_append_page (GTK_NOTEBOOK (tab), vbox,
			    gtk_label_new_with_mnemonic ("_Ventas"));

  g_signal_connect (G_OBJECT (tab), "switch-page",
		    G_CALLBACK (RefreshCurrentTab), NULL);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_show (scroll);
  if (solo_venta != TRUE)
    gtk_widget_set_size_request (scroll, 450, 200);
  else
    gtk_widget_set_size_request (scroll, 440, 150);

  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (hbox), scroll, FALSE, FALSE, 3);

  ventastats->store_ventas_stats = gtk_tree_store_new (6,
						       G_TYPE_STRING,
						       G_TYPE_STRING,
						       G_TYPE_STRING,
						       G_TYPE_STRING,
						       G_TYPE_STRING,
						       G_TYPE_STRING);

  ventastats->tree_ventas_stats = gtk_tree_view_new_with_model (GTK_TREE_MODEL
								(ventastats->store_ventas_stats));
  gtk_widget_show (ventastats->tree_ventas_stats);
  gtk_widget_set_size_request (ventastats->tree_ventas_stats, 250, 200);
  gtk_container_add (GTK_CONTAINER (scroll), ventastats->tree_ventas_stats);

  ventastats->selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (ventastats->tree_ventas_stats));

  g_signal_connect (G_OBJECT (ventastats->selection), "changed",
		    G_CALLBACK (ChangeVenta), NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Fecha", renderer,
						     "text", 0,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (ventastats->tree_ventas_stats), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("ID Venta", renderer,
						     "text", 1,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (ventastats->tree_ventas_stats), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Maq.", renderer,
						     "text", 2,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (ventastats->tree_ventas_stats), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Vendedor", renderer,
						     "text", 3,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (ventastats->tree_ventas_stats), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Monto", renderer,
						     "text", 4,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (ventastats->tree_ventas_stats), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_alignment (column, 0.5);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Tipo Pago", renderer,
						     "text", 5,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (ventastats->tree_ventas_stats), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);
  vbox2 = gtk_vbox_new (FALSE, 3);
  gtk_widget_show (vbox2);
  gtk_box_pack_start (GTK_BOX (hbox), vbox2, FALSE, FALSE, 3);

  /*
    Right box
  */

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 3);

  label = gtk_label_new ("");
  gtk_widget_show (label);
  gtk_label_set_markup (GTK_LABEL (label),
			"<span size=\"xx-large\">Desde: </span>");
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);

  label_from = GTK_LABEL (gtk_label_new (""));
  gtk_widget_show (GTK_WIDGET (label_from));
  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (label_from), FALSE, FALSE, 3);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 3);

  label = gtk_label_new ("");
  gtk_widget_show (label);
  gtk_label_set_markup (GTK_LABEL (label),
			"<span size=\"xx-large\">Hasta: </span>");
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);

  label_to = GTK_LABEL (gtk_label_new (""));
  gtk_widget_show (GTK_WIDGET (label_to));
  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (label_to), FALSE, FALSE, 3);

  /*
    End
  */

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_show (scroll);
  if (solo_venta != TRUE)
    gtk_widget_set_size_request (scroll, 450, 200);
  else
    gtk_widget_set_size_request (scroll, 440, 150);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (hbox), scroll, FALSE, FALSE, 3);

  ventastats->store_venta = gtk_tree_store_new (4,
						G_TYPE_STRING,
						G_TYPE_STRING,
						G_TYPE_STRING,
						G_TYPE_STRING);

  ventastats->tree_venta = gtk_tree_view_new_with_model (GTK_TREE_MODEL (ventastats->store_venta));
  gtk_widget_set_size_request (GTK_WIDGET (ventastats->tree_venta), -1, 200);
  gtk_widget_show (ventastats->tree_venta);
  gtk_container_add (GTK_CONTAINER (scroll), ventastats->tree_venta);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Producto", renderer,
						     "text", 0,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (ventastats->tree_venta), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_min_width (column, 260);
  gtk_tree_view_column_set_max_width (column, 260);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Cantidad", renderer,
						     "text", 1,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (ventastats->tree_venta), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Unitario", renderer,
						     "text", 2,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (ventastats->tree_venta), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Total", renderer,
						     "text", 3,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (ventastats->tree_venta), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  //  GetSelectectedDate ();

  /*
    Caja Costado Detalle
  */
  /*
    vbox2 = gtk_vbox_new (FALSE, 0);
    gtk_widget_show (vbox2);
    gtk_box_pack_start (GTK_BOX (hbox), vbox2, FALSE, FALSE, 0);
  */

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 3);

  button = gtk_button_new_from_stock (GTK_STOCK_PRINT);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 3);
  gtk_widget_show (button);

  libro->tree = GTK_TREE_VIEW (ventastats->tree_ventas_stats);
  libro->title = "Libro de Ventas";
  libro->name = "ventas";
  libro->date_string = NULL;
  libro->cols[0].name = "Fecha";
  libro->cols[0].num = 0;
  libro->cols[1].name = "Monto";
  libro->cols[1].num = 4;
  libro->cols[2].name = NULL;

  libro->son->tree = GTK_TREE_VIEW (ventastats->tree_venta);
  libro->son->cols[0].name = "Producto";
  libro->son->cols[0].num = 0;
  libro->son->cols[1].name = "Cantidad";
  libro->son->cols[1].num = 1;
  libro->son->cols[2].name = "Unitario";
  libro->son->cols[2].num = 2;
  libro->son->cols[3].name = "Total";
  libro->son->cols[3].num = 3;
  libro->son->cols[4].name = NULL;

  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (PrintTwoTree), (gpointer)libro);

  /*
    Estadisticas inferiores
  */

  hbox2 = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox2, FALSE, FALSE, 3);

  table = gtk_table_new (3, 6, FALSE);
  gtk_box_pack_start (GTK_BOX (hbox2), table, FALSE, FALSE, 3);
  gtk_widget_show (table);

  label = gtk_label_new ("");
  gtk_label_set_markup (GTK_LABEL (label),
			"<span size=\"xx-large\">Ventas Contado: </span>");
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
  gtk_table_attach_defaults (GTK_TABLE (table), label,
			     0, 1,
			     0, 1);
  gtk_widget_show (label);

  ventastats->total_cash_sell_label = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (ventastats->total_cash_sell_label), 1, 0);
  gtk_widget_show (ventastats->total_cash_sell_label);
  gtk_table_attach_defaults (GTK_TABLE (table), ventastats->total_cash_sell_label,
			     1, 2,
			     0, 1);

  gtk_table_set_col_spacing (GTK_TABLE (table), 2, 20);

  label = gtk_label_new ("");
  gtk_label_set_markup (GTK_LABEL (label),
			"<span size=\"xx-large\">N潞 Vtas: </span>");
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
  gtk_table_attach_defaults (GTK_TABLE (table), label,
			     3, 4,
			     0, 1);
  gtk_widget_show (label);

  n_ventas = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (n_ventas), 0.5, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), n_ventas,
			     4, 5,
			     0, 1);
  gtk_widget_show (n_ventas);

  gtk_table_set_col_spacing (GTK_TABLE (table), 4, 20);

  label = gtk_label_new ("");
  gtk_label_set_markup (GTK_LABEL (label),
			"<span size=\"xx-large\">Vta. Promedio: </span>");
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
  gtk_table_attach_defaults (GTK_TABLE (table), label,
			     5, 6,
			     0, 1);
  gtk_widget_show (label);

  venta_promedio = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (venta_promedio), 1, 0);
  gtk_table_attach_defaults (GTK_TABLE (table), venta_promedio,
			     6, 7,
			     0, 1);
  gtk_widget_show (venta_promedio);


  label = gtk_label_new ("");
  gtk_label_set_markup (GTK_LABEL (label),
			"<span size=\"xx-large\">Ventas a Credito: </span>");
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
  gtk_table_attach_defaults (GTK_TABLE (table), label,
			     0, 1,
			     1, 2);
  gtk_widget_show (label);

  ventastats->total_credit_sell_label = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (ventastats->total_credit_sell_label), 1, 0);
  gtk_table_attach_defaults (GTK_TABLE (table), ventastats->total_credit_sell_label,
			     1, 2,
			     1, 2);
  gtk_widget_show (ventastats->total_credit_sell_label);

  label = gtk_label_new ("");
  gtk_label_set_markup (GTK_LABEL (label),
			"<span size=\"xx-large\">N潞 Vtas: </span>");
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
  gtk_table_attach_defaults (GTK_TABLE (table), label,
			     3, 4,
			     1, 2);
  gtk_widget_show (label);

  credito_n_ventas = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (credito_n_ventas), 0.5, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), credito_n_ventas,
			     4, 5,
			     1, 2);
  gtk_widget_show (credito_n_ventas);


  label = gtk_label_new ("");
  gtk_label_set_markup (GTK_LABEL (label),
			"<span size=\"xx-large\">Vta. Promedio: </span>");
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
  gtk_table_attach_defaults (GTK_TABLE (table), label,
			     5, 6,
			     1, 2);
  gtk_widget_show (label);

  credito_venta_promedio = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (credito_venta_promedio), 1, 0);
  gtk_table_attach_defaults (GTK_TABLE (table), credito_venta_promedio,
			     6, 7,
			     1, 2);
  gtk_widget_show (credito_venta_promedio);

  label = gtk_label_new ("");
  gtk_label_set_markup (GTK_LABEL (label),
			"<span size=\"xx-large\">Total Vtas. Periodo: </span>");
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
  gtk_table_attach_defaults (GTK_TABLE (table), label,
			     0, 1,
			     2, 3);
  gtk_widget_show (label);

  ventastats->total_sell_label = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (ventastats->total_sell_label), 1, 0);
  gtk_table_attach_defaults (GTK_TABLE (table), ventastats->total_sell_label,
			     1, 2,
			     2, 3);
  gtk_widget_show (ventastats->total_sell_label);

  label = gtk_label_new ("");
  gtk_label_set_markup (GTK_LABEL (label),
			"<span size=\"xx-large\">N潞 Vtas: </span>");
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
  gtk_table_attach_defaults (GTK_TABLE (table), label,
			     3, 4,
			     2, 3);
  gtk_widget_show (label);

  total_n_ventas = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (total_n_ventas), 0.5, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), total_n_ventas,
			     4, 5,
			     2, 3);
  gtk_widget_show (total_n_ventas);

  label = gtk_label_new ("");
  gtk_label_set_markup (GTK_LABEL (label),
			"<span size=\"xx-large\">Vta. Promedio: </span>");
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
  gtk_table_attach_defaults (GTK_TABLE (table), label,
			     5, 6,
			     2, 3);
  gtk_widget_show (label);


  total_venta_promedio = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (total_venta_promedio), 1, 0);
  gtk_table_attach_defaults (GTK_TABLE (table), total_venta_promedio,
			     6, 7,
			     2, 3);
  gtk_widget_show (total_venta_promedio);

  FillTreeView ();


  /*
    End Ventas
   */

  /*
    Start Caja
   */
  /*
  vbox = gtk_vbox_new (FALSE, 3);
  gtk_widget_show (vbox);

  gtk_notebook_append_page (GTK_NOTEBOOK (tab), vbox,
			    gtk_label_new ("Caja"));

  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_show (scroll);
  gtk_widget_set_size_request (scroll, 350, 200);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (vbox), scroll, FALSE, FALSE, 3);


  ventastats->store_caja = gtk_list_store_new (5,
					  G_TYPE_STRING,
					  G_TYPE_STRING,
					  G_TYPE_STRING,
					  G_TYPE_STRING,
					  G_TYPE_STRING);

  //temp  FillCajaStats (store);

  ventastats->tree_caja = gtk_tree_view_new_with_model (GTK_TREE_MODEL (ventastats->store_caja));
  gtk_widget_show (ventastats->tree_caja);
  gtk_container_add (GTK_CONTAINER (scroll), ventastats->tree_caja);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Fecha De Inicio", renderer,
						     "text", 0,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (ventastats->tree_caja), column);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Inicia Con:", renderer,
						     "text", 1,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (ventastats->tree_caja), column);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Fecha De Termino", renderer,
						     "text", 2,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (ventastats->tree_caja), column);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Termina Con:", renderer,
						     "text", 3,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (ventastats->tree_caja), column);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Perdida", renderer,
						     "text", 4,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (ventastats->tree_caja), column);
  gtk_tree_view_column_set_resizable (column, FALSE);

  */
  /*
    End Caja
  */

  /*
    Start Ranking Ventas
   */

  vbox = gtk_vbox_new (FALSE, 3);
  gtk_widget_show (vbox);

  gtk_notebook_append_page (GTK_NOTEBOOK (tab), vbox,
			    gtk_label_new_with_mnemonic ("_Ranking Ventas"));

  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_show (scroll);
  gtk_widget_set_size_request (scroll, 350, 430);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (vbox), scroll, FALSE, FALSE, 3);

  ventastats->rank_store = gtk_list_store_new (9,
					       G_TYPE_STRING,
					       G_TYPE_STRING,
					       G_TYPE_INT,
					       G_TYPE_STRING,
					       G_TYPE_DOUBLE,
					       G_TYPE_INT,
					       G_TYPE_INT,
					       G_TYPE_INT,
					       G_TYPE_DOUBLE);

  ventastats->rank_tree = gtk_tree_view_new_with_model (GTK_TREE_MODEL (ventastats->rank_store));
  gtk_widget_show (ventastats->rank_tree);
  gtk_container_add (GTK_CONTAINER (scroll), ventastats->rank_tree);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Producto", renderer,
						     "text", 0,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (ventastats->rank_tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 0);
  gtk_tree_view_column_set_min_width (column, 160);
  gtk_tree_view_column_set_max_width (column, 160);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Marca", renderer,
						     "text", 1,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (ventastats->rank_tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 1);
  gtk_tree_view_column_set_min_width (column, 90);
  gtk_tree_view_column_set_max_width (column, 90);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Medida", renderer,
						     "text", 2,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (ventastats->rank_tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 2);
  gtk_tree_view_column_set_min_width (column, 60);
  gtk_tree_view_column_set_max_width (column, 60);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Unid.", renderer,
						     "text", 3,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (ventastats->rank_tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 3);
  gtk_tree_view_column_set_min_width (column, 40);
  gtk_tree_view_column_set_max_width (column, 40);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Unidades", renderer,
						     "text", 4,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (ventastats->rank_tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 4);
  gtk_tree_view_column_set_min_width (column, 50);
  gtk_tree_view_column_set_max_width (column, 50);
  gtk_tree_view_column_set_resizable (column, FALSE);

  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)4, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Vendido $", renderer,
						     "text", 5,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (ventastats->rank_tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 5);
  gtk_tree_view_column_set_min_width (column, 70);
  gtk_tree_view_column_set_max_width (column, 70);
  gtk_tree_view_column_set_resizable (column, FALSE);

  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)5, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Costo", renderer,
						     "text", 6,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (ventastats->rank_tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 6);
  gtk_tree_view_column_set_min_width (column, 50);
  gtk_tree_view_column_set_max_width (column, 50);
  gtk_tree_view_column_set_resizable (column, FALSE);

  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)6, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Contrib.", renderer,
						     "text", 7,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (ventastats->rank_tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 7);
  gtk_tree_view_column_set_min_width (column, 60);
  gtk_tree_view_column_set_max_width (column, 60);
  gtk_tree_view_column_set_resizable (column, FALSE);

  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)7, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Margen", renderer,
						     "text", 8,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (ventastats->rank_tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 8);
  gtk_tree_view_column_set_min_width (column, 50);
  gtk_tree_view_column_set_max_width (column, 50);
  gtk_tree_view_column_set_resizable (column, FALSE);

  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)8, NULL);

  hbox2 = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (vbox), hbox2, FALSE, FALSE, 3);
  gtk_widget_show (hbox2);

  table = gtk_table_new (2, 5, FALSE);
  gtk_box_pack_end (GTK_BOX (hbox2), table, FALSE, FALSE, 3);
  gtk_widget_show (table);

  gtk_table_set_col_spacings (GTK_TABLE (table), 5);

  label = gtk_label_new ("");
  gtk_label_set_markup (GTK_LABEL (label),
			g_strdup_printf ("<span size=\"xx-large\">Total:\t\t</span>"));
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
  gtk_table_attach_defaults (GTK_TABLE (table), label,
			     0, 1,
			     0, 1);
  gtk_widget_show (label);

  label = gtk_label_new ("");
  gtk_label_set_markup (GTK_LABEL (label),
			g_strdup_printf ("<span size=\"x-large\"><b>Vendido</b></span>"));
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
  gtk_table_attach_defaults (GTK_TABLE (table), label,
			     1, 2,
			     0, 1);
  gtk_widget_show (label);

  total_vendidos = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (total_vendidos), 0, 0);
  gtk_table_attach_defaults (GTK_TABLE (table), total_vendidos,
			     1, 2,
			     1, 2);
  gtk_widget_show (total_vendidos);

  label = gtk_label_new ("");
  gtk_label_set_markup (GTK_LABEL (label),
			g_strdup_printf ("<span size=\"x-large\"><b>Costo</b></span>"));
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
  gtk_table_attach_defaults (GTK_TABLE (table), label,
			     2, 3,
			     0, 1);
  gtk_widget_show (label);

  total_costo = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (total_costo), 0, 0);
  gtk_table_attach_defaults (GTK_TABLE (table), total_costo,
			     2, 3,
			     1, 2);
  gtk_widget_show (total_costo);

  label = gtk_label_new ("");
  gtk_label_set_markup (GTK_LABEL (label),
			g_strdup_printf ("<span size=\"x-large\"><b>Contrib</b></span>"));
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
  gtk_table_attach_defaults (GTK_TABLE (table), label,
			     3, 4,
			     0, 1);
  gtk_widget_show (label);

  total_contrib = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (total_contrib), 0, 0);
  gtk_table_attach_defaults (GTK_TABLE (table), total_contrib,
			     3, 4,
			     1, 2);
  gtk_widget_show (total_contrib);

  label = gtk_label_new ("");
  gtk_label_set_markup (GTK_LABEL (label),
			g_strdup_printf ("<span size=\"x-large\"><b>Margen</b></span>"));
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
  gtk_table_attach_defaults (GTK_TABLE (table), label,
			     4, 5,
			     0, 1);
  gtk_widget_show (label);


  total_margen = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (total_margen), 0, 0);
  gtk_table_attach_defaults (GTK_TABLE (table), total_margen,
			     4, 5,
			     1, 2);
  gtk_widget_show (total_margen);

  /*  hbox2 = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (vbox), hbox2, FALSE, FALSE, 3);
  gtk_widget_show (hbox2);
  */
  button = gtk_button_new_from_stock (GTK_STOCK_PRINT);
  gtk_box_pack_start (GTK_BOX (hbox2), button, FALSE, FALSE, 3);
  gtk_widget_show (button);

  print->tree = GTK_TREE_VIEW (ventastats->rank_tree);
  print->title = "Ranking de Ventas";
  print->date_string = NULL;
  print->cols[0].name = "Producto";
  print->cols[1].name = "Marca";
  print->cols[2].name = "Medida";
  print->cols[3].name = "Unidad";
  print->cols[4].name = "Unidades";
  print->cols[5].name = "Vendido $";
  print->cols[6].name = "Costo";
  print->cols[7].name = "Contrib";
  print->cols[8].name = "Margen";
  print->cols[9].name = NULL;

  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (PrintTree), (gpointer)print);


  //temp  FillProductsRank ();

  /*
    End Ranking Ventas
  */

  /*
    Start Proveedores
   */

  vbox = gtk_vbox_new (FALSE, 3);
  gtk_widget_show (vbox);

  gtk_notebook_append_page (GTK_NOTEBOOK (tab), vbox,
			    gtk_label_new_with_mnemonic ("_Proveedores"));


  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_show (scroll);
  gtk_widget_set_size_request (scroll, 350, 150);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (vbox), scroll, FALSE, FALSE, 3);

  ventastats->proveedores_store = gtk_list_store_new (6,
						      G_TYPE_STRING,
						      G_TYPE_STRING,
						      G_TYPE_DOUBLE,
						      G_TYPE_INT,
						      G_TYPE_STRING,
						      G_TYPE_INT,
						      -1);

  ventastats->proveedores_tree = gtk_tree_view_new_with_model (GTK_TREE_MODEL (ventastats->proveedores_store));
  gtk_container_add (GTK_CONTAINER (scroll), ventastats->proveedores_tree);
  gtk_widget_show (ventastats->proveedores_tree);

  g_signal_connect (G_OBJECT (gtk_tree_view_get_selection (GTK_TREE_VIEW (ventastats->proveedores_tree))),
		    "changed", G_CALLBACK (FillProductsProveedor), NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Proveedor", renderer,
						     "text", 0,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (ventastats->proveedores_tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 0);
  gtk_tree_view_column_set_min_width (column, 200);
  gtk_tree_view_column_set_max_width (column, 200);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Rut", renderer,
						     "text", 1,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (ventastats->proveedores_tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 1);
  gtk_tree_view_column_set_min_width (column, 90);
  gtk_tree_view_column_set_max_width (column, 90);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Unidades", renderer,
						     "text", 2,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (ventastats->proveedores_tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 2);
  gtk_tree_view_column_set_min_width (column, 100);
  gtk_tree_view_column_set_max_width (column, 100);
  gtk_tree_view_column_set_resizable (column, FALSE);

  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)2, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Comprado", renderer,
						     "text", 3,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (ventastats->proveedores_tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 3);
  gtk_tree_view_column_set_min_width (column, 100);
  gtk_tree_view_column_set_max_width (column, 100);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Margen", renderer,
						     "text", 4,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (ventastats->proveedores_tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 4);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Contribuci贸n", renderer,
						     "text", 5,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (ventastats->proveedores_tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 5);
  gtk_tree_view_column_set_resizable (column, FALSE);

  //temp  FillListProveedores ();


  frame = gtk_frame_new ("Descripci贸n");
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 3);
  gtk_widget_show (frame);

  table = gtk_table_new (4, 4, FALSE);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  label = gtk_label_new ("Raz贸n Social: ");
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label,
			     0, 1,
			     0, 1);
  gtk_widget_show (label);

  ventastats->proveedor_razon = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (ventastats->proveedor_razon), 0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), ventastats->proveedor_razon,
			     1, 2,
			     0, 1);
  gtk_widget_show (ventastats->proveedor_razon);

  label = gtk_label_new ("Direcci贸n: ");
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label,
			     0, 1,
			     1, 2);
  gtk_widget_show (label);

  ventastats->proveedor_direccion = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (ventastats->proveedor_direccion), 0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), ventastats->proveedor_direccion,
			     1, 2,
			     1, 2);
  gtk_widget_show (ventastats->proveedor_direccion);

  label = gtk_label_new ("Fono: ");
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label,
			     0, 1,
			     2, 3);
  gtk_widget_show (label);

  ventastats->proveedor_fono = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (ventastats->proveedor_fono), 0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), ventastats->proveedor_fono,
			     1, 2,
			     2, 3);
  gtk_widget_show (ventastats->proveedor_fono);

  label = gtk_label_new ("Web ");
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label,
			     0, 1,
			     3, 4);
  gtk_widget_show (label);

  ventastats->proveedor_web = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (ventastats->proveedor_web), 0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), ventastats->proveedor_web,
			     1, 2,
			     3, 4);
  gtk_widget_show (ventastats->proveedor_web);

  label = gtk_label_new ("Giro: ");
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label,
			     2, 3,
			     0, 1);
  gtk_widget_show (label);

  ventastats->proveedor_giro = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (ventastats->proveedor_giro), 0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), ventastats->proveedor_giro,
			     3, 4,
			     0, 1);
  gtk_widget_show (ventastats->proveedor_giro);

  label = gtk_label_new ("Comuna: ");
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label,
			     2, 3,
			     1, 2);
  gtk_widget_show (label);

  ventastats->proveedor_comuna = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (ventastats->proveedor_comuna), 0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), ventastats->proveedor_comuna,
			     3, 4,
			     1, 2);
  gtk_widget_show (ventastats->proveedor_comuna);

  label = gtk_label_new ("E-Mail: ");
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label,
			     2, 3,
			     2, 3);
  gtk_widget_show (label);

  ventastats->proveedor_email = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (ventastats->proveedor_email), 0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), ventastats->proveedor_email,
			     3, 4,
			     2, 3);
  gtk_widget_show (ventastats->proveedor_email);

  label = gtk_label_new ("Observaciones: ");
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label,
			     2, 3,
			     3, 4);
  gtk_widget_show (label);

  ventastats->proveedor_observaciones = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (ventastats->proveedor_observaciones), 0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), ventastats->proveedor_observaciones,
			     3, 4,
			     3, 4);
  gtk_widget_show (ventastats->proveedor_observaciones);

  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_show (scroll);
  gtk_widget_set_size_request (scroll, 350, 200);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (vbox), scroll, FALSE, FALSE, 3);

  proveedores_store = gtk_tree_store_new (6,
					  G_TYPE_STRING,
					  G_TYPE_STRING,
					  G_TYPE_STRING,
					  G_TYPE_STRING,
					  G_TYPE_DOUBLE,
					  G_TYPE_DOUBLE,
					  -1);

  proveedores_tree = gtk_tree_view_new_with_model (GTK_TREE_MODEL (proveedores_store));
  gtk_container_add (GTK_CONTAINER (scroll), proveedores_tree);
  gtk_widget_show (proveedores_tree);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Producto", renderer,
						     "text", 0,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (proveedores_tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 0);
  gtk_tree_view_column_set_min_width (column, 150);
  gtk_tree_view_column_set_max_width (column, 150);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Marca", renderer,
						     "text", 1,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (proveedores_tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 1);
  gtk_tree_view_column_set_min_width (column, 150);
  gtk_tree_view_column_set_max_width (column, 150);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Unidad", renderer,
						     "text", 2,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (proveedores_tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Capacidad", renderer,
						     "text", 3,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (proveedores_tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Comprado", renderer,
						     "text", 4,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (proveedores_tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 4);
  gtk_tree_view_column_set_resizable (column, FALSE);

  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)4, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Vendido", renderer,
						     "text", 5,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (proveedores_tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 5);
  gtk_tree_view_column_set_resizable (column, FALSE);

  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)5, NULL);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  /*
    We fill all the necessary data to print the two treevies above
  */

  proveedores_print->tree = GTK_TREE_VIEW (ventastats->proveedores_tree);
  proveedores_print->title = "Libro";
  proveedores_print->name = "proveedores";
  proveedores_print->date_string = NULL;
  proveedores_print->cols[0].name = "Proveedore";
  proveedores_print->cols[0].num = 0;
  proveedores_print->cols[1].name = "Rut";
  proveedores_print->cols[1].num = 1;
  proveedores_print->cols[2].name = "Unidades";
  proveedores_print->cols[2].num = 2;
  proveedores_print->cols[3].name = "Comprado";
  proveedores_print->cols[3].num = 3;
  proveedores_print->cols[4].name = "Margen";
  proveedores_print->cols[4].num = 4;
  proveedores_print->cols[5].name = "Contribuci髇";
  proveedores_print->cols[5].num = 5;
  proveedores_print->cols[6].name = NULL;

  proveedores_print->son->tree = GTK_TREE_VIEW (proveedores_tree);
  proveedores_print->son->cols[0].name = "Producto";
  proveedores_print->son->cols[0].num = 0;
  proveedores_print->son->cols[1].name = "Marca";
  proveedores_print->son->cols[1].num = 1;
  proveedores_print->son->cols[2].name = "Unidad";
  proveedores_print->son->cols[2].num = 2;
  proveedores_print->son->cols[3].name = "Capacidad";
  proveedores_print->son->cols[3].num = 3;
  proveedores_print->son->cols[4].name = "Comprado";
  proveedores_print->son->cols[4].num = 4;
  proveedores_print->son->cols[5].name = "Vendido";
  proveedores_print->son->cols[5].num = 5;
  proveedores_print->son->cols[6].name = NULL;

  button = gtk_button_new_from_stock (GTK_STOCK_PRINT);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 3);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (PrintTwoTree), (gpointer)proveedores_print);

  table = gtk_table_new (2, 4, FALSE);
  gtk_box_pack_end (GTK_BOX (hbox), table, FALSE, FALSE, 3);
  gtk_widget_show (table);

  gtk_table_set_col_spacings (GTK_TABLE (table), 10);

  label = gtk_label_new ("");
  gtk_label_set_markup (GTK_LABEL (label), "<span size=\"xx-large\"><b>Total : </b></span>");
  gtk_table_attach_defaults (GTK_TABLE (table), label,
			     0, 1,
			     0, 1);
  gtk_widget_show (label);

  label = gtk_label_new ("");
  gtk_label_set_markup (GTK_LABEL (label), "<span size=\"x-large\"><b>Productos</b></span>");
  gtk_table_attach_defaults (GTK_TABLE (table), label,
			     1, 2,
			     0, 1);
  gtk_widget_show (label);

  total_productos = gtk_label_new ("");
  gtk_table_attach_defaults (GTK_TABLE (table), total_productos,
			     1, 2,
			     1, 2);
  gtk_widget_show (total_productos);

  label = gtk_label_new ("");
  gtk_label_set_markup (GTK_LABEL (label), "<span size=\"x-large\"><b>Comprado</b></span>");
  gtk_table_attach_defaults (GTK_TABLE (table), label,
			     2, 3,
			     0, 1);
  gtk_widget_show (label);

  total_comprado = gtk_label_new ("");
  gtk_table_attach_defaults (GTK_TABLE (table), total_comprado,
			     2, 3,
			     1, 2);
  gtk_widget_show (total_comprado);

  label = gtk_label_new ("");
  gtk_label_set_markup (GTK_LABEL (label), "<span size=\"x-large\"><b>Vendido</b></span>");
  gtk_table_attach_defaults (GTK_TABLE (table), label,
			     3, 4,
			     0, 1);
  gtk_widget_show (label);


  total_vendido = gtk_label_new ("");
  gtk_table_attach_defaults (GTK_TABLE (table), total_vendido,
			     3, 4,
			     1, 2);
  gtk_widget_show (total_vendido);

  /*
    End Proveedores
  */

  /*
    Start Clientes
   */

  vbox = gtk_vbox_new (FALSE, 3);
  gtk_widget_show (vbox);

  gtk_notebook_append_page (GTK_NOTEBOOK (tab), vbox,
			    gtk_label_new_with_mnemonic ("C_lientes"));

  creditos_box (vbox);

  /*
    End Clientes
  */

  /*
    Start Morosos
   */
  /*
  vbox = gtk_vbox_new (FALSE, 3);
  gtk_widget_show (vbox);

  gtk_notebook_append_page (GTK_NOTEBOOK (tab), vbox,
			    gtk_label_new ("Morosos"));

  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_show (scroll);
  gtk_widget_set_size_request (scroll, 350, (MODULE_BOX_HEIGHT - 70));
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (vbox), scroll, FALSE, FALSE, 3);

  ventastats->store_morosos = gtk_list_store_new (3,
						  G_TYPE_STRING,
						  G_TYPE_STRING,
						  G_TYPE_STRING,
						  -1);

  ventastats->tree_morosos = gtk_tree_view_new_with_model (GTK_TREE_MODEL (ventastats->store_morosos));
  gtk_container_add (GTK_CONTAINER (scroll), ventastats->tree_morosos);
  gtk_widget_show (ventastats->tree_morosos);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Nombre", renderer,
						     "text", 0,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (ventastats->tree_morosos), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 1);
  gtk_tree_view_column_set_min_width (column, 200);
  gtk_tree_view_column_set_max_width (column, 200);
  gtk_tree_view_column_set_resizable (column, FALSE);


  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Rut", renderer,
						     "text", 0,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (ventastats->tree_morosos), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 1);
  gtk_tree_view_column_set_min_width (column, 100);
  gtk_tree_view_column_set_max_width (column, 100);
  gtk_tree_view_column_set_resizable (column, FALSE);


  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Fecha Vencimiento", renderer,
						     "text", 0,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (ventastats->tree_morosos), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 1);
  gtk_tree_view_column_set_min_width (column, 100);
  gtk_tree_view_column_set_max_width (column, 100);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Monto Deuda", renderer,
						     "text", 0,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (ventastats->tree_morosos), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 1);
  gtk_tree_view_column_set_min_width (column, 100);
  gtk_tree_view_column_set_max_width (column, 100);
  gtk_tree_view_column_set_resizable (column, FALSE);

  */
  /*
    End Morosos
   */
  /*
    Inicio Informe Vendedores

    Para habilitar esta opcion ahi que tener
    precaucion en la funcion que refresca cada tab
  */

  /*
  vbox = gtk_vbox_new (FALSE, 0);
  gtk_notebook_append_page (GTK_NOTEBOOK (tab), vbox,
			    gtk_label_new_with_mnemonic ("_Comisiones"));
  gtk_widget_set_size_request (GTK_WIDGET (tab), MODULE_BOX_WIDTH - 5, MODULE_BOX_HEIGHT);
  gtk_widget_show (vbox);

  ComisionesTab (vbox);
  */

  /*
     Fin Vendedores
  */


  /*
    Inicio Caja
  */

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox);
  gtk_notebook_append_page (GTK_NOTEBOOK (tab), vbox,
			    gtk_label_new_with_mnemonic ("_Caja"));
  gtk_widget_set_size_request (GTK_WIDGET (tab), MODULE_BOX_WIDTH - 5, MODULE_BOX_HEIGHT);

  CajaTab (vbox);

  /*
    Fin Caja
  */

  /*
    Inicio Compras
  */

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox);
  gtk_notebook_append_page (GTK_NOTEBOOK (tab), vbox,
			    gtk_label_new_with_mnemonic ("Co_mpras"));
  gtk_widget_set_size_request (GTK_WIDGET (tab), MODULE_BOX_WIDTH - 5, MODULE_BOX_HEIGHT);


  InformeCompras (vbox);

  /*
    Fin Compras
  */

  /*
    Inicio Pago Facturas
  */

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox);
  gtk_notebook_append_page (GTK_NOTEBOOK (tab), vbox,
			    gtk_label_new_with_mnemonic ("Pago _Facturas"));
  gtk_widget_set_size_request (GTK_WIDGET (tab), MODULE_BOX_WIDTH - 5, MODULE_BOX_HEIGHT);


  InformeFacturas (vbox);

}

void
GetSelectedDate (void)
{
  gchar *from = g_strdup (gtk_button_get_label (button_from));
  gchar *to = g_strdup (gtk_button_get_label (button_to));

  if (strchr (from, '/') != NULL)
    {
      ventastats->selected_from_day = atoi (g_strdup_printf ("%c%c", from[0], from[1]));
      ventastats->selected_from_month = atoi (g_strdup_printf ("%c%c", from[3], from[4]));
      ventastats->selected_from_year = atoi (g_strdup_printf ("%c%c%c%c", from[6], from[7], from[8], from[9]));

      gtk_label_set_markup (GTK_LABEL (label_from),
			    g_strdup_printf ("<span size=\"xx-large\">%.2d/%.2d/%d</span>",
					     ventastats->selected_from_day,
					     ventastats->selected_from_month,
					     ventastats->selected_from_year));
    }
  if (strchr (to, '/') != NULL)
    {
      ventastats->selected_to_day = atoi (g_strdup_printf ("%c%c", to[0], to[1]));
      ventastats->selected_to_month = atoi (g_strdup_printf ("%c%c", to[3], to[4]));
      ventastats->selected_to_year = atoi (g_strdup_printf ("%c%c%c%c", to[6], to[7], to[8], to[9]));
      gtk_label_set_markup (GTK_LABEL (label_to),
			    g_strdup_printf ("<span size=\"xx-large\">%.2d/%.2d/%d</span>",
					     ventastats->selected_to_day,
					     ventastats->selected_to_month,
					     ventastats->selected_to_year));
    }
}

void
FillStatsStore (GtkTreeStore *store)
{
  gchar *pago = NULL;
  gint tuples, i;
  gint sell_type;
  GtkTreeIter iter;
  PGresult *res;

  res = SearchTuplesByDate
    (ventastats->selected_from_year, ventastats->selected_from_month,ventastats->selected_from_day,
     ventastats->selected_to_year, ventastats->selected_to_month,ventastats->selected_to_day,
     "fecha", "id, maquina, vendedor, monto, date_part('day', fecha), date_part('month', fecha), "
     "date_part('year', fecha), date_part('hour', fecha), date_part('minute', fecha), tipo_venta");


  tuples = PQntuples (res);

  if (tuples == 0)
    return;

  gtk_tree_store_clear (store);

  for (i = 0; i < tuples; i++)
    {
      sell_type = atoi (PQgetvalue (res, i, 9));

      switch (sell_type)
	{
	case CASH:
	  pago = "Contado";
	  break;
	case CREDITO:
	  pago = "Credito";
	  break;
	case CHEQUE:
	  pago = "Cheque";
	  break;
	case TARJETA:
	  pago = "Tarjeta";
	  break;
	default:
	  pago = "Indefinido";
	  break;
	}

      gtk_tree_store_append (store, &iter, NULL);
      gtk_tree_store_set (store, &iter,
			  0,
			  g_strdup_printf
			  ("%.2d/%.2d/%s %.2d:%.2d", atoi (PQgetvalue (res, i, 4)),
			   atoi (PQgetvalue (res, i, 5)), PQgetvalue (res, i, 6),
			   atoi (PQgetvalue (res, i, 7)), atoi (PQgetvalue (res, i, 8))),
			  1, PQgetvalue (res, i, 0),
			  2, PQgetvalue (res, i, 1),
			  3, PQgetvalue (res, i, 2),
			  4, PutPoints (g_strdup_printf
					("%.0f", strtod (PQgetvalue (res, i, 3), (char **)NULL))),
			  5, pago,
			  -1);
    }

}

void
FillTreeView (void)
{
  gtk_tree_store_clear (ventastats->store_ventas_stats);
  gtk_tree_store_clear (ventastats->store_venta);

  FillStatsStore (ventastats->store_ventas_stats);

  SetTotalSell ();

}

void
SetTotalSell (void)
{
  gint total_cash_sell;
  gint total_cash;
  gint total_credit_sell;
  gint total_credit;
  gint total_sell;
  gint total_ventas;

  total_cash_sell = GetTotalCashSell (ventastats->selected_from_year,
				      ventastats->selected_from_month,
				      ventastats->selected_from_day,
				      ventastats->selected_to_year,
				      ventastats->selected_to_month,
				      ventastats->selected_to_day,
				      &total_cash);

  total_credit_sell = GetTotalCreditSell (ventastats->selected_from_year,
					  ventastats->selected_from_month,
					  ventastats->selected_from_day,
					  ventastats->selected_to_year,
					  ventastats->selected_to_month,
					  ventastats->selected_to_day,
					  &total_credit);

  total_sell = GetTotalSell (ventastats->selected_from_year,
			     ventastats->selected_from_month,
			     ventastats->selected_from_day,
			     ventastats->selected_to_year,
			     ventastats->selected_to_month,
			     ventastats->selected_to_day,
			     &total_ventas);

  gtk_label_set_markup (GTK_LABEL (ventastats->total_cash_sell_label),
		      g_strdup_printf ("<span size=\"xx-large\">$%s</span>",
				       PutPoints (g_strdup_printf ("%d", total_cash_sell))));

  if (total_cash_sell != 0)
    gtk_label_set_markup (GTK_LABEL (n_ventas),
			  g_strdup_printf ("<span size=\"xx-large\">%d</span>", total_cash));

  if (total_cash_sell != 0)
    gtk_label_set_markup (GTK_LABEL (venta_promedio),
			  g_strdup_printf ("<span size=\"xx-large\">$%s</span>",
					   PutPoints (g_strdup_printf ("%d", total_cash_sell / total_cash))));

  gtk_label_set_markup (GTK_LABEL (ventastats->total_credit_sell_label),
		      g_strdup_printf ("<span size=\"xx-large\">$%s</span>",
				       PutPoints (g_strdup_printf ("%d", total_credit_sell))));

  gtk_label_set_markup (GTK_LABEL (credito_n_ventas),
			g_strdup_printf ("<span size=\"xx-large\">%d</span>", total_credit));

  if (total_credit_sell != 0)
    gtk_label_set_markup (GTK_LABEL (credito_venta_promedio),
			  g_strdup_printf ("<span size=\"xx-large\">$%s</span>",
					   PutPoints (g_strdup_printf ("%d", total_credit_sell / total_credit))));

  gtk_label_set_markup (GTK_LABEL (ventastats->total_sell_label),
		      g_strdup_printf ("<span size=\"xx-large\">$%s</span>",
					PutPoints (g_strdup_printf ("%d", total_sell))));

  gtk_label_set_markup (GTK_LABEL (total_n_ventas),
			g_strdup_printf ("<span size=\"xx-large\">%d</span>", total_ventas));

  if (total_ventas != 0)
    gtk_label_set_markup (GTK_LABEL (total_venta_promedio),
			  g_strdup_printf ("<span size=\"xx-large\">$%s</span>",
					   PutPoints (g_strdup_printf ("%d", total_sell / total_ventas))));
}

void
ChangeVenta (void)
{
  GtkTreeIter iter;
  gchar *idventa;
  gint i, tuples;
  PGresult *res;

  if (gtk_tree_selection_get_selected (ventastats->selection, NULL, &iter) == TRUE)
    {
      gtk_tree_model_get (GTK_TREE_MODEL (ventastats->store_ventas_stats), &iter,
			  1, &idventa,
			  -1);

      gtk_tree_store_clear (ventastats->store_venta);

      res = EjecutarSQL
	(g_strdup_printf
	 ("SELECT descripcion, marca, contenido, unidad, cantidad, precio, (cantidad * precio) AS "
	  "monto FROM products_sell_history WHERE id_venta=%s", idventa));

      tuples = PQntuples (res);

      for (i = 0; i < tuples; i++)
	{
	  gtk_tree_store_append (ventastats->store_venta, &iter, NULL);
	  gtk_tree_store_set (ventastats->store_venta, &iter,
			      0, g_strdup_printf ("%s %s %s %s", PQgetvalue (res, i, 0),
						  PQgetvalue (res, i, 1), PQgetvalue (res, i, 2),
						  PQgetvalue (res, i, 3)),
			      1, PQgetvalue (res, i, 4),
			      2, PutPoints (PQgetvalue (res, i, 5)),
			      3, PutPoints (g_strdup_printf
					    ("%.0f", strtod (PQgetvalue (res, i, 6),
							     (char **)NULL))),
			      -1);
	}
    }
}

void
FillCajaStats (void)
{
  PGresult *res;
  gint i, tuples;
  GtkTreeIter iter;

  res = EjecutarSQL ("SELECT inicio, termino, date_part('day',fecha_inicio), date_part('month',fecha_inicio), "
		     "date_part('year',fecha_inicio), date_part('day',fecha_termino), date_part('month',fecha_termino), "
		     "date_part('year',fecha_termino), perdida FROM caja");

  tuples = PQntuples (res);

  for (i = 0; i < tuples; i++)
    {
      gtk_list_store_append (ventastats->store_caja, &iter);
      gtk_list_store_set (ventastats->store_caja, &iter,
			  0, g_strdup_printf ("%.2d/%.2d/%.4d", atoi (PQgetvalue (res, i, 2)), atoi (PQgetvalue (res, i, 3)),
					      atoi (PQgetvalue (res, i, 4))),
			  1, PutPoints (PQgetvalue (res, i, 0)),
			  2, g_strdup_printf ("%.2d/%.2d/%.4d", atoi (PQgetvalue (res, i, 5)), atoi (PQgetvalue (res, i, 6)),
					      atoi (PQgetvalue (res, i, 7))),
			  3, PutPoints (PQgetvalue (res, i, 1)),
			  4, PutPoints (PQgetvalue (res, i, 8)),
			  -1);
    }
}

void
FillProductsRank (void)
{
  PGresult *res;
  gint i, tuples;
  GtkTreeIter iter;
  gint vendidos = 0, costo = 0, contrib = 0;
  gdouble margen = 0;

  res = ReturnProductsRank (ventastats->selected_from_year, ventastats->selected_from_month, ventastats->selected_from_day,
			    ventastats->selected_to_year, ventastats->selected_to_month, ventastats->selected_to_day);

  tuples = PQntuples (res);

  gtk_list_store_clear (ventastats->rank_store);

  for (i = 0; i < tuples; i++)
    {
      vendidos += atoi (PQgetvalue (res, i, 5));
      costo += atoi (PQgetvalue (res, i, 6));
      contrib += atoi (PQgetvalue (res, i, 7));

      gtk_list_store_append (ventastats->rank_store, &iter);
      gtk_list_store_set (ventastats->rank_store, &iter,
			  0, PQgetvalue (res, i, 0),
			  1, PQgetvalue (res, i, 1),
			  2, atoi (PQgetvalue (res, i, 2)),
			  3, PQgetvalue (res, i, 3),
			  4, strtod (PUT (PQgetvalue (res, i, 4)), (char **)NULL),
			  5, atoi (PQgetvalue (res, i, 5)),
			  6, atoi (PQgetvalue (res, i, 6)),
			  7, atoi (PQgetvalue (res, i, 7)),
			  8, (((gdouble)atoi (PQgetvalue (res, i, 7)) /
			       atoi (PQgetvalue (res, i, 6))) * 100),
			  -1);
  }

  gtk_label_set_markup (GTK_LABEL (total_vendidos),
			g_strdup_printf ("<span size=\"x-large\">$%s</span>", PutPoints
					 (g_strdup_printf ("%d", vendidos))));
  gtk_label_set_markup (GTK_LABEL (total_costo),
			g_strdup_printf ("<span size=\"x-large\">$%s</span>", PutPoints
					 (g_strdup_printf ("%d", costo))));
  gtk_label_set_markup (GTK_LABEL (total_contrib),
			g_strdup_printf ("<span size=\"x-large\">$%s</span>", PutPoints
					 (g_strdup_printf ("%d", contrib))));

  margen = (((gdouble) contrib / costo) * 100);

  if (margen != 0)
    gtk_label_set_markup (GTK_LABEL (total_margen),
			  g_strdup_printf ("<span size=\"x-large\">%.2f%%</span>", margen));
  else
    gtk_label_set_markup (GTK_LABEL (total_margen),
			  g_strdup_printf ("<span size=\"x-large\">0%%</span>"));
}

void
FillListProveedores (void)
{
  GtkTreeIter iter;
  gint i, tuples, margen;
  PGresult *res;

  res = EjecutarSQL ("SELECT nombre, rut, (SELECT SUM (cantidad_ingresada) FROM products_buy_history WHERE id_compra IN (SELECT id FROM compras WHERE rut_proveedor=proveedores.rut)), (SELECT SUM (cantidad_ingresada * precio) FROM products_buy_history WHERE id_compra IN (SELECT id FROM compras WHERE rut_proveedor=proveedores.rut))::integer, (SELECT SUM (margen) / COUNT (*) FROM products_buy_history WHERE id_compra IN (SELECT id FROM compras WHERE rut_proveedor=proveedores.rut))::integer, (SELECT SUM (precio_venta - (precio * (margen / 100) +1))  FROM products_buy_history WHERE id_compra IN (SELECT id FROM compras WHERE rut_proveedor=proveedores.rut)) FROM proveedores");


  tuples = PQntuples (res);

  gtk_list_store_clear (ventastats->proveedores_store);

  for (i = 0; i < tuples; i++)
    {
      margen = atoi (PQgetvalue (res, i, 4));

      gtk_list_store_append (ventastats->proveedores_store, &iter);
      gtk_list_store_set (ventastats->proveedores_store, &iter,
			  0, PQgetvalue (res, i, 0),
			  1, PQgetvalue (res, i, 1),
			  2, strtod (PQgetvalue (res, i, 2), (char **)NULL),
			  3, atoi (PQgetvalue (res, i, 3)),
			  4, g_strdup_printf ("%d%%", margen ? margen : 0),
			  5, atoi (PQgetvalue (res, i, 5)),
			  -1);

    }
}

void
InformeFacturasShow (void)
{
  gint tuples, tuples2, i, j;
  gint year = 0, month = 0, day = 0;
  gint monto_fecha = 0;
  GtkTreeIter fecha_iter, factura_iter, guia_iter;

  PGresult *res, *res2;


  /*
    Facturas por Pagar
  */

  res = EjecutarSQL
    ("SELECT t1.id, t1.num_factura, t1.monto, date_part ('day', t1.fecha), date_part('month', t1.fecha), date_part('year', t1.fecha), t1.id_compra, date_part ('day', fecha_pago) AS pay_day, date_part ('month', fecha_pago) AS pay_month, date_part ('year', fecha_pago) AS pay_year, t1.forma_pago, t1.id, t1.rut_proveedor FROM facturas_compras AS t1 WHERE pagada='f' ORDER BY pay_year, pay_month, pay_day ASC");

  tuples = PQntuples (res);

  gtk_tree_store_clear (ventastats->pagar_store);

  for (i = 0; i < tuples; i++)
    {
      if (atoi (PQgetvalue (res, i, 7)) > day || atoi (PQgetvalue (res, i, 8)) > month
	  || atoi (PQgetvalue (res, i, 9)) > year)
	{
	  if (gtk_tree_store_iter_is_valid (ventastats->pagar_store, &fecha_iter) != FALSE)
	    {
	      gtk_tree_store_set (ventastats->pagar_store, &fecha_iter,
				  6, PutPoints (g_strdup_printf ("%d", monto_fecha)),
				  -1);
	      monto_fecha = 0;
	    }

	  day = atoi (PQgetvalue (res, i, 7));
	  month = atoi (PQgetvalue (res, i, 8));
	  year = atoi (PQgetvalue (res, i, 9));

	  gtk_tree_store_append (ventastats->pagar_store, &fecha_iter, NULL);
	  gtk_tree_store_set (ventastats->pagar_store, &fecha_iter,
			      5, g_strdup_printf
			      ("%.2d/%.2d/%.4d", atoi (PQgetvalue (res, i, 7)),
			       atoi (PQgetvalue (res, i, 8)), atoi (PQgetvalue (res, i, 9))),
			      -1);
	}

      gtk_tree_store_append (ventastats->pagar_store, &factura_iter, &fecha_iter);
      gtk_tree_store_set (ventastats->pagar_store, &factura_iter,
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
	      gtk_tree_store_append (ventastats->pagar_store, &guia_iter, &factura_iter);
	      gtk_tree_store_set (ventastats->pagar_store, &guia_iter,
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
  if (gtk_tree_store_iter_is_valid (ventastats->pagar_store, &fecha_iter) != FALSE)
    {
      gtk_tree_store_set (ventastats->pagar_store, &fecha_iter,
			  6, PutPoints (g_strdup_printf ("%d", monto_fecha)),
			  -1);
      monto_fecha = 0;
    }

  /*
    Facturas Pagadas
  */

   res = EjecutarSQL
    ("SELECT t1.id, t1.num_factura, t1.monto, date_part ('day', t1.fecha), date_part('month', t1.fecha), date_part('year', t1.fecha), t1.id_compra, date_part ('day', fecha_pago) AS pay_day, date_part ('month', fecha_pago) AS pay_month, date_part ('year', fecha_pago) AS pay_year, t1.forma_pago, t1.id, t1.rut_proveedor FROM facturas_compras AS t1 WHERE pagada='t' ORDER BY pay_year, pay_month, pay_day ASC");

  tuples = PQntuples (res);

  gtk_tree_store_clear (ventastats->pagadas_store);

  for (i = 0; i < tuples; i++)
    {
      if (atoi (PQgetvalue (res, i, 7)) > day || atoi (PQgetvalue (res, i, 8)) > month
	  || atoi (PQgetvalue (res, i, 9)) > year)
	{
	  if (gtk_tree_store_iter_is_valid (ventastats->pagadas_store, &fecha_iter) != FALSE)
	    {
	      gtk_tree_store_set (ventastats->pagar_store, &fecha_iter,
				  6, PutPoints (g_strdup_printf ("%d", monto_fecha)),
				  -1);
	      monto_fecha = 0;
	    }

	  day = atoi (PQgetvalue (res, i, 7));
	  month = atoi (PQgetvalue (res, i, 8));
	  year = atoi (PQgetvalue (res, i, 9));

	  gtk_tree_store_append (ventastats->pagadas_store, &fecha_iter, NULL);
	  gtk_tree_store_set (ventastats->pagadas_store, &fecha_iter,
			      5, g_strdup_printf
			      ("%.2d/%.2d/%.4d", atoi (PQgetvalue (res, i, 7)),
			       atoi (PQgetvalue (res, i, 8)), atoi (PQgetvalue (res, i, 9))),
			      -1);
	}

      gtk_tree_store_append (ventastats->pagadas_store, &factura_iter, &fecha_iter);
      gtk_tree_store_set (ventastats->pagadas_store, &factura_iter,
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
			  7, PutPoints (PQgetvalue (res, i, 2)),
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
	      gtk_tree_store_append (ventastats->pagadas_store, &guia_iter, &factura_iter);
	      gtk_tree_store_set (ventastats->pagadas_store, &guia_iter,
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
  if (gtk_tree_store_iter_is_valid (ventastats->pagadas_store, &fecha_iter) != FALSE)
    {
      gtk_tree_store_set (ventastats->pagadas_store, &fecha_iter,
			  7, PutPoints (g_strdup_printf ("%d", monto_fecha)),
			  -1);
      monto_fecha = 0;
    }

}

void
InformeFacturas (GtkWidget *box)
{
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *scroll;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkWidget *button;

  Print *pagar = (Print *) malloc (sizeof (Print));

  Print *pagadas = (Print *) malloc (sizeof (Print));

  vbox = gtk_vbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (box), vbox, FALSE, FALSE, 3);
  gtk_widget_show (vbox);

  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_size_request (scroll, 350, 210);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (vbox), scroll, FALSE, FALSE, 3);
  gtk_widget_show (scroll);

  ventastats->pagar_store = gtk_tree_store_new (7,
						G_TYPE_STRING,
						G_TYPE_STRING,
						G_TYPE_STRING,
						G_TYPE_STRING,
						G_TYPE_STRING,
						G_TYPE_STRING,
						G_TYPE_STRING);

  ventastats->pagar_tree = gtk_tree_view_new_with_model (GTK_TREE_MODEL (ventastats->pagar_store));
  gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (ventastats->pagar_tree), TRUE);
  gtk_container_add (GTK_CONTAINER (scroll), ventastats->pagar_tree);
  gtk_widget_show (ventastats->pagar_tree);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("", renderer,
						     "text", 0,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (ventastats->pagar_tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_min_width (column, 30);
  gtk_tree_view_column_set_max_width (column, 30);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Rut", renderer,
						     "text", 1,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (ventastats->pagar_tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_min_width (column, 75);
  gtk_tree_view_column_set_max_width (column, 75);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Numero", renderer,
						     "text", 2,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (ventastats->pagar_tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_min_width (column, 120);
  gtk_tree_view_column_set_max_width (column, 120);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Compra", renderer,
						     "text", 3,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (ventastats->pagar_tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("F. Emision", renderer,
						     "text", 4,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (ventastats->pagar_tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Fecha Pagos", renderer,
						     "text", 5,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (ventastats->pagar_tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_min_width (column, 80);
  gtk_tree_view_column_set_max_width (column, 80);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Monto", renderer,
						     "text", 6,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (ventastats->pagar_tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  button = gtk_button_new_from_stock (GTK_STOCK_PRINT);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);
  gtk_widget_show (button);

  pagar->tree = GTK_TREE_VIEW (ventastats->pagar_tree);
  pagar->title = "Facturas Pendientes de pago";
  pagar->name = "pagar";
  pagar->date_string = NULL;
  pagar->cols[0].name = "";
  pagar->cols[0].num = 0;
  pagar->cols[1].name = "Rut";
  pagar->cols[1].num = 1;
  pagar->cols[2].name = "Numero";
  pagar->cols[2].num = 0;
  pagar->cols[3].name = "Compra";
  pagar->cols[3].num = 3;
  pagar->cols[4].name = "F. Emision";
  pagar->cols[4].num = 4;
  pagar->cols[5].name = "Fecha Pagos";
  pagar->cols[5].num = 4;
  pagar->cols[6].name = "Monto";
  pagar->cols[6].num = 6;
  pagar->cols[7].name = NULL;

  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (PrintTree), (gpointer)pagar);

  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_size_request (scroll, 350, 210);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (vbox), scroll, FALSE, FALSE, 3);
  gtk_widget_show (scroll);

  ventastats->pagadas_store = gtk_tree_store_new (8,
						  G_TYPE_STRING,
						  G_TYPE_STRING,
						  G_TYPE_STRING,
						  G_TYPE_STRING,
						  G_TYPE_STRING,
						  G_TYPE_STRING,
						  G_TYPE_STRING,
						  G_TYPE_STRING);

  ventastats->pagadas_tree = gtk_tree_view_new_with_model (GTK_TREE_MODEL (ventastats->pagadas_store));
  gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (ventastats->pagadas_tree), TRUE);
  gtk_container_add (GTK_CONTAINER (scroll), ventastats->pagadas_tree);
  gtk_widget_show (ventastats->pagadas_tree);

   renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("", renderer,
						     "text", 0,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (ventastats->pagadas_tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_min_width (column, 30);
  gtk_tree_view_column_set_max_width (column, 30);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Rut", renderer,
						     "text", 1,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (ventastats->pagadas_tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_min_width (column, 75);
  gtk_tree_view_column_set_max_width (column, 75);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Numero", renderer,
						     "text", 2,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (ventastats->pagadas_tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_min_width (column, 120);
  gtk_tree_view_column_set_max_width (column, 120);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Compra", renderer,
						     "text", 3,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (ventastats->pagadas_tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("F. Emision", renderer,
						     "text", 4,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (ventastats->pagadas_tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Fecha Pagos", renderer,
						     "text", 5,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (ventastats->pagadas_tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_min_width (column, 80);
  gtk_tree_view_column_set_max_width (column, 80);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Forma Pago", renderer,
						     "text", 6,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (ventastats->pagadas_tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Monto", renderer,
						     "text", 7,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (ventastats->pagadas_tree), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  button = gtk_button_new_from_stock (GTK_STOCK_PRINT);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);
  gtk_widget_show (button);

  pagadas->tree = GTK_TREE_VIEW (ventastats->pagadas_tree);
  pagadas->title = "Facturas Pagadas";
  pagadas->name = "pagadas";
  pagadas->date_string = NULL;
  pagadas->cols[0].name = "";
  pagadas->cols[0].num = 0;
  pagadas->cols[1].name = "Rut";
  pagadas->cols[1].num = 1;
  pagadas->cols[2].name = "Numero";
  pagadas->cols[2].num = 0;
  pagadas->cols[3].name = "Compra";
  pagadas->cols[3].num = 3;
  pagadas->cols[4].name = "F. Emision";
  pagadas->cols[4].num = 4;
  pagadas->cols[5].name = "Fecha Pagos";
  pagadas->cols[5].num = 4;
  pagadas->cols[6].name = "Forma Pago";
  pagadas->cols[6].num = 6;
  pagadas->cols[7].name = "Monto";
  pagadas->cols[7].num = 7;
  pagadas->cols[8].name = NULL;

  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (PrintTree), (gpointer)pagadas);

}

void
RefreshCurrentTab (GtkNotebook *notebook, GtkNotebookPage *page, guint page_num,
		   gpointer user_data)
{

  if (page_num != 4)
    gtk_toggle_button_set_active (caja_button, FALSE);

  switch (page_num)
    {
    case 0:
      FillTreeView ();
      break;
    case 1:
      FillProductsRank ();
      break;
    case 2:
      FillListProveedores ();
      break;
    case 3:
      FillClientStore (creditos->store);
      break;
    case 4:
      FillCajaData ();
      break;
    case 5:
      InformeComprasShow ();
      break;
    case 6:
      InformeFacturasShow ();
      break;
    default:
      break;
    }
}
