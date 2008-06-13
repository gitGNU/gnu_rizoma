/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4;
       c-indentation-style: gnu -*- */
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
#define _XOPEN_SOURCE 600
#include<features.h>

#include<gtk/gtk.h>

#include<stdlib.h>
#include<string.h>

#include<math.h>

#include"tipos.h"

#include"administracion_productos.h"
#include"postgres-functions.h"
#include"errors.h"
#include"printing.h"
#include"compras.h"

#include"utils.h"

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

/**
 * This function is connected the accept button of the adjust stock window.
 *
 * @param widget the widget that emited the signal
 * @param data the user data
 */void
AjustarMercaderia (GtkWidget *widget, gpointer data)
{
  gint merma_id;
  gint active;
  gdouble cantidad;
  //  gdouble stock;
  gchar *barcode;
  GtkTreeIter iter;
  GtkListStore *store;
  GtkWidget *aux_widget;
  gchar *endptr=NULL;

  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_informerca_barcode"));
  barcode = g_strdup(gtk_label_get_text(GTK_LABEL(aux_widget)));

  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_adjust_new_stock"));
  cantidad = strtod (PUT (g_strdup (gtk_entry_get_text (GTK_ENTRY (aux_widget)))), &endptr);

  if ((cantidad == 0) && g_str_equal(endptr, ""))
    {
      ErrorMSG(aux_widget, "Debe ingresar un número mayor que cero");
      return;
    }

  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "cmbbox_adjust_motive"));
  active = gtk_combo_box_get_active (GTK_COMBO_BOX (aux_widget));

  if (active == -1)
    {
      ErrorMSG (GTK_WIDGET (aux_widget), "Debe Seleccionar un motivo de la merma");
      return;
    }

  store = GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(aux_widget)));
  if (!(gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(store), &iter, g_strdup_printf("%d",active))))
    {
      g_printerr("%s: Troubles with the path", G_STRFUNC);
      return;
    }

  gtk_tree_model_get (GTK_TREE_MODEL (store), &iter,
		      0, &merma_id,
		      -1);

  AjusteStock (cantidad, merma_id, barcode);

  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "wnd_adjust_product"));
  gtk_widget_hide (aux_widget);

  FillFields (NULL, NULL);
}


/**
 * This function close the adjust stock window.
 *
 * @param button the button that emited the signal
 * @param data the user data
 */void
CloseAjusteWin (GtkButton *button, gpointer data)
{
  GtkWidget *aux_widget;

  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "wnd_adjust_product"));
  gtk_widget_hide (aux_widget);
  return;
}


/**
 * This function raise and setup the adjust stock window.
 *
 * @param widget The widget that emited the signal
 * @param data The user data
 */void
AjusteWin (GtkWidget *widget, gpointer data)
{
  GtkWidget *aux_widget;
  GtkWidget *combo_merma;
  GtkListStore *combo_store;

  PGresult *res;
  gint tuples, i;
  gchar *barcode;
  GtkTreeIter iter;


  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_adjust_new_stock"));
  gtk_entry_set_text(GTK_ENTRY(aux_widget), "");

  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_informerca_barcode"));
  barcode = g_strdup(gtk_label_get_text(GTK_LABEL(aux_widget)));

  aux_widget = GTK_WIDGET(gtk_builder_get_object (builder, "treeview_find_products"));

  if (!(g_str_equal(barcode, "")))
    {

      aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_adjust_current_stock"));
      gtk_label_set_markup (GTK_LABEL (aux_widget),
                            g_strdup_printf ("%.3f", GetCurrentStock (barcode)));

      res = EjecutarSQL ("SELECT id, nombre FROM select_tipo_merma() "
                         "AS (id int4, nombre varchar(20))");

      tuples = PQntuples (res);

      combo_merma = GTK_WIDGET(gtk_builder_get_object(builder, "cmbbox_adjust_motive"));
      combo_store = GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(combo_merma)));

      if (combo_store == NULL)
	{
	  GtkCellRenderer *cell;
	  //merma
	  combo_store = gtk_list_store_new (2,
					    G_TYPE_INT,    //0 id
					    G_TYPE_STRING);//1 nombre

	  gtk_combo_box_set_model (GTK_COMBO_BOX(combo_merma), GTK_TREE_MODEL(combo_store));

	  cell = gtk_cell_renderer_text_new ();
	  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT(combo_merma), cell, TRUE);
	  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT(combo_merma), cell,
					  "text", 1,
					  NULL);
	}

      gtk_list_store_clear (combo_store);

      for (i = 0; i < tuples; i++)
	{
	  gtk_list_store_append (combo_store, &iter);

	  gtk_list_store_set (combo_store, &iter,
			      0, atoi (PQvaluebycol(res, i, "id")),
			      1, PQvaluebycol(res, i, "nombre"),
			      -1);
	}

      aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_adjust_new_stock"));
      gtk_widget_grab_focus(aux_widget);

      aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "wnd_adjust_product"));
      gtk_widget_show_all(aux_widget);

    }
}

