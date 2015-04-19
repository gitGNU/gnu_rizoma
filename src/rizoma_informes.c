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
/* #include <pthread.h> */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "tipos.h"
#include "postgres-functions.h"
#include "config_file.h"
#include "utils.h"
#include "errors.h"
#include "encriptar.h"
#include "rizoma_errors.h"
#include "printing.h"

GtkBuilder *builder;
PGresult *res_sells;
gint contador, fin;
GDate *date_begin;
GDate *date_end;
gint hrIni, hrFin, minIni, minFin;
//int flagProgress;

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
  gint idventa;
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
         ("SELECT descripcion, marca, contenido, unidad, cantidad, vd.precio, "
	  "       round(vd.iva) AS iva, round(vd.otros) AS otros, "
	  "       round((cantidad * vd.precio)) AS monto "
	  "FROM venta_detalle vd, producto "
	  "WHERE producto.barcode=vd.barcode AND id_venta=%d", idventa));

      tuples = PQntuples (res);

      for (i = 0; i < tuples; i++)
        {
          gtk_list_store_append (store_detail, &iter);
          gtk_list_store_set (store_detail, &iter,
                              0, g_strdup_printf ("%s %s %s %s", PQvaluebycol (res, i, "descripcion"),
                                                  PQvaluebycol (res, i, "marca"), PQvaluebycol (res, i, "contenido"),
                                                  PQvaluebycol (res, i, "unidad")),
                              1, strtod (PUT (g_strdup (PQvaluebycol (res, i, "cantidad"))), (char **)NULL),
                              2, strtod (PUT (g_strdup (PQvaluebycol (res, i, "precio"))), (char **)NULL),
			      3, atoi (g_strdup (PQvaluebycol (res, i, "iva"))),
                              4, atoi (g_strdup (PQvaluebycol (res, i, "otros"))),
                              5, atoi (PQvaluebycol (res, i, "monto")),
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
         ("SELECT descripcion, marca, contenido, unidad, cantidad, devolucion_detalle.precio, round((cantidad * devolucion_detalle.precio)) AS "
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
 * Es llamada por el evento "change" del treeview
 * "treeview_sells_exempt_tax"
 *
 * Rellena el detalle de la venta exenta seleccionada en el
 * treeview "treeview_sells_exempt_tax_details"
 *
 * @param: GtkTreeSelection treeselection
 * @param: gpointer user_data
 */
void
fill_exempt_sells_details (GtkTreeSelection *treeselection, gpointer user_data)
{
  GtkTreeView *tree = GTK_TREE_VIEW (builder_get (builder, "treeview_sells_exempt_tax"));
  GtkTreeModel *model = gtk_tree_view_get_model (tree);
  GtkTreeSelection *selection = gtk_tree_view_get_selection (tree);
  GtkListStore *store_detail = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (builder_get (builder, "treeview_sells_exempt_tax_details"))));
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
         ("SELECT codigo_corto, descripcion, marca, contenido, unidad, cantidad, "
	  "       venta_detalle.precio, round((cantidad * venta_detalle.precio)) AS monto "
	  "FROM venta_detalle, producto "
	  "WHERE producto.barcode=venta_detalle.barcode "
	  "AND id_venta=%s", idventa));

      tuples = PQntuples (res);

      for (i = 0; i < tuples; i++)
        {
          gtk_list_store_append (store_detail, &iter);
          gtk_list_store_set (store_detail, &iter,
			      0, g_strdup_printf ("%s", PQvaluebycol (res, i, "codigo_corto")),
                              1, g_strdup_printf ("%s %s %s %s", PQvaluebycol (res, i, "descripcion"),
                                                  PQvaluebycol (res, i, "marca"), PQvaluebycol (res, i, "contenido"),
                                                  PQvaluebycol (res, i, "unidad")),
                              2, strtod (PUT (g_strdup (PQvaluebycol (res, i, "cantidad"))), (char **)NULL),
                              3, strtod (PUT (g_strdup (PQvaluebycol (res, i, "precio"))), (char **)NULL),
                              4, atoi (PQvaluebycol (res, i, "monto")),
                              -1);
        }
    }
}

/**
 *
 *
 */
void
change_sell_rank_mp (GtkCellRendererText *cell, gchar *path_string, gchar *stock_fisico, gpointer data)
{
  GtkTreeView *tree = GTK_TREE_VIEW (builder_get (builder, "treeview_sell_rank_mp"));
  GtkTreeModel *model = gtk_tree_view_get_model (tree);
  GtkTreeSelection *selection = gtk_tree_view_get_selection (tree);
  GtkListStore *store_sr_deriv = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (builder_get (builder, "treeview_sell_rank_deriv"))));
  GtkTreeIter iter;
  gchar *barcode_producto;
  gint i, tuples;
  PGresult *res;

  if (gtk_tree_selection_get_selected (selection, NULL, &iter) == TRUE)
    {
      gtk_tree_model_get (model, &iter,
                          0, &barcode_producto,
                          -1);

      gtk_list_store_clear (GTK_LIST_STORE (store_sr_deriv));

      res = ReturnDerivProductsRank (g_date_get_year (date_begin), g_date_get_month (date_begin), g_date_get_day (date_begin),
				     g_date_get_year (date_end), g_date_get_month (date_end), g_date_get_day (date_end), barcode_producto);

      tuples = PQntuples (res);

      for (i = 0; i < tuples; i++)
        {
          gtk_list_store_append (store_sr_deriv, &iter);
          gtk_list_store_set (store_sr_deriv, &iter,
			      0, g_strdup (PQvaluebycol (res, i, "descripcion")),
			      1, g_strdup (PQvaluebycol (res, i, "marca")),
			      2, g_strdup (PQvaluebycol (res, i, "contenido")),
			      3, g_strdup (PQvaluebycol (res, i, "unidad")),
			      4, strtod (PUT (g_strdup (PQvaluebycol (res, i, "cantidad"))), (char **)NULL),
			      5, lround (strtod (PUT (g_strdup (PQvaluebycol (res, i, "monto_vendido"))), (char **)NULL)),
			      6, lround (strtod (PUT (g_strdup (PQvaluebycol (res, i, "costo"))), (char **)NULL)),
			      7, lround (strtod (PUT (g_strdup (PQvaluebycol (res, i, "contribucion"))), (char **)NULL)),
			      8, ((strtod (PUT (g_strdup (PQvaluebycol (res, i, "contribucion"))), (char **)NULL) /
				   strtod (PUT (g_strdup (PQvaluebycol (res, i, "costo"))), (char **)NULL)) * 100),
                              -1);
        }
    }
}


/**
 *
 *
 */
void
change_sell_rank_mc (GtkCellRendererText *cell, gchar *path_string, gchar *stock_fisico, gpointer data)
{
  GtkTreeView *tree = GTK_TREE_VIEW (builder_get (builder, "treeview_sell_rank_mc"));
  GtkTreeModel *model = gtk_tree_view_get_model (tree);
  GtkTreeSelection *selection = gtk_tree_view_get_selection (tree);
  GtkListStore *store_sr_comp = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (builder_get (builder, "treeview_sell_rank_comp"))));
  GtkTreeIter iter;
  gchar *barcode_producto;
  gint i, tuples;
  PGresult *res;

  if (gtk_tree_selection_get_selected (selection, NULL, &iter) == TRUE)
    {
      gtk_tree_model_get (model, &iter,
                          0, &barcode_producto,
                          -1);

      gtk_list_store_clear (GTK_LIST_STORE (store_sr_comp));

      res = ReturnCompProductsRank (g_date_get_year (date_begin), g_date_get_month (date_begin), g_date_get_day (date_begin),
				    g_date_get_year (date_end), g_date_get_month (date_end), g_date_get_day (date_end), barcode_producto);

      tuples = PQntuples (res);

      for (i = 0; i < tuples; i++)
        {
          gtk_list_store_append (store_sr_comp, &iter);
          gtk_list_store_set (store_sr_comp, &iter,
			      0, g_strdup (PQvaluebycol (res, i, "descripcion")),
			      1, g_strdup (PQvaluebycol (res, i, "marca")),
			      2, g_strdup (PQvaluebycol (res, i, "contenido")),
			      3, g_strdup (PQvaluebycol (res, i, "unidad")),
			      4, strtod (PUT (g_strdup (PQvaluebycol (res, i, "cantidad"))), (char **)NULL),
			      5, lround (strtod (PUT (g_strdup (PQvaluebycol (res, i, "monto_vendido"))), (char **)NULL)),
			      6, lround (strtod (PUT (g_strdup (PQvaluebycol (res, i, "costo"))), (char **)NULL)),
			      7, lround (strtod (PUT (g_strdup (PQvaluebycol (res, i, "contribucion"))), (char **)NULL)),
			      8, ((strtod (PUT (g_strdup (PQvaluebycol (res, i, "contribucion"))), (char **)NULL) /
				   strtod (PUT (g_strdup (PQvaluebycol (res, i, "costo"))), (char **)NULL)) * 100),
                              -1);
        }
    }
}


/*------------- TRANSFER RANK (CHANGE EVENT) START ----------------------*/

/**
 *
 *
 */
void
change_transfer_rank_mp (GtkCellRendererText *cell, gchar *path_string, gchar *stock_fisico, gpointer data)
{
  GtkTreeView *tree = GTK_TREE_VIEW (builder_get (builder, "treeview_mp_transfer_rank"));
  GtkTreeModel *model = gtk_tree_view_get_model (tree);
  GtkTreeSelection *selection = gtk_tree_view_get_selection (tree);
  GtkListStore *store_sr_deriv = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (builder_get (builder, "treeview_derivates_transfer_rank"))));
  GtkTreeIter iter;
  gchar *barcode_producto;
  gint i, tuples;
  PGresult *res;

  if (gtk_tree_selection_get_selected (selection, NULL, &iter) == TRUE)
    {
      gtk_tree_model_get (model, &iter,
                          0, &barcode_producto,
                          -1);

      gtk_list_store_clear (GTK_LIST_STORE (store_sr_deriv));

      res = ReturnDerivTransferRank (g_date_get_year (date_begin), g_date_get_month (date_begin), g_date_get_day (date_begin),
				     g_date_get_year (date_end), g_date_get_month (date_end), g_date_get_day (date_end), barcode_producto);

      tuples = PQntuples (res);

      for (i = 0; i < tuples; i++)
        {
          gtk_list_store_append (store_sr_deriv, &iter);
          gtk_list_store_set (store_sr_deriv, &iter,
			      0, g_strdup (PQvaluebycol (res, i, "barcode")),
			      1, g_strdup (PQvaluebycol (res, i, "descripcion")),
			      2, g_strdup (PQvaluebycol (res, i, "marca")),
			      3, g_strdup (PQvaluebycol (res, i, "contenido")),
			      4, g_strdup (PQvaluebycol (res, i, "unidad")),
			      5, strtod (PUT (g_strdup (PQvaluebycol (res, i, "cantidad"))), (char **)NULL),
			      6, lround (strtod (PUT (g_strdup (PQvaluebycol (res, i, "costo"))), (char **)NULL)),
                              -1);
        }
    }
}


/**
 *
 *
 */
void
change_transfer_rank_mc (GtkCellRendererText *cell, gchar *path_string, gchar *stock_fisico, gpointer data)
{
  GtkTreeView *tree = GTK_TREE_VIEW (builder_get (builder, "treeview_offer_transfer_rank"));
  GtkTreeModel *model = gtk_tree_view_get_model (tree);
  GtkTreeSelection *selection = gtk_tree_view_get_selection (tree);
  GtkListStore *store_tr_comp = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (builder_get (builder, "treeview_components_transfer_rank"))));
  GtkTreeIter iter;
  gchar *barcode_producto;
  gint i, tuples;
  PGresult *res;

  if (gtk_tree_selection_get_selected (selection, NULL, &iter) == TRUE)
    {
      gtk_tree_model_get (model, &iter,
                          0, &barcode_producto,
                          -1);

      gtk_list_store_clear (GTK_LIST_STORE (store_tr_comp));

      res = ReturnCompTransferRank (g_date_get_year (date_begin), g_date_get_month (date_begin), g_date_get_day (date_begin),
				    g_date_get_year (date_end), g_date_get_month (date_end), g_date_get_day (date_end), barcode_producto);

      tuples = PQntuples (res);

      for (i = 0; i < tuples; i++)
        {
          gtk_list_store_append (store_tr_comp, &iter);
          gtk_list_store_set (store_tr_comp, &iter,
			      0, g_strdup (PQvaluebycol (res, i, "barcode")),
			      1, g_strdup (PQvaluebycol (res, i, "descripcion")),
			      2, g_strdup (PQvaluebycol (res, i, "marca")),
			      3, g_strdup (PQvaluebycol (res, i, "contenido")),
			      4, g_strdup (PQvaluebycol (res, i, "unidad")),
			      5, strtod (PUT (g_strdup (PQvaluebycol (res, i, "cantidad"))), (char **)NULL),
			      6, lround (strtod (PUT (g_strdup (PQvaluebycol (res, i, "costo"))), (char **)NULL)),
                              -1);
        }
    }
}


/*------------- TRANSFER RANK (CHANGE EVENT) END ----------------------*/

/**
 * Es llamada cuando se edita "stock físico" en el tree_view_cuadratura (signal edited).
 *
 * Esta funcion actualiza el campo "Diferencia" a partir de la resta entre "stock teorico"
 * y "stock físico" (stock_teorico - stock_fisico)
 *
 */
void
on_real_stock_cell_renderer_edited (GtkCellRendererText *cell, gchar *path_string, gchar *stock_fisico, gpointer data)
{
  GtkTreeModel *model = GTK_TREE_MODEL (data);
  GtkTreePath *path = gtk_tree_path_new_from_string (path_string);
  GtkTreeIter iter;
  gdouble stock_fisicoNum = strtod (PUT (stock_fisico), (char **)NULL);
  gdouble stock_teorico;
  //gchar *diferencia;

  gtk_tree_model_get_iter (model, &iter, path);
  gtk_tree_path_free (path);

  gtk_tree_model_get (model, &iter,
                      13, &stock_teorico,
                      -1);

  if (stock_fisicoNum > stock_teorico)
    {
      AlertMSG (GTK_WIDGET (builder_get (builder, "tree_view_cuadratura")),
		"Las unidades que exedan el stock teórico deben ser\n"
		"ajustadas mediante un procedimiento de compra");
      return;
    }
  if (stock_fisicoNum < 0)
    {
      AlertMSG (GTK_WIDGET (builder_get (builder, "tree_view_cuadratura")),
		"El stock físico debe ser un valor positivo");
      return;
    }

  gtk_list_store_set (GTK_LIST_STORE (model), &iter,
		      14, stock_fisicoNum,
		      -1);

  gdouble diferenciaNum = stock_teorico - stock_fisicoNum;

  gtk_list_store_set (GTK_LIST_STORE (model), &iter,
		      15, diferenciaNum,
		      -1);

  if (diferenciaNum != 0)
    {
      // Se habilita el botón guardar
      gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "btn_save_cuadratura")), TRUE);
    }
}

/**
 * Es llamado cuando se presiona el botón "Guardar" en la pestaña cuadratura (signal clicked).
 *
 * Esta funcion guarda el campo "Diferencia" en la tabla merma con "Diferencia cuadratura"
 * como motivo.
 *
 */
void
on_btn_save_cuadratura_clicked()
{
  GtkTreeView *treeview = GTK_TREE_VIEW (builder_get (builder, "tree_view_cuadratura"));
  GtkTreeModel *model = gtk_tree_view_get_model (treeview);
  GtkTreeIter iter;
  gdouble stock_teorico;
  gdouble stock_fisico;
  gdouble diferencia;
  gchar *codigo_corto;
  gboolean valid;

  // Obtengo la primera fila para empezar a iterar --
  valid = gtk_tree_model_get_iter_first (model, &iter);
  while (valid)
    {
      // Obtengo los valores del treeview --
      gtk_tree_model_get (model, &iter,
			  0, &codigo_corto,
                          13, &stock_teorico,
                          14, &stock_fisico,
			  15, &diferencia,
                          -1);

      printf("%f %f %s\n", stock_teorico, stock_fisico, codigo_corto);

      // Se guarda la merma con motivo "Diferencia Cuadratura" --
      if (stock_teorico > stock_fisico)
	{
	  // Se obtiene el barcode a partir del código corto
	   gchar *barcode;
	  barcode = PQvaluebycol (EjecutarSQL (g_strdup_printf ("SELECT barcode FROM producto WHERE codigo_corto='%s'",
								codigo_corto))
				  , 0, "barcode");

	  // 5 = "Diferencia Cuadratura" en la tabla tipo_merma
	  AjusteStock (stock_fisico, 5, barcode);
	  printf ("Guardar\n");
	}

      // Itero a la siguiente fila --
      valid = gtk_tree_model_iter_next (model, &iter); /* Me da TRUE si itera a la siguiente */
    }

  // Se deshabilita el botón guardar
  gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "btn_save_cuadratura")), FALSE);
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
                                g_strdup_printf ("<b>%s</b>", PutPoints ( g_strdup_printf ("%d", atoi(PQvaluebycol (res, 0, "cash_sells")) +
                                                                                                 atoi(PQvaluebycol (res, 0, "cash_on_mixed_sell"))) )));

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

	  gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_mbox_bottle_deposit")),
                                g_strdup_printf ("<b>%s</b>", PutPoints (PQvaluebycol (res, 0, "bottle_deposit"))));

          gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_mbox_ingr_total")),
                                g_strdup_printf
                                ("<b>$ %s</b>", PutPoints
                                 (g_strdup_printf ("%d", atoi (PQvaluebycol (res, 0, "cash_sells"))
                                                   + atoi (PQvaluebycol (res, 0, "cash_on_mixed_sell"))
                                                   + atoi (PQvaluebycol (res, 0, "cash_income"))
                                                   + atoi (PQvaluebycol (res, 0, "cash_payed_money"))
                                                   + atoi (PQvaluebycol (res, 0, "cash_box_start"))
						   + atoi (PQvaluebycol (res, 0, "bottle_deposit"))
                                                   ))));

          /* if (strcmp (PQvaluebycol (res, 0, 8), "") != 0) */
          /*   gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_mbox_out_invoice")), */
          /*                         g_strdup_printf ("<b>%s</b>", PutPoints (PQvaluebycol (res, 0, 8)))); */

          gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_mbox_out")),
                                g_strdup_printf ("<b>%s</b>", PutPoints (PQvaluebycol (res, 0, "cash_outcome"))));

          gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_mbox_loss")),
                                g_strdup_printf ("<b>%s</b>", PutPoints (PQvaluebycol (res, 0, "cash_loss_money"))));

	  gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_mbox_nullify_sell")),
                                g_strdup_printf ("<b>%s</b>", PutPoints (PQvaluebycol (res, 0, "nullify_sell"))));

	  gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_mbox_current_expenses")),
                                g_strdup_printf ("<b>%s</b>", PutPoints (PQvaluebycol (res, 0, "current_expenses"))));

	  gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_mbox_bottle_return")),
                                g_strdup_printf ("<b>%s</b>", PutPoints (PQvaluebycol (res, 0, "bottle_return"))));

	  gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_close_out")),
                                g_strdup_printf ("<b>%s</b>", PutPoints (PQvaluebycol (res, 0, "cash_close_outcome"))));

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
				   + atoi (PQvaluebycol (res, 0, "nullify_sell"))
				   + atoi (PQvaluebycol (res, 0, "current_expenses"))
				   + atoi (PQvaluebycol (res, 0, "bottle_return"))
				   + atoi (PQvaluebycol (res, 0, "cash_close_outcome"))
                                   ))));

          gtk_label_set_markup
            (GTK_LABEL (builder_get (builder, "lbl_money_box_total")),
             g_strdup_printf
             ("<span size=\"xx-large\"><b>$ %s</b></span>",
              PutPoints
              (g_strdup_printf ("%d",
                                atoi (PQvaluebycol (res, 0, "cash_box_start"))
                                + atoi (PQvaluebycol (res, 0, "cash_sells"))
                                + atoi (PQvaluebycol (res, 0, "cash_on_mixed_sell"))
                                + atoi (PQvaluebycol (res, 0, "cash_payed_money"))
                                + atoi (PQvaluebycol (res, 0, "cash_income"))
				+ atoi (PQvaluebycol (res, 0, "bottle_deposit"))
                                - atoi (PQvaluebycol (res, 0, "cash_loss_money"))
                                - atoi (PQvaluebycol (res, 0, "cash_outcome"))
				- atoi (PQvaluebycol (res, 0, "nullify_sell"))
				- atoi (PQvaluebycol (res, 0, "current_expenses"))
				- atoi (PQvaluebycol (res, 0, "bottle_return"))
				- atoi (PQvaluebycol (res, 0, "cash_close_outcome"))
				))));
        }
    }
}


/**
 * Es llamada cuando se presiona en el "tree_view_enviados"
 * o "tree_view_recibidos" (signal changed).
 *
 * Obtiene el id seleccionado y depliega la informacion correspondiente
 * en el  "tree_view_enviado_detalle" o "tree_view_recibido_detalle"
 * según corresponda.
 *
 */

void
fill_traspaso_detalle ()
{
  GtkListStore *store;
  GtkTreeIter iter, iter2;
  gint i, tuples;
  PGresult *res;
  char *sql;

  // Obtiene la página de traspasos desde la cual se esta llamando a esta funcion
  GtkNotebook *notebook = GTK_NOTEBOOK (builder_get (builder, "ntbk_traspasos"));
  gint page_num = gtk_notebook_get_current_page (notebook);

  // Para obtener la seleccion del treeview correspondiente desde traspasos
  GtkTreeView *tree = GTK_TREE_VIEW (builder_get (builder, (page_num == 0) ? "tree_view_enviado" : "tree_view_recibido"));
  GtkTreeModel *model = gtk_tree_view_get_model (tree);
  GtkTreeSelection *selection = gtk_tree_view_get_selection (tree);
  gchar *id;

  if (gtk_tree_selection_get_selected (selection, NULL, &iter2) == TRUE)
    {
      // Obtiene el campo id de la fila seleccionada
      gtk_tree_model_get (model, &iter2,
                          1, &id,
                          -1);

      // Crea y limpia el store correspondiente, dependiendo de la página desde la que se nos llama
      store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (builder_get (builder, (page_num == 0) ? "tree_view_enviado_detalle": "tree_view_recibido_detalle"))));
      gtk_list_store_clear (store);

      sql = g_strdup_printf ( "SELECT p.codigo_corto, p.descripcion, td.cantidad, td.precio, (td.cantidad*td.precio) AS subtotal "
			      "FROM producto p "
			      "INNER join traspaso_detalle td "
			      "ON p.barcode = td.barcode "
			      "AND td.id_traspaso = '%s'", id);

      res = EjecutarSQL (sql);
      tuples = PQntuples (res);
      g_free (sql);

      if (res == NULL) return;

      for (i = 0; i < tuples; i++)
	{
	  gtk_list_store_append (store, &iter);
	  gtk_list_store_set (store, &iter,
			      0, PQvaluebycol (res, i, "codigo_corto"),
			      1, PQvaluebycol (res, i, "descripcion"),
			      2, strtod(PUT(PQvaluebycol (res, i, "cantidad")),(char **)NULL),
			      3, PutPoints (PQvaluebycol (res, i, "precio")),
			      4, PutPoints (PQvaluebycol (res, i, "subtotal")),
			      -1);
	}
    }
}


