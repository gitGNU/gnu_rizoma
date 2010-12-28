/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4;
   c-indentation-style: gnu -*- */
/*rizoma_informes.c
 *
 *    Copyright (C) 2008 Rizoma Tecnologia Limitada <info@rizoma.cl>
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

#include <gtk/gtk.h>
#include <unistd.h>
#include <glib.h>


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tipos.h"
#include "postgres-functions.h"
#include "config_file.h"
#include "utils.h"
#include "encriptar.h"
#include "rizoma_errors.h"
#include "printing.h"

GtkBuilder *builder;
PGresult *res_sells;
gint contador, fin;
GDate *date_begin;
GDate *date_end;


/**
 * Es llamada cuando se presiona en el tree_view_sells (signal changed).
 * 
 * Esta funcion visualiza los productos de un detalle de la venta en el tree_view
 *
 */

void
ChangeVenta (void)
{
  GtkTreeView *tree = GTK_TREE_VIEW (builder_get (builder, "tree_view_sells"));
  GtkTreeModel *model = gtk_tree_view_get_model (tree);
  GtkTreeSelection *selection = gtk_tree_view_get_selection (tree);
  GtkListStore *store_detail = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (builder_get (builder, "tree_view_sell_detail"))));
  GtkTreeIter iter;
  gchar *idventa;
  gint i, tuples;
  PGresult *res;
  
    
  if (gtk_tree_selection_get_selected (selection, NULL, &iter) == TRUE)
    {
      gtk_tree_model_get (model, &iter,
                          1, &idventa,
                          -1);

      gtk_list_store_clear (GTK_LIST_STORE (store_detail));

      /* consulta que arroja el detalle de una venta*/
  

      res = EjecutarSQL2
        (g_strdup_printf
         ("SELECT descripcion, marca, contenido, unidad, cantidad, venta_detalle.precio, (cantidad * venta_detalle.precio)::int AS monto FROM venta_detalle, producto WHERE producto.barcode=venta_detalle.barcode and id_venta=%s", idventa));

      tuples = PQntuples (res);

      for (i = 0; i < tuples; i++)
        {
          gtk_list_store_append (store_detail, &iter);
          gtk_list_store_set (store_detail, &iter,
                              0, g_strdup_printf ("%s %s %s %s", PQvaluebycol (res, i, "descripcion"),
                                                  PQvaluebycol (res, i, "marca"), PQvaluebycol (res, i, "contenido"),
                                                  PQvaluebycol (res, i, "unidad")),
                              1, PQvaluebycol (res, i, "cantidad"),
                              2, PutPoints (PQvaluebycol (res, i, "precio")),
                              3, PQvaluebycol (res, i, "monto"),
                              -1);
        }
    }  
}

/**
 * Es llamada cuando se presiona en el tree_view_devolucion (signal changed).
 * 
 * Esta funcion visualiza los productos de un detalle de la devolucion en el tree_view.
 *
 */

void
ChangeDevolucion (void)
{
  GtkTreeView *tree = GTK_TREE_VIEW (builder_get (builder, "tree_view_devolucion"));
  GtkTreeModel *model = gtk_tree_view_get_model (tree);
  GtkTreeSelection *selection = gtk_tree_view_get_selection (tree);
  GtkListStore *store_detail = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (builder_get (builder, "tree_view_devolucion_detalle"))));
  GtkTreeIter iter;
  gchar *iddevolucion;
  gint i, tuples;
  PGresult *res;

  if (gtk_tree_selection_get_selected (selection, NULL, &iter) == TRUE)
    {
      gtk_tree_model_get (model, &iter,
                          1, &iddevolucion,
                          -1);

      gtk_list_store_clear (GTK_LIST_STORE (store_detail));

      /* consulta que arroja el detalle de una devolucion*/
      res = EjecutarSQL
        (g_strdup_printf
         ("SELECT descripcion, marca, contenido, unidad, cantidad, devolucion_detalle.precio, (cantidad * devolucion_detalle.precio)::int AS "
          "monto FROM devolucion_detalle, producto WHERE producto.barcode = devolucion_detalle.barcode and id_devolucion=%s", iddevolucion));

      tuples = PQntuples (res);

      for (i = 0; i < tuples; i++)
        {
          gtk_list_store_append (store_detail, &iter);
          gtk_list_store_set (store_detail, &iter,
                              0, g_strdup_printf ("%s %s %s %s", PQvaluebycol (res, i, "descripcion"),
                                                  PQvaluebycol (res, i, "marca"), PQvaluebycol (res, i, "contenido"),
                                                  PQvaluebycol (res, i, "unidad")),
                              1, PQvaluebycol (res, i, "cantidad"),
                              2, PutPoints (PQvaluebycol (res, i, "precio")),
                              3, PQvaluebycol (res, i, "monto"),
                              -1);
        }
    }
}

/**
 * Es llamada cuando se presiona en el tree_view_cash_box_lists (signal changed).
 * 
 * Esta funcion visualiza los datos de caja de cada apertura y cierre (montos
 * de ventas, perdidad, ingresos, egresos, etc)
 *
 */