/**
 * This callback is connected to the save button of the modificate
 * product window.
 *
 * Saves the new properties of the product into the database.
 */
void
GuardarModificacionesProducto (void)
{
  GtkWidget *widget;
  gchar *barcode;
  gchar *stock_minimo;
  gchar *margen;
  gchar *new_venta;
  gboolean mayorista;
  gint precio_mayorista;
  gint cantidad_mayorista;

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_informerca_barcode"));
  barcode = g_strdup (gtk_label_get_text (GTK_LABEL (widget)));

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_informerca_minstock"));
  stock_minimo = g_strdup (gtk_entry_get_text (GTK_ENTRY (widget)));

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_infomerca_percentmargin"));
  margen = g_strdup (gtk_entry_get_text (GTK_ENTRY (widget)));

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_informerca_price"));
  new_venta = g_strdup (gtk_entry_get_text (GTK_ENTRY (widget)));

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "radio_mayorist_yes"));
  mayorista = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_informerca_pricemayorist"));
  precio_mayorista = atoi (g_strdup (gtk_entry_get_text (GTK_ENTRY (widget))));

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_informerca_cantmayorist"));
  cantidad_mayorista = atoi (g_strdup (gtk_entry_get_text (GTK_ENTRY (widget))));

  if (g_str_equal (stock_minimo, ""))
    ErrorMSG (stock_min, "Debe setear stock minimo");
  else if (g_str_equal (margen, ""))
    ErrorMSG (margen_entry, "Debe poner un valor de margen para el producto");
  else if (g_str_equal (new_venta, ""))
    ErrorMSG (precio_venta, "Debe insertar un precio de venta");
  else
    {
      SetModificacionesProducto (barcode, stock_minimo, margen, new_venta, FALSE, 0, mayorista, precio_mayorista,
                                 cantidad_mayorista);

      GtkWidget *treeview;
      GtkTreeSelection *selec;
      GtkListStore *store;
      GtkTreeIter iter;
      gchar *selec_barcode;

      treeview = GTK_WIDGET(gtk_builder_get_object(builder, "treeview_find_products"));
      selec = gtk_tree_view_get_selection (GTK_TREE_VIEW(treeview));
      store = GTK_LIST_STORE(gtk_tree_view_get_model (GTK_TREE_VIEW(treeview)));
      if (gtk_tree_selection_get_selected (selec, NULL, &iter) == TRUE)
	{

	  gtk_tree_model_get (GTK_TREE_MODEL (store), &iter,
			      1, &selec_barcode,
			      -1);

	  if (!(g_str_equal(barcode, selec_barcode)))
	    {
	      g_printerr("\nThe barcode selected in the treeview is diferent from the one that is being modified\n");
	      return;
	    }

	  gtk_list_store_set (GTK_LIST_STORE (store), &iter,
			      7, atoi(new_venta),
                          -1);
	  statusbar_push (GTK_STATUSBAR(gtk_builder_get_object(builder, "statusbar")),
			  "Ha sido editado el producto existosamente", 5000);
	}
      //FillFields (NULL, NULL);
    }
}