/**
 * Callback when "change" event on 'treeview_buy_invoice' is triggered.
 *
 * This function shows (on the 'treeview_buy_invoice_details')
 * the information correspondig to the row selected
 * on the 'treeview_buy_invoice'.
 */
void
on_selection_buy_invoice_change (GtkTreeSelection *selection, gpointer data)
{
  GtkTreeView *treeview = GTK_TREE_VIEW (builder_get (builder, "treeview_buy_invoice_details"));
  GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model (treeview));
  gtk_list_store_clear (store);

  GtkTreeModel *model = gtk_tree_view_get_model (gtk_tree_selection_get_tree_view (selection));
  GtkTreeIter iter;

  gint i, tuples;
  gchar *q;
  PGresult *res;

  gint id_compra = 0 ;
  gint id_factura_compra;

  if (gtk_tree_selection_get_selected (selection, NULL, &iter) == TRUE)
    {
      gtk_tree_model_get (model, &iter,
			  8, &id_factura_compra,
			  -1);

      q = g_strdup_printf ("SELECT fc.id_compra, fcd.barcode, fcd.cantidad AS cantidad_comprada, fcd.precio AS costo, round((fcd.cantidad * fcd.precio)) AS monto_compra, "
			   "(SELECT round((cad.cantidad_anulada * cad.costo)) "
			   "        FROM compra_anulada_detalle cad "
			   "        INNER JOIN compra_anulada ca "
			   "        ON ca.id = cad.id_compra_anulada "
			   "        WHERE ca.id_compra = fc.id_compra) AS monto_anulado, "
			   "(SELECT cad.cantidad_anulada "
			   "        FROM compra_anulada_detalle cad "
			   "        INNER JOIN compra_anulada ca "
			   "        ON ca.id = cad.id_compra_anulada "
			   "        WHERE ca.id_compra = fc.id_compra) AS cantidad_anulada, "
			   "(SELECT p.descripcion FROM producto p WHERE p.barcode = fcd.barcode) AS descripcion "
			   "FROM factura_compra_detalle fcd "
			   "INNER JOIN factura_compra fc "
			   "ON fc.id = fcd.id_factura_compra "
			   "WHERE fc.id = %d", id_factura_compra);

      res = EjecutarSQL (q);
      g_free (q);
      tuples = PQntuples (res);

      for (i = 0; i < tuples; i++)
	{
	  gtk_list_store_append (store, &iter);
	  gtk_list_store_set (store, &iter,
			      0, g_strdup (PQvaluebycol (res, i, "barcode")),
			      1, PQvaluebycol (res, i, "descripcion"),
			      2, strtod (PUT(g_strdup (PQvaluebycol (res, i, "cantidad_comprada"))), (char **)NULL),
			      3, strtod (PUT(g_strdup (PQvaluebycol (res, i, "cantidad_anulada"))), (char **)NULL),
			      4, strtod (PUT(g_strdup (PQvaluebycol (res, i, "costo"))), (char **)NULL),
			      5, atoi (g_strdup (PQvaluebycol (res, i, "monto_compra"))),
			      6, atoi ((g_str_equal (PQvaluebycol (res, i, "monto_anulado"),"")) ?
				      "0" : PQvaluebycol (res, i, "monto_anulado")),
			      7, id_compra,
			      8, id_factura_compra,
			      -1);
	}

      PQclear (res);
    }
}


/**
 * Callback when "change" event on 'treeview_buy' is triggered.
 *
 * This function shows (on the 'treeview_buy_invoice')
 * the information correspondig to the row selected
 * on the 'treeview_buy'.
 */
void
on_selection_buy_change (GtkTreeSelection *selection, gpointer data)
{
  GtkTreeView *treeview = GTK_TREE_VIEW (builder_get (builder, "treeview_buy_invoice"));
  GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model (treeview));
  gtk_list_store_clear (store);

  GtkTreeModel *model = gtk_tree_view_get_model (gtk_tree_selection_get_tree_view (selection));
  GtkTreeIter iter;

  gint i, tuples;
  gchar *q;
  PGresult *res;

  gint id_compra;
  gchar *nombre_proveedor, *rut_proveedor;

  if (gtk_tree_selection_get_selected (selection, NULL, &iter) == TRUE)
    {
      gtk_tree_model_get (model, &iter,
			  0, &id_compra,
			  5, &nombre_proveedor,
			  6, &rut_proveedor,
			  -1);

      gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_buy_provider_name")),
			    g_strdup_printf ("<span weight=\"ultrabold\">%s</span>", nombre_proveedor));
      gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_buy_provider_rut")),
			    g_strdup_printf ("<span weight=\"ultrabold\">%s</span>", rut_proveedor));

      q = g_strdup_printf ("SELECT fc.id, fc.num_factura, fc.pagada, fc.id_compra, "
			   "date_part ('year', fc.fecha) AS f_year, "
			   "date_part ('month', fc.fecha) AS f_month, "
			   "date_part ('day', fc.fecha) AS f_day, "

			   "date_part ('year', fc.fecha_documento) AS fd_year, "
			   "date_part ('month', fc.fecha_documento) AS fd_month, "
			   "date_part ('day', fc.fecha_documento) AS fd_day, "

			   "date_part ('year', fc.fecha_pago) AS fp_year, "
			   "date_part ('month', fc.fecha_pago) AS fp_month, "
			   "date_part ('day', fc.fecha_pago) AS fp_day, "

			   "(SELECT round(SUM (cantidad * precio)) "
		                   "FROM factura_compra_detalle fcd "
		                   "WHERE fcd.id_factura_compra = fc.id) AS monto_compra, "

			   "(SELECT round(SUM (cantidad_anulada * costo)) "
		                   "FROM compra_anulada_detalle cad "
			           "INNER JOIN compra_anulada ca ON "
			           "cad.id_compra_anulada = ca.id "
		                   "WHERE ca.id_compra = fc.id_compra) AS monto_anulado, "

			   "(SELECT anulada_pi FROM compra WHERE id = fc.id_compra) AS anulada "

			   "FROM factura_compra fc "
			   "WHERE fc.id_compra = %d", id_compra);

      res = EjecutarSQL (q);
      g_free (q);
      tuples = PQntuples (res);

      for (i = 0; i < tuples; i++)
	{
	  gtk_list_store_append (store, &iter);
	  gtk_list_store_set (store, &iter,
			      0, atoi (g_strdup (PQvaluebycol (res, i, "num_factura"))),
			      1, g_strdup_printf ("%s/%s/%s",
						  PQvaluebycol (res, i, "f_day"),
						  PQvaluebycol (res, i, "f_month"),
						  PQvaluebycol (res, i, "f_year")),
			      2, g_strdup_printf ("%s/%s/%s",
						  PQvaluebycol (res, i, "fd_day"),
						  PQvaluebycol (res, i, "fd_month"),
						  PQvaluebycol (res, i, "fd_year")),
			      3, g_strdup_printf ("%s", (g_str_equal (PQvaluebycol (res, i, "pagada"), "t")) ? "Si" : "No"),
			      4, g_strdup_printf ("%s/%s/%s",
						  PQvaluebycol (res, i, "fp_day"),
						  PQvaluebycol (res, i, "fp_month"),
						  PQvaluebycol (res, i, "fp_year")),
			      5, atoi (g_strdup (PQvaluebycol (res, i, "monto_compra"))),
			      6, atoi ((g_str_equal (PQvaluebycol (res, i, "monto_anulado"),"")) ?
				       "0" : PQvaluebycol (res, i, "monto_anulado")),
			      7, id_compra,
			      8, atoi (g_strdup (PQvaluebycol (res, i, "id"))),
			      -1);
	}

      PQclear (res);
    }
}


/**
 * This function fill the combobox with buy or sell status
 * ('TODAS','VIGENTES', 'ANULADAS') to filter report.
 *
 * @param: gchar combobox_name : The combobox name to fill
 *
 */
void
fill_filter_nullify_cmbbox (gchar *combobox_name)
{
  GtkWidget *combo;
  GtkTreeIter iter;
  GtkListStore *modelo;

  combo = GTK_WIDGET (gtk_builder_get_object(builder, combobox_name));
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

  gtk_list_store_append(modelo, &iter);
  gtk_list_store_set(modelo, &iter,
		     0, 0,
		     1, "TODAS",
		     -1);

  gtk_list_store_append(modelo, &iter);
  gtk_list_store_set(modelo, &iter,
		     0, 1,
		     1, "Vigentes",
		     -1);

  gtk_list_store_append(modelo, &iter);
  gtk_list_store_set(modelo, &iter,
		     0, 2,
		     1, "Anuladas",
		     -1);

  gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 0);
}


/**
 * This function fill the combobox with transfer type
 * ('ENVIADOS', 'RECIBIDOS') to filter transfer report.
 *
 * @param: gchar combobox_name : The combobox name to fill
 *
 */