void
fill_caja_data (void)
{
  GtkTreeView *tree = GTK_TREE_VIEW (builder_get (builder, "tree_view_cash_box_lists"));
  GtkTreeModel *model = gtk_tree_view_get_model (tree);
  GtkTreeSelection *selection = gtk_tree_view_get_selection (tree);
  GtkTreeIter iter;

  PGresult *res;
  gchar *query;
  gint cash_box_id;

  clean_container (GTK_CONTAINER (builder_get (builder, "table_data")));

  if (gtk_tree_selection_get_selected (selection, NULL, &iter) == TRUE)
    {
      gtk_tree_model_get (model, &iter,
                          0, &cash_box_id,
                          -1);

      query = g_strdup_printf ("select to_char (open_date, 'DD-MM-YYYY HH24:MI') as open_date_formatted, "
                               "to_char (close_date, 'DD-MM-YYYY HH24:MI') as close_date_formatted, * from cash_box_report (%d)", cash_box_id);
      res = EjecutarSQL (query);
      g_free (query);

      if (res != NULL && PQntuples (res) != 0)
        {
          gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_open")),
                                g_strdup_printf ("<b>%s</b>", PQvaluebycol (res, 0, "open_date_formatted")));

          gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_close")),
                                g_strdup_printf ("<b>%s</b>", PQvaluebycol (res, 0, "close_date_formatted")));

          gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_mbox_start")),
                                g_strdup_printf ("<b>%s</b>",
                                                 PutPoints(g_strdup_printf ("%d",
                                                                            (atoi (PQvaluebycol (res, 0, "cash_box_start"))
                                                                             )))));

          gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_mbox_cash")),
                                g_strdup_printf ("<b>%s</b>", PutPoints (PQvaluebycol (res, 0, "cash_sells"))));

          gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_mbox_payed_money")),
                                g_strdup_printf ("<b>%s</b>", PutPoints (PQvaluebycol (res, 0, "cash_payed_money"))));

          /* if (strcmp (PQvaluebycol (res, 0, 2), "") != 0) */
          /*   gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_mbox_doc")), */
          /*                         g_strdup_printf ("<b>%s</b>", PutPoints (PQvaluebycol (res, 0, 2)))); */

          /* if (strcmp (PQvaluebycol (res, 0, 4), "") != 0) */
          /*   gtk_label_set_markup (GTK_LABEL (pago_ventas), */
          /*                         g_strdup_printf ("<b>%s</b>", PutPoints (PQvaluebycol (res, 0, 4)))); */

          gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_mbox_other")),
                                g_strdup_printf ("<b>%s</b>", PutPoints (PQvaluebycol (res, 0, "cash_income"))));

          gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_mbox_ingr_total")),
                                g_strdup_printf
                                ("<b>$ %s</b>", PutPoints
                                 (g_strdup_printf ("%d", atoi (PQvaluebycol (res, 0, "cash_sells"))
                                                   + atoi (PQvaluebycol (res, 0, "cash_income"))
                                                   + atoi (PQvaluebycol (res, 0, "cash_payed_money"))
                                                   + atoi (PQvaluebycol (res, 0, "cash_box_start"))
                                                   ))));

          /* if (strcmp (PQvaluebycol (res, 0, 8), "") != 0) */
          /*   gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_mbox_out_invoice")), */
          /*                         g_strdup_printf ("<b>%s</b>", PutPoints (PQvaluebycol (res, 0, 8)))); */

          gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_mbox_out")),
                                g_strdup_printf ("<b>%s</b>", PutPoints (PQvaluebycol (res, 0, "cash_outcome"))));

          gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_mbox_loss")),
                                g_strdup_printf ("<b>%s</b>", PutPoints (PQvaluebycol (res, 0, "cash_loss_money"))));

          /* if (strcmp (PQvaluebycol (res, 0, 6), "") != 0) */
          /*   gtk_label_set_markup (GTK_LABEL (gastos_corrientes), */
          /*                         g_strdup_printf ("<b>%s</b>", PutPoints (PQvaluebycol (res, 0, 6)))); */

          /* if (strcmp (PQvaluebycol (res, 0, 7), "") != 0) */
          /*   gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_mbox_out_other")), */
          /*                         g_strdup_printf ("<b>%s</b>", PutPoints (PQvaluebycol (res, 0, 7)))); */

          gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_mbox_out_total")),
                                g_strdup_printf
                                ("<b>$ %s</b>", PutPoints
                                 (g_strdup_printf
                                  ("%d",
                                   atoi (PQvaluebycol (res, 0, "cash_outcome"))
                                   + atoi (PQvaluebycol (res, 0, "cash_loss_money"))
                                   ))));

          gtk_label_set_markup
            (GTK_LABEL (builder_get (builder, "lbl_money_box_total")),
             g_strdup_printf
             ("<span size=\"xx-large\"><b>$ %s</b></span>",
              PutPoints
              (g_strdup_printf ("%d",
                                (atoi (PQvaluebycol (res, 0, "cash_box_start"))
                                + atoi (PQvaluebycol (res, 0, "cash_sells"))
                                + atoi (PQvaluebycol (res, 0, "cash_payed_money")))
                                + atoi (PQvaluebycol (res, 0, "cash_income"))
                                - atoi (PQvaluebycol (res, 0, "cash_loss_money"))
                                - atoi (PQvaluebycol (res, 0, "cash_outcome"))
                                        ))));
         
        }
    }
}




/**
 * Es llamada por la funcion "check_passwd".
 *
 * Esta funcion carga los datos "rizoma-informe.ui" y visualiza la
 * ventana wnd_reports" con sus respectivos tree_views e informacion, ademas
 * de generar los datos para exportarlos a GNumeric.
 *
 *
 */