/**
 * This callback is associated with the accept button of the adjust
 * margin window.
 *
 * Saves the new stock of the product.
 * @param editable
 * @param data
 */void
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
                          g_strdup_printf ("%ld", lround (precio)));

      gtk_label_set_markup (GTK_LABEL (contrib_unit),
                            g_strdup_printf
                            ("<b>%ld</b>", lround ((gdouble)fifo * (gdouble)new_margen / 100)));
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
                          g_strdup_printf ("%ld", lround (precio)));

      gtk_label_set_markup (GTK_LABEL (contrib_unit),
                            g_strdup_printf
                            ("<b>%ld</b>", lround ((gdouble)fifo * (gdouble)new_margen / 100)));
    }

  contri_unit = atoi (g_strdup (gtk_label_get_text (GTK_LABEL (contrib_unit))));

  new_venta = atoi (g_strdup (gtk_entry_get_text (GTK_ENTRY (precio_venta))));

  stock = GetCurrentStock (barcode);

  gtk_label_set_markup (GTK_LABEL (contri_proy),
                        g_strdup_printf ("<b>$%d</b>", contri_unit * stock));

  gtk_label_set_markup (GTK_LABEL (stock_valor),
                        g_strdup_printf ("<b>$%d</b>", stock * new_venta));
}

/**
 * This function initialize the 'Mercaderia' tab.
 *
 * It must be called from the startup of the application, because this
 * function setup the UI
 */