void
fill_transfer_type_cmbbox (gchar *combobox_name)
{
  GtkWidget *combo;
  GtkTreeIter iter;
  GtkListStore *modelo;

  combo = GTK_WIDGET (gtk_builder_get_object(builder, combobox_name));
  modelo = GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(combo)));

  if (modelo == NULL)
    {
      GtkCellRenderer *cell;
      modelo = gtk_list_store_new (2,
				   G_TYPE_BOOLEAN,
				   G_TYPE_STRING);

      gtk_combo_box_set_model(GTK_COMBO_BOX(combo), GTK_TREE_MODEL(modelo));

      cell = gtk_cell_renderer_text_new();
      gtk_cell_layout_pack_start (GTK_CELL_LAYOUT(combo), cell, TRUE);
      gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo), cell,
				     "text", 1,
				     NULL);
    }

  gtk_list_store_clear(modelo);

  gtk_list_store_append(modelo, &iter);
  gtk_list_store_set(modelo, &iter,
		     0, TRUE,
		     1, "ENVIADOS",
		     -1);

  gtk_list_store_append(modelo, &iter);
  gtk_list_store_set(modelo, &iter,
		     0, FALSE,
		     1, "RECIBIDOS",
		     -1);

  gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 0);
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

  /*Ranking ventas*/
  Print *ranking = (Print *) malloc (sizeof (Print));
  Print *ranking_mp = (Print *) malloc (sizeof (Print));
  Print *ranking_mc = (Print *) malloc (sizeof (Print));
  /*Ranking traspasos*/
  Print *ranking_traspaso = (Print *) malloc (sizeof (Print));
  Print *ranking_traspaso_mp = (Print *) malloc (sizeof (Print));
  Print *ranking_traspaso_mc = (Print *) malloc (sizeof (Print));
  /*otros*/
  Print *cuadratura = (Print *) malloc (sizeof (Print));
  Print *proveedor = (Print *) malloc (sizeof (Print));

  Print *libro = (Print *) malloc (sizeof (Print));
  Print *libro2 = (Print *) malloc (sizeof (Print));
  libro->son = (Print *) malloc (sizeof (Print));
  libro2->son = (Print *) malloc (sizeof (Print));

  // Informe traspasos
  Print *libroEnviados = (Print *) malloc (sizeof (Print));
  Print *libroRecibidos = (Print *) malloc (sizeof (Print));
  libroEnviados->son = (Print *) malloc (sizeof (Print));
  libroRecibidos->son = (Print *) malloc (sizeof (Print));

  Print *libro_compra = (Print *) malloc (sizeof (Print));
  libro_compra->son = (Print *) malloc (sizeof (Print));
  libro_compra->son->son = (Print *) malloc (sizeof (Print));

  // Informe Ventas Exentas
  Print *exempt = (Print *) malloc (sizeof (Print));
  exempt->son = (Print *) malloc (sizeof (Print));

  // Informe de mercaderías Inmovilizadas
  Print *inmovil = (Print *) malloc (sizeof (Print));

  // Informe de Movimiento mercaderías
  Print *movimiento = (Print *) malloc (sizeof (Print));

  builder = gtk_builder_new ();

  gtk_builder_add_from_file (builder, DATADIR"/ui/rizoma-informes.ui", &error);

  if (error != NULL)
    g_printerr ("%s\n", error->message);

  gtk_builder_add_from_file (builder, DATADIR"/ui/rizoma-common.ui", &error);

  if (error != NULL)
    g_printerr ("%s\n", error->message);

  gtk_builder_connect_signals (builder, NULL);


  /*Inicializando SpinButton de Horas*/
  GtkSpinButton *spnBtnHourIni = GTK_SPIN_BUTTON (builder_get (builder, "spnBtnHourIni"));
  GtkSpinButton *spnBtnMinIni  = GTK_SPIN_BUTTON (builder_get (builder, "spnBtnMinIni" ));
  GtkSpinButton *spnBtnHourFin = GTK_SPIN_BUTTON (builder_get (builder, "spnBtnHourFin"));
  GtkSpinButton *spnBtnMinFin  = GTK_SPIN_BUTTON (builder_get (builder, "spnBtnMinFin" ));

  /*Setting Range*/
  gtk_spin_button_set_range(spnBtnHourIni, 0, 23);
  gtk_spin_button_set_range(spnBtnMinIni,  0, 59);

  gtk_spin_button_set_range(spnBtnHourFin, 0, 23);
  gtk_spin_button_set_range(spnBtnMinFin,  0, 59);

  /*Setting Increments*/
  gtk_spin_button_set_increments (spnBtnHourIni, 1, 0);
  gtk_spin_button_set_increments (spnBtnMinIni,  1, 0);
  gtk_spin_button_set_increments (spnBtnHourFin, 1, 0);
  gtk_spin_button_set_increments (spnBtnMinFin,  1, 0);

  /*Set Init Value*/
  gtk_spin_button_set_value (spnBtnHourFin, 23);
  gtk_spin_button_set_value (spnBtnMinFin,  59);
  
  /* Sells */
  store = gtk_list_store_new (7,
                              G_TYPE_STRING,  //Fecha
                              G_TYPE_INT,     //ID Venta
                              G_TYPE_STRING,  //ID Maquina
                              G_TYPE_STRING,  //Vendedor
                              G_TYPE_INT,     //Monto
			      G_TYPE_STRING,  //Tipo pago
                              G_TYPE_STRING); //Estado

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
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)4, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Tipo Pago", renderer,
                                                     "text", 5,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 5);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Estado", renderer,
                                                     "text", 6,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 6);
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

  store = gtk_list_store_new (6,
                              G_TYPE_STRING,  //Producto (marca, descripcion, contendo unidad)
                              G_TYPE_DOUBLE,  //Cantidad (vendida)
                              G_TYPE_DOUBLE,     //Unitario (precio por unidad)
			      G_TYPE_INT,     //IVA (en pesos)
			      G_TYPE_INT,     //Otros impuestos (en pesos)
                              G_TYPE_INT);    //Sub-Total (precio unitario * cantidad)

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
  gtk_tree_view_column_set_expand (column, TRUE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Cantidad", renderer,
                                                     "text", 1,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 1);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)1, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Unitario", renderer,
                                                     "text", 2,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 2);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)2, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("IVA", renderer,
                                                     "text", 3,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 3);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)3, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Otros Imp.", renderer,
                                                     "text", 4,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 4);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)4, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Sub-Total", renderer,
                                                     "text", 5,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 5);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)5, NULL);

  libro->son->tree = treeview;
  libro->son->cols[0].name = "Producto";
  libro->son->cols[0].num = 0;
  libro->son->cols[1].name = "Cantidad";
  libro->son->cols[1].num = 1;
  libro->son->cols[2].name = "Unitario";
  libro->son->cols[2].num = 2;
  libro->son->cols[3].name = "IVA";
  libro->son->cols[3].num = 3;
  libro->son->cols[4].name = "Otros impuestos";
  libro->son->cols[4].num = 4;
  libro->son->cols[5].name = "Sub-Total";
  libro->son->cols[5].num = 5;
  libro->son->cols[6].name = NULL;

  g_signal_connect (builder_get (builder, "btn_print_sells"), "clicked",
                    G_CALLBACK (PrintTwoTree), (gpointer)libro);


  /* End Sells */


  /* Sells Rank */

  store = gtk_list_store_new (10,
			      G_TYPE_STRING, //Barcode
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

  selection = gtk_tree_view_get_selection (treeview);

  /* g_signal_connect (G_OBJECT (selection), "changed", */
  /*                   G_CALLBACK (change_sell_rank), NULL); */

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Producto", renderer,
                                                     "text", 1,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 1);
  gtk_tree_view_column_set_min_width (column, 160);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_expand (column, TRUE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Marca", renderer,
                                                     "text", 2,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 2);
  gtk_tree_view_column_set_min_width (column, 90);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Medida", renderer,
                                                     "text", 3,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 3);
  gtk_tree_view_column_set_min_width (column, 60);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Unid.", renderer,
                                                     "text", 4,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 4);
  gtk_tree_view_column_set_min_width (column, 40);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Unidades", renderer,
                                                     "text", 5,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 5);
  gtk_tree_view_column_set_min_width (column, 50);
  gtk_tree_view_column_set_resizable (column, FALSE);

  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)5, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Vendido $", renderer,
                                                     "text", 6,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 6);
  gtk_tree_view_column_set_min_width (column, 70);
  gtk_tree_view_column_set_resizable (column, FALSE);

  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)6, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Costo $", renderer,
                                                     "text", 7,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 7);
  gtk_tree_view_column_set_min_width (column, 50);
  gtk_tree_view_column_set_resizable (column, FALSE);

  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)7, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Contrib. $", renderer,
                                                     "text", 8,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 8);
  gtk_tree_view_column_set_min_width (column, 60);
  gtk_tree_view_column_set_resizable (column, FALSE);

  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)8, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Margen %", renderer,
                                                     "text", 9,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 9);
  gtk_tree_view_column_set_min_width (column, 50);
  gtk_tree_view_column_set_resizable (column, FALSE);

  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)9, NULL);

  ranking->tree = treeview;
  ranking->title = "Ranking de Ventas";
  ranking->date_string = NULL;
  ranking->cols[0].name = "Barcode";
  ranking->cols[1].name = "Producto";
  ranking->cols[2].name = "Marca";
  ranking->cols[3].name = "Medida";
  ranking->cols[4].name = "Unidad";
  ranking->cols[5].name = "Unidades";
  ranking->cols[6].name = "Vendido $";
  ranking->cols[7].name = "Costo";
  ranking->cols[8].name = "Contrib";
  ranking->cols[9].name = "Margen";
  ranking->cols[10].name = NULL;

  g_signal_connect (builder_get (builder, "btn_print_rank"), "clicked",
                    G_CALLBACK (PrintTree), (gpointer)ranking);


  /* End Sells Rank */


  /* Sells Rank MP*/

  store = gtk_list_store_new (10,
			      G_TYPE_STRING, // barcode
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_DOUBLE,
                              G_TYPE_INT,
                              G_TYPE_INT,
			      G_TYPE_INT,
			      G_TYPE_DOUBLE);

  treeview = GTK_TREE_VIEW (builder_get (builder, "treeview_sell_rank_mp"));
  gtk_tree_view_set_model (treeview, GTK_TREE_MODEL (store));

  selection = gtk_tree_view_get_selection (treeview);
  g_signal_connect (G_OBJECT (selection), "changed",
                    G_CALLBACK (change_sell_rank_mp), NULL);


  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Producto", renderer,
                                                     "text", 1,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 1);
  gtk_tree_view_column_set_min_width (column, 160);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_expand (column, TRUE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Marca", renderer,
                                                     "text", 2,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 2);
  gtk_tree_view_column_set_min_width (column, 90);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Medida", renderer,
                                                     "text", 3,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 3);
  gtk_tree_view_column_set_min_width (column, 60);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Unid.", renderer,
                                                     "text", 4,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 4);
  gtk_tree_view_column_set_min_width (column, 40);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Unidades", renderer,
                                                     "text", 5,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 5);
  gtk_tree_view_column_set_min_width (column, 50);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)5, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Vendido $", renderer,
                                                     "text", 6,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 6);
  gtk_tree_view_column_set_min_width (column, 70);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)6, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Costo $", renderer,
                                                     "text", 7,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 7);
  gtk_tree_view_column_set_min_width (column, 50);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)7, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Contrib. $", renderer,
                                                     "text", 8,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 8);
  gtk_tree_view_column_set_min_width (column, 60);
  gtk_tree_view_column_set_resizable (column, FALSE);

  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)8, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Margen %", renderer,
                                                     "text", 9,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 9);
  gtk_tree_view_column_set_min_width (column, 50);
  gtk_tree_view_column_set_resizable (column, FALSE);

  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)9, NULL);

  ranking_mp->tree = treeview;
  ranking_mp->title = "Ranking de Ventas (Materias primas)";
  ranking_mp->date_string = NULL;
  ranking_mp->cols[0].name = "Barcode";
  ranking_mp->cols[1].name = "Producto";
  ranking_mp->cols[2].name = "Marca";
  ranking_mp->cols[3].name = "Medida";
  ranking_mp->cols[4].name = "Unidad";
  ranking_mp->cols[5].name = "Unidades";
  ranking_mp->cols[6].name = "Vendido $";
  ranking_mp->cols[7].name = "Costo";
  ranking_mp->cols[8].name = "Contrib";
  ranking_mp->cols[9].name = "Margen";
  ranking_mp->cols[10].name = NULL;

  g_signal_connect (builder_get (builder, "btn_print_rank_mp"), "clicked",
		    G_CALLBACK (PrintTree), (gpointer)ranking_mp);


  /* End Sells Rank MP */

  /* Sells Rank Derivates*/
  store = gtk_list_store_new (9,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_DOUBLE,
                              G_TYPE_INT,
                              G_TYPE_INT,
			      G_TYPE_INT,
			      G_TYPE_DOUBLE);

  treeview = GTK_TREE_VIEW (builder_get (builder, "treeview_sell_rank_deriv"));
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
  gtk_tree_view_column_set_expand (column, TRUE);

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

  /* End Sells Rank Derivates */



  /*--------------------------------Sell Rank MC-------------------------------*/
  /* Sells Rank MC*/

  store = gtk_list_store_new (10,
			      G_TYPE_STRING, // barcode
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_DOUBLE,
                              G_TYPE_INT,
                              G_TYPE_INT,
			      G_TYPE_INT,
			      G_TYPE_DOUBLE);

  treeview = GTK_TREE_VIEW (builder_get (builder, "treeview_sell_rank_mc"));
  gtk_tree_view_set_model (treeview, GTK_TREE_MODEL (store));

  selection = gtk_tree_view_get_selection (treeview);
  g_signal_connect (G_OBJECT (selection), "changed",
                    G_CALLBACK (change_sell_rank_mc), NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Producto", renderer,
                                                     "text", 1,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 1);
  gtk_tree_view_column_set_min_width (column, 160);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_expand (column, TRUE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Marca", renderer,
                                                     "text", 2,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 2);
  gtk_tree_view_column_set_min_width (column, 90);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Medida", renderer,
                                                     "text", 3,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 3);
  gtk_tree_view_column_set_min_width (column, 60);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Unid.", renderer,
                                                     "text", 4,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 4);
  gtk_tree_view_column_set_min_width (column, 40);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Unidades", renderer,
                                                     "text", 5,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 5);
  gtk_tree_view_column_set_min_width (column, 50);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)5, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Vendido $", renderer,
                                                     "text", 6,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 6);
  gtk_tree_view_column_set_min_width (column, 70);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)6, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Costo $", renderer,
                                                     "text", 7,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 7);
  gtk_tree_view_column_set_min_width (column, 50);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)7, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Contrib. $", renderer,
                                                     "text", 8,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 8);
  gtk_tree_view_column_set_min_width (column, 60);
  gtk_tree_view_column_set_resizable (column, FALSE);

  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)8, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Margen %", renderer,
                                                     "text", 9,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 9);
  gtk_tree_view_column_set_min_width (column, 50);
  gtk_tree_view_column_set_resizable (column, FALSE);

  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)9, NULL);

  ranking_mc->tree = treeview;
  ranking_mc->title = "Ranking de Ventas (Mercaderías Compuestas)";
  ranking_mc->date_string = NULL;
  ranking_mc->cols[0].name = "Barcode";
  ranking_mc->cols[1].name = "Producto";
  ranking_mc->cols[2].name = "Marca";
  ranking_mc->cols[3].name = "Medida";
  ranking_mc->cols[4].name = "Unidad";
  ranking_mc->cols[5].name = "Unidades";
  ranking_mc->cols[6].name = "Vendido $";
  ranking_mc->cols[7].name = "Costo";
  ranking_mc->cols[8].name = "Contrib";
  ranking_mc->cols[9].name = "Margen";
  ranking_mc->cols[10].name = NULL;

  g_signal_connect (builder_get (builder, "btn_print_rank_mc"), "clicked",
		    G_CALLBACK (PrintTree), (gpointer)ranking_mc);

  /* End Sells Rank MC */

  /* Sells Rank Componentes*/
  store = gtk_list_store_new (9,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_DOUBLE,
                              G_TYPE_INT,
                              G_TYPE_INT,
			      G_TYPE_INT,
			      G_TYPE_DOUBLE);

  treeview = GTK_TREE_VIEW (builder_get (builder, "treeview_sell_rank_comp"));
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
  gtk_tree_view_column_set_expand (column, TRUE);

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

  /* End Sells Rank Components */

  /*---------------------------------------------------------------------------*/

  /* Cash Box */
  store = gtk_list_store_new (6,
                              G_TYPE_INT,
                              G_TYPE_STRING,
                              G_TYPE_INT,
                              G_TYPE_STRING,
                              G_TYPE_INT,
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
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)2, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Fecha Cierre", renderer,
                                                     "text", 3,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Retiro al cierre", renderer,
                                                     "text", 4,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)4, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Saldo Cierre", renderer,
                                                     "text", 5,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)5, NULL);

  /* End Cash Box */

   /* Devoluciones*/
  store = gtk_list_store_new (4,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
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
  //gtk_tree_view_column_set_min_width (column, 260);
  //gtk_tree_view_column_set_max_width (column, 260);
  gtk_tree_view_column_set_expand (column, TRUE);

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
  gtk_tree_view_column_set_expand (column, TRUE);

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

  proveedor->tree = treeview;
  proveedor->title = "Informe Proveedor";
  proveedor->date_string = NULL;
  proveedor->cols[0].name = "Proveedor";
  proveedor->cols[1].name = "Unidades";
  proveedor->cols[2].name = "Comprado $";
  proveedor->cols[3].name = "Margen %";
  proveedor->cols[4].name = "Contribución $";
  proveedor->cols[5].name = NULL;

  g_signal_connect (builder_get (builder, "btn_print_providers"), "clicked",
                    G_CALLBACK (PrintTree), (gpointer)proveedor);

  /*
    End Proveedores
   */


  /*
    Start Cuadratura
   */

  store = gtk_list_store_new (16,
			      G_TYPE_STRING, // 0, Codigo Corto
                              G_TYPE_STRING, // 1, Descripción Mercadería
			      G_TYPE_STRING, // 2, Marca
                              G_TYPE_DOUBLE, // 3, Stock Inicial
                              G_TYPE_DOUBLE, // 4, Compras
			      G_TYPE_DOUBLE, // 5, Anulaciones de Compras
                              G_TYPE_DOUBLE, // 6, Ventas
			      G_TYPE_DOUBLE, // 7, Anulaciones de Ventas
			      G_TYPE_DOUBLE, // 8, Insumido (vendido - anulado)
                              G_TYPE_DOUBLE, // 9, Devoluciones
			      G_TYPE_DOUBLE, // 10, Mermas
			      G_TYPE_DOUBLE, // 11, Enviados
			      G_TYPE_DOUBLE, // 12, Recibidos
			      G_TYPE_DOUBLE, // 13, Stock teorico
			      G_TYPE_DOUBLE, // 14, Stock Físico
			      G_TYPE_DOUBLE, // 15, Diferencia
                              -1);

  treeview = GTK_TREE_VIEW (builder_get (builder, "tree_view_cuadratura"));
  gtk_tree_view_set_model (treeview, GTK_TREE_MODEL (store));
  selection = gtk_tree_view_get_selection (treeview);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Codigo", renderer,
						     "text", 0,
						     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 0);
  gtk_tree_view_column_set_max_width (column, 70);
  gtk_tree_view_column_set_resizable (column, FALSE);


  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Descripción Mercadería", renderer,
						     "text", 1,
						     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 1);
  gtk_tree_view_column_set_min_width (column, 3000);
  gtk_tree_view_column_set_max_width (column, 300);
  gtk_tree_view_column_set_resizable (column, FALSE);


  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Marca", renderer,
						     "text", 2,
						     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 2);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Stock inicial", renderer,
						     "text", 3,
						     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 3);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)3, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Compras", renderer,
						     "text", 4,
						     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 4);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)4, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Compras Anul.", renderer,
						     "text", 5,
						     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 5);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)5, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Ventas", renderer,
						     "text", 6,
						     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 6);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)6, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Ventas Anul.", renderer,
						     "text", 7,
						     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 7);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)7, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Insumido", renderer,
						     "text", 8,
						     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 8);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)8, NULL);


  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Devoluciones", renderer,
						     "text", 9,
						     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 9);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)9, NULL);


  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Mermas", renderer,
						     "text", 10,
						     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 10);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)10, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Enviados", renderer,
						     "text", 11,
						     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 11);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)11, NULL);


  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Recibidos", renderer,
						     "text", 12,
						     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 12);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)12, NULL);


  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Stock teórico", renderer,
						     "text", 13,
						     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 13);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)13, NULL);


  renderer = gtk_cell_renderer_text_new ();
  g_object_set (renderer,
		"editable", TRUE,
		NULL);
  g_signal_connect (G_OBJECT (renderer), "edited",
		    G_CALLBACK (on_real_stock_cell_renderer_edited), (gpointer)store);
  column = gtk_tree_view_column_new_with_attributes ("Stock físico", renderer,
						     "text", 14,
						     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 14);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)14, NULL);


  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Diferencia", renderer,
						     "text", 15,
						     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 15);
  gtk_tree_view_column_set_min_width (column, 90);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)15, NULL);


  cuadratura->tree = treeview;
  cuadratura->title = "Informe Cuadratura";
  cuadratura->date_string = NULL;
  cuadratura->cols[0].name = "Codigo Corto";
  cuadratura->cols[1].name = "Descripción";
  cuadratura->cols[2].name = "Marca";
  cuadratura->cols[3].name = "Stock Inicial";
  cuadratura->cols[4].name = "Compras";
  cuadratura->cols[5].name = "Compras Anuladas";
  cuadratura->cols[6].name = "Ventas";
  cuadratura->cols[7].name = "Ventas Anuladas";
  cuadratura->cols[8].name = "Insumido";
  cuadratura->cols[9].name = "Devoluciones";
  cuadratura->cols[10].name = "Mermas";
  cuadratura->cols[11].name = "Envios";
  cuadratura->cols[12].name = "Recibidos";
  cuadratura->cols[13].name = "Stock Teórico";
  cuadratura->cols[14].name = "Stock Físico";
  cuadratura->cols[15].name = "Diferencia";
  cuadratura->cols[16].name = NULL;

  g_signal_connect (builder_get (builder, "btn_print_cuadratura"), "clicked",
                    G_CALLBACK (PrintTree), (gpointer)cuadratura);

  // El botón guardar inicia "deshabilitado"
  gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "btn_save_cuadratura")), FALSE);

  /*
    End Cuadratura
   */


  /*
    Start Informe Traspasos
   */

  // ENVIO -----
  store = gtk_list_store_new (4,
			      G_TYPE_STRING, // 0, Fecha
			      G_TYPE_STRING, // 1, ID
			      G_TYPE_STRING, // 2, Destino
			      G_TYPE_STRING, // 3, Monto Total
                              -1);

  treeview = GTK_TREE_VIEW (builder_get (builder, "tree_view_enviado"));
  gtk_tree_view_set_model (treeview, GTK_TREE_MODEL (store));
  selection = gtk_tree_view_get_selection (treeview);

  g_signal_connect (G_OBJECT (selection), "changed",
		    G_CALLBACK (fill_traspaso_detalle), NULL);

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
  column = gtk_tree_view_column_new_with_attributes ("ID", renderer,
                                                     "text", 1,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 1);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Destino", renderer,
                                                     "text", 2,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 2);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Monto Total", renderer,
						     "text", 3,
						     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 3);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)3, NULL);

  libroEnviados->tree = treeview;
  libroEnviados->title = "Libro de Traspasos de envio";
  libroEnviados->name = "ventas";
  libroEnviados->date_string = NULL;
  libroEnviados->cols[0].name = "Fecha";
  libroEnviados->cols[0].num = 0;
  libroEnviados->cols[1].name = "Destino";
  libroEnviados->cols[1].num = 2;
  libroEnviados->cols[2].name = "Monto";
  libroEnviados->cols[2].num = 3;
  libroEnviados->cols[3].name = NULL;


  // Envio Detalle --

  store = gtk_list_store_new (5,
			      G_TYPE_STRING, // 0, Codigo corto
			      G_TYPE_STRING, // 1, Descripcion
			      G_TYPE_DOUBLE, // 2, Unidad
			      G_TYPE_STRING, // 3, Costo promedio
			      G_TYPE_STRING, // 4, Sub total
                              -1);

  treeview = GTK_TREE_VIEW (builder_get (builder, "tree_view_enviado_detalle"));
  gtk_tree_view_set_model (treeview, GTK_TREE_MODEL (store));
  selection = gtk_tree_view_get_selection (treeview);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Codigo corto", renderer,
                                                     "text", 0,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 0);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Descripción", renderer,
                                                     "text", 1,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 1);
  //gtk_tree_view_column_set_min_width (column, 300);
  gtk_tree_view_column_set_resizable (column, TRUE);
  gtk_tree_view_column_set_expand (column, TRUE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Unid.", renderer,
                                                     "text", 2,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 2);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)2, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Costo", renderer,
						     "text", 3,
						     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 3);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)3, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Sub Total", renderer,
						     "text", 4,
						     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 4);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)4, NULL);

  libroEnviados->son->tree = treeview;
  libroEnviados->son->cols[0].name = "Codigo Corto";
  libroEnviados->son->cols[0].num = 0;
  libroEnviados->son->cols[1].name = "Descripción";
  libroEnviados->son->cols[1].num = 1;
  libroEnviados->son->cols[2].name = "Unidades";
  libroEnviados->son->cols[2].num = 2;
  libroEnviados->son->cols[3].name = "Costo";
  libroEnviados->son->cols[3].num = 3;
  libroEnviados->son->cols[4].name = "Sub Total";
  libroEnviados->son->cols[4].num = 4;
  libroEnviados->son->cols[5].name = NULL;

  g_signal_connect (builder_get (builder, "btn_print_enviado"), "clicked",
                    G_CALLBACK (PrintTwoTree), (gpointer)libroEnviados);


  // RECEPCION ----
  store = gtk_list_store_new (4,
			      G_TYPE_STRING, // 0, Fecha
			      G_TYPE_STRING, // 1, ID
			      G_TYPE_STRING, // 2, Procedencia
			      G_TYPE_STRING, // 3, Monto Total
                              -1);

  treeview = GTK_TREE_VIEW (builder_get (builder, "tree_view_recibido"));
  gtk_tree_view_set_model (treeview, GTK_TREE_MODEL (store));
  selection = gtk_tree_view_get_selection (treeview);

  g_signal_connect (G_OBJECT (selection), "changed",
		    G_CALLBACK (fill_traspaso_detalle), NULL);

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
  column = gtk_tree_view_column_new_with_attributes ("ID", renderer,
                                                     "text", 1,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 1);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Procedencia", renderer,
                                                     "text", 2,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 2);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Monto Total", renderer,
						     "text", 3,
						     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 3);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)3, NULL);

  libroRecibidos->tree = treeview;
  libroRecibidos->title = "Libro de Traspasos de Recepcion";
  libroRecibidos->name = "Recibidos";
  libroRecibidos->date_string = NULL;
  libroRecibidos->cols[0].name = "Fecha";
  libroRecibidos->cols[0].num = 0;
  libroRecibidos->cols[1].name = "Destino";
  libroRecibidos->cols[1].num = 2;
  libroRecibidos->cols[2].name = "Monto";
  libroRecibidos->cols[2].num = 3;
  libroRecibidos->cols[3].name = NULL;

  // Recepcion Detalle --

  store = gtk_list_store_new (5,
			      G_TYPE_STRING, // 0, Codigo corto
			      G_TYPE_STRING, // 1, Descripcion
			      G_TYPE_DOUBLE, // 2, Unidad
			      G_TYPE_STRING, // 3, Costo promedio
			      G_TYPE_STRING, // 4, Sub total
                              -1);

  treeview = GTK_TREE_VIEW (builder_get (builder, "tree_view_recibido_detalle"));
  gtk_tree_view_set_model (treeview, GTK_TREE_MODEL (store));
  selection = gtk_tree_view_get_selection (treeview);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Codigo corto", renderer,
                                                     "text", 0,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 0);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Descripción", renderer,
                                                     "text", 1,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 1);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_expand (column, TRUE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Unid.", renderer,
                                                     "text", 2,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 2);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)2, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Costo", renderer,
						     "text", 3,
						     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 3);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)3, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Sub Total", renderer,
						     "text", 4,
						     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 4);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)4, NULL);

  libroRecibidos->son->tree = treeview;
  libroRecibidos->son->cols[0].name = "Codigo Corto";
  libroRecibidos->son->cols[0].num = 0;
  libroRecibidos->son->cols[1].name = "Descripción";
  libroRecibidos->son->cols[1].num = 1;
  libroRecibidos->son->cols[2].name = "Unidades";
  libroRecibidos->son->cols[2].num = 2;
  libroRecibidos->son->cols[3].name = "Costo";
  libroRecibidos->son->cols[3].num = 3;
  libroRecibidos->son->cols[4].name = "Sub Total";
  libroRecibidos->son->cols[4].num = 4;
  libroRecibidos->son->cols[5].name = NULL;

  g_signal_connect (builder_get (builder, "btn_print_recibido"), "clicked",
                    G_CALLBACK (PrintTwoTree), (gpointer)libroRecibidos);

  /*
    END Informe Traspasos
   */


  /*
    ----------- INFORME RANKING TRASPASOS ------------
   */
  store = gtk_list_store_new (7,
			      G_TYPE_STRING,  //Barcode
                              G_TYPE_STRING,  //Descripcion
                              G_TYPE_STRING,  //Marca
                              G_TYPE_STRING,  //Contenido
                              G_TYPE_STRING,  //Unidad
			      G_TYPE_DOUBLE,  //Cantidad
                              G_TYPE_INT);    //Costo

  treeview = GTK_TREE_VIEW (builder_get (builder, "treeview_general_transfer_rank"));
  gtk_tree_view_set_model (treeview, GTK_TREE_MODEL (store));

  selection = gtk_tree_view_get_selection (treeview);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Producto", renderer,
                                                     "text", 1,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 1);
  gtk_tree_view_column_set_min_width (column, 160);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_expand (column, TRUE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Marca", renderer,
                                                     "text", 2,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 2);
  gtk_tree_view_column_set_min_width (column, 90);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Contenido", renderer,
                                                     "text", 3,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 3);
  gtk_tree_view_column_set_min_width (column, 60);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Unid.", renderer,
                                                     "text", 4,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 4);
  gtk_tree_view_column_set_min_width (column, 40);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Cantidad", renderer,
                                                     "text", 5,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 5);
  gtk_tree_view_column_set_min_width (column, 50);
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

  ranking_traspaso->tree = treeview;
  ranking_traspaso->title = "Ranking de traspasos";
  ranking_traspaso->date_string = NULL;
  ranking_traspaso->cols[0].name = "Barcode";
  ranking_traspaso->cols[1].name = "Producto";
  ranking_traspaso->cols[2].name = "Marca";
  ranking_traspaso->cols[3].name = "Contenido";
  ranking_traspaso->cols[4].name = "Unidad";
  ranking_traspaso->cols[5].name = "Cantidad";
  ranking_traspaso->cols[6].name = "Costo";
  ranking_traspaso->cols[7].name = NULL;

  g_signal_connect (builder_get (builder, "btn_print_transfer_rank"), "clicked",
		    G_CALLBACK (PrintTree), (gpointer)ranking_traspaso);


  //Treeview Materias primas
  store = gtk_list_store_new (7,
			      G_TYPE_STRING,  //Barcode
                              G_TYPE_STRING,  //Descripcion
                              G_TYPE_STRING,  //Marca
                              G_TYPE_STRING,  //Contenido
                              G_TYPE_STRING,  //Unidad
			      G_TYPE_DOUBLE,  //Cantidad
                              G_TYPE_INT);    //Costo

  treeview = GTK_TREE_VIEW (builder_get (builder, "treeview_mp_transfer_rank"));
  gtk_tree_view_set_model (treeview, GTK_TREE_MODEL (store));

  selection = gtk_tree_view_get_selection (treeview);

  g_signal_connect (G_OBJECT (selection), "changed",
		    G_CALLBACK (change_transfer_rank_mp), NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Producto", renderer,
                                                     "text", 1,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 1);
  gtk_tree_view_column_set_min_width (column, 160);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_expand (column, TRUE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Marca", renderer,
                                                     "text", 2,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 2);
  gtk_tree_view_column_set_min_width (column, 90);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Contenido", renderer,
                                                     "text", 3,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 3);
  gtk_tree_view_column_set_min_width (column, 60);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Unid.", renderer,
                                                     "text", 4,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 4);
  gtk_tree_view_column_set_min_width (column, 40);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Cantidad", renderer,
                                                     "text", 5,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 5);
  gtk_tree_view_column_set_min_width (column, 50);
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

  ranking_traspaso_mp->tree = treeview;
  ranking_traspaso_mp->title = "Ranking traspaso de materias primas";
  ranking_traspaso_mp->date_string = NULL;
  ranking_traspaso_mp->cols[0].name = "Barcode";
  ranking_traspaso_mp->cols[1].name = "Producto";
  ranking_traspaso_mp->cols[2].name = "Marca";
  ranking_traspaso_mp->cols[3].name = "Contenido";
  ranking_traspaso_mp->cols[4].name = "Unidad";
  ranking_traspaso_mp->cols[5].name = "Cantidad";
  ranking_traspaso_mp->cols[6].name = "Costo";
  ranking_traspaso_mp->cols[7].name = NULL;

  g_signal_connect (builder_get (builder, "btn_print_transfer_rank_mp"), "clicked",
		    G_CALLBACK (PrintTree), (gpointer)ranking_traspaso_mp);

  //Treeview Derivados
  store = gtk_list_store_new (7,
			      G_TYPE_STRING,  //Barcode
                              G_TYPE_STRING,  //Descripcion
                              G_TYPE_STRING,  //Marca
                              G_TYPE_STRING,  //Contenido
                              G_TYPE_STRING,  //Unidad
			      G_TYPE_DOUBLE,  //Cantidad
                              G_TYPE_INT);    //Costo

  treeview = GTK_TREE_VIEW (builder_get (builder, "treeview_derivates_transfer_rank"));
  gtk_tree_view_set_model (treeview, GTK_TREE_MODEL (store));

  selection = gtk_tree_view_get_selection (treeview);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Producto", renderer,
                                                     "text", 1,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 1);
  gtk_tree_view_column_set_min_width (column, 160);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_expand (column, TRUE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Marca", renderer,
                                                     "text", 2,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 2);
  gtk_tree_view_column_set_min_width (column, 90);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Contenido", renderer,
                                                     "text", 3,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 3);
  gtk_tree_view_column_set_min_width (column, 60);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Unid.", renderer,
                                                     "text", 4,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 4);
  gtk_tree_view_column_set_min_width (column, 40);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Cantidad", renderer,
                                                     "text", 5,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 5);
  gtk_tree_view_column_set_min_width (column, 50);
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


  //Treeview Mercaderias compuestas
  store = gtk_list_store_new (7,
			      G_TYPE_STRING,  //Barcode
                              G_TYPE_STRING,  //Descripcion
                              G_TYPE_STRING,  //Marca
                              G_TYPE_STRING,  //Contenido
                              G_TYPE_STRING,  //Unidad
			      G_TYPE_DOUBLE,  //Cantidad
                              G_TYPE_INT);    //Costo

  treeview = GTK_TREE_VIEW (builder_get (builder, "treeview_offer_transfer_rank"));
  gtk_tree_view_set_model (treeview, GTK_TREE_MODEL (store));

  selection = gtk_tree_view_get_selection (treeview);

  g_signal_connect (G_OBJECT (selection), "changed",
		    G_CALLBACK (change_transfer_rank_mc), NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Producto", renderer,
                                                     "text", 1,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 1);
  gtk_tree_view_column_set_min_width (column, 160);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_expand (column, TRUE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Marca", renderer,
                                                     "text", 2,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 2);
  gtk_tree_view_column_set_min_width (column, 90);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Contenido", renderer,
                                                     "text", 3,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 3);
  gtk_tree_view_column_set_min_width (column, 60);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Unid.", renderer,
                                                     "text", 4,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 4);
  gtk_tree_view_column_set_min_width (column, 40);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Cantidad", renderer,
                                                     "text", 5,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 5);
  gtk_tree_view_column_set_min_width (column, 50);
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

    ranking_traspaso_mc->tree = treeview;
  ranking_traspaso_mc->title = "Ranking traspaso de ofertas";
  ranking_traspaso_mc->date_string = NULL;
  ranking_traspaso_mc->cols[0].name = "Barcode";
  ranking_traspaso_mc->cols[1].name = "Producto";
  ranking_traspaso_mc->cols[2].name = "Marca";
  ranking_traspaso_mc->cols[3].name = "Contenido";
  ranking_traspaso_mc->cols[4].name = "Unidad";
  ranking_traspaso_mc->cols[5].name = "Cantidad";
  ranking_traspaso_mc->cols[6].name = "Costo";
  ranking_traspaso_mc->cols[7].name = NULL;

  g_signal_connect (builder_get (builder, "btn_print_transfer_rank_mc"), "clicked",
		    G_CALLBACK (PrintTree), (gpointer)ranking_traspaso_mc);


  //Treeview Mercaderias componentes
  store = gtk_list_store_new (7,
			      G_TYPE_STRING,  //Barcode
                              G_TYPE_STRING,  //Descripcion
                              G_TYPE_STRING,  //Marca
                              G_TYPE_STRING,  //Contenido
                              G_TYPE_STRING,  //Unidad
			      G_TYPE_DOUBLE,  //Cantidad
                              G_TYPE_INT);    //Costo

  treeview = GTK_TREE_VIEW (builder_get (builder, "treeview_components_transfer_rank"));
  gtk_tree_view_set_model (treeview, GTK_TREE_MODEL (store));

  selection = gtk_tree_view_get_selection (treeview);

  /* g_signal_connect (G_OBJECT (selection), "changed", */
  /*                   G_CALLBACK (change_sell_rank), NULL); */

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Producto", renderer,
                                                     "text", 1,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 1);
  gtk_tree_view_column_set_min_width (column, 160);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_expand (column, TRUE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Marca", renderer,
                                                     "text", 2,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 2);
  gtk_tree_view_column_set_min_width (column, 90);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Contenido", renderer,
                                                     "text", 3,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 3);
  gtk_tree_view_column_set_min_width (column, 60);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Unid.", renderer,
                                                     "text", 4,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 4);
  gtk_tree_view_column_set_min_width (column, 40);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Cantidad", renderer,
                                                     "text", 5,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 5);
  gtk_tree_view_column_set_min_width (column, 50);
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

  /*ranking->tree = treeview;
  ranking->title = "Ranking de Ventas";
  ranking->date_string = NULL;
  ranking->cols[0].name = "Barcode";
  ranking->cols[1].name = "Producto";
  ranking->cols[2].name = "Marca";
  ranking->cols[3].name = "Contenido";
  ranking->cols[4].name = "Unidad";
  ranking->cols[5].name = "Cantidad";
  ranking->cols[6].name = "Costo";
  ranking->cols[7].name = NULL;*/

  /*g_signal_connect (builder_get (builder, "btn_print_rank"), "clicked",
    G_CALLBACK (PrintTree), (gpointer)ranking);*/



  // TODOOOOOOO: Agregar los treeviews

  /*
    ---------- END INFORME RANKING TRASPASOS ------------
   */


  /*
    START Informe Compras
   */

  //Compras
  store = gtk_list_store_new (7,
			      G_TYPE_INT,     //Id
			      G_TYPE_STRING,  //Date
			      G_TYPE_STRING,  //Status
			      G_TYPE_INT,     //Buy amount
			      G_TYPE_INT,     //Nullify amount
			      G_TYPE_STRING,  //Nombre proveedor
			      G_TYPE_STRING); //Rut proveedor

  treeview = GTK_TREE_VIEW (gtk_builder_get_object(builder, "treeview_buy"));
  gtk_tree_view_set_model (treeview, GTK_TREE_MODEL(store));

  selection = gtk_tree_view_get_selection (treeview);
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
  g_signal_connect (G_OBJECT (selection), "changed",
		    G_CALLBACK (on_selection_buy_change), NULL);

  //ID
  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("ID", renderer,
						    "text", 0,
						    NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 0);
  gtk_tree_view_column_set_resizable (column, FALSE);

  //Date
  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("Fecha Pedido", renderer,
						    "text", 1,
						    NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 1);
  gtk_tree_view_column_set_resizable (column, FALSE);

  //Status
  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("Estado", renderer,
						    "text", 2,
						    NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 2);
  gtk_tree_view_column_set_resizable (column, FALSE);

  //Buy amount
  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("Monto Compra", renderer,
						    "text", 3,
						    NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 3);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)3, NULL);

  //Nullify amount
  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("Monto Anulado", renderer,
						    "text", 4,
						    NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 4);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)4, NULL);

  libro_compra->tree = treeview;
  libro_compra->title = "Libro de Compras";
  libro_compra->name = "Compras";
  libro_compra->date_string = NULL;
  libro_compra->cols[0].name = "ID";
  libro_compra->cols[0].num = 0;
  libro_compra->cols[1].name = "Fecha";
  libro_compra->cols[1].num = 1;
  libro_compra->cols[2].name = "Estado";
  libro_compra->cols[2].num = 2;
  libro_compra->cols[3].name = "Monto Compra";
  libro_compra->cols[3].num = 3;
  libro_compra->cols[4].name = "Monto Anulado";
  libro_compra->cols[4].num = 4;
  libro_compra->cols[5].name = NULL;

  //Facturas
  store = gtk_list_store_new (9,
			      G_TYPE_INT,    // N° Factura
			      G_TYPE_STRING, // Fecha Ingreso
			      G_TYPE_STRING, // Pagada
			      G_TYPE_STRING, // Fecha Pago
			      G_TYPE_STRING, // Fecha Documento
			      G_TYPE_INT,    // Buy amount
			      G_TYPE_INT,    // Nullify amount
			      G_TYPE_INT,    // ID Compra
			      G_TYPE_INT);   // ID Factura Compra

  treeview = GTK_TREE_VIEW (gtk_builder_get_object(builder, "treeview_buy_invoice"));
  gtk_tree_view_set_model (treeview, GTK_TREE_MODEL(store));

  selection = gtk_tree_view_get_selection (treeview);
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
  g_signal_connect (G_OBJECT(selection), "changed",
		    G_CALLBACK (on_selection_buy_invoice_change), NULL);

  //ID
  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("N° Factura", renderer,
						    "text", 0,
						    NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 0);
  gtk_tree_view_column_set_resizable (column, FALSE);

  //Fecha Ingreso
  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("Fecha Ing.", renderer,
						    "text", 1,
						    NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 1);
  gtk_tree_view_column_set_resizable (column, FALSE);

  //Fecha Documento
  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("Fecha Doc.", renderer,
						    "text", 2,
						    NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 2);
  gtk_tree_view_column_set_resizable (column, FALSE);

  //Pagado
  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("Pagada", renderer,
						    "text", 3,
						    NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 3);
  gtk_tree_view_column_set_resizable (column, FALSE);

  //Fecha Pago
  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("Fecha Pago", renderer,
						    "text", 4,
						    NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 4);
  gtk_tree_view_column_set_resizable (column, FALSE);

  //Buy amount
  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("Monto Compra", renderer,
						    "text", 5,
						    NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 5);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)5, NULL);

  //Nullify amount
  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("Monto Anulado", renderer,
						    "text", 6,
						    NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 6);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)6, NULL);


  libro_compra->son->tree = treeview;
  libro_compra->son->cols[0].name = "N° Factura";
  libro_compra->son->cols[0].num = 0;
  libro_compra->son->cols[1].name = "Fecha Ingreso";
  libro_compra->son->cols[1].num = 1;
  libro_compra->son->cols[2].name = "Fecha Documento";
  libro_compra->son->cols[2].num = 2;
  libro_compra->son->cols[3].name = "Pagada";
  libro_compra->son->cols[3].num = 3;
  libro_compra->son->cols[4].name = "Fecha Pago";
  libro_compra->son->cols[4].num = 4;
  libro_compra->son->cols[5].name = "Monto Anulado";
  libro_compra->son->cols[5].num = 5;
  libro_compra->son->cols[6].name = NULL;


  //Detalle Factura
  store = gtk_list_store_new (9,
			      G_TYPE_STRING, //barcode
			      G_TYPE_STRING, //description
			      G_TYPE_DOUBLE, //cantity purchased
			      G_TYPE_DOUBLE, //cantity nullified
			      G_TYPE_DOUBLE, //Price
			      G_TYPE_INT,    //Buy amount
			      G_TYPE_INT,    //Nullify amount
			      G_TYPE_INT,    //Id_compra
			      G_TYPE_INT);   //id_factura_compra

  treeview = GTK_TREE_VIEW(gtk_builder_get_object(builder, "treeview_buy_invoice_details"));
  gtk_tree_view_set_model(treeview, GTK_TREE_MODEL(store));
  gtk_tree_selection_set_mode (gtk_tree_view_get_selection (treeview), GTK_SELECTION_NONE);

  //barcode
  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("Cod. Barras", renderer,
						    "text", 0,
						    NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 0);
  gtk_tree_view_column_set_resizable (column, FALSE);

  //description
  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("Descripcion", renderer,
						    "text", 1,
						    NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 1);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_expand (column, TRUE);

  //Cantity purchased
  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("Cant. comprada", renderer,
						    "text", 2,
						    NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 2);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)2, NULL);

  //Cantity Nullified
  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("Cant. anulada", renderer,
						    "text", 3,
						    NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 3);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)3, NULL);

  //price
  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("Costo", renderer,
						    "text", 4,
						    NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 4);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)4, NULL);

  //Buy amount
  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("Monto Compra", renderer,
						    "text", 5,
						    NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 5);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)5, NULL);

  //Nullify amount
  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("Monto Anulado", renderer,
						    "text", 6,
						    NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 6);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)6, NULL);

  libro_compra->son->son->tree = treeview;
  libro_compra->son->son->cols[0].name = "Barcode";
  libro_compra->son->son->cols[0].num = 0;
  libro_compra->son->son->cols[1].name = "Descripcion";
  libro_compra->son->son->cols[1].num = 1;
  libro_compra->son->son->cols[2].name = "Cant. comprada";
  libro_compra->son->son->cols[2].num = 2;
  libro_compra->son->son->cols[3].name = "Cant. anulada";
  libro_compra->son->son->cols[3].num = 3;
  libro_compra->son->son->cols[4].name = "Precio";
  libro_compra->son->son->cols[4].num = 4;
  libro_compra->son->son->cols[5].name = "Monto Compra";
  libro_compra->son->son->cols[5].num = 5;
  libro_compra->son->son->cols[6].name = "Monto Anulado";
  libro_compra->son->son->cols[6].num = 6;
  libro_compra->son->son->cols[7].name = NULL;

  g_signal_connect (builder_get (builder, "btn_print_buy_products"), "clicked",
                    G_CALLBACK (PrintThreeTree), (gpointer)libro_compra);


  /*
    END Informe Compras
   */


  /*
    START informe ventas exentas a impuesto
   */
  store = gtk_list_store_new (6,
                              G_TYPE_STRING,  //Fecha
                              G_TYPE_STRING,  //ID Venta
                              G_TYPE_STRING,  //Maquina
                              G_TYPE_STRING,  //Vendedor
			      G_TYPE_STRING,  //Tipo Pago
                              G_TYPE_INT);    //Monto
                            //G_TYPE_STRING);

  treeview = GTK_TREE_VIEW (builder_get (builder, "treeview_sells_exempt_tax"));
  gtk_tree_view_set_model (treeview, GTK_TREE_MODEL (store));

  selection = gtk_tree_view_get_selection (treeview);

  g_signal_connect (G_OBJECT (selection), "changed",
		    G_CALLBACK (fill_exempt_sells_details), NULL);

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
  column = gtk_tree_view_column_new_with_attributes ("Tipo Pago", renderer,
                                                     "text", 4,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 4);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Monto", renderer,
                                                     "text", 5,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_alignment (column, 0.5);
  gtk_tree_view_column_set_sort_column_id (column, 5);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)5, NULL);

  /* renderer = gtk_cell_renderer_text_new (); */
  /* column = gtk_tree_view_column_new_with_attributes ("Estado", renderer, */
  /*                                                    "text", 6, */
  /*                                                    NULL); */
  /* gtk_tree_view_append_column (treeview, column); */
  /* gtk_tree_view_column_set_alignment (column, 0.5); */
  /* g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL); */
  /* gtk_tree_view_column_set_sort_column_id (column, 6); */
  /* gtk_tree_view_column_set_resizable (column, FALSE); */

  exempt->tree = treeview;
  exempt->title = "Libro de Ventas Exentas";
  exempt->name = "ventas";
  exempt->date_string = NULL;
  exempt->cols[0].name = "Fecha";
  exempt->cols[0].num = 0;
  exempt->cols[1].name = "Monto";
  exempt->cols[1].num = 4;
  exempt->cols[2].name = NULL;


  // Ventas exetas a impuesto (detalle)
  store = gtk_list_store_new (5,
			      G_TYPE_STRING,  //Codigo_corto
                              G_TYPE_STRING,  //marca, descripcion, contenido, unidad
                              G_TYPE_DOUBLE,  //cantidad
                              G_TYPE_DOUBLE,  //unitario
                              G_TYPE_INT);    //total

  treeview = GTK_TREE_VIEW (builder_get (builder, "treeview_sells_exempt_tax_details"));
  gtk_tree_view_set_model (treeview, GTK_TREE_MODEL (store));

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Codigo", renderer,
                                                     "text", 0,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 0);
  gtk_tree_view_column_set_resizable (column, FALSE);
  //gtk_tree_view_column_set_min_width (column, 260);
  //gtk_tree_view_column_set_max_width (column, 260);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Producto", renderer,
                                                     "text", 1,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 1);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_min_width (column, 260);
  gtk_tree_view_column_set_max_width (column, 260);
  gtk_tree_view_column_set_expand (column, TRUE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Cantidad", renderer,
                                                     "text", 2,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 2);
  gtk_tree_view_column_set_resizable (column, FALSE);

  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)2, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Unitario", renderer,
                                                     "text", 3,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 3);
  gtk_tree_view_column_set_resizable (column, FALSE);

  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)3, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Total", renderer,
                                                     "text", 4,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 4);
  gtk_tree_view_column_set_resizable (column, FALSE);

  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)4, NULL);

  exempt->son->tree = treeview;
  exempt->son->cols[0].name = "Codigo";
  exempt->son->cols[0].num = 0;
  exempt->son->cols[1].name = "Producto";
  exempt->son->cols[1].num = 1;
  exempt->son->cols[2].name = "Cantidad";
  exempt->son->cols[2].num = 2;
  exempt->son->cols[3].name = "Unitario";
  exempt->son->cols[3].num = 3;
  exempt->son->cols[4].name = "Total";
  exempt->son->cols[4].num = 4;
  exempt->son->cols[5].name = NULL;

  //g_signal_connect (builder_get (builder, "btn_print_sells"), "clicked",
  //G_CALLBACK (PrintTwoTree), (gpointer)libro);


  //Montos Vendidos con Cheques de restaurant
  store = gtk_list_store_new (4,
			      G_TYPE_INT,     //id_venta
			      G_TYPE_STRING,  //Fecha Venta
			      G_TYPE_STRING,  //Codigo Cheque
			      G_TYPE_INT);    //Monto Cheque

  treeview = GTK_TREE_VIEW (builder_get (builder, "treeview_ticket_amount"));
  gtk_tree_view_set_model (treeview, GTK_TREE_MODEL (store));

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("ID Venta", renderer,
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
  column = gtk_tree_view_column_new_with_attributes ("F.Venta", renderer,
                                                     "text", 1,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 1);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Cod. Cheque", renderer,
                                                     "text", 2,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 2);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Monto Cheque", renderer,
                                                     "text", 3,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 3);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)3, NULL);

  /* End Sells (tax exempt) */


  /*
    Start 'Mercadería inmovilizada'
   */
  store = gtk_list_store_new (10,
			      G_TYPE_STRING,  //Barcode mercaderia
			      G_TYPE_STRING,  //Codigo mercaderia
			      G_TYPE_STRING,  //Descripcion mercaderia
			      G_TYPE_DOUBLE,  //Stock Inicial
			      G_TYPE_DOUBLE,  //Unidades Vendidas
			      G_TYPE_DOUBLE,  //Monto Vendido ($)
			      G_TYPE_DOUBLE,  //% de venta
			      G_TYPE_DOUBLE,  //Stock Actual
			      G_TYPE_DOUBLE,  //Costo Unitario
			      G_TYPE_DOUBLE); //Total inmovilizado ($)

  treeview = GTK_TREE_VIEW (builder_get (builder, "treeview_mercaderia_inmovil"));
  gtk_tree_view_set_model (treeview, GTK_TREE_MODEL (store));

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Codigo", renderer,
                                                     "text", 1,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 1);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Descripcion", renderer,
                                                     "text", 2,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 2);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_expand (column, TRUE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("stock\nInicial", renderer,
                                                     "text", 3,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 3);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)3, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Unidades\nVendidas", renderer,
                                                     "text", 4,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 4);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)4, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Total\nVendido ($)", renderer,
                                                     "text", 5,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 5);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)5, NULL);


  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Porcentaje\nVendido (%)", renderer,
                                                     "text", 6,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 6);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)6, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("stock\nActual", renderer,
                                                     "text", 7,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 7);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)7, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Costo\nUnitario ($)", renderer,
                                                     "text", 8,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 8);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)8, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Total\nInmovilizado ($)", renderer,
                                                     "text", 9,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 9);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)9, NULL);

  inmovil->tree = treeview;
  inmovil->title = "Informe de mercaderias inmovilizadas";
  inmovil->date_string = NULL;
  inmovil->cols[0].name = "Barcode";
  inmovil->cols[1].name = "Codigo Corto";
  inmovil->cols[2].name = "Descripción";
  inmovil->cols[3].name = "Stock Inicial";
  inmovil->cols[4].name = "Unidades Vendidas";
  inmovil->cols[5].name = "Monto Vendido ($)";
  inmovil->cols[6].name = "Porcentaje venta (%)";
  inmovil->cols[7].name = "Stock Actual";
  inmovil->cols[8].name = "Costo Unitario";
  inmovil->cols[9].name = "Total Inmovilizado";
  inmovil->cols[10].name = NULL;

  g_signal_connect (builder_get (builder, "btn_print_low_sale"), "clicked",
                    G_CALLBACK (PrintTree), (gpointer)inmovil);

  //Filtros
  g_signal_connect (G_OBJECT (builder_get (builder, "entry_percent_sale")), "insert-text",
		    G_CALLBACK (only_numberd_filter), NULL);
  g_signal_connect (G_OBJECT (builder_get (builder, "entry_unit_sale")), "insert-text",
		    G_CALLBACK (only_numberd_filter), NULL);

  /*
     End 'Mercadería inmovilizada'
   */



  /*
    Start 'Movimiento Mercaderías'
   */
  store = gtk_list_store_new (5,
			      G_TYPE_STRING,  //Fecha
			      G_TYPE_DOUBLE,  //Stock
			      G_TYPE_INT,     //Valor Stock
			      G_TYPE_DOUBLE,  //Cantidad vendida
			      G_TYPE_INT);    //Valor Vendido

  treeview = GTK_TREE_VIEW (builder_get (builder, "treeview_movimiento"));
  gtk_tree_view_set_model (treeview, GTK_TREE_MODEL (store));

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Fecha", renderer,
                                                     "text", 0,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 0);
  gtk_tree_view_column_set_resizable (column, FALSE);


  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Stock", renderer,
                                                     "text", 1,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 1);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)1, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Valor Stock ($)", renderer,
                                                     "text", 2,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 2);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)2, NULL);


  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Unidades Vendidas", renderer,
                                                     "text", 3,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 3);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)3, NULL);


  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Valor Vendido ($)", renderer,
                                                     "text", 4,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_sort_column_id (column, 4);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)4, NULL);


  movimiento->tree = treeview;
  movimiento->title = "Informe de movimiento de mercaderia";
  movimiento->date_string = NULL;
  movimiento->cols[0].name = "Fecha";
  movimiento->cols[1].name = "Stock (Unidades)";
  movimiento->cols[2].name = "Valor Stock ($)";
  movimiento->cols[3].name = "Vendido (Unidades)";
  movimiento->cols[4].name = "Valor Vendido ($)";
  movimiento->cols[10].name = NULL;

  g_signal_connect (builder_get (builder, "btn_print_movimiento"), "clicked",
                    G_CALLBACK (PrintTree), (gpointer)movimiento);

  /*
     End 'Movimiento_mercadería'
   */


  //Titulo
  gtk_window_set_title (GTK_WINDOW (gtk_builder_get_object (builder, "wnd_reports")),
			g_strdup_printf ("POS Rizoma Comercio: Informes - Conectado a [%s@%s]",
					 config_profile,
					 rizoma_get_value ("SERVER_HOST")));

  gtk_widget_show_all (GTK_WIDGET (gtk_builder_get_object (builder, "wnd_reports")));

  // --- Se ocultan los widget de filtro de informes ---
  gtk_widget_hide (GTK_WIDGET (builder_get (builder, "cmb_family_filter")));
  gtk_widget_hide (GTK_WIDGET (builder_get (builder, "btn_apply_family_filter")));
  gtk_widget_hide (GTK_WIDGET (builder_get (builder, "cmb_stores")));
  gtk_widget_hide (GTK_WIDGET (builder_get (builder, "btn_filter_stores")));
  gtk_widget_hide (GTK_WIDGET (builder_get (builder, "cmb_transfer_type")));
  gtk_widget_hide (GTK_WIDGET (builder_get (builder, "btn_choose_transfer_type")));

  fill_filter_nullify_cmbbox ("cmb_buy_report");
  fill_filter_nullify_cmbbox ("cmb_sell_report");
  fill_transfer_type_cmbbox ("cmb_transfer_type");

  // Se inicializan con la fecha actual
  gtk_entry_set_text ((GtkEntry *) gtk_builder_get_object (builder,"entry_date_begin"), CurrentDate(1));
  gtk_entry_set_text ((GtkEntry *) gtk_builder_get_object (builder,"entry_date_end"), CurrentDate(1));

  /*Set filter hour visibility*/
  if (!rizoma_get_value_boolean ("INFORME_FILTRO_HORA"))
    gtk_widget_hide (GTK_WIDGET(builder_get (builder, "hboxHourFilter")));

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
  /* g_thread_init(NULL); */

  /* gdk_threads_init(); */

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
  /* gdk_threads_enter(); */
  gtk_main();
  /* gdk_threads_leave(); */

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
          sell_type = atoi (PQvaluebycol (res_sells, i, "tipo_venta"));
          switch (sell_type)
            {
            case CASH:
              pago = "Contado";
              break;
            case CREDITO:
              pago = "Credito";
              break;
	    case CHEQUE_RESTAURANT:
              pago = "Cheque Rest.";
              break;
	    case MIXTO:
              pago = "Mixto";
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
                              1, atoi (PQgetvalue (res_sells, i, 0)),
                              2, PQgetvalue (res_sells, i, 1),
                              3, PQgetvalue (res_sells, i, 2),
                              4, atoi (PQgetvalue (res_sells, i, 3)),
                              5, pago,
			      6, g_strdup_printf("%s", (g_str_equal (PQgetvalue (res_sells, i, 6),
								     PQgetvalue (res_sells, i, 0))) ?
						 "Anulada":"Vigente"),
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
 * visualizando en el tree_view_sells.
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
	      sell_type = atoi (PQvaluebycol (res_sells, i, "tipo_venta"));
	      switch (sell_type)
		{
		case CASH:
		  pago = "Contado";
		  break;
		case CREDITO:
		  pago = "Credito";
		  break;
		case CHEQUE_RESTAURANT:
		  pago = "Cheque Rest.";
		  break;
		case MIXTO:
		  pago = "Mixto";
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
				  1, atoi (PQgetvalue (res_sells, i, 0)),
				  2, PQgetvalue (res_sells, i, 1),
				  3, PQgetvalue (res_sells, i, 2),
				  4, atoi (PQgetvalue (res_sells, i, 3)),
				  5, pago,
				  6, g_strdup_printf("%s", (g_str_equal (PQgetvalue (res_sells, i, 6),
									 PQgetvalue (res_sells, i, 0))) ?
						     "Anulada":"Vigente"),
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
              sell_type = atoi (PQvaluebycol (res_sells, i, "tipo_venta"));
              switch (sell_type)
                {
                case CASH:
                  pago = "Contado";
                  break;
                case CREDITO:
                  pago = "Credito";
                  break;
		case CHEQUE_RESTAURANT:
		  pago = "Cheque Rest.";
		  break;
		case MIXTO:
		  pago = "Mixto";
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
                                  1, atoi (PQgetvalue (res_sells, i, 0)),
                                  2, PQgetvalue (res_sells, i, 1),
                                  3, PQgetvalue (res_sells, i, 2),
                                  4, atoi (PQgetvalue (res_sells, i, 3)),
                                  5, pago,
				  6, g_strdup_printf("%s", (g_str_equal (PQgetvalue (res_sells, i, 6),
									 PQgetvalue (res_sells, i, 0))) ?
						     "Anulada":"Vigente"),
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
          sell_type = atoi (PQvaluebycol (res_sells, i, "tipo_venta"));
          switch (sell_type)
            {
            case CASH:
              pago = "Contado";
              break;
            case CREDITO:
              pago = "Credito";
              break;
	    case CHEQUE_RESTAURANT:
              pago = "Cheque Rest.";
              break;
	    case MIXTO:
              pago = "Mixto";
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
                              1, atoi (PQgetvalue (res_sells, i, 0)),
                              2, PQgetvalue (res_sells, i, 1),
                              3, PQgetvalue (res_sells, i, 2),
                              4, atoi (PQgetvalue (res_sells, i, 3)),
                              5, pago,
			      6, g_strdup_printf("%s", (g_str_equal (PQgetvalue (res_sells, i, 6),
								     PQgetvalue (res_sells, i, 0))) ?
						 "Anulada":"Vigente"),
                              -1);
        }
    }

  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_adelante")), FALSE);
  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_ultimo")), FALSE);

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
  GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (builder_get (builder,"tree_view_sells"))));
  gchar *pago = NULL;
  gint tuples, i;
  gint sell_type;
  GtkTreeIter iter;

  GtkWidget *combo;
  GtkTreeModel *model_combo;
  GtkTreeIter iter_combo;
  gchar *store_combo;
  gint active;

  // Combo-Box
  combo = GTK_WIDGET (gtk_builder_get_object(builder, "cmb_sell_report"));
  active = gtk_combo_box_get_active (GTK_COMBO_BOX (combo));

  /* Verifica si hay alguna opción seleccionada*/
  if (active == -1)
    store_combo = "TODAS";
  else
    {
      model_combo = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));
      gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combo), &iter_combo);

      gtk_tree_model_get (model_combo, &iter_combo,
                          1, &store_combo,
                          -1);
    }

  printf ("%s\n",store_combo);

  gtk_list_store_clear (store);
  gtk_list_store_clear (GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (builder_get (builder, "tree_view_sell_detail")))));

  /* esta funcion  SearchTuplesByDate() llama a una consulta de sql, que
     retorna los datos de ventas en un intervalo de fechas*/
    res_sells = SearchTuplesByDate
      (g_date_get_year (date_begin), g_date_get_month (date_begin), g_date_get_day (date_begin), hrIni, minIni,
       g_date_get_year (date_end), g_date_get_month (date_end), g_date_get_day (date_end), hrFin, minFin,
       "fecha", " id, maquina, "
                " (SELECT usuario FROM users WHERE id = vendedor) AS vendedor, "
                " monto, to_char (fecha, 'DD/MM/YY HH24:MI:SS') as fmt_fecha, tipo_venta,"
                " (SELECT id_sale FROM venta_anulada WHERE id_sale = id) id_null_sell",
       store_combo);

  tuples = PQntuples (res_sells);

  if (tuples > 0)
    gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_print_sells")), TRUE);

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
    {
      gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_print_sells")), FALSE);
      return;
    }

  /* verifica que tipo de venta es*/
  for (i = 0; i < tuples; i++)
    {
      if (i < 100 )
        {
          sell_type = atoi (PQvaluebycol (res_sells, i, "tipo_venta"));
          switch (sell_type)
            {
            case CASH:
              pago = "Contado";
              break;
            case CREDITO:
              pago = "Credito";
              break;
	    case CHEQUE_RESTAURANT:
              pago = "Cheque Rest.";
              break;
	    case MIXTO:
              pago = "Mixto";
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
                              1, atoi (PQgetvalue (res_sells, i, 0)),
                              2, PQgetvalue (res_sells, i, 1),
                              3, PQgetvalue (res_sells, i, 2),
                              4, atoi (PQgetvalue (res_sells, i, 3)),
                              5, pago,
			      6, g_strdup_printf("%s", (g_str_equal (PQgetvalue (res_sells, i, 6),
								     PQgetvalue (res_sells, i, 0))) ?
						 "Anulada":"Vigente"),
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
 * Es llamada cuando se presiona el boton "btn_ultimo" (signal clicked).
 *
 * Esta funcion inserta las 100 ultimas ventas en el tree_view_sells.
 *
 */
void
on_togglebtn_clicked()
{
  gchar *str_tb = g_strdup (gtk_label_get_text (GTK_LABEL (builder_get (builder, "labelTB"))));

  if(strcmp(str_tb, "Activado") == 0)
    {
      gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "labelTB")), "Desactivado");
      //gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_print_sells")), FALSE);
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
      //gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_print_sells")), TRUE);

      GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (builder_get (builder,
												 "tree_view_sells"))));
      gchar *pago = NULL;
      gint tuples, i;
      gint sell_type;
      GtkTreeIter iter;

      GtkWidget *combo;
      GtkTreeModel *model_combo;
      GtkTreeIter iter_combo;
      gchar *store_combo;
      gint active;

      // Combo-Box
      combo = GTK_WIDGET (gtk_builder_get_object(builder, "cmb_sell_report"));
      active = gtk_combo_box_get_active (GTK_COMBO_BOX (combo));

      /* Verifica si hay alguna opción seleccionada*/
      if (active == -1)
	store_combo = "TODAS";
      else
	{
	  model_combo = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));
	  gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combo), &iter_combo);

	  gtk_tree_model_get (model_combo, &iter_combo,
			      1, &store_combo,
			      -1);
	}

      printf ("%s\n",store_combo);

      gtk_list_store_clear (store);
      gtk_list_store_clear (GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (builder_get (builder,
												 "tree_view_sell_detail")))));

      /* esta funcion  SearchTuplesByDate() llama a una consulta de sql, que
	 retorna los datos de ventas en un intervalo de fechas*/
      res_sells = SearchTuplesByDate
	(g_date_get_year (date_begin), g_date_get_month (date_begin), g_date_get_day (date_begin), hrIni, minIni,
         g_date_get_year (date_end), g_date_get_month (date_end), g_date_get_day (date_end), hrFin, minFin,
	 "fecha", " id, maquina, vendedor, monto, to_char (fecha, 'DD/MM/YY HH24:MI:SS') as fmt_fecha, tipo_venta,"
	          " (SELECT id_sale FROM venta_anulada WHERE id_sale = id) id_null_sell",
	 store_combo);

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
	  sell_type = atoi (PQvaluebycol (res_sells, i, "tipo_venta"));
	  switch (sell_type)
	    {
	    case CASH:
	      pago = "Contado";
	      break;
	    case CREDITO:
	      pago = "Credito";
	      break;
	    case CHEQUE_RESTAURANT:
              pago = "Cheque Rest.";
              break;
	    case MIXTO:
              pago = "Mixto";
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
			      1, atoi (PQgetvalue (res_sells, i, 0)),
			      2, PQgetvalue (res_sells, i, 1),
			      3, PQgetvalue (res_sells, i, 2),
			      4, atoi (PQgetvalue (res_sells, i, 3)),
			      5, pago,
			      6, g_strdup_printf("%s", (g_str_equal (PQgetvalue (res_sells, i, 6),
								     PQgetvalue (res_sells, i, 0))) ?
						 "Anulada":"Vigente"),
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
 * opcion 9 del switch.
 *
 * Esta funcion a traves de una consulta sql retorna los datos de
 * ventas exentas en un lapso de tiempo, luego las visualiza a traves del
 * tree_view correspondiente.
 *
 */
void
fill_exempt_sells ()
{
  GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (builder_get (builder,"treeview_sells_exempt_tax"))));
  gchar *pago = NULL;
  gint tuples, i;
  gint sell_type;
  GtkTreeIter iter;
  PGresult *res;
  gchar *q;

  /*Datos labels*/
  gint monto_ventas_exentas, monto_cheques;
  gchar *nventas, *ncheques;
  monto_ventas_exentas = monto_cheques = 0;

  gtk_list_store_clear (store);
  gtk_list_store_clear (GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (builder_get (builder, "treeview_sells_exempt_tax_details")))));

  /* esta funcion  SearchTuplesByDate() llama a una consulta de sql, que
     retorna los datos de ventas en un intervalo de fechas */
  res = exempt_sells_on_date
    (g_date_get_year (date_begin), g_date_get_month (date_begin), g_date_get_day (date_begin),
     g_date_get_year (date_end), g_date_get_month (date_end), g_date_get_day (date_end));

  tuples = PQntuples (res);
  nventas = PutPoints (g_strdup_printf ("%d", tuples));

  /* verifica que tipo de venta es*/
  for (i = 0; i < tuples; i++)
    {
      sell_type = atoi (PQvaluebycol (res, i, "tipo_venta"));
      switch (sell_type)
	{
	case CASH:
	  pago = "Contado";
	  break;
	case CREDITO:
	  pago = "Credito";
	  break;
	case CHEQUE_RESTAURANT:
	  pago = "Cheque Rest.";
	  break;
	case MIXTO:
	  pago = "Mixto";
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
			  0, PQvaluebycol (res, i, "fmt_fecha"),
			  1, PQvaluebycol (res, i, "id"),
			  2, PQvaluebycol (res, i, "maquina"),
			  3, PQvaluebycol (res, i, "vendedor"),
			  4, pago,
			  5, atoi (PQvaluebycol (res, i, "monto")),
			  -1);
      monto_ventas_exentas += atoi (PQvaluebycol (res, i, "monto"));
    }


  /* Rellena la información de los cheques de restaurant en el lapso
     de tiempo determinado */
  store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (gtk_builder_get_object (builder, "treeview_ticket_amount"))));
  gtk_list_store_clear (store);

  q = g_strdup_printf ("SELECT cr.id, cr.id_venta, cr.codigo, cr.facturado, cr.monto, "
		       "cr.pagado, v.monto AS monto_venta, "

		       "date_part ('year', v.fecha) AS fvta_year, "
		       "date_part ('month', v.fecha) AS fvta_month, "
		       "date_part ('day', v.fecha) AS fvta_day, "

		       "date_part ('year', fecha_vencimiento) AS fvto_year, "
		       "date_part ('month', fecha_vencimiento) AS fvto_month, "
		       "date_part ('day', fecha_vencimiento) AS fvto_day "

		       "FROM cheque_rest cr INNER JOIN venta v ON cr.id_venta = v.id "
		       "WHERE v.fecha >= to_timestamp ('%.2d %.2d %.4d', 'DD MM YYYY') "
		       "AND v.fecha <= to_timestamp ('%.2d %.2d %.4d', 'DD MM YYYY')",
		       g_date_get_day (date_begin), g_date_get_month (date_begin), g_date_get_year (date_begin),
		       g_date_get_day (date_end)+1, g_date_get_month (date_end), g_date_get_year (date_end));

  res = EjecutarSQL (q);
  tuples = PQntuples (res);
  ncheques = PutPoints (g_strdup_printf ("%d", tuples));

  for (i = 0; i < tuples; i++)
    {
      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
			  0, atoi (PQvaluebycol (res, i, "id_venta")),
			  1, g_strdup_printf ("%s-%s-%s",
					      PQvaluebycol (res, i, "fvta_day"),
					      PQvaluebycol (res, i, "fvta_month"),
					      PQvaluebycol (res, i, "fvta_year")),
			  2, PQvaluebycol (res, i, "codigo"),
			  3, atoi (PQvaluebycol (res, i, "monto")),
			  -1);
      monto_cheques += atoi (PQvaluebycol (res, i, "monto"));
    }

  //Numero de ventas
  gtk_label_set_text (GTK_LABEL (builder_get (builder, "lbl_num_exempt_sells")), nventas);
  //Numero de cheques
  gtk_label_set_text (GTK_LABEL (builder_get (builder, "lbl_num_cheques")), ncheques);
  //Total ventas no afectas a impuesto
  gtk_label_set_text (GTK_LABEL (builder_get (builder, "lbl_total_exempt_sell_amount")),
		      g_strconcat ("$ ",PutPoints (g_strdup_printf ("%d", monto_ventas_exentas)), NULL));
  //Total cheques
  gtk_label_set_text (GTK_LABEL (builder_get (builder, "lbl_cheques_amount")),
		      g_strconcat ("$ ",PutPoints (g_strdup_printf ("%d", monto_cheques)), NULL));
  //Total no afecto a impuesto
  gtk_label_set_text (GTK_LABEL (builder_get (builder, "lbl_total_exempt_amount")),
		      g_strconcat ("$ ",PutPoints (g_strdup_printf ("%d", monto_cheques+monto_ventas_exentas)), NULL));

}


