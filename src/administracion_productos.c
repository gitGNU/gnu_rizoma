/*administracion_productos.c
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

#include<stdlib.h>
#include<string.h>

#include<math.h>

#include"tipos.h"
#include"main.h"
#include"administracion_productos.h"
#include"postgres-functions.h"
#include"errors.h"
#include"printing.h"
#include"compras.h"
#include"dimentions.h"

GtkWidget *codigo_entry;
GtkWidget *product_entry;
GtkWidget *precio_entry;
GtkWidget *barcode_entry;
GtkWidget *stock_entry;
GtkWidget *marca_entry;
GtkWidget *cantidad_entry;
GtkWidget *unidad_entry;

GtkWidget *impuesto_adic;
GtkWidget *familia;
GtkWidget *perecible;
GtkWidget *ventas_dias;
GtkWidget *stock_dias;
GtkWidget *stock_min;
GtkWidget *margen_entry;
GtkWidget *total_unidades_vendidas;

GtkWidget *costo_promedio;
GtkWidget *contrib_unit;
GtkWidget *precio_venta;
GtkWidget *mermita;
GtkWidget *mermata;
GtkWidget *ici;

GtkWidget *comp_totales;
GtkWidget *total_vendido;
GtkWidget *indice_t;
GtkWidget *contri_agr;
GtkWidget *contri_proy;
GtkWidget *stock_valor;

GtkWidget *elab_date;
GtkWidget *venc_date;

gboolean Deleting;

GtkWidget *inv_total_stock;
GtkWidget *valor_total_stock;
GtkWidget *contri_total_stock;

GtkWidget *combo_merma;

GtkWidget *label_found;

GtkWidget *canje_buttons_t;
GtkWidget *canje_buttons_f;
GtkWidget *stock_pro;

GtkWidget *tasa_canje;

GtkWidget *mayor_buttons_t;
GtkWidget *mayor_buttons_f;
GtkWidget *mayor_cantidad;
GtkWidget *mayor_precio;

GtkWidget *combo_proveedores;
GtkWidget *model_proveedores;

GtkWidget *entry_devolucion;
GtkWidget *entry_recivir;

GtkWidget *mod_window;
gboolean fraccionm = FALSE;
GtkWidget *combo_imp;


void
Recepcion (GtkWidget *widget, gpointer data)
{
    gboolean out = (gboolean) data;
    gchar *q;

    if (out == FALSE)
    {
	gtk_widget_set_sensitive (main_window, TRUE);
	gtk_widget_destroy (gtk_widget_get_toplevel (widget));
    }
    else if (out == TRUE)
    {
	gchar *barcode = g_strdup (gtk_label_get_text (GTK_LABEL (barcode_entry)));
	gchar *cantidad = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry_recivir)));

	if (strcmp (cantidad, "") != 0)
	{
	    q = g_strdup_printf ("SELECT id FROM hay_devolucion(%s) as (id int4)", barcode);
	    if (GetDataByOne
		(q) == NULL)
	    {
		AlertMSG (entry_recivir, "No existe devoluciones de este producto");
		return;
	    }
	    g_free(q);
	    q = g_strdup_printf ("SELECT respuesta FROM puedo_devolver(%s,%s) "
				 "as (respuesta bool)", CUT (cantidad), barcode);
	    if (g_str_equal(GetDataByOne(q), "t"))
	    {
		g_free(q);
		q = g_strdup_printf ("SELECT cantidad FROM max_prods_a_devolver"
				     "(%s)", barcode);

		gchar *msg;
		msg = g_strdup_printf("En esta recepción no puede recibir mas "
				      "de %s productos", GetDataByOne (q));

		AlertMSG (entry_recivir, msg);

		g_free(q);
		g_free(msg);
		return;
	    }

	    Recivir (barcode, cantidad);
	}

	gtk_widget_set_sensitive (main_window, TRUE);
	gtk_widget_destroy (gtk_widget_get_toplevel (widget));

	FillFields (ingreso->selection, NULL);
    }
}

void
RecivirWindow (GtkWidget *widget, gpointer data)
{
    GtkWidget *window;
    GtkWidget *hbox;
    GtkWidget *vbox;
    GtkWidget *button;
    GtkWidget *label;

    gtk_widget_set_sensitive (main_window, FALSE);

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (GTK_WINDOW (window), "Recepción de Mercadería");
    gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER_ALWAYS);
    gtk_window_set_resizable (GTK_WINDOW (window), FALSE);
    gtk_widget_show (window);

    g_signal_connect (G_OBJECT (window), "destroy",
		      G_CALLBACK (Recepcion), (gpointer)FALSE);

    vbox = gtk_vbox_new (FALSE, 3);
    gtk_container_add (GTK_CONTAINER (window), vbox);
    gtk_widget_show (vbox);
    hbox = gtk_hbox_new (FALSE, 3);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
    gtk_widget_show (hbox);

    label = gtk_label_new ("Cantidad a recibir : ");
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
    gtk_widget_show (label);

    entry_recivir = gtk_entry_new ();
    gtk_box_pack_start (GTK_BOX (hbox), entry_recivir, FALSE, FALSE, 3);
    gtk_widget_show (entry_recivir);

    hbox = gtk_hbox_new (FALSE, 3);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
    gtk_widget_show (hbox);

    button = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
    gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 3);
    gtk_widget_show (button);

    g_signal_connect (G_OBJECT (button), "clicked",
		      G_CALLBACK (Recepcion), (gpointer) FALSE);

    button = gtk_button_new_from_stock (GTK_STOCK_OK);
    gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);
    gtk_widget_show (button);

    g_signal_connect (G_OBJECT (entry_recivir), "activate",
		      G_CALLBACK (SendCursorTo), (gpointer)button);

    g_signal_connect (G_OBJECT (button), "clicked",
		      G_CALLBACK (Recepcion), (gpointer)TRUE);

}

void
Devolucion (GtkWidget *widget, gpointer data)
{
    gboolean out = (gboolean) data;

    if (out == FALSE)
    {
	gtk_widget_set_sensitive (main_window, TRUE);
	gtk_widget_destroy (gtk_widget_get_toplevel (widget));
    }
    else if (out == TRUE)
    {
	gchar *barcode = g_strdup (gtk_label_get_text (GTK_LABEL (barcode_entry)));
	gchar *cantidad = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry_devolucion)));

	if (strcmp (cantidad, "") != 0)
	{
	    if (strtod (PUT (cantidad), (char **)NULL) > GetCurrentStock (barcode))
	    {
		AlertMSG (entry_devolucion, "No puede devolver mas del stock actual");
		return;
	    }

	    Devolver (barcode, cantidad);
	}

	gtk_widget_set_sensitive (main_window, TRUE);
	gtk_widget_destroy (gtk_widget_get_toplevel (widget));

	FillFields (ingreso->selection, NULL);
    }
}

void
DevolucionWindow (GtkWidget *widget, gpointer data)
{
    GtkWidget *window;
    GtkWidget *hbox;
    GtkWidget *vbox;
    GtkWidget *button;
    GtkWidget *label;

    gtk_widget_set_sensitive (main_window, FALSE);

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (GTK_WINDOW (window), "Devolución de Mercadería");
    gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER_ALWAYS);
    gtk_window_set_resizable (GTK_WINDOW (window), FALSE);
    gtk_widget_show (window);

    g_signal_connect (G_OBJECT (window), "destroy",
		      G_CALLBACK (Devolucion), (gpointer)FALSE);

    vbox = gtk_vbox_new (FALSE, 3);
    gtk_container_add (GTK_CONTAINER (window), vbox);
    gtk_widget_show (vbox);

    hbox = gtk_hbox_new (FALSE, 3);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
    gtk_widget_show (hbox);

    label = gtk_label_new ("Cantidad a devolver : ");
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
    gtk_widget_show (label);

    entry_devolucion = gtk_entry_new ();
    gtk_box_pack_start (GTK_BOX (hbox), entry_devolucion, FALSE, FALSE, 3);
    gtk_widget_show (entry_devolucion);

    hbox = gtk_hbox_new (FALSE, 3);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
    gtk_widget_show (hbox);

    button = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
    gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 3);
    gtk_widget_show (button);

    g_signal_connect (G_OBJECT (button), "clicked",
		      G_CALLBACK (Devolucion), (gpointer)FALSE);

    button = gtk_button_new_from_stock (GTK_STOCK_OK);
    gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);
    gtk_widget_show (button);

    g_signal_connect (G_OBJECT (entry_devolucion), "activate",
		      G_CALLBACK (SendCursorTo), (gpointer)button);

    g_signal_connect (G_OBJECT (button), "clicked",
		      G_CALLBACK (Devolucion), (gpointer)TRUE);
}

void
AjustarMercaderia (GtkWidget *widget, gpointer data)
{
    gint active;
    gdouble cantidad;
    //  gdouble stock;
    gchar *barcode;
    GtkTreeIter iter;
    GtkTreeSelection *selection = gtk_tree_view_get_selection
	(GTK_TREE_VIEW (ingreso->treeview_products));

    if (data == NULL)
    {
	gtk_widget_destroy (gtk_widget_get_toplevel (widget));
	return;
    }

    active = gtk_combo_box_get_active (GTK_COMBO_BOX (combo_merma));
    // cantidad = (gdouble)atoi (g_strdup (gtk_entry_get_text (GTK_ENTRY (data))));
    cantidad = strtod (PUT (g_strdup (gtk_entry_get_text (GTK_ENTRY (data)))), (char **)NULL);

    gtk_tree_selection_get_selected (selection, NULL, &iter);
    gtk_tree_model_get (GTK_TREE_MODEL (ingreso->store), &iter,
			1, &barcode,
			-1);

    if (active == -1)
	ErrorMSG (GTK_WIDGET (active), "Debe Seleccionar un motivo de la merma");
    else
    {
	AjusteStock (cantidad, active+1, barcode);
	gtk_widget_destroy (gtk_widget_get_toplevel (widget));

	FillFields (ingreso->selection, NULL);
    }
}

void
AjusteWin (GtkWidget *widget, gpointer data)
{
    GtkWidget *window;
    GtkWidget *hbox;
    GtkWidget *vbox;
    GtkWidget *entry;
    GtkWidget *label;
    GtkWidget *button;

    PGresult *res;
    gint tuples, i;
    gchar *barcode;
    GtkTreeIter iter;
    GtkTreeSelection *selection = gtk_tree_view_get_selection
	(GTK_TREE_VIEW (ingreso->treeview_products));

    if (gtk_tree_selection_get_selected (selection, NULL, &iter) == TRUE)
    {

	gtk_tree_model_get (GTK_TREE_MODEL (ingreso->store), &iter,
			    1, &barcode,
			    -1);

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (window), "Ajuste de Mercadería");
	gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER_ALWAYS);
	gtk_widget_show (window);
	gtk_window_present (GTK_WINDOW (window));
	//      gtk_window_set_transient_for (GTK_WINDOW (window), GTK_WINDOW (main_window));
	gtk_window_set_resizable (GTK_WINDOW (window), FALSE);
	gtk_widget_set_size_request (window, 220, -1);

	g_signal_connect (G_OBJECT (window), "destroy",
			  G_CALLBACK (AjustarMercaderia), NULL);

	vbox = gtk_vbox_new (FALSE, 3);
	gtk_container_add (GTK_CONTAINER (window), vbox);
	gtk_widget_show (vbox);

	hbox = gtk_hbox_new (FALSE, 3);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
	gtk_widget_show (hbox);

	label = gtk_label_new ("");
	gtk_label_set_markup (GTK_LABEL (label),
			      g_strdup_printf ("Stock: %.2f", GetCurrentStock (barcode)));
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
	gtk_widget_show (label);

	label = gtk_label_new ("Inventario:");
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
	gtk_widget_show (label);

	entry = gtk_entry_new ();
	gtk_box_pack_start (GTK_BOX (hbox), entry, FALSE, FALSE, 3);
	gtk_widget_show (entry);

	gtk_window_set_focus (GTK_WINDOW (window), entry);

	res = EjecutarSQL ("SELECT id, nombre FROM select_tipo_merma() "
			   "AS (id int4, nombre varchar(20))");

	tuples = PQntuples (res);

	combo_merma = gtk_combo_box_new_text ();
	gtk_box_pack_start (GTK_BOX (vbox), combo_merma, FALSE, FALSE, 3);
	gtk_widget_show (combo_merma);

	for (i = 0; i < tuples; i++)
	    gtk_combo_box_append_text (GTK_COMBO_BOX (combo_merma),
				       g_strdup_printf ("%s", PQgetvalue (res, i, 1)));

	g_signal_connect (G_OBJECT (entry), "activate",
			  G_CALLBACK (SendCursorTo), combo_merma);

	hbox = gtk_hbox_new (FALSE, 3);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
	gtk_widget_show (hbox);

	button = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
	gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 3);
	gtk_widget_show (button);

	g_signal_connect (G_OBJECT (button), "clicked",
			  G_CALLBACK (AjustarMercaderia), NULL);

	button = gtk_button_new_from_stock (GTK_STOCK_OK);
	gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);
	gtk_widget_show (button);

	g_signal_connect (G_OBJECT (button), "clicked",
			  G_CALLBACK (AjustarMercaderia), (gpointer)entry);

	g_signal_connect (G_OBJECT (combo_merma), "changed",
			  G_CALLBACK (SendCursorTo), button);
    }
}

void
GuardarModificacionesProducto (void)
{
    gchar *barcode = g_strdup (gtk_label_get_text (GTK_LABEL (barcode_entry)));
    gchar *stock_minimo = g_strdup (gtk_entry_get_text (GTK_ENTRY (stock_min)));
    gchar *margen = g_strdup (gtk_entry_get_text (GTK_ENTRY (margen_entry)));
    gchar *new_venta = g_strdup (gtk_entry_get_text (GTK_ENTRY (precio_venta)));
    gboolean canjeable = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (canje_buttons_t));
    gint tasa = atoi (g_strdup (gtk_entry_get_text (GTK_ENTRY (tasa_canje))));
    gboolean mayorista = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (mayor_buttons_t));
    gint precio_mayorista = atoi (g_strdup (gtk_entry_get_text (GTK_ENTRY (mayor_precio))));
    gint cantidad_mayorista = atoi (g_strdup (gtk_entry_get_text (GTK_ENTRY (mayor_cantidad))));

    if (strcmp (stock_minimo, "") == 0)
	ErrorMSG (stock_min, "Debe setear stock minimo");
    else if (strcmp (margen, "") == 0)
	ErrorMSG (margen_entry, "Debe poner un valor de margen para el producto");
    else if (strcmp (new_venta, "") == 0)
	ErrorMSG (precio_venta, "Debe insertar un precio de venta");
    else
    {
	SetModificacionesProducto (barcode, stock_minimo, margen, new_venta, canjeable, tasa, mayorista, precio_mayorista,
				   cantidad_mayorista);
	FillFields (ingreso->selection, NULL);
    }
}

void
ModificarMargenVenta (GtkEditable *editable, gpointer data)
{
    gboolean margen = (gboolean) data;
    gchar *barcode = g_strdup (gtk_label_get_text (GTK_LABEL (barcode_entry)));
    gint new_margen = atoi (g_strdup (gtk_entry_get_text (GTK_ENTRY (margen_entry))));
    gint new_venta = new_venta = atoi (g_strdup (gtk_entry_get_text (GTK_ENTRY (precio_venta))));
    gint fifo = atoi (g_strdup (gtk_label_get_text (GTK_LABEL (costo_promedio))));
    gint contri_unit, stock;
    gdouble precio;
    gdouble iva = GetIVA (barcode);
    gdouble otros = GetOtros (barcode);

    if (strcmp (gtk_entry_get_text (GTK_ENTRY (margen_entry)), "") == 0 &&
	strcmp (gtk_entry_get_text (GTK_ENTRY (precio_venta)), "") == 0)
	return;

    if (fifo == 0)
	return;

    iva = (gdouble) iva / 100;
    if (otros != -1)
	otros = (gdouble) otros / 100;

    if (margen == TRUE)
    {
	gtk_entry_set_text (GTK_ENTRY (precio_venta), "");

	if (otros == -1)
	    precio = (gdouble) ((gdouble)(fifo * (gdouble)(new_margen + 100)) * (iva+1)) / 100;
	else
	{
	    precio = (gdouble) fifo + (gdouble)((gdouble)(fifo * new_margen) / 100);
	    precio = (gdouble)((gdouble)(precio * iva) +
			       (gdouble)(precio * otros) + (gdouble) precio);
	}

	gtk_entry_set_text (GTK_ENTRY (precio_venta),
			    g_strdup_printf ("%d", lround (precio)));

	gtk_label_set_markup (GTK_LABEL (contrib_unit),
			      g_strdup_printf
			      ("<b>%d</b>", lround ((gdouble)fifo * (gdouble)new_margen / 100)));
    }
    else if (margen == FALSE)
    {
	gtk_entry_set_text (GTK_ENTRY (margen_entry), "");

	if (otros == -1)
	    precio = (gdouble) ((new_venta / (gdouble)((iva+1) * fifo)) - 1) * 100;
	else
	{
	    precio = (gdouble) new_venta / (gdouble)(iva + otros + 1);
	    precio = (gdouble) precio - fifo;
	    precio = (gdouble)(precio / fifo) * 100;
	}

	gtk_entry_set_text (GTK_ENTRY (margen_entry),
			    g_strdup_printf ("%d", lround (precio)));

	gtk_label_set_markup (GTK_LABEL (contrib_unit),
			      g_strdup_printf
			      ("<b>%d</b>", lround ((gdouble)fifo * (gdouble)new_margen / 100)));
    }

    contri_unit = atoi (g_strdup (gtk_label_get_text (GTK_LABEL (contrib_unit))));

    new_venta = atoi (g_strdup (gtk_entry_get_text (GTK_ENTRY (precio_venta))));

    stock = GetCurrentStock (barcode);

    gtk_label_set_markup (GTK_LABEL (contri_proy),
			  g_strdup_printf ("<b>$%d</b>", contri_unit * stock));

    gtk_label_set_markup (GTK_LABEL (stock_valor),
			  g_strdup_printf ("<b>$%d</b>", stock * new_venta));
}

void
admini_box (GtkWidget *main_box)
{
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *box;
    GtkWidget *vbox2;
    GtkWidget *frame;

    GtkWidget *label;
    GtkWidget *button;
    GtkWidget *print_button;

    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;

    GSList *group_canj;
    GSList *group_mayor;

    GtkWidget *scroll;

    Print *print = (Print *) malloc (sizeof (Print));

    vbox = gtk_vbox_new (FALSE, 3);
    gtk_widget_show (vbox);
    if (solo_venta != TRUE)
	gtk_widget_set_size_request (vbox, MODULE_BOX_WIDTH - 5, -1);
    else
	gtk_widget_set_size_request (vbox, MODULE_LITTLE_BOX_WIDTH - 5, -1);
    gtk_box_pack_start (GTK_BOX (main_box), vbox, FALSE, FALSE, 0);

    /*
      Buscador
    */

    hbox = gtk_hbox_new (FALSE, 3);
    gtk_widget_show (hbox);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

    ingreso->buscar_entry = gtk_entry_new ();
    gtk_box_pack_start (GTK_BOX (hbox), ingreso->buscar_entry, FALSE, FALSE, 3);
    gtk_widget_show (ingreso->buscar_entry);

    g_signal_connect (G_OBJECT (ingreso->buscar_entry), "activate",
		      G_CALLBACK (BuscarProductosParaListar), NULL);

    button = gtk_button_new_from_stock (GTK_STOCK_FIND);
    gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 3);
    gtk_widget_show (button);

    g_signal_connect (G_OBJECT (button), "clicked",
		      G_CALLBACK (BuscarProductosParaListar), NULL);

    box = gtk_vbox_new (FALSE, 3);
    gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);
    label = gtk_label_new ("Stock Valorizado");
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
    gtk_widget_show (box);
    inv_total_stock = gtk_label_new ("");
    gtk_label_set_markup (GTK_LABEL (inv_total_stock),
			  g_strdup_printf ("<span foreground=\"blue\"><b>$%s</b></span>",
					   PutPoints (InversionTotalStock ())));
    gtk_box_pack_start (GTK_BOX (box), inv_total_stock, FALSE, FALSE, 0);
    gtk_widget_show (inv_total_stock);

    box = gtk_vbox_new (FALSE, 3);
    gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);
    label = gtk_label_new ("Valorizado de Venta");
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
    gtk_widget_show (box);
    valor_total_stock = gtk_label_new ("");
    gtk_label_set_markup (GTK_LABEL (valor_total_stock),
			  g_strdup_printf ("<span foreground=\"blue\"><b>$%s</b></span>",
					   PutPoints (ValorTotalStock ())));
    gtk_box_pack_start (GTK_BOX (box), valor_total_stock, FALSE, FALSE, 0);
    gtk_widget_show (valor_total_stock);

    box = gtk_vbox_new (FALSE, 3);
    gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);
    label = gtk_label_new ("Contribución Proyectada");
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
    gtk_widget_show (box);
    contri_total_stock = gtk_label_new ("");
    gtk_label_set_markup (GTK_LABEL (contri_total_stock),
			  g_strdup_printf ("<span foreground=\"blue\"><b>$%s</b></span>",
					   PutPoints (ContriTotalStock ())));
    gtk_box_pack_start (GTK_BOX (box), contri_total_stock, FALSE, FALSE, 0);
    gtk_widget_show (contri_total_stock);

    /*
      Caja Inferior con Lista de Productos
    */

    hbox = gtk_hbox_new (FALSE, 3);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
    gtk_widget_show (hbox);

    scroll = gtk_scrolled_window_new (NULL, NULL);
    gtk_widget_show (scroll);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
				    GTK_POLICY_AUTOMATIC,
				    GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start (GTK_BOX (hbox), scroll, FALSE, FALSE, 0);

    ingreso->store = gtk_list_store_new (10,
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

    //  ReturnProductsStore (ingreso->store);

    ingreso->treeview_products = gtk_tree_view_new_with_model (GTK_TREE_MODEL (ingreso->store));
    if (solo_venta != TRUE)
	gtk_widget_set_size_request (ingreso->treeview_products, MODULE_BOX_WIDTH - 10, 200);
    else
	gtk_widget_set_size_request (ingreso->treeview_products, MODULE_LITTLE_BOX_WIDTH - 5, 110);
    gtk_widget_show (ingreso->treeview_products);
    gtk_container_add (GTK_CONTAINER (scroll), ingreso->treeview_products);

    /* Ahora llenamos la struct con los datos necesarios para poder imprimir el treeview */
    print->tree = GTK_TREE_VIEW (ingreso->treeview_products);
    print->title = "Listado de Productos";
    print->date_string = NULL;
    print->cols[0].name = "Codigo";
    print->cols[1].name = "Codigo de Barras";
    print->cols[2].name = "Producto";
    print->cols[3].name = "Marca";
    print->cols[4].name = "Cantidad";
    print->cols[5].name = "Unidad";
    print->cols[6].name = "Stock";
    print->cols[7].name = "Precio";
    print->cols[8].name = NULL;

    ingreso->selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (ingreso->treeview_products));

    gtk_tree_selection_set_mode (ingreso->selection, GTK_SELECTION_SINGLE);

    g_signal_connect (G_OBJECT (ingreso->selection), "changed",
		      G_CALLBACK (FillFields), NULL);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("Código", renderer,
						       "text", 0,
						       "foreground", 8,
						       "foreground-set", 9,
						       NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (ingreso->treeview_products), column);
    gtk_tree_view_column_set_alignment (column, 0.5);
    g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
    gtk_tree_view_column_set_sort_column_id (column, 0);
    gtk_tree_view_column_set_resizable (column, FALSE);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("Código de Barras", renderer,
						       "text", 1,
						       "foreground", 8,
						       "foreground-set", 9,
						       NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (ingreso->treeview_products), column);
    gtk_tree_view_column_set_alignment (column, 0.5);
    g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
    gtk_tree_view_column_set_resizable (column, FALSE);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("Descripción", renderer,
						       "text", 2,
						       "foreground", 8,
						       "foreground-set", 9,
						       NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (ingreso->treeview_products), column);
    gtk_tree_view_column_set_alignment (column, 0.5);
    gtk_tree_view_column_set_min_width (column, 150);
    gtk_tree_view_column_set_max_width (column, 150);
    gtk_tree_view_column_set_resizable (column, FALSE);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("Marca", renderer,
						       "text", 3,
						       "foreground", 8,
						       "foreground-set", 9,
						       NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (ingreso->treeview_products), column);
    gtk_tree_view_column_set_alignment (column, 0.5);
    gtk_tree_view_column_set_min_width (column, 96);
    gtk_tree_view_column_set_max_width (column, 96);
    gtk_tree_view_column_set_resizable (column, FALSE);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("Cant.", renderer,
						       "text", 4,
						       "foreground", 8,
						       "foreground-set", 9,
						       NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (ingreso->treeview_products), column);
    gtk_tree_view_column_set_alignment (column, 0.5);
    g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
    gtk_tree_view_column_set_sort_column_id (column, 4);
    gtk_tree_view_column_set_min_width (column, 60);
    gtk_tree_view_column_set_max_width (column, 60);
    gtk_tree_view_column_set_resizable (column, FALSE);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("Unid", renderer,
						       "text", 5,
						       "foreground", 8,
						       "foreground-set", 9,
						       NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (ingreso->treeview_products), column);
    gtk_tree_view_column_set_alignment (column, 0.5);
    g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
    gtk_tree_view_column_set_min_width (column, 38);
    gtk_tree_view_column_set_max_width (column, 38);
    gtk_tree_view_column_set_resizable (column, FALSE);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("Stock", renderer,
						       "text", 6,
						       "foreground", 8,
						       "foreground-set", 9,
						       NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (ingreso->treeview_products), column);
    gtk_tree_view_column_set_alignment (column, 0.5);
    g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
    gtk_tree_view_column_set_sort_column_id (column, 6);
    gtk_tree_view_column_set_resizable (column, FALSE);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("Precio", renderer,
						       "text", 7,
						       "foreground", 8,
						       "foreground-set", 9,
						       NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (ingreso->treeview_products), column);
    gtk_tree_view_column_set_alignment (column, 0.5);
    g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
    gtk_tree_view_column_set_sort_column_id (column, 7);
    gtk_tree_view_column_set_resizable (column, FALSE);

    hbox = gtk_hbox_new (FALSE, 3);
    gtk_widget_show (hbox);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

    label = gtk_label_new ("");
    gtk_label_set_markup (GTK_LABEL (label),
			  "<b>Se han encontrado:</b> ");
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
    gtk_widget_show (label);

    label_found = gtk_label_new ("");
    gtk_box_pack_start (GTK_BOX (hbox), label_found, FALSE, FALSE, 3);
    gtk_widget_show (label_found);

    /* Descripcion Mercaderia */

    frame = gtk_frame_new ("Ingresar de Productos");
    gtk_widget_show (frame);
    gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);

    vbox2 = gtk_vbox_new (FALSE, 0);
    gtk_widget_show (vbox2);
    gtk_container_add (GTK_CONTAINER (frame), vbox2);

    hbox = gtk_hbox_new (FALSE, 0);
    gtk_widget_show (hbox);
    gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);

    box = gtk_vbox_new (FALSE, 3);
    gtk_widget_show (box);
    gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);
    label = gtk_label_new ("Código de Barras");
    gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
    barcode_entry = gtk_label_new ("");
    gtk_widget_set_size_request (GTK_WIDGET (barcode_entry), 110, -1);
    gtk_box_pack_start (GTK_BOX (box), barcode_entry, FALSE, FALSE, 0);
    gtk_widget_show (barcode_entry);

    box = gtk_vbox_new (FALSE, 3);
    gtk_widget_show (box);
    gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);
    label = gtk_label_new ("Código Simple");
    gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
    ingreso->codigo_entry = gtk_label_new ("");
    gtk_box_pack_start (GTK_BOX (box), ingreso->codigo_entry, FALSE, FALSE, 0);
    gtk_widget_show (ingreso->codigo_entry);

    box = gtk_vbox_new (FALSE, 3);
    gtk_widget_show (box);
    gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);
    label = gtk_label_new ("Descripción");
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
    ingreso->product_entry = gtk_label_new ("");
    gtk_widget_set_size_request (GTK_WIDGET (ingreso->product_entry), 150, -1);
    gtk_box_pack_start (GTK_BOX (box), ingreso->product_entry, FALSE, FALSE, 0);
    gtk_widget_show (ingreso->product_entry);

    box = gtk_vbox_new (FALSE, 3);
    gtk_widget_show (box);
    gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);
    label = gtk_label_new ("Marca");
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
    ingreso->marca_entry = gtk_label_new("");
    gtk_widget_set_size_request (GTK_WIDGET (ingreso->marca_entry), 120, -1);
    gtk_box_pack_start (GTK_BOX (box), ingreso->marca_entry, FALSE, FALSE, 0);
    gtk_widget_show (ingreso->marca_entry);

    /*  hbox = gtk_hbox_new (FALSE, 3);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 3);
    */
    box = gtk_vbox_new (FALSE, 3);
    gtk_widget_show (box);
    gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);
    label = gtk_label_new ("Contenido: ");
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
    ingreso->cantidad_entry = gtk_label_new ("");
    gtk_widget_set_size_request (GTK_WIDGET (ingreso->cantidad_entry), 70, -1);
    gtk_box_pack_start (GTK_BOX (box), ingreso->cantidad_entry, FALSE, FALSE, 0);
    gtk_widget_show (ingreso->cantidad_entry);

    box = gtk_vbox_new (FALSE, 3);
    gtk_widget_show (box);
    gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);
    label = gtk_label_new ("Unidad");
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
    ingreso->unidad_entry = gtk_label_new ("");
    gtk_widget_set_size_request (GTK_WIDGET (ingreso->unidad_entry), 50, -1);
    gtk_box_pack_start (GTK_BOX (box), ingreso->unidad_entry, FALSE, FALSE, 0);
    gtk_widget_show (ingreso->unidad_entry);

    hbox = gtk_hbox_new (FALSE, 0);
    gtk_widget_show (hbox);
    gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);

    box = gtk_vbox_new (FALSE, 3);
    gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);
    label = gtk_label_new ("Impuesto Adicional");
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
    gtk_widget_show (box);
    impuesto_adic = gtk_label_new ("");
    gtk_misc_set_alignment (GTK_MISC (impuesto_adic), 0.5, 0.5);
    gtk_widget_set_size_request (impuesto_adic, 130, -1);
    gtk_box_pack_start (GTK_BOX (box), impuesto_adic, FALSE, FALSE, 0);
    gtk_widget_show (impuesto_adic);

    /*
      box = gtk_vbox_new (FALSE, 3);
      gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);
      label = gtk_label_new ("Familia");
      gtk_widget_show (label);
      gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
      gtk_widget_show (box);
      familia = gtk_label_new ("");
      gtk_widget_set_size_request (familia, 110, -1);
      gtk_box_pack_start (GTK_BOX (box), familia, FALSE, FALSE, 0);
      gtk_widget_show (familia);
    */
    box = gtk_vbox_new (FALSE, 3);
    gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);
    label = gtk_label_new ("Tipo");
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
    gtk_widget_show (box);
    perecible = gtk_label_new ("");
    gtk_widget_set_size_request (perecible, 80, -1);
    gtk_box_pack_start (GTK_BOX (box), perecible, FALSE, FALSE, 0);
    gtk_widget_show (perecible);

    box = gtk_vbox_new (FALSE, 3);
    gtk_widget_show (box);
    gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);
    label = gtk_label_new ("T. Vendido");
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
    total_unidades_vendidas = gtk_label_new ("");
    gtk_widget_show (total_unidades_vendidas);
    gtk_widget_set_size_request (GTK_WIDGET (total_unidades_vendidas), 50, -1);
    gtk_box_pack_start (GTK_BOX (box), total_unidades_vendidas, FALSE, FALSE, 0);

    box = gtk_vbox_new (FALSE, 3);
    gtk_widget_show (box);
    gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 0);
    label = gtk_label_new ("Stock");
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
    ingreso->stock_entry = gtk_label_new ("");
    gtk_widget_show (ingreso->stock_entry);
    gtk_widget_set_size_request (GTK_WIDGET (ingreso->stock_entry), 40, -1);
    gtk_box_pack_start (GTK_BOX (box), ingreso->stock_entry, FALSE, FALSE, 0);

    box = gtk_vbox_new (FALSE, 3);
    gtk_widget_show (box);
    gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);
    label = gtk_label_new ("Ventas/Día ");
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
    ventas_dias = gtk_label_new ("");
    gtk_widget_show (ventas_dias);
    gtk_widget_set_size_request (GTK_WIDGET (ventas_dias), 40, -1);
    gtk_box_pack_start (GTK_BOX (box), ventas_dias, FALSE, FALSE, 0);

    box = gtk_vbox_new (FALSE, 3);
    gtk_widget_show (box);
    gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 0);
    label = gtk_label_new ("Stock Dias");
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
    stock_dias = gtk_label_new ("");
    gtk_widget_show (stock_dias);
    gtk_widget_set_size_request (GTK_WIDGET (stock_dias), 40, -1);
    gtk_box_pack_start (GTK_BOX (box), stock_dias, FALSE, FALSE, 0);

    box = gtk_vbox_new (FALSE, 3);
    gtk_widget_show (box);
    gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);
    label = gtk_label_new ("Stock M.");
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
    stock_min = gtk_entry_new_with_max_length (10);
    gtk_widget_show (stock_min);
    gtk_widget_set_size_request (GTK_WIDGET (stock_min), 40, -1);
    gtk_box_pack_start (GTK_BOX (box), stock_min, FALSE, FALSE, 0);

    hbox = gtk_hbox_new (FALSE, 3);
    gtk_widget_show (hbox);
    gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 3);

    box = gtk_vbox_new (FALSE, 3);
    gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);
    label = gtk_label_new ("Costo Promedio");
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
    gtk_widget_show (box);
    costo_promedio = gtk_label_new ("");
    gtk_widget_set_size_request (costo_promedio, 50, -1);
    gtk_box_pack_start (GTK_BOX (box), costo_promedio, FALSE, FALSE, 0);
    gtk_widget_show (costo_promedio);

    box = gtk_vbox_new (FALSE, 3);
    gtk_widget_show (box);
    gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);
    label = gtk_label_new ("Margen %");
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
    margen_entry = gtk_entry_new_with_max_length (10);
    gtk_entry_set_alignment (GTK_ENTRY (margen_entry), 1);
    gtk_widget_set_size_request (GTK_WIDGET (margen_entry), 50, -1);
    gtk_box_pack_start (GTK_BOX (box), margen_entry, FALSE, FALSE, 0);
    gtk_widget_show (margen_entry);

    g_signal_connect (G_OBJECT (margen_entry), "activate",
		      G_CALLBACK (ModificarMargenVenta), (gpointer)TRUE);

    box = gtk_vbox_new (FALSE, 3);
    gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);
    label = gtk_label_new ("Contrib Unit.");
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
    gtk_widget_show (box);
    contrib_unit = gtk_label_new ("");
    gtk_widget_set_size_request (contrib_unit, 50, -1);
    gtk_box_pack_start (GTK_BOX (box), contrib_unit, FALSE, FALSE, 0);
    gtk_widget_show (contrib_unit);

    box = gtk_vbox_new (FALSE, 3);
    gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);
    label = gtk_label_new ("Precio Venta");
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
    gtk_widget_show (box);
    precio_venta = gtk_entry_new_with_max_length (10);
    gtk_entry_set_alignment (GTK_ENTRY (precio_venta), 1);
    gtk_widget_set_size_request (precio_venta, 50, -1);
    gtk_box_pack_start (GTK_BOX (box), precio_venta, FALSE, FALSE, 0);
    gtk_widget_show (precio_venta);

    g_signal_connect (G_OBJECT (precio_venta), "activate",
		      G_CALLBACK (ModificarMargenVenta), (gpointer)FALSE);

    box = gtk_vbox_new (FALSE, 3);
    gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);
    label = gtk_label_new ("Inversión Stock");
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
    gtk_widget_show (box);
    stock_valor = gtk_label_new ("");
    gtk_widget_set_size_request (stock_valor, 50, -1);
    gtk_box_pack_start (GTK_BOX (box), stock_valor, FALSE, FALSE, 0);
    gtk_widget_show (stock_valor);

    box = gtk_vbox_new (FALSE, 3);
    gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);
    label = gtk_label_new ("Merma Unid.");
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
    gtk_widget_show (box);
    mermita = gtk_label_new ("");
    gtk_widget_set_size_request (mermita, 50, -1);
    gtk_box_pack_start (GTK_BOX (box), mermita, FALSE, FALSE, 0);
    gtk_widget_show (mermita);

    box = gtk_vbox_new (FALSE, 3);
    gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);
    gtk_widget_show (box);
    label = gtk_label_new ("Merma %");
    gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
    gtk_widget_show (label);
    mermata = gtk_label_new ("");
    gtk_widget_set_size_request (mermata, 50, -1);
    gtk_box_pack_start (GTK_BOX (box), mermata, FALSE, FALSE, 0);
    gtk_widget_show (mermata);

    box = gtk_vbox_new (FALSE, 3);
    gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);
    label = gtk_label_new ("ICI");
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
    gtk_widget_show (box);
    ici = gtk_label_new ("");
    gtk_widget_set_size_request (ici, 50, -1);
    gtk_box_pack_start (GTK_BOX (box), ici, FALSE, FALSE, 0);
    gtk_widget_show (ici);

    hbox = gtk_hbox_new (FALSE, 3);
    gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 3);
    gtk_widget_show (hbox);

    box = gtk_vbox_new (FALSE, 3);
    gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);
    label = gtk_label_new ("Compras Totales");
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
    gtk_widget_show (box);
    comp_totales = gtk_label_new ("");
    gtk_widget_set_size_request (comp_totales, 50, -1);
    gtk_box_pack_start (GTK_BOX (box), comp_totales, FALSE, FALSE, 0);
    gtk_widget_show (comp_totales);

    box = gtk_vbox_new (FALSE, 3);
    gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);
    label = gtk_label_new ("Total Vendido $");
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
    gtk_widget_show (box);
    total_vendido = gtk_label_new ("");
    gtk_widget_set_size_request (total_vendido, 50, -1);
    gtk_box_pack_start (GTK_BOX (box), total_vendido, FALSE, FALSE, 0);
    gtk_widget_show (total_vendido);

    box = gtk_vbox_new (FALSE, 3);
    gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);
    label = gtk_label_new ("Indice T");
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
    gtk_widget_show (box);
    indice_t = gtk_label_new ("");
    gtk_widget_set_size_request (indice_t, 50, -1);
    gtk_box_pack_start (GTK_BOX (box), indice_t, FALSE, FALSE, 0);
    gtk_widget_show (indice_t);

    box = gtk_vbox_new (FALSE, 3);
    gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);
    label = gtk_label_new ("Contrib Agreg.");
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
    gtk_widget_show (box);
    contri_agr = gtk_label_new ("");
    gtk_widget_set_size_request (contri_agr, 50, -1);
    gtk_box_pack_start (GTK_BOX (box), contri_agr, FALSE, FALSE, 0);
    gtk_widget_show (contri_agr);

    box = gtk_vbox_new (FALSE, 3);
    gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);
    label = gtk_label_new ("Contrib. Proyect.");
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
    gtk_widget_show (box);
    contri_proy = gtk_label_new ("");
    gtk_widget_set_size_request (contri_proy, 50, -1);
    gtk_box_pack_start (GTK_BOX (box), contri_proy, FALSE, FALSE, 0);
    gtk_widget_show (contri_proy);

    box = gtk_vbox_new (FALSE, 3);
    gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);
    label = gtk_label_new ("Fecha Elab.");
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
    gtk_widget_show (box);
    elab_date = gtk_label_new ("");
    gtk_widget_set_size_request (elab_date, 50, -1);
    gtk_box_pack_start (GTK_BOX (box), elab_date, FALSE, FALSE, 0);
    gtk_widget_show (elab_date);

    box = gtk_vbox_new (FALSE, 6);
    gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);
    label = gtk_label_new ("Fecha Venc.");
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
    gtk_widget_show (box);
    venc_date = gtk_label_new ("");
    gtk_widget_set_size_request (venc_date, 50, -1);
    gtk_box_pack_start (GTK_BOX (box), venc_date, FALSE, FALSE, 0);
    gtk_widget_show (venc_date);

    hbox = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);
    gtk_widget_show (hbox);

    box = gtk_vbox_new (FALSE, 3);
    gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 2);
    label = gtk_label_new ("Canjeable");
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
    gtk_widget_show (box);

    vbox = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (box), vbox, FALSE, FALSE, 0);
    gtk_widget_show (vbox);

    canje_buttons_f = gtk_radio_button_new_with_label (NULL, "No");
    gtk_box_pack_end (GTK_BOX (vbox), canje_buttons_f, FALSE, FALSE, 2);
    gtk_widget_show (canje_buttons_f);

    //  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (canje_buttons), FALSE);

    group_canj = gtk_radio_button_get_group (GTK_RADIO_BUTTON (canje_buttons_f));
    canje_buttons_t = gtk_radio_button_new_with_label (group_canj, "Sí");
    gtk_box_pack_end (GTK_BOX (vbox), canje_buttons_t, FALSE, FALSE, 2);
    gtk_widget_show (canje_buttons_t);

    box = gtk_vbox_new (FALSE, 3);
    gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 2);
    label = gtk_label_new ("Stock Pro.");
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
    gtk_widget_show (box);
    stock_pro = gtk_label_new ("");
    gtk_widget_set_size_request (stock_pro, 50, -1);
    gtk_box_pack_start (GTK_BOX (box), stock_pro, FALSE, FALSE, 0);
    gtk_widget_show (stock_pro);

    box = gtk_vbox_new (FALSE, 3);
    gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 2);
    label = gtk_label_new ("Tasa de canje");
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
    gtk_widget_show (box);
    tasa_canje = gtk_entry_new ();
    gtk_widget_set_size_request (tasa_canje, 50, -1);
    gtk_box_pack_start (GTK_BOX (box), tasa_canje, FALSE, FALSE, 0);
    gtk_widget_show (tasa_canje);

    gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);
    gtk_widget_show (hbox);

    box = gtk_vbox_new (FALSE, 3);
    gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 2);
    label = gtk_label_new ("Mayorista");
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
    gtk_widget_show (box);

    vbox = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (box), vbox, FALSE, FALSE, 0);
    gtk_widget_show (vbox);

    mayor_buttons_f = gtk_radio_button_new_with_label (NULL, "No");
    gtk_box_pack_end (GTK_BOX (vbox), mayor_buttons_f, FALSE, FALSE, 2);
    gtk_widget_show (mayor_buttons_f);

    group_mayor = gtk_radio_button_get_group (GTK_RADIO_BUTTON (mayor_buttons_f));
    mayor_buttons_t = gtk_radio_button_new_with_label (group_mayor, "Sí");
    gtk_box_pack_end (GTK_BOX (vbox), mayor_buttons_t, FALSE, FALSE, 2);
    gtk_widget_show (mayor_buttons_t);

    box = gtk_vbox_new (FALSE, 3);
    gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 2);
    label = gtk_label_new ("Cant. Mayor.");
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
    gtk_widget_show (box);
    mayor_cantidad = gtk_entry_new ();
    gtk_widget_set_size_request (mayor_cantidad, 50, -1);
    gtk_box_pack_start (GTK_BOX (box), mayor_cantidad, FALSE, FALSE, 0);
    gtk_widget_show (mayor_cantidad);

    gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);
    gtk_widget_show (hbox);

    box = gtk_vbox_new (FALSE, 3);
    gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 2);
    label = gtk_label_new ("Precio Mayor.");
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
    gtk_widget_show (box);
    mayor_precio = gtk_entry_new ();
    gtk_widget_set_size_request (mayor_precio, 50, -1);
    gtk_box_pack_start (GTK_BOX (box), mayor_precio, FALSE, FALSE, 0);
    gtk_widget_show (mayor_precio);

    gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);
    gtk_widget_show (hbox);

    box = gtk_vbox_new (FALSE, 2);
    gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 2);
    label = gtk_label_new ("Proveedores");
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
    gtk_widget_show (box);
    combo_proveedores = gtk_combo_box_new_text ();
    gtk_widget_set_size_request (combo_proveedores, 50, -1);
    gtk_box_pack_start (GTK_BOX (box), combo_proveedores, FALSE, FALSE, 0);
    gtk_widget_show (combo_proveedores);

    hbox = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);
    gtk_widget_show (hbox);

    button = gtk_button_new_with_label ("Devolución");
    gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
    gtk_widget_show (button);

    g_signal_connect (G_OBJECT (button), "clicked",
		      G_CALLBACK (DevolucionWindow), NULL);

    button = gtk_button_new_with_label ("Recibir");
    gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
    gtk_widget_show (button);

    g_signal_connect (G_OBJECT (button), "clicked",
		      G_CALLBACK (RecivirWindow), NULL);

    /*
      Bottom buttons
    */

    hbox = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);
    gtk_widget_show (hbox);

    /* Separamos el boton de guardar con 8 pixeles de la orilla*/
    /* es una gtk_box vacia*/
    /*  if (solo_venta == TRUE)
	{
	box = gtk_vbox_new (FALSE, 6);
	gtk_box_pack_end (GTK_BOX (hbox), box, FALSE, FALSE, 8);
	gtk_widget_show (box);
	}
    */

    button = gtk_button_new_from_stock (GTK_STOCK_SAVE);
    gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);
    gtk_widget_show (button);

    g_signal_connect (G_OBJECT (button), "clicked",
		      G_CALLBACK (GuardarModificacionesProducto), NULL);

    button = gtk_button_new_from_stock (GTK_STOCK_DELETE);
    gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);
    gtk_widget_show (button);

    g_signal_connect (G_OBJECT (button), "clicked",
		      G_CALLBACK (EliminarProductoDB), NULL);

    button = gtk_button_new_with_label ("Ajuste");
    gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);
    gtk_widget_show (button);

    g_signal_connect (G_OBJECT (button), "clicked",
		      G_CALLBACK (AjusteWin), NULL);

    print_button = gtk_button_new_from_stock (GTK_STOCK_PRINT);
    gtk_box_pack_end (GTK_BOX (hbox), print_button, FALSE, FALSE, 3);
    gtk_widget_show (print_button);

    g_signal_connect (G_OBJECT (print_button), "clicked",
		      G_CALLBACK (PrintTree), (gpointer)print);

    button = gtk_button_new_with_label ("Modificar");
    gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);
    gtk_widget_show (button);

    g_signal_connect (G_OBJECT (button), "clicked",
		      G_CALLBACK (ModificarProducto), NULL);
}

