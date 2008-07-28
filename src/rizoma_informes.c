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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tipos.h"
#include "postgres-functions.h"
#include "config_file.h"
#include "utils.h"
#include "encriptar.h"
#include "rizoma_errors.h"

GtkBuilder *builder;

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

      res = EjecutarSQL
        (g_strdup_printf
         ("SELECT descripcion, marca, contenido, unidad, cantidad, venta_detalle.precio, (cantidad * venta_detalle.precio)::int AS "
          "monto FROM venta_detalle, producto WHERE producto.barcode=venta_detalle.barcode and id_venta=%s", idventa));

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

void
reports_win (void)
{
  GtkListStore *store;
  GtkTreeView *treeview;
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;
  GtkTreeSelection *selection;

  GError *error = NULL;

  builder = gtk_builder_new ();

  gtk_builder_add_from_file (builder, DATADIR"/ui/rizoma-informes.ui", &error);

  if (error != NULL) {
    g_printerr ("%s\n", error->message);
  }

  gtk_builder_add_from_file (builder, DATADIR"/ui/rizoma-common.ui", &error);

  if (error != NULL) {
    g_printerr ("%s\n", error->message);
  }
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
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("ID Venta", renderer,
                                                     "text", 1,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Maq.", renderer,
                                                     "text", 2,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Vendedor", renderer,
                                                     "text", 3,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Monto", renderer,
                                                     "text", 4,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_alignment (column, 0.5);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Tipo Pago", renderer,
                                                     "text", 5,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);



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
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Unitario", renderer,
                                                     "text", 2,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Total", renderer,
                                                     "text", 3,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

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
  gtk_tree_view_column_set_max_width (column, 160);
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
  gtk_tree_view_column_set_max_width (column, 90);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Medida", renderer,
                                                     "text", 2,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
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
  gtk_tree_view_append_column (treeview, column);
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
  gtk_tree_view_append_column (treeview, column);
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
  gtk_tree_view_append_column (treeview, column);
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
  gtk_tree_view_append_column (treeview, column);
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
  gtk_tree_view_append_column (treeview, column);
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
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 8);
  gtk_tree_view_column_set_min_width (column, 50);
  gtk_tree_view_column_set_max_width (column, 50);
  gtk_tree_view_column_set_resizable (column, FALSE);

  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)8, NULL);


  /* End Sells Rank */

  gtk_widget_show_all (GTK_WIDGET (gtk_builder_get_object (builder, "wnd_reports")));
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

void
fill_sells_list (GDate *date_begin, GDate *date_end)
{
  GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (builder_get (builder, "tree_view_sells"))));

  gchar *pago = NULL;
  gint tuples, i;
  gint sell_type;
  GtkTreeIter iter;
  PGresult *res;

  res = SearchTuplesByDate
    (g_date_get_year (date_begin), g_date_get_month (date_begin), g_date_get_day (date_begin),
     g_date_get_year (date_end), g_date_get_month (date_end), g_date_get_day (date_end),
     "fecha", "id, maquina, vendedor, monto, date_part('day', fecha), date_part('month', fecha), "
     "date_part('year', fecha), date_part('hour', fecha), date_part('minute', fecha), tipo_venta");


  tuples = PQntuples (res);

  if (tuples == 0)
    return;

  gtk_list_store_clear (store);

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

      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
                          0,
                          g_strdup_printf
                          ("%.2d/%.2d/%s %.2d:%.2d", atoi (PQgetvalue (res, i, 4)),
                           atoi (PQgetvalue (res, i, 5)), PQgetvalue (res, i, 6),
                           atoi (PQgetvalue (res, i, 7)), atoi (PQgetvalue (res, i, 8))),
                          1, PQgetvalue (res, i, 0),
                          2, PQgetvalue (res, i, 1),
                          3, PQgetvalue (res, i, 2),
                          4, PQgetvalue (res, i, 3),
                          5, pago,
                          -1);
    }

}