/**
 * Es llamada por la funcion "on_btn_get_stat_clicked()", si se escoge la
 * opcion 10 del switch.
 *
 * Esta funcion a traves de una consulta sql retorna los datos de
 * de las mercaderias inmovilizadas en un lapso de tiempo, luego las
 * visualiza a traves del tree_view correspondiente.
 *
 */
void
fill_inmovilizados (GtkWidget *widget, gpointer user_data)
{
  GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (builder_get (builder,"treeview_mercaderia_inmovil"))));
  gint tuples, i;
  GtkTreeIter iter;
  PGresult *res;
  gchar *avg_unid_vend, *unid_vend;

  avg_unid_vend = g_strdup (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_percent_sale"))));
  unid_vend = g_strdup (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_unit_sale"))));

  gtk_list_store_clear (store);

  /*Se obtienen los datos de ventas de mercaderías desde una fecha inicial hasta ahora*/
  res = inmovilizados_en_periodo
    (g_date_get_year (date_begin), g_date_get_month (date_begin), g_date_get_day (date_begin), avg_unid_vend, unid_vend);

  tuples = PQntuples (res);

  for (i = 0; i < tuples; i++)
    {
      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
      			  0, PQvaluebycol (res, i, "barcode"),
      			  1, PQvaluebycol (res, i, "codigo_corto"),
      			  2, PQvaluebycol (res, i, "descripcion"),
      			  3, strtod (PUT (PQvaluebycol (res, i,"stock_inicial")), (char **)NULL),
      			  4, strtod (PUT (PQvaluebycol (res, i,"unidades_vendidas")), (char **)NULL),
      			  5, (strtod (PUT (PQvaluebycol (res, i,"unidades_vendidas")), (char **)NULL) *
			      strtod (PUT (PQvaluebycol (res, i,"precio")), (char **)NULL)),
      			  6, strtod (PUT (PQvaluebycol (res, i,"porcentaje_vendido")), (char **)NULL),
      			  7, strtod (PUT (PQvaluebycol (res, i,"stock_teorico")), (char **)NULL),
      			  8, strtod (PUT (PQvaluebycol (res, i,"costo_promedio")), (char **)NULL),
			  9, (strtod (PUT (PQvaluebycol (res, i,"costo_promedio")), (char **)NULL)*
			      strtod (PUT (PQvaluebycol (res, i,"stock_teorico")), (char **)NULL)),
      			  -1);
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
                           "and fecha<to_timestamp ('%.2d %.2d %.4d', 'DD MM YYYY')  order by id"
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
                            2, PutPoints (PQgetvalue (res, i, 2)),
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
  gchar *q;

  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_get_stat")), FALSE);

  /* consulta de sql que retona las suma, promedio y numero de devoluciones */
  q = g_strdup_printf ("select count(*), sum(monto), avg(monto) from devolucion where fecha>=to_timestamp ('%.2d %.2d %.4d', 'DD MM YYYY') "
		       "and fecha<to_timestamp ('%.2d %.2d %.4d', 'DD MM YYYY')",
		       g_date_get_day (date_begin), g_date_get_month (date_begin), g_date_get_year (date_begin),
		       g_date_get_day (date_end)+1, g_date_get_month (date_end), g_date_get_year (date_end));

  res = EjecutarSQL (q);

  if (res == NULL) return (NULL); //TODO: OJO con este retorno

  /*se visualizan los datos en las correspondientes etiquetas (labels)*/
  else
    {
      gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_devolucion_amount_total")),
                            g_strdup_printf ("<b>%s</b>",
                                             PUT(g_strdup_printf ("%.2f",
								  strtod (PUT (PQvaluebycol (res, 0,"sum")),
									  (char **)NULL) ))));
      gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_devoluciones_num")),
                            g_strdup_printf ("<b>%s</b>",
                                             PutPoints (g_strdup_printf ("%d",
									 atoi (PQvaluebycol (res, 0,"count"))))));
      gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_devoluciones_avg")),
                            g_strdup_printf ("<b>%s</b>",
                                             PUT(g_strdup_printf ("%.2f",
								  strtod (PUT (PQvaluebycol (res, 0,"avg")),
									  (char **)NULL) ))));
    }


  //gtk_timeout_remove (flagProgress);
  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_get_stat")), TRUE);
  GtkWidget *progreso = GTK_WIDGET (builder_get (builder, "progressbar"));
  gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR (progreso), 0);
  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (progreso), ".. Listo ..");

  return (NULL); //TODO: OJO con este retorno
}