void
Ingresar_Producto (gpointer data)
{
    gchar *sentencia, *q;
    PGresult *res;
    gchar *codigo = g_strdup (gtk_entry_get_text (GTK_ENTRY (ingreso->codigo_entry)));
    gchar *product = g_strdup (gtk_entry_get_text (GTK_ENTRY (ingreso->product_entry)));
    gchar *precio = g_strdup (gtk_entry_get_text (GTK_ENTRY (ingreso->precio_entry)));

    q = g_strdup_printf ("SELECT existe_producto('%s')", codigo);
    res = EjecutarSQL(q);
    if (g_str_equal (PQgetvalue (res,0,0), "t"))
	ErrorMSG (ingreso->codigo_entry, "Ya existe un producto con el mismo código!");
    else
    {
	g_free (q);
	q = g_strdup_printf ("SELECT existe_producto(%s)", product);
	res = EjecutarSQL(q);
	if (g_str_equal(PQgetvalue (res,0,0), "t"))
	    ErrorMSG (ingreso->product_entry, "Ya existe un producto con el mismo nombre!");
	else
	{
	    sentencia = g_strdup_printf("SELECT insert_producto(%s,'%s',%s)",
					product,codigo,precio);
	    EjecutarSQL (sentencia);
	    g_free (sentencia);
	    gtk_entry_set_text (GTK_ENTRY (ingreso->codigo_entry), "");
	    gtk_entry_set_text (GTK_ENTRY (ingreso->product_entry), "");
	    gtk_entry_set_text (GTK_ENTRY (ingreso->precio_entry), "");
	}
    }
    g_free (q);
}