void
admini_box ()
{
  GtkListStore *store;
  GtkWidget *widget;
  GtkWidget *treeview;
  GtkTreeSelection *selection;

  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  Print *print;

  print = (Print *) malloc (sizeof (Print));

  //stock valorizado
  inv_total_stock = GTK_WIDGET(gtk_builder_get_object (builder, "lbl_merca_stock_valorizado"));
  gtk_label_set_markup (GTK_LABEL (inv_total_stock),
                        g_strdup_printf ("<span foreground=\"blue\"><b>$ %s</b></span>",
                                         PutPoints (InversionTotalStock ())));

  //valorizado de venta
  valor_total_stock = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_merca_valorizado_venta"));
  gtk_label_set_markup (GTK_LABEL (valor_total_stock),
                        g_strdup_printf ("<span foreground=\"blue\"><b>$ %s</b></span>",
                                         PutPoints (ValorTotalStock ())));

  //contribucion proyectada
  contri_total_stock = GTK_WIDGET(gtk_builder_get_object (builder, "lbl_merca_contrib_proyectada"));
  gtk_label_set_markup (GTK_LABEL (contri_total_stock),
                        g_strdup_printf ("<span foreground=\"blue\"><b>$ %s</b></span>",
                                         PutPoints (ContriTotalStock ())));
  //products list
  store = gtk_list_store_new (10,
			      G_TYPE_STRING,  //0 shortcode
			      G_TYPE_STRING,  //1 barcode
			      G_TYPE_STRING,  //2 description
			      G_TYPE_STRING,  //3 brand
			      G_TYPE_INT,     //4 cantidad
			      G_TYPE_STRING,  //5 unit
			      G_TYPE_INT,     //6 stock
			      G_TYPE_INT,     //7 price
			      G_TYPE_STRING,  //8
			      G_TYPE_BOOLEAN);//9

  treeview = GTK_WIDGET(gtk_builder_get_object (builder, "treeview_find_products"));
  gtk_tree_view_set_model (GTK_TREE_VIEW(treeview), GTK_TREE_MODEL(store));


  /* Ahora llenamos la struct con los datos necesarios para poder imprimir el treeview */
  print->tree = GTK_TREE_VIEW (treeview);
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

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));

  g_signal_connect(G_OBJECT(selection), "changed",
		   G_CALLBACK(FillFields), NULL);

  gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);

  /* g_signal_connect (G_OBJECT (selection), "changed", */
  /*                   G_CALLBACK (FillFields), NULL); */

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Código", renderer,
                                                     "text", 0,
                                                     "foreground", 8,
                                                     "foreground-set", 9,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
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
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Descripción", renderer,
                                                     "text", 2,
                                                     "foreground", 8,
                                                     "foreground-set", 9,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
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
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
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
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
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
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
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
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
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
  gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 7);
  gtk_tree_view_column_set_resizable (column, FALSE);

  //this signal is connected in this way because glade/gtkbuilde does
  //not respect the signature of the user_data when is seted with the glade
  widget = GTK_WIDGET (gtk_builder_get_object (builder, "entry_infomerca_percentmargin"));
  g_signal_connect (G_OBJECT (widget), "activate",
                    G_CALLBACK (ModificarMargenVenta), (gpointer)TRUE);
  ///////////

  widget = GTK_WIDGET (gtk_builder_get_object (builder, "entry_informerca_price"));
  g_signal_connect (G_OBJECT (widget), "activate",
                    G_CALLBACK (ModificarMargenVenta), (gpointer)FALSE);

  //////////

  widget = GTK_WIDGET(gtk_builder_get_object (builder, "btn_infomerca_print"));
  g_signal_connect (G_OBJECT (widget), "clicked",
                    G_CALLBACK (PrintTree), (gpointer)print);

  ////////////////////////////////////////////////////////


  /* button = gtk_button_new_with_label ("Devolución"); */
  /* gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0); */
  /* gtk_widget_show (button); */

  /* g_signal_connect (G_OBJECT (button), "clicked", */
  /*                   G_CALLBACK (DevolucionWindow), NULL); */

  /* button = gtk_button_new_with_label ("Recibir"); */
  /* gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0); */
  /* gtk_widget_show (button); */

  /* g_signal_connect (G_OBJECT (button), "clicked", */
  /*                   G_CALLBACK (RecivirWindow), NULL); */
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
  GtkTreeSelection *selec;
  GtkWidget *treeview;
  GtkWidget *entry;
  GtkListStore *store;

  if (Deleting != TRUE)
    {
      treeview = GTK_WIDGET(gtk_builder_get_object(builder, "treeview_find_products"));
      selec = gtk_tree_view_get_selection (GTK_TREE_VIEW(treeview));
      store = GTK_LIST_STORE(gtk_tree_view_get_model (GTK_TREE_VIEW(treeview)));

      gtk_tree_selection_get_selected (selec, NULL, &iter);

      gtk_tree_model_get (GTK_TREE_MODEL (store), &iter,
                          0, &codigo,
                          2, &product,
                          7, &precio,
                          -1);

      entry = GTK_WIDGET(gtk_builder_get_object(builder, "entry_"));
      gtk_entry_set_text (GTK_ENTRY (ingreso->codigo_entry_edit), g_strdup (codigo));
      gtk_entry_set_text (GTK_ENTRY (ingreso->product_entry_edit), g_strdup (product));
      gtk_entry_set_text (GTK_ENTRY (ingreso->precio_entry_edit), g_strdup_printf ("%d", precio));
    }
}


/**
 * This function is the callback connected to the 'row-activated'
 * signal of the GtkTreeView.
 *
 * It populates the labels and the entries with information associated
 * with the product that user selected.
 *
 * Note: just pass NULL parameters and the function will work, the
 * paremeter were not eliminated just for backward compatibility with
 * old code.
 *
 * @param selection unused parameter
 * @param data unused paramter
 */void