void
reports_win (void)
{
  GtkListStore *store;
  GtkTreeView *treeview;
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;
  GtkTreeSelection *selection;

  GError *error = NULL;

  Print *print = (Print *) malloc (sizeof (Print));

  Print *libro = (Print *) malloc (sizeof (Print));
  Print *libro2 = (Print *) malloc (sizeof (Print));
  libro->son = (Print *) malloc (sizeof (Print));
  libro2->son = (Print *) malloc (sizeof (Print));


  builder = gtk_builder_new ();

  gtk_builder_add_from_file (builder, DATADIR"/ui/rizoma-informes.ui", &error);

  if (error != NULL)
    g_printerr ("%s\n", error->message);

  gtk_builder_add_from_file (builder, DATADIR"/ui/rizoma-common.ui", &error);

  if (error != NULL)
    g_printerr ("%s\n", error->message);

  gtk_builder_connect_signals (builder, NULL);


  /* Sells */
  store = gtk_list_store_new (6,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_STRING);

  treeview = GTK_TREE_VIEW (builder_get (builder, "tree_view_sells"));
  gtk_tree_view_set_model (treeview, GTK_TREE_MODEL (store));

  selection = gtk_tree_view_get_selection (treeview);

  g_signal_connect (G_OBJECT (selection), "changed",
                    G_CALLBACK (ChangeVenta), NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Fecha", renderer,
                                                     "text", 0,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 0);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("ID Venta", renderer,
                                                     "text", 1,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 1);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Maq.", renderer,
                                                     "text", 2,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 2);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Vendedor", renderer,
                                                     "text", 3,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 3);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Monto", renderer,
                                                     "text", 4,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_alignment (column, 0.5);
  gtk_tree_view_column_set_sort_column_id (column, 4);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Tipo Pago", renderer,
                                                     "text", 5,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 5);
  gtk_tree_view_column_set_resizable (column, FALSE);

  libro->tree = treeview;
  libro->title = "Libro de Ventas";
  libro->name = "ventas";
  libro->date_string = NULL;
  libro->cols[0].name = "Fecha";
  libro->cols[0].num = 0;
  libro->cols[1].name = "Monto";
  libro->cols[1].num = 4;
  libro->cols[2].name = NULL;

  store = gtk_list_store_new (4,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_STRING);

  treeview = GTK_TREE_VIEW (builder_get (builder, "tree_view_sell_detail"));
  gtk_tree_view_set_model (treeview, GTK_TREE_MODEL (store));

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Producto", renderer,
                                                     "text", 0,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
   gtk_tree_view_column_set_sort_column_id (column, 0);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_min_width (column, 260);
  gtk_tree_view_column_set_max_width (column, 260);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Cantidad", renderer,
                                                     "text", 1,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 1);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Unitario", renderer,
                                                     "text", 2,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 2);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Total", renderer,
                                                     "text", 3,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 3);
  gtk_tree_view_column_set_resizable (column, FALSE);

  libro->son->tree = treeview;
  libro->son->cols[0].name = "Producto";
  libro->son->cols[0].num = 0;
  libro->son->cols[1].name = "Cantidad";
  libro->son->cols[1].num = 1;
  libro->son->cols[2].name = "Unitario";
  libro->son->cols[2].num = 2;
  libro->son->cols[3].name = "Total";
  libro->son->cols[3].num = 3;
  libro->son->cols[4].name = NULL;

  g_signal_connect (builder_get (builder, "btn_print_sells"), "clicked",
                    G_CALLBACK (PrintTwoTree), (gpointer)libro);


  /* End Sells */


  /* Sells Rank */

  store = gtk_list_store_new (9,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_INT,
                              G_TYPE_STRING,
                              G_TYPE_DOUBLE,
                              G_TYPE_INT,
                              G_TYPE_INT,
                              G_TYPE_INT,
                              G_TYPE_DOUBLE);

  treeview = GTK_TREE_VIEW (builder_get (builder, "tree_view_sell_rank"));
  gtk_tree_view_set_model (treeview, GTK_TREE_MODEL (store));

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Producto", renderer,
                                                     "text", 0,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 0);
  gtk_tree_view_column_set_min_width (column, 160);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Marca", renderer,
                                                     "text", 1,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 1);
  gtk_tree_view_column_set_min_width (column, 90);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Medida", renderer,
                                                     "text", 2,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 2);
  gtk_tree_view_column_set_min_width (column, 60);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Unid.", renderer,
                                                     "text", 3,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 3);
  gtk_tree_view_column_set_min_width (column, 40);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Unidades", renderer,
                                                     "text", 4,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 4);
  gtk_tree_view_column_set_min_width (column, 50);
  gtk_tree_view_column_set_resizable (column, FALSE);

  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)4, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Vendido $", renderer,
                                                     "text", 5,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 5);
  gtk_tree_view_column_set_min_width (column, 70);
  gtk_tree_view_column_set_resizable (column, FALSE);

  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)5, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Costo $", renderer,
                                                     "text", 6,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 6);
  gtk_tree_view_column_set_min_width (column, 50);
  gtk_tree_view_column_set_resizable (column, FALSE);

  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)6, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Contrib. $", renderer,
                                                     "text", 7,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 7);
  gtk_tree_view_column_set_min_width (column, 60);
  gtk_tree_view_column_set_resizable (column, FALSE);

  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)7, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Margen %", renderer,
                                                     "text", 8,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 8);
  gtk_tree_view_column_set_min_width (column, 50);
  gtk_tree_view_column_set_resizable (column, FALSE);

  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)8, NULL);

  print->tree = treeview;
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

  g_signal_connect (builder_get (builder, "btn_print_rank"), "clicked",
                    G_CALLBACK (PrintTree), (gpointer)print);


  /* End Sells Rank */

  /* Cash Box */

  store = gtk_list_store_new (5,
                              G_TYPE_INT,
                              G_TYPE_STRING,
                              G_TYPE_INT,
                              G_TYPE_STRING,
                              G_TYPE_INT);

  treeview = GTK_TREE_VIEW (builder_get (builder, "tree_view_cash_box_lists"));
  gtk_tree_view_set_model (treeview, GTK_TREE_MODEL (store));

  selection = gtk_tree_view_get_selection (treeview);

  g_signal_connect (G_OBJECT (selection), "changed",
                    G_CALLBACK (fill_caja_data), NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Id", renderer,
                                                     "text", 0,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Fecha Apertura", renderer,
                                                     "text", 1,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Monto Apertura", renderer,
                                                     "text", 2,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Fecha Cierre", renderer,
                                                     "text", 3,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Monto Cierre", renderer,
                                                     "text", 4,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  /* End Cash Box */

   /* Devoluciones*/
  store = gtk_list_store_new (4,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_INT,
                              G_TYPE_STRING);

  treeview = GTK_TREE_VIEW (builder_get (builder, "tree_view_devolucion"));
  gtk_tree_view_set_model (treeview, GTK_TREE_MODEL (store));

  selection = gtk_tree_view_get_selection (treeview);

  g_signal_connect (G_OBJECT (selection), "changed",
                    G_CALLBACK (ChangeDevolucion), NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Fecha", renderer,
                                                     "text", 0,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 0);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("ID Devolución", renderer,
                                                     "text", 1,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 1);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Monto", renderer,
                                                     "text", 2,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 2);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Proveedor", renderer,
                                                     "text", 3,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 3);
  gtk_tree_view_column_set_resizable (column, FALSE);

 
  
  libro2->tree = treeview;
  libro2->title = "Libro de Devoluciones";
  libro2->name = "devoluciones";
  libro2->date_string = NULL;
  libro2->cols[0].name = "Fecha";
  libro2->cols[0].num = 0;
  libro2->cols[1].name = "Monto";
  libro2->cols[1].num = 4;
  libro2->cols[2].name = NULL;

  
  
  store = gtk_list_store_new (4,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_STRING);

  treeview = GTK_TREE_VIEW (builder_get (builder, "tree_view_devolucion_detalle"));
  gtk_tree_view_set_model (treeview, GTK_TREE_MODEL (store));

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Producto", renderer,
                                                     "text", 0,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 0);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_min_width (column, 260);
  gtk_tree_view_column_set_max_width (column, 260);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Cantidad", renderer,
                                                     "text", 1,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 1);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Unitario", renderer,
                                                     "text", 2,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 2);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Total", renderer,
                                                     "text", 3,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 3);
  gtk_tree_view_column_set_resizable (column, FALSE);
  
  
  libro2->son->tree = treeview;
  libro2->son->cols[0].name = "Producto";
  libro2->son->cols[0].num = 0;
  libro2->son->cols[1].name = "Cantidad";
  libro2->son->cols[1].num = 1;
  libro2->son->cols[2].name = "Unitario";
  libro2->son->cols[2].num = 2;
  libro2->son->cols[3].name = "Total";
  libro2->son->cols[3].num = 3;
  libro2->son->cols[4].name = NULL;
  
  
  g_signal_connect (builder_get (builder, "btn_print_devolucion"), "clicked",
                    G_CALLBACK (PrintTwoTree), (gpointer)libro2);


  /* End Devoluciones */

  /*
    Start Proveedores
   */

  
  store = gtk_list_store_new (5,
                              G_TYPE_STRING,
                              G_TYPE_DOUBLE,
                              G_TYPE_INT,
                              G_TYPE_DOUBLE,
                              G_TYPE_INT,
                              -1);
  
 
  treeview = GTK_TREE_VIEW (builder_get (builder, "tree_view_proveedores"));
  gtk_tree_view_set_model (treeview, GTK_TREE_MODEL (store));
  selection = gtk_tree_view_get_selection (treeview);
  
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Proveedor", renderer,
						     "text", 0,
						     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 0);
  gtk_tree_view_column_set_min_width (column, 3000);
  gtk_tree_view_column_set_max_width (column, 300);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Unidades", renderer,
						     "text", 1,
						     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 1);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)1, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Comprado $", renderer,
						     "text", 2,
						     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 2);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)2, NULL);
  
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Margen %", renderer,
						     "text", 3,
						     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 3);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)3, NULL);


  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Contribución $", renderer,
						     "text", 4,
						     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 4);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)4, NULL);
 

  /*
    End Proveedores
   */


  /*
    Start Cuadratura
   */


  store = gtk_list_store_new (10,
                              G_TYPE_STRING, // 0, Descripcion Mercadería 
			      G_TYPE_STRING, // 1, Marca
                              G_TYPE_DOUBLE, // 2, Stock Inicial
                              G_TYPE_DOUBLE, // 3, Compras
                              G_TYPE_DOUBLE, // 4, Ventas
                              G_TYPE_DOUBLE, // 5, Devoluciones
			      G_TYPE_DOUBLE, // 6, Mermas
			      G_TYPE_DOUBLE, // 7, Stock teorico
			      G_TYPE_DOUBLE, // 8, Stock Físico
			      G_TYPE_DOUBLE, // 9, Diferencia
                              -1);
  
 
  treeview = GTK_TREE_VIEW (builder_get (builder, "tree_view_cuadratura"));
  gtk_tree_view_set_model (treeview, GTK_TREE_MODEL (store));
  selection = gtk_tree_view_get_selection (treeview);
  
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Descripción Mercadería", renderer,
						     "text", 0,
						     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 0);
  gtk_tree_view_column_set_min_width (column, 3000);
  gtk_tree_view_column_set_max_width (column, 300);
  gtk_tree_view_column_set_resizable (column, FALSE);


  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Marca", renderer,
						     "text", 1,
						     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 1);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Stock inicial", renderer,
						     "text", 2,
						     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 2);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)2, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Compras", renderer,
						     "text", 3,
						     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 3);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)3, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Ventas", renderer,
						     "text", 4,
						     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 4);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)4, NULL);
  
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Devoluciones", renderer,
						     "text", 5,
						     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 5);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)5, NULL);


  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Mermas", renderer,
						     "text", 6,
						     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 6);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)6, NULL);


  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Stock teórico", renderer,
						     "text", 7,
						     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 7);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)7, NULL);


  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Stock físico", renderer,
						     "text", 8,
						     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 8);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)8, NULL);


  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Diferencia", renderer,
						     "text", 9,
						     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 9);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)9, NULL);


  print->tree = treeview;
  print->title = "Informe Cuadratura";
  print->date_string = NULL;
  print->cols[0].name = "Descripcion";
  print->cols[1].name = "Marca";
  print->cols[2].name = "Stock Inicial";
  print->cols[3].name = "Compras";
  print->cols[4].name = "Ventas";
  print->cols[5].name = "Devoluciones";
  print->cols[6].name = "Mermas";
  print->cols[7].name = "Stock Teórico";
  print->cols[8].name = "Stock Físico";
  print->cols[9].name = "Diferencia";
  print->cols[10].name = NULL;

  //g_signal_connect (builder_get (builder, "btn_print_cuadratura"), "clicked",
  //                  G_CALLBACK (PrintTree), (gpointer)print);


  /*
    End Cuadratura
   */


  gtk_widget_show_all (GTK_WIDGET (gtk_builder_get_object (builder, "wnd_reports")));
}