gint
ReturnProductsStore (GtkListStore *store)
{
    gint tuples, i;

    GtkTreeIter iter;
    PGresult *res;

    // saca todos los productos dese la base de datos
    //TODO: implementar conexion asincrónica para traer grandes
    //cantidades de informacion, y mediante el uso de Threads

    res = EjecutarSQL ("SELECT * FROM select_producto()");

    tuples = PQntuples (res);

    if (tuples == 1)
	gtk_label_set_markup (GTK_LABEL (label_found),
			      g_strdup_printf ("<b>%d producto</b>", tuples));
    else if (tuples == 0)
	gtk_label_set_markup (GTK_LABEL (label_found),"<b>Sin Productos</b>");
    else
	gtk_label_set_markup (GTK_LABEL (label_found),
			      g_strdup_printf ("<b>%d productos</b>", tuples));

    gtk_list_store_clear (store);

    for (i = 0; i < tuples; i++)
    {
	gtk_list_store_append (store, &iter);
	gtk_list_store_set (store, &iter,
			    0, PQvaluebycol( res, i, "codigo_corto" ),
			    1, PQvaluebycol( res, i, "barcode" ),
			    2, PQvaluebycol( res, i, "descripcion" ),
			    3, PQvaluebycol( res, i, "marca" ),
			    4, atoi (PQvaluebycol( res, i, "contenido" )),
			    5, PQvaluebycol( res, i, "unidad" ),
			    6, atoi (PQvaluebycol( res, i, "stock" )),
			    7, atoi (PQvaluebycol( res, i, "precio" )),
			    8, (atoi (PQvaluebycol (res, i, "stock")) <= atoi (PQvaluebycol (res, i, "stock_min")) &&
				atoi (PQvaluebycol (res, i, "stock_min")) != 0) ? "Red" : "Black",
			    9, TRUE,
			    -1);
    }

    return 0;
}