/**
 * Es llamada por la funcion "on_btn_get_stat_clicked()", si se escoge la
 * opcion 0 del switch.
 *
 * Esta funcion a traves de una consulta sql retorna los montos totales,
 * promedio, y numero de ventas al contado, credito o descuentos en un lapso
 * de tiempo, luego las  visualiza a traves los labels correspondiente.
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
  gint total_iva;
  gint total_otros;

  gint sell_average;
  gint discount_average;
  gint credit_average;
  gint total_cash_average;

  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_get_stat")), FALSE);

  /* funcion que retorna el total de la ventas al contado en un intervalo de tiempo*/
  total_cash_sell = GetTotalCashSell (g_date_get_year (date_begin), g_date_get_month (date_begin),
				      g_date_get_day (date_begin),  g_date_get_year (date_end),
				      g_date_get_month (date_end), g_date_get_day (date_end),
                                      &total_cash);

  /*Esta funcion entrega el total del iva y otros dentro del rango de fecha entregado*/
  total_taxes_on_time_interval (g_date_get_year (date_begin), g_date_get_month (date_begin),
				g_date_get_day (date_begin),  g_date_get_year (date_end),
				g_date_get_month (date_end), g_date_get_day (date_end),
				&total_iva, &total_otros);

  /* Consulta que retorna el numero de ventas con descuento y la suma total
     con descuento en un intervalo de tiempo */
  res = EjecutarSQL
    (g_strdup_printf ("SELECT * FROM sells_get_totals (to_date ('%.2d %.2d %.4d', 'DD MM YYYY'), to_date ('%.2d %.2d %.4d', 'DD MM YYYY'))",
                      g_date_get_day (date_begin), g_date_get_month (date_begin), g_date_get_year (date_begin),
                      g_date_get_day (date_end), g_date_get_month (date_end), g_date_get_year (date_end)));

  if (res != NULL)
    {
      total_cash_discount = atoi (PQvaluebycol (res, 0, "total_cash_discount"));
      total_discount = atoi (PQvaluebycol (res, 0, "total_discount"));
    }

  /* Funcion que retorna el total de ventas a credito en un intervarlo de tiempo */
  total_credit_sell = GetTotalCreditSell (g_date_get_year (date_begin), g_date_get_month (date_begin),
					  g_date_get_day (date_begin), g_date_get_year (date_end),
					  g_date_get_month (date_end), g_date_get_day (date_end),
                                          &total_credit);

  /* Funcion que retorna el total de todas las ventas(al contado, con
     descuento y a credito ) en un intervarlo de tiempo */
  total_sell = GetTotalSell (g_date_get_year (date_begin), g_date_get_month (date_begin),
			     g_date_get_day (date_begin), g_date_get_year (date_end),
			     g_date_get_month (date_end), g_date_get_day (date_end),
                             &total_ventas);

  /*Taxes*/
  gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_total_iva")),
                        g_strdup_printf ("<span weight='bold'>%s</span>",
                                         PutPoints (g_strdup_printf ("%d", total_iva))));

  gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_total_otros")),
                        g_strdup_printf ("<span weight='bold'>%s</span>",
                                         PutPoints (g_strdup_printf ("%d", total_otros))));

  gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_total_impuestos")),
                        g_strdup_printf ("<span weight='bold'>%s</span>",
                                         PutPoints (g_strdup_printf ("%d", total_iva + total_otros))));

  /*Cash sell*/
  gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_sell_cash_amount")),
                        g_strdup_printf ("<span weight='bold'>%s</span>",
                                         PutPoints (g_strdup_printf ("%d", total_cash_sell))));

  //if (total_cash_sell != 0)
  gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_sell_cash_n")),
			g_strdup_printf ("<span weight='bold'>%s</span>",
					 PutPoints (g_strdup_printf ("%d", total_cash))));

  total_cash_average = (total_cash_sell == 0 || total_cash == 0) ? 0 : total_cash_sell / total_cash;
  //if (total_cash_sell != 0)
  gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_sell_cash_average")),
			g_strdup_printf ("<span weight='bold'>%s</span>",
					 PutPoints (g_strdup_printf ("%d", total_cash_average))));

  /*Credit sell*/
  gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_sell_credit_amount")),
                        g_strdup_printf ("<span weight='bold'>%s</span>",
                                         PutPoints (g_strdup_printf ("%d", total_credit_sell))));

  gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_sell_credit_n")),
                        g_strdup_printf ("<span weight='bold'>%s</span>",
                                         PutPoints (g_strdup_printf ("%d", total_credit))));

  credit_average = (total_credit_sell == 0 || total_credit == 0) ? 0 : total_credit_sell / total_credit;
  //if (total_credit_sell != 0)
  gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_sell_credit_average")),
			g_strdup_printf ("<span weight='bold'>%s</span>",
					 PutPoints (g_strdup_printf ("%d", credit_average))));

  /*Total sell*/
  gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_sell_total_amount")),
                        g_strdup_printf ("<span weight='bold'>%s</span>",
                                         PutPoints (g_strdup_printf ("%d", total_sell))));

  gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_sell_total_n")),
                        g_strdup_printf ("<span weight='bold'>%s</span>",
                                         PutPoints (g_strdup_printf ("%d",total_ventas))));

  sell_average = (total_sell == 0 || total_ventas == 0) ? 0 : total_sell / total_ventas;
  //if (total_ventas != 0)
  gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_sell_average")),
			g_strdup_printf ("<span weight='bold'>%s</span>",
					 PutPoints (g_strdup_printf ("%d", sell_average))));

  /*Discounts*/
  gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_sell_discount")),
                        g_strdup_printf ("<span weight='bold'>%s</span>",
                                         PutPoints (g_strdup_printf ("%d", total_cash_discount))));


  gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_sell_discount_n")),
                        g_strdup_printf ("<span weight='bold'>%s</span>",
                                         PutPoints (g_strdup_printf ("%d", total_discount))));

  discount_average = (total_cash_discount == 0 || total_discount == 0) ? 0 : total_cash_discount / total_discount;
  //if (total_cash_discount != 0)
  gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_sell_discount_avarage")),
			g_strdup_printf ("<span weight='bold'>%s</span>",
					 PutPoints (g_strdup_printf ("%d", discount_average))));

  //gtk_timeout_remove (flagProgress);
  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_get_stat")), TRUE);
  GtkWidget *progreso = GTK_WIDGET (builder_get (builder, "progressbar"));
  gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR (progreso), 0);
  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (progreso), ".. Listo ..");

  return (NULL); //TODO: OJO con este retorno
}