/**
 * Funcion Principal
 *
 * Carga los valores de rizoma-login.ui y visualiza la ventana del
 * login "login_window"
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


  /* init threads */
  //g_thread_init(NULL);
  
  //gdk_threads_init();
  
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
        }
    } while (*profiles++ != NULL);

  gtk_combo_box_set_model (combo, (GtkTreeModel *)model);
  gtk_combo_box_set_active (combo, 0);

  gtk_widget_show_all ((GtkWidget *)login_window);

  /* enter the GTK main loop */
  //gdk_threads_enter();
  gtk_main();
  //gdk_threads_leave();

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
      gtk_widget_destroy (GTK_WIDGET(gtk_builder_get_object (builder,"login_window")));
      g_object_unref ((gpointer) builder);
      builder = NULL;

      reports_win ();

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
 * Es llamada cuando se presiona el boton "btn_primero" (signal clicked).
 *
 * Esta funcion inserta los primeros 100 ventas en el tree_view_sells.
 *
 */

void
on_btn_primero_clicked()
{
  gint hasta, i;
  gint sell_type;
  GtkTreeIter iter;
  gchar *pago = NULL;
  GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (builder_get (builder, "tree_view_sells"))));

  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_atras")), FALSE);
  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_primero")), FALSE);
  hasta = 100;

  gtk_list_store_clear (store);

  gtk_list_store_clear (GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (builder_get (builder, "tree_view_sell_detail")))));
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "actual_fin_lbl")),  g_strdup_printf ("%d",hasta));
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "actual_inicio_lbl")), "1" );


  for (i = 0 ; i < hasta; i++)
    {
      if (i < hasta )
        {
          sell_type = atoi (PQgetvalue (res_sells, i, 5));
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
          gtk_list_store_append (store, &iter);
          gtk_list_store_set (store, &iter,
                              0, PQgetvalue (res_sells, i, 4),
                              1, PQgetvalue (res_sells, i, 0),
                              2, PQgetvalue (res_sells, i, 1),
                              3, PQgetvalue (res_sells, i, 2),
                              4, PQgetvalue (res_sells, i, 3),
                              5, pago,
                              -1);
        }
    }

  contador = i;
  
  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_adelante")), TRUE);
  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_ultimo")), TRUE);
  return;
  

}

/**
 * Es llamada cuando se presiona el boton "btn_atras" (signal clicked).
 *
 * Esta funcion inserta 100 ventas anteriores a las ventas que se estan
 * visualizando en el tree_view_sells..
 *
 */

void
on_btn_atras_clicked()
{
  gint hasta, i;
  gint sell_type;
  GtkTreeIter iter;
  gchar *pago = NULL;
  GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (builder_get (builder, "tree_view_sells"))));
  
  if ((contador - 100) == 100 || (contador - 100) == 0)
    {
      gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_atras")), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_primero")), FALSE);
      
      hasta = 100;
    }

  else
    {
      hasta = contador - 100;      
    }
    
  if (hasta >= 0)
    {
  gtk_list_store_clear (store);

  gtk_list_store_clear (GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (builder_get (builder, "tree_view_sell_detail")))));
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "actual_fin_lbl")),  g_strdup_printf ("%d",hasta));
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "actual_inicio_lbl")), g_strdup_printf ("%d",hasta - 100 + 1) );


  for (i = hasta - 100 ; i < hasta; i++)
    {
      if (i < hasta )
        {
          sell_type = atoi (PQgetvalue (res_sells, i, 5));
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
          gtk_list_store_append (store, &iter);
          gtk_list_store_set (store, &iter,
                              0, PQgetvalue (res_sells, i, 4),
                              1, PQgetvalue (res_sells, i, 0),
                              2, PQgetvalue (res_sells, i, 1),
                              3, PQgetvalue (res_sells, i, 2),
                              4, PQgetvalue (res_sells, i, 3),
                              5, pago,
                              -1);
        }
    }

    contador = i;
    }
  
  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_adelante")), TRUE);
  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_ultimo")), TRUE);
  return;
  

}

/**
 * Es llamada cuando se presiona el boton "btn_adelante" (signal clicked).
 *
 * Esta funcion inserta las 100 ventas posteriore a las ventas que se estan
 * visualizando en el tree_view_sells.
 *
 */

void
on_btn_adelante_clicked()
{
  gint hasta, i;
  gint sell_type;
  GtkTreeIter iter;
  gchar *pago = NULL;
  GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (builder_get (builder, "tree_view_sells"))));

  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "actual_inicio_lbl")),  g_strdup_printf ("%d",contador + 1));

  
  if ((contador + 100) >= fin)
    hasta = fin;
  else
    {
      hasta = contador + 100;
      gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_atras")), TRUE);
      gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_primero")), TRUE);
      
    }
    
  if (hasta <= fin)
    {
      gtk_list_store_clear (store);
      gtk_list_store_clear (GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (builder_get (builder, "tree_view_sell_detail")))));

      gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "actual_fin_lbl")), g_strdup_printf ("%d",hasta));
      
      for (i = contador ; i < hasta; i++)
        {
          if (i < hasta )
            {
              sell_type = atoi (PQgetvalue (res_sells, i, 5));
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
              gtk_list_store_append (store, &iter);
              gtk_list_store_set (store, &iter,
                                  0, PQgetvalue (res_sells, i, 4),
                                  1, PQgetvalue (res_sells, i, 0),
                                  2, PQgetvalue (res_sells, i, 1),
                                  3, PQgetvalue (res_sells, i, 2),
                                  4, PQgetvalue (res_sells, i, 3),
                                  5, pago,
                                  -1);
            }
        }

      if (hasta == fin)
        {
          hasta = fin + 1;
          gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_adelante")), FALSE);
          gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_ultimo")), FALSE);
          gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_atras")), TRUE);
          gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_primero")), TRUE);
        }
      
      else
        contador = i;
    }

  return;

}