void
FillEditFields (GtkTreeSelection *selection, gpointer data)
{
    GtkTreeIter iter;
    gchar *product, *codigo;
    gint precio;

    if (Deleting != TRUE)
    {
	gtk_tree_selection_get_selected (ingreso->selection, NULL, &iter);

	gtk_tree_model_get (GTK_TREE_MODEL (ingreso->store), &iter,
			    0, &codigo,
			    2, &product,
			    7, &precio,
			    -1);

	gtk_entry_set_text (GTK_ENTRY (ingreso->codigo_entry_edit), g_strdup (codigo));
	gtk_entry_set_text (GTK_ENTRY (ingreso->product_entry_edit), g_strdup (product));
	gtk_entry_set_text (GTK_ENTRY (ingreso->precio_entry_edit), g_strdup_printf ("%d", precio));
    }
}

void
FillFields(GtkTreeSelection *selection, gpointer data)
{
    GtkTreeIter iter;
    PGresult *res;
    gchar *barcode;
    gint vendidos;
    gdouble stock;
    gint i, tuples;
    gint compras_totales;
    gdouble ici_total, merma;
    gdouble mermaporc;
    gint contri_unit;
    gint contrib_agreg;
    gint contrib_proyect;
    gint valor_stock;
    gint margen, fifo;
    gint cantidad_mayorista, precio_mayorista;
    gchar *q;

    if (gtk_tree_selection_get_selected (selection, NULL, &iter) == TRUE)
    {
	gtk_tree_model_get (GTK_TREE_MODEL (ingreso->store), &iter,
			    1, &barcode,
			    -1);

	q = g_strdup_printf ("SELECT * FROM informacion_producto( %s )", barcode);
	res = EjecutarSQL(q);
	g_free(q);

	stock = strtod (PUT (PQvaluebycol( res, 0, "stock")), (char **)NULL);

	margen = atoi (PQvaluebycol (res, 0, "margen_promedio"));

	merma = (gdouble) atoi (PQvaluebycol (res, 0, "merma_unid"));

	fifo = atoi (PQvaluebycol (res, 0, "costo_promedio"));

	contri_unit = lround ((gdouble)fifo * (gdouble)margen / 100);

	contrib_agreg = atoi (PQvaluebycol (res, 0, "contrib_agregada"));

	compras_totales = GetTotalBuys (barcode);

	vendidos = atoi (PQvaluebycol (res, 0, "vendidos"));

	if (merma != 0)
	    mermaporc =  (gdouble)(merma / (stock + vendidos + merma)) *  100;
	else
	    mermaporc = 0;

	//      contrib_proyect = (gdouble)((fifo * margen) / 100) * stock;
	contrib_proyect = contri_unit * stock;

	if (contrib_agreg != 0)
	    ici_total = (gdouble) contrib_agreg / InversionAgregada (barcode);
	else
	    ici_total = 0;

	valor_stock = fifo * stock;

	precio_mayorista = atoi (PQvaluebycol (res, 0, "precio_mayor"));

	cantidad_mayorista = atoi (PQvaluebycol (res, 0, "cantidad_mayor"));

	gtk_list_store_set (GTK_LIST_STORE (ingreso->store), &iter,
			    6, atoi (PQvaluebycol (res, 0, "stock")),
			    7, atoi (PQvaluebycol (res, 0, "margen_promedio")),
			    8, (stock <= atoi (PQvaluebycol (res, 0, "stock_min")) &&
				atoi (PQvaluebycol (res, 0, "stock_min")) != 0) ? "Red" : "Black",
			    -1);


	gtk_label_set_markup (GTK_LABEL (barcode_entry),
			      g_strdup_printf ("<b>%s</b>", barcode));

	gtk_label_set_markup (GTK_LABEL (ingreso->codigo_entry),
			      g_strdup_printf ("<b>%s</b>", PQvaluebycol (res, 0, "codigo_corto")));
	gtk_label_set_markup (GTK_LABEL (ingreso->product_entry),
			      g_strdup_printf ("<b>%s</b>", PQvaluebycol (res, 0, "descripcion")));
	gtk_label_set_markup (GTK_LABEL (ingreso->marca_entry),
			      g_strdup_printf ("<b>%s</b>", PQvaluebycol (res, 0, "marca")));
	gtk_label_set_markup (GTK_LABEL (ingreso->cantidad_entry),
			      g_strdup_printf ("<b>%s</b>", PQvaluebycol (res, 0, "contenido")));
	gtk_label_set_markup (GTK_LABEL (ingreso->unidad_entry),
			      g_strdup_printf ("<b>%s</b>", PQvaluebycol (res, 0, "unidad")));

	gtk_label_set_markup (GTK_LABEL (total_unidades_vendidas),
			      g_strdup_printf ("<b>%d</b>", vendidos));

	gtk_label_set_markup (GTK_LABEL (ingreso->stock_entry),
			      g_strdup_printf ("<b>%.2f</b>", stock));

	gtk_label_set_markup (GTK_LABEL (ventas_dias),
			      g_strdup_printf ("<b>%.2f</b>", strtod (PUT (PQvaluebycol (res, 0, "unidades_merma")), (char **)NULL)));

	gtk_label_set_markup (GTK_LABEL (stock_dias),
			      g_strdup_printf ("<b>%.2f</b>", GetDayToSell (barcode)));

	gtk_entry_set_text (GTK_ENTRY (stock_min), PQvaluebycol (res, 0, "stock_min"));

	gtk_label_set_markup (GTK_LABEL (costo_promedio),
			      g_strdup_printf ("<b>%d</b>", fifo));

	gtk_label_set_markup (GTK_LABEL (impuesto_adic),
			      g_strdup_printf ("<b>%s</b>", GetLabelImpuesto (barcode)));
	gtk_misc_set_alignment (GTK_MISC (impuesto_adic), 0.5, 0.5);

	gtk_label_set_markup (GTK_LABEL (perecible),
			      g_strdup_printf ("<b>%s</b>", GetPerecible (barcode)));

	gtk_entry_set_text (GTK_ENTRY (margen_entry),
			    g_strdup_printf ("%d", margen));

	gtk_label_set_markup (GTK_LABEL (contrib_unit),
			      g_strdup_printf ("<b>$%d</b>", contri_unit));

	gtk_entry_set_text (GTK_ENTRY (precio_venta), PQvaluebycol (res, 0, "precio"));

	gtk_label_set_markup (GTK_LABEL (stock_valor),
			      g_strdup_printf ("<b>$%d</b>", valor_stock));

	gtk_label_set_markup (GTK_LABEL (mermita),
			      g_strdup_printf ("<b>%.2f</b>", merma));

	gtk_label_set_markup
	    (GTK_LABEL (mermata), g_strdup_printf ("<b>%.2f %%</b>", mermaporc));

	gtk_label_set_markup (GTK_LABEL (ici),
			      g_strdup_printf ("<b>%.2f%%</b>", ici_total));

	gtk_label_set_markup (GTK_LABEL (comp_totales),
			      g_strdup_printf ("<b>$%d</b>", compras_totales));

	if(strcmp (PQvaluebycol (res, 0, "contrib_agregada"), "") == 0)
	    gtk_label_set_markup (GTK_LABEL (total_vendido), "");
	else
	    gtk_label_set_markup (GTK_LABEL (total_vendido),
				  g_strdup_printf ("<b>$%s</b>", PQvaluebycol (res, 0, "vendidos")));

	gtk_label_set_markup (GTK_LABEL (contri_agr),
			      g_strdup_printf ("<b>$%d</b>", contrib_agreg));

	gtk_label_set_markup (GTK_LABEL (contri_proy),
			      g_strdup_printf ("<b>$%d</b>", contrib_proyect));

	gtk_label_set_markup (GTK_LABEL (elab_date),
			      g_strdup_printf ("<b>%s</b>", GetElabDate (barcode, stock)));

	gtk_label_set_markup (GTK_LABEL (venc_date),
			      g_strdup_printf ("<b>%s</b>", GetVencDate (barcode, stock)));

	if (strcmp (PQvaluebycol (res, 0, "canje"), "t") == 0)
	{
	    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (canje_buttons_t), TRUE);
	    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (canje_buttons_f), FALSE);
	}
	else
	{
	    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (canje_buttons_t), FALSE);
	    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (canje_buttons_f), TRUE);
	}

	gtk_label_set_markup (GTK_LABEL (stock_pro),
			      g_strdup_printf ("<b>%s</b>", PQvaluebycol (res, 0, "stock_pro")));

	gtk_entry_set_text (GTK_ENTRY (tasa_canje), PQvaluebycol (res, 0, "tasa_canje"));



	if (strcmp (PQvaluebycol (res, 0, "mayorista"), "t") == 0)
	{
	    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (mayor_buttons_t), TRUE);
	    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (mayor_buttons_f), FALSE);
	}
	else
	{
	    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (mayor_buttons_t), FALSE);
	    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (mayor_buttons_f), TRUE);
	}

	q = g_strdup_printf("SELECT nombre FROM select_proveedor_for_product(%s)",
			    barcode);
	res = EjecutarSQL (q);
	g_free(q);

	tuples = PQntuples (res);

	model_proveedores = gtk_list_store_new (1,
						G_TYPE_STRING);

	gtk_combo_box_set_model (GTK_COMBO_BOX (combo_proveedores),
				 GTK_TREE_MODEL (model_proveedores));

	for (i = 0; i < tuples; i++)
	    gtk_combo_box_append_text (GTK_COMBO_BOX (combo_proveedores),
				       PQvaluebycol (res, i, "nombre"));

	gtk_combo_box_set_active (GTK_COMBO_BOX (combo_proveedores), 0);

	gtk_entry_set_text (GTK_ENTRY (mayor_cantidad), g_strdup_printf ("%d", cantidad_mayorista));
	gtk_entry_set_text (GTK_ENTRY (mayor_precio), g_strdup_printf ("%d", precio_mayorista));

    }
}