FillFields(GtkTreeSelection *selection, gpointer data)
{
  GtkTreeIter iter;
  PGresult *res;
  gchar *barcode;
  gint vendidos;
  gdouble stock;
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
  GtkListStore *store;
  GtkWidget *treeview;
  GtkWidget *aux_widget;
  GtkTreeSelection *selec;

  treeview = GTK_WIDGET(gtk_builder_get_object(builder, "treeview_find_products"));
  store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(treeview)));
  selec = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));

  if (gtk_tree_selection_get_selected (selec, NULL, &iter) == TRUE)
    {
      gtk_tree_model_get (GTK_TREE_MODEL (store), &iter,
                          1, &barcode,
                          -1);

      q = g_strdup_printf ("SELECT * FROM informacion_producto (%s, '')", barcode);
      res = EjecutarSQL(q);
      g_free(q);

      stock = g_ascii_strtod (PQvaluebycol( res, 0, "stock"), NULL);

      margen = atoi (PQvaluebycol (res, 0, "margen_promedio"));

      merma = (gdouble) atoi (PQvaluebycol (res, 0, "unidades_merma"));

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

      aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_informerca_barcode"));
      gtk_label_set_markup (GTK_LABEL (aux_widget),
                            g_strdup_printf ("<b>%s</b>", barcode));

      aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_informerca_shortcode"));
      gtk_label_set_markup (GTK_LABEL (aux_widget),
                            g_strdup_printf ("<b>%s</b>", PQvaluebycol (res, 0, "codigo_corto")));

      aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_informerca_desc"));
      gtk_label_set_markup (GTK_LABEL (aux_widget),
                            g_strdup_printf ("<b>%s</b>", PQvaluebycol (res, 0, "descripcion")));

      aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_informerca_brand"));
      gtk_label_set_markup (GTK_LABEL (aux_widget),
                            g_strdup_printf ("<b>%s</b>", PQvaluebycol (res, 0, "marca")));

      aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_informerca_content"));
      gtk_label_set_markup (GTK_LABEL (aux_widget),
                            g_strdup_printf ("<b>%s</b>", PQvaluebycol (res, 0, "contenido")));

      aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_informerca_unit"));
      gtk_label_set_markup (GTK_LABEL (aux_widget),
                            g_strdup_printf ("<b>%s</b>", PQvaluebycol (res, 0, "unidad")));

      /* aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_informerca_sold")); */
      /* gtk_label_set_markup (GTK_LABEL (aux_widget), */
      /*                       g_strdup_printf ("<b>%d</b>", vendidos)); */

      aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_informerca_stock"));
      gtk_label_set_markup (GTK_LABEL (aux_widget),
                            g_strdup_printf ("<b>%.2f</b>", stock));

      aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_informerca_sales_day"));
      gtk_label_set_markup (GTK_LABEL (aux_widget),
                            g_strdup_printf ("<b>%.2f</b>", g_ascii_strtod (PQvaluebycol (res, 0, "ventas_dia"), NULL)));

      /* aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_informerca_")); */
      /* gtk_label_set_markup (GTK_LABEL (stock_dias), */
      /*                       g_strdup_printf ("<b>%.2f</b>", GetDayToSell (barcode))); */

      aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_informerca_minstock"));
      gtk_entry_set_text (GTK_ENTRY (aux_widget),
			  PQvaluebycol (res, 0, "stock_min"));

      aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_informerca_avg_cost"));
      gtk_label_set_markup (GTK_LABEL (aux_widget),
                            g_strdup_printf ("<b>%d</b>", fifo));

      aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_informerca_extratax"));
      gtk_label_set_markup (GTK_LABEL (aux_widget),
                            g_strdup_printf ("<b>%s</b>", GetLabelImpuesto (barcode)));

      /* aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "")); */
      /* gtk_label_set_markup (GTK_LABEL (perecible), */
      /*                       g_strdup_printf ("<b>%s</b>", GetPerecible (barcode))); */

      aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_infomerca_percentmargin"));
      gtk_entry_set_text (GTK_ENTRY (aux_widget),
                          g_strdup_printf ("%d", margen));

      aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_informerca_contrib_unit"));
      gtk_label_set_markup (GTK_LABEL (aux_widget),
                            g_strdup_printf ("<b>$%d</b>", contri_unit));

      aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_informerca_price"));
      gtk_entry_set_text (GTK_ENTRY (aux_widget), PQvaluebycol (res, 0, "precio"));

      aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_informerca_invstock"));
      gtk_label_set_markup (GTK_LABEL (aux_widget),
                            g_strdup_printf ("<b>$%d</b>", valor_stock));

      aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_informerca_mermauni"));
      gtk_label_set_markup (GTK_LABEL (aux_widget),
                            g_strdup_printf ("<b>%.2f</b>", merma));

      /* aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "")); */
      /* gtk_label_set_markup (GTK_LABEL (mermata), */
      /* 			    g_strdup_printf ("<b>%.2f %%</b>", mermaporc)); */

      /* aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "")); */
      /* gtk_label_set_markup (GTK_LABEL (ici), */
      /*                       g_strdup_printf ("<b>%.2f%%</b>", ici_total)); */

      aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_informerca_totalbuy"));
      gtk_label_set_markup (GTK_LABEL (aux_widget),
                            g_strdup_printf ("<b>$%d</b>", compras_totales));

      aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_informerca_sold"));
      if(g_str_equal (PQvaluebycol (res, 0, "contrib_agregada"), ""))
        gtk_label_set_markup (GTK_LABEL (aux_widget), "");
      else
        gtk_label_set_markup (GTK_LABEL (aux_widget),
                              g_strdup_printf ("<b>$ %.2f</b>",
					       g_ascii_strtod(PQvaluebycol (res, 0, "total_vendido"),
									       NULL)));

      aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_informerca_contribadded"));
      gtk_label_set_markup (GTK_LABEL (aux_widget),
                            g_strdup_printf ("<b>$%d</b>", contrib_agreg));

      aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_informerca_contribproyectada"));
      gtk_label_set_markup (GTK_LABEL (aux_widget),
                            g_strdup_printf ("<b>$%d</b>", contrib_proyect));

      /* aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "")); */
      /* gtk_label_set_markup (GTK_LABEL (stock_pro), */
      /*                       g_strdup_printf ("<b>%s</b>", PQvaluebycol (res, 0, "stock_pro"))); */

      /* aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "")); */
      /* gtk_entry_set_text (GTK_ENTRY (tasa_canje), PQvaluebycol (res, 0, "tasa_canje")); */



      if (strcmp (PQvaluebycol (res, 0, "mayorista"), "t") == 0)
        {
	  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "radio_mayorist_yes"));
          gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (aux_widget), TRUE);
	  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "radio_mayorist_no"));
	  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (aux_widget), FALSE);
        }
      else
        {
	  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "radio_mayorist_yes"));
          gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (aux_widget), FALSE);
	  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "radio_mayorist_no"));
	  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (aux_widget), TRUE);
        }

      /* q = g_strdup_printf("SELECT nombre FROM select_proveedor_for_product(%s)", */
      /*                     barcode); */
      /* res = EjecutarSQL (q); */
      /* g_free(q); */

      /* tuples = PQntuples (res); */

      /* model_proveedores = GTK_WIDGET (gtk_list_store_new (1, */
      /*                                                     G_TYPE_STRING)); */

      /* gtk_combo_box_set_model (GTK_COMBO_BOX (combo_proveedores), */
      /*                          GTK_TREE_MODEL (model_proveedores)); */

      /* for (i = 0; i < tuples; i++) */
      /*   gtk_combo_box_append_text (GTK_COMBO_BOX (combo_proveedores), */
      /*                              PQvaluebycol (res, i, "nombre")); */

      /* gtk_combo_box_set_active (GTK_COMBO_BOX (combo_proveedores), 0); */

      aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_informerca_cantmayorist"));
      gtk_entry_set_text (GTK_ENTRY (aux_widget), g_strdup_printf ("%d", cantidad_mayorista));

      aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_informerca_pricemayorist"));
      gtk_entry_set_text (GTK_ENTRY (aux_widget), g_strdup_printf ("%d", precio_mayorista));
    }
}