/**
 * Es llamada cuando se presiona el boton "btn_ultimo" (signal clicked).
 *
 * Esta funcion inserta las 100 ultimas ventas en el tree_view_sells.
 *
 */
void
on_btn_ultimo_clicked()
{
  gint hasta, i;
  gint sell_type;
  GtkTreeIter iter;
  gchar *pago = NULL;
  GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (builder_get (builder, "tree_view_sells"))));
  
  hasta = fin;
  contador = fin - (fin % 100);
 
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "actual_inicio_lbl")),  g_strdup_printf ("%d",contador + 1));
  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_atras")), TRUE);
  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_primero")), TRUE);
      
  gtk_list_store_clear (store);
  gtk_list_store_clear (GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (builder_get (builder, "tree_view_sell_detail")))));

  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "actual_fin_lbl")), g_strdup_printf ("%d",fin));

  for (i = contador ; i < hasta; i++)
    {
      if (i < hasta )
        {
          sell_type = atoi (PQgetvalue (res_sells, i, 5));
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
          gtk_list_store_append (store, &iter);
          gtk_list_store_set (store, &iter,
                              0, PQgetvalue (res_sells, i, 4),
                              1, PQgetvalue (res_sells, i, 0),
                              2, PQgetvalue (res_sells, i, 1),
                              3, PQgetvalue (res_sells, i, 2),
                              4, PQgetvalue (res_sells, i, 3),
                              5, pago,
                              -1);
        }
    }

  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_adelante")), FALSE);
  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_ultimo")), FALSE);

  return;

}

/**
 * Es llamada cuando se presiona el boton "btn_ultimo" (signal clicked).
 *
 * Esta funcion inserta las 100 ultimas ventas en el tree_view_sells.
 *
 */
void
on_togglebtn_clicked()
{
  gchar *str_tb = gtk_label_get_text (GTK_LABEL (builder_get (builder, "labelTB")));

  if(strcmp(str_tb, "Activado") == 0)
    {
      gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "labelTB")), "Desactivado");
      gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_print_sells")), FALSE);
      gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label29")), "     a     ");
      gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label30")), "     de     ");
      fill_sells_list(); 
    }
  else
    {
      gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "labelTB")), "Activado");
      gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_adelante")), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_ultimo")), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_atras")), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_primero")), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_print_sells")), TRUE);
      
      GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (builder_get (builder,
												 "tree_view_sells"))));
      gchar *pago = NULL;
      gint tuples, i;
      gint sell_type;
      GtkTreeIter iter;

      gtk_list_store_clear (store);
      gtk_list_store_clear (GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (builder_get (builder,
												 "tree_view_sell_detail")))));

      /* esta funcion  SearchTuplesByDate() llama a una consulta de sql, que
	 retorna los datos de ventas en un intervalo de fechas*/ 
      res_sells = SearchTuplesByDate
	(g_date_get_year (date_begin), g_date_get_month (date_begin), g_date_get_day (date_begin),
	 g_date_get_year (date_end), g_date_get_month (date_end), g_date_get_day (date_end),
	 "fecha", " id, maquina, vendedor, monto, to_char (fecha, 'DD/MM/YY HH24:MI:SS') as fmt_fecha, tipo_venta");

      tuples = PQntuples (res_sells);

      /* si las tuplas son mayores a 100, se activan los botones de adelante y
	 ultimo, y se inactivan los de atras y primero*/

      gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "actual_inicio_lbl")), "");
      gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "actual_fin_lbl")), "");
      gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "fin_lbl")), "");
      gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label29")), "");
      gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label30")), "");
      if (tuples == 0)
	return;

      /* verifica que tipo de venta es*/
      for (i = 0; i < tuples; i++)
	{
	  /* if (i < 100 ) */
	  /*   { */
	  sell_type = atoi (PQgetvalue (res_sells, i, 5));
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
	  gtk_list_store_append (store, &iter);
	  gtk_list_store_set (store, &iter,
			      0, PQgetvalue (res_sells, i, 4),
			      1, PQgetvalue (res_sells, i, 0),
			      2, PQgetvalue (res_sells, i, 1),
			      3, PQgetvalue (res_sells, i, 2),
			      4, PQgetvalue (res_sells, i, 3),
			      5, pago,
			      -1);
	}
      /* else */
      /*   { */
      /*     contador = 100; */
      /*     fin = tuples; */
      /*     return; */
      /*   } */
    }


return;

}




/**
 * Es llamada por la funcion "on_btn_get_stat_clicked()", si se escoge la
 * opcion 0 del switch" 
 *
 * Esta funcion a traves de una consulta sql llama a una funcion que retorna
 * los datos de las ventas en un lapso de tiempo, luego las visualiza a
 * traves de el tree_view correspondiente.
 *
 */



void
fill_sells_list ()
{
  GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (builder_get (builder,
											     "tree_view_sells"))));

  gchar *pago = NULL;
  gint tuples, i;
  gint sell_type;
  GtkTreeIter iter;

  gtk_list_store_clear (store);
  gtk_list_store_clear (GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (builder_get (builder, "tree_view_sell_detail")))));

  /* esta funcion  SearchTuplesByDate() llama a una consulta de sql, que
     retorna los datos de ventas en un intervalo de fechas*/ 
  res_sells = SearchTuplesByDate
    (g_date_get_year (date_begin), g_date_get_month (date_begin), g_date_get_day (date_begin),
     g_date_get_year (date_end), g_date_get_month (date_end), g_date_get_day (date_end),
     "fecha", " id, maquina, vendedor, monto, to_char (fecha, 'DD/MM/YY HH24:MI:SS') as fmt_fecha, tipo_venta");

  tuples = PQntuples (res_sells);

  /* si las tuplas son mayores a 100, se activan los botones de adelante y
     ultimo, y se inactivan los de atras y primero*/
  if (tuples > 100)
    {
      gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_adelante")), TRUE);
      gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_ultimo")), TRUE);
      gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_atras")), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_primero")), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "togglebutton")), TRUE);
    }

  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "actual_inicio_lbl")), "1");
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "actual_fin_lbl")), "100");
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "fin_lbl")), g_strdup_printf ("%d",tuples));
  
  if (tuples == 0)
    return;

  /* verifica que tipo de venta es*/
  for (i = 0; i < tuples; i++)
    {
      if (i < 100 )
        {
          sell_type = atoi (PQgetvalue (res_sells, i, 5));
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
          gtk_list_store_append (store, &iter);
          gtk_list_store_set (store, &iter,
                              0, PQgetvalue (res_sells, i, 4),
                              1, PQgetvalue (res_sells, i, 0),
                              2, PQgetvalue (res_sells, i, 1),
                              3, PQgetvalue (res_sells, i, 2),
                              4, PQgetvalue (res_sells, i, 3),
                              5, pago,
                              -1);
        }
      else
        {
          contador = 100;
          fin = tuples;
          return;
        }
    }
}

/**
 * Es llamada por la funcion "on_btn_get_stat_clicked()", si se escoge la
 * opcion 3 del switch.  
 *
 * Esta funcion a traves de una consulta sql retorna los datos de una
 * devolcion en un lapso de tiempo, luego las visualiza a traves de el
 * tree_view correspondiente.
 *
 */