void
EliminarProductoDB (GtkButton *button, gpointer data)
{
    GtkTreeIter iter;
    GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (ingreso->treeview_products));
    gchar *codigo;
    gint stock;

    if (gtk_tree_selection_get_selected (selection, NULL, &iter) == TRUE)
    {
	Deleting = TRUE;

	gtk_tree_model_get (GTK_TREE_MODEL (ingreso->store), &iter,
			    0, &codigo,
			    6, &stock,
			    -1);

	if (stock == 0)
	{
	    gtk_list_store_remove (GTK_LIST_STORE (ingreso->store), &iter);

	    DeleteProduct (codigo);
	}
	else
	    ErrorMSG (GTK_WIDGET (selection), "Solo se puede eliminar productos \n con stock mayor a 0");
	Deleting = FALSE;
    }
}

void
CloseProductWindow (void)
{
    gtk_widget_destroy (ingreso->products_window);

    ingreso->products_window = NULL;

    gtk_widget_set_sensitive (main_window, TRUE);
}

void
SaveChanges (void)
{
    gchar *product = g_strdup (gtk_entry_get_text (GTK_ENTRY (ingreso->product_entry_edit)));
    gchar *codigo = g_strdup (gtk_entry_get_text (GTK_ENTRY (ingreso->codigo_entry_edit)));
    gchar *barcode = NULL;
    gint precio = atoi (g_strdup (gtk_entry_get_text (GTK_ENTRY (ingreso->precio_entry_edit))));

    GtkTreeIter iter;

    if (strcmp (codigo, "") != 0)
    {
	if (gtk_tree_selection_get_selected (ingreso->selection, NULL, &iter) == TRUE)
	{
	    gtk_tree_model_get (GTK_TREE_MODEL (ingreso->store), &iter,
				1, &barcode,
				-1);

	    if (DataProductUpdate (barcode, codigo, product, precio) == TRUE)
	    {
		gtk_list_store_set (ingreso->store, &iter,
				    7, precio,
				    -1);

		ExitoMSG (ingreso->product_entry, "Se actualizaron los datos con exito!");
	    }
	    else
		ErrorMSG (ingreso->product_entry, "No se pudieron actualizar los datos!!");
	}
    }
}