void
fill_totals (GDate *date_begin, GDate *date_end)
{
  gint total_cash_sell;
  gint total_cash;
  gint total_credit_sell;
  gint total_credit;
  gint total_sell;
  gint total_ventas;

  total_cash_sell = GetTotalCashSell (g_date_get_year (date_begin), g_date_get_month (date_begin), g_date_get_day (date_begin),
                                      g_date_get_year (date_end), g_date_get_month (date_end), g_date_get_day (date_end),
                                      &total_cash);

  total_credit_sell = GetTotalCreditSell (g_date_get_year (date_begin), g_date_get_month (date_begin), g_date_get_day (date_begin),
                                          g_date_get_year (date_end), g_date_get_month (date_end), g_date_get_day (date_end),
                                          &total_credit);

  total_sell = GetTotalSell (g_date_get_year (date_begin), g_date_get_month (date_begin), g_date_get_day (date_begin),
                             g_date_get_year (date_end), g_date_get_month (date_end), g_date_get_day (date_end),
                             &total_ventas);

  gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_sell_cash_amount")),
                        g_strdup_printf ("<span>$%s</span>",
                                         PutPoints (g_strdup_printf ("%d", total_cash_sell))));

  if (total_cash_sell != 0)
    gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_sell_cash_n")),
                          g_strdup_printf ("<span>%d</span>", total_cash));

  if (total_cash_sell != 0)
    gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_sell_cash_average")),
                          g_strdup_printf ("<span>$%s</span>",
                                           PutPoints (g_strdup_printf ("%d", total_cash_sell / total_cash))));

  gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_sell_credit_amount")),
                        g_strdup_printf ("<span>$%s</span>",
                                         PutPoints (g_strdup_printf ("%d", total_credit_sell))));

  gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_sell_credit_n")),
                        g_strdup_printf ("<span>%d</span>", total_credit));

  if (total_credit_sell != 0)
    gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_sell_credit_average")),
                          g_strdup_printf ("<span>$%s</span>",
                                           PutPoints (g_strdup_printf ("%d", total_credit_sell / total_credit))));

  gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_sell_total_amount")),
                        g_strdup_printf ("<span>$%s</span>",
                                         PutPoints (g_strdup_printf ("%d", total_sell))));

  gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_sell_total_n")),
                        g_strdup_printf ("<span>%d</span>", total_ventas));

  if (total_ventas != 0)
    gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_sell_average")),
                          g_strdup_printf ("<span>$%s</span>",
                                           PutPoints (g_strdup_printf ("%d", total_sell / total_ventas))));
}

void
fill_products_rank (GDate *date_begin, GDate *date_end)
{
  GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (builder_get (builder, "tree_view_sell_rank"))));
  GtkTreeIter iter;
  PGresult *res;
  gint i, tuples;
  gint vendidos = 0, costo = 0, contrib = 0;
  gdouble margen = 0;

  res = ReturnProductsRank (g_date_get_year (date_begin), g_date_get_month (date_begin), g_date_get_day (date_begin),
                            g_date_get_year (date_end), g_date_get_month (date_end), g_date_get_day (date_end));

  tuples = PQntuples (res);

  gtk_list_store_clear (store);

  for (i = 0; i < tuples; i++)
    {
      vendidos += atoi (PQvaluebycol (res, i, "sold_amount"));
      costo += atoi (PQvaluebycol (res, i, "costo"));
      contrib += atoi (PQvaluebycol (res, i, "contrib"));

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

  gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_rank_sold")),
                        g_strdup_printf ("<span size=\"x-large\">$%s</span>", PutPoints
                                         (g_strdup_printf ("%d", vendidos))));
  gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_rank_cost")),
                        g_strdup_printf ("<span size=\"x-large\">$%s</span>", PutPoints
                                         (g_strdup_printf ("%d", costo))));
  gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_rank_contrib")),
                        g_strdup_printf ("<span size=\"x-large\">$%s</span>", PutPoints
                                         (g_strdup_printf ("%d", contrib))));

  margen = (((gdouble) contrib / costo) * 100);

  if (margen > 0)
    gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_rank_margin")),
                          g_strdup_printf ("<span size=\"x-large\">%.2f%%</span>", margen));
  else
    gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_rank_margin")),
                          g_strdup_printf ("<span size=\"x-large\">0%%</span>"));
}