/**
 * This function is a callback connected to the delete product button
 * present in the 'mercaderia' tab.
 *
 * @param button The button that emited the signal
 * @param data the user data
 */
void
EliminarProductoDB (GtkButton *button, gpointer data)
{
  GtkWidget *treeview;
  GtkListStore *store;
  GtkTreeIter iter;
  GtkTreeSelection *selection;
  gchar *codigo;
  gint stock;

  treeview = GTK_WIDGET(gtk_builder_get_object(builder, "treeview_find_products"));
  selection  = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
  store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (treeview)));

  if (gtk_tree_selection_get_selected (selection, NULL, &iter) == TRUE)
    {
      Deleting = TRUE;

      gtk_tree_model_get (GTK_TREE_MODEL (store), &iter,
                          1, &codigo,
                          6, &stock,
                          -1);

      if (stock == 0)
        {
          if (DeleteProduct (codigo))
	    gtk_list_store_remove (GTK_LIST_STORE (store), &iter);
	  else
	    ErrorMSG (GTK_WIDGET (treeview), "No pudo ser borrado el producto \n"
		      "aun debe encontrarse en uso por alguna parte del sistema");
        }
      else
        ErrorMSG (GTK_WIDGET (treeview), "Solo se puede eliminar productos \n con stock mayor a 0");
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

/**
 * This function is the callback connected to the search button
 * present in the 'mercaderia' tab.
 *
 * This populates the model with the result returned by the search
 * function of the database
 *
 */
void
BuscarProductosParaListar (void)
{
  PGresult *res;
  gchar *q;
  gchar *string;
  gint i, resultados;
  GtkTreeIter iter;
  GtkWidget *widget;
  GtkListStore *store;

  widget = GTK_WIDGET(gtk_builder_get_object (builder,"find_product_entry"));
  string = g_strdup (gtk_entry_get_text(GTK_ENTRY(widget)));
  q = g_strdup_printf ( "SELECT * FROM buscar_producto( '%s', "
			"'{\"barcode\", \"codigo_corto\",\"marca\",\"descripcion\"}',"
			"TRUE, FALSE )", string);
  res = EjecutarSQL (q);
  g_free (q);

  resultados = PQntuples (res);

  widget = GTK_WIDGET(gtk_builder_get_object (builder,"lbl_find_num_products"));
  gtk_label_set_markup (GTK_LABEL (widget),
                        g_strdup_printf ("<b>%d producto(s)</b>", resultados));

  widget = GTK_WIDGET(gtk_builder_get_object (builder,"treeview_find_products"));
  store = GTK_LIST_STORE(gtk_tree_view_get_model (GTK_TREE_VIEW(widget)));
  gtk_list_store_clear (store);

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
                          8, (atoi (PQvaluebycol (res, i, "stock")) <= atoi (PQvaluebycol (res, i, "stock_min")) &&
                              atoi (PQvaluebycol (res, i, "stock_min")) != 0) ? "Red" : "Black",
                          9, TRUE,
                          -1);
    }
}