void
BuscarProductosParaListar (void)
{
    PGresult *res;
    gchar *q;
    gchar *string;
    gint i, resultados;
    GtkTreeIter iter;

    string = g_strdup (gtk_entry_get_text (GTK_ENTRY (ingreso->buscar_entry)));
    q = g_strdup_printf( "SELECT * FROM buscar_producto( '%s', '{\"barcode\", \"codigo_corto\",\"marca\",\"descripcion\"}', true )", string);

    res = EjecutarSQL (q);
    g_free (q);

    resultados = PQntuples (res);

    gtk_label_set_markup (GTK_LABEL (label_found),
			  g_strdup_printf ("<b>%d producto(s)</b>", resultados));

    gtk_list_store_clear (ingreso->store);

    for (i = 0; i < resultados; i++)
    {
	gtk_list_store_append (ingreso->store, &iter);
	gtk_list_store_set (ingreso->store, &iter,
			    0, PQvaluebycol (res, i, "barcode"),
			    1, PQvaluebycol (res, i, "codigo_corto"),
			    2, PQvaluebycol (res, i, "marca"),
			    3, PQvaluebycol (res, i, "descripcion"),
			    4, atoi (PQvaluebycol (res, i, "contenido")),
			    5, PQvaluebycol (res, i, "unidad"),
			    6, atoi (PQvaluebycol (res, i, "stock")),
			    7, atoi (PQvaluebycol (res, i, "precio")),
			    8, (atoi (PQvaluebycol (res, i, "stock")) <= atoi (PQvaluebycol (res, i, "stock_min")) &&
				atoi (PQvaluebycol (res, i, "stock_min")) != 0) ? "Red" : "Black",
			    9, TRUE,
			    -1);
    }
}