void
fill_devolucion ()
{
  GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (builder_get (builder, "tree_view_devolucion"))));
  gchar *query;
  gint tuples, i;
  GtkTreeIter iter;
  PGresult *res;

  gtk_list_store_clear (store);
  gtk_list_store_clear (GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (builder_get (builder, "tree_view_devolucion_detalle")))));


  /* consulta de sql que retorna los datos necesarios de una devolucion en
     un intrevalo de tiempo*/
  query = g_strdup_printf ("select fecha, id, monto, proveedor from devolucion where fecha>=to_timestamp ('%.2d %.2d %.4d', 'DD MM YYYY') "
                           "and fecha<=to_timestamp ('%.2d %.2d %.4d', 'DD MM YYYY')  order by id"
                           , g_date_get_day (date_begin),g_date_get_month (date_begin), g_date_get_year (date_begin)
                           , g_date_get_day (date_end) + 1,g_date_get_month (date_end),  g_date_get_year (date_end));
    
  
  res = EjecutarSQL (query);
  g_free (query);

  tuples = PQntuples (res);

  if (tuples == 0) return;
  
  /* visualiza los datos de la devolucion en el tree_view */

    for (i = 0; i < tuples; i++)
      {
        gtk_list_store_append (store, &iter);
        gtk_list_store_set (store, &iter,
                            0, PQgetvalue (res, i, 0),
                            1, PQgetvalue (res, i, 1),
                            2, PQgetvalue (res, i, 2),
                            3, PQgetvalue (res, i, 3),
                            -1);
      }
}

/**
 * Es llamada por la funcion "on_btn_get_stat_clicked()", si se escoge la
 * opcion 3 del switch.  
 *
 * Esta funcion a traves de una consulta sql retorna los montos totales,
 * promedio, y numero de devoluciones en un lapso de tiempo,luego las
 * visualiza  a traves los labels  correspondientes.
 *
 */


void
*fill_totals_dev ()
{
  PGresult *res;

  /* consulta de sql que retona las suma, promedio y numero de devoluciones */
  
  res = EjecutarSQL
    (g_strdup_printf ("select count(*), sum(monto), avg(monto) from devolucion where fecha>=to_timestamp ('%.2d %.2d %.4d', 'DD MM YYYY') "
                      "and fecha<=to_timestamp ('%.2d %.2d %.4d', 'DD MM YYYY')",
                      g_date_get_day (date_begin), g_date_get_month (date_begin), g_date_get_year (date_begin),
                      g_date_get_day (date_end), g_date_get_month (date_end), g_date_get_year (date_end))
     );

  if (res == NULL) return;

  /*se visualizan los datos en las correspondientes etiquetas (labels)*/
  else    
    {
      gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_devolucion_amount_total")),
                            g_strdup_printf ("<b>%s</b>",
                                             PutPoints(g_strdup_printf ("%d",
                                                                        (atoi (PQvaluebycol (res, 0,"sum"))
                                                                         )))));
      gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_devoluciones_num")),
                            g_strdup_printf ("<b>%s</b>",
                                             PutPoints(g_strdup_printf ("%d",
                                                                        (atoi (PQvaluebycol (res, 0,"count"))
                                                                         )))));
      gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_devoluciones_avg")),
                            g_strdup_printf ("<b>%s</b>",
                                             PutPoints(g_strdup_printf ("%d",
                                                                        (atoi (PQvaluebycol (res, 0,"avg"))
                                                                         )))));
    }
 

}

/**
 * Es llamada por la funcion "on_btn_get_stat_clicked()", si se escoge la
 * opcion 0 del switch.  
 *
 * Esta funcion a traves de una consulta sql retorna los montos totales,
 * promedio, y numero de ventas al contado, credito o descuentos en un lapso
 * de tiempo,luego las  visualiza a traves los labels correspondiente.
 *
 */

void
*fill_totals () 
{
  PGresult *res;
  gint total_cash_sell;
  gint total_cash;
  gint total_cash_discount;
  gint total_discount;
  gint total_credit_sell;
  gint total_credit;
  gint total_sell;
  gint total_ventas;

  GtkWidget * progreso = GTK_WIDGET (builder_get (builder, "progressbar"));
  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_get_stat")),FALSE);
    
  /* funcion que retorna el total de la ventas al contado en un intervalo de tiempo*/

  gtk_progress_bar_set_fraction((GtkProgressBar*) progreso,0.1);
  total_cash_sell = GetTotalCashSell (g_date_get_year (date_begin), g_date_get_month (date_begin), g_date_get_day (date_begin),
                                      g_date_get_year (date_end), g_date_get_month (date_end), g_date_get_day (date_end),
                                      &total_cash);

  gtk_progress_bar_set_fraction((GtkProgressBar*) progreso,0.35);


  /* Consulta que retorna el numero de ventas con descuento y la suma total
     con descuento en un intervalo de tiempo */
  res = EjecutarSQL
    (g_strdup_printf ("select * from sells_get_totals (to_date ('%.2d %.2d %.4d', 'DD MM YYYY'), to_date ('%.2d %.2d %.4d', 'DD MM YYYY'))",
                      g_date_get_day (date_begin), g_date_get_month (date_begin), g_date_get_year (date_begin),
                      g_date_get_day (date_end), g_date_get_month (date_end), g_date_get_year (date_end)));

 
 

  gtk_progress_bar_set_fraction((GtkProgressBar*) progreso,0.60);
  
  if (res != NULL)
    {
      total_cash_discount = atoi (PQvaluebycol (res, 0, "total_cash_discount"));
      total_discount = atoi (PQvaluebycol (res, 0, "total_discount"));
    }

  /* Funcion que retorna el total de ventas a credito en un intervarlo de tiempo */
  total_credit_sell = GetTotalCreditSell (g_date_get_year (date_begin), g_date_get_month (date_begin), g_date_get_day (date_begin),
                                          g_date_get_year (date_end), g_date_get_month (date_end), g_date_get_day (date_end),
                                          &total_credit);

  gtk_progress_bar_set_fraction((GtkProgressBar*) progreso,0.65);
  

  /* Funcion que retorna el total de todas las ventas(al contado, con
     descuento y a credito ) en un intervarlo de tiempo */
  total_sell = GetTotalSell (g_date_get_year (date_begin), g_date_get_month (date_begin), g_date_get_day (date_begin),
                             g_date_get_year (date_end), g_date_get_month (date_end), g_date_get_day (date_end),
                             &total_ventas);

  gtk_progress_bar_set_fraction((GtkProgressBar*) progreso,0.85);

  gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_sell_cash_amount")),
                        g_strdup_printf ("<span>$%s</span>",
                                         PutPoints (g_strdup_printf ("%d", total_cash_sell))));

  if (total_cash_sell != 0)
    gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_sell_cash_n")),
                          g_strdup_printf ("<span>%s</span>",
                                           PutPoints (g_strdup_printf ("%d", total_cash))));

  if (total_cash_sell != 0)
    gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_sell_cash_average")),
                          g_strdup_printf ("<span>$%s</span>",
                                           PutPoints (g_strdup_printf ("%d", total_cash_sell / total_cash))));

  gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_sell_credit_amount")),
                        g_strdup_printf ("<span>$%s</span>",
                                         PutPoints (g_strdup_printf ("%d", total_credit_sell))));

  gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_sell_credit_n")),
                        g_strdup_printf ("<span>%s</span>",
                                         PutPoints (g_strdup_printf ("%d", total_credit))));

  if (total_credit_sell != 0)
    gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_sell_credit_average")),
                          g_strdup_printf ("<span>$%s</span>",
                                           PutPoints (g_strdup_printf ("%d", total_credit_sell / total_credit))));

  gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_sell_total_amount")),
                        g_strdup_printf ("<span>$%s</span>",
                                         PutPoints (g_strdup_printf ("%d", total_sell))));

  gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_sell_total_n")),
                        g_strdup_printf ("<span>%s</span>",
                                         PutPoints (g_strdup_printf ("%d",total_ventas))));

   gtk_progress_bar_set_fraction((GtkProgressBar*) progreso,0.95);
  if (total_ventas != 0)
    gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_sell_average")),
                          g_strdup_printf ("<span>$%s</span>",
                                           PutPoints (g_strdup_printf ("%d", total_sell / total_ventas))));

  gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_sell_discount")),
                        g_strdup_printf ("<span>$%s</span>",
                                         PutPoints (g_strdup_printf ("%d", total_cash_discount))));
    

  gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_sell_discount_n")),
                        g_strdup_printf ("<span>%s</span>",
                                         PutPoints (g_strdup_printf ("%d", total_discount))));

  if (total_cash_discount != 0)
  gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_sell_discount_avarage")),
                        g_strdup_printf ("<span>$%s</span>",
                                         PutPoints (g_strdup_printf ("%d", total_cash_discount / total_discount))));

  gtk_progress_bar_set_fraction((GtkProgressBar*) progreso,0);
  gtk_progress_bar_set_text (GTK_PROGRESS_BAR(progreso),"Listo ..");
  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_get_stat")),TRUE);

}