/*---------------------------TRANSFER RANK START------------------*/
/**
 * Es llamada por la funcion "on_btn_get_stat_clicked()", si se escoge la
 * opcion 1 del switch.
 *
 * Esta funcion a traves de una consulta sql retorna los productos transferidos
 * pero rankeados y luego los visualiza en el tree_view correspondiente.
 *
 * @param: traspaso_envio = TRUE muestra los enviadose ELSE muestre los recibidos
 *
 */
void
fill_transfer_rank (gboolean traspaso_envio)
{
  GtkListStore *store =
    GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (builder_get (builder, "treeview_general_transfer_rank"))));
  GtkTreeIter iter;
  PGresult *res;
  gint i, tuples;
  glong total_costo, costo;

  total_costo = 0;
  costo = 0;

  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_get_stat")), FALSE);
  /* funcion que llama una funcion sql que retorna los productos vendidos y
     los ordena por mas vendidos ademas de agregarle otros parametros */
  res = ReturnTransferRank (g_date_get_year (date_begin), g_date_get_month (date_begin), g_date_get_day (date_begin),
                            g_date_get_year (date_end), g_date_get_month (date_end), g_date_get_day (date_end), traspaso_envio);

  tuples = PQntuples (res);

  gtk_list_store_clear (store);

  /*visualiza los productos en el tree_view*/
  for (i = 0; i < tuples; i++)
    {
      costo = lround (strtod (PUT (g_strdup (PQvaluebycol (res, i, "costo"))), (char **)NULL));
      total_costo += costo;
      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
			  0, PQvaluebycol (res, i, "barcode"),
                          1, PQvaluebycol (res, i, "descripcion"),
                          2, PQvaluebycol (res, i, "marca"),
                          3, PQvaluebycol (res, i, "contenido"),
                          4, PQvaluebycol (res, i, "unidad"),
                          5, strtod (PUT(PQvaluebycol (res, i, "amount")), (char **)NULL),
                          6, costo,
                          -1);
    }

  gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_transfer_rank_cost")),
                        g_strdup_printf ("<span size=\"x-large\">$ %s</span>",
                                         PutPoints (g_strdup_printf ("%ld", total_costo))));

  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_get_stat")), TRUE);
}

/**
 * Called by 'on_btn_get_stat_clicked()' when the 'Ranking Traspasos' tab and
 * 'Materias Primas' subtab are selected.
 *
 * Fill the 'treeview_mp_transfer_rank' treeview with raw materials information.
 *
 * @param: traspaso_envio = TRUE muestra los enviadose ELSE muestre los recibidos
 */
void
fill_transfer_rank_mp (gboolean traspaso_envio)
{
  GtkListStore *store =
    GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (builder_get (builder, "treeview_mp_transfer_rank"))));
  GtkListStore *deriv_store =
    GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (builder_get (builder, "treeview_derivates_transfer_rank"))));
  GtkTreeIter iter;
  PGresult *res;
  gint i, tuples;
  glong total_costo, costo;

  total_costo = 0;
  costo = 0;

  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_get_stat")), FALSE);
  /* funcion que llama una funcion sql que retorna los productos vendidos y
     los ordena por mas vendidos ademas de agregarle otros parametros */
  res = ReturnMpTransferRank (g_date_get_year (date_begin), g_date_get_month (date_begin), g_date_get_day (date_begin),
			      g_date_get_year (date_end), g_date_get_month (date_end), g_date_get_day (date_end), traspaso_envio);

  tuples = PQntuples (res);

  gtk_list_store_clear (store);
  gtk_list_store_clear (deriv_store);

  /* viualiza los productos en el tree_view*/

  for (i = 0; i < tuples; i++)
    {
      costo = lround (strtod (PUT (g_strdup (PQvaluebycol (res, i, "costo"))), (char **)NULL));
      total_costo += costo;
      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
			  0, PQvaluebycol (res, i, "barcode"),
			  1, g_strdup (PQvaluebycol (res, i, "descripcion")),
			  2, g_strdup (PQvaluebycol (res, i, "marca")),
			  3, g_strdup (PQvaluebycol (res, i, "contenido")),
			  4, g_strdup (PQvaluebycol (res, i, "unidad")),
			  5, strtod (PUT (g_strdup (PQvaluebycol (res, i, "cantidad"))), (char **)NULL),
			  6, costo,
			  -1);
    }

  gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_transfer_rank_cost")),
                        g_strdup_printf ("<span size=\"x-large\">$ %s</span>",
                                         PutPoints (g_strdup_printf ("%ld", total_costo))));

  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_get_stat")), TRUE);
}


/**
 * Called by 'on_btn_get_stat_clicked()' when the 'Ranking Traspaso' tab and
 * 'Ofertas' subtab are selected.
 *
 * Fill the 'treeview_offer_transfer_rank' treeview with 'complex merchandise' information.
 *
 * @param: traspaso_envio = TRUE muestra los enviadose ELSE muestre los recibidos
 */
void
fill_transfer_rank_mc (gboolean traspaso_envio)
{
  GtkListStore *store =
    GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (builder_get (builder, "treeview_offer_transfer_rank"))));
  GtkListStore *comp_store =
    GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (builder_get (builder, "treeview_components_transfer_rank"))));
  GtkTreeIter iter;
  PGresult *res;
  gint i, tuples;
  glong total_costo, costo;

  total_costo = 0;
  costo = 0;

  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_get_stat")), FALSE);
  /* funcion que llama una funcion sql que retorna los productos vendidos y
     los ordena por mas vendidos ademas de agregarle otros parametros */
  res = ReturnMcTransferRank (g_date_get_year (date_begin), g_date_get_month (date_begin), g_date_get_day (date_begin),
			      g_date_get_year (date_end), g_date_get_month (date_end), g_date_get_day (date_end), traspaso_envio);

  tuples = PQntuples (res);

  gtk_list_store_clear (store);
  gtk_list_store_clear (comp_store);

  /* viualiza los productos en el tree_view*/
  for (i = 0; i < tuples; i++)
    {
      costo = lround (strtod (PUT (g_strdup (PQvaluebycol (res, i, "costo"))), (char **)NULL));
      total_costo += costo;
      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
			  0, PQvaluebycol (res, i, "barcode"),
			  1, g_strdup (PQvaluebycol (res, i, "descripcion")),
			  2, g_strdup (PQvaluebycol (res, i, "marca")),
			  3, g_strdup (PQvaluebycol (res, i, "contenido")),
			  4, g_strdup (PQvaluebycol (res, i, "unidad")),
			  5, strtod (PUT (g_strdup (PQvaluebycol (res, i, "cantidad"))), (char **)NULL),
			  6, costo,
			  -1);
    }

  gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_transfer_rank_cost")),
                        g_strdup_printf ("<span size=\"x-large\">$ %s</span>",
                                         PutPoints (g_strdup_printf ("%ld", total_costo))));

  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_get_stat")), TRUE);
}

/*----------------------------TRANSFER RANK END-------------------*/


/**
 * Es llamada por la funcion "on_btn_get_stat_clicked()", si se escoge la
 * opcion 1 del switch.
 *
 * Esta funcion a traves de una consulta sql retorna los productos vendidos
 * pero rankiados y luego los visualiza en el tree_view correspondiente.
 *
 */

void
fill_products_rank (gint familia)
{
  GtkListStore *store =
    GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (builder_get (builder, "tree_view_sell_rank"))));
  GtkTreeIter iter;
  PGresult *res;
  gint i, tuples;
  gdouble costo, vendido, contribucion,
    costo_total, vendido_total, contribucion_total;
  gdouble margen;

  margen = costo = vendido = contribucion = costo_total = vendido_total = contribucion_total = 0;

  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_get_stat")), FALSE);
  /* funcion que llama una funcion sql que retorna los productos vendidos y
     los ordena por mas vendidos ademas de agregarle otros parametros */
  res = ReturnProductsRank (g_date_get_year (date_begin), g_date_get_month (date_begin), g_date_get_day (date_begin),
                            g_date_get_year (date_end), g_date_get_month (date_end), g_date_get_day (date_end), familia);

  tuples = PQntuples (res);

  gtk_list_store_clear (store);

  /*visualiza los productos en el tree_view*/
  for (i = 0; i < tuples; i++)
    {
      vendido = strtod (PUT (g_strdup (PQvaluebycol (res, i, "sold_amount"))), (char **)NULL);
      costo = strtod (PUT (g_strdup (PQvaluebycol (res, i, "costo"))), (char **)NULL);
      contribucion = strtod (PUT (g_strdup (PQvaluebycol (res, i, "contrib"))), (char **)NULL);

      vendido_total += vendido;
      costo_total += costo;
      contribucion_total += contribucion;

      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
			  0, PQvaluebycol (res, i, "barcode"),
                          1, PQvaluebycol (res, i, "descripcion"),
                          2, PQvaluebycol (res, i, "marca"),
                          3, atoi (PQvaluebycol (res, i, "contenido")),
                          4, PQvaluebycol (res, i, "unidad"),
                          5, strtod (PUT(PQvaluebycol (res, i, "amount")), (char **)NULL),
                          6, lround (vendido),
                          7, lround (costo),
                          8, lround (contribucion),
                          9, ((contribucion / costo) * 100),
                          -1);
    }

  /*res = EjecutarSQL
    (g_strdup_printf ("SELECT round(sum(sold_amount)) as vendidos, round(sum(costo)) as costo, round(sum(contrib)) as contrib, round(((sum(contrib) / sum(costo)) *100)::numeric , 2) as margen FROM ranking_ventas (to_timestamp ('%.2d %.2d %.4d', 'DD MM YYYY')::date, to_timestamp ('%.2d %.2d %.4d', 'DD MM YYYY')::date)",
		      g_date_get_day (date_begin), g_date_get_month (date_begin), g_date_get_year (date_begin),
		      g_date_get_day (date_end)+1, g_date_get_month (date_end), g_date_get_year (date_end))
		      );

		      if (res == NULL) return;*/

  /* visualiza las sumas de los productos en sus respectivos labels */
  gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_rank_sold")),
                        g_strdup_printf ("<span size=\"x-large\">$ %s</span>",
                                         PutPoints (g_strdup_printf ("%ld", lround (vendido_total) ))));

  gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_rank_cost")),
                        g_strdup_printf ("<span size=\"x-large\">$ %s</span>",
                                         PutPoints (g_strdup_printf ("%ld", lround (costo_total) ))));

  gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_rank_contrib")),
                        g_strdup_printf ("<span size=\"x-large\">$ %s</span>",
                                         PutPoints (g_strdup_printf ("%ld", lround (contribucion_total) ))));

  margen = (contribucion_total == 0 || costo_total == 0) ? 0 : (contribucion_total/costo_total)*100;

  gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_rank_margin")),
                        g_strdup_printf ("<span size=\"x-large\">%s %%</span>",
                                         g_strdup_printf ("%.2f", margen )));

  //gtk_timeout_remove (flagProgress);
  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_get_stat")), TRUE);
  GtkWidget *progreso = GTK_WIDGET (builder_get (builder, "progressbar"));
  gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR (progreso), 0);
  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (progreso), ".. Listo ..");
}

/**
 * Called by 'on_btn_get_stat_clicked()' when the 'Ranking Ventas' tab and
 * 'Materias Primas' subtab are selected.
 *
 * Fill the 'treeview_sell_rank_mp' treeview with raw materials information.
 */
void
fill_products_rank_mp (gint familia)
{
  GtkListStore *store =
    GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (builder_get (builder, "treeview_sell_rank_mp"))));
  GtkListStore *deriv_store =
    GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (builder_get (builder, "treeview_sell_rank_deriv"))));
  GtkTreeIter iter;
  PGresult *res;
  gint i, tuples;
  glong costo, vendido, contribucion,
    costo_total, vendido_total, contribucion_total;
  gdouble margen;

  margen = costo = vendido = contribucion = costo_total = vendido_total = contribucion_total = 0;

  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_get_stat")), FALSE);
  /* funcion que llama una funcion sql que retorna los productos vendidos y
     los ordena por mas vendidos ademas de agregarle otros parametros */
  res = ReturnMpProductsRank (g_date_get_year (date_begin), g_date_get_month (date_begin), g_date_get_day (date_begin),
			      g_date_get_year (date_end), g_date_get_month (date_end), g_date_get_day (date_end), familia);

  tuples = PQntuples (res);

  gtk_list_store_clear (store);
  gtk_list_store_clear (deriv_store);

  /* viualiza los productos en el tree_view*/

  for (i = 0; i < tuples; i++)
    {
      vendido = lround (strtod (PUT (g_strdup (PQvaluebycol (res, i, "monto_vendido"))), (char **)NULL));
      costo = lround (strtod (PUT (g_strdup (PQvaluebycol (res, i, "costo"))), (char **)NULL));
      contribucion = lround (strtod (PUT (g_strdup (PQvaluebycol (res, i, "contribucion"))), (char **)NULL));

      vendido_total += vendido;
      costo_total += costo;
      contribucion_total += contribucion;

      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
			  0, PQvaluebycol (res, i, "barcode"),
			  1, g_strdup (PQvaluebycol (res, i, "descripcion")),
			  2, g_strdup (PQvaluebycol (res, i, "marca")),
			  3, g_strdup (PQvaluebycol (res, i, "contenido")),
			  4, g_strdup (PQvaluebycol (res, i, "unidad")),
			  5, strtod (PUT (g_strdup (PQvaluebycol (res, i, "cantidad"))), (char **)NULL),
			  6, vendido,
                          7, costo,
                          8, contribucion,
                          9, (gdouble)(((gdouble)contribucion / (gdouble)costo) * 100),
			  -1);
    }

  /*res = EjecutarSQL
    (g_strdup_printf ("SELECT ROUND (SUM (monto_vendido)) AS vendidos, "
		      "       ROUND (SUM (costo)) AS costo, "
		      "       ROUND (SUM (contribucion)) AS contrib, "
		      "       ROUND (((SUM (contribucion) / SUM (costo)) *100)::NUMERIC, 2) AS margen "
		      "FROM ranking_ventas_mp (TO_TIMESTAMP ('%.2d %.2d %.4d', 'DD MM YYYY')::DATE, "
		      "                        TO_TIMESTAMP ('%.2d %.2d %.4d', 'DD MM YYYY')::DATE)",
		      g_date_get_day (date_begin), g_date_get_month (date_begin), g_date_get_year (date_begin),
		      g_date_get_day (date_end)+1, g_date_get_month (date_end), g_date_get_year (date_end))
     );

     if (res == NULL) return;*/

  /* visualiza las sumas de los productos en sus respectivos labels */
  gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_rank_sold")),
                        g_strdup_printf ("<span size=\"x-large\">$ %s</span>",
                                         PutPoints (g_strdup_printf ("%ld", vendido_total))));

  gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_rank_cost")),
                        g_strdup_printf ("<span size=\"x-large\">$ %s</span>",
                                         PutPoints (g_strdup_printf ("%ld", costo_total))));

  gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_rank_contrib")),
                        g_strdup_printf ("<span size=\"x-large\">$ %s</span>",
                                         PutPoints (g_strdup_printf ("%ld", contribucion_total))));

  margen = (contribucion_total == 0 || costo_total == 0) ? 0 : ((gdouble)contribucion_total/(gdouble)costo_total)*100;
  gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_rank_margin")),
                        g_strdup_printf ("<span size=\"x-large\">%s %%</span>",
                                         g_strdup_printf ("%.2f", margen )));

  //gtk_timeout_remove (flagProgress);
  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_get_stat")), TRUE);
  GtkWidget *progreso = GTK_WIDGET (builder_get (builder, "progressbar"));
  gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR (progreso), 0);
  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (progreso), ".. Listo ..");
}


/**
 * Called by 'on_btn_get_stat_clicked()' when the 'Ranking Ventas' tab and
 * 'Mercaderías Compuestas' subtab are selected.
 *
 * Fill the 'treeview_sell_rank_mc' treeview with 'complex merchandise' information.
 */
void
fill_products_rank_mc (gint familia)
{
  GtkListStore *store =
    GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (builder_get (builder, "treeview_sell_rank_mc"))));
  GtkListStore *comp_store =
    GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (builder_get (builder, "treeview_sell_rank_comp"))));
  GtkTreeIter iter;
  PGresult *res;
  gint i, tuples;
  glong costo, vendido, contribucion,
    costo_total, vendido_total, contribucion_total;
  gdouble margen;

  margen = costo = vendido = contribucion = costo_total = vendido_total = contribucion_total = 0;

  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_get_stat")), FALSE);
  /* funcion que llama una funcion sql que retorna los productos vendidos y
     los ordena por mas vendidos ademas de agregarle otros parametros */
  res = ReturnMcProductsRank (g_date_get_year (date_begin), g_date_get_month (date_begin), g_date_get_day (date_begin),
			      g_date_get_year (date_end), g_date_get_month (date_end), g_date_get_day (date_end), familia);

  tuples = PQntuples (res);

  gtk_list_store_clear (store);
  gtk_list_store_clear (comp_store);

  /* viualiza los productos en el tree_view*/
  for (i = 0; i < tuples; i++)
    {
      vendido = lround (strtod (PUT (g_strdup (PQvaluebycol (res, i, "monto_vendido"))), (char **)NULL));
      costo = lround (strtod (PUT (g_strdup (PQvaluebycol (res, i, "costo"))), (char **)NULL));
      contribucion = lround (strtod (PUT (g_strdup (PQvaluebycol (res, i, "contribucion"))), (char **)NULL));

      vendido_total += vendido;
      costo_total += costo;
      contribucion_total += contribucion;

      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
			  0, PQvaluebycol (res, i, "barcode"),
			  1, g_strdup (PQvaluebycol (res, i, "descripcion")),
			  2, g_strdup (PQvaluebycol (res, i, "marca")),
			  3, g_strdup (PQvaluebycol (res, i, "contenido")),
			  4, g_strdup (PQvaluebycol (res, i, "unidad")),
			  5, strtod (PUT (g_strdup (PQvaluebycol (res, i, "cantidad"))), (char **)NULL),
			  6, vendido,
                          7, costo,
                          8, contribucion,
                          9, (gdouble)(((gdouble)contribucion / (gdouble)costo) * 100),
			  -1);
    }

  /*res = EjecutarSQL
    (g_strdup_printf ("SELECT ROUND (SUM (monto_vendido)) AS vendidos, "
		      "       ROUND (SUM (costo)) AS costo, "
		      "       ROUND (SUM (contribucion)) AS contrib, "
		      "       ROUND (((SUM (contribucion) / SUM (costo)) *100)::NUMERIC, 2) AS margen "
		      "FROM ranking_ventas_mc (TO_TIMESTAMP ('%.2d %.2d %.4d', 'DD MM YYYY')::DATE, "
		      "                        TO_TIMESTAMP ('%.2d %.2d %.4d', 'DD MM YYYY')::DATE)",
		      g_date_get_day (date_begin), g_date_get_month (date_begin), g_date_get_year (date_begin),
		      g_date_get_day (date_end)+1, g_date_get_month (date_end), g_date_get_year (date_end))
     );

  if (res == NULL) return;*/

  /* visualiza las sumas de los productos en sus respectivos labels */
  gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_rank_sold")),
                        g_strdup_printf ("<span size=\"x-large\">$ %s</span>",
                                         PutPoints (g_strdup_printf ("%ld", vendido_total))));

  gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_rank_cost")),
                        g_strdup_printf ("<span size=\"x-large\">$ %s</span>",
                                         PutPoints (g_strdup_printf ("%ld", costo_total))));

  gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_rank_contrib")),
                        g_strdup_printf ("<span size=\"x-large\">$ %s</span>",
                                         PutPoints (g_strdup_printf ("%ld", contribucion_total))));

  margen = (contribucion_total == 0 || costo_total == 0) ? 0 : ((gdouble)contribucion_total/(gdouble)costo_total)*100;
  gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_rank_margin")),
                        g_strdup_printf ("<span size=\"x-large\">%s %%</span>",
					 g_strdup_printf ("%.2f", margen )));

  //gtk_timeout_remove (flagProgress);
  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_get_stat")), TRUE);
  GtkWidget *progreso = GTK_WIDGET (builder_get (builder, "progressbar"));
  gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR (progreso), 0);
  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (progreso), ".. Listo ..");
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
  query = g_strdup_printf ("SELECT id, fecha_inicio, inicio, fecha_termino, termino, perdida, "
			   "       (SELECT cash_close_outcome FROM cash_box_report (id)) AS monto_pre_cierre "
			   "FROM caja "
			   "WHERE fecha_inicio>=to_timestamp ('%.2d %.2d %.4d', 'DD MM YYYY') "
			   "AND (fecha_termino<=to_timestamp ('%.2d %.2d %.4d', 'DD MM YYYY') OR fecha_termino IS NULL) "
			   "ORDER BY id"
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
                          4, atoi (PQvaluebycol (res, i, "monto_pre_cierre")),
			  5, atoi (PQvaluebycol (res, i, "termino")),
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

  gtk_list_store_clear (store);

  res = EjecutarSQL ("SELECT nombre, "
                     "       (SELECT SUM (cantidad_ingresada) FROM compra_detalle WHERE id_compra IN (SELECT id FROM compra WHERE rut_proveedor=proveedor.rut)) as unidades,"
                     "       round((SELECT SUM (cantidad_ingresada * precio) FROM compra_detalle WHERE id_compra IN (SELECT id FROM compra WHERE rut_proveedor=proveedor.rut))) as comprado,"
                     "       round((SELECT (SUM (margen) / COUNT (*)::numeric) FROM compra_detalle WHERE id_compra IN (SELECT id FROM compra WHERE rut_proveedor=proveedor.rut))::numeric,3) as margen,"
                     "       round((SELECT SUM (precio_venta - (precio * (margen / 100) +1))  FROM compra_detalle WHERE id_compra IN (SELECT id FROM compra WHERE rut_proveedor=proveedor.rut))) as contribucion "
                     "FROM proveedor");

  tuples = PQntuples (res);

  if (res == NULL) return;

  for (i = 0; i < tuples; i++)
    {

      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
                          0, PQvaluebycol (res, i, "nombre"),
			  1, strtod(PUT(PQvaluebycol (res, i, "unidades")),(char **)NULL),
			  2, atoi(PQvaluebycol (res, i, "comprado")),
                          3, strtod(PUT(PQvaluebycol (res, i, "margen")),(char **)NULL),
                          4, atoi(PQvaluebycol (res, i, "contribucion")),
			  -1);

    }
}