void
ModificarProducto (void)
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
    GtkTreeIter iter;
    gint active;

    PGresult *res;
    gint tuples, i;
    GSList *group;

    gtk_widget_set_sensitive (main_window, FALSE);

    if (gtk_tree_selection_get_selected (ingreso->selection, NULL, &iter) == TRUE)
    {
	gtk_tree_model_get (GTK_TREE_MODEL (ingreso->store), &iter,
			    1, &barcode,
			    -1);
    }
    else
    {
	return;
    }

    q = g_strdup_printf ("SELECT codigo_corto, descripcion, marca, unidad, "
			 "contenido, precio FROM select_producto(%s)", barcode);

    res = EjecutarSQL(q);
    g_free(q);

    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
	g_printerr("error en %s\n%s",G_STRFUNC, PQresultErrorMessage(res));
	return;
    }

    codigo = PQgetvalue (res, 0, 0);
    description = PQgetvalue(res, 0, 1);
    marca = PQgetvalue(res, 0, 2);
    unidad = PQgetvalue( res, 0, 3);
    contenido = PQgetvalue (res, 0, 4);
    precio = PQgetvalue (res, 0, 5);

    compra->see_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_position (GTK_WINDOW (compra->see_window),
			     GTK_WIN_POS_CENTER_ALWAYS);
    gtk_window_set_title (GTK_WINDOW (compra->see_window),
			  "Descripcion Producto");

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

    fraccionm = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));

    if (VentaFraccion (barcode) == TRUE)
    {
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
	fraccionm = TRUE;
    }
    else
    {
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), FALSE);
	fraccionm = FALSE;
    }

    g_signal_connect (G_OBJECT (button), "toggled",
		      G_CALLBACK (ToggleSelect), (gpointer)"3");

    g_signal_connect (G_OBJECT (button), "toggled",
		      G_CALLBACK (ChangeSave), NULL);

    /*
      hbox = gtk_hbox_new (FALSE, 3);
      gtk_widget_show (hbox);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

      label = gtk_label_new ("Familia del Producto: ");
      gtk_widget_show (label);
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);


      res = EjecutarSQL ("SELECT * FROM familias ORDER BY id DESC");

      tuples = PQntuples (res);

      combo_fami = gtk_combo_box_new_text ();
      gtk_box_pack_end (GTK_BOX (hbox), combo_fami, FALSE, FALSE, 3);
      gtk_widget_show (combo_fami);

      for (i = 0; i < tuples; i++)
      gtk_combo_box_append_text (GTK_COMBO_BOX (combo_fami),
      g_strdup_printf ("%s",
      PQgetvalue (res, i, 1)));
      active = GetFami (barcode);

      gtk_combo_box_set_active (GTK_COMBO_BOX (combo_fami), active - 1);

      g_signal_connect (G_OBJECT (combo_fami), "changed",
      G_CALLBACK (ChangeSave), NULL);
    */
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


    res = EjecutarSQL ("SELECT id, descripcion, monto "
		       "FROM impuestos WHERE id!=0");

    tuples = PQntuples (res);

    combo_imp = gtk_combo_box_new_text ();
    gtk_box_pack_end (GTK_BOX (hbox), combo_imp, FALSE, FALSE, 3);
    gtk_widget_show (combo_imp);

    gtk_combo_box_append_text (GTK_COMBO_BOX (combo_imp), "Ninguno");

    active_char = GetOtrosName (barcode);

    for (i = 0; i < tuples; i++)
    {
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_imp),
				   g_strdup_printf ("%s",
						    PQgetvalue (res, i, 1)));

	if (active_char != NULL)
	{
	    if (strcmp (PQgetvalue (res, i, 1), active_char) == 0 &&
		strcmp (PQgetvalue (res, i, 1), ""))
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