/**
 * Es llamada por la funcion "on_btn_get_stat_clicked()", si se escoge la
 * opcion 1 del switch.  
 *
 * Esta funcion a traves de una consulta sql retorna los productos vendidos
 * pero rankiados y luego los visualiza en el tree_view correspondiente. 
 *
 */


void
fill_products_rank ()
{
  GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (builder_get (builder, "tree_view_sell_rank"))));
  GtkTreeIter iter;
  PGresult *res;
  gint i, tuples;

  /* funcion que llama una funcion sql que retorna los productos vendidos y
     los ordena por mas vendidos ademas de agregarle otros parametros */
  res = ReturnProductsRank (g_date_get_year (date_begin), g_date_get_month (date_begin), g_date_get_day (date_begin),
                            g_date_get_year (date_end), g_date_get_month (date_end), g_date_get_day (date_end));

  tuples = PQntuples (res);

  gtk_list_store_clear (store);

  /* viualiza los productos en el tree_view*/

  for (i = 0; i < tuples; i++)
    {
  
      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
                          0, PQvaluebycol (res, i, "descripcion"),
                          1, PQvaluebycol (res, i, "marca"),
                          2, atoi (PQvaluebycol (res, i, "contenido")),
                          3, PQvaluebycol (res, i, "unidad"),
                          4, g_ascii_strtod (PQvaluebycol (res, i, "amount"), (gchar **)NULL),
                          5, atoi (PQvaluebycol (res, i, "sold_amount")),
                          6, atoi (PQvaluebycol (res, i, "costo")),
                          7, atoi (PQvaluebycol (res, i, "contrib")),
                          8, (((gdouble)atoi (PQvaluebycol (res, i, "contrib")) /
                               atoi (PQvaluebycol (res, i, "costo"))) * 100),
                          -1);
    }


    res = EjecutarSQL
      (g_strdup_printf ("SELECT trunc(sum(sold_amount)) as vendidos, sum(costo) as costo,sum(contrib) as contrib, round(((sum(contrib) / sum(costo)) *100)::numeric , 3)  as margen FROM ranking_ventas (to_timestamp ('%.2d %.2d %.4d', 'DD MM YYYY')::date, to_timestamp ('%.2d %.2d %.4d', 'DD MM YYYY')::date)",
                        g_date_get_day (date_begin), g_date_get_month (date_begin), g_date_get_year (date_begin),
                        g_date_get_day (date_end)+1, g_date_get_month (date_end), g_date_get_year (date_end))
       );

  if (res == NULL) return;
  
  /* visualiza las sumas de los productos en sus respectivos labels */
  
  gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_rank_sold")),
                        g_strdup_printf ("<span size=\"x-large\">$ %s</span>",
                                         PutPoints (PQvaluebycol (res, 0, "vendidos"))));
  gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_rank_cost")),
                        g_strdup_printf ("<span size=\"x-large\">$ %s</span>",
                                         PutPoints (PQvaluebycol (res, 0, "costo"))));
  
  gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_rank_contrib")),
                        g_strdup_printf ("<span size=\"x-large\">$ %s</span>",
                                         PutPoints (PQvaluebycol (res, 0, "contrib"))));

  gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_rank_margin")),
                        g_strdup_printf ("<span size=\"x-large\">$ %s</span>",
                                         PQvaluebycol (res, 0, "margen"))); 
}

/**
 * Es llamada por la funcion "on_btn_get_stat_clicked()", si se escoge la
 * opcion 2 del switch.  
 *
 * Esta funcion a traves de una consulta sql retorna los datos de caja en un
 * intrevalo de tiempo, luego las  visualiza en el tree_view correspondiente.
 *
 * @param date_begin  fecha de inicio de la consulta
 * @param date_end fecha de termino de la consulta
 */

void
fill_cash_box_list ()
{
  GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (builder_get (builder, "tree_view_cash_box_lists"))));
  GtkTreeIter iter;
  PGresult *res;
  gchar *query;
  gint i, tuples;

  /* consultas que retorna los datos de caja en un intervalo de fechas  */
  query = g_strdup_printf ("select id, fecha_inicio, inicio, fecha_termino, termino, perdida from caja where fecha_inicio>=to_timestamp ('%.2d %.2d %.4d', 'DD MM YYYY') "
                               "and (fecha_termino<=to_timestamp ('%.2d %.2d %.4d', 'DD MM YYYY') or fecha_termino is null) order by id"
                               , g_date_get_day (date_begin), g_date_get_month (date_begin), g_date_get_year (date_begin)
                               , g_date_get_day (date_end) + 1, g_date_get_month (date_end), g_date_get_year (date_end));   

  
  res = EjecutarSQL (query);
  g_free (query);

  if (res == NULL) return;

  tuples = PQntuples (res);

  gtk_list_store_clear (store);

  /* visualiza en el tree_view los datos de caja */
  for (i = 0; i < tuples; i++)
    {
      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
                          0, atoi (PQvaluebycol (res, i, "id")),
                          1, PQvaluebycol (res, i, "fecha_inicio"),
                          2, atoi (PQvaluebycol (res, i, "inicio")),
                          3, PQvaluebycol (res, i, "fecha_termino"),
                          4, atoi (PQvaluebycol (res, i, "termino")),
                          -1);
    }
}