/**
 * Es llamado por "on_btn_get_stat_clicked()"
 *
 * Obtiene la información de cuadratura y la depliega en en treeview "tree_view_cuadratura"
 */
void
fill_cuadratura (gint familia)
{
  GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (builder_get (builder, "tree_view_cuadratura"))));
  GtkTreeIter iter;
  gint i, tuples;
  PGresult *res;
  char *sql, *filtro_familia;

  gtk_list_store_clear (store);

  if (familia == 0)
    filtro_familia = g_strdup ("");
  else
    filtro_familia = g_strdup_printf("WHERE familia = %d", familia);

  sql = g_strdup_printf ( "SELECT codigo_corto, (descripcion||' '||cont_un) AS descripcion, marca, familia, stock_inicial, compras_periodo, anulaciones_c_periodo, ventas_periodo, anulaciones_periodo, insumidos_periodo, devoluciones_periodo, mermas_periodo, enviados_periodo, recibidos_periodo, stock_teorico "
			  "FROM producto_en_periodo('%.4d-%.2d-%.2d') %s",
			  g_date_get_year (date_begin), g_date_get_month (date_begin), g_date_get_day (date_begin),
			  filtro_familia);

  res = EjecutarSQL (sql);
  tuples = PQntuples (res);
  g_free (sql);

  if (res == NULL) return;

  for (i = 0; i < tuples; i++)
    {
      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
			  0, PQvaluebycol (res, i, "codigo_corto"),
                          1, PQvaluebycol (res, i, "descripcion"),
			  2, PQvaluebycol (res, i, "marca"),
			  3, strtod(PUT(PQvaluebycol (res, i, "stock_inicial")),(char **)NULL),
			  4, strtod(PUT(PQvaluebycol (res, i, "compras_periodo")),(char **)NULL),
			  5, strtod(PUT(PQvaluebycol (res, i, "anulaciones_c_periodo")),(char **)NULL),
                          6, strtod(PUT(PQvaluebycol (res, i, "ventas_periodo")),(char **)NULL),
			  7, strtod(PUT(PQvaluebycol (res, i, "anulaciones_periodo")),(char **)NULL),
			  8, strtod(PUT(PQvaluebycol (res, i, "insumidos_periodo")),(char **)NULL),
                          9, strtod(PUT(PQvaluebycol (res, i, "devoluciones_periodo")),(char **)NULL),
			  10, strtod(PUT(PQvaluebycol (res, i, "mermas_periodo")),(char **)NULL),
			  11, strtod(PUT(PQvaluebycol (res, i, "enviados_periodo")),(char **)NULL),
			  12, strtod(PUT(PQvaluebycol (res, i, "recibidos_periodo")),(char **)NULL),
			  13, strtod(PUT(PQvaluebycol (res, i, "stock_teorico")),(char **)NULL),
			  14, strtod(PUT(PQvaluebycol (res, i, "stock_teorico")),(char **)NULL),
			  //15, 0,
			  -1);
    }
}


/**
 * Es llamado por "on_btn_get_stat_clicked()"
 *
 * Obtiene la información del movimiento de las mercaderías
 */
void
fill_movimiento (GtkWidget *widget, gpointer user_data)
{
  GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (builder_get (builder, "treeview_movimiento"))));
  GtkTreeIter iter;
  gint i, tuples;
  PGresult *res;
  gchar *barcode;
  gboolean todos;

  todos = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (builder_get (builder, "rdbtn_mov_todos")));

  //Se obtiene el barcode
  if (todos == TRUE)
    barcode = g_strdup("0");
  else
    barcode = g_strdup (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_mov_barcode"))));

  if (barcode == NULL || g_str_equal (barcode, ""))
    {
      ErrorMSG (GTK_WIDGET (builder_get (builder, "entry_mov_barcode")), "Debe especificar un codigo de barras");
      return;
    }

  gtk_list_store_clear (store);

  res = movimiento_en_rango (date_begin, date_end, barcode);
  tuples = PQntuples (res);

  if (res == NULL) return;

  for (i = 0; i < tuples; i++)
    {
      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
			  0, PQvaluebycol (res, i, "fecha"),
                          1, strtod (PUT(PQvaluebycol (res, i, "stock")), (char **)NULL),
			  2, atoi (PQvaluebycol (res, i, "valor_stock")),
			  3, strtod (PUT(PQvaluebycol (res, i, "unidades_vendidas")), (char **)NULL),
			  4, atoi (PQvaluebycol (res, i, "valor_vendido")),
			  -1);
    }
}


/**
 * Es llamado por "on_btn_get_stat_clicked()"
 *
 * Obtiene la información de traspasos y la depliega en en treeview "
 * tree_view_enviados" o "tree_view_recibidos" según corresponda.
 *
 */

void
fill_traspaso (gchar *local)
{
  GtkListStore *store;
  GtkTreeIter iter;
  gint i, tuples;
  PGresult *res;
  gchar *sql;
  gchar *local_consulta;

  // Obtiene la página de traspasos desde la cual se esta llamando a esta funcion
  GtkNotebook *notebook = GTK_NOTEBOOK (builder_get (builder, "ntbk_traspasos"));
  gint page_num = gtk_notebook_get_current_page (notebook);

  if (local == NULL || g_str_equal (local, "TODOS"))
    local_consulta = g_strdup ("");
  else
    local_consulta = g_strdup_printf("AND b.nombre = '%s'", local);

  // Se obtienen y se limpian los stores correspondientes
  store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (builder_get (builder, (page_num == 0) ? "tree_view_enviado": "tree_view_recibido"))));
  gtk_list_store_clear (store);
  gtk_list_store_clear (GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (builder_get (builder, (page_num == 0) ? "tree_view_enviado_detalle": "tree_view_recibido_detalle")))));

  if(page_num == 0)
    sql = g_strdup_printf ( "SELECT fecha, t.id, b.nombre AS destino, monto "
			    "FROM traspaso t "
			    "INNER JOIN bodega b "
			    "ON t.destino = b.id "
			    "WHERE destino != 1 "
			    "%s "
			    "AND fecha BETWEEN '%.4d-%.2d-%.2d' AND '%.4d-%.2d-%.2d 23:59:59' "
			    "ORDER BY fecha ASC"
			    , local_consulta
			    , g_date_get_year (date_begin), g_date_get_month (date_begin), g_date_get_day (date_begin)
			    , g_date_get_year (date_end), g_date_get_month (date_end), g_date_get_day (date_end));
  else
    sql = g_strdup_printf ( "SELECT fecha, t.id, b.nombre AS origen, monto "
			    "FROM traspaso t "
			    "INNER JOIN bodega b "
			    "ON t.origen = b.id "
			    "WHERE origen != 1 "
			    "%s "
			    "AND fecha BETWEEN '%.4d-%.2d-%.2d' AND '%.4d-%.2d-%.2d 23:59:59' "
			    "ORDER BY fecha ASC"
			    , local_consulta
			    , g_date_get_year (date_begin), g_date_get_month (date_begin), g_date_get_day (date_begin)
			    , g_date_get_year (date_end), g_date_get_month (date_end), g_date_get_day (date_end));

  res = EjecutarSQL (sql);
  tuples = PQntuples (res);
  g_free (sql);

  if (res == NULL) return;

  for (i = 0; i < tuples; i++)
    {
      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
			  0, PQvaluebycol (res, i, "fecha"),
                          1, PQvaluebycol (res, i, "id"),
			  2, PQvaluebycol (res, i, (page_num == 0) ? "destino" : "origen"),
			  3, PutPoints (PQvaluebycol (res, i, "monto")),
			  -1);
    }
}


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

  q = g_strdup_printf ("SELECT rut, dv, nombre "
                       "FROM buscar_proveedor ('%%%s%%')", str_schr);
  g_free (str_schr);

  res = EjecutarSQL (q);
  g_free (q);

  tuples = PQntuples (res);

  store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_srch_provider"))));

  gtk_list_store_clear (store);

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
}


/**
 * Is called by "fill_purchases_list".
 *
 * this function shows the "wnd_srch_provider"
 * to list the available providers
 *
 * @param: GtkButton *button: button from signal is triggered
 * @param: gpointer user_data: user data
 */
void
wnd_search_provider (GtkEntry *entry, gpointer user_data)
{
  GtkWindow *window;
  GtkTreeView *tree = GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_srch_provider"));
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
  on_entry_srch_provider_activate(entry);

  gtk_widget_show_all (GTK_WIDGET (window));
}


/**
 * Is called by "btn_buy_search" (signal click).
 *
 * this function shows the buy that meet the search criteria
 * on treeview "treeview_buy"
 *
 * @param: GtkButton *button: button from signal is triggered
 * @param: gpointer user_data: user data
 */
void
fill_purchases_list (GtkWidget *widget, gpointer user_data)
{
  /*Treeview*/
  GtkTreeView *treeview;
  GtkListStore *store;
  GtkTreeIter iter;

  /*Limpiando Treeview*/
  treeview = GTK_TREE_VIEW (gtk_builder_get_object (builder, "treeview_buy"));
  store = GTK_LIST_STORE (gtk_tree_view_get_model (treeview));
  gtk_list_store_clear (store);

  /*Limpiando Labels*/
  gtk_label_set_text (GTK_LABEL (builder_get (builder, "lbl_buy_provider_name")), "");
  gtk_label_set_text (GTK_LABEL (builder_get (builder, "lbl_buy_provider_rut")), "");

  /*Obteniendo datos de los entry*/
  gchar *id_compra = g_strdup (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_buy_id"))));
  gchar *num_factura = g_strdup (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_buy_invoice"))));
  gchar *proveedor = g_strdup (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_buy_provider"))));

  GtkWidget *combo;
  GtkTreeModel *model_combo;
  GtkTreeIter iter_combo;
  gchar *store_combo;
  gint active;

  combo = GTK_WIDGET (gtk_builder_get_object(builder, "cmb_buy_report"));
  active = gtk_combo_box_get_active (GTK_COMBO_BOX (combo));

  /* Verifica si hay alguna opción seleccionada*/
  if (active == -1)
    store_combo = "TODAS";
  else
    {
      model_combo = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));
      gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combo), &iter_combo);

      gtk_tree_model_get (model_combo, &iter_combo,
                          1, &store_combo,
                          -1);
    }

  printf ("%s\n",store_combo);

  gchar *q, *q2;
  PGresult *res;
  gint tuples, i;

  q = g_strdup_printf ("SELECT c.id, "
		       "date_part ('year', c.fecha) AS year, "
		       "date_part ('month', c.fecha) AS month, "
		       "date_part ('day', c.fecha) AS day, "
		       "date_part ('hour', c.fecha) AS hour, "
		       "date_part ('minute', c.fecha) AS minute, "
		       "c.rut_proveedor, c.anulada_pi, "
		       "(SELECT nombre FROM proveedor WHERE rut = c.rut_proveedor) AS nombre_proveedor, "
		       "(SELECT round(SUM (cantidad * precio)) "
		            "FROM factura_compra_detalle fcd "
		            "INNER JOIN factura_compra fc "
		            "ON fcd.id_factura_compra = fc.id "
		            "AND fc.id_compra = c.id) AS monto_compra, "
		       "(SELECT round(SUM (cantidad_anulada * costo)) "
		            "FROM compra_anulada_detalle cad "
		            "INNER JOIN compra_anulada ca "
		            "ON cad.id_compra_anulada = ca.id "
		            "AND ca.id_compra = c.id) AS monto_anulado "
		       "FROM compra c INNER JOIN factura_compra fc "
		       "ON c.id = fc.id_compra "
		       "WHERE c.ingresada = 't' "
		       "AND c.anulada = 'f'");

  if (g_str_equal(store_combo, "Anuladas"))
    q = g_strdup_printf (" %s AND c.anulada_pi = 't'", q);
  else if (g_str_equal(store_combo, "Vigentes"))
    q = g_strdup_printf (" %s AND c.anulada_pi = 'f'", q);

  /*Comprobando datos*/
  if (!g_str_equal(id_compra, ""))
    q = g_strdup_printf (" %s AND c.id = %s", q, id_compra);

  if (!g_str_equal(num_factura, ""))
    q = g_strdup_printf (" %s AND fc.num_factura = %s", q, num_factura);

  if (!g_str_equal(proveedor, ""))
    {
      q2 = g_strdup_printf ("SELECT rut, dv, nombre "
			    "FROM buscar_proveedor ('%%%s%%')", proveedor);
      res = EjecutarSQL (q2);
      g_free (q2);
      tuples = PQntuples (res);

      if (tuples > 1)
	{
	  wnd_search_provider (GTK_ENTRY (builder_get (builder, "entry_buy_provider")), NULL);
	  return;
	}

      else if (tuples == 1)
	q = g_strdup_printf (" %s AND fc.rut_proveedor = %s", q, PQvaluebycol(res,0,"rut"));
      else if (tuples == 0)
	AlertMSG (widget, "No existen proveedores con el nombre o rut que ha especificado");
    }

  q = g_strdup_printf (" %s AND c.fecha BETWEEN '%.4d-%.2d-%.2d' AND '%.4d-%.2d-%.2d 23:59:59'", q,
		       g_date_get_year (date_begin), g_date_get_month (date_begin), g_date_get_day (date_begin),
		       g_date_get_year (date_end), g_date_get_month (date_end), g_date_get_day (date_end));

  q = g_strdup_printf (" %s GROUP BY c.id, c.fecha, c.rut_proveedor, c.anulada_pi", q);

  // Se ejecuta la consulta construida
  res = EjecutarSQL (q);
  g_free (q);
  tuples = PQntuples (res);

  if ( (tuples == 0 && g_str_equal(proveedor, "")) &&
       (g_str_equal(id_compra, "") && g_str_equal(num_factura, "")) )
    AlertMSG (widget, "No se encontraron resultados en el intervalo fechas seleccionadas");
  else if (tuples == 0)
    AlertMSG (widget, "No se encontraron resultados que cumplan con los criterios especificados");

  for (i = 0; i < tuples; i++)
    {
      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
			  0, atoi (g_strdup (PQvaluebycol (res, i, "id"))),
			  1, g_strdup_printf ("%s/%s/%s %s:%s",
					      PQvaluebycol (res, i, "day"),
					      PQvaluebycol (res, i, "month"),
					      PQvaluebycol (res, i, "year"),
					      PQvaluebycol (res, i, "hour"),
					      PQvaluebycol (res, i, "minute")),
			  2, g_strdup_printf ("%s", (g_str_equal (PQvaluebycol (res, i, "anulada_pi"), "t")) ?
					      "Anulada" : "Vigente"),
			  3, atoi (PQvaluebycol (res, i, "monto_compra")),
			  4, atoi ((g_str_equal (PQvaluebycol (res, i, "monto_anulado"),"")) ?
				   "0" : PQvaluebycol (res, i, "monto_anulado")),
			  5, PQvaluebycol (res, i, "nombre_proveedor"),
			  6, PQvaluebycol (res, i, "rut_proveedor"),
			  -1);
    }

  PQclear (res);
}

/**
 * This function avanza en base al tiempo el progreso de una barra de progreso
 *
 * @param data the user data (en este caso es un GTK_PROGRESS)
 **/
gint
progress_timeout(gpointer data)
{
    gfloat new_val;
    GtkAdjustment *adj;

    /* Calculate the value of the progress bar using the
     * value range set in the adjustment object */

    new_val = gtk_progress_get_value( GTK_PROGRESS(data) ) + 1;

    adj = GTK_PROGRESS (data)->adjustment;
    if (new_val > adj->upper)
      new_val = adj->lower;

    /* Set the new value */
    gtk_progress_set_value (GTK_PROGRESS (data), new_val);

    /* As this is a timeout function, return TRUE so that it
     * continues to get called */
    return(TRUE);
}

/**
 * This function configura un gtk_progress_bar para poder avanzarlo
 *
 * @param progreso widget de gtk_progress_bar
 */
//gint
void
avanzar_barra_progreso (GtkWidget * progreso)
{
  GtkAdjustment *adj;
  adj = (GtkAdjustment *) gtk_adjustment_new (0, 1, 150, 0, 0, 0);
  gtk_progress_bar_new_with_adjustment (adj);
  //flagProgress = gtk_timeout_add (100, progress_timeout, progreso);
}


/**
 * Is called by "on_ntbk_reports_switch_page" and "on_btn_get_stat_clicked()"
 *
 * This function show transfer information (recibed and sent)
 * on "ntbk_traspasos".
 *
 */
void
calcular_traspasos (void)
{
  PGresult *res;
  char *sql;

  if (g_date_get_year (date_begin) == G_DATE_BAD_YEAR || g_date_get_year (date_end) == G_DATE_BAD_YEAR)
    // Total Enviados
    sql = g_strdup_printf ( "SELECT SUM(monto) AS enviados "
			    "FROM traspaso "
			    "WHERE origen = 1" );
  else
    sql = g_strdup_printf ( "SELECT SUM(monto) AS enviados "
			    "FROM traspaso "
			    "WHERE origen = 1 "
			    "AND fecha BETWEEN '%.4d-%.2d-%.2d' AND '%.4d-%.2d-%.2d 23:59:59' "
			    , g_date_get_year (date_begin), g_date_get_month (date_begin), g_date_get_day (date_begin)
			    , g_date_get_year (date_end), g_date_get_month (date_end), g_date_get_day (date_end));

  res = EjecutarSQL (sql);
  g_free (sql);

  gchar *enviado, *recibido;

  if (res == NULL)
    return;

  enviado = PQvaluebycol (res, 0, "enviados");
  enviado = g_strdup_printf (g_str_equal ("", enviado) ? " No hay productos enviados" :
			                                 "<span size=\"12000\" weight=\"bold\"> %s </span>", PutPoints(enviado));

  gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_total_enviado")), enviado);


  if (g_date_get_year (date_begin) == G_DATE_BAD_YEAR || g_date_get_year (date_end) == G_DATE_BAD_YEAR)
    // Total Recibidos
    sql = g_strdup_printf ( "SELECT SUM(monto) AS recibidos "
			    "FROM traspaso "
			    "WHERE origen != 1" );
  else
    sql = g_strdup_printf ( "SELECT SUM(monto) AS recibidos "
			    "FROM traspaso "
			    "WHERE origen != 1 "
			    "AND fecha BETWEEN '%.4d-%.2d-%.2d' AND '%.4d-%.2d-%.2d 23:59:59' "
			    , g_date_get_year (date_begin), g_date_get_month (date_begin), g_date_get_day (date_begin)
			    , g_date_get_year (date_end), g_date_get_month (date_end), g_date_get_day (date_end));

  res = EjecutarSQL (sql);
  g_free (sql);

  if (res == NULL)
    return;

  recibido = PQvaluebycol (res, 0, "recibidos");

  gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_total_recibido")),
                        g_strdup_printf (g_str_equal ("",recibido) ? " No hay productos recibidos" :
					                             "<span size=\"12000\" weight=\"bold\"> %s </span>", PutPoints (recibido)));
}

/**
 * Es llamada cuando se presiona el boton "btn_get_stat" (signal clicked)
 *
 * Esta funcion extrae las fechas ingresadas, y segun notebook se escoge una opcion
 *
 */