void
fill_caja_data (GDate *date)
{
  PGresult *res;
  gchar *query;
  gint year = g_date_get_year (date);
  gint month = g_date_get_month (date);
  gint day = g_date_get_day (date);

  clean_container (GTK_CONTAINER (builder_get (builder, "table_data")));

  query = g_strdup_printf
    ("SELECT (SELECT inicio FROM caja WHERE date_part('year', fecha_inicio)=%d AND date_part('month', fecha_inicio)=%d AND date_part('day', fecha_inicio)=%d order by fecha_inicio asc limit 1) AS inicio, (SELECT SUM (monto - descuento) FROM venta WHERE date_part('year', fecha)=%d AND date_part('month', fecha)=%d AND date_part('day', fecha)=%d AND tipo_venta=%d) AS ventas_efect, (SELECT SUM(t1.monto) FROM cheques AS t1 WHERE id_venta IN (SELECT id FROM venta WHERE date_part('year', fecha)=%d AND date_part('month', fecha)=%d AND date_part('day', fecha)=%d AND date_part('year', fecha)=date_part('year', t1.fecha) AND date_part('month', fecha)=date_part('month', t1.fecha) AND date_part('day', fecha)=date_part('day', t1.fecha))) AS ventas_doc, (SELECT SUM (monto) FROM egreso WHERE date_part('year', fecha)=%d AND date_part('month', fecha)=%d AND date_part('day', fecha)=%d AND tipo=1) AS retiros, (SELECT SUM(monto_abonado) FROM abono WHERE date_part('year', fecha_abono)=%d AND date_part('month', fecha_abono)=%d AND date_part('day', fecha_abono)=%d) AS pago_credit, (SELECT SUM(monto) FROM ingreso WHERE date_part('year', fecha)=%d AND date_part('month', fecha)=%d AND date_part('day', fecha)=%d) AS otros, (SELECT SUM (monto) FROM egreso WHERE date_part('year', fecha)=%d AND date_part('month', fecha)=%d AND date_part('day', fecha)=%d AND tipo=3) AS gasto, (SELECT SUM (monto) FROM egreso WHERE date_part('year', fecha)=%d AND date_part('month', fecha)=%d AND date_part('day', fecha)=%d AND tipo=2) AS otros_egresos, (SELECT SUM(monto) FROM factura_compra WHERE id IN (SELECT id_fact FROM pagos  WHERE caja='t' AND date_part('year', fecha)=%d AND date_part('month', fecha)=%d AND date_part('day', fecha)=%d)) AS facturas",
     year, month+1, day, year, month+1, day, CASH, year, month+1, day, year, month+1,
     day, year, month+1, day, year, month+1, day, year, month+1, day, year,
     month+1, day, year, month+1, day);
  g_print ("%s\n", query);
  res = EjecutarSQL (query);
  g_free (query);

  if (res != NULL && PQntuples (res) != 0 && strcmp (PQgetvalue (res, 0, 0), "") != 0)
    {
      if (strcmp (PQgetvalue (res, 0, 0), "") != 0)
        gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_mbox_start")),
                              g_strdup_printf ("<b>%s</b>", PutPoints (PQgetvalue (res, 0, 0))));

      if (strcmp (PQgetvalue (res, 0, 1), "") != 0)
        gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_mbox_cash")),
                              g_strdup_printf ("<b>%s</b>", PutPoints (PQgetvalue (res, 0, 1))));

      if (strcmp (PQgetvalue (res, 0, 2), "") != 0)
        gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_mbox_doc")),
                              g_strdup_printf ("<b>%s</b>", PutPoints (PQgetvalue (res, 0, 2))));

      /* if (strcmp (PQgetvalue (res, 0, 4), "") != 0) */
      /*   gtk_label_set_markup (GTK_LABEL (pago_ventas), */
      /*                         g_strdup_printf ("<b>%s</b>", PutPoints (PQgetvalue (res, 0, 4)))); */

      if (strcmp (PQgetvalue (res, 0, 5), "") != 0)
        gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_mbox_other")),
                              g_strdup_printf ("<b>%s</b>", PutPoints (PQgetvalue (res, 0, 5))));

      gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_mbox_ingr_total")),
                            g_strdup_printf
                            ("<b>$\t%s</b>", PutPoints
                             (g_strdup_printf
                              ("%d", atoi (PQgetvalue (res, 0, 0)) +
                               atoi (PQgetvalue (res, 0, 1)) + atoi (PQgetvalue (res, 0, 2)) +
                               atoi (PQgetvalue (res, 0, 4)) + atoi (PQgetvalue (res, 0, 5))))));

      if (strcmp (PQgetvalue (res, 0, 8), "") != 0)
        gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_mbox_out_invoice")),
                              g_strdup_printf ("<b>%s</b>", PutPoints (PQgetvalue (res, 0, 8))));

      if (strcmp (PQgetvalue (res, 0, 3), "") != 0)
        gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_mbox_out")),
                              g_strdup_printf ("<b>%s</b>", PutPoints (PQgetvalue (res, 0, 3))));

      /* if (strcmp (PQgetvalue (res, 0, 6), "") != 0) */
      /*   gtk_label_set_markup (GTK_LABEL (gastos_corrientes), */
      /*                         g_strdup_printf ("<b>%s</b>", PutPoints (PQgetvalue (res, 0, 6)))); */

      if (strcmp (PQgetvalue (res, 0, 7), "") != 0)
        gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_mbox_out_other")),
                              g_strdup_printf ("<b>%s</b>", PutPoints (PQgetvalue (res, 0, 7))));

      gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_mbox_out_total")),
                            g_strdup_printf
                            ("<b>$\t%s</b>", PutPoints
                             (g_strdup_printf
                              ("%d", atoi (PQgetvalue (res, 0, 8)) +
                               atoi (PQgetvalue (res, 0, 3)) + atoi (PQgetvalue (res, 0, 6)) +
                               atoi (PQgetvalue (res, 0, 7))))));

      gtk_label_set_markup
        (GTK_LABEL (builder_get (builder, "lbl_money_box_total")),
         g_strdup_printf
         ("<span size=\"xx-large\"><b>$\t%s</b></span>",
          PutPoints
          (g_strdup_printf
           ("%d",
            (atoi (PQgetvalue (res, 0, 0)) + atoi (PQgetvalue (res, 0, 1)) +
             atoi (PQgetvalue (res, 0, 2)) + atoi (PQgetvalue (res, 0, 4)) +
             atoi (PQgetvalue (res, 0, 5))) -
            (atoi (PQgetvalue (res, 0, 8)) +
             atoi (PQgetvalue (res, 0, 3)) + atoi (PQgetvalue (res, 0, 6)) +
             atoi (PQgetvalue (res, 0, 7)))))));


    }
}


void
on_btn_get_stat_clicked ()
{
  GtkNotebook *notebook = GTK_NOTEBOOK (builder_get (builder, "ntbk_reports"));
  gint page_num = gtk_notebook_get_current_page (notebook);;
  const gchar *str_begin = gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_date_begin")));
  const gchar *str_end = gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_date_end")));
  GDate *date_begin = g_date_new ();
  GDate *date_end = g_date_new ();
  GDate *date = g_date_new ();

  if (page_num != 2)
    {
      if (g_str_equal (str_begin, "") || g_str_equal (str_end, "")) return;

      g_date_set_parse (date_begin, str_begin);
      g_date_set_parse (date_end, str_end);
    }

  switch (page_num)
    {
    case 0:
      fill_sells_list (date_begin, date_end);
      fill_totals (date_begin, date_end);
      break;
    case 1:
      fill_products_rank (date_begin, date_end);
      break;
    case 2:
      g_date_set_parse (date, gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_money_box_date"))));
      if (g_date_valid (date)) fill_caja_data (date);
      break;
    default:
      break;
    }
}