/**
 * This function saves the modifications that the user entedered in
 * the modification product window
 *
 */
void
ModificarProducto (void)
{
  GtkWidget *widget;
  GtkWidget *combo_imp;
  GtkTreeIter iter;
  GtkListStore *combo_store;

  gchar *q;
  gchar *barcode;
  gint active;
  gint otros_index;

  PGresult *res;
  gint tuples, i;

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_informerca_barcode"));
  barcode = g_strdup(gtk_label_get_text(GTK_LABEL(widget)));

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_edit_prod_barcode"));
  gtk_entry_set_text(GTK_ENTRY(widget), barcode);

  q = g_strdup_printf ("SELECT codigo_corto, descripcion, marca, unidad, "
                       "contenido, precio FROM select_producto(%s)", barcode);
  res = EjecutarSQL(q);
  g_free(q);

  if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
      g_printerr("error en %s\n%s",G_STRFUNC, PQresultErrorMessage(res));
      return;
    }

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_edit_prod_shortcode"));
  gtk_entry_set_text(GTK_ENTRY(widget), PQvaluebycol (res, 0, "codigo_corto"));

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_edit_prod_desc"));
  gtk_entry_set_text(GTK_ENTRY(widget), PQvaluebycol (res, 0, "descripcion"));

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_edit_prod_brand"));
  gtk_entry_set_text(GTK_ENTRY(widget), PQvaluebycol (res, 0, "marca"));

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_edit_prod_unit"));
  gtk_entry_set_text(GTK_ENTRY(widget), PQvaluebycol ( res, 0, "unidad"));

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_edit_prod_content"));
  gtk_entry_set_text(GTK_ENTRY(widget), PQvaluebycol (res, 0, "contenido"));

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_edit_prod_price"));
  gtk_entry_set_text(GTK_ENTRY(widget), PQvaluebycol (res, 0, "precio"));

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "checkbtn_edit_prod_fraccionaria"));
  if (VentaFraccion (barcode))
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), TRUE);
  else
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), FALSE);


  widget = GTK_WIDGET(gtk_builder_get_object(builder, "checkbtn_edit_prod_perecible"));
  if (g_str_equal (GetPerecible (barcode), "Perecible"))
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), TRUE);
  else
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), FALSE);


  widget = GTK_WIDGET(gtk_builder_get_object(builder, "checkbtn_edit_prod_iva"));
  if (GetIVA (barcode) != -1)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), TRUE);
  else
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), FALSE);


  combo_imp = GTK_WIDGET(gtk_builder_get_object(builder, "cmbbox_edit_prod_extratax"));
  combo_store = GTK_LIST_STORE(gtk_combo_box_get_model (GTK_COMBO_BOX(combo_imp)));

  if (combo_store == NULL)
    {
      GtkCellRenderer *cell;

      combo_store = gtk_list_store_new (3,
					G_TYPE_INT,    //0 id
					G_TYPE_STRING, //1 descripcion
					G_TYPE_DOUBLE);//2 monto

      gtk_combo_box_set_model (GTK_COMBO_BOX(combo_imp), GTK_TREE_MODEL(combo_store));

      cell = gtk_cell_renderer_text_new ();
      gtk_cell_layout_pack_start (GTK_CELL_LAYOUT(combo_imp), cell, TRUE);
      gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT(combo_imp), cell,
				      "text", 1,
				      NULL);
    }

  gtk_list_store_clear (combo_store);

  gtk_list_store_append (combo_store, &iter);

  gtk_list_store_set (combo_store, &iter,
		      0, -1,
		      1, "Ninguno",
		      2, 0.0,
		      -1);


  otros_index = GetOtrosIndex(barcode);
  active = -1;

  res = EjecutarSQL ("SELECT id, descripcion, monto "
                     "FROM impuesto WHERE id!=0");
  tuples = PQntuples (res);

  for (i = 0; i < tuples; i++)
    {
      gtk_list_store_append (combo_store, &iter);

      gtk_list_store_set (combo_store, &iter,
			  0, atoi (PQvaluebycol(res, i, "id")),
			  1, PQvaluebycol (res, i, "descripcion"),
			  2, g_ascii_strtod (PQvaluebycol(res, i, "monto"), NULL),
			  -1);
      if (atoi (PQvaluebycol(res, i, "id")) == otros_index)
	active = i+1;
    }

  gtk_combo_box_set_active (GTK_COMBO_BOX (combo_imp), active != -1 ? active : 0);

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "wnd_mod_product"));
  gtk_widget_show_all(widget);
}

/**
 * This function closes the modification product window
 *
 */
void
CloseProductDescription (void)
{
  GtkWidget *widget;

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "wnd_mod_product"));

  gtk_widget_hide (widget);
}