void
on_btn_get_stat_clicked (GtkWidget *widget, gpointer user_data)
{
  GtkComboBox *combo;
  GtkTreeModel *modelo;
  GtkTreeIter iter;

  GtkNotebook *notebook = GTK_NOTEBOOK (builder_get (builder, "ntbk_reports"));
  gint page_num = gtk_notebook_get_current_page (notebook);
  GtkNotebook *sub_notebook;
  gint sub_page_num;


  //gint hrIni, hrFin, minIni, minFin;
  hrIni  = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (builder_get (builder, "spnBtnHourIni")));
  minIni = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (builder_get (builder, "spnBtnMinIni" )));
  hrFin  = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (builder_get (builder, "spnBtnHourFin")));
  minFin = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (builder_get (builder, "spnBtnMinFin" )));

  //printf("%d:%d  -  %d:%d\n", hrIni, minIni, hrFin, minFin);

  
  if (page_num == 5)
    {
      /* Informe de proveedores */
      fill_provider ();
    }
  else
    {
      const gchar *str_begin = gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_date_begin")));
      const gchar *str_end = gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_date_end")));
      /* pthread_t idPthread; */
      /* GtkWidget *progreso = GTK_WIDGET (builder_get (builder, "progressbar")); */

      date_begin = g_date_new ();
      date_end = g_date_new ();

      if (g_str_equal (str_begin, "") || g_str_equal (str_end, "")) return;
      g_date_set_parse (date_begin, str_begin);
      g_date_set_parse (date_end, str_end);

      // Todos los demas informes
      switch (page_num)
        {
        case 0:
	  /* idPthread = 0; */
	  /* gtk_progress_bar_set_text (GTK_PROGRESS_BAR(progreso), ".. Cargando .."); */
	  /* flagProgress = avanzar_barra_progreso(progreso); */

          /* Informe de ventas */
          fill_sells_list();
          clean_container (GTK_CONTAINER
			   (gtk_widget_get_parent (GTK_WIDGET (builder_get (builder, "lbl_sell_cash_amount")))));
	  /* llama a la funcion fill_totals() en un nuevo thread(hilo)*/
	  /* pthread_create(&idPthread, NULL, fill_totals, NULL); */
	  /* pthread_detach(idPthread); */
	  fill_totals();
          break;

        case 1:
	  sub_notebook = GTK_NOTEBOOK (builder_get (builder, "ntbk_sells_rank"));
	  sub_page_num = gtk_notebook_get_current_page (sub_notebook);
	  combo = GTK_COMBO_BOX (builder_get (builder, "cmb_family_filter"));
	  modelo = GTK_TREE_MODEL (gtk_combo_box_get_model(combo));

	  gint familia;
	  gtk_combo_box_get_active_iter (combo, &iter);

	  gtk_tree_model_get (modelo, &iter,
			      0, &familia,
			      -1);

	  if (sub_page_num == 0)
	    {
	      /* idPthread = 1; */
	      /* gtk_progress_bar_set_text (GTK_PROGRESS_BAR(progreso), ".. Cargando .."); */
	      /* flagProgress = avanzar_barra_progreso(progreso); */

	      /* Informe de ranking de ventas */
	      /* llama a la funcion fill_products_rank() en un nuevo thread(hilo)*/
	      /* pthread_create(&idPthread, NULL, fill_products_rank, NULL); */
	      /* pthread_detach(idPthread); */
	      fill_products_rank(familia);
	    }
	  else if (sub_page_num == 1)
	    {
	      /* idPthread = 2; */
	      /* gtk_progress_bar_set_text (GTK_PROGRESS_BAR(progreso), ".. Cargando .."); */
	      /* flagProgress = avanzar_barra_progreso(progreso); */

	      /* Informe de ranking de ventas */
	      /* llama a la funcion fill_products_rank() en un nuevo thread(hilo)*/
	      /* pthread_create(&idPthread, NULL, fill_products_rank_mp, NULL); */
	      /* pthread_detach(idPthread); */
	      fill_products_rank_mp(familia);
	    }
	  else if (sub_page_num == 2)
	    {
	      fill_products_rank_mc(familia);
	    }
          break;

	case 2:
	  fill_purchases_list (widget, NULL);
	  break;

        case 3:
          /* Informe de caja*/
          fill_cash_box_list ();
          break;

        case 4:
	  /* idPthread = 4; */
	  /* gtk_progress_bar_set_text (GTK_PROGRESS_BAR(progreso), ".. Cargando .."); */
	  /* flagProgress = avanzar_barra_progreso(progreso); */

          /* Informe de devolucion */
          fill_devolucion ();
          /* llama a la funcion fill_totals_dev() en un nuevo thread(hilo)*/
	  /* pthread_create(&idPthread, NULL, fill_totals_dev, NULL); */
	  /* pthread_detach(idPthread); */
	  fill_totals_dev();
          break;

	case 6:
	  /*Informe Cuadratura*/
	  fill_cuadratura (0);
	  break;

	case 7:
	  /*Informe Traspasos*/
	  fill_traspaso (NULL);
	  calcular_traspasos ();
	  break;

	case 8:
	  sub_notebook = GTK_NOTEBOOK (builder_get (builder, "ntbk_transfers_rank"));
	  sub_page_num = gtk_notebook_get_current_page (sub_notebook);
	  combo = GTK_COMBO_BOX (builder_get (builder, "cmb_transfer_type"));
	  modelo = GTK_TREE_MODEL (gtk_combo_box_get_model(combo));

	  gboolean traspaso_envio;
	  gtk_combo_box_get_active_iter (combo, &iter);

	  gtk_tree_model_get (modelo, &iter,
			      0, &traspaso_envio,
			      -1);

	  if (sub_page_num == 0)
	    {
	      fill_transfer_rank(traspaso_envio);
	    }
	  else if (sub_page_num == 1)
	    {
	      fill_transfer_rank_mp(traspaso_envio);
	    }
	  else if (sub_page_num == 2)
	    {
	      fill_transfer_rank_mc(traspaso_envio);
	    }
	  break;

	case 9:
	  /*Informe Exentos*/
	  fill_exempt_sells ();
	  break;

	case 10:
	  /*Informe Exentos*/
	  fill_inmovilizados (NULL, NULL);
	  break;

	case 11:
	  /*Informe Exentos*/
	  fill_movimiento (NULL, NULL);
	  break;

        default:
          break;
        }
    }
}


/**
 * This function add the selected provider on
 * 'entry_buy_provider' to search purchases associated to him
 *
 * @param: GtkTreeView from where to get provider's rut
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

      gtk_entry_set_text (GTK_ENTRY (builder_get (builder, "entry_buy_provider")), *strs);
      on_btn_get_stat_clicked (GTK_WIDGET (builder_get (builder, "entry_buy_provider")), NULL);

      gtk_widget_hide (GTK_WIDGET (builder_get (builder, "wnd_srch_provider")));
    }
}


/**
 * Is triggered by "switch-page" event on "ntbk_reports"
 *
 * This function aims update the pages information when
 * to switch between them.
 *
 */
void
on_ntbk_reports_switch_page (GtkNotebook *notebook, GtkNotebookPage *page, guint page_num, gpointer user_data)
{
  GtkWidget *combo;
  GtkTreeIter iter;
  GtkListStore *modelo;
  gint tuples,i,sub_page_num;
  PGresult *res;
  GtkNotebook *sub_notebook;


  /*Set filter hour visibility*/
  if (page_num == 0) {
    if (rizoma_get_value_boolean ("INFORME_FILTRO_HORA"))
      gtk_widget_show (GTK_WIDGET(builder_get (builder, "hboxHourFilter")));
    else
      gtk_widget_hide (GTK_WIDGET(builder_get (builder, "hboxHourFilter")));
  } else {
     gtk_widget_hide (GTK_WIDGET(builder_get (builder, "hboxHourFilter")));
  }
  
  

  //NOTA: Cada página se preocupa SOLO de sus PROPIOS widgets (mostrarlos, rellenarlos y ocultarlos)
  /*Paginas 1 y 6*/
  if (page_num == 6 || page_num == 1)
    {
      gtk_widget_show (GTK_WIDGET (builder_get (builder, "cmb_family_filter")));
      gtk_widget_show (GTK_WIDGET (builder_get (builder, "btn_apply_family_filter")));

      res = EjecutarSQL (g_strdup_printf ("SELECT id, nombre FROM familias"));
      tuples = PQntuples (res);

      combo = GTK_WIDGET (gtk_builder_get_object(builder, "cmb_family_filter"));
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

      gtk_list_store_append(modelo, &iter);
      gtk_list_store_set(modelo, &iter,
			 0, 0,
			 1, "TODOS",
			 -1);

      for (i=0 ; i < tuples ; i++)
	{
	  gtk_list_store_append(modelo, &iter);
	  gtk_list_store_set(modelo, &iter,
			     0, atoi(PQvaluebycol(res, i, "id")),
			     1, PQvaluebycol(res, i, "nombre"),
			     -1);
	}

      gtk_combo_box_set_active (GTK_COMBO_BOX (gtk_builder_get_object (builder, "cmb_family_filter")), 0);

      // Botón imprimir en las subpestañas del ranking de ventas
      if (page_num == 1)
	{
	  sub_notebook = GTK_NOTEBOOK (builder_get (builder, "ntbk_sells_rank"));
	  sub_page_num = gtk_notebook_get_current_page (sub_notebook);

	  if (sub_page_num == 0) //Sell rank
	    {
	      gtk_widget_hide (GTK_WIDGET (builder_get (builder, "btn_print_rank_mc")));
	      gtk_widget_hide (GTK_WIDGET (builder_get (builder, "btn_print_rank_mp")));
	      gtk_widget_show (GTK_WIDGET (builder_get (builder, "btn_print_rank")));
	    }
	  else if (sub_page_num == 1) //Sell rank MP
	    {
	      gtk_widget_hide (GTK_WIDGET (builder_get (builder, "btn_print_rank_mc")));
	      gtk_widget_hide (GTK_WIDGET (builder_get (builder, "btn_print_rank")));
	      gtk_widget_show (GTK_WIDGET (builder_get (builder, "btn_print_rank_mp")));
	    }
	  else if (sub_page_num == 2) //Sell rank MC
	    {
	      gtk_widget_hide (GTK_WIDGET (builder_get (builder, "btn_print_rank")));
	      gtk_widget_hide (GTK_WIDGET (builder_get (builder, "btn_print_rank_mp")));
	      gtk_widget_show (GTK_WIDGET (builder_get (builder, "btn_print_rank_mc")));
	    }
	}
    }
  else // Si (page_num != 6 || page_num != 1) Se ocultan los widget para filtrar familias
    {
      gtk_widget_hide (GTK_WIDGET (builder_get (builder, "cmb_family_filter")));
      gtk_widget_hide (GTK_WIDGET (builder_get (builder, "btn_apply_family_filter")));
    }

  /*Si se selecciona la "pagina 6" (la pestaña cuadratura) y el entry de la fecha de termino esta habilitado*/
  if ((page_num == 6 || page_num == 10) &&
     gtk_widget_get_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "entry_date_end"))) == TRUE)
    {
      gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "entry_date_end")), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "button3")), FALSE);
    }
  else if ((page_num != 6 && page_num != 10) &&
     gtk_widget_get_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "entry_date_end"))) == FALSE)
    {
      gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "entry_date_end")), TRUE);
      gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "button3")), TRUE);
    }

  /*Pagina 7. Traspasos*/
  if (page_num == 7)
    {
      // Se muestran los widget para filtrar las tiendas
      gtk_widget_show (GTK_WIDGET (builder_get (builder, "cmb_stores")));
      gtk_widget_show (GTK_WIDGET (builder_get (builder, "btn_filter_stores")));

      // Se calculan los traspasos y se muestran
      //fill_traspaso (NULL);
      //calcular_traspasos();

      // Se inicia mostrando el botón para imprimir el informe de los envios
      // y oculta el de los recibidos
      gtk_widget_show (GTK_WIDGET (builder_get (builder, "btn_print_enviado")));
      gtk_widget_hide (GTK_WIDGET (builder_get (builder, "btn_print_recibido")));

      GtkWidget *combo;
      GtkTreeIter iter;
      GtkListStore *modelo;
      gint tuples,i;
      PGresult *res;

      res = EjecutarSQL (g_strdup_printf ("SELECT id, nombre FROM bodega "
					  "WHERE nombre NOT IN (SELECT nombre FROM negocio) AND estado=true"));
      tuples = PQntuples (res);

      combo = GTK_WIDGET (gtk_builder_get_object(builder, "cmb_stores"));
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

      gtk_list_store_append(modelo, &iter);
      gtk_list_store_set(modelo, &iter,
			 0, 0,
			 1, "TODOS",
			 -1);

      for (i=0 ; i < tuples ; i++)
	{
	  gtk_list_store_append(modelo, &iter);
	  gtk_list_store_set(modelo, &iter,
			     0, atoi(PQvaluebycol(res, i, "id")),
			     1, PQvaluebycol(res, i, "nombre"),
			     -1);
	}

      gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 0);
    }
  else // SI (page_num != 7) Se ocultan los widget para filtrar las tiendas
    {
      gtk_widget_hide (GTK_WIDGET (builder_get (builder, "cmb_stores")));
      gtk_widget_hide (GTK_WIDGET (builder_get (builder, "btn_filter_stores")));
    }

  /*Página 8. Ranking traspaso*/
  if (page_num == 8)
    {
      gtk_widget_show (GTK_WIDGET (builder_get (builder, "cmb_transfer_type")));
      gtk_widget_show (GTK_WIDGET (builder_get (builder, "btn_choose_transfer_type")));

      // Botón imprimir en las subpestañas del ranking de ventas
      sub_notebook = GTK_NOTEBOOK (builder_get (builder, "ntbk_transfers_rank"));
      sub_page_num = gtk_notebook_get_current_page (sub_notebook);

      if (sub_page_num == 0) //Sell rank
	{
	  gtk_widget_hide (GTK_WIDGET (builder_get (builder, "btn_print_transfer_rank_mc")));
	  gtk_widget_hide (GTK_WIDGET (builder_get (builder, "btn_print_transfer_rank_mp")));
	  gtk_widget_show (GTK_WIDGET (builder_get (builder, "btn_print_transfer_rank")));
	}
      else if (sub_page_num == 1) //Sell rank MP
	{
	  gtk_widget_hide (GTK_WIDGET (builder_get (builder, "btn_print_transfer_rank")));
	  gtk_widget_hide (GTK_WIDGET (builder_get (builder, "btn_print_transfer_rank_mc")));
	  gtk_widget_show (GTK_WIDGET (builder_get (builder, "btn_print_transfer_rank_mp")));
	}
      else if (sub_page_num == 2) //Sell rank MC
	{
	  gtk_widget_hide (GTK_WIDGET (builder_get (builder, "btn_print_transfer_rank")));
	  gtk_widget_hide (GTK_WIDGET (builder_get (builder, "btn_print_transfer_rank_mp")));
	  gtk_widget_show (GTK_WIDGET (builder_get (builder, "btn_print_transfer_rank_mc")));
	}
    }
  else if (page_num != 8)
    {
      gtk_widget_hide (GTK_WIDGET (builder_get (builder, "cmb_transfer_type")));
      gtk_widget_hide (GTK_WIDGET (builder_get (builder, "btn_choose_transfer_type")));
    }
}

void
on_tree_view_traspasos_grab_focus (GtkNotebook *notebook, GtkNotebookPage *page, guint page_num, gpointer user_data)
{
  /*Si se selecciona la "pagina 6" (la pestaña cuadratura) y el entry de la fecha de termino esta habilitado*/
  if(page_num == 0)
    {
      gtk_widget_show (GTK_WIDGET (builder_get (builder, "btn_print_enviado")));
      gtk_widget_hide (GTK_WIDGET (builder_get (builder, "btn_print_recibido")));
    }
  else if(page_num == 1)
    {
      gtk_widget_hide (GTK_WIDGET (builder_get (builder, "btn_print_enviado")));
      gtk_widget_show (GTK_WIDGET (builder_get (builder, "btn_print_recibido")));
    }
}

void
on_btn_filter_stores_clicked ()
{
  gchar *store;
  GtkTreeIter iter;
  GtkWidget *combo;
  GtkTreeModel *model;
  gint active;

  combo = GTK_WIDGET (gtk_builder_get_object(builder, "cmb_stores"));
  active = gtk_combo_box_get_active (GTK_COMBO_BOX (combo));

  /* Verifica si se selecciono un destino del combobox*/
  if (active == -1)
    {
      ErrorMSG (combo, "Debe seleccionar un local");
      store = NULL;
      return;
    }
  else
    {
      model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));
      gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combo), &iter);

      gtk_tree_model_get (model, &iter,
                          1, &store,
                          -1);
    }
  fill_traspaso (store);

  /*Totales enviados y recibidos*/
  PGresult *res;
  char *sql, *local_consulta;

  if ((store == NULL || g_str_equal (store, "TODOS")) || active == -1)
    local_consulta = g_strdup ("");
  else
    local_consulta = g_strdup_printf ("AND b.nombre = '%s'", store);


  if (g_date_get_year (date_begin) == G_DATE_BAD_YEAR || g_date_get_year (date_end) == G_DATE_BAD_YEAR)
    // Total Enviados
    sql = g_strdup_printf ( "SELECT SUM(monto) AS enviados "
			    "FROM traspaso t "
			    "INNER JOIN bodega b "
			    "ON t.origen = b.id "
			    "%s "
			    "WHERE origen = 1"
			    ,local_consulta);
  else
    sql = g_strdup_printf ( "SELECT SUM(monto) AS enviados "
			    "FROM traspaso t "
			    "INNER JOIN bodega b "
			    "ON t.destino = b.id "
			    "%s "
			    "WHERE origen = 1 "
			    "AND fecha BETWEEN '%.4d-%.2d-%.2d' AND '%.4d-%.2d-%.2d 23:59:59' "
			    , local_consulta
			    , g_date_get_year (date_begin), g_date_get_month (date_begin), g_date_get_day (date_begin)
			    , g_date_get_year (date_end), g_date_get_month (date_end), g_date_get_day (date_end));

  res = EjecutarSQL (sql);
  g_free (sql);

  gchar *enviado, *recibido;

  if (res == NULL)
    return;

  enviado = PQvaluebycol (res, 0, "enviados");
  enviado = g_strdup_printf ((g_str_equal("", enviado)) ? " No hay productos enviados" : "%s", enviado);

  gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_total_enviado")), enviado);


  if (g_date_get_year (date_begin) == G_DATE_BAD_YEAR || g_date_get_year (date_end) == G_DATE_BAD_YEAR)
    // Total Recibidos
    sql = g_strdup_printf ( "SELECT SUM(monto) AS recibidos "
			    "FROM traspaso t "
			    "INNER JOIN bodega b "
			    "ON t.origen = b.id "
			    "%s "
			    "WHERE origen != 1"
			    , local_consulta);
  else
    sql = g_strdup_printf ( "SELECT SUM(monto) AS recibidos "
			    "FROM traspaso t "
			    "INNER JOIN bodega b "
			    "ON t.origen = b.id "
			    "%s "
			    "WHERE origen != 1 "
			    "AND fecha BETWEEN '%.4d-%.2d-%.2d' AND '%.4d-%.2d-%.2d 23:59:59' "
			    , local_consulta
			    , g_date_get_year (date_begin), g_date_get_month (date_begin), g_date_get_day (date_begin)
			    , g_date_get_year (date_end), g_date_get_month (date_end), g_date_get_day (date_end));

  res = EjecutarSQL (sql);
  g_free (sql);

  if (res == NULL)
    return;

  recibido = PQvaluebycol (res, 0, "recibidos");

  gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_total_recibido")),
                        g_strdup_printf ((g_str_equal("",recibido)) ? " No hay productos recibidos" : "%s", recibido));

}


void
on_btn_apply_family_filter_clicked ()
{
  gchar *store;
  gint familia;
  GtkTreeIter iter;
  GtkWidget *combo;
  GtkTreeModel *model;
  gint active;

  combo = GTK_WIDGET (gtk_builder_get_object(builder, "cmb_family_filter"));
  active = gtk_combo_box_get_active (GTK_COMBO_BOX (combo));

  /* Verifica si se selecciono un destino del combobox*/
  if (active == -1)
    {
      ErrorMSG (combo, "Debe seleccionar una familia");
      return;
    }
  else
    {
      model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));
      gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combo), &iter);

      gtk_tree_model_get (model, &iter,
			  0, &familia,
                          1, &store,
                          -1);
    }

  printf ("%s: %d\n", store, familia);

  // Obtiene la página del notebook de informe que está activa (para saber que informe se desa filtrar)
  GtkNotebook *notebook, *sub_notebook;
  gint page_num, sub_page_num;

  //Treeview de informes
  notebook = GTK_NOTEBOOK (builder_get (builder, "ntbk_reports"));
  page_num = gtk_notebook_get_current_page (notebook);
  //Treeview de ranking
  sub_notebook = GTK_NOTEBOOK (builder_get (builder, "ntbk_sells_rank"));
  sub_page_num = gtk_notebook_get_current_page (sub_notebook);

  /* Se filtra el informe de la página que se encuentra activa */
  if (page_num == 1) //Ranking de ventas
    {
      if (sub_page_num == 0) //ranking ventas general
	(g_str_equal (store, "TODOS")) ? fill_products_rank (0) : fill_products_rank (familia);
      else if (sub_page_num == 1) //ranking ventas materias primas
	(g_str_equal (store, "TODOS")) ? fill_products_rank_mp (0) : fill_products_rank_mp (familia);
    }
  else if (page_num == 6) //Cuadratura
    (g_str_equal (store, "TODOS")) ? fill_cuadratura (0) : fill_cuadratura (familia);
}


/**
 * Is triggered by "switch-page" event on "ntbk_sell_rank"
 *
 * This function aims update the pages information when
 * to switch between them.
 */
void
on_ntbk_sells_rank_switch_page (GtkNotebook *notebook, GtkNotebookPage *page, guint page_num, gpointer user_data)
{
  if (page_num == 0) //Sell rank
    {
      gtk_widget_hide (GTK_WIDGET (builder_get (builder, "btn_print_rank_mc")));
      gtk_widget_hide (GTK_WIDGET (builder_get (builder, "btn_print_rank_mp")));
      gtk_widget_show (GTK_WIDGET (builder_get (builder, "btn_print_rank")));
    }
  else if (page_num == 1) //Sell rank MP
    {
      gtk_widget_hide (GTK_WIDGET (builder_get (builder, "btn_print_rank_mc")));
      gtk_widget_hide (GTK_WIDGET (builder_get (builder, "btn_print_rank")));
      gtk_widget_show (GTK_WIDGET (builder_get (builder, "btn_print_rank_mp")));
    }
  else if (page_num == 2) //Sell rank MC
    {
      gtk_widget_hide (GTK_WIDGET (builder_get (builder, "btn_print_rank")));
      gtk_widget_hide (GTK_WIDGET (builder_get (builder, "btn_print_rank_mp")));
      gtk_widget_show (GTK_WIDGET (builder_get (builder, "btn_print_rank_mc")));
    }
}


/**
 * Is triggered by "switch-page" event on "ntbk_transfers_rank"
 *
 * This function aims update the pages information when
 * to switch between them.
 */
void
on_ntbk_transfers_rank_switch_page (GtkNotebook *notebook, GtkNotebookPage *page, guint page_num, gpointer user_data)
{
  GtkComboBox *combo;
  GtkTreeModel *modelo;
  GtkTreeIter iter;

  combo = GTK_COMBO_BOX (builder_get (builder, "cmb_transfer_type"));
  modelo = GTK_TREE_MODEL (gtk_combo_box_get_model(combo));

  gboolean traspaso_envio;
  gtk_combo_box_get_active_iter (combo, &iter);

  gtk_tree_model_get (modelo, &iter,
		      0, &traspaso_envio,
		      -1);

  if (page_num == 0) //Sell rank
    {
      fill_transfer_rank(traspaso_envio);

      gtk_widget_hide (GTK_WIDGET (builder_get (builder, "btn_print_transfer_rank_mc")));
      gtk_widget_hide (GTK_WIDGET (builder_get (builder, "btn_print_transfer_rank_mp")));
      gtk_widget_show (GTK_WIDGET (builder_get (builder, "btn_print_transfer_rank")));
    }
  else if (page_num == 1) //Sell rank MP
    {
      fill_transfer_rank_mp(traspaso_envio);

      gtk_widget_hide (GTK_WIDGET (builder_get (builder, "btn_print_transfer_rank")));
      gtk_widget_hide (GTK_WIDGET (builder_get (builder, "btn_print_transfer_rank_mc")));
      gtk_widget_show (GTK_WIDGET (builder_get (builder, "btn_print_transfer_rank_mp")));
    }
  else if (page_num == 2) //Sell rank MC
    {
      fill_transfer_rank_mc(traspaso_envio);

      gtk_widget_hide (GTK_WIDGET (builder_get (builder, "btn_print_transfer_rank")));
      gtk_widget_hide (GTK_WIDGET (builder_get (builder, "btn_print_transfer_rank_mp")));
      gtk_widget_show (GTK_WIDGET (builder_get (builder, "btn_print_transfer_rank_mc")));
    }
}