void
fill_provider ()
{
  GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (builder_get (builder, "tree_view_proveedores"))));
  GtkTreeIter iter;
  gint i, tuples;
  PGresult *res;

  res = EjecutarSQL ("SELECT nombre, "
                     "       (SELECT SUM (cantidad_ingresada) FROM compra_detalle WHERE id_compra IN (SELECT id FROM compra WHERE rut_proveedor=proveedor.rut)) as unidades,"
                     "       (SELECT SUM (cantidad_ingresada * precio) FROM compra_detalle WHERE id_compra IN (SELECT id FROM compra WHERE rut_proveedor=proveedor.rut))::integer as comprado,"
                     "       round((SELECT (SUM (margen) / COUNT (*)::numeric) FROM compra_detalle WHERE id_compra IN (SELECT id FROM compra WHERE rut_proveedor=proveedor.rut))::numeric,3) as margen,"
                     "       (SELECT SUM (precio_venta - (precio * (margen / 100) +1))  FROM compra_detalle WHERE id_compra IN (SELECT id FROM compra WHERE rut_proveedor=proveedor.rut))::integer as contribucion "
                     "FROM proveedor");

  tuples = PQntuples (res);
  
  if (res == NULL) return;
  
  for (i = 0; i < tuples; i++)
    {
      
      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
                          0, PQvaluebycol (res, i, "nombre"),
			  1, g_strtod(PUT(PQvaluebycol (res, i, "unidades")),(gchar **)NULL),
			  2, atoi(PQvaluebycol (res, i, "comprado")),
                          3, g_strtod(PQvaluebycol (res, i, "margen"),(gchar **)NULL),
                          4, atoi(PQvaluebycol (res, i, "contribucion")),
			  -1);
			  
    }
}


/**
 * Es llamado por on_btn_get_stat_clicked (void)
 *
 * Obtiene la información de cuadratura y la depliega en en treeview "tree_view_cuadratura"
 *
 */

void
fill_cuadratura ()
{
  GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (builder_get (builder, "tree_view_cuadratura"))));
  GtkTreeIter iter;
  gint i, tuples;
  PGresult *res;
  char *sql;
  
  gtk_list_store_clear (store);
  
  sql = g_strdup_printf ( "SELECT to_char(fecha, 'DD MM YYYY') AS fecha FROM compra ORDER BY fecha ASC" );

  char * fecha;
  fecha = g_strdup_printf ("%.2d %.2d %.4d", g_date_get_day (date_begin), g_date_get_month (date_begin), g_date_get_year (date_begin));
  res = EjecutarSQL (sql);
  g_free (sql);
  tuples = PQntuples (res);

  // Si la fecha de la primera compra es igual a la fecha seleccionada
  if (strcmp (PQvaluebycol (res, 0, "fecha"), fecha) == 0)
    sql = g_strdup_printf ( "SELECT descripcion, marca, stock_inicial, compras_periodo, ventas_periodo, devoluciones_periodo, mermas_periodo, stock_teorico "
			    "FROM producto_en_periodo('%.4d-%.2d-%.2d', TRUE)",
			    g_date_get_year (date_begin), g_date_get_month (date_begin), g_date_get_day (date_begin) );
  else
    sql = g_strdup_printf ( "SELECT descripcion, marca, stock_inicial, compras_periodo, ventas_periodo, devoluciones_periodo, mermas_periodo, stock_teorico "
			    "FROM producto_en_periodo('%.4d-%.2d-%.2d', FALSE)",
			    g_date_get_year (date_begin), g_date_get_month (date_begin), g_date_get_day (date_begin) );
  
  res = EjecutarSQL (sql);
  tuples = PQntuples (res);
  g_free (sql);
  
  if (res == NULL) return;
  
  for (i = 0; i < tuples; i++)
    {
      
      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
                          0, PQvaluebycol (res, i, "descripcion"),
			  1, PQvaluebycol (res, i, "marca"),
			  2, g_strtod(PUT(PQvaluebycol (res, i, "stock_inicial")),(gchar **)NULL),
			  3, g_strtod(PUT(PQvaluebycol (res, i, "compras_periodo")),(gchar **)NULL),
                          4, g_strtod(PUT(PQvaluebycol (res, i, "ventas_periodo")),(gchar **)NULL),
                          5, g_strtod(PUT(PQvaluebycol (res, i, "devoluciones_periodo")),(gchar **)NULL),
			  6, g_strtod(PUT(PQvaluebycol (res, i, "mermas_periodo")),(gchar **)NULL),
			  7, g_strtod(PUT(PQvaluebycol (res, i, "stock_teorico")),(gchar **)NULL),
			  //8, 0,
			  //9, 0,
			  -1);
			  
    }
}


/**
 * Es llamada cuando se presiona el boton "btn_get_stat" (signal clicked)
 *
 * Esta funcion extrae las fechas ingresadas, y segun notebook se escoge una opcion
 *
 */

void
on_btn_get_stat_clicked ()
{
  
  GtkNotebook *notebook = GTK_NOTEBOOK (builder_get (builder, "ntbk_reports"));
  gint page_num = gtk_notebook_get_current_page (notebook);
  if (page_num == 4)
    {
      /* Informe de proveedores */
      fill_provider ();
    }

  else
    {
      const gchar *str_begin = gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_date_begin")));
      const gchar *str_end = gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_date_end")));
      date_begin = g_date_new ();
      date_end = g_date_new ();
      GtkWidget * progreso = GTK_WIDGET (builder_get (builder, "progressbar"));
  
      if (g_str_equal (str_begin, "") || g_str_equal (str_end, "")) return;

      g_date_set_parse (date_begin, str_begin);
      g_date_set_parse (date_end, str_end);
    
      switch (page_num)
        {
    
        case 0:
          /* Informe de ventas */
          fill_sells_list(); 
          gtk_progress_bar_set_text (GTK_PROGRESS_BAR(progreso),"Cargando ..");  
          clean_container (GTK_CONTAINER (gtk_widget_get_parent (GTK_WIDGET (builder_get (builder, "lbl_sell_cash_amount")))));

          /* llama a la funcion fill_totals() en un nuevo thread(hilo)*/
          //g_thread_create(fill_totals, NULL, FALSE, &error);
          fill_totals();
          break;
        case 1:
          /* Informe de ranking de ventas */
          fill_products_rank ();
          break;
        case 2:
          /* Informe de caja*/
          fill_cash_box_list ();
          break;
        case 3:
          /* Informe de devolucion */
          fill_devolucion ();
          /* llama a la funcion fill_totals_dev() en un nuevo thread(hilo)*/
          fill_totals_dev();
          //g_thread_create(fill_totals_dev, NULL, FALSE, &error);
          break;
	case 5:
	  fill_cuadratura ();
	  break;
	  
        default:
          break;
        }
    }
}


void
on_tree_view_cuadratura_grab_focus (GtkWidget *widget, gpointer user_data)
{
  // TODO: La señal switch entrega el numero de la página antes de cambiar de página, osea no da el numero 
  // de la página a la que cambiaste sino que da el numero de página desde donde te cambiaste.

  /*Si se selecciona la "pagina 5" (la pestaña cuadratura) y el entry de la fecha de termino esta habilitado*/
  /*if (gtk_notebook_get_current_page (GTK_NOTEBOOK (builder_get (builder, "ntbk_reports"))) == 5
      && gtk_widget_get_sensitive (GTK_ENTRY (gtk_builder_get_object (builder, "entry_date_end"))) == TRUE)
    {
      gtk_widget_set_sensitive (GTK_ENTRY (gtk_builder_get_object (builder, "entry_date_end")), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "button3")), FALSE);
    }
  else if (gtk_widget_get_sensitive (GTK_ENTRY (gtk_builder_get_object (builder, "entry_date_end"))) == FALSE
	   && gtk_notebook_get_current_page (GTK_NOTEBOOK (builder_get (builder, "ntbk_reports"))) != 6)
    {
      gtk_widget_set_sensitive (GTK_ENTRY (gtk_builder_get_object (builder, "entry_date_end")), TRUE);
      gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "button3")), TRUE);
    }*/
}
