/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4;
   c-indentation-style: gnu -*- */
/* compras.c
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

#include<gtk/gtk.h>
#include<gdk/gdkkeysyms.h>

#include<stdlib.h>
#include<string.h>
#include<ctype.h>
#include<math.h>
#include"tipos.h"

#include"compras.h"

#include"ventas.h"
#include"credito.h"
#include"administracion_productos.h"
#include"postgres-functions.h"
#include"manejo_productos.h"
#include"proveedores.h"
#include"errors.h"
#include"caja.h"

#include"printing.h"
#include"utils.h"
#include"config_file.h"
#include"encriptar.h"
#include"rizoma_errors.h"
#include"vale.h"


GtkBuilder *builder;

GtkWidget *entry_plazo;

gint tipo_producto = 0; //El tipo de producto que se ha seleccionado en compra
gint calcular = 0;

/*FLAG:
  Auxiliar para el proceso de búsqueda de un producto
  permite saber si se desea buscar un producto para
  agregar a la compra, o una materia prima para 
  añadir derivados a ella
*/
gboolean add_to_comp = FALSE;

// Representa el numero de filas que se han 
// borrado en el filtro de busqueda
gint numFilasBorradas = 0;

gchar *rut_proveedor_global = NULL;

/* Contienen las filas del treeview que son borradas */
GArray *rutBorrado = NULL, *nombreBorrado = NULL, 
  *descripcionBorrada = NULL, *lapRepBorrado = NULL;

void
toggle_cell (GtkCellRendererToggle *cellrenderertoggle,
             gchar *path_string, gpointer user_data)
{
  GtkWidget *tree = (GtkWidget *) user_data;
  GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree));
  GtkTreePath *path = gtk_tree_path_new_from_string (path_string);
  gboolean toggle_item;
  GtkTreeIter iter;
  gint column = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (cellrenderertoggle), "column"));
  gchar *barcode;
  Producto *producto;

  gtk_tree_model_get_iter (model, &iter, path);

  gtk_tree_model_get (model, &iter,
                      0, &barcode,
                      column, &toggle_item,
                      -1);

  producto = SearchProductByBarcode (barcode, TRUE);

  toggle_item ^= 1;

  producto->canjear = toggle_item;

  gtk_tree_store_set (GTK_TREE_STORE (model), &iter,
                      column, toggle_item,
                      -1);

  gtk_tree_path_free (path);
}


/**
 * This function will edit the cell
 * Can edit a cell of G_TYPE STRING or INT
 *
 * //TODO: About arguments - Completar
 * @param cellrenderertext
 * @param path_string
 * @param new_string
 * @param user_data the user data
 */
void
edit_cell (GtkCellRendererText *cellrenderertext,
           gchar *path_string, gchar *new_string, gpointer user_data)
{
  GtkWidget *tree = (GtkWidget *) user_data;

  /* We get back the model */
  GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree));

  /* We get the path to the cell from the string */
  GtkTreePath *path = gtk_tree_path_new_from_string (path_string);
  GtkTreeIter iter;

  gchar *barcode;
  Producto *producto;

  /* What's the column number? */
  gint column = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (cellrenderertext), "column"));

  /* We get the type of the column */
  GType column_type = gtk_tree_model_get_column_type (model, column);

  gtk_tree_model_get_iter (model, &iter, path);

  gtk_tree_model_get (model, &iter,
                      0, &barcode,
                      -1);

  producto = SearchProductByBarcode (barcode, TRUE);

  switch (column_type)
    {
    case G_TYPE_INT:
      {
        gint new_number;                // Our new number
        new_number = atoi (new_string); // We get a string so we have to use atoi()

        if (new_number > (producto->stock_pro + (producto->stock_pro * ((double)1 / producto->tasa_canje))))
          {
            AlertMSG (tree, "No Puede canjear mas del stock en pro");
            gtk_tree_path_free (path);
            return;
          }

        /* We set the new value */
        gtk_tree_store_set (GTK_TREE_STORE (model), &iter,
                            column, new_number,
                            -1);

        producto->cuanto = new_number;
      }
      break;
    }
  gtk_tree_path_free (path);
}

/**
 * This callback is triggered when a cell on 
 * 'tree_view_pending_request_detail' is edited. (editable-event)
 * @param cell
 * @param path_string
 * @param new_cantity
 * @param data -> A GtkListStore
 */
void
on_buy_cantity_cell_renderer_edited (GtkCellRendererText *cell, gchar *path_string, gchar *cantity, gpointer data)
{
  // Treeview detalle de la compra
  GtkTreeModel *model;
  GtkTreePath *path;
  GtkTreeIter iter;
  
  // Treeview de la compra
  GtkTreeModel *store_pending_request;
  GtkTreeSelection *selection;

  // Variables para recoger información
  gchar *codigo;
  gchar *q;
  gint id_compra;
  gdouble new_cantity;
  gdouble joined_cantity;

  PGresult *res;

  /*Obtiene el modelo del treeview compra detalle*/
  model = GTK_TREE_MODEL (data);
  path = gtk_tree_path_new_from_string (path_string);

  /*Obtiene datos del treeview compra*/
  store_pending_request = gtk_tree_view_get_model (GTK_TREE_VIEW (builder_get (builder, "tree_view_pending_requests")));
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (builder_get (builder, "tree_view_pending_requests")));
  
  /* obtiene el iter para poder obtener y setear datos del treeview */
  gtk_tree_model_get_iter (model, &iter, path);
 
  //Se obtiene el codigo del producto y la cantidad ingresada
  gtk_tree_model_get (model, &iter,
                      0, &codigo,
		      4, &joined_cantity,
                      -1);
  
  /*Valida y Obtiene el precio nuevo*/
  if (!is_numeric (cantity))
    {
      ErrorMSG (GTK_WIDGET (builder_get (builder, "tree_view_pending_requests")),
		"La cantidad debe ser un valor numérico");
      return;
    }

  new_cantity = strtod (PUT (cantity), (char **)NULL);
  
  if (new_cantity < joined_cantity)
    {
      ErrorMSG (GTK_WIDGET (builder_get (builder, "tree_view_pending_requests")),
		"La cantidad solicitada debe ser mayor a la cantidad ya ingresada");
      return;
    }
  
  // Se setean los datos modificados en el treeview
  gtk_list_store_set (GTK_LIST_STORE (model), &iter,
		      2, new_cantity,
		      -1);

  //Se obtiene el id de la compra
  gtk_tree_selection_get_selected (selection, NULL, &iter);
  gtk_tree_model_get (store_pending_request, &iter,
                      0, &id_compra,
                      -1);

  //Se modifica el valor en compra
  q = g_strdup_printf ("SELECT barcode "
		       "FROM codigo_corto_to_barcode('%s')",
		       codigo);
  res = EjecutarSQL(q);
  codigo = g_strdup (PQvaluebycol (res, 0, "barcode"));

  q = g_strdup_printf ("UPDATE compra_detalle cd "
		       "SET cantidad = %s "
		       "WHERE cd.barcode_product = %s "
		       "AND cd.id_compra = %d",
		       CUT (g_strdup_printf ("%.3f", new_cantity)),
		       codigo, id_compra);
  EjecutarSQL(q);

  IngresoDetalle (selection, NULL);
}


/**
 * This callback is triggered when a cell on 
 * 'tree_view_pending_request_detail' is edited. (editable-event)
 * @param cell
 * @param path_string
 * @param new_price
 * @param data -> A GtkListStore
 */
void
on_buy_price_cell_renderer_edited (GtkCellRendererText *cell, gchar *path_string, gchar *ingress_price, gpointer data)
{
  // Treeview detalle de la compra
  GtkTreeModel *model;
  GtkTreePath *path;
  GtkTreeIter iter;
  
  // Treeview de la compra
  GtkTreeModel *store_pending_request;
  GtkTreeSelection *selection;

  // Variables para recoger información
  gchar *codigo;
  gchar *q;
  gint id_compra;
  gdouble new_price;

  PGresult *res;

  /*Obtiene el modelo del treeview compra detalle*/
  model = GTK_TREE_MODEL (data);
  path = gtk_tree_path_new_from_string (path_string);

  /*Obtiene datos del treeview compra*/
  store_pending_request = gtk_tree_view_get_model (GTK_TREE_VIEW (builder_get (builder, "tree_view_pending_requests")));
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (builder_get (builder, "tree_view_pending_requests")));

  /*Valida y Obtiene el precio nuevo*/
  if (!is_numeric (ingress_price))
    {
      ErrorMSG (GTK_WIDGET (model), "Costo unitario debe ser un valor numérico");
      return;
    }

  new_price = strtod (PUT (ingress_price), (char **)NULL);

  /*obtiene el iter para poder obtener y setear datos del treeview*/
  gtk_tree_model_get_iter (model, &iter, path);

  //Se setean los datos modificados en el treeview
  gtk_list_store_set (GTK_LIST_STORE (model), &iter,
		      2, new_price,
		      -1);

  //Se obtiene el codigo del producto
  gtk_tree_model_get (model, &iter,
                      0, &codigo,
                      -1);

  //Se obtiene el id de la compra
  gtk_tree_selection_get_selected (selection, NULL, &iter);
  gtk_tree_model_get (store_pending_request, &iter,
                      0, &id_compra,
                      -1);

  //Se modifica el valor en compra
  q = g_strdup_printf ("SELECT barcode "
		       "FROM codigo_corto_to_barcode('%s')",
		       codigo);
  res = EjecutarSQL(q);
  codigo = g_strdup (PQvaluebycol (res, 0, "barcode"));

  q = g_strdup_printf ("UPDATE compra_detalle cd "
		       "SET precio = %s "
		       "WHERE cd.barcode_product = %s "
		       "AND cd.id_compra = %d",
		       CUT (g_strdup_printf ("%.3f", new_price)),
		       codigo, id_compra);
  EjecutarSQL(q);

  IngresoDetalle (selection, NULL);
}


/**
 * This function hide "wnd_nullify_buy" window and clean
 * the Prods list
 *
 * @param: widget
 * @param: data
 */
gboolean
on_wnd_nullify_buy_close (GtkWidget *widget, gpointer data)
{
  gtk_widget_hide (GTK_WIDGET (builder_get (builder, "wnd_nullify_buy")));
  clean_container (GTK_CONTAINER (builder_get (builder, "wnd_nullify_buy")));
  //Si existe algo en la estructura "lista_mod_prod" se limpia
  clean_lista_mod_prod ();
  return TRUE;
}

/**
 * This function hide "wnd_edit_transfers" window and clean
 * the Prods list
 *
 * @param: widget
 * @param: data
 */
void
on_wnd_edit_transfers_close (GtkWidget *widget, gpointer data)
{
  gtk_widget_hide (GTK_WIDGET (builder_get (builder, "wnd_edit_transfers")));
  //Si existe algo en la estructura "lista_mod_prod" se limpia
  clean_lista_mod_prod ();
}


/**
 * This callback is triggered when a cell on 
 * 'treeview_nullify_buy_invoice_details' when
 * the cantity is edited. (editable-event)
 *
 * @param cell
 * @param path_string
 * @param new_cantity
 * @param data -> A GtkListStore
 */
void
on_mod_buy_cantity_cell_renderer_edited (GtkCellRendererText *cell, gchar *path_string, gchar *cantity, gpointer data)
{
  //Treeview detalle de la compra
  GtkTreeView *treeview;
  GtkTreeModel *model;
  GtkTreePath *path;
  GtkTreeIter iter;
  
  //Variables para recoger información
  gint total;
  gint tipo;
  gint id_fc;
  gchar *barcode;
  gchar *color;
  gdouble new_cantity;
  gdouble previous_cantity;
  gdouble original_cantity;
  gdouble cost;
  gdouble modificable;
  gboolean original;

  /*Original es FALSE hasta que se demuetre lo contrario*/
  original = FALSE;

  /*Tipo es MOD hasta que se demuestre los contrario*/
  tipo = MOD;

  /*Obtiene el modelo del treeview compra detalle*/
  model = GTK_TREE_MODEL (data);
  path = gtk_tree_path_new_from_string (path_string);

  /*Obtiene el iter para poder obtener y setear datos del treeview*/
  gtk_tree_model_get_iter (model, &iter, path);

  //Se obtiene el codigo del producto y la cantidad ingresada
  gtk_tree_model_get (model, &iter,
		      0, &barcode,
                      2, &previous_cantity,
  		      3, &cost,
		      6, &id_fc,
                      -1);
  
  /*Valida y Obtiene el precio nuevo*/
  if (!is_numeric (cantity))
    {
      ErrorMSG (GTK_WIDGET (builder_get (builder, "treeview_nullify_buy_invoice_details")),
		"La cantidad debe ser un valor numérico");
      return;
    }

  new_cantity = strtod (PUT (cantity), (char **)NULL);

  if (new_cantity <= 0)
    {
      ErrorMSG (GTK_WIDGET (builder_get (builder, "treeview_nullify_buy_invoice_details")),
		"La cantidad debe ser mayor a cero");
      return;
    }

  if (new_cantity != previous_cantity)
    gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_save_mod_buy")), TRUE);
  else
    return;
  
  /*Se agrega a la lista de los productos modificados*/

  //Se verifica si ya existe en la lista
  if (lista_mod_prod->header != NULL)
    lista_mod_prod->prods = buscar_prod (lista_mod_prod->header, barcode);
  else
    lista_mod_prod->prods = NULL;

  //Si no existe en la lista se agrega y se inicializa
  if (lista_mod_prod->prods == NULL)
    {
      add_to_mod_prod_list (barcode, FALSE);
      lista_mod_prod->prods->prod->id_factura_compra = id_fc;
      lista_mod_prod->prods->prod->cantidad_original = previous_cantity; /*Se guarda cantidad original*/
      original_cantity = previous_cantity; /*Esta variable se usará para verificar si es modificable*/
      lista_mod_prod->prods->prod->cantidad_nueva = new_cantity; /*Se guarda la nueva cantidad*/
      lista_mod_prod->prods->prod->accion = MOD; /*Indica que se modificará este producto en la BD (enum action)*/
      original = FALSE;
    } /*Si el producto existe pero se le modifica el costo por 1era vez*/
  else if (lista_mod_prod->prods->prod->cantidad_nueva == 0)
    {
      lista_mod_prod->prods->prod->id_factura_compra = id_fc;
      lista_mod_prod->prods->prod->cantidad_original = previous_cantity; /*Se guarda la cantidad priginal*/
      original_cantity = previous_cantity; /*Esta variable se usará para verificar si es modificable*/
      lista_mod_prod->prods->prod->cantidad_nueva = new_cantity; /*Se guarda la nueva cantidad*/
      original = FALSE;
    }
  else //Si estaba en la lista y se modifica el costo por enésima vez (incluyendo productos agregados)
    {  //Si el costo nuevo es igual al original
      if (new_cantity == lista_mod_prod->prods->prod->cantidad_original)
	{
	  lista_mod_prod->prods->prod->cantidad_nueva = 0;
	  
	  //Si no hay modificaciones en costo
	  if (lista_mod_prod->prods->prod->costo_nuevo == 0)
	    {
	      /*Esta variable se usará para verificar si es modificable*/
	      original_cantity = lista_mod_prod->prods->prod->cantidad_original;

	      drop_prod_to_mod_list (barcode);
	      original = TRUE;
	    }

	  if (cantidad_total_prods (lista_mod_prod->header) == 0) //Si no quedan productos en la lista
	    gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_save_mod_buy")), FALSE);
	} 
      else //De lo contrario se modifica (Los productos agregados siempre llegaran aquí)
	{ 
	  lista_mod_prod->prods->prod->id_factura_compra = id_fc;
	  lista_mod_prod->prods->prod->cantidad_nueva = new_cantity;
	  original_cantity = lista_mod_prod->prods->prod->cantidad_original;

	  original = FALSE;
	  tipo = lista_mod_prod->prods->prod->accion;
	}
    }

  /* SI se agregan unidades es modificable sin necesidad de realizar una comprobación
     SINO Si el producto existe en esta factura se realiza una comprobación mas
     De no existir en la factura es modificable pues no tiene datos historicos en esa factura */
  if (new_cantity > original_cantity)
    modificable = 1;
  else if (DataExist (g_strdup_printf ("SELECT barcode FROM factura_compra_detalle WHERE id_factura_compra = %d", id_fc)))
    modificable = cantidad_compra_es_modificable (barcode, new_cantity, id_fc);
  else
    modificable = 1;
  
  /* Si modificable es <= 0, quiere decir que 'cantidad_compra_es_modificable' devolvió ese valor 
     y por lo tanto no es modificable*/
  if (modificable <= 0)
    {
      if (modificable <= 0 && new_cantity == original_cantity)
	printf ("El producto %s de la factura %d devolvio modificable <= 0\n"
		"con su stock original, revisar producto con cuadratura \n", barcode, id_fc);
      
      gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_save_mod_buy")), FALSE);
      drop_prod_to_mod_list (barcode);
      ErrorMSG (GTK_WIDGET (builder_get (builder, "treeview_nullify_buy_invoice_details")),
		g_strdup_printf ("Debido a las transacciones realizadas sobre el producto %s en fechas \n"
				 "posteriores a esta compra, la cantidad mínima debe ser: %.3f",
				 barcode, (previous_cantity + modificable) ));
      //NOTA: modificable es 0 o negativo, por lo tanto se restan o permanece igual
      return;
    }

  /*color*/
  if (original == TRUE)
    color = g_strdup ("Black");
  else
    {
      if (tipo == MOD)
	color = g_strdup ("Red");
      else if (tipo == ADD)
	color = g_strdup ("Blue");
    }

  // Se setean los datos modificados en el treeview
  gtk_list_store_set (GTK_LIST_STORE (model), &iter,
		      2, new_cantity,                 //cantidad
		      4, lround (new_cantity * cost), //subtotal
		      7, color,
		      8, TRUE,
		      -1);

  //Se recalcula el costo total de la factura seleccionada
  treeview = GTK_TREE_VIEW (builder_get (builder, "treeview_nullify_buy_invoice_details"));
  total = lround (sum_treeview_column (treeview, 4, G_TYPE_INT));
  
  //Se actualiza el nuevo costo en el treeview de la factura
  treeview = GTK_TREE_VIEW (builder_get (builder, "treeview_nullify_buy_invoice"));
  model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
  gtk_tree_selection_get_selected (gtk_tree_view_get_selection (treeview), NULL, &iter);

  gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                      4, total,
                      -1);

  //Se recalcula el costo total de la compra
  total = lround (sum_treeview_column (treeview, 4, G_TYPE_INT));

  //Se actualiza el nuevo costo en el treeview de la compra
  treeview = GTK_TREE_VIEW (builder_get (builder, "treeview_nullify_buy"));
  model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
  gtk_tree_selection_get_selected (gtk_tree_view_get_selection (treeview), NULL, &iter);

  gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                      2, total,
                      -1);
}

/**
 * This callback is triggered when a cell on 
 * 'treeview_nullify_buy_invoice_details' when
 * the cantity is edited. (editable-event)
 *
 * @param cell
 * @param path_string
 * @param new_price
 * @param data -> A GtkListStore
 */
void
on_mod_buy_price_cell_renderer_edited (GtkCellRendererText *cell, gchar *path_string, gchar *cost, gpointer data)
{
  // Treeview detalle de la compra
  GtkTreeView *treeview;
  GtkTreeModel *model;
  GtkTreePath *path;
  GtkTreeIter iter;
  
  // Variables para recoger información
  gint total;
  gint tipo;
  gint id_fc;
  gchar *barcode;
  gchar *color;
  gdouble new_cost;
  gdouble previous_cost;
  gdouble cantity;
  gboolean original;

  /*Original es FALSE hasta que se demuetre lo contrario*/
  original = FALSE;
  
  /*Tipo es MOD hasta que se demuestre los contrario*/
  tipo = MOD;

  /*Obtiene el modelo del treeview compra detalle*/
  model = GTK_TREE_MODEL (data);
  path = gtk_tree_path_new_from_string (path_string);

  /* obtiene el iter para poder obtener y setear datos del treeview */
  gtk_tree_model_get_iter (model, &iter, path);
 
  //Se obtiene el el precio original
  gtk_tree_model_get (model, &iter,
		      0, &barcode,
		      2, &cantity,
  		      3, &previous_cost,
		      6, &id_fc,
                      -1);
  
  /*Valida y Obtiene el precio nuevo*/
  if (!is_numeric (cost))
    {
      ErrorMSG (GTK_WIDGET (builder_get (builder, "treeview_nullify_buy_invoice_details")),
		"El precio de compra debe ser un valor numérico");
      return;
    }

  new_cost = strtod (PUT (cost), (char **)NULL);
  
  if (new_cost <= 0)
    {
      ErrorMSG (GTK_WIDGET (builder_get (builder, "treeview_nullify_buy_invoice_details")),
		"El costo debe ser mayor a cero");
      return;
    }

  //Si asegura que continúe solo si se cambió el costo
  if (new_cost != previous_cost)
    gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_save_mod_buy")), TRUE);
  else
    return;
  
  /*Se agrega a la lista de los productos modificados*/  

  //Se verifica si ya existe en la lista
  if (lista_mod_prod->header != NULL)
    lista_mod_prod->prods = buscar_prod (lista_mod_prod->header, barcode);
  else
    lista_mod_prod->prods = NULL;  

  //Si no existe en la lista se agrega y se inicializa
  if (lista_mod_prod->prods == NULL) //PEEEEE
    {
      add_to_mod_prod_list (barcode, FALSE);
      lista_mod_prod->prods->prod->id_factura_compra = id_fc;
      lista_mod_prod->prods->prod->costo_original = previous_cost; /*Se guarda el costo original*/
      lista_mod_prod->prods->prod->costo_nuevo = new_cost; /*Se guarda el nuevo costo*/
      lista_mod_prod->prods->prod->accion = MOD; /*Indica que se modificará este producto en la BD (enum action)*/
      original = FALSE;
    } /*Si el producto existe pero se le modifica el costo por 1era vez*/
  else if (lista_mod_prod->prods->prod->costo_original == 0)
    {
      lista_mod_prod->prods->prod->id_factura_compra = id_fc;
      lista_mod_prod->prods->prod->costo_original = previous_cost; /*Se guarda el costo original*/
      lista_mod_prod->prods->prod->costo_nuevo = new_cost; /*Se guarda el nuevo costo*/
      original = FALSE;
    }
  else //Si estaba en la lista y se modifica el costo por enésima vez (incluyendo productos agregados)
    {  //Si el costo nuevo es igual al original
      if (new_cost == lista_mod_prod->prods->prod->costo_original)
	{	  
	  lista_mod_prod->prods->prod->costo_nuevo = 0;
	  
	  //Si no hay modificaciones en cantidad
	  if (lista_mod_prod->prods->prod->cantidad_nueva == 0)
	    {
	      drop_prod_to_mod_list (barcode);
	      original = TRUE;
	    }

	  if (cantidad_total_prods (lista_mod_prod->header) == 0) //Si no quedan productos en la lista
	    gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_save_mod_buy")), FALSE);          
	}
      else //De lo contrario se modifica (Los productos agregados siempre llegaran aquí)
	{ 
	  lista_mod_prod->prods->prod->id_factura_compra = id_fc;
	  lista_mod_prod->prods->prod->costo_nuevo = new_cost;
	  original = FALSE;
	  tipo = lista_mod_prod->prods->prod->accion;
	}
    }

  /*color*/
  if (original == TRUE)
    color = g_strdup ("Black");
  else
    {
      if (tipo == MOD)
	color = g_strdup ("Red");
      else if (tipo == ADD)
	color = g_strdup ("Blue");
    }

  // Se setean los datos modificados en el treeview
  gtk_list_store_set (GTK_LIST_STORE (model), &iter,
		      3, new_cost,                    //costo
		      4, lround (new_cost * cantity), //subtotal
		      7, color,
		      8, TRUE,
		      -1);

  //Se recalcula el costo total de la factura seleccionada
  treeview = GTK_TREE_VIEW (builder_get (builder, "treeview_nullify_buy_invoice_details"));
  total = lround (sum_treeview_column (treeview, 4, G_TYPE_INT));
  
  //Se actualiza el nuevo costo en el treeview de la factura
  treeview = GTK_TREE_VIEW (builder_get (builder, "treeview_nullify_buy_invoice"));
  model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
  gtk_tree_selection_get_selected (gtk_tree_view_get_selection (treeview), NULL, &iter);

  gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                      4, total,
                      -1);

  //Se recalcula el costo total de la compra
  total = lround (sum_treeview_column (treeview, 4, G_TYPE_INT));

  //Se actualiza el nuevo costo en el treeview de la compra
  treeview = GTK_TREE_VIEW (builder_get (builder, "treeview_nullify_buy"));
  model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
  gtk_tree_selection_get_selected (gtk_tree_view_get_selection (treeview), NULL, &iter);

  gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                      2, total,
                      -1);
}


/**
 * show window "wnd_new_prod_on_buy" when satisfied the
 * conditions
 *
 * @param: button
 * @param: data
 */
void
on_btn_add_buy_prod_clicked (GtkButton *button, gpointer data)
{
  if (get_treeview_length (GTK_TREE_VIEW (builder_get (builder, "treeview_nullify_buy_invoice_details"))) > 0)
    gtk_widget_show (GTK_WIDGET (builder_get (builder, "wnd_new_prod_on_buy")));
  else
    ErrorMSG (GTK_WIDGET (builder_get (builder, "entry_nullify_buy_id")), 
	      "Primero debe buscar una compra y seleccionar una factura");

  clean_container (GTK_CONTAINER (builder_get (builder, "wnd_new_prod_on_buy")));
  gtk_widget_grab_focus (GTK_WIDGET (builder_get (builder, "entry_prod_on_buy_barcode")));
}


/**
 * Callback from "btn_add_prod_on_buy" Button
 * (signal click)
 * Add a product to list to mod the invoice
 *
 * Añade el producto deleccionado a la lista de productos
 * que serán añadidos a la factura.
 *
 * @param GtkButton *button
 * @param gpointer data
 */
void
on_btn_add_prod_on_buy_clicked (GtkButton *button, gpointer data)
{
  GtkTreeView *treeview;
  GtkTreeModel *model;
  GtkListStore *store;
  GtkTreeIter iter;

  gdouble costo;
  gchar *barcode;
  gint id_buy;
  gint id_invoice;

  PGresult *res;

  barcode = g_strdup (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_prod_on_buy_barcode"))));

  if (g_str_equal (barcode, ""))
    return;

  res = EjecutarSQL (g_strdup_printf ("SELECT barcode "
				      "FROM producto "
				      "WHERE barcode = %s "
				      "OR codigo_corto = '%s'", barcode, barcode));

  if (res == NULL || PQntuples (res) <= 0)
    {
      ErrorMSG (GTK_WIDGET (builder_get (builder, "entry_prod_on_buy_barcode")), 
		"No existe un producto con ese codigo corto o de barras");
      return;
    }

  barcode = PQvaluebycol (res, 0, "barcode");
  gtk_entry_set_text (GTK_ENTRY (builder_get (builder, "entry_prod_on_buy_barcode")), barcode);

  /*Comprueba si ya esta en el treeview*/
  treeview = GTK_TREE_VIEW (builder_get (builder, "treeview_nullify_buy_invoice_details"));
  model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
  store = GTK_LIST_STORE (gtk_tree_view_get_model (treeview));

  if (compare_treeview_column (treeview, 0, G_TYPE_STRING, barcode))
    {
      ErrorMSG (GTK_WIDGET (builder_get (builder, "entry_prod_on_buy_barcode")), 
		"Ya existe un producto con ese codigo corto o de barras en la compra");
      return;
    }

  if (!gtk_tree_model_get_iter_first (model, &iter))
    {
      return;
    }

  /*Se obtiene el id de compra y facturas de esta seleccion*/
  gtk_tree_model_get (model, &iter,
		      5, &id_buy,
  		      6, &id_invoice,
                      -1);

  /*Se agrega a la lista de los productos modificados*/  
  //Se verifica si ya existe en la lista
  if (lista_mod_prod->header != NULL)
    lista_mod_prod->prods = buscar_prod (lista_mod_prod->header, barcode);
  else
    lista_mod_prod->prods = NULL;  

  //Si no existe en la lista se agrega y se inicializa
  if (lista_mod_prod->prods == NULL) //PEEEEE
    {
      costo = get_last_buy_price (barcode);
      add_to_mod_prod_list (barcode, FALSE);
      lista_mod_prod->prods->prod->id_factura_compra = id_invoice;
      lista_mod_prod->prods->prod->costo_original = costo;
      lista_mod_prod->prods->prod->costo_nuevo = costo;
      lista_mod_prod->prods->prod->cantidad_original = 1;
      lista_mod_prod->prods->prod->cantidad_nueva = 1;
      lista_mod_prod->prods->prod->accion = ADD;
    }
  else
    {
      //Error
      ErrorMSG (GTK_WIDGET (builder_get (builder, "entry_prod_on_buy_barcode")), 
		"El producto ya existe en la lista");
      return;
    }


  /*Se 'selecciona' la primera fila de este treeview*/
  if (!gtk_tree_model_get_iter_first (model, &iter))
    {
      return;
    }

  /*Agregar al treeview*/
  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter,
		      0, lista_mod_prod->prods->prod->barcode,
		      1, lista_mod_prod->prods->prod->descripcion,
		      2, lista_mod_prod->prods->prod->cantidad_nueva,
		      3, lista_mod_prod->prods->prod->costo_nuevo,
		      4, lround (lista_mod_prod->prods->prod->costo_nuevo * lista_mod_prod->prods->prod->cantidad_nueva),
		      5, id_buy,
		      6, id_invoice,
		      7, "Blue",
		      8, TRUE,
		      -1);

  gtk_widget_hide (GTK_WIDGET (gtk_builder_get_object (builder, "wnd_new_prod_on_buy")));

  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_save_mod_buy")), TRUE);
}

/**
 * Callback from "entry_prod_on_buy_barcode" entry.
 * (signal activate)
 *
 * Comprueba que exista el codigo o barcode ingresado, de existir
 * setea el entry con el barcode del producto.
 *
 * @param: GtkEntry *entry
 * @param: gpointer data
 */
void 
on_entry_prod_on_buy_barcode_activate (GtkEntry *entry, gpointer data)
{
  gchar *barcode;
  PGresult *res;
  GtkTreeView *treeview;

  barcode = g_strdup (gtk_entry_get_text (entry));
  res = EjecutarSQL (g_strdup_printf ("SELECT barcode "
				      "FROM producto "
				      "WHERE barcode = %s "
				      "OR codigo_corto = '%s'", barcode, barcode));

  if (res != NULL && PQntuples (res) > 0)
    {
      barcode = PQvaluebycol (res, 0, "barcode");

      /*Comprueba si ya esta en el treeview*/
      treeview = GTK_TREE_VIEW (builder_get (builder, "treeview_nullify_buy_invoice_details"));
      if (compare_treeview_column (treeview, 0, G_TYPE_STRING, barcode))
	{
	  ErrorMSG (GTK_WIDGET (builder_get (builder, "entry_prod_on_buy_barcode")), 
		    "Ya existe un producto con ese codigo corto o de barras en la compra");
	  return;
	}

      gtk_entry_set_text (entry, barcode);
      gtk_widget_grab_focus (GTK_WIDGET (builder_get (builder, "btn_add_prod_on_buy")));
    }
  else
    ErrorMSG (GTK_WIDGET (entry), "No existe un producto con ese codigo corto de barras");
}



/**
 * Remove products from "treeview_nullify_buy_invoice_details"
 * and add them to Prods list with "DEL" flag.
 *
 * @param: button
 * @param: data
 */
void
on_btn_rmv_buy_prod_clicked (GtkButton *button, gpointer data)
{
  GtkTreeView *treeview;
  GtkTreeModel *model;
  GtkTreeIter iter;

  gchar *barcode;
  gdouble cantity;
  gdouble cost;
  gint position, id_fc;

  treeview = GTK_TREE_VIEW (builder_get (builder, "treeview_nullify_buy_invoice_details"));
  model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
  if (gtk_tree_selection_get_selected (gtk_tree_view_get_selection (treeview), NULL, &iter))
    {
      gtk_tree_model_get (model, &iter,
			  0, &barcode,
			  2, &cantity,
			  3, &cost,
			  6, &id_fc,
			  -1);

      //Se verifica si ya existe en la lista
      if (lista_mod_prod->header != NULL)
	lista_mod_prod->prods = buscar_prod (lista_mod_prod->header, barcode);
      else
	lista_mod_prod->prods = NULL;

      //Si no existe en la lista se agrega y se inicializa
      if (lista_mod_prod->prods == NULL) //PEEEEE
	{
	  add_to_mod_prod_list (barcode, FALSE);
	  lista_mod_prod->prods->prod->id_factura_compra = id_fc;
	  lista_mod_prod->prods->prod->costo_original = cost;
	  lista_mod_prod->prods->prod->cantidad_original = cantity;
	  lista_mod_prod->prods->prod->accion = DEL;
	}
      //Si existe y es ADD se quita de la lista
      else if (lista_mod_prod->prods->prod->accion == ADD)
	rmv_prod_from_prod_list (barcode, lista_mod_prod->prods->prod->lugar);
      //Si existe y es MOD ahora se cambia a DEL
      else if (lista_mod_prod->prods->prod->accion == MOD)
	lista_mod_prod->prods->prod->accion = DEL;
      else if (lista_mod_prod->prods->prod->accion == DEL)
	printf ("Se agrega el producto %s para DEL que ya existe con dicha accion\n",
		barcode);

      position = atoi (gtk_tree_model_get_string_from_iter (model, &iter));
      gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
      select_back_deleted_row ("treeview_nullify_buy_invoice_details", position);

      if (cantidad_total_prods (lista_mod_prod->header) == 0) //Si no quedan productos en la lista
	gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_save_mod_buy")), FALSE);
      else
	gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_save_mod_buy")), TRUE);
    }  
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
calculate_shipping_amount (GtkEditable *editable,
			   gchar *new_text,
			   gint new_text_length,
			   gint *position,
			   gpointer user_data)
{
  gchar *total_invoice_text;
  gint total_invoice;
  gint sub_total_productos = atoi (g_strdup (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_products_invoice_amount")))));
  gint resto;
  
  total_invoice_text = g_strdup_printf ("%s%s", gtk_entry_get_text (GTK_ENTRY (editable)), new_text);
  total_invoice = atoi (total_invoice_text);
  resto = total_invoice - sub_total_productos;

  if (resto < 0)
    {
      gtk_label_set_markup (GTK_LABEL (builder_get (builder, "subtotal_shipping_invoice")),
			    g_strdup_printf ("<span color =\"red\">%s</span> ",
					     PutPoints (g_strdup_printf ("%d", resto))));
      gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_ok_ingress_invoice")), FALSE);
    }
  else
    {
      gtk_label_set_text (GTK_LABEL (builder_get (builder, "subtotal_shipping_invoice")),
			  g_strdup_printf ("%s", PutPoints (g_strdup_printf ("%d", resto))));
      gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_ok_ingress_invoice")), TRUE);
    }
}


/**
 * Build "wnd_compras" - rizoma-compras's window
 * initiallizes and show all its components
 * @param void
 */
void
compras_win (void)
{
  GtkWidget *compras_gui;
  //gchar *fullscreen_opt = NULL;

  GtkListStore *store;
  GtkTreeView *treeview;

  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;

  GtkTreeSelection *selection;

  GError *error = NULL;

  /*ComboBox - PrecioCompra*/
  GtkComboBox *combo;
  GtkTreeIter iter;
  GtkListStore *modelo;
  GtkCellRenderer *cell2;

  compra = (Compra *) g_malloc (sizeof (Compra));
  compra->header = NULL;
  compra->products_list = NULL;
  compra->header_compra = NULL;
  compra->products_compra = NULL;
  compra->current = NULL;

  compra->documentos = NULL;

  //Estructura para la modificación de compras ingresadas
  lista_mod_prod = (ListaModProd *) g_malloc (sizeof (ListaModProd));
  lista_mod_prod->header = NULL;
  lista_mod_prod->prods = NULL;
  lista_mod_prod->check = NULL;

  builder = gtk_builder_new ();

  gtk_builder_add_from_file (builder, DATADIR"/ui/rizoma-compras.ui", &error);

  if (error != NULL)
    g_printerr ("%s\n", error->message);

  gtk_builder_add_from_file (builder, DATADIR"/ui/rizoma-common.ui", &error);

  if (error != NULL)
    g_printerr ("%s\n", error->message);

  gtk_builder_connect_signals (builder, NULL);
  compras_gui = GTK_WIDGET (gtk_builder_get_object (builder, "wnd_compras"));

  //Titulo
  gtk_window_set_title (GTK_WINDOW (compras_gui), 
			g_strdup_printf ("POS Rizoma Comercio: Compras - Conectado a [%s@%s]",
					 config_profile,
					 rizoma_get_value ("SERVER_HOST")));

  if (rizoma_get_value_boolean ("TRASPASO"))
    {
      gtk_widget_show (GTK_WIDGET (builder_get (builder, "btn_traspaso_enviar")));
      gtk_widget_show (GTK_WIDGET (builder_get (builder, "btn_traspaso_recibir")));
      gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "rdbutton_transfer_mode")), TRUE);

      gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_traspaso_enviar")), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_traspaso_recibir")), FALSE);
    }

  // check if the window must be set to fullscreen
  //fullscreen_opt = rizoma_get_value ("FULLSCREEN");

  if (rizoma_get_value_boolean("FULLSCREEN"))
    gtk_window_fullscreen (GTK_WINDOW (compras_gui));

  /* History TreeView */
  store = gtk_list_store_new (8,
                              G_TYPE_STRING, //Fecha
                              G_TYPE_STRING, //Id compra
                              G_TYPE_STRING, //Proveedor
                              G_TYPE_DOUBLE, //cantidad solicitada
			      G_TYPE_DOUBLE, //cantidad ingresada
                              G_TYPE_DOUBLE, //Costo sin impuestos
			      G_TYPE_DOUBLE, //Costo con impuestos
			      G_TYPE_INT); //Total con impuestos

  treeview = GTK_TREE_VIEW (gtk_builder_get_object (builder, "product_history_tree_view"));
  gtk_tree_view_set_model (GTK_TREE_VIEW (treeview), GTK_TREE_MODEL (store));

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Fecha", renderer,
                                                     "text", 0,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Id", renderer,
                                                     "text", 1,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Proveedor", renderer,
                                                     "text", 2,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_expand (column, TRUE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Solicitado", renderer,
                                                     "text", 3,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)3, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Ingresado", renderer,
                                                     "text", 4,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)4, NULL);


  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Costo Neto", renderer,
                                                     "text", 5,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)5, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Costo Bruto", renderer,
                                                     "text", 6,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)6, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Total Bruto", renderer,
                                                     "text", 7,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)7, NULL);


  /* End History TreeView */

  /* Buying Products TreeView */

  store = gtk_list_store_new (8,
			      G_TYPE_STRING,   // Barcode
                              G_TYPE_STRING,   // Codigo
                              G_TYPE_STRING,   // Marca, Descripcion, Contenido y Unidad
                              G_TYPE_DOUBLE,   // Cantidad
			      G_TYPE_DOUBLE,   // Precio Unitario
                              G_TYPE_DOUBLE,   // Costo Unitario
                              G_TYPE_INT,      // Total
			      G_TYPE_BOOLEAN); // Transferible (traspaso envio)

  if (compra->header_compra != NULL)
    {
      compra->products_compra = compra->header_compra;
      do
	{
	  compra->current = compra->products_compra->product;
	  AddToTree ();
	  compra->products_compra = compra->products_compra->next;
	} while (compra->products_compra != compra->header_compra);
    }

  treeview = GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_products_buy_list"));
  gtk_tree_view_set_model (GTK_TREE_VIEW (treeview), GTK_TREE_MODEL (store));

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Codigo", renderer,
                                                     "text", 1,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Producto", renderer,
                                                     "text", 2,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_expand (column, TRUE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Cantidad", renderer,
                                                     "text", 3,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_resizable (column, FALSE);

  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)3, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Precio Unit.", renderer,
                                                     "text", 4,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)4, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Costo Unit.", renderer,
                                                     "text", 5,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)5, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("SubTotal", renderer,
                                                     "text", 6,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)6, NULL);

  /* End Buying Products TreeView*/


  /* Pending Requests */

  store = gtk_list_store_new (6,
                              G_TYPE_INT,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_INT,
                              G_TYPE_STRING,
                              G_TYPE_BOOLEAN);

  treeview = GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_pending_requests"));
  gtk_tree_view_set_model (GTK_TREE_VIEW (treeview), GTK_TREE_MODEL (store));

  selection = gtk_tree_view_get_selection (treeview);

  gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);

  g_signal_connect (G_OBJECT (selection), "changed",
                    G_CALLBACK (IngresoDetalle), NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("ID", renderer,
                                                     "text", 0,
                                                     "foreground", 4,
                                                     "foreground-set", 5,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Fecha", renderer,
                                                     "text", 1,
                                                     "foreground", 4,
                                                     "foreground-set", 5,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Proveedor", renderer,
                                                     "text", 2,
                                                     "foreground", 4,
                                                     "foreground-set", 5,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Costo Total", renderer,
                                                     "text", 3,
                                                     "foreground", 4,
                                                     "foreground-set", 5,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)3, NULL);


  /* End Pending Requests */

  /* Pending Request Detail */

  store = gtk_list_store_new (8,
                              G_TYPE_STRING,   //codigo
                              G_TYPE_STRING,   //producto
                              G_TYPE_DOUBLE,   //Costo unitario
                              G_TYPE_DOUBLE,   //cantidad solicitada
                              G_TYPE_DOUBLE,   //cantidad ingresada
                              G_TYPE_INT,      //sub-total
                              G_TYPE_STRING,
                              G_TYPE_BOOLEAN);

  treeview = GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_pending_request_detail"));
  gtk_tree_view_set_model (GTK_TREE_VIEW (treeview), GTK_TREE_MODEL (store));

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Codigo", renderer,
                                                     "text", 0,
                                                     "foreground", 6,
                                                     "foreground-set", 7,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Producto", renderer,
                                                     "text", 1,
                                                     "foreground", 6,
                                                     "foreground-set", 7,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  gtk_tree_view_column_set_resizable (column, FALSE);

  
  renderer = gtk_cell_renderer_text_new ();
  g_object_set (renderer,"editable", TRUE, NULL);
  g_signal_connect (G_OBJECT (renderer), "edited",
		    G_CALLBACK (on_buy_price_cell_renderer_edited), (gpointer)store);
  column = gtk_tree_view_column_new_with_attributes ("Costo Unit.", renderer,
                                                     "text", 2,
                                                     "foreground", 6,
                                                     "foreground-set", 7,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)2, NULL);

  renderer = gtk_cell_renderer_text_new ();
  g_object_set (renderer,"editable", TRUE, NULL);
  g_signal_connect (G_OBJECT (renderer), "edited",
		    G_CALLBACK (on_buy_cantity_cell_renderer_edited), (gpointer)store);
  column = gtk_tree_view_column_new_with_attributes ("Cant. Sol.", renderer,
                                                     "text", 3,
                                                     "foreground", 6,
                                                     "foreground-set", 7,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)3, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Cant. Ing.", renderer,
                                                     "text", 4,
                                                     "foreground", 6,
                                                     "foreground-set", 7,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)4, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Sub-Total", renderer,
                                                     "text", 5,
                                                     "foreground", 6,
                                                     "foreground-set", 7,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)5, NULL);

  /* End Pending Request Detail */

  /* Guide -> Invoice */

  store = gtk_list_store_new (3,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_STRING);

  treeview = GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_pending_guide"));
  gtk_tree_view_set_model (GTK_TREE_VIEW (treeview), GTK_TREE_MODEL (store));

  g_signal_connect (G_OBJECT (gtk_tree_view_get_selection (treeview)), "changed",
                    G_CALLBACK (on_tree_selection_pending_guide_changed), NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("ID Guia", renderer,
                                                     "text", 0,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Nº Guia", renderer,
                                                     "text", 1,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Monto", renderer,
                                                     "text", 2,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  store = gtk_list_store_new (3,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_STRING);

  treeview = GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_guide_invoice"));
  gtk_tree_view_set_model (GTK_TREE_VIEW (treeview), GTK_TREE_MODEL (store));

  g_signal_connect (G_OBJECT (gtk_tree_view_get_model (treeview)), "row-changed",
                    G_CALLBACK (CalcularTotalesGuias), NULL);

  g_signal_connect (G_OBJECT (gtk_tree_view_get_model (treeview)), "row-deleted",
                    G_CALLBACK (CalcularTotalesGuias), NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("ID Guia", renderer,
                                                     "text", 0,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Nº Guia", renderer,
                                                     "text", 1,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Monto", renderer,
                                                     "text", 2,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  store = gtk_list_store_new (5,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_DOUBLE,
                              G_TYPE_DOUBLE);

  treeview = GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_guide_invoice_detail"));
  gtk_tree_view_set_model (GTK_TREE_VIEW (treeview), GTK_TREE_MODEL (store));

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Codigo", renderer,
                                                     "text", 0,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Producto", renderer,
                                                     "text", 1,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Unit.", renderer,
                                                     "text", 2,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Cant.", renderer,
                                                     "text", 3,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)3, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Sub-Total", renderer,
                                                     "text", 4,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)4, NULL);

  /* End Guide -> Invoice */

  /* Pay Invoices */

  store = gtk_list_store_new (7,
                              G_TYPE_STRING,   //ID Factura
                              G_TYPE_STRING,   //RUT
                              G_TYPE_STRING,   //N° Factura
                              G_TYPE_STRING,   //ID compra
                              G_TYPE_STRING,   //F. Emisión
                              G_TYPE_STRING,   //F. Pagos
                              G_TYPE_INT);     //Monto Compra

  treeview = GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_invoice_list"));
  gtk_tree_view_set_model (GTK_TREE_VIEW (treeview), GTK_TREE_MODEL (store));

  selection = gtk_tree_view_get_selection (treeview);

  gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);

  g_signal_connect (G_OBJECT (selection), "changed",
                    G_CALLBACK (on_tree_view_invoice_list_selection_changed), NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("ID Factura", renderer,
                                                     "text", 0,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);


  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Rut", renderer,
                                                     "text", 1,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_rut, (gpointer)1, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("N° Factura", renderer,
                                                     "text", 2,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("ID Compra", renderer,
                                                     "text", 3,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("F. Emision", renderer,
                                                     "text", 4,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Fecha Pagos", renderer,
                                                     "text", 5,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Monto", renderer,
                                                     "text", 6,
                                                     NULL);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)6, NULL);

  store = gtk_list_store_new (5,
                              G_TYPE_STRING,  //Codigo
                              G_TYPE_STRING,  //Descripcion
                              G_TYPE_DOUBLE,  //Costo unitario
                              G_TYPE_DOUBLE,  //Cantidad
                              G_TYPE_INT);    //Sub-total

  treeview = GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_invoice_detail"));
  gtk_tree_view_set_model (GTK_TREE_VIEW (treeview), GTK_TREE_MODEL (store));

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Codigo", renderer,
                                                     "text", 0,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Producto", renderer,
                                                     "text", 1,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Costo Unit.", renderer,
                                                     "text", 2,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)2, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Cant.", renderer,
                                                     "text", 3,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)3, NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Sub-Total Neto", renderer,
                                                     "text", 4,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
  gtk_tree_view_column_set_alignment (column, 0.5);
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_column_set_resizable (column, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)4, NULL);

  /*ComboBox - PrecioCompra*/
  modelo = gtk_list_store_new (1, G_TYPE_STRING);
  combo = (GtkComboBox *) gtk_builder_get_object (builder, "cmbPrecioCompra");
  cell2 = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT(combo), cell2, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT(combo), cell2, "text", 0, NULL);

  /*Se agregan las opciones al combobox*/
  gtk_list_store_append (modelo, &iter);
  gtk_list_store_set (modelo, &iter, 0, "Costo neto", -1);

  gtk_list_store_append (modelo, &iter);
  gtk_list_store_set (modelo, &iter, 0, "Costo bruto", -1);

  gtk_combo_box_set_model (combo, (GtkTreeModel *)modelo);
  gtk_combo_box_set_active (combo, 0);

  /* End Pay Invoices */

  //mercaderia
  admini_box();

  //setup the providers tab
  proveedores_box();

  //Setup edit product button
  //setup_mod_prod_menu (); TODO: Hacer que funcione


  //Default disable entry and buttons
  //TODO: Ver como simplificar el set_sensitive
  gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "entry_buy_price")), FALSE);
  gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "entry_buy_gain")), FALSE);
  gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "entry_sell_price")), FALSE);
  gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "entry_buy_amount")), FALSE);
  gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "edit_product_button")), FALSE);
  gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "button_calculate")), FALSE);
  gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "button_add_product_list")), FALSE);
  gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "button_buy")), FALSE);
  gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "cmbPrecioCompra")), FALSE);

  //Compras - Mercadería
  //gtk_widget_set_sensitive (GTK_ENTRY (gtk_builder_get_object (builder, "btn_infomerca_save")), FALSE);
  //Compras - Mercadería - Editar Producto
  gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "entry_edit_prod_shortcode")), FALSE);
  gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "entry_edit_prod_barcode")), FALSE);


  //Restricciones a los entry 

  //Pestaña compras (barcode, costo, margen, precio y cantidad)
  gtk_entry_set_max_length (GTK_ENTRY (builder_get (builder, "entry_buy_barcode")), 25);
  gtk_entry_set_max_length (GTK_ENTRY (builder_get (builder, "entry_buy_price")), 9);
  gtk_entry_set_max_length (GTK_ENTRY (builder_get (builder, "entry_buy_gain")), 6);
  gtk_entry_set_max_length (GTK_ENTRY (builder_get (builder, "entry_sell_price")), 9);
  gtk_entry_set_max_length (GTK_ENTRY (builder_get (builder, "entry_buy_amount")), 6);

  //Pestaña mercaderías
  gtk_entry_set_max_length (GTK_ENTRY (builder_get (builder, "entry_infomerca_percentmargin")), 7);
  gtk_entry_set_max_length (GTK_ENTRY (builder_get (builder, "entry_informerca_price")), 9);
  gtk_entry_set_max_length (GTK_ENTRY (builder_get (builder, "entry_informerca_cantmayorist")), 4);
  gtk_entry_set_max_length (GTK_ENTRY (builder_get (builder, "entry_informerca_pricemayorist")), 9);
  gtk_entry_set_max_length (GTK_ENTRY (builder_get (builder, "entry_informerca_dstock")), 6);

  //Anulación de compras solo puede ser efectuada por el administrador
  if (user_data->user_id != 1)
    gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "btn_nullify_buy_pi")), FALSE);

  //Focus Control
  //TODO: Solucionar el foco de las pestañas (El foco debe cambiarse a los entrys principales)
  gtk_widget_set_can_focus (GTK_WIDGET (builder_get (builder, "buy_notebook")), TRUE);
  gtk_widget_grab_focus (GTK_WIDGET (builder_get (builder, "entry_buy_barcode")));
  
  //Se oculta la pestaña "Ingreso Facturas" hasta que se termine la funcionalidad
  gtk_notebook_remove_page (GTK_NOTEBOOK (builder_get (builder, "buy_notebook")), 2);
  //Se oculta la opción para ingresar guias
  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "radiobutton8")), FALSE);

  //signals
  g_signal_connect (G_OBJECT (builder_get (builder, "btn_new_product")), "enter-notify-event",
		    G_CALLBACK (show_description), NULL);
  g_signal_connect (G_OBJECT (builder_get (builder, "btn_new_offer")), "enter-notify-event",
		    G_CALLBACK (show_description), NULL);
  g_signal_connect (G_OBJECT (builder_get (builder, "btn_new_mp")), "enter-notify-event",
  		    G_CALLBACK (show_description), NULL);
  g_signal_connect (G_OBJECT (builder_get (builder, "btn_make_service")), "enter-notify-event",
  		    G_CALLBACK (show_description), NULL);

  g_signal_connect (G_OBJECT (builder_get (builder, "btn_new_product")), "leave-notify-event",
		    G_CALLBACK (show_default), NULL);
  g_signal_connect (G_OBJECT (builder_get (builder, "btn_new_offer")), "leave-notify-event",
		    G_CALLBACK (show_default), NULL);
  g_signal_connect (G_OBJECT (builder_get (builder, "btn_new_mp")), "leave-notify-event",
  		    G_CALLBACK (show_default), NULL);
  g_signal_connect (G_OBJECT (builder_get (builder, "btn_make_service")), "leave-notify-event",
  		    G_CALLBACK (show_default), NULL);


  // Conectando la señal 'insert-text' para calcular diferencia con el monto total
  g_signal_connect (G_OBJECT (builder_get (builder, "entry_int_total_invoice")), "changed",
		    G_CALLBACK (calculate_shipping_amount), NULL);

  //TODO: Que se use esta funcion para todas las ventanas
  only_number_filer_on_container (GTK_CONTAINER (builder_get (builder, "wnd_asoc_comp")));
  only_number_filer_on_container (GTK_CONTAINER (builder_get (builder, "wnd_ingress_invoice")));
  gtk_widget_show_all (compras_gui);

  poblar_pedido_temporal ();

} // compras_win (void)


void
SearchProductHistory (GtkEntry *entry, gchar *barcode)
{
  gdouble day_to_sell;
  PGresult *res;
  gchar *q;
  gchar *codigo;
  gchar *tipo, *derivada, *materia_prima, *corriente, *compuesta;
  gboolean modo_traspaso, modo_inventario;

  //ID, tipos de mercaderías
  corriente = g_strdup (PQvaluebycol (EjecutarSQL ("SELECT id FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'CORRIENTE'"), 0, "id"));
  derivada = g_strdup (PQvaluebycol (EjecutarSQL ("SELECT id FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'DERIVADA'"), 0, "id"));
  compuesta = g_strdup (PQvaluebycol (EjecutarSQL ("SELECT id FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'COMPUESTA'"), 0, "id"));
  materia_prima = g_strdup (PQvaluebycol (EjecutarSQL ("SELECT id FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'MATERIA PRIMA'"), 0, "id"));

  modo_traspaso = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (builder_get (builder, "rdbutton_transfer_mode")));
  modo_inventario = rizoma_get_value_boolean ("MODO_INVENTARIO");

  codigo = barcode;
  q = g_strdup_printf ("SELECT barcode "
		       "FROM codigo_corto_to_barcode('%s')",
		       barcode);
  res = EjecutarSQL(q);

  /*Si entra aqui significa que se ingresó un codigo corto, y se pasa a codigo de barra*/
  if (PQntuples (res) == 1)
    {
      barcode = g_strdup (PQvaluebycol(res, 0, "barcode"));
      PQclear (res);
      gtk_entry_set_text (GTK_ENTRY (entry),barcode);
    }

  g_free(q);
  q = g_strdup_printf ("SELECT existe_producto(%s)", barcode);

  if (g_str_equal(GetDataByOne (q), "t"))
    {
      g_free(q);
      q = g_strdup_printf ("SELECT estado, tipo, barcode FROM producto where barcode = '%s'", barcode);
      res = EjecutarSQL(q);
      tipo = g_strdup (PQvaluebycol (res, 0, "tipo"));
      tipo_producto = atoi (tipo);

      // Con esto se asegura que sea realmente el barcode (para evitar los ceros anterior al barcode)
      gtk_entry_set_text (GTK_ENTRY (builder_get (builder, "entry_buy_barcode")),
			  g_strdup (PQvaluebycol (res, 0, "barcode")));

      /*Para asegurarse de que el producto no haya sido inhabilitado (borrado) por el administrador*/
      if (strcmp (PQvaluebycol (res, 0, "estado"),"f") == 0)
	{
	  GtkWidget *aux_widget;
	  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "compras_gui"));
	  gchar *str = g_strdup_printf("El código %s no está disponible, fué invalidado por el administrador", codigo);
	  AlertMSG (aux_widget, str);

	  //Limipia todo antes de irse
	  gtk_entry_set_text (GTK_ENTRY (builder_get (builder, "entry_buy_barcode")), "");
	  g_free (str);
	  g_free (q);
	  g_free (codigo);
	  PQclear (res);
	  return;
	}

      ShowProductHistory (barcode);

      q = g_strdup_printf ("select * from informacion_producto (%s, '')", barcode);
      res = EjecutarSQL (q);
      g_free (q);

      gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_stock")),
                            g_strdup_printf ("<span weight=\"ultrabold\">%.2f</span>", strtod (PUT(PQvaluebycol (res, 0, "stock")), (char **)NULL)));

      day_to_sell = atoi (PQvaluebycol (res, 0, "stock_day"));

      if (day_to_sell != 0)
        {
          gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_stock_further")),
                                g_strdup_printf ("<span weight=\"ultrabold\">%.2f dias"
                                                 "</span>", day_to_sell));
        }
      else
        {
          gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_stock_further")),
                                g_strdup_printf ("<span weight=\"ultrabold\">indefinidos dias</span>"));
        }

      gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_sell_price")),
                            g_strdup_printf ("<span weight=\"ultrabold\">%s/%s</span>",
                                             PutPoints (PQvaluebycol (res, 0, "precio")), PutPoints (PQvaluebycol (res, 0, "precio_mayor"))));

      gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_product_desc")),
                            g_strdup_printf ("<span weight=\"ultrabold\">%s</span>", PQvaluebycol (res, 0, "descripcion")));

      gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_mark")),
                            g_strdup_printf ("<span weight=\"ultrabold\">%s</span>", PQvaluebycol (res, 0, "marca")));

      gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_cont")),
                            g_strdup_printf ("<span weight=\"ultrabold\">%s</span>", PUT (PQvaluebycol (res, 0, "contenido"))));

      gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_unit")),
                            g_strdup_printf ("<span weight=\"ultrabold\">%s</span>", PQvaluebycol (res, 0, "unidad")));

      gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_fifo")),
                            g_strdup_printf ("<span weight=\"ultrabold\">%s</span>",
					     PUT (PQvaluebycol (res, 0, "costo_promedio"))));

      gtk_label_set_markup (GTK_LABEL (builder_get (builder, "label_code")),
                            g_strdup_printf ("<span weight=\"ultrabold\">%s</span>",PQvaluebycol (res, 0, "codigo_corto")));

      //Enable entry sensitive (all when is discret merchancy, some when is raw material)
      //TODO: Buscar la forma de simplificar los sensitive
      
      gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "button_new_product")), FALSE); //OJO
      gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "entry_buy_barcode")), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "edit_product_button")), TRUE); //OJO

      // Modo inventario
      if (modo_inventario == TRUE)
	{
	  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "entry_buy_price")), FALSE);
	  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "entry_buy_gain")), TRUE);
	  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "entry_sell_price")), TRUE);
	  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "button_calculate")), TRUE);
	      
	  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "entry_buy_amount")), FALSE);

	  if (g_str_equal (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_buy_gain"))),""))
	    gtk_widget_grab_focus (GTK_WIDGET (builder_get (builder, "entry_buy_gain")));
	  else
	    gtk_widget_grab_focus (GTK_WIDGET (builder_get (builder, "entry_sell_price")));

	  return;
	}

      // Acciones por tipo de mercaderia (y modo traspaso = FALSE)
      if (g_str_equal (tipo, materia_prima) && modo_traspaso == FALSE)
	{
	  //gtk_entry_set_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_buy_gain")), "0");
	  //gtk_entry_set_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_sell_price")), "0");	  
	  gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "entry_buy_price")), TRUE);	  
	  gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "button_calculate")), TRUE);
	  gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "rdbutton_transfer_mode")), FALSE);
	  
	  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "cmbPrecioCompra")), TRUE);
	  gtk_widget_grab_focus (GTK_WIDGET (builder_get (builder, "entry_buy_price")));
	}
      else if (g_str_equal (tipo, corriente) && modo_traspaso == FALSE)
	{
	  gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "entry_buy_price")), TRUE);
	  gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "entry_buy_gain")), TRUE);
	  gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "entry_sell_price")), TRUE);
	  gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "button_calculate")), TRUE);
	  gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "rdbutton_transfer_mode")), FALSE);

	  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "cmbPrecioCompra")), TRUE);
	  gtk_widget_grab_focus (GTK_WIDGET (builder_get (builder, "entry_buy_price")));
	}
      else if (g_str_equal (tipo, compuesta) || g_str_equal (tipo, derivada))
	{
	  gtk_entry_set_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_buy_price")), PUT (PQvaluebycol (res, 0, "costo_promedio")));
	  gtk_entry_set_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_sell_price")), PUT (PQvaluebycol (res, 0, "precio")));
	  
	  if (modo_traspaso == FALSE)
	    {
	      gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "entry_buy_price")), FALSE);
	      gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "entry_buy_gain")), FALSE);
	      gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "entry_sell_price")), FALSE);
	      gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "button_calculate")), FALSE);
	      gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "rdbutton_transfer_mode")), FALSE);
	      gtk_widget_grab_focus (GTK_WIDGET (builder_get (builder, "edit_product_button")));
	    }
	}
      
      // Modo traspaso = TRUE
      if (modo_traspaso == TRUE)
	{
	  gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "rdbutton_buy_mode")), FALSE);

	  if (!g_str_equal (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_buy_price"))), ""))
	    {
	      gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "entry_buy_price")), FALSE);
	      gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "entry_buy_gain")), FALSE);
	      gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "entry_sell_price")), FALSE);
	      gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "button_calculate")), FALSE);
	      gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "entry_buy_amount")), TRUE);
	      
	      gtk_widget_grab_focus (GTK_WIDGET (builder_get (builder, "entry_buy_amount")));
	      gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "button_add_product_list")), TRUE);
	    }
	  else if (g_str_equal (tipo, corriente) || g_str_equal (tipo, materia_prima))
	    {
	      gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "entry_buy_price")), TRUE);
	      gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "entry_buy_gain")), TRUE);
	      gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "entry_sell_price")), TRUE);
	      gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "button_calculate")), TRUE);
	      
	      gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "entry_buy_amount")), FALSE);

	      gtk_widget_grab_focus (GTK_WIDGET (builder_get (builder, "entry_buy_price")));
	    }
	  else // Si de casualidad se selecciona una MC o MDer sin costo en modo traspaso
	    {
	      gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "entry_buy_price")), FALSE);
	      gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "entry_buy_gain")), FALSE);
	      gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "entry_sell_price")), FALSE);
	      gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "button_calculate")), FALSE);
	      gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "entry_buy_amount")), FALSE);

	      //gtk_widget_grab_focus (GTK_WIDGET (builder_get (builder, "button_clear")));
	    }
	}

      gtk_widget_grab_focus (GTK_WIDGET (builder_get (builder, "entry_buy_price")));
    } // if (g_str_equal( GetDataByOne (q), "t"))
  else
    {
      gtk_widget_grab_focus (GTK_WIDGET (builder_get (builder, "button_new_product")));
    }
} // SearchProductHistory (GtkEntry *entry, gchar *barcode)


/**
 * Callback connected to the button of the dialog used to modificate a
 * product in the 'Mercaderia' tab
 *
 * @param widget the widget that emited the signal
 * @param data the user data
 */
void
Save (GtkWidget *widget, gpointer data)
{
  GtkWidget *aux_widget;
  GtkTreeModel *model;
  GtkTreeIter iter;

  GtkWidget *combo_imp;
  gchar *barcode;
  gchar *codigo;
  gchar *description;
  gchar *marca;
  gchar *unidad;
  gchar *contenido;
  gchar *precio;
  gint otros;
  gint familia = 0;
  gboolean iva;
  gboolean fraccion;
  gboolean perecible;

  GtkComboBox *combo_unit = GTK_COMBO_BOX (builder_get (builder, "cmb_box_edit_product_unit"));
  GtkComboBox *combo_family = GTK_COMBO_BOX (builder_get (builder, "cmb_edit_product_family"));
  gint tab = gtk_notebook_get_current_page ( GTK_NOTEBOOK (builder_get (builder, "buy_notebook")));

  aux_widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_edit_prod_barcode"));
  barcode = g_strdup (gtk_entry_get_text (GTK_ENTRY (aux_widget)));

  aux_widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_edit_prod_shortcode"));
  codigo = g_strdup (gtk_entry_get_text (GTK_ENTRY (aux_widget)));

  aux_widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_edit_prod_desc"));
  description = g_strdup (gtk_entry_get_text (GTK_ENTRY (aux_widget)));

  aux_widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_edit_prod_brand"));
  marca = g_strdup (gtk_entry_get_text (GTK_ENTRY (aux_widget)));

  /* aux_widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_edit_prod_unit")); */
  /* unidad = g_strdup (gtk_entry_get_text (GTK_ENTRY (aux_widget))); */

  aux_widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_edit_prod_content"));
  contenido = g_strdup (gtk_entry_get_text (GTK_ENTRY (aux_widget)));

  aux_widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_edit_prod_price"));
  precio = g_strdup (gtk_entry_get_text (GTK_ENTRY (aux_widget)));

  aux_widget = GTK_WIDGET (gtk_builder_get_object(builder, "checkbtn_edit_prod_fraccionaria"));
  fraccion = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(aux_widget));

  aux_widget = GTK_WIDGET (gtk_builder_get_object(builder, "checkbtn_edit_prod_perecible"));
  perecible = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(aux_widget));

  aux_widget = GTK_WIDGET (gtk_builder_get_object(builder, "checkbtn_edit_prod_iva"));
  iva = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(aux_widget));

  combo_imp = GTK_WIDGET (gtk_builder_get_object(builder, "cmbbox_edit_prod_extratax"));

  if (gtk_combo_box_get_active (GTK_COMBO_BOX (combo_imp)) != -1)
    {
      model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo_imp));
      gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combo_imp), &iter);

      gtk_tree_model_get (model, &iter,
                          0, &otros,
                          -1);
    }
  else
    otros = -1;


  /* aux_widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_edit_prod_unit")); */
  /* unidad = g_strdup (gtk_entry_get_text (GTK_ENTRY (aux_widget))); */

  if (gtk_combo_box_get_active (GTK_COMBO_BOX (combo_unit)) != -1)
    {
      model = gtk_combo_box_get_model (combo_unit);
      gtk_combo_box_get_active_iter (combo_unit, &iter);

      gtk_tree_model_get (model, &iter,
			  1, &unidad,
			  -1);
    }
  else
    {
      ErrorMSG (GTK_WIDGET (combo_unit), "Seleccione una unidad");
      return;
    }


  /*Familia*/
  if (gtk_combo_box_get_active (GTK_COMBO_BOX (combo_family)) != -1)
    {
      model = gtk_combo_box_get_model (combo_family);
      gtk_combo_box_get_active_iter (combo_family, &iter);

      gtk_tree_model_get (model, &iter,
			  0, &familia,
			  -1);
    }
  else
    {
      ErrorMSG (GTK_WIDGET (combo_family), "Seleccione una unidad");
      return;
    }


  // TODO: Revisar el Otros, porque en la base de datos se guardan 0
  // TODO: Una vez que el entry solo pueda recibir valores numéricos se puede borrar esta condición
  if (!is_numeric (contenido))
    {
      ErrorMSG (GTK_WIDGET (builder_get (builder, "entry_edit_prod_content")),"Debe ingresar un valor numérico");
      return;
    }

  SaveModifications (codigo, description, marca, unidad, contenido, precio,
                     iva, otros, barcode, perecible, fraccion, familia);

  if (tab == 0)
    {
      SearchProductHistory (GTK_ENTRY (gtk_builder_get_object (builder, "entry_buy_barcode")), codigo);
    }
  else if (tab == 3)
    {
      FillFields (NULL, NULL);
    }
  gtk_widget_hide (GTK_WIDGET (gtk_builder_get_object (builder, "wnd_mod_product")));
} //void Save (GtkWidget *widget, gpointer data)


/**
 * Es llamada por las funciones "AddToProductsList()" y "on_button_calculate_clicked()"
 *
 * Esta funcion se encarga de calcular la cifra restante entre: porcentaje de ganancia,
 * el precio de compra, tomando en cuenta si es bruto o neto y calculando sus impuestos
 * en el caso correspondiente y el precio final.
 *
 */
void
CalcularPrecioFinal (void)
{
  gchar *barcode = g_strdup (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_buy_barcode"))));
  gdouble ingresa = strtod (PUT (g_strdup (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_buy_price"))))), (char **)NULL);
  gdouble ganancia = strtod (PUT (g_strdup (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_buy_gain"))))), (char **)NULL);
  gdouble precio_final = strtod (PUT (g_strdup (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_sell_price"))))), (char **)NULL);
  gdouble precio;
  gdouble porcentaje;
  gdouble iva = GetIVA (barcode);
  gdouble otros = GetOtros (barcode);

  /* Obtener valores cmbPrecioCompra */
  GtkComboBox *combo = (GtkComboBox *) gtk_builder_get_object (builder, "cmbPrecioCompra");
  GtkTreeIter iter;
  GtkTreeModel *model = gtk_combo_box_get_model (combo);
  gtk_combo_box_get_active_iter (combo, &iter);
  gchar *opcion;
  gtk_tree_model_get (model, &iter, 0, &opcion, -1);

  printf ("Opcion: %s\n", opcion);

  if (iva != 0)
    iva = (gdouble)iva / 100 + 1;

  // TODO: Revisión (Concenso de la variable)
  if (otros == 0)
    otros = 0;

  /*
    IVA = 1,19;
    Z = precio final
    Y = margen;
    X = Ingresa;

    Z     1
    X = ----- * ----
    Y+1    1,19
  */

  if ((ganancia == 0 && precio_final == 0 && ingresa != 0) ||
      (ganancia == 0 && precio_final == 0 && ingresa == 0) ||
      (ganancia != 0 && precio_final == 0 && ingresa == 0)  )
    {
      ErrorMSG (GTK_WIDGET (builder_get (builder, "entry_buy_price")),"Se requieren al menos 2 valores para efectuar el cálculo");
    }

  /* -- Initial price ("ingresa") is calculated here -- */
  else if (ingresa == 0 && ganancia >= 0 && precio_final != 0)
    {
      calcular = 1; // FLAG - Permite saber que se ha realizado un cálculo.

      //Si tiene solo IVA
      if (otros == 0 && iva != 0  && g_str_equal (opcion, "Costo neto"))
        precio = (gdouble) ((gdouble)(precio_final / iva) / (gdouble) (ganancia + 100)) * 100;

      //Si tiene ambos impuestos
      else if (iva != 0 && otros != 0 && g_str_equal (opcion, "Costo neto"))
        {
          iva = (gdouble) iva - 1;
          otros = (gdouble) otros / 100;

          precio = (gdouble) precio_final / (gdouble)(iva + otros + 1);
          precio = (gdouble) precio / (gdouble)(ganancia / 100 + 1);
        }

      //Si no tiene impuestos
      else if ((iva == 0 && otros == 0) || g_str_equal (opcion, "Costo bruto"))
	precio = (gdouble) (precio_final / (gdouble) (ganancia + 100)) * 100;

      gtk_entry_set_text (GTK_ENTRY (builder_get (builder, "entry_buy_price")),
                          g_strdup_printf ("%ld", lround (precio)));
    }

  /* -- Profit porcent ("ganancia") is calculated here -- */
  else if (ganancia == 0 && ingresa != 0 && precio_final != 0)
    {
      calcular = 1; // FLAG - Permite saber que se ha realizado un cálculo.

      //Si solo tiene IVA
      if (otros == 0 && iva != 0 && g_str_equal (opcion, "Costo neto"))
        porcentaje = (gdouble) ((precio_final / (gdouble)(iva * ingresa)) -1) * 100;

      //Si tiene ambos impuestos
      else if (iva != 0 && otros != 0 && g_str_equal (opcion, "Costo neto"))
        {
          iva = (gdouble) iva - 1;
          otros = (gdouble) otros / 100;

          precio = (gdouble) precio_final / (gdouble)(iva + otros + 1);
          ganancia = (gdouble) precio - ingresa;
          porcentaje = (gdouble)(ganancia / ingresa) * 100;
        }

      //Si no tiene impuestos
      else if ((iva == 0 && otros == 0) || g_str_equal (opcion, "Costo bruto"))
        porcentaje = (gdouble) ((precio_final / ingresa) - 1) * 100;

      gtk_entry_set_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_buy_gain")),
                          g_strdup_printf ("%ld", lround (porcentaje)));
    }

  /* -- Final price ("precio_final") is calculated here -- */
  else if (precio_final == 0 && ingresa != 0 && ganancia >= 0)
    {
      calcular = 1; // FLAG - Permite saber que se ha realizado un cálculo.

      //Si solo tiene IVA
      if (otros == 0 && iva != 0 && g_str_equal (opcion, "Costo neto"))
        precio = (gdouble) ((gdouble)(ingresa * (gdouble)(ganancia + 100)) * iva) / 100;

      //Si tiene ambos impuestos
      else if (iva != 0 && otros != 0 && g_str_equal (opcion, "Costo neto"))
        {
          iva = (gdouble) iva - 1;
          otros = (gdouble) otros / 100;

          precio = (gdouble) ingresa + (gdouble)((gdouble)(ingresa * ganancia ) / 100);
          precio = (gdouble)((gdouble)(precio * iva) +
                             (gdouble)(precio * otros) + (gdouble) precio);
        }

      //Si no tiene impuestos
      else if ((iva == 0 && otros == 0) || g_str_equal (opcion, "Costo bruto"))
        precio = (gdouble)(ingresa * (gdouble)(ganancia + 100)) / 100;

      if (ganancia == 0)
        gtk_entry_set_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_buy_gain")), "0");

      gtk_entry_set_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_sell_price")),
                          g_strdup_printf ("%.2f", precio));
    }
  else
    ErrorMSG (GTK_WIDGET (builder_get (builder, "entry_buy_price")), "Solo 2 campos deben ser llenados");

  if(calcular == 1)
    {
      //Inhabilitar la escritura en los entry de precio cuando el cálculo de ha realizado
      //TODO: Ver como simplificar los sensitive
      gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "entry_buy_price")), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "entry_buy_gain")), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "entry_sell_price")), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "button_calculate")), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "cmbPrecioCompra")), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "entry_buy_amount")), TRUE);
      gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "button_add_product_list")), TRUE);
    }
} // CalcularPrecioFinal (void)


void
AddToTree (void)
{
  GtkTreeIter iter;
  GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (gtk_builder_get_object( builder, "tree_view_products_buy_list"))));
  gboolean enviable;

  enviable = (compra->current->stock < compra->current->cantidad) ? FALSE : TRUE;

  gtk_list_store_insert_after (store, &iter, NULL);
  gtk_list_store_set (store, &iter,
		      0, compra->current->barcode,
                      1, compra->current->codigo,
                      2, g_strdup_printf ("%s %s %d %s", compra->current->producto,
                                          compra->current->marca, compra->current->contenido,
                                          compra->current->unidad),
                      3, compra->current->cantidad,
		      4, compra->current->precio,
                      5, compra->current->precio_compra,
                      6, lround ((gdouble) compra->current->cantidad * compra->current->precio_compra),
		      7, enviable,
                      -1);

  compra->current->iter = iter;

  if (enviable == FALSE)
    gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_traspaso_enviar")), FALSE);
}


void
add_to_buy_struct (gchar *barcode, gdouble cantidad, gdouble precio_compra, gdouble margen, gdouble precio)
{
  GtkListStore *store_buy;
  store_buy = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (builder_get (builder, "tree_view_products_buy_list"))));

  Producto *check;

  if (compra->header_compra != NULL)
    check = SearchProductByBarcode (barcode, FALSE);
  else
    check = NULL;

  if (check == NULL)
    {
      if (CompraAgregarALista (barcode, cantidad, precio, precio_compra, margen, FALSE))
	{
	  AddToTree ();
	}
      else
	{
	  return;
	}
    }
  else
    {
      //No se permite ingresar 2 costos o precios distintos al mismo producto en una misma instancia de compra
      if (check->precio_compra != precio_compra)
	{
	  CleanStatusProduct (1);
	  ErrorMSG (GTK_WIDGET (builder_get (builder, "entry_buy_price")), 
		    g_strdup_printf ("Ya ha ingresado este producto con $ %.3f de costo neto, si desea modificarlo \n"
				     "remuévalo de la lista e ingréselo nuevamente con sus valores correspondientes.", check->precio_compra));
	  return;
	}
      else if (check->precio != precio)
	{
	  CleanStatusProduct (1);
	  ErrorMSG (GTK_WIDGET (builder_get (builder, "entry_sell_price")), 
		    g_strdup_printf ("Ya ha ingresado este producto con $ %s como precio de venta, si desea modificarlo \n"
				     "remuévalo de la lista e ingréselo nuevamente con sus valores correspondientes.", PutPoints (g_strdup_printf ("%.2f", check->precio))));
	  return;
	}
	  
      check->cantidad += cantidad;

      gtk_list_store_set (store_buy, &check->iter,
			  3, check->cantidad,
			  6, lround ((gdouble)check->cantidad * check->precio_compra),
			  -1);

      if (check->stock < check->cantidad)
	gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_traspaso_enviar")), FALSE);
    }

  gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_total_buy")),
			g_strdup_printf ("<span size=\"xx-large\"><b>%s</b></span>",
					 PutPoints (g_strdup_printf
						    ("%li", lround (CalcularTotalCompra (compra->header_compra))))));
}


void 
poblar_pedido_temporal (void)
{
  PGresult *res;
  gint i, tuples;

  gchar *barcode; 
  gdouble cantidad, precio_compra; 
  gint margen, precio;

  res = get_pedido_temporal ();
  tuples = PQntuples (res);

  for (i=0; i<tuples; i++)
    {
      barcode = g_strdup (PQvaluebycol (res, i, "barcode"));
      cantidad = strtod (PUT (g_strdup (PQvaluebycol (res, i, "cantidad"))), (char **)NULL);
      precio_compra = strtod (PUT (g_strdup (PQvaluebycol (res, i, "costo_promedio"))), (char **)NULL);
      margen = atoi (g_strdup (PQvaluebycol (res, i, "margen")));
      precio = atoi (g_strdup (PQvaluebycol (res, i, "precio")));

      add_to_buy_struct (barcode, cantidad, precio_compra, margen, precio);
    }

  if (tuples > 0)
    gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "button_buy")), TRUE);
}

void
AddToProductsList (void)
{
  gchar *barcode = g_strdup (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_buy_barcode"))));
  gdouble cantidad;
  gdouble precio_compra = strtod (PUT(g_strdup (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_buy_price"))))), (char **)NULL);
  gdouble margen = strtod (PUT(g_strdup (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_buy_gain"))))), (char **)NULL);
  gdouble precio = strtod (PUT(g_strdup (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_sell_price"))))), (char **)NULL);

  gboolean modo_traspaso = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (builder_get (builder, "rdbutton_transfer_mode")));

  /*Se obtiene el tipo de la mercadería seleccionada*/
  PGresult *res = get_product_information (barcode, "", "estado, tipo, stock");
  
  gchar *materia_prima = g_strdup (PQvaluebycol 
				   (EjecutarSQL ("SELECT id FROM tipo_mercaderia "
						 "WHERE UPPER(nombre) LIKE 'MATERIA PRIMA'"), 0, "id"));

  gchar *tipo = g_strdup (PQvaluebycol (res, 0, "tipo"));
  //gdouble stock = strtod (PUT (g_strdup (PQvaluebycol (res, 0, "stock"))), (char **)NULL);
    
  /*Se regulariza el precio de compra de acuerdo a si es neto o bruto*/

  //Obtener valores cmbPrecioCompra
  GtkComboBox *combo = (GtkComboBox *) gtk_builder_get_object (builder, "cmbPrecioCompra");
  GtkTreeIter iter;
  GtkTreeModel *model = gtk_combo_box_get_model (combo);
  gtk_combo_box_get_active_iter (combo, &iter);
  gchar *opcion;
  gtk_tree_model_get (model, &iter, 0, &opcion, -1);

  //Se asegura de obtener el precio neto del producto como precio_compra
  if (g_str_equal (opcion, "Costo bruto"))
    {
      gdouble iva, otros, total_imp;
      iva = otros = total_imp = 0;
      iva = GetIVA (barcode);  
      otros = GetOtros (barcode);
      
      if (iva != 0)
	iva = (gdouble)iva/100;
      if (otros != 0)
	otros = (gdouble)otros/100;
     
      total_imp = iva + otros + 1;      
      precio_compra = precio_compra / total_imp;
    }

  if (g_str_equal (barcode, ""))
    {
      ErrorMSG (GTK_WIDGET (builder_get (builder, "entry_buy_barcode")),
		"Debe seleccionar una mercadería");
      return;
    }

  GtkListStore *store_history;

  cantidad = strtod (PUT (g_strdup (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_buy_amount"))))), (char **)NULL);

  // EN MODO TRASPASO
  if (modo_traspaso == TRUE)
    {
      GtkTreeView *treeview = GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_products_buy_list"));
      if (get_treeview_length(treeview) == 0)
	{
	  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_traspaso_recibir")), TRUE);
	  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_traspaso_enviar")), TRUE);
	}
    }

  if (precio_compra != 0 && (g_str_equal (tipo, materia_prima) == TRUE || // Si es materia prima no necesita precio de venta
			     (strcmp (GetCurrentPrice (barcode), "0") == 0 || precio != 0))
      && strcmp (barcode, "") != 0) //&& margen >= 0)
    {
      add_to_buy_struct (barcode, cantidad, precio_compra, margen, precio);
      add_to_pedido_temporal (barcode, cantidad, precio_compra, margen, precio);
      
      store_history = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (gtk_builder_get_object( builder, "product_history_tree_view"))));
      gtk_list_store_clear (store_history);

      CleanStatusProduct (0);

      //TODO: Se setea el sensitive cada vez que se agrega un producto a la lista, se debe evaluar
      if (modo_traspaso == TRUE)
	gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "button_buy")), FALSE);
      else
	gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "button_buy")), TRUE);

      gtk_widget_grab_focus (GTK_WIDGET (gtk_builder_get_object (builder, "entry_buy_barcode")));
    }
  else
    {
      ErrorMSG (GTK_WIDGET (builder_get (builder, "entry_buy_price")), 
		g_strdup_printf ("El producto debe tener costo y precio para ser agregado a la lista de %s", 
				 (modo_traspaso) ? "traspasos":"compras"));
      //CalcularPrecioFinal ();
      //AddToProductsList (); esto produciría un loop infinito
    }
} // void AddToProductsList (void)


void
MoveCursor (GtkTreeView *treeview, GtkTreePath *arg1, GtkTreeViewColumn *arg2,
            gpointer data)
{
  GtkWidget *window = (GtkWidget *) data;

  gtk_window_set_focus (GTK_WINDOW (window), compra->nota_entry);
}

void
CloseBuyWindow (void)
{
  gtk_widget_destroy (compra->buy_window);
  compra->buy_window = NULL;
}


void
BuyWindow (void)
{
  /*  PGresult *res;
      gint tuples, i;
  */
  
  /*
  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *label;
  GtkWidget *button;
  GtkWidget *button_add;

  GtkWidget *scroll;
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;
  GtkTreeSelection *selection;
  GtkTreeIter iter;

  gtk_widget_set_sensitive (main_window, FALSE);

  compra->buy_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_widget_set_size_request (compra->buy_window, 300, -1);
  gtk_window_set_resizable (GTK_WINDOW (compra->buy_window), FALSE);
  gtk_window_set_position (GTK_WINDOW (compra->buy_window), GTK_WIN_POS_CENTER_ALWAYS);
  gtk_window_set_title (GTK_WINDOW (compra->buy_window), "Datos Proveedor");
  //  gtk_window_set_transient_for (GTK_WINDOW (compra->buy_window), GTK_WINDOW (main_window));
  gtk_widget_show (compra->buy_window);
  gtk_window_present (GTK_WINDOW (compra->buy_window));

  g_signal_connect (G_OBJECT (compra->buy_window), "destroy",
                    G_CALLBACK (CloseBuyWindow), NULL);

  vbox = gtk_vbox_new (FALSE, 3);
  gtk_widget_show (vbox);
  gtk_container_add (GTK_CONTAINER (compra->buy_window), vbox);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  label = gtk_label_new ("Rut Proveedor    :");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);

  compra->rut_label = gtk_label_new ("");
  gtk_widget_show (compra->rut_label);
  gtk_box_pack_start (GTK_BOX (hbox), compra->rut_label, FALSE, FALSE, 3);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  label = gtk_label_new ("Nombre Proveedor :");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);

  compra->nombre_label = gtk_label_new ("");
  gtk_widget_show (compra->nombre_label);
  gtk_box_pack_start (GTK_BOX (hbox),compra->nombre_label, FALSE, FALSE, 3);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  label = gtk_label_new ("Nota Pedido");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);

  compra->nota_entry = gtk_entry_new ();
  gtk_widget_show (compra->nota_entry);
  gtk_box_pack_end (GTK_BOX (hbox), compra->nota_entry, FALSE, FALSE, 3);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  button_add = gtk_button_new_with_mnemonic ("Agregar _Proveedor");
  gtk_widget_show (button_add);
  gtk_box_pack_end (GTK_BOX (hbox), button_add, FALSE, FALSE, 3);

  g_signal_connect (G_OBJECT (button_add), "clicked",
                    G_CALLBACK (AddProveedorWindow), NULL);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  label = gtk_label_new ("Plazo de pago: ");
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
  gtk_widget_show (label);

  entry_plazo = gtk_entry_new_with_max_length (3);
  gtk_box_pack_start (GTK_BOX (hbox), entry_plazo, FALSE, FALSE, 3);
  gtk_widget_show (entry_plazo);

  g_signal_connect (G_OBJECT (compra->nota_entry), "activate",
                    G_CALLBACK (SendCursorTo), (gpointer) entry_plazo);

  label = gtk_label_new (" dias.");
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
  gtk_widget_show (label);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_end (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  button = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
  gtk_widget_show (button);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 3);

  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (CloseBuyWindow), NULL);

  button = gtk_button_new_from_stock (GTK_STOCK_OK);
  gtk_widget_show (button);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);

  g_signal_connect (G_OBJECT (entry_plazo), "activate",
                    G_CALLBACK (SendCursorTo), (gpointer)button);

  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (Comprar), NULL);

  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_show (scroll);
  gtk_widget_set_size_request (scroll, 280, 200);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (vbox), scroll, FALSE, FALSE, 3);*/

  GtkTreeView *treeview;
  GtkListStore *store;
  GtkTreeSelection *selection;
  GtkTreeIter iter;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  treeview = GTK_TREE_VIEW (builder_get (builder, "treeview_prov"));
  if (gtk_tree_view_get_model (treeview) == NULL)
    {
      store = gtk_list_store_new (2,
				  G_TYPE_STRING,
				  G_TYPE_STRING);
  
      gtk_tree_view_set_model (treeview, GTK_TREE_MODEL (store));

      selection = gtk_tree_view_get_selection (treeview);
      gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);

      g_signal_connect (G_OBJECT (selection), "changed",
			G_CALLBACK (Seleccionado), NULL);

      /* Definido desde glade
	 g_signal_connect (G_OBJECT (treeview), "row-activated",
	 G_CALLBACK (gtk_widget_grab_focus), (gpointer) builder_get (builder, "entry_nota_pedido"));*/

      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes ("Rut", renderer,
							 "text", 0,
							 NULL);
      gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
      //gtk_tree_view_column_set_min_width (column, 75);
      //gtk_tree_view_column_set_max_width (column, 75);
      gtk_tree_view_column_set_resizable (column, FALSE);
      gtk_tree_view_column_set_cell_data_func (column, renderer, control_rut, (gpointer)0, NULL);

      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes ("Proveedor", renderer,
							 "text", 1,
							 NULL);
      gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
      gtk_tree_view_column_set_min_width (column, 200);
      gtk_tree_view_column_set_max_width (column, 200);
      gtk_tree_view_column_set_resizable (column, FALSE);
    }
  else
    {
      store = GTK_LIST_STORE (gtk_tree_view_get_model (treeview));
    }

  selection = gtk_tree_view_get_selection (treeview);

  clean_container (GTK_CONTAINER (builder_get (builder, "wnd_provider_data")));
  FillProveedores ();

  gtk_widget_show (GTK_WIDGET (builder_get (builder, "wnd_provider_data")));

  if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (store), &iter) == TRUE)
    {      
      //gtk_widget_grab_focus (GTK_WIDGET (treeview));
      gtk_tree_selection_select_path (selection, gtk_tree_path_new_from_string ("0"));
      gtk_window_set_focus (GTK_WINDOW (builder_get (builder, "wnd_provider_data")), GTK_WIDGET (treeview));      
    }    
  else
    gtk_widget_grab_focus (GTK_WIDGET (builder_get (builder, "btn_add_provider")));  

  //gtk_window_set_focus (GTK_WINDOW (compra->buy_window), treeview);
} // void BuyWindow (void)


/**
 * This function is a callback from
 * 'btn_der_mp_yes' button (signal click)
 *
 * Show 'wnd_comp_deriv' window
 *
 * @param button the button
 * @param user_data the user data
 */
void
on_btn_der_mp_yes_clicked (GtkButton *button, gpointer user_data)
{
  gchar *q, *barcode;//, *costo;
  PGresult *res;
  gint tuples, i;

  GtkTreeView *treeview;
  GtkListStore *store;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkTreeIter iter;

  gtk_widget_hide (GTK_WIDGET (builder_get (builder, "wnd_derivar_mp")));

  /*Se obtienen los datos de la materia prima*/
  barcode = g_strdup (gtk_label_get_text (GTK_LABEL (builder_get (builder, "lbl_barcode_mp"))));
  //costo = g_strdup (gtk_label_get_text (GTK_LABEL (builder_get (builder, "lbl_mother_price"))));
  
  res = EjecutarSQL (g_strdup_printf ("SELECT codigo_corto, marca, descripcion "
				      "FROM producto WHERE barcode = %s", barcode));

  gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_descripcion_mp")), 
			g_strdup_printf ("<span font_weight='bold' size='12500'>%s %s</span>",
					 g_strdup (PQvaluebycol (res, 0, "descripcion")),
					 g_strdup (PQvaluebycol (res, 0, "marca")) ));

  gtk_label_set_text (GTK_LABEL (builder_get (builder, "lbl_codigo_mp")),
		      g_strdup (PQvaluebycol (res, 0, "codigo_corto")) );

  gtk_widget_grab_focus (GTK_WIDGET (builder_get (builder, "btn_make_deriv")));
  treeview = GTK_TREE_VIEW (gtk_builder_get_object (builder, "treeview_deriv"));
  if (gtk_tree_view_get_model (treeview) == NULL)
    {
      store = gtk_list_store_new (5,
				  G_TYPE_STRING,  //barcode
				  G_TYPE_STRING,  //Codigo Corto
				  G_TYPE_STRING,  //Marca Descripción
				  G_TYPE_INT,     //Precio
				  G_TYPE_DOUBLE); //Cantidad

      gtk_tree_view_set_model (GTK_TREE_VIEW (treeview), GTK_TREE_MODEL (store));

      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes ("Codigo", renderer,
							 "text", 1,
							 NULL);
      gtk_tree_view_append_column (treeview, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
      gtk_tree_view_column_set_resizable (column, FALSE);

      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes ("Descripcion", renderer,
							 "text", 2,
							 NULL);
      gtk_tree_view_append_column (treeview, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
      gtk_tree_view_column_set_resizable (column, FALSE);

      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes ("Precio", renderer,
							 "text", 3,
							 NULL);
      gtk_tree_view_append_column (treeview, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
      gtk_tree_view_column_set_resizable (column, FALSE);
      gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)3, NULL);

      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes ("Cant. Madre", renderer,
							 "text", 4,
							 NULL);
      gtk_tree_view_append_column (treeview, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
      gtk_tree_view_column_set_resizable (column, FALSE);
      gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)4, NULL);
    }

  /*Clean container debe ejecutarse al cerrar la ventana*/
  //clean_container (GTK_CONTAINER (builder_get (builder, "wnd_comp_deriv")));

  //Se limpia el treeview "deriv"
  //gtk_list_store_clear (store);

  //Agrega los productos derivados de mother (de tenerlo) TODO: Debe estar en postgres function
  q = g_strdup_printf ("SELECT cmc.cant_mud AS cantidad_mud, cmc.barcode_complejo AS barcode, "
		       "       prd.codigo_corto AS codigo, prd.marca, prd.descripcion, prd.precio "
		       "FROM componente_mc cmc "
		       "INNER JOIN producto AS prd "
		       "ON prd.barcode = cmc.barcode_complejo "
		       "WHERE cmc.barcode_componente = '%s' "
		       "AND (SELECT estado FROM producto WHERE barcode = cmc.barcode_complejo) = true",
		       barcode);
  res = EjecutarSQL(q);

  store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (gtk_builder_get_object (builder, "treeview_deriv"))));

  if (res != NULL)
    {
      tuples = PQntuples (res);
      for (i = 0; i < tuples; i++)
	{
	  gtk_list_store_append (store, &iter);
	  gtk_list_store_set (store, &iter, 
			      0, g_strdup (PQvaluebycol (res, i, "barcode")),
			      1, g_strdup (PQvaluebycol (res, i, "codigo")),
			      2, g_strdup_printf ("%s %s",
						  PQvaluebycol (res, i, "descripcion"),
						  PQvaluebycol (res, i, "marca")),
			      3, atoi (g_strdup (PQvaluebycol (res, i, "precio"))),
			      4, strtod (PUT (g_strdup (PQvaluebycol (res, i, "cantidad_mud"))), (char **)NULL),
			      -1);
	}
    }

  /*gtk_label_set_text (GTK_LABEL (builder_get (builder, "lbl_subtotal_mother_price")),
    g_strdup_printf ("%.3f",compra->products_compra->product->precio_compra));*/

  gtk_widget_show (GTK_WIDGET (builder_get (builder, "wnd_comp_deriv")));
}


/**
 * Añade el producto especificado a la lista de 
 * componentes "treeview_components".
 *
 * @param: product barcode
 */
void
add_to_comp_list (gchar *barcode)
{
  PGresult *res;
  GtkListStore *store;
  GtkTreeIter iter;
  GtkTreeIter iter_p;
  gdouble costo_total;
  gdouble cantidad_previa, costo;
  GtkTreeView *treeview = GTK_TREE_VIEW (gtk_builder_get_object (builder, "treeview_components"));

  res = get_product_information (barcode, "", "codigo_corto, marca, descripcion, costo_promedio, tipo");
  store = GTK_LIST_STORE (gtk_tree_view_get_model (treeview));
  if (res != NULL)
    {
      /* Averiguamos si la mercadería ya existe en el treeview */
      // De existir solamente aumentamos la cantidad en uno
      if (get_treeview_column_matched (treeview, 0, G_TYPE_STRING, (void *)barcode, &iter_p))
	{
	  gtk_tree_model_get (GTK_TREE_MODEL (store), &iter_p,
			      3, &costo, //costo unitario
			      4, &cantidad_previa,
			      -1);

	  gtk_list_store_set (store, &iter_p,
			      4, cantidad_previa + 1, //nueva cantidad
			      5, costo * (cantidad_previa + 1), //subtotal
			      -1);
	}
      else //De no existir de agrega a la lista
	{
	  gtk_list_store_append (store, &iter);
	  gtk_list_store_set (store, &iter, 
			      0, barcode,
			      1, g_strdup (PQvaluebycol (res, 0, "codigo_corto")),
			      2, g_strdup_printf ("%s %s",					      
						  PQvaluebycol (res, 0, "descripcion"),
						  PQvaluebycol (res, 0, "marca")),
			      3, strtod (PUT (g_strdup (PQvaluebycol (res, 0, "costo_promedio"))), (char **)NULL),
			      4, strtod (PUT (g_strdup ("1")), (char **)NULL),
			      5, strtod (PUT (g_strdup (PQvaluebycol (res, 0, "costo_promedio"))), (char **)NULL),
			      6, atoi (g_strdup (PQvaluebycol (res, 0, "tipo"))),
			      -1);
	}
    }

  costo_total = sum_treeview_column (treeview, 3, G_TYPE_DOUBLE);
  gtk_label_set_text (GTK_LABEL (builder_get (builder, "lbl_subtotal_costo_compuesto")),
		      g_strdup_printf ("%.2f", costo_total));

}



/**
 * Selecciona el producto elegido y lo muestra en la
 * ventana correspondiente (compra, compuestos)
 */
void
AddFoundProduct (void)
{
  GtkTreeView *treeview = GTK_TREE_VIEW (builder_get (builder, "treeview_buscador"));
  GtkTreeIter iter;
  GtkTreeModel *model = gtk_tree_view_get_model (treeview);
  gchar *barcode;
  gchar *codigo;

  if (gtk_tree_selection_get_selected (gtk_tree_view_get_selection (treeview), NULL, &iter))
    {
      gtk_tree_model_get (model, &iter,
                          0, &codigo,
			  1, &barcode,
                          -1);
      
      //Si se llamó desde la ventana de compra principal simplemente se selecciona el producto
      if (add_to_comp == FALSE)
	{
	  gtk_entry_set_text (GTK_ENTRY (builder_get (builder, "entry_buy_barcode")), barcode);
	  gtk_widget_hide (GTK_WIDGET (builder_get (builder, "wnd_buscador")));
	  SearchProductHistory (GTK_ENTRY (builder_get (builder, "entry_buy_barcode")), codigo);
	}
      //Si se llama desde la ventana de creación de compuestos, agrega el producto a la lista de compuestos
      else
	{
	  gtk_widget_hide (GTK_WIDGET (builder_get (builder, "wnd_buscador")));
	  add_to_comp_list (barcode);
	  
	  if (gtk_widget_get_sensitive (GTK_WIDGET (builder_get (builder, "btn_asoc_comp"))) == FALSE)
	    gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_asoc_comp")), TRUE);
	}
    }

  clean_container (GTK_CONTAINER (builder_get (builder, "wnd_buscador")));
}

/**
 * Callback from "btn_close_wnd_buscador" Button
 * (signal click) and "wnd_buscador" (signal delete-event).
 *
 * Close and clean the "wnd_buscador" window.
 *
 * @param: button
 * @param: user_data
 */
void
close_wnd_buscador (GtkButton *button, gpointer user_data)
{
  clean_container (GTK_CONTAINER (builder_get (builder, "wnd_buscador")));
  gtk_widget_hide (GTK_WIDGET (builder_get (builder, "wnd_buscador")));
}


/**
 * Callback from "entry_buscar" (signal-activate) and "btn_search_merchandise"
 *
 * Busca las mercaderías que coincidan con el patrón de búsqueda (ingresado en el "entry_buscar")
 * y rellena el treeview "treeview_buscador" de la ventana "wnd_buscador"
 */
void
SearchName (void)
{
  gchar *search_string = g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_buscar"))));
  gchar *q, *barcode;
  PGresult *res;
  gint i, resultado;
  //gchar *materia_prima;

  GtkTreeView *treeview = GTK_TREE_VIEW (gtk_builder_get_object (builder, "treeview_buscador"));
  GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model (treeview));
  GtkTreeIter iter;
  
  q = g_strdup_printf ("SELECT codigo_corto, barcode, descripcion, marca, "
                       "contenido, unidad, stock FROM buscar_productos('%s%%')",
                       search_string);

  if (add_to_comp == TRUE)
    {
      /* materia_prima = g_strdup (PQvaluebycol (EjecutarSQL ("SELECT id " */
      /* 							   "FROM tipo_mercaderia " */
      /* 							   "WHERE UPPER(nombre) LIKE 'MATERIA PRIMA'"), 0, "id")); */

      barcode = g_strdup (gtk_label_get_text (GTK_LABEL (builder_get (builder, "lbl_comp_barcode"))));
      //TODO: Y de ningun producto que lo tenga a él mismo como componente
      q = g_strdup_printf ("%s WHERE barcode != %s "			  
			   //"AND tipo != %s "
			   "AND costo_promedio > 0", q, barcode); //, materia_prima);
    }

  res = EjecutarSQL (q);
  g_free (q);

  resultado = PQntuples (res);

  gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_found")),
                        g_strdup_printf ("<b>%d producto(s)</b>", resultado));

  gtk_list_store_clear (store);

  for (i = 0; i < resultado; i++)
    {
      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
                          0, PQvaluebycol (res, i, "codigo_corto"),
                          1, PQvaluebycol (res, i, "barcode"),
                          2, PQvaluebycol (res, i, "descripcion"),
                          3, PQvaluebycol (res, i, "marca"),
                          4, PQvaluebycol (res, i, "contenido"),
                          5, PQvaluebycol (res, i, "unidad"),
                          6, atoi (PQvaluebycol (res, i, "stock")),
                          -1);
    }

  if (resultado > 0)
    {
      gtk_tree_selection_select_path (gtk_tree_view_get_selection (treeview), gtk_tree_path_new_from_string ("0"));
      gtk_widget_grab_focus (GTK_WIDGET (treeview));
    }
}


void
SearchByName (void)
{
  GtkWindow *window;

  GtkListStore *store;
  GtkTreeView *treeview;

  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  gchar *string = g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_buy_barcode"))));

  treeview = GTK_TREE_VIEW (gtk_builder_get_object (builder, "treeview_buscador"));

  if (gtk_tree_view_get_model (treeview) == NULL )
    {
      store = gtk_list_store_new (7,
                                  G_TYPE_STRING,
                                  G_TYPE_STRING,
                                  G_TYPE_STRING,
                                  G_TYPE_STRING,
                                  G_TYPE_STRING,
                                  G_TYPE_STRING,
                                  G_TYPE_INT);

      gtk_tree_view_set_model (treeview, GTK_TREE_MODEL (store));

      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes ("Codigo", renderer,
                                                         "text", 0,
                                                         NULL);
      gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      gtk_tree_view_column_set_resizable (column, FALSE);

      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes ("Codigo de Barras", renderer,
                                                         "text", 1,
                                                         NULL);
      gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      gtk_tree_view_column_set_resizable (column, FALSE);

      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes ("Descripción del Producto", renderer,
                                                         "text", 2,
                                                         NULL);
      gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      gtk_tree_view_column_set_min_width (column, 400);
      //      gtk_tree_view_column_set_max_width (column, 300);
      gtk_tree_view_column_set_resizable (column, FALSE);

      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes ("Marca", renderer,
                                                         "text", 3,
                                                         NULL);
      gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      gtk_tree_view_column_set_min_width (column, 100);
      gtk_tree_view_column_set_max_width (column, 100);
      gtk_tree_view_column_set_resizable (column, FALSE);

      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes ("Cant.", renderer,
                                                         "text", 4,
                                                         NULL);
      gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
      gtk_tree_view_column_set_resizable (column, FALSE);

      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes ("Unid.", renderer,
                                                         "text", 5,
                                                         NULL);
      gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
      gtk_tree_view_column_set_resizable (column, FALSE);

      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes ("Stock", renderer,
                                                         "text", 6,
                                                         NULL);
      gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
      gtk_tree_view_column_set_resizable (column, FALSE);
    }

  window = GTK_WINDOW (gtk_builder_get_object (builder, "wnd_buscador"));
  gtk_widget_show_all (GTK_WIDGET (window));
  gtk_widget_grab_focus (GTK_WIDGET (gtk_builder_get_object (builder, "entry_buscar")));

  if ( ! g_str_equal (string, ""))
    {
      gtk_entry_set_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_buscar")), string);
      SearchName ();
    }
} // void SearchByName (void)


void
Comprar (GtkWidget *widget, gpointer data)
{
  GtkWidget *widget_aux;
  widget_aux = GTK_WIDGET (builder_get (builder, "lbl_provider_rut"));
  gchar *rut = CutPoints (g_strdup (gtk_label_get_text (GTK_LABEL (widget_aux))));
  widget_aux = GTK_WIDGET (builder_get (builder, "entry_nota_pedido"));
  gchar *nota = g_strdup (gtk_entry_get_text (GTK_ENTRY (widget_aux)));
  widget_aux = GTK_WIDGET (builder_get (builder, "entry_pay_days"));
  gint dias_pago = atoi (g_strdup (gtk_entry_get_text (GTK_ENTRY (widget_aux))));

  if (g_str_equal (rut, ""))
    {
      ErrorMSG (GTK_WIDGET (builder_get (builder, "btn_add_proveedor")), "Debe Seleccionar un Proveedor");
    }
  else
    {
      rut = g_strndup (rut, strlen (rut)-2);

      AgregarCompra (rut, nota, dias_pago);

      //CloseBuyWindow ();
      gtk_widget_hide (GTK_WIDGET (builder_get (builder,"wnd_provider_data")));

      ClearAllCompraData ();
      clean_pedido_temporal ();

      // Es llamada por ClearAllCompraData
      //CleanStatusProduct (0);

      compra->header_compra = NULL;
      compra->products_compra = NULL;
    }

  //Se libera la variable global
  if (rut_proveedor_global != NULL)
    {
      g_free (rut_proveedor_global);
      rut_proveedor_global = NULL;
    }
}


void
ShowProductHistory (gchar *barcode)
{
  PGresult *res;
  GtkTreeIter iter;

  GtkListStore *store;

  //gchar *barcode = g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object( builder, "entry_buy_barcode" ))));
  gint i, tuples;
  gdouble precio = 0;
  gdouble cantidad = 0;
  gdouble cantidad_ingresada = 0;
  gdouble iva, otros;
  iva = otros = 0;

  res = EjecutarSQL
    (g_strdup_printf
     ("SELECT (SELECT nombre FROM proveedor WHERE rut=c.rut_proveedor) AS proveedor, "
      "       (SELECT precio FROM producto WHERE barcode = %s) AS precio_venta, "
      "       date_part('day', c.fecha) AS dia, date_part('month', c.fecha) AS mes, date_part('year', c.fecha) AS ano, "
      "       c.id, cd.iva, cd.otros_impuestos, cd.precio, cd.cantidad, cd.cantidad_ingresada "
      "FROM compra c INNER JOIN compra_detalle cd "
      "       ON c.id = cd.id_compra "
      "       INNER JOIN producto p "
      "       ON cd.barcode_product = p.barcode "
      "WHERE  c.anulada_pi = false "
      "       AND p.barcode = '%s' "
      "       AND (cd.cantidad_ingresada > 0 OR c.anulada = false) "
      "ORDER BY c.fecha DESC", barcode, barcode));

  tuples = PQntuples (res);

  store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (gtk_builder_get_object( builder, "product_history_tree_view"))));

  gtk_list_store_clear (store);

  for (i = 0; i < tuples; i++)
    {
      iva = strtod (PUT(PQvaluebycol (res, i, "iva")), (char **)NULL); //IVA total ($)
      otros = strtod (PUT(PQvaluebycol (res, i, "otros_impuestos")), (char **)NULL); //OTROS total ($)
      precio = strtod (PUT(PQvaluebycol (res, i, "precio")), (char **)NULL);

      if (iva > 0) //Si IVA no es 0 (en dinero)
	precio = precio + (iva / strtod (PUT(PQvaluebycol (res, i, "cantidad")), (char **)NULL));

      if (otros > 0) //Si otros_impuestos no es 0 (en dinero)
	precio = precio + (otros / strtod (PUT(PQvaluebycol (res, i, "cantidad")), (char **)NULL));

      cantidad = strtod (PUT(PQvaluebycol (res, i, "cantidad")), (char **)NULL);
      cantidad_ingresada = strtod (PUT(PQvaluebycol (res, i, "cantidad_ingresada")), (char **)NULL);

      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
                          0, g_strdup_printf ("%.2d/%.2d/%s", atoi (PQvaluebycol (res, i, "dia")),
                                              atoi (PQvaluebycol (res, i, "mes")), PQvaluebycol (res, i, "ano")),
                          1, PQvaluebycol (res, i, "id"),
                          2, PQvaluebycol (res, i, "proveedor"),
                          3, cantidad,
			  4, cantidad_ingresada,
                          5, strtod (PUT (PQvaluebycol (res, i, "precio")), (char **)NULL),
			  6, precio,
			  7, lround (cantidad * precio),
                          -1);

    }

  // Coloca el último precio de compra y el precio de venta del producto en los
  // entry correspondientes.
  if (tuples > 0)
    {
      gtk_entry_set_text (GTK_ENTRY (builder_get (builder, "entry_buy_price")), PUT (PQvaluebycol (res, 0, "precio")));
      gtk_entry_set_text (GTK_ENTRY (builder_get (builder, "entry_sell_price")), PUT (PQvaluebycol (res, 0, "precio_venta")));
    }
}


void
ClearAllCompraData (void)
{
  GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_products_buy_list"))));
  gtk_list_store_clear (store);
  CleanStatusProduct (0);
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_total_buy")), "\t\t\t");
}


void
InsertarCompras (void)
{
  GtkTreeView *treeview = GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_pending_requests"));
  GtkTreeSelection *selection = gtk_tree_view_get_selection (treeview);
  GtkTreeIter iter;
  GtkListStore *store_pending_request = GTK_LIST_STORE (gtk_tree_view_get_model (treeview));
  GtkListStore *store_pending_request_detail = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_pending_request_detail"))));

  PGresult *res;
  gint tuples, i, id_compra;

  res = EjecutarSQL( "SELECT * FROM get_compras()" );

  gtk_list_store_clear (store_pending_request);
  gtk_list_store_clear (store_pending_request_detail);

  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_pending_net_total")), "");
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_pending_task_total")), "");
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_pending_other_task_total")), "");
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_pending_total")), "");

  tuples = PQntuples (res);

  for (i = 0; i < tuples; i++)
    {
      id_compra = atoi (PQvaluebycol(res, i, "id_compra"));

      gtk_list_store_append (store_pending_request, &iter);
      gtk_list_store_set (store_pending_request, &iter,
                          0, id_compra,
                          1, g_strdup_printf ("%.2d/%.2d/%s", atoi (PQvaluebycol (res, i, "dia")),
                                              atoi (PQvaluebycol (res, i, "mes")), PQvaluebycol (res, i, "ano")),
                          2, PQvaluebycol (res, i, "nombre"),
                          3, lround (strtod (PUT (g_strdup (PQvaluebycol (res, i, "precio"))), (char **)NULL)),
                          4, ReturnIncompletProducts (id_compra) ? "Red" : "Black",
                          5, TRUE,
                          -1);

      if (i == 0) gtk_tree_selection_select_iter (selection, &iter);
    }
}


void
IngresoDetalle (GtkTreeSelection *selection, gpointer data)
{
  gint i, id, tuples;
  gboolean color;
  GtkTreeIter iter;

  GtkListStore *store_pending_request = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_pending_requests"))));
  GtkListStore *store_pending_request_detail = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_pending_request_detail"))));

  PGresult *res;

  gdouble sol, ing;
  gdouble cantidad;

  if (compra->header != NULL)
    CompraListClean ();

  if (gtk_tree_selection_get_selected (selection, NULL, &iter) == TRUE)
    {
      gtk_tree_model_get (GTK_TREE_MODEL (store_pending_request), &iter,
                          0, &id,
                          -1);

      gtk_list_store_clear (store_pending_request_detail);

      res = EjecutarSQL ( g_strdup_printf( "SELECT * FROM get_detalle_compra( %d ) ", id ) );

      if (res == NULL)
        return;

      tuples = PQntuples (res);

      if (tuples == 0)
        return;

      for (i = 0; i < tuples; i++)
        {
          sol = strtod (PUT (PQvaluebycol(res, i, "cantidad")), (char **)NULL);
          ing = strtod (PUT (PQvaluebycol(res, i, "cantidad_ingresada")), (char **)NULL);

          if (ing<sol && ing >0)
            color = TRUE;
          else
            color = FALSE;

          gtk_list_store_append (store_pending_request_detail, &iter);
          gtk_list_store_set (store_pending_request_detail, &iter,
                              0, PQvaluebycol(res, i, "codigo_corto"),
                              1, g_strdup_printf ("%s %s %s %s", PQvaluebycol(res, i, "descripcion"),
                                                  PQvaluebycol(res, i, "marca"), PQvaluebycol(res, i, "contenido"),
                                                  PQvaluebycol(res, i, "unidad")),
                              2, strtod (PUT (PQvaluebycol(res, i, "precio")), (char **)NULL),
                              3, sol,
                              4, ing,
                              5, atoi (g_strdup (PQvaluebycol (res, i, "costo_ingresado"))),
                              6, color ? "Red" : "Black",
                              7, TRUE,
                              -1);

          if (ing != 0)
            cantidad = (gdouble) sol - ing;
          else
            cantidad = (gdouble) sol;

          CompraAgregarALista (PQvaluebycol(res, i, "barcode"), cantidad, atoi (PQvaluebycol(res, i, "precio_venta")),
                               strtod (PUT((PQvaluebycol(res, i, "precio"))), (char **)NULL), atoi (PQvaluebycol(res, i, "margen")), TRUE);
        }

      CalcularTotales ();
    }
} // void IngresoDetalle (GtkTreeSelection *selection, gpointer data)


void
IngresarCompra (gboolean invoice, gint n_document, gchar *monto, gint costo_bruto_transporte, GDate *date)
{
  Productos *products = compra->header;
  gint id, doc;
  gchar *rut_proveedor, *nombre_proveedor;
  gchar *q;
  PGresult *res;
  gint total_productos = atoi (monto);
  GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_pending_requests")));
  GtkListStore *store_pending_request = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_pending_requests"))));
  GtkTreeIter iter;

  if (gtk_tree_selection_get_selected (selection, NULL, &iter) == FALSE) return;

  gtk_tree_model_get (GTK_TREE_MODEL (store_pending_request), &iter,
                      0, &id,
                      -1);


  q = g_strdup_printf ("SELECT * FROM get_proveedor_compra( %d )", id);
  res = EjecutarSQL (q);
  rut_proveedor = g_strdup (PQvaluebycol (res, 0, "rut_out"));
  nombre_proveedor = g_strdup (PQvaluebycol (res, 0, "nombre_out"));
  g_free (q);

  if (invoice == TRUE)
    {
      doc = IngresarFactura (n_document, id, rut_proveedor, total_productos, g_date_get_day (date), g_date_get_month (date), g_date_get_year (date), 0, costo_bruto_transporte);
    }
  else
    {
      doc = IngresarGuia (n_document, id, g_date_get_day (date), g_date_get_month (date), g_date_get_year (date));
    }

  if (products != NULL)
    {
      do
	{
	  IngresarProducto (products->product, id, rut_proveedor);

	  IngresarDetalleDocumento (products->product, id, doc, invoice);

	  products = products->next;
	} while (products != compra->header);
    }

  PrintValeCompra (compra->header, id, n_document, nombre_proveedor);

  CompraIngresada ();

  InsertarCompras ();  
}


/**
 * Es llamada por la funcion "AddProveedor()".
 *
 * Esta funcion agrega al proveedor a la lista (tree view) de la busqueda de proveedores.
 *
 */
void
FillProveedores (void)
{
  PGresult *res;
  gint tuples, i;
  GtkTreeModel *model;
  GtkListStore *store;
  GtkTreeView *treeview;

  GtkTreeIter iter;

  /*consulta de sql que llama a la funcion select_proveedor() que retorna
    los proveedores */

  if (rut_proveedor_global != NULL)
    res = EjecutarSQL (g_strdup_printf ("SELECT rut, dv, nombre FROM select_proveedor(%s) "
					"ORDER BY nombre ASC", rut_proveedor_global));
  else
    res = EjecutarSQL ("SELECT rut, dv, nombre FROM select_proveedor() "
		       "ORDER BY nombre ASC");

  if (res == NULL)
    return;

  tuples = PQntuples (res);
  
  treeview = GTK_TREE_VIEW (builder_get (builder, "treeview_prov"));
  model = gtk_tree_view_get_model (treeview);
  store = GTK_LIST_STORE (model);
  gtk_list_store_clear (store);

  /* aqui se agregan los proveedores al tree view*/
  for (i = 0; i < tuples; i++)
    {
      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
                          0, g_strconcat (PQvaluebycol(res, i, "rut"),
					  PQvaluebycol(res, i, "dv"), NULL),
                          1, PQvaluebycol(res, i, "nombre"),
                          -1);
    }

  if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (store), &iter) == TRUE)
    {      
      //gtk_widget_grab_focus (GTK_WIDGET (treeview));
      gtk_window_set_focus (GTK_WINDOW (builder_get (builder, "wnd_provider_data")), 
			    GTK_WIDGET (treeview));
      gtk_tree_selection_select_path (gtk_tree_view_get_selection (treeview),
				      gtk_tree_path_new_from_string ("0"));
    }
  else
    gtk_widget_grab_focus (GTK_WIDGET (builder_get (builder, "btn_add_provider")));

}


/**
 * Es llamada por la funcion "AddProveedor()".
 *
 * Esta funcion cierra la ventana de "AddProveedorWindow()".
 *
 * @param button the button
 * @param user_data the user data
 *
 */
void
CloseAddProveedorWindow (GtkWidget *button, gpointer data)
{
  //GtkWidget *window = (GtkWidget *)data;
  //gtk_widget_destroy (window);

  gtk_widget_hide (GTK_WIDGET (builder_get (builder, "wnd_new_provider")));
  //gtk_window_set_focus (GTK_WINDOW (compra->buy_window), compra->tree_prov);
  gtk_widget_grab_focus (GTK_WIDGET (builder_get (builder, "treeview_prov")));
  FillProveedores ();
}


/**
 * Es llamada cuando el boton aceptar de "AddProveedorWindow()" es presionado (signal click).
 *
 * Esta funcion verifica que todos los campos de proveedor sean correctamente
 * rellenados y luego llama a la funcion "AddProveedorToDB()" que registra al
 * proveedor en la base de datos.
 *
 * @param widget the widget that emited the signal
 * @param data the user data
 *
 */
void
AddProveedor (GtkWidget *widget, gpointer data)
{
  compra->rut_add = GTK_WIDGET (builder_get (builder, "entry_np_rut"));
  gchar *rut = g_strdup (gtk_entry_get_text (GTK_ENTRY (compra->rut_add)));
  compra->rut_ver = GTK_WIDGET (builder_get (builder, "entry_np_dv"));
  gchar *rut_ver = g_strdup (gtk_entry_get_text (GTK_ENTRY (compra->rut_ver)));
  compra->nombre_add = GTK_WIDGET (builder_get (builder, "entry_np_nombre"));
  gchar *nombre = g_strdup (gtk_entry_get_text (GTK_ENTRY (compra->nombre_add)));
  compra->direccion_add = GTK_WIDGET (builder_get (builder, "entry_np_direccion"));
  gchar *direccion = g_strdup (gtk_entry_get_text (GTK_ENTRY (compra->direccion_add)));
  compra->ciudad_add = GTK_WIDGET (builder_get (builder, "entry_np_ciudad"));
  gchar *ciudad = g_strdup (gtk_entry_get_text (GTK_ENTRY (compra->ciudad_add)));
  compra->comuna_add = GTK_WIDGET (builder_get (builder, "entry_np_comuna"));
  gchar *comuna = g_strdup (gtk_entry_get_text (GTK_ENTRY (compra->comuna_add)));
  compra->telefono_add = GTK_WIDGET (builder_get (builder, "entry_np_telefono"));
  gchar *telefono = g_strdup (gtk_entry_get_text (GTK_ENTRY (compra->telefono_add)));
  compra->email_add = GTK_WIDGET (builder_get (builder, "entry_np_email"));
  gchar *email = g_strdup (gtk_entry_get_text (GTK_ENTRY (compra->email_add)));
  compra->web_add = GTK_WIDGET (builder_get (builder, "entry_np_web"));
  gchar *web = g_strdup (gtk_entry_get_text (GTK_ENTRY (compra->web_add)));
  compra->contacto_add = GTK_WIDGET (builder_get (builder, "entry_np_contacto"));
  gchar *contacto = g_strdup (gtk_entry_get_text (GTK_ENTRY (compra->contacto_add)));
  compra->giro_add = GTK_WIDGET (builder_get (builder, "entry_np_giro"));
  gchar *giro = g_strdup (gtk_entry_get_text (GTK_ENTRY (compra->giro_add)));

  if (strcmp (rut, "") == 0)
    {
      ErrorMSG (compra->rut_add, "Debe Escribir el rut completo");
      return;
    }
  else if ((GetDataByOne
            (g_strdup_printf ("SELECT * FROM proveedor WHERE rut='%s'", rut))) != NULL)
    {
      ErrorMSG (compra->rut_add, "Ya existe un proveedor con el mismo rut");
      return;
    }
  else if (g_str_equal (rut_ver, ""))
    {
      ErrorMSG (compra->rut_ver, "Debe ingresar el dígito verificador del rut");
      return;
    }
  else if (g_str_equal (nombre, ""))
    {
      ErrorMSG (compra->nombre_add, "Debe escribir el nombre del proveedor");
      return;
    }
  else if (g_str_equal (direccion, ""))
    {
      ErrorMSG (compra->direccion_add, "Debe escribir la direccion");
      return;
    }
  else if (g_str_equal (comuna, ""))
    {
      ErrorMSG (compra->comuna_add, "Debe escribir la comuna");
      return;
    }
  else if (g_str_equal (telefono, ""))
    {
      ErrorMSG (compra->telefono_add, "Debe escribir el telefono");
      return;
    }

  else if (g_str_equal (giro, ""))
    {
      ErrorMSG (compra->giro_add, "Debe escribir el giro");
      return;
    }

  if (VerificarRut (rut, rut_ver) != TRUE)
    {
      ErrorMSG (compra->rut_ver, "El rut no es valido!");
      return;
    }

  if (HaveCharacters (telefono))
    {
      ErrorMSG (compra->telefono_add, "Debe ingresar solo números en el campo telefono");
      return;
    }

  //CloseAddProveedorWindow (NULL, data);
  gtk_widget_hide (GTK_WIDGET (builder_get (builder, "wnd_new_provider")));

  AddProveedorToDB (g_strdup_printf ("%s-%s", rut, rut_ver),
                    nombre,
                    direccion,
                    ciudad,
                    comuna,
                    telefono,
                    email,
                    web,
                    contacto,
                    giro);

  FillProveedores ();
} // void AddProveedor (GtkWidget *widget, gpointer data)


/**
 *
 * Esta funcion visualiza la ventana para agregar un proveedor y agrega dos señales
 *
 * @param widget the widget that emited the signal
 */
void
AddProveedorWindow (GtkButton *button)
{
  clean_container (GTK_CONTAINER (builder_get (builder, "wnd_new_provider")));
  gtk_widget_show (GTK_WIDGET (builder_get (builder, "wnd_new_provider")));
}


void
Seleccionado (GtkTreeSelection *selection, gpointer data)
{
  GtkTreeIter iter;  
  gchar *rut, *rut_completo;
  PGresult *res;
  gchar *q;
  GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (builder_get (builder, "treeview_prov"))));
  
  if (gtk_tree_selection_get_selected (selection, NULL, &iter) == TRUE)
    {
      gtk_tree_model_get (GTK_TREE_MODEL (store), &iter,
                          0, &rut_completo,
                          -1);
      rut = g_strndup (rut_completo, strlen (rut_completo)-1);
      q = g_strdup_printf ("SELECT rut, nombre FROM select_proveedor (%s)", rut);
      res = EjecutarSQL (q);
      g_free (q);

      gtk_label_set_text (GTK_LABEL (builder_get (builder, "lbl_provider_rut")), //PQvaluebycol( res, 0, "rut"));
			  formato_rut (rut_completo));
      gtk_label_set_text (GTK_LABEL (builder_get (builder, "lbl_provider_name")),
                          PQvaluebycol (res, 0, "nombre"));
    }
}

/**
 * Anulacion de compras no ingresadas
 * (se anula la petición de compra)
 */
void
on_btn_nullify_buy_clicked (void)
{
  GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_pending_requests")));
  GtkListStore *store_pending_request = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_pending_requests"))));
  GtkTreeIter iter;
  gint id_compra;

  if (gtk_tree_selection_get_selected (selection, NULL, &iter) == TRUE)
    {
      gtk_tree_model_get (GTK_TREE_MODEL (store_pending_request), &iter,
                          0, &id_compra,
                          -1);
      AnularCompraDB (id_compra);
      InsertarCompras ();
    }
}


void
on_btn_nullify_product_clicked (void)
{
  PGresult *res;

  GtkTreeSelection *selection1 = gtk_tree_view_get_selection (GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_pending_requests")));
  GtkListStore *store_pending_request = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_pending_requests"))));

  GtkTreeSelection *selection2 = gtk_tree_view_get_selection (GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_pending_request_detail")));
  GtkListStore *store_pending_request_detail = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_pending_request_detail"))));

  GtkTreeIter iter1, iter2;
  gint id_compra;
  gint total_prod_anul;
  gint total_prod_sol;
  gdouble unidades_ingresadas;
  gchar *codigo_producto, *barcode;
  gchar *q;

  if (gtk_tree_selection_get_selected (selection1, NULL, &iter1) &&
      gtk_tree_selection_get_selected (selection2, NULL, &iter2))
    {
      gtk_tree_model_get (GTK_TREE_MODEL (store_pending_request), &iter1,
                          0, &id_compra,
                          -1);
      gtk_tree_model_get (GTK_TREE_MODEL (store_pending_request_detail), &iter2,
                          0, &codigo_producto,
                          -1);

      barcode = codigo_corto_to_barcode (codigo_producto);

      //TODO: pasar esto a funciones de la base pero por el momento funciona
      q = g_strdup_printf ("SELECT COUNT(*) AS cantidad_prod_sol, "
			   "       (SELECT COUNT(*) "
			   "	       FROM compra_detalle "
			   "	       WHERE anulado = true "
			   "	       AND id_compra = %d) AS cantidad_prod_anul, "
			   "       (SELECT cantidad_ingresada "
			   "	       FROM compra_detalle "
			   "	       WHERE barcode_product = %s "
			   "	       AND id_compra = %d) AS unidades_ingresadas "
			   "FROM compra_detalle "
			   "WHERE id_compra = %d", id_compra, barcode, id_compra, id_compra);
      res = EjecutarSQL (q);
      g_free (q);

      total_prod_sol = atoi (g_strdup (PQvaluebycol (res, 0, "cantidad_prod_sol")));
      total_prod_anul = atoi (g_strdup (PQvaluebycol (res, 0, "cantidad_prod_anul")));
      unidades_ingresadas = strtod (PUT (PQvaluebycol (res, 0, "unidades_ingresadas")), (char **)NULL);

      //Se actualiza el estado del producto y de haber ingreso, se iguala con la cantidad pedida
      q = g_strdup_printf ("UPDATE compra_detalle "
			   "SET anulado='t' %s "
			   "WHERE barcode_product = %s "
			   "AND id_compra=%d", 
			   (unidades_ingresadas > 0) ? 
			   g_strdup_printf (", cantidad = %s", 
					    CUT (g_strdup_printf ("%.3f", unidades_ingresadas))) : "",
			   barcode, id_compra);
      res = EjecutarSQL (q);
      g_free (q);

      //Si todos los productos pedidos estan anulados, se anula la compra
      if (total_prod_sol == total_prod_anul)
	{
	  q = g_strdup_printf ("UPDATE compra SET anulada='t' "
			       "WHERE id NOT IN (SELECT id_compra "
			       "                 FROM compra_detalle "
			       "                 WHERE id_compra=%d AND anulado='f') "
			       "AND id=%d", id_compra, id_compra);
	  res = EjecutarSQL (q);
	  g_free (q);
	}

      InsertarCompras ();
    }
}


/**
 * This function is a callback from
 * 'button_clear' button (signal click)
 *
 * call to the CleanStatusProduct function with
 * option != 0 (to a partial cleaning)
 *
 * @param button the button
 * @param user_data the user data
 */
void
on_button_clear_clicked (GtkButton *button, gpointer user_data)
{
  gboolean modo_traspaso;
  modo_traspaso = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (builder_get (builder, "rdbutton_transfer_mode")));
  
  /*Si el modo traspaso esta activado, limpia todo*/
  if (modo_traspaso)
    CleanStatusProduct (0);
  else
    CleanStatusProduct (1);
}

/**
 * This function clean the status product
 *
 * If option = 0 a full cleaning is done, 
 * else partial cleaning is done as the case
 *
 * @param gint option
 */
void
CleanStatusProduct (gint option)
{
  GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (gtk_builder_get_object (builder, "product_history_tree_view"))));
  gchar *barcode;
  gint derivada, compuesta;
  gboolean modo_inventario;

  modo_inventario = rizoma_get_value_boolean ("MODO_INVENTARIO");

  //Solo importa obtener el barcode si es una limpieza "parcial"
  barcode = (option == 1) ?
    g_strdup (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_buy_barcode")))) : "";

  //Si hay un producto seleccionado y es derivado o compuesto
  if (!g_str_equal (barcode, ""))
    {
      derivada = atoi (g_strdup (PQvaluebycol (EjecutarSQL ("SELECT id FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'DERIVADA'"), 0, "id")));
      compuesta = atoi (g_strdup (PQvaluebycol (EjecutarSQL ("SELECT id FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'COMPUESTA'"), 0, "id")));
      if (tipo_producto == derivada || tipo_producto == compuesta)
	{
	  CleanStatusProduct (0);
	  return;
	}
    }

  if (gtk_widget_get_sensitive (GTK_WIDGET (builder_get (builder, "entry_buy_price"))) == FALSE &&
      !g_str_equal (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_buy_price"))), "") &&
      option == 1)
    {
      if (modo_inventario == TRUE && 
	  gtk_widget_get_sensitive (GTK_WIDGET (builder_get (builder, "entry_buy_gain"))) == TRUE)
	{
	  CleanStatusProduct (0);
	  return;
	}
	
      if (modo_inventario == FALSE)
	gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "entry_buy_price")), TRUE);

      gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "entry_buy_gain")), TRUE);
      gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "entry_sell_price")), TRUE);
      gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "button_calculate")), TRUE);
      gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "cmbPrecioCompra")), TRUE);
      gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "entry_buy_amount")), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "button_add_product_list")), FALSE);
      gtk_entry_set_text (GTK_ENTRY (builder_get (builder, "entry_buy_gain")), "");
      gtk_widget_grab_focus (GTK_WIDGET (builder_get (builder, "entry_sell_price")));
      return;
    }

  gtk_list_store_clear (store);

  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_product_desc")), "");
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_mark")), "");
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_unit")), "");
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_stock")), "");
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_stock_further")), "");
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_sell_price")), "");
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_fifo")), "");
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_code")), "");
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_cont")), "");

  gtk_entry_set_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_buy_price")), "");
  
  if (modo_inventario == FALSE)
    {
      gtk_entry_set_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_buy_gain")), "");
      gtk_entry_set_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_buy_amount")), "1");
    }

  gtk_entry_set_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_sell_price")), "");
  gtk_entry_set_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_buy_barcode")), "");

  //Habilitar la escritura en el campo del código de barra cuando se limpie todo
  //TODO: Ver como simplificar el set_sensitive
  gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "entry_buy_barcode")), TRUE);
  gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "entry_buy_price")), FALSE);
  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "cmbPrecioCompra")), FALSE);
  gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "entry_buy_gain")), FALSE);
  gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "entry_sell_price")), FALSE);
  gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "entry_buy_amount")), FALSE);
  gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "button_new_product")), TRUE);
  gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "edit_product_button")), FALSE);
  gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "button_calculate")), FALSE);
  gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "button_add_product_list")), FALSE);

  GtkTreeView *treeview = GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_products_buy_list"));
  if (get_treeview_length (treeview) == 0)
    {
      gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "rdbutton_buy_mode")), TRUE);

      //Se habilita el modo traspaso si se inició rizoma con modo traspaso activado
      if (gtk_widget_get_visible (GTK_WIDGET (gtk_builder_get_object (builder, "btn_traspaso_recibir"))) &&
	  rizoma_get_value_boolean ("TRASPASO"))
	gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "rdbutton_transfer_mode")), TRUE);

      gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "btn_traspaso_recibir")), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "btn_traspaso_enviar")), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "button_buy")), FALSE);

      gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_suggest_buy")), TRUE);
      gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_nullify_buy_pi")), TRUE);
    }

  gtk_widget_grab_focus (GTK_WIDGET (builder_get (builder, "entry_buy_barcode")));

  calcular = 0;
  tipo_producto = 0;
}


gboolean
CheckDocumentData (GtkWidget *wnd, gboolean invoice, gchar *rut_proveedor, gint id, gchar *n_documento, gchar *monto, GDate *date)
{
  PGresult *res;
  GDate *date_buy = g_date_new ();

  res = EjecutarSQL (g_strdup_printf ("SELECT date_part('day', fecha), "
                                      "date_part('month', fecha), "
                                      "date_part('year', fecha) "
                                      "FROM compra WHERE id=%d", id));

  g_date_set_dmy (date_buy, atoi (PQgetvalue( res, 0, 0)), atoi (PQgetvalue( res, 0, 1)), atoi (PQgetvalue( res, 0, 2)));
  
  /*
  if (invoice)
    {
      if (g_date_compare (date_buy, date) > 0)
         ErrorMSG (wnd, "La fecha de emision del documento no puede ser menor a la fecha de compra");
    }
  else
    {
      if (g_date_compare (date_buy, date) > 0 )
         ErrorMSG (wnd, "La fecha de emision del documento no puede ser menor a la fecha de compra");
    }
  */

  if (invoice)
    {
      if (strcmp (n_documento, "") == 0 || atoi (n_documento) <= 0)
        {
          ErrorMSG (wnd, "Ingrese un número de documento válido (debe ser numérico)");
          return FALSE;
        }
      else if (strcmp (monto, "") == 0 || atoi (monto) <= 0)
        {
          ErrorMSG (wnd, "El Monto del documento debe ser ingresado");
          return FALSE;
        }
      else
        {
          if (DataExist (g_strdup_printf ("SELECT num_factura FROM factura_compra "
					  "WHERE rut_proveedor='%s' AND num_factura=%s "
					  "AND id NOT IN (SELECT id_factura_compra FROM compra_anulada)", rut_proveedor, n_documento)) == TRUE)
            {
              ErrorMSG (GTK_WIDGET (builder_get (builder, "entry_int_ingress_factura_n")), 
			g_strdup_printf ("Ya existe la factura %s ingresada de este proveedor", n_documento));
              return FALSE;
            }
          return TRUE;
        }
    }
  else
    {
      if (strcmp (n_documento, "") == 0 || atoi (n_documento) <= 0)
        {
          ErrorMSG (wnd, "Debe Obligatoriamente ingresar el numero del documento");
          return FALSE;
        }
      else if (strcmp (monto, "") == 0 || atoi (monto) <= 0)
        {
          ErrorMSG (wnd, "El Monto del documento debe ser ingresado");
          return FALSE;
        }
      else
        {
          if (DataExist (g_strdup_printf ("SELECT numero FROM guias_compra WHERE rut_proveedor='%s' AND numero=%s", rut_proveedor, n_documento)) == TRUE)
            {
              ErrorMSG (wnd, g_strdup_printf ("Ya existe la guia %s ingresada de este proveedor", n_documento));
              return FALSE;
            }
          return TRUE;
        }
    }
}


void
FillPagarFacturas (gchar *rut_proveedor)
{
  gchar *q;
  gint tuples, i;
  gint total_amount = 0;
  GtkListStore *store_invoice = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (builder_get (builder, "tree_view_invoice_list"))));
  GtkListStore *store_invoice_detail = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (builder_get (builder, "tree_view_invoice_detail"))));
  GtkTreeIter iter;

  PGresult *res;

  if (rut_proveedor == NULL) return;

  q = g_strdup_printf ("SELECT fc.id, fc.num_factura, fc.monto, "
		       "date_part ('day', fc.fecha) as dia, date_part('month', fc.fecha) as mes, date_part('year', fc.fecha) as ano, "
		       "fc.id_compra, date_part ('day', fecha_pago) AS pay_day, date_part ('month', fecha_pago) AS pay_month, date_part ('year', fecha_pago) AS pay_year, "
		       "fp.nombre, fc.id, fc.rut_proveedor, "
		       "(SELECT dv FROM proveedor WHERE rut = fc.rut_proveedor) AS dv "
		       "FROM formas_pago fp "
		       "INNER JOIN factura_compra fc "
		       "ON fp.id = fc.forma_pago "
		       "INNER JOIN compra c "
		       "ON fc.id_compra = c.id "
		       "WHERE fc.rut_proveedor='%s' "
		       "AND fc.pagada='f' "
		       "AND c.anulada_pi = false "
		       "ORDER BY pay_year, pay_month, pay_day ASC", rut_proveedor);

  res = EjecutarSQL (q);
  g_free (q);

  if (res == NULL) return;

  tuples = PQntuples (res);

  gtk_list_store_clear (store_invoice);
  gtk_list_store_clear (store_invoice_detail);

  for (i = 0; i < tuples; i++)
    {
      gtk_list_store_append (store_invoice, &iter);
      gtk_list_store_set (store_invoice, &iter,
                          0, PQvaluebycol (res, i, "id"),
                          1, g_strconcat (PQvaluebycol (res, i, "rut_proveedor"),
					  PQvaluebycol (res, i, "dv"), NULL),
                          2, PQvaluebycol (res, i, "num_factura"),
                          3, PQvaluebycol (res, i, "id_compra"),
                          4, g_strdup_printf ("%.2d/%.2d/%.4d", atoi (PQvaluebycol (res, i, "dia")),
                                              atoi (PQvaluebycol (res, i, "mes")), atoi (PQvaluebycol (res, i, "ano"))),
                          5, g_strdup_printf ("%.2d/%.2d/%.4d", atoi (PQvaluebycol (res, i, "pay_day")),
                                              atoi (PQvaluebycol (res, i, "pay_month")), atoi (PQvaluebycol (res, i, "pay_year"))),
                          6, atoi (PQvaluebycol (res, i, "monto")),
                          -1);
      total_amount += atoi (PQvaluebycol (res, i, "monto"));
    }

  gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_invoice_total_amount")),
                        g_strdup_printf ("<b>$ %s</b>", PutPoints (g_strdup_printf("%d", total_amount))));
}


void
FillGuias (gchar *rut_proveedor)
{
  GtkTreeView *treeview = GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_pending_guide"));
  GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model (treeview));
  GtkTreeIter iter;
  PGresult *res;
  gint tuples, i;
  gchar *q;

  q = g_strdup_printf ("SELECT id, numero, (SELECT SUM ((precio * cantidad) + iva + otros_impuestos) FROM compra_detalle WHERE compra_detalle.id_compra=guias_compra.id_compra) as monto "
                       "FROM guias_compra WHERE rut_proveedor='%s'", rut_proveedor);
  res = EjecutarSQL (q);

  tuples = PQntuples (res);

  gtk_list_store_clear (store);

  for (i = 0; i < tuples; i++)
    {
      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
                          0, PQvaluebycol (res, i, "id"),
                          1, PQvaluebycol (res, i, "numero"),
                          2, PQvaluebycol (res, i, "monto"),
                          -1);
    }
}


void
FillDetGuias (GtkTreeSelection *selection, gpointer data)
{
  gint tuples, i;
  gchar *guia;
  GtkTreeIter iter;
  PGresult *res;
  gchar *rut_proveedor = g_strdup (gtk_label_get_text (GTK_LABEL (compra->fact_rut)));

  if (gtk_tree_selection_get_selected (selection, NULL, &iter) == TRUE)
    {

      gtk_tree_model_get (GTK_TREE_MODEL (compra->store_guias), &iter,
                          0, &guia,
                          -1);

      res =
	EjecutarSQL
	(g_strdup_printf ("SELECT t1.codigo, t1.descripcion,  t2.cantidad, t2.precio, t2.cantidad * t2.precio AS "
			  "total, t2.barcode, t2.id_compra, t1.marca, t1.contenido, t1.unidad FROM producto AS "
			  "t1, documentos_detalle AS t2 WHERE t2.id_compra=(SELECT id_compra FROM guias_compra "
			  "WHERE numero=%s AND rut_proveedor='%s') AND t1.barcode=t2.barcode AND t2.numero=%s",
			  guia, rut_proveedor, guia));

      tuples = PQntuples (res);

      gtk_tree_store_clear (compra->store_det_guias);

      for (i = 0; i < tuples; i++)
        {
          gtk_tree_store_append (compra->store_det_guias, &iter, NULL);
          gtk_tree_store_set (compra->store_det_guias, &iter,
                              0, PQgetvalue (res, i, 0),
                              1, g_strdup_printf ("%s %s %s %s", PQgetvalue (res, i, 1),
                                                  PQgetvalue (res, i, 7), PQgetvalue (res, i, 8),
                                                  PQgetvalue (res, i, 9)),
                              2, PQgetvalue (res, i, 2),
                              3, PQgetvalue (res, i, 3),
                              4, PutPoints (g_strdup_printf ("%d", atoi (PQgetvalue (res, i, 4)))),
                              5, CheckProductIntegrity (PQgetvalue (res, i, 6), PQgetvalue (res, i, 5)) ? "Black" : "Red",
                              6, TRUE,
                              -1);
        }
    } // if (gtk_tree_selection_get_selected (selection, NULL, &iter) == TRUE)
}


void
FoundProveedor (GtkWidget *widget, gpointer data)
{
  gchar *string = g_strdup (gtk_entry_get_text (GTK_ENTRY (data)));
  PGresult *res;
  gint tuples, i;
  GtkTreeIter iter;

  res = EjecutarSQL (g_strdup_printf
                     ("SELECT * FROM proveedor WHERE lower(nombre) LIKE lower('%%%s%%') "
                      "OR rut LIKE '%%%s%%'", string, string));

  tuples = PQntuples (res);

  gtk_list_store_clear (compra->store_prov);

  for (i = 0; i < tuples; i++)
    {
      gtk_list_store_append (compra->store_prov, &iter);
      gtk_list_store_set (compra->store_prov, &iter,
                          0, PQvaluebycol (res, i, "rut"),
                          1, PQvaluebycol (res, i, "nombre"),
                          -1);
    }
}


void
FillProveedorData (gchar *rut, gboolean guias)
{
  PGresult *res;

  res = EjecutarSQL (g_strdup_printf ("SELECT * FROM select_proveedor('%s')", rut));

  if (guias == TRUE)
    {
      ClearFactData ();

      gtk_entry_set_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_guide_invoice_provider")), PQvaluebycol (res, 0, "nombre"));

      gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_guide_invoice_rut")),
                            g_strdup_printf ("<span weight=\"ultrabold\">%s</span>", 
					     formato_rut (g_strconcat (PQvaluebycol (res, 0, "rut"), 
								       PQvaluebycol (res, 0, "dv"), NULL))));

      gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_guide_invoice_contact")),
                            g_strdup_printf ("<span weight=\"ultrabold\">%s</span>",
                                             PQvaluebycol (res, 0, "contacto")));

      gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_guide_invoice_address")),
                            g_strdup_printf ("<span weight=\"ultrabold\">%s</span>",
                                             PQvaluebycol (res, 0, "direccion")));

      gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_guide_invoice_comuna")),
                            g_strdup_printf ("<span weight=\"ultrabold\">%s</span>",
                                             PQvaluebycol (res, 0, "comuna")));

      gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_guide_invoice_fono")),
                            g_strdup_printf ("<span weight=\"ultrabold\">%s</span>",
                                             PQvaluebycol (res, 0, "telefono")));

      gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_guide_invoice_mail")),
                            g_strdup_printf ("<span weight=\"ultrabold\">%s</span>",
                                             PQvaluebycol (res, 0, "email")));

      gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_guide_invoice_web")),
                            g_strdup_printf ("<span weight=\"ultrabold\">%s</span>",
                                             PQvaluebycol (res, 0, "web")));

      FillGuias (rut);

    }
  else if (guias == FALSE)
    {
      gtk_entry_set_text (GTK_ENTRY (builder_get (builder, "entry_invoice_provider")), PQvaluebycol (res, 0, "nombre"));

      gtk_label_set_markup (GTK_LABEL (builder_get (builder, "label_invoice_rut")),
                            g_strdup_printf ("<span weight=\"ultrabold\">%s</span>", 
					     formato_rut (g_strconcat (PQvaluebycol (res, 0, "rut"),
								       PQvaluebycol (res, 0, "dv"), NULL))));

      gtk_label_set_markup (GTK_LABEL (builder_get (builder, "label_invoice_contact")),
                            g_strdup_printf ("<span weight=\"ultrabold\">%s</span>",
                                             PQvaluebycol (res, 0, "contacto")));

      gtk_label_set_markup (GTK_LABEL (builder_get (builder, "label_invoice_address")),
                            g_strdup_printf ("<span weight=\"ultrabold\">%s</span>",
                                             PQvaluebycol (res, 0, "direccion")));

      gtk_label_set_markup (GTK_LABEL (builder_get (builder, "label_invoice_comuna")),
                            g_strdup_printf ("<span weight=\"ultrabold\">%s</span>",
                                             PQvaluebycol (res, 0, "comuna")));

      gtk_label_set_markup (GTK_LABEL (builder_get (builder, "label_invoice_fono")),
                            g_strdup_printf ("<span weight=\"ultrabold\">%s</span>",
                                             PQvaluebycol (res, 0, "telefono")));

      gtk_label_set_markup (GTK_LABEL (builder_get (builder, "label_invoice_mail")),
                            g_strdup_printf ("<span weight=\"ultrabold\">%s</span>",
                                             PQvaluebycol (res, 0, "email")));

      gtk_label_set_markup (GTK_LABEL (builder_get (builder, "label_invoice_web")),
                            g_strdup_printf ("<span weight=\"ultrabold\">%s</span>",
                                             PQvaluebycol (res, 0, "web")));

      FillPagarFacturas (rut);
    }

  if (guias == TRUE)
    gtk_widget_grab_focus (GTK_WIDGET (gtk_builder_get_object (builder, "entry_guide_invoice_n_invoice")));
  else
    gtk_widget_grab_focus (GTK_WIDGET (builder_get (builder, "entry_invoice_provider")));
}


void
CalcularTotales (void)
{
  GtkTreeView *treeview;
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkTreeIter iter;

  Productos *products = compra->header;

  gdouble total_neto = 0, total_iva = 0, total_otros = 0, total = 0;
  gdouble iva, otros;

  treeview = GTK_TREE_VIEW (builder_get (builder, "tree_view_pending_requests"));
  model = gtk_tree_view_get_model (treeview);

  do
    {
      iva = GetIVA (products->product->barcode);
      otros = GetOtros (products->product->barcode);

      if (products->product->canjear != TRUE)
	total = (gdouble)products->product->precio_compra * products->product->cantidad;
      else
	total = (gdouble)products->product->precio_compra * (products->product->cantidad - products->product->cuanto);

      total_neto += total;

      if (iva != 0)
	total_iva += (total * (gdouble) iva / 100);

      if (otros != 0)
	total_otros += (total * (gdouble) otros / 100);

      products = products->next;
    } while (products != compra->header);

  total = total_neto + total_otros + total_iva;

  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_pending_net_total")),
                      PutPoints (g_strdup_printf ("%ld", lround (total_neto))));
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_pending_task_total")),
                      PutPoints (g_strdup_printf ("%ld", lround (total_iva))));
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_pending_other_task_total")),
                      PutPoints (g_strdup_printf ("%ld", lround (total_otros))));
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_pending_total")),
                      PutPoints (g_strdup_printf ("%ld", lround (total))));
  
  selection = gtk_tree_view_get_selection (treeview);
  if (gtk_tree_selection_get_selected (selection, NULL, &iter))
    gtk_list_store_set (GTK_LIST_STORE (model), &iter,
			3, lround (total_neto),
			-1);
}


void
CalcularTotalesGuias (void)
{
  gint total_neto = 0, total_iva = 0, total_otros = 0, total = 0;
  gchar *guia;
  PGresult *res;
  gchar *rut_proveedor = g_strdup (gtk_label_get_text (GTK_LABEL (gtk_builder_get_object (builder, "label_guide_invoice_rut"))));

  GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_guide_invoice")));
  GtkTreeIter iter;

  if (gtk_tree_model_get_iter_first (model, &iter) == TRUE)
    {
      gtk_tree_model_get (model, &iter,
                          1, &guia,
                          -1);
      if (guia != NULL)
        {
          while (1)
            {
              res = EjecutarSQL (g_strdup_printf ("SELECT SUM (t1.precio * t3.cantidad) AS neto, SUM (t3.iva) AS iva, SUM (t3.otros) AS otros, SUM ((t1.precio * t3.cantidad) + t3.iva + t3.otros) AS total  FROM compra_detalle AS t1, guias_compra AS t2, guias_compra_detalle AS t3 WHERE t1.id_compra=(SELECT id_compra FROM guias_compra WHERE numero=%s AND rut_proveedor='%s') AND t2.numero=%s AND t1.barcode_product=t3.barcode and t3.id_guias_compra=t2.id",
                                                  guia, rut_proveedor, guia));

              total_neto += atoi (PQvaluebycol (res, 0, "neto"));

              total_iva += atoi (PQvaluebycol (res, 0, "iva"));

              total_otros += atoi (PQvaluebycol (res, 0, "otros"));

              total += atoi (PQvaluebycol (res, 0, "total"));

              if (gtk_tree_model_iter_next (model, &iter) != TRUE)
                break;
              else
                gtk_tree_model_get (model, &iter,
                                    0, &guia,
                                    -1);

            }

          gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_guide_invoice_neto")),
                              PutPoints (g_strdup_printf ("%d", total_neto)));
          gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_guide_invoice_task")),
                              PutPoints (g_strdup_printf ("%d", total_iva)));
          gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_guide_invoice_other_tasks")),
                              PutPoints (g_strdup_printf ("%d", total_otros)));

          gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_guide_invoice_total")),
                              PutPoints (g_strdup_printf ("%d", total)));

          CheckMontoGuias ();
        }
    }
  else
    {
      gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_guide_invoice_neto")), "");
      gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_guide_invoice_task")), "");
      gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_guide_invoice_other_tasks")), "");
      gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_guide_invoice_total")), "");
    }
}


void
CheckMontoGuias (void)
{
  gint total_guias = atoi (CutPoints (g_strdup (gtk_label_get_text (GTK_LABEL (gtk_builder_get_object (builder, "label_guide_invoice_total"))))));
  gint total_fact = atoi (CutPoints (g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_guide_invoice_amount"))))));

  if (total_guias == 0)
    return;

  if (total_guias < total_fact)
    {
      gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "btn_guide_invoice_ok")), FALSE);
    }
  else if (total_guias > total_fact)
    {
      gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "btn_guide_invoice_ok")), FALSE);
    }
  else if (total_guias == total_fact)
    {
      gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "btn_guide_invoice_ok")), TRUE);
      gtk_widget_grab_focus (GTK_WIDGET (gtk_builder_get_object (builder, "btn_guide_invoice_ok")));
    }
}


void
CheckMontoIngreso (GtkWidget *btn_ok, gint total, gint total_doc)
{
  if (total < total_doc)
    {
      gtk_widget_set_sensitive (btn_ok, FALSE);
    }
  else if (total > total_doc)
    {
      gtk_widget_set_sensitive (btn_ok, FALSE);
    }
  else if (total == total_doc)
    {
      gtk_widget_set_sensitive (btn_ok, TRUE);
      gtk_widget_grab_focus (btn_ok);
    }
}


void
AskIngreso (void)
{
  GtkWidget *treeview;
  treeview = GTK_WIDGET (builder_get (builder, "tree_view_pending_requests"));
  if (get_treeview_length (GTK_TREE_VIEW (treeview)) < 1)
    {
      ErrorMSG (treeview, "No existen ingresos pendientes. \n"
		"Debe ingresar uno o más pedidos a comprar");
      return;
    }

  GtkWindow *wnd_ingress = GTK_WINDOW (gtk_builder_get_object (builder, "wnd_ingress_buy"));

  gtk_widget_grab_focus (GTK_WIDGET (builder_get (builder, "button_ok_ingress")));
  gtk_widget_show_all (GTK_WIDGET (wnd_ingress));
}


void
SetElabVencDate (GtkCalendar *calendar, gpointer data)
{
  GtkButton *button = (GtkButton *) data;
  guint day, month, year;

  gtk_calendar_get_date (calendar, &year, &month, &day);

  gtk_button_set_label (button, g_strdup_printf ("%.2u/%.2u/%.4u", day, month+1, year));

  SetToggleMode (GTK_TOGGLE_BUTTON (button), NULL);
}


void
AskElabVenc (GtkWidget *wnd, gboolean invoice)
{
  GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_pending_requests")));
  GtkListStore *store_pending_request = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_pending_requests"))));
  GtkTreeIter iter;
  gint id;
  gchar *rut_proveedor;

  GList *list = gtk_container_get_children (GTK_CONTAINER (wnd));
  gint nth;
  gint costo_bruto_transporte;
  gchar *widget_name = NULL;

  GtkEntry *entry_n = NULL;
  GtkEntry *entry_amount = NULL;
  GtkEntry *entry_date = NULL;

  gchar *n_documento = NULL;
  gchar *monto = NULL;

  GDate *date = g_date_new ();

  while (list != NULL)
    {
      if (GTK_IS_ENTRY (list->data))
        {
          widget_name = g_strdup (gtk_buildable_get_name (GTK_BUILDABLE (list->data)));

          if (validate_string ("n$", widget_name))
            {
              entry_n = GTK_ENTRY (list->data);
            }
          else if (validate_string ("amount$", widget_name))
            {
              entry_amount = GTK_ENTRY (list->data);
            }
          else if (validate_string ("date$", widget_name))
            {
              entry_date = GTK_ENTRY (list->data);
            }
          g_free (widget_name);
        }
      else if (GTK_IS_CONTAINER (list->data) && !GTK_IS_TREE_VIEW (list->data))
        {
          nth = g_list_index (list, list->data);
          list = g_list_concat (list, gtk_container_get_children (GTK_CONTAINER(list->data)));
          list = g_list_nth (list, nth);
        }

      list = g_list_next (list);
    }
  g_list_free (list);

  n_documento = g_strdup (gtk_entry_get_text (entry_n));
  if (n_documento == NULL || HaveCharacters (n_documento))
    {
      ErrorMSG (GTK_WIDGET (entry_n), 
		"Ingrese un número de factura válido (debe ser numérico)");
      return;
    }


  monto = g_strdup (gtk_entry_get_text (entry_amount));

  if (entry_amount == NULL || entry_date == NULL || entry_n == NULL)
    {
      gtk_widget_hide (wnd);
      ErrorMSG (GTK_WIDGET (entry_date), "Hubo un fallo obteniendo los datos, contacte a su proveedor");
      return;
    }

  g_date_set_parse (date, gtk_entry_get_text (entry_date));
  if (!g_date_valid (date))
    {
      ErrorMSG (GTK_WIDGET (entry_date), "Debe ingresar la fecha de emisión del documento");
      return;
    }

  if (gtk_tree_selection_get_selected (selection, NULL, &iter) == FALSE)
    return;

  gtk_tree_model_get (GTK_TREE_MODEL (store_pending_request), &iter,
                      0, &id,
                      -1);

  rut_proveedor = GetDataByOne (g_strdup_printf ("SELECT rut_proveedor FROM compra WHERE id=%d", id));

  if (HaveCharacters (monto) || HaveCharacters (n_documento))
    {
      ErrorMSG (GTK_WIDGET (entry_amount), "El formulario contiene caracteres invalidos.");
      return;
    }

  if (g_str_equal (gtk_buildable_get_name (GTK_BUILDABLE (wnd)), "wnd_ingress_invoice"))
    costo_bruto_transporte = strtod (CutPoints (g_strdup (gtk_label_get_text (GTK_LABEL (builder_get (builder, "subtotal_shipping_invoice"))))), (char **)NULL);
  else
    costo_bruto_transporte = 0;

  if (CheckDocumentData (wnd, invoice, rut_proveedor, id, n_documento, monto, date) == FALSE) return;
  // CONTINUAR ...
  IngresarCompra (invoice, atoi (n_documento), monto, costo_bruto_transporte, date);

  gtk_widget_hide (wnd);
}


int
main (int argc, char **argv)
{
  //GtkWindow *login_window;
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

  gtk_widget_show_all (GTK_WIDGET (builder_get (builder, "login_window")));

  gtk_main ();

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

      compras_win ();

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
on_entry_buy_barcode_activate (GtkEntry *entry, gpointer user_data)
{
  gchar *barcode = g_strdup (gtk_entry_get_text (entry));

  if (HaveCharacters (barcode) == TRUE || g_str_equal (barcode, ""))
    {
      SearchByName ();
    }
  else
    {
      SearchProductHistory (entry, barcode);
    }
}


void
on_buy_notebook_switch_page (GtkNotebook *notebook, GtkNotebookPage *page, guint page_num, gpointer user_data)
{
  GtkWidget *widget;

  switch (page_num)
    {
    case 0:
      gtk_widget_grab_focus (GTK_WIDGET (builder_get (builder, "entry_buy_barcode")));
      break;
    case 1:
      InsertarCompras ();
      break;
    case 2:
      clean_container (GTK_CONTAINER (gtk_builder_get_object (builder, "vbox_guide_invoice")));
      break;
    case 4: //mercaderia tab
      //update_labels_total_merchandise ();
      widget = GTK_WIDGET(gtk_builder_get_object(builder, "find_product_entry"));
      gtk_editable_select_region(GTK_EDITABLE(widget), 0, -1);
      gtk_widget_grab_focus(widget);
    default:
      break;
    }
}

void
on_entry_buy_price_activate (GtkEntry *entry, gpointer user_data)
{
  if (gtk_widget_get_sensitive (GTK_WIDGET (builder_get (builder, "entry_buy_gain"))) == FALSE &&
      gtk_widget_get_sensitive (GTK_WIDGET (builder_get (builder, "entry_sell_price"))) == FALSE)
    gtk_widget_grab_focus (GTK_WIDGET (builder_get (builder, "button_calculate")));
  else
    gtk_widget_grab_focus (GTK_WIDGET (builder_get (builder, "entry_buy_gain")));
}


/**
 * Es llamada cuando el boton "button_calculate" es presionado (signal click).
 *
 * Esta funcion verifica si se han ingresado valores a los entry
 * "entry_buy_price", "entry_buy_gain", "entry_sell_price", si es así llama a
 * la funcion CalcularPrecioFinal.
 *
 * @param button the button
 * @param user_data the user data
 */
void
on_button_calculate_clicked (GtkButton *button, gpointer data)
{
  gchar *barcode = g_strdup (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_buy_barcode"))));
  gdouble ingresa = strtod (PUT (g_strdup (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_buy_price"))))), (char **)NULL);
  gdouble ganancia = strtod (PUT (g_strdup (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_buy_gain"))))), (char **)NULL);
  gdouble precio_final = strtod (PUT (g_strdup (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_sell_price"))))), (char **)NULL);

  gchar *q = g_strdup_printf ("SELECT estado, tipo FROM producto where barcode = '%s'", barcode);
  gchar *materia_prima = g_strdup (PQvaluebycol 
				   (EjecutarSQL ("SELECT id FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'MATERIA PRIMA'"), 0, "id"));
  gchar *tipo = g_strdup (PQvaluebycol (EjecutarSQL(q), 0, "tipo"));

  //Si es materia prima no se realiza un cálculo
  if (g_str_equal (tipo, materia_prima))
    {
      gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "entry_buy_price")), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "button_calculate")), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "entry_buy_amount")), TRUE);
      gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "button_add_product_list")), TRUE);
      return;
    }

  if (ingresa == 0 && ganancia == 0 && precio_final == 0)
    {
      ErrorMSG (GTK_WIDGET (gtk_builder_get_object (builder, "entry_buy_price")),
                "Ingrese los valores a calcular");
      return;
    }
  else
    {
      CalcularPrecioFinal();
    }
}


/**
 * Es llamada cuando el boton "button_add_product_list" es presionado (signal click).
 *
 * Esta funcion llama a la funcion  AddToProductsList.
 *
 * @param button the button
 * @param user_data the user data
 */
void
on_button_add_product_list_clicked (GtkButton *button, gpointer data)
{
  gdouble unidades;
  gchar *unidades_text;

  unidades_text = g_strdup (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_buy_amount"))));
  unidades = strtod (PUT(unidades_text), (char **)NULL);

  if (unidades <= 0 || !is_numeric (unidades_text))
    {
      ErrorMSG (GTK_WIDGET (gtk_builder_get_object (builder, "entry_buy_amount")),
                "Ingrese una cantidad válida de unidades");
      return;
    }
  else
    {      
      calcular = 0;
      AddToProductsList ();
    }
}


void
on_entry_sell_price_activate (GtkEntry *entry)
{
  gboolean modo_inventario;
  modo_inventario = rizoma_get_value_boolean ("MODO_INVENTARIO");
  
  if (modo_inventario == FALSE)
    gtk_widget_grab_focus (GTK_WIDGET (builder_get (builder, "button_calculate")));
  else
    {
      on_button_calculate_clicked (NULL, NULL);
      if (g_str_equal (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_buy_amount"))), "1"))
	gtk_widget_grab_focus (GTK_WIDGET (builder_get (builder, "entry_buy_amount")));
      else
	on_button_add_product_list_clicked (NULL, NULL);
    }
}


/**
 * Inicializa una ventana de creación de mercadería
 * dependiendo del tipo que desee crear
 * 
 * @param gchar tipo: tipo de mercadería a crear ("md", "mp", "mc")
 * mercadería corriente, materia prima, mercadería derivada
 *
 */
void
create_new_merchandise (gchar *tipo)
{
  gchar *barcode = g_strdup (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_buy_barcode"))));
  gchar *cod_corto;
  gchar *barcode_madre;
  Productos *prod;
  GtkWidget *ventana, *barcode_w, *codigo_corto_w, *descripcion_w, *marca_w, 
    *contenido_w, *radio_fraccion_no, *radio_fraccion_yes, *radio_imp, *radio_imp_no;

  GtkComboBox *combo, *cmb_unit, *cmb_family;

  /*Según el tipo de mercadería que se desee crear*/
  if (g_str_equal (tipo, "md")) //Si es mercadería corriente
    {
      combo = GTK_COMBO_BOX (builder_get (builder, "cmbbox_new_product_imp_others"));
      cmb_unit = GTK_COMBO_BOX (gtk_builder_get_object(builder, "cmb_box_new_product_unit"));
      cmb_family = GTK_COMBO_BOX (gtk_builder_get_object(builder, "cmb_new_product_family"));

      barcode_w = GTK_WIDGET (builder_get (builder, "entry_new_product_barcode"));
      codigo_corto_w = GTK_WIDGET (builder_get (builder, "entry_new_product_code"));
      descripcion_w = GTK_WIDGET (builder_get (builder, "entry_new_product_desc"));
      marca_w = GTK_WIDGET (builder_get (builder, "entry_new_product_brand"));
      contenido_w = GTK_WIDGET (builder_get (builder, "entry_new_product_cont"));
      
      radio_fraccion_no = GTK_WIDGET (builder_get (builder, "radio_btn_fractional_no"));
      radio_imp = GTK_WIDGET (builder_get (builder, "radio_btn_task_yes"));

      ventana = GTK_WIDGET (builder_get (builder, "wnd_new_product"));
    }
  else if (g_str_equal (tipo, "mp")) //Si es materia prima
    {
      combo = GTK_COMBO_BOX (builder_get (builder, "cmbbox_new_mp_imp_others"));
      cmb_unit = GTK_COMBO_BOX (gtk_builder_get_object (builder, "cmb_box_new_mp_unit"));
      cmb_family = GTK_COMBO_BOX (gtk_builder_get_object (builder, "cmb_new_mp_family"));
      
      barcode_w = GTK_WIDGET (builder_get (builder, "entry_new_mp_barcode"));
      codigo_corto_w = GTK_WIDGET (builder_get (builder, "entry_new_mp_code"));
      descripcion_w = GTK_WIDGET (builder_get (builder, "entry_new_mp_desc"));
      marca_w = GTK_WIDGET (builder_get (builder, "entry_new_mp_brand"));
      contenido_w = GTK_WIDGET (builder_get (builder, "entry_new_mp_cont"));
      
      radio_fraccion_no = GTK_WIDGET (builder_get (builder, "radio_btn_mp_fractional_no"));
      radio_imp = GTK_WIDGET (builder_get (builder, "radio_btn_mp_task_yes"));

      ventana = GTK_WIDGET (builder_get (builder, "wnd_new_mp"));
    }
  else if (g_str_equal (tipo, "mcd")) //Si es mercadería derivada
    {
      barcode_madre = g_strdup (gtk_label_get_text (GTK_LABEL (builder_get (builder, "lbl_barcode_mp"))));
      prod = CreateNew (barcode_madre, 1);
      
      combo = GTK_COMBO_BOX (builder_get (builder, "cmbbox_new_mcd_imp_others"));
      cmb_unit = GTK_COMBO_BOX (gtk_builder_get_object (builder, "cmb_box_new_mcd_unit"));
      cmb_family = GTK_COMBO_BOX (gtk_builder_get_object (builder, "cmb_new_mcd_family"));
      
      barcode_w = GTK_WIDGET (builder_get (builder, "entry_new_mcd_barcode"));
      codigo_corto_w = GTK_WIDGET (builder_get (builder, "entry_new_mcd_code"));
      descripcion_w = GTK_WIDGET (builder_get (builder, "entry_new_mcd_desc"));
      marca_w = GTK_WIDGET (builder_get (builder, "entry_new_mcd_brand"));      
      contenido_w = GTK_WIDGET (builder_get (builder, "entry_new_mcd_cont"));
      
      radio_fraccion_yes = GTK_WIDGET (builder_get (builder, "radio_btn_mcd_fractional_yes"));
      radio_fraccion_no = GTK_WIDGET (builder_get (builder, "radio_btn_mcd_fractional_no"));
      radio_imp = GTK_WIDGET (builder_get (builder, "radio_btn_mcd_task_yes"));
      radio_imp_no = GTK_WIDGET (builder_get (builder, "radio_btn_mcd_task_no"));

      ventana = GTK_WIDGET (builder_get (builder, "wnd_new_mcd"));
    }

  clean_container (GTK_CONTAINER (ventana));

  gtk_entry_set_text (GTK_ENTRY (barcode_w), barcode);

  //Se rellenan los campos de barcode y codigo_corto con las información correspondiente
  if (strlen (barcode) > 0 && strlen (barcode) <= 6)
    {
      cod_corto = g_strdup (barcode);
      
      if (!codigo_disponible (cod_corto))
	{
	  cod_corto = sugerir_codigo (cod_corto, 4, 6);
	  gtk_entry_set_text (GTK_ENTRY (codigo_corto_w), cod_corto);
	}
      else
	gtk_entry_set_text (GTK_ENTRY (codigo_corto_w), cod_corto);

      /* - Sugerencia de barcode - */
      barcode = sugerir_codigo (barcode, 6, 18);

      //Se setea el campo del barcode con esta sugerencia
      gtk_entry_set_text (GTK_ENTRY (barcode_w), g_strdup (barcode));

      gtk_widget_grab_focus (GTK_WIDGET (descripcion_w));
    }
  else
    {
      /*El proceso de creación de materias derivadas debe sugerir un barcode*/
      if (g_str_equal (tipo, "mcd"))
	{
	  if (!codigo_disponible (barcode))
	    {
	      barcode = sugerir_codigo (barcode, 6, 18);
	      gtk_entry_set_text (GTK_ENTRY (barcode_w), barcode);
	    }
	  else
	    gtk_entry_set_text (GTK_ENTRY (barcode_w), barcode);
	}

      //Se sugiere un codigo corto
      cod_corto = g_strndup (barcode, 5);
      if (!codigo_disponible (cod_corto))
	{
	  cod_corto = sugerir_codigo (cod_corto, 4, 6);
	  gtk_entry_set_text (GTK_ENTRY (codigo_corto_w), cod_corto);
	}
      else
	gtk_entry_set_text (GTK_ENTRY (codigo_corto_w), cod_corto);

      gtk_widget_grab_focus (GTK_WIDGET (codigo_corto_w));
    }


  /*Se rellenan los combobox impuestos, unidad y familia*/
  if (g_str_equal (tipo, "mcd"))
    {
      fill_combo_impuestos (combo, prod->product->otros_id);
      fill_combo_unidad (cmb_unit, prod->product->unidad);
      fill_combo_familias (cmb_family, prod->product->familia);
    }
  else
    {
      fill_combo_impuestos (combo, 0);
      fill_combo_unidad (cmb_unit, "");
      fill_combo_familias (cmb_family, 0);
    }

  /*La mercadería derivada hereda datos de su madre*/
  if (g_str_equal (tipo, "mcd"))
    {
      gtk_entry_set_text (GTK_ENTRY (marca_w), prod->product->marca);
      gtk_entry_set_text (GTK_ENTRY (contenido_w), g_strdup_printf ("%d", prod->product->contenido));
      //gtk_entry_set_text (GTK_ENTRY (descripcion_w), prod->product->producto);
      gtk_widget_grab_focus ((descripcion_w));
    }

  //Seleccion radiobutton
  if (g_str_equal (tipo, "mcd"))
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio_fraccion_yes), (prod->product->fraccion == TRUE) ? TRUE : FALSE);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio_fraccion_no), (prod->product->fraccion == FALSE) ? TRUE : FALSE);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio_imp), (prod->product->iva != 0) ? TRUE : FALSE);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio_imp_no), (prod->product->iva == 0) ? TRUE : FALSE);
    }
  else
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio_fraccion_no), TRUE);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio_imp), TRUE);
    }

  //Ventana creacion de productos
  gtk_entry_set_max_length (GTK_ENTRY (barcode_w), 18);
  gtk_entry_set_max_length (GTK_ENTRY (codigo_corto_w), 16);
  gtk_entry_set_max_length (GTK_ENTRY (marca_w), 20);
  gtk_entry_set_max_length (GTK_ENTRY (descripcion_w), 35);
  gtk_entry_set_max_length (GTK_ENTRY (contenido_w), 10);

  gtk_widget_show_all (ventana);
}


/**
 * Show de 'wnd_new_compuesta' window
 * To make a new merchandise
 */
void
create_new_compuesta (void)
{
  GtkWidget *ventana, *barcode_w, *codigo_corto_w, *descripcion_w, *marca_w, 
    *contenido_w; // *radio_fraccion_no;
  GtkComboBox *cmb_family, *cmb_unit;
  gchar *barcode, *cod_corto;

  //Ventana
  ventana = GTK_WIDGET (builder_get (builder, "wnd_new_compuesta"));
  clean_container (GTK_CONTAINER (ventana));

  //Widgets
  barcode_w = GTK_WIDGET (builder_get (builder, "entry_new_comp_barcode"));
  codigo_corto_w = GTK_WIDGET (builder_get (builder, "entry_new_comp_code"));
  descripcion_w = GTK_WIDGET (builder_get (builder, "entry_new_comp_desc"));
  marca_w = GTK_WIDGET (builder_get (builder, "entry_new_comp_brand"));
  contenido_w = GTK_WIDGET (builder_get (builder, "entry_new_comp_cont"));

  //Rellenar Familia
  cmb_family = GTK_COMBO_BOX (gtk_builder_get_object (builder, "cmb_new_comp_family"));
  fill_combo_familias (cmb_family, 0);
  //Rellenar Cantidad
  cmb_unit = GTK_COMBO_BOX (gtk_builder_get_object (builder, "cmb_new_comp_unit"));
  fill_combo_unidad (cmb_unit, "");

  //Radio Buttons
  //radio_fraccion_no = GTK_WIDGET (builder_get (builder, "radio_btn_comp_fractional_no"));
  //gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio_fraccion_no), TRUE);

  //Ventana creacion de productos
  gtk_entry_set_max_length (GTK_ENTRY (barcode_w), 18);
  gtk_entry_set_max_length (GTK_ENTRY (codigo_corto_w), 16);
  gtk_entry_set_max_length (GTK_ENTRY (descripcion_w), 25);
  gtk_entry_set_max_length (GTK_ENTRY (marca_w), 20);
  gtk_entry_set_max_length (GTK_ENTRY (contenido_w), 10);

  //Contenido
  barcode = g_strdup (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_buy_barcode"))));
  gtk_entry_set_text (GTK_ENTRY (barcode_w), barcode);

  //Se ve si es barcode o codigo_corto
  if (strlen (barcode) > 0 && strlen (barcode) <= 6)
    {
      cod_corto = g_strdup (barcode);
      
      if (!codigo_disponible (cod_corto))
	{
	  cod_corto = sugerir_codigo (cod_corto, 4, 6);
	  gtk_entry_set_text (GTK_ENTRY (codigo_corto_w), cod_corto);
	}
      else
	gtk_entry_set_text (GTK_ENTRY (codigo_corto_w), cod_corto);

      /* - Sugerencia de barcode - */
      barcode = sugerir_codigo (barcode, 6, 18);

      //Se setea el campo del barcode con esta sugerencia
      gtk_entry_set_text (GTK_ENTRY (barcode_w), g_strdup (barcode));

      gtk_widget_grab_focus (GTK_WIDGET (descripcion_w));
    }
  else
    gtk_widget_grab_focus (GTK_WIDGET (codigo_corto_w));

  gtk_widget_show (ventana);
}


/**
 * This callback is triggered when a cell on 
 * 'treeview_sizes' is edited. (editable-event)
 * @param cell
 * @param path_string
 * @param new_size
 * @param data -> A GtkListStore
 */
void
on_sizes_cell_renderer_edited (GtkCellRendererText *cell, gchar *path_string, gchar *new_size, gpointer data)
{
  GtkTreeModel *model = GTK_TREE_MODEL (data);
  GtkTreePath *path = gtk_tree_path_new_from_string (path_string);
  GtkTreeIter iter;
  
  /* obtiene el iter para poder obtener y setear 
     datos del treeview */
  gtk_tree_model_get_iter (model, &iter, path);

  // Se setean los datos modificados en el treeview
  gtk_list_store_set (GTK_LIST_STORE (model), &iter,
		      0, FALSE,    // False indica que es un dato agregado a mano
		      2, new_size, // Se le inserta la nueva talla
		      -1);
}


/**
 * This callback is triggered when a cell on 
 * 'treeview_sizes' is edited. (editable-event)
 *
 * @param cell
 * @param path_string
 * @param new_size
 * @param data -> A GtkListStore
 */
void
on_sizes_code_cell_renderer_edited (GtkCellRendererText *cell, gchar *path_string, gchar *new_size, gpointer data)
{
  GtkTreeModel *model = GTK_TREE_MODEL (data);
  GtkTreePath *path = gtk_tree_path_new_from_string (path_string);
  GtkTreeIter iter;
  
  /* obtiene el iter para poder obtener y setear 
     datos del treeview */
  gtk_tree_model_get_iter (model, &iter, path);

  //Se asegura que la talla agregada contenga 2 dígitos
  if (strlen (new_size) != 2)
    {
      ErrorMSG (GTK_WIDGET (builder_get (builder, "treeview_sizes")),
		"El código de talla debe contener 2 dígitos");
      return;
    }
  
  // Se setean los datos modificados en el treeview
  gtk_list_store_set (GTK_LIST_STORE (model), &iter,
		      0, FALSE,    // False indica que es un dato agregado a mano
		      1, new_size, // Se le inserta la nueva talla
		      -1);
}


/** This function is a callback from
 * 'btn_add_new_family_clicked' button (signal click)
 *
 * Save a new family
 *
 * @param button the button
 * @param user_data the user data
 */
void
save_new_family (GtkButton *button, gpointer user_data)
{
  GtkComboBox *combo;
  GtkWindow *parent, *win;
  GtkButton *btn;
  //GtkListStore *store;

  PGresult *res;
  gint tuples;
  gchar *familia = g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_new_family"))));
  gchar *tipo = g_strdup (gtk_buildable_get_name (GTK_BUILDABLE (button)));

  win = GTK_WINDOW (gtk_builder_get_object(builder, "wnd_new_family"));

  /*Al editar o agregar una mercadería*/
  if (g_str_equal (tipo, "btn_save_family_amd")) //Mercadería corriente
    {
      parent = GTK_WINDOW (gtk_builder_get_object(builder, "wnd_new_family"));
      gtk_window_set_transient_for(win, parent);
      combo = GTK_COMBO_BOX (gtk_builder_get_object(builder, "cmb_new_product_family"));
      btn = GTK_BUTTON (gtk_builder_get_object(builder, "btn_new_family"));
    }
  else if (g_str_equal (tipo, "btn_save_family_amp")) //Materia prima
    {
      parent = GTK_WINDOW (gtk_builder_get_object(builder, "wnd_new_family"));
      gtk_window_set_transient_for(win, parent);
      combo = GTK_COMBO_BOX (gtk_builder_get_object(builder, "cmb_new_mp_family"));
      btn = GTK_BUTTON (gtk_builder_get_object(builder, "btn_new_mp_family"));
    }
  else if (g_str_equal (tipo, "btn_save_family_amc")) //derivada
    {
      parent = GTK_WINDOW (gtk_builder_get_object (builder, "wnd_new_family"));
      gtk_window_set_transient_for (win, parent);
      combo = GTK_COMBO_BOX (gtk_builder_get_object (builder, "cmb_new_mcd_family"));
      btn = GTK_BUTTON (gtk_builder_get_object(builder, "btn_new_mcd_family"));
    }
  else if (g_str_equal (tipo, "btn_save_family_amcc")) //Compuesta
    {
      parent = GTK_WINDOW (gtk_builder_get_object (builder, "wnd_new_family"));
      gtk_window_set_transient_for (win, parent);
      combo = GTK_COMBO_BOX (gtk_builder_get_object (builder, "cmb_new_comp_family"));
      btn = GTK_BUTTON (gtk_builder_get_object (builder, "btn_new_amcc_family"));
    }
  else if (g_str_equal (tipo, "btn_save_family_em")) //Editar Mercadería (cualquiera)
    {
      parent = GTK_WINDOW (gtk_builder_get_object(builder, "wnd_mod_product"));
      gtk_window_set_transient_for(win, parent);
      combo = GTK_COMBO_BOX (gtk_builder_get_object(builder, "cmb_edit_product_family"));
      btn = GTK_BUTTON (gtk_builder_get_object(builder, "btn_edit_family"));
    }

  gchar *q = g_strdup_printf ("SELECT * FROM familias WHERE nombre = upper('%s')", familia);
  res = EjecutarSQL (q);
  tuples = PQntuples (res);

  if (tuples > 0)
    {
      gtk_widget_hide (GTK_WIDGET (win));
      gtk_entry_set_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_new_family")), "");
      ErrorMSG (GTK_WIDGET (btn), "La familia que ingresó ya existe.");
    }
  else
    {
      q = g_strdup_printf ("INSERT INTO familias VALUES (DEFAULT, upper('%s'))", familia);
      res = EjecutarSQL(q);

      res = EjecutarSQL ("SELECT * FROM familias");
      tuples = PQntuples (res);

      //store = GTK_LIST_STORE (gtk_combo_box_get_model (combo));

      //gtk_list_store_clear (store);
      fill_combo_familias (combo, tuples);

      gtk_widget_hide (GTK_WIDGET (win));
      gtk_entry_set_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_new_family")), "");
    }
}

/**
 * This function is a callback from
 * 'btn_show_new_family_win' button (signal click)
 *
 * clean and show 'wnd_new_family' window
 *
 * @param button the button
 * @param user_data the user data
 */
void
on_btn_new_family_clicked (GtkButton *button, gpointer user_data)
{
  gchar *nombre_boton = g_strdup (gtk_buildable_get_name (GTK_BUILDABLE (button)));
  
  gtk_entry_set_text(GTK_ENTRY (gtk_builder_get_object (builder, "entry_new_family")), "");
  gtk_widget_show_all (GTK_WIDGET (builder_get (builder, "wnd_new_family")));

  //Se ocultan todos los botones
  gtk_widget_hide (GTK_WIDGET (builder_get (builder, "btn_save_family_amd"))); //En Agregar Mercadería Corriente (discreta)
  gtk_widget_hide (GTK_WIDGET (builder_get (builder, "btn_save_family_amp"))); //En Agregar Materia Prima
  gtk_widget_hide (GTK_WIDGET (builder_get (builder, "btn_save_family_amc"))); //En Agregar MerCadería derivada
  gtk_widget_hide (GTK_WIDGET (builder_get (builder, "btn_save_family_amcc"))); //En Agregar MerCadería Compuesta

  gtk_widget_hide (GTK_WIDGET (builder_get (builder, "btn_save_family_em"))); //En editar mercadería

  //Se muestra solo el botón guardar que corresponda
  if (g_str_equal (nombre_boton, "btn_new_family")) //Ventana nueva mercadería corriente (discreta)
    gtk_widget_show (GTK_WIDGET (builder_get (builder, "btn_save_family_amd")));
  else if (g_str_equal (nombre_boton, "btn_new_mp_family")) //Ventana nueva materia prima
    gtk_widget_show (GTK_WIDGET (builder_get (builder, "btn_save_family_amp")));
  else if (g_str_equal (nombre_boton, "btn_new_mcd_family")) //Ventana nueva mercadería derivada
    gtk_widget_show (GTK_WIDGET (builder_get (builder, "btn_save_family_amc")));
  else if (g_str_equal (nombre_boton, "btn_new_amcc_family")) //Ventana nueva mercadería compuesta
    gtk_widget_show (GTK_WIDGET (builder_get (builder, "btn_save_family_amcc")));

  else if (g_str_equal (nombre_boton, "btn_edit_family")) //Ventana editar mercadería (el que sea)
    gtk_widget_show (GTK_WIDGET (builder_get (builder, "btn_save_family_em")));
}

/**
 * This function is a callback from
 * 'btn_new_unidad' or 'btn_edit_unidad' 
 *  button (signal click)
 *
 * clean and show 'wnd_new_unit' window
 * and hide the corresponding save button.
 *
 * @param button the button
 * @param user_data the user data
 */
void
show_new_unidad_win_clicked (GtkButton *button, gpointer user_data)
{
  gchar *nombre_boton = g_strdup (gtk_buildable_get_name (GTK_BUILDABLE (button)));

  gtk_entry_set_text(GTK_ENTRY (gtk_builder_get_object (builder, "entry_unidad")), "");
  gtk_widget_show (GTK_WIDGET (builder_get (builder, "wnd_new_unit")));

  //Se ocultan todos los botones guardar
  gtk_widget_hide (GTK_WIDGET (builder_get (builder, "btn_save_unidad_amd"))); //En agregar mercadería corriente (discreta)
  gtk_widget_hide (GTK_WIDGET (builder_get (builder, "btn_save_unidad_amp"))); //En agregar materia prima
  gtk_widget_hide (GTK_WIDGET (builder_get (builder, "btn_save_unidad_amc"))); //En Agregar MerCadería derivada
  gtk_widget_hide (GTK_WIDGET (builder_get (builder, "btn_save_unidad_amcc"))); //En Agregar MerCadería Compuesta

  gtk_widget_hide (GTK_WIDGET (builder_get (builder, "btn_save_unidad_em"))); //En editar mercadería

  //Se muestra solo el botón guardar que corresponda
  if (g_str_equal (nombre_boton, "btn_new_unidad")) //Ventana nueva mercadería corriente (discreta)
    gtk_widget_show (GTK_WIDGET (builder_get (builder, "btn_save_unidad_amd")));
  else if (g_str_equal (nombre_boton, "btn_new_mp_unidad")) //Ventana nueva materia prima
    gtk_widget_show (GTK_WIDGET (builder_get (builder, "btn_save_unidad_amp")));
  else if (g_str_equal (nombre_boton, "btn_new_mcd_unidad")) //Ventana nueva mercadería derivada
    gtk_widget_show (GTK_WIDGET (builder_get (builder, "btn_save_unidad_amc")));
  else if (g_str_equal (nombre_boton, "btn_new_mcc_unidad")) //Ventana nueva mercadería compuesta
    gtk_widget_show (GTK_WIDGET (builder_get (builder, "btn_save_unidad_amcc")));

  else if (g_str_equal (nombre_boton, "btn_edit_unidad")) //Ventana editar mercadería (el que sea)
    gtk_widget_show (GTK_WIDGET (builder_get (builder, "btn_save_unidad_em")));
}


/**
 * This callback is triggeres by 'btn_create_codes' button
 * 
 * Get all the pieces of code and creates the corresponding
 * codes from them.
 *
 * @param button the button
 * @param user_data the user data
 */
void
on_btn_create_codes_clicked (GtkButton *button, gpointer data)
{
  GtkTreeView *treeview;
  GtkTreeModel *model;
  GtkListStore *store;
  GtkTreeIter iter;
  gboolean valid;

  gint i, j, partida_color, partida_size, num_codes;
  gboolean existente, base_existente;
  gint num_sizes = get_treeview_length (GTK_TREE_VIEW (builder_get (builder, "treeview_sizes")));
  gint num_colors = get_treeview_length (GTK_TREE_VIEW (builder_get (builder, "treeview_colors")));
  gchar *sizes[num_sizes];
  gchar *colors[num_colors];
  gint new_colors, new_sizes;
  new_colors = new_sizes = num_codes = 0;
  partida_color = partida_size = 0;

  /*Gets all new sizes*/
  treeview = GTK_TREE_VIEW (builder_get (builder, "treeview_sizes"));
  model = gtk_tree_view_get_model (treeview);
  store = GTK_LIST_STORE (model);
  valid = gtk_tree_model_get_iter_first (model, &iter);
  i = 0;
  while (valid)
    {
      gtk_tree_model_get (model, &iter,
			  0, &existente,
			  1, &sizes[i],
			  -1);

      if (existente == FALSE)
	new_sizes++;
      
      i++;
      valid = gtk_tree_model_iter_next (model, &iter); /* Me da TRUE si itera a la siguiente */
    }  
    
  /*Gets all new colors*/
  treeview = GTK_TREE_VIEW (builder_get (builder, "treeview_colors"));
  model = gtk_tree_view_get_model (treeview);
  store = GTK_LIST_STORE (model);
  valid = gtk_tree_model_get_iter_first (model, &iter);
  i = 0;
  while (valid)
    {
      gtk_tree_model_get (model, &iter,
			  0, &existente,
			  1, &colors[i],
			  -1);

      if (existente == FALSE)
	new_colors++;

      i++;
      valid = gtk_tree_model_iter_next (model, &iter); /* Me da TRUE si itera a la siguiente */
    }

  /*-Creating codes -*/

  /*get fragments*/
  gchar *code_base = g_strdup ("");
  GObject *clothes_base_code[5];
  clothes_base_code[0] = builder_get (builder, "entry_clothes_depto");
  clothes_base_code[1] = builder_get (builder, "entry_clothes_temp");
  clothes_base_code[2] = builder_get (builder, "entry_clothes_year");
  clothes_base_code[3] = builder_get (builder, "entry_clothes_sub_depto");
  clothes_base_code[4] = builder_get (builder, "entry_clothes_id");

  for (i=0; i<5; i++)
    code_base = g_strdup_printf ("%s%s", code_base, gtk_entry_get_text (GTK_ENTRY (clothes_base_code[i])));
  
  /*codigos + tamaños*/
  gchar *code_sizes[num_sizes];
  for (i=0; i<num_sizes; i++)
    {
      code_sizes[i] = g_strdup_printf ("%s%s", code_base, sizes[i]);
    }
 
  // (si hay nuevas tallas o colores y hay a lo menos 1 de ambos) o
  // (si el codigo base es nuevo y existe a lo menos 1 color y talla)
  base_existente = DataExist (g_strdup_printf ("SELECT * FROM clothes_code WHERE codigo_corto like '%s%%'",code_base));

  if (((new_sizes > 0 || new_colors > 0) && (num_sizes > 0 && num_colors > 0)) ||
      (!base_existente && (num_sizes > 0 && num_colors > 0)))
    {
      //Solo se deshabilita si no tiene texto en el
      if (g_str_equal (gtk_entry_get_text 
		       (GTK_ENTRY (builder_get (builder, "entry_new_clothes_brand"))), ""))
	gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "entry_new_clothes_brand")), TRUE);

      //Solo se deshabilita si no tiene texto en el
      if (g_str_equal (gtk_entry_get_text 
		       (GTK_ENTRY (builder_get (builder, "entry_new_clothes_desc"))), ""))
	gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "entry_new_clothes_desc")), TRUE);

      gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_add_new_clothes")), TRUE);
    }
  else
    {
      if (!base_existente)
	ErrorMSG (GTK_WIDGET (builder_get (builder, "btn_add_size")), 
		  "Debe ingresar a lo menos una talla y un color");
      else
	ErrorMSG (GTK_WIDGET (builder_get (builder, "btn_add_size")), 
		  "Debe ingresar a lo menos una talla o un color nuevo");
      return;
    }

  /*Se rellena el treeview de codigos cortos*/
  treeview = GTK_TREE_VIEW (builder_get (builder, "treeview_shortcodes"));
  model = gtk_tree_view_get_model (treeview);
  store = GTK_LIST_STORE (model);

  /*Quitamos solamente los codigos nuevos*/
  num_codes = get_treeview_length (treeview);
  i=0;
  valid = gtk_tree_model_get_iter_first (model, &iter);
  while (valid && i < num_codes)
    {
      gtk_tree_model_get (model, &iter,
			  0, &existente,
			  -1);

      if (existente == FALSE)
	gtk_list_store_remove (store, &iter);
      else
	valid = gtk_tree_model_iter_next (model, &iter); /* Me da TRUE si itera a la siguiente */
      i++;
    }

  if (base_existente == FALSE)
    {
      store = GTK_LIST_STORE (model);
      for (i=0; i<num_sizes; i++)
	{
	  for (j=0; j<num_colors; j++)
	    {
	      gtk_list_store_append (store, &iter);
	      gtk_list_store_set (store, &iter,
				  0, FALSE,
				  1, g_strdup_printf ("%s%s", code_sizes[i], colors[j]),
				  -1);	      
	    }
	}
    }
  else /*Agregamos nuevos codigos*/
    {
      if (new_sizes > 0 && new_colors == 0)
	{
	  partida_size = num_sizes - new_sizes;
	  store = GTK_LIST_STORE (model);
	  for (i=partida_size; i<num_sizes; i++)
	    {
	      for (j=0; j<num_colors; j++)
		{
		  gtk_list_store_append (store, &iter);
		  gtk_list_store_set (store, &iter,
				      0, FALSE,
				      1, g_strdup_printf ("%s%s", code_sizes[i], colors[j]),
				      -1);	      
		}
	    }
	}
      else if (new_colors > 0 && new_sizes == 0)
	{
	  partida_color = num_colors - new_colors;
	  store = GTK_LIST_STORE (model);
	  for (i=0; i<num_sizes; i++)
	    {
	      for (j=partida_color; j<num_colors; j++)
		{
		  gtk_list_store_append (store, &iter);
		  gtk_list_store_set (store, &iter,
				      0, FALSE,
				      1, g_strdup_printf ("%s%s", code_sizes[i], colors[j]),
				      -1);	      
		}
	    }
	}
      else if (new_colors > 0 && new_sizes > 0)
	{
	  partida_size = num_sizes - new_sizes;;
	  partida_color = num_colors - new_colors;
	  store = GTK_LIST_STORE (model);
	  for (i=partida_size; i<num_sizes; i++)
	    {
	      for (j=partida_color; j<num_colors; j++)
		{
		  gtk_list_store_append (store, &iter);
		  gtk_list_store_set (store, &iter,
				      0, FALSE,
				      1, g_strdup_printf ("%s%s", code_sizes[i], colors[j]),
				      -1);	      
		}
	    }
	}


    }
}


/**
 * This callback is triggeres by 'btn_add_clothes' button
 * 
 * Create clothes from information on 'wnd_new_clothing'
 *
 * @param button the button
 * @param user_data the user data
 */
void
on_btn_add_new_clothes_clicked (GtkButton *button, gpointer data)
{
  GtkTreeView *treeview;
  GtkTreeModel *model;
  //GtkListStore *store;
  GtkTreeIter iter;
  gboolean valid;
  
  gboolean registrado; // Para saber si es un codigo previamente registrado
  gchar *marca, *descripcion, *sub_depto_code;
  gchar *tipo;
  gint i, fila, largo;
  i = fila = 0;

  sub_depto_code = g_strdup (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_clothes_sub_depto"))));
  marca = g_strdup (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_new_clothes_brand"))));
  descripcion = g_strdup (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_new_clothes_desc"))));
  
  //Se comprueba que tengan datos
  if (g_str_equal (marca, ""))
    {
      ErrorMSG (GTK_WIDGET (builder_get (builder, "entry_new_clothes_brand")),
		"Debe ingresar una marca");
      return;
    }
  else if (g_str_equal (descripcion, ""))
    {
      ErrorMSG (GTK_WIDGET (builder_get (builder, "entry_new_clothes_desc")),
		"Debe ingresar una descripción");
      return;
    }

  /* Se registra la información del subdepartamento 
     (se asocia 'descripcion') al fragmento sub_depto */
  registrar_nuevo_sub_depto (sub_depto_code, descripcion);
  

  // Se obtienen todas las tallas
  fila = 0;
  treeview = GTK_TREE_VIEW (builder_get (builder, "treeview_sizes"));
  
  largo = get_treeview_length (treeview);
  gchar *talla, *codigo_talla;
  gchar *tallas[largo], *codigos_tallas[largo];

  model = gtk_tree_view_get_model (treeview);
  //store = GTK_LIST_STORE (model);
  valid = gtk_tree_model_get_iter_first (model, &iter);
  while (valid)
    {
      gtk_tree_model_get (model, &iter,
			  0, &registrado,
			  1, &codigo_talla,
			  2, &talla,
			  -1);

      if (registrado == FALSE)
	{
	  codigos_tallas[fila] = g_strdup (codigo_talla);
	  tallas[fila] = g_strdup (talla);
	  fila++;
	}

      valid = gtk_tree_model_iter_next (model, &iter); /* Me da TRUE si itera a la siguiente */
    }

  // Se registran las tallas nuevas
  for (i=0; i<fila; i++)
    registrar_nueva_talla (g_strdup_printf ("%s", codigos_tallas[i]),
			   g_strdup_printf ("%s", tallas[i]));


  // Se obtienen todos los colores
  fila = 0;
  treeview = GTK_TREE_VIEW (builder_get (builder, "treeview_colors"));
  
  largo = get_treeview_length (treeview);
  gchar *color, *codigo_color;
  gchar *colores[largo], *codigos_colores[largo];

  model = gtk_tree_view_get_model (treeview);
  //store = GTK_LIST_STORE (model);
  valid = gtk_tree_model_get_iter_first (model, &iter);
  while (valid)
    {
      gtk_tree_model_get (model, &iter,
			  0, &registrado,
			  1, &codigo_color,
			  2, &color,
			  -1);

      if (registrado == FALSE)
	{
	  codigos_colores[fila] = g_strdup (codigo_color);
	  colores[fila] = g_strdup (color);
	  fila++;
	}

      valid = gtk_tree_model_iter_next (model, &iter); /* Me da TRUE si itera a la siguiente */
    }

  // Se registran los colores nuevos
  for (i=0; i<fila; i++)
    registrar_nuevo_color (g_strdup_printf ("%s", codigos_colores[i]),
			   g_strdup_printf ("%s", colores[i]));

  
  // Se obtienen todos los códigos generados 
  fila = 0;
  treeview = GTK_TREE_VIEW (builder_get (builder, "treeview_shortcodes"));

  largo = get_treeview_length (treeview);
  gchar *codigos[largo]; // Para guardar todos los codigos nuevos
  gchar *code;           // Para guardar el codigo del treeview

  model = gtk_tree_view_get_model (treeview);
  //store = GTK_LIST_STORE (model);
  valid = gtk_tree_model_get_iter_first (model, &iter);
  while (valid)
    {
      gtk_tree_model_get (model, &iter,
			  0, &registrado,
			  1, &code,
			  -1);

      if (registrado == FALSE)
	{
	  codigos[fila] = g_strdup (code);
	  fila++;
	}
      valid = gtk_tree_model_iter_next (model, &iter); /* Me da TRUE si itera a la siguiente */
    }

  // Se registran todos los codigos nuevos que se han generado
  for (i=0; i<fila; i++)
    registrar_nuevo_codigo (g_strdup_printf ("%s", codigos[i]));


  //Tipo producto (corriente)
  tipo = PQvaluebycol (EjecutarSQL ("SELECT id FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'CORRIENTE'"), 0, "id");

  // Se registran los productos
  for (i=0; i<fila; i++)
    AddNewProductToDB (codigos[i], "0", descripcion, marca, "1",
		       "UN", TRUE, 0, 1, FALSE, FALSE, atoi (tipo));
  
  clean_container (GTK_CONTAINER (gtk_builder_get_object (builder, "wnd_new_clothing")));
  gtk_widget_hide (GTK_WIDGET (gtk_builder_get_object (builder, "wnd_new_clothing")));
}


/**
 * This callback is triggered when a cell on 
 * 'treeview_sizes' is edited. (editable-event)
 * @param cell
 * @param path_string
 * @param new_size
 * @param data -> A GtkListStore
 */
void
on_code_color_cell_renderer_edited (GtkCellRendererText *cell, gchar *path_string, gchar *new_c_color, gpointer data)
{
  GtkTreeModel *model = GTK_TREE_MODEL (data);
  GtkTreePath *path = gtk_tree_path_new_from_string (path_string);
  GtkTreeIter iter;
  
  /* obtiene el iter para poder obtener y setear 
     datos del treeview */
  gtk_tree_model_get_iter (model, &iter, path);

  //Se asegura que el código del color contenga 2 dígitos
  if (strlen (new_c_color) != 2)
    {
      ErrorMSG (GTK_WIDGET (builder_get (builder, "treeview_colors")),
		"El código del color debe contener 2 dígitos");
      return;
    }

  // Se setean los datos modificados en el treeview
  gtk_list_store_set (GTK_LIST_STORE (model), &iter,
		      0, FALSE,    // False indica que es un dato agregado a mano
		      1, new_c_color, // Se le inserta la nueva talla
		      -1);

}

/**
 * This callback is triggered when a cell on 
 * 'treeview_sizes' is edited. (editable-event)
 * @param cell
 * @param path_string
 * @param new_size
 * @param data -> A GtkListStore
 */
void
on_color_cell_renderer_edited (GtkCellRendererText *cell, gchar *path_string, gchar *new_color, gpointer data)
{
  GtkTreeModel *model = GTK_TREE_MODEL (data);
  GtkTreePath *path = gtk_tree_path_new_from_string (path_string);
  GtkTreeIter iter;
  
  /* obtiene el iter para poder obtener y setear 
     datos del treeview */
  gtk_tree_model_get_iter (model, &iter, path);
  
  // Se setean los datos modificados en el treeview
  gtk_list_store_set (GTK_LIST_STORE (model), &iter,
		      0, FALSE,    // False indica que es un dato agregado a mano
		      2, new_color, // Se le inserta la nueva talla
		      -1);

}


/**
 * Rellena el detalle del traspaso correspondiente a al
 * traspaso seleccionado
 */
void
on_selection_transfer_et_change (GtkTreeSelection *selection, gpointer data)
{
  GtkTreeView *treeview = GTK_TREE_VIEW (builder_get (builder, "treeview_transfers_detail_et"));
  GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model (treeview));

  GtkTreeModel *model = gtk_tree_view_get_model (gtk_tree_selection_get_tree_view (selection));
  GtkTreeIter iter;

  gint i, tuples, id_traspaso;
  gchar *q, *color;
  PGresult *res;

  gint tipo_compuesto, tipo_deriv, tipo_producto;

  /*tipo de mercaderia*/
  tipo_compuesto = atoi (g_strdup (PQvaluebycol (EjecutarSQL ("SELECT id FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'COMPUESTA'"), 0, "id")));
  tipo_deriv = atoi (g_strdup (PQvaluebycol (EjecutarSQL ("SELECT id FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'DERIVADA'"), 0, "id")));

  gtk_list_store_clear (store);
  if (gtk_tree_selection_get_selected (selection, NULL, &iter) == TRUE)
    {
      gtk_tree_model_get (model, &iter,
			  0, &id_traspaso,
			  -1);

      q = g_strdup_printf ("SELECT td.barcode, td.cantidad, td.precio, td.costo_modificado, t.origen, "
			   "       ROUND (td.cantidad * td.precio) AS subtotal, p.descripcion, p.tipo "
			   "FROM traspaso t "
			   "INNER JOIN traspaso_detalle td "
			   "ON t.id = td.id_traspaso "
			   "INNER JOIN (SELECT barcode, descripcion, tipo FROM producto) AS p "
			   "ON p.barcode = td.barcode "
			   "WHERE t.id = %d", id_traspaso);
      
      res = EjecutarSQL (q);
      g_free (q);
      tuples = PQntuples (res);
      
      for (i = 0; i < tuples; i++)
	{
	  tipo_producto = atoi (g_strdup (PQvaluebycol (res, i, "tipo")));
	  if (tipo_producto == tipo_compuesto || tipo_producto == tipo_deriv)
	    color = "#535353";
	  else
	    color = g_str_equal (PQvaluebycol (res, i, "costo_modificado"), "t") ? "#62932c" : "Black";

	  gtk_list_store_append (store, &iter);
	  gtk_list_store_set (store, &iter,
			      0, g_strdup (PQvaluebycol (res, i, "barcode")),
			      1, g_strdup (PQvaluebycol (res, i, "descripcion")),
			      2, strtod (PUT (g_strdup (PQvaluebycol (res, i, "cantidad"))), (char **)NULL),
			      3, strtod (PUT (g_strdup (PQvaluebycol (res, i, "precio"))), (char **)NULL),
			      4, atoi (g_strdup (PQvaluebycol (res, i, "subtotal"))),
			      5, tipo_producto,
			      6, id_traspaso,
			      7, atoi (g_strdup (PQvaluebycol (res, i, "origen"))),
			      8, g_str_equal (g_strdup (PQvaluebycol (res, i, "costo_modificado")), "t") ? TRUE : FALSE,
			      9, color,
			      10, TRUE,
			      -1);
	}
      
      PQclear (res);
    }
}


/**
 * This callback is triggered when a cell on 
 * 'treeview_transfers_detail_et' when
 * the cantity is edited. (editable-event)
 *
 * @param cell
 * @param path_string
 * @param new_price
 * @param data -> A GtkListStore
 */
void
on_mod_et_buy_price_cell_renderer_edited (GtkCellRendererText *cell, gchar *path_string, gchar *cost, gpointer data)
{
  //Treeview detalle de la compra
  GtkTreeView *treeview;
  GtkTreeModel *model;
  GtkTreePath *path;
  GtkTreeIter iter;
  
  //Variables para recoger información
  gint total;
  gint tipo;
  gint id_traspaso;
  gchar *barcode;
  gchar *color;
  gdouble new_cost;
  gdouble previous_cost;
  gdouble cantity;
  gboolean original;
  gboolean costo_editable;

  /*Original es FALSE hasta que se demuetre lo contrario*/
  original = FALSE;
  
  /*Tipo es MOD hasta que se demuestre los contrario*/
  tipo = MOD;

  /*Obtiene el modelo del treeview compra detalle*/
  model = GTK_TREE_MODEL (data);
  path = gtk_tree_path_new_from_string (path_string);

  /*obtiene el iter para poder obtener y setear datos del treeview*/
  gtk_tree_model_get_iter (model, &iter, path);
 
  //Se obtiene el el precio original
  gtk_tree_model_get (model, &iter,
		      0, &barcode,
		      2, &cantity,
  		      3, &previous_cost,
		      6, &id_traspaso,
		      8, &costo_editable,
                      -1);
  
  /*Valida y Obtiene el costo nuevo*/
  if (!costo_editable)
    {
      ErrorMSG (GTK_WIDGET (builder_get (builder, "treeview_transfers_detail_et")),
		"Solo se puede modificar el costo de aquellos productos\n"
		"que obtuvieron su costo a través del traspaso seleccionado\n"
		"(aquellos resaltados con el color 'verde').");
      return;
    }

  if (!is_numeric (cost))
    {
      ErrorMSG (GTK_WIDGET (builder_get (builder, "treeview_transfers_detail_et")),
		"El precio de compra debe ser un valor numérico");
      return;
    }

  new_cost = strtod (PUT (cost), (char **)NULL);
  
  if (new_cost <= 0)
    {
      ErrorMSG (GTK_WIDGET (builder_get (builder, "treeview_transfers_detail_et")),
		"El costo debe ser mayor a cero");
      return;
    }

  //Si asegura que continúe solo si se cambió el costo
  if (new_cost != previous_cost)
    gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_save_et")), TRUE);
  else
    return;
  
  /*Se agrega a la lista de los productos modificados*/  

  //Se verifica si ya existe en la lista
  if (lista_mod_prod->header != NULL)
    lista_mod_prod->prods = buscar_prod (lista_mod_prod->header, barcode);
  else
    lista_mod_prod->prods = NULL;

  //Si no existe en la lista se agrega y se inicializa
  if (lista_mod_prod->prods == NULL) //PEEEEE
    {
      add_to_mod_prod_list (barcode, FALSE);
      lista_mod_prod->prods->prod->id_traspaso = id_traspaso;
      lista_mod_prod->prods->prod->costo_original = previous_cost; /*Se guarda el costo original*/
      lista_mod_prod->prods->prod->costo_nuevo = new_cost; /*Se guarda el nuevo costo*/
      lista_mod_prod->prods->prod->accion = MOD; /*Indica que se modificará este producto en la BD (enum action)*/
      original = FALSE;
    } /*Si el producto existe pero se le modifica el costo por 1era vez*/
  else if (lista_mod_prod->prods->prod->costo_original == 0)
    {
      lista_mod_prod->prods->prod->id_traspaso = id_traspaso;
      lista_mod_prod->prods->prod->costo_original = previous_cost; /*Se guarda el costo original*/
      lista_mod_prod->prods->prod->costo_nuevo = new_cost; /*Se guarda el nuevo costo*/
      original = FALSE;
    }
  else //Si estaba en la lista y se modifica el costo por enésima vez (incluyendo productos agregados)
    {  //Si el costo nuevo es igual al original
      if (new_cost == lista_mod_prod->prods->prod->costo_original)
	{	  
	  lista_mod_prod->prods->prod->costo_nuevo = 0;
	  
	  //Si no hay modificaciones en cantidad
	  if (lista_mod_prod->prods->prod->cantidad_nueva == 0)
	    {
	      drop_prod_to_mod_list (barcode);
	      original = TRUE;
	    }

	  if (cantidad_total_prods (lista_mod_prod->header) == 0) //Si no quedan productos en la lista
	    gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_save_et")), FALSE);
	}
      else //De lo contrario se modifica (Los productos agregados siempre llegaran aquí)
	{
	  lista_mod_prod->prods->prod->id_factura_compra = id_traspaso;
	  lista_mod_prod->prods->prod->costo_nuevo = new_cost;
	  original = FALSE;
	  tipo = lista_mod_prod->prods->prod->accion;
	}
    }

  /*color*/
  if (original == TRUE)
    {
      if (costo_editable)
	color = g_strdup ("#62932c");
      else
	color = g_strdup ("Black");
    }
  else
    {
      if (tipo == MOD)
	color = g_strdup ("Red");
      else if (tipo == ADD)
	color = g_strdup ("Blue");
    }

  // Se setean los datos modificados en el treeview
  gtk_list_store_set (GTK_LIST_STORE (model), &iter,
		      3, new_cost,                    //costo
		      4, lround (new_cost * cantity), //subtotal
		      9, color,
		      10, TRUE,
		      -1);

  //Se recalcula el costo total de la factura seleccionada
  treeview = GTK_TREE_VIEW (builder_get (builder, "treeview_transfers_detail_et"));
  total = lround (sum_treeview_column (treeview, 4, G_TYPE_INT));
  
  //Se actualiza el nuevo costo en el treeview de la factura
  treeview = GTK_TREE_VIEW (builder_get (builder, "treeview_transfers_et"));
  model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
  gtk_tree_selection_get_selected (gtk_tree_view_get_selection (treeview), NULL, &iter);

  gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                      5, total,
                      -1);
}


/**
 * This callback is triggered when a cell on 
 * 'treeview_nullify_buy_invoice_details' when
 * the cantity is edited. (editable-event)
 *
 * @param cell
 * @param path_string
 * @param new_cantity
 * @param data -> A GtkListStore
 */
void
on_mod_et_cantity_cell_renderer_edited (GtkCellRendererText *cell, gchar *path_string, gchar *cantity, gpointer data)
{
  //Treeview detalle de la compra
  GtkTreeView *treeview;
  GtkTreeModel *model;
  GtkTreePath *path;
  GtkTreeIter iter;
  
  //Variables para recoger información
  gint total;
  gint tipo;
  gint id_traspaso;
  gchar *barcode;
  gchar *color, *ea, *ec; /*color, etiqueta apertura, etiqueta cierre*/
  gdouble new_cantity;
  gdouble previous_cantity;
  gdouble original_cantity;
  gdouble cost;
  gdouble modificable;
  gboolean costo_editable;
  gboolean original;
  gint origen;
  gint tipo_compuesto, tipo_deriv, tipo_producto;

  /*tipo de mercaderia*/
  tipo_compuesto = atoi (g_strdup (PQvaluebycol (EjecutarSQL ("SELECT id FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'COMPUESTA'"), 0, "id")));
  tipo_deriv = atoi (g_strdup (PQvaluebycol (EjecutarSQL ("SELECT id FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'DERIVADA'"), 0, "id")));


  /*Original es FALSE hasta que se demuetre lo contrario*/
  original = FALSE;

  /*Tipo es MOD hasta que se demuestre los contrario*/
  tipo = MOD;

  /*Obtiene el modelo del treeview compra detalle*/
  model = GTK_TREE_MODEL (data);
  path = gtk_tree_path_new_from_string (path_string);

  /*Obtiene el iter para poder obtener y setear datos del treeview*/
  gtk_tree_model_get_iter (model, &iter, path);

  //Se obtiene el codigo del producto y la cantidad ingresada
  gtk_tree_model_get (model, &iter,
		      0, &barcode,
                      2, &previous_cantity,
  		      3, &cost,
		      5, &tipo_producto,
		      6, &id_traspaso,
		      7, &origen,
		      8, &costo_editable,
                      -1);
  
  /*Valida el tipo de producto*/
  if (tipo_producto == tipo_compuesto || tipo_producto == tipo_deriv)
    {
      ea = "<span color = '#535353'>";
      ec = "</span>";
      ErrorMSG (GTK_WIDGET (builder_get (builder, "treeview_transfers_detail_et")),
		g_strdup_printf ("No se puede modificar la cantidad de los productos "
				 "%s compuestos u ofertas %s o %s derivados %s", ea, ec, ea, ec));
      return;
    }

  /*Valida y Obtiene el precio nuevo*/
  if (!is_numeric (cantity))
    {
      ErrorMSG (GTK_WIDGET (builder_get (builder, "treeview_transfers_detail_et")),
		"La cantidad debe ser un valor numérico");
      return;
    }

  new_cantity = strtod (PUT (cantity), (char **)NULL);

  if (new_cantity <= 0)
    {
      ErrorMSG (GTK_WIDGET (builder_get (builder, "treeview_transfers_detail_et")),
		"La cantidad debe ser mayor a cero");
      return;
    }

  if (new_cantity != previous_cantity)
    gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_save_et")), TRUE);
  else
    return;
  
  /*Se agrega a la lista de los productos modificados*/  

  //Se verifica si ya existe en la lista
  if (lista_mod_prod->header != NULL)
    lista_mod_prod->prods = buscar_prod (lista_mod_prod->header, barcode);
  else
    lista_mod_prod->prods = NULL;

  //Si no existe en la lista se agrega y se inicializa
  if (lista_mod_prod->prods == NULL)
    {
      add_to_mod_prod_list (barcode, FALSE);
      lista_mod_prod->prods->prod->id_traspaso = id_traspaso;
      lista_mod_prod->prods->prod->cantidad_original = previous_cantity; /*Se guarda cantidad original*/
      original_cantity = previous_cantity; /*Esta variable se usará para verificar si es modificable*/
      lista_mod_prod->prods->prod->cantidad_nueva = new_cantity; /*Se guarda la nueva cantidad*/
      lista_mod_prod->prods->prod->accion = MOD; /*Indica que se modificará este producto en la BD (enum action)*/
      original = FALSE;
    } /*Si el producto existe pero se le modifica el costo por 1era vez*/
  else if (lista_mod_prod->prods->prod->cantidad_nueva == 0)
    {
      lista_mod_prod->prods->prod->id_traspaso = id_traspaso;
      lista_mod_prod->prods->prod->cantidad_original = previous_cantity; /*Se guarda la cantidad priginal*/
      original_cantity = previous_cantity; /*Esta variable se usará para verificar si es modificable*/
      lista_mod_prod->prods->prod->cantidad_nueva = new_cantity; /*Se guarda la nueva cantidad*/
      original = FALSE;
    }
  else //Si estaba en la lista y se modifica el costo por enésima vez (incluyendo productos agregados)
    {  //Si el costo nuevo es igual al original
      if (new_cantity == lista_mod_prod->prods->prod->cantidad_original)
	{
	  lista_mod_prod->prods->prod->cantidad_nueva = 0;
	  
	  //Si no hay modificaciones en costo
	  if (lista_mod_prod->prods->prod->costo_nuevo == 0)
	    {
	      /*Esta variable se usará para verificar si es modificable*/
	      original_cantity = lista_mod_prod->prods->prod->cantidad_original;

	      drop_prod_to_mod_list (barcode);
	      original = TRUE;
	    }

	  if (cantidad_total_prods (lista_mod_prod->header) == 0) //Si no quedan productos en la lista
	    gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_save_et")), FALSE);
	} 
      else //De lo contrario se modifica (Los productos agregados siempre llegaran aquí)
	{ 
	  lista_mod_prod->prods->prod->id_traspaso = id_traspaso;
	  lista_mod_prod->prods->prod->cantidad_nueva = new_cantity;
	  original_cantity = lista_mod_prod->prods->prod->cantidad_original;

	  original = FALSE;
	  tipo = lista_mod_prod->prods->prod->accion;
	}
    }

  /* SI se recibió más cantidad o se envió menos cantidad es modificable
     SINO se comprueba que la "diminución" del stock no genere un stock negativo en alguna transacción futura
     (Si se agregan productos a traspaso tener en cuanta si es un envío o una recepción)
  */
  if ( (origen != 1 && new_cantity > original_cantity) || (origen == 1 && new_cantity < original_cantity) )
    modificable = 1;
  else if (DataExist (g_strdup_printf ("SELECT barcode FROM traspaso_detalle WHERE id_traspaso = %d", id_traspaso)))
    modificable = cantidad_traspaso_es_modificable (barcode, original_cantity, new_cantity, id_traspaso, origen);
  else
    modificable = 0;
  
  /* Si modificable es <= 0, quiere decir que 'cantidad_traspaso_es_modificable' devolvió ese valor 
     y por lo tanto no es modificable*/
  if (modificable <= 0)
    {
      gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_save_mod_buy")), FALSE);
      drop_prod_to_mod_list (barcode);

      if (origen == 1)
	ErrorMSG (GTK_WIDGET (builder_get (builder, "treeview_transfers_detail_et")),
		  g_strdup_printf ("Debido a las transacciones realizadas sobre el producto %s en fechas \n"
				   "posteriores a este traspaso, se pueden enviar maximo: %.3f unidades",
				   barcode, (original_cantity + modificable*-1) ));
      else 
	ErrorMSG (GTK_WIDGET (builder_get (builder, "treeview_transfers_detail_et")),
		  g_strdup_printf ("Debido a las transacciones realizadas sobre el producto %s en fechas \n"
				   "posteriores a este traspaso, se pueden recibir como minimo: %.3f unidades",
				   barcode, (modificable*-1) ));

      //NOTA: modificable es 0 o negativo, por lo tanto se restan o permanece igual
      return;
    }

  /*color*/
  if (original == TRUE)
    {
      if (costo_editable)
	color = g_strdup ("#62932c");
      else
	color = g_strdup ("Black");
    }
  else
    {
      if (tipo == MOD)
	color = g_strdup ("Red");
      else if (tipo == ADD)
	color = g_strdup ("Blue");
    }

  // Se setean los datos modificados en el treeview
  gtk_list_store_set (GTK_LIST_STORE (model), &iter,
		      2, new_cantity,                 //cantidad
		      4, lround (new_cantity * cost), //subtotal
		      9, color,
		      10, TRUE,
		      -1);

  //Se recalcula el costo total de la factura seleccionada
  treeview = GTK_TREE_VIEW (builder_get (builder, "treeview_transfers_detail_et"));
  total = lround (sum_treeview_column (treeview, 4, G_TYPE_INT));
  
  //Se actualiza el nuevo costo en el treeview de la factura
  treeview = GTK_TREE_VIEW (builder_get (builder, "treeview_transfers_et"));
  model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
  gtk_tree_selection_get_selected (gtk_tree_view_get_selection (treeview), NULL, &iter);

  gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                      5, total,
                      -1);
}

/**
 * Callback from 'btn_edit_transfers' (clicked-signal)
 *
 * Make and show the 'wnd_edit_transfers' window.
 *
 * @param: GtkButton *button (the button from trigger signal)
 * @param: gpointer data (optional data)
 */
void
on_btn_edit_transfers_clicked (GtkButton *button, gpointer data)
{
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  GtkTreeView *treeview_transfer;
  GtkListStore *store_transfer;
  GtkTreeSelection *selection_transfer;

  GtkTreeView *treeview_details;
  GtkListStore *store_details;

  // TreeView Facturas
  treeview_transfer = GTK_TREE_VIEW (builder_get (builder, "treeview_transfers_et"));
  store_transfer = GTK_LIST_STORE (gtk_tree_view_get_model (treeview_transfer));
  if (store_transfer == NULL)
    {
      store_transfer = gtk_list_store_new (8,
					   G_TYPE_INT,     // ID Traspaso
					   G_TYPE_STRING,  // Fecha Traspaso
					   G_TYPE_STRING,  // Local Origen Traspaso
					   G_TYPE_STRING,  // Local Destino Traspaso
					   G_TYPE_STRING,  // Tipo Traspaso
					   G_TYPE_INT,     // Monto Traspaso
					   G_TYPE_INT,     // ID Origen
					   G_TYPE_INT);    // ID Destino

      gtk_tree_view_set_model (treeview_transfer, GTK_TREE_MODEL (store_transfer));
      
      selection_transfer = gtk_tree_view_get_selection (treeview_transfer);
      gtk_tree_selection_set_mode (selection_transfer, GTK_SELECTION_SINGLE);
      g_signal_connect (G_OBJECT (selection_transfer), "changed",
			G_CALLBACK (on_selection_transfer_et_change), NULL);

      //ID
      renderer = gtk_cell_renderer_text_new();
      column = gtk_tree_view_column_new_with_attributes("ID Traspaso", renderer,
                                                        "text", 0,
                                                        NULL);
      gtk_tree_view_append_column (treeview_transfer, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
      gtk_tree_view_column_set_sort_column_id (column, 0);
      gtk_tree_view_column_set_resizable (column, FALSE);

      //Fecha Traspaso
      renderer = gtk_cell_renderer_text_new();
      column = gtk_tree_view_column_new_with_attributes("Fecha Traspaso", renderer,
                                                        "text", 1,
                                                        NULL);
      gtk_tree_view_append_column (treeview_transfer, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
      gtk_tree_view_column_set_sort_column_id (column, 1);
      gtk_tree_view_column_set_resizable (column, FALSE);

      //Local Origen
      renderer = gtk_cell_renderer_text_new();
      column = gtk_tree_view_column_new_with_attributes("Origen", renderer,
                                                        "text", 2,
                                                        NULL);
      gtk_tree_view_append_column (treeview_transfer, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
      gtk_tree_view_column_set_sort_column_id (column, 2);
      gtk_tree_view_column_set_resizable (column, FALSE);

      //Local Destino
      renderer = gtk_cell_renderer_text_new();
      column = gtk_tree_view_column_new_with_attributes("Destino", renderer,
                                                        "text", 3,
                                                        NULL);
      gtk_tree_view_append_column (treeview_transfer, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
      gtk_tree_view_column_set_sort_column_id (column, 3);
      gtk_tree_view_column_set_resizable (column, FALSE);

      //Tipo Traspaso
      renderer = gtk_cell_renderer_text_new();
      column = gtk_tree_view_column_new_with_attributes("Tipo traspaso", renderer,
                                                        "text", 4,
                                                        NULL);
      gtk_tree_view_append_column (treeview_transfer, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
      gtk_tree_view_column_set_sort_column_id (column, 4);
      gtk_tree_view_column_set_resizable (column, FALSE);

      //Monto total
      renderer = gtk_cell_renderer_text_new();
      column = gtk_tree_view_column_new_with_attributes("Monto Total", renderer,
                                                        "text", 5,
                                                        NULL);
      gtk_tree_view_append_column (treeview_transfer, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
      gtk_tree_view_column_set_sort_column_id (column, 5);
      gtk_tree_view_column_set_resizable (column, FALSE);
      gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)5, NULL);
    }

  //TreeView Detalle Factura
  treeview_details = GTK_TREE_VIEW (builder_get (builder, "treeview_transfers_detail_et"));
  store_details = GTK_LIST_STORE (gtk_tree_view_get_model (treeview_details));
  if (store_details == NULL)
    {
      store_details = gtk_list_store_new (11,
                                          G_TYPE_STRING, //barcode
                                          G_TYPE_STRING, //description
                                          G_TYPE_DOUBLE, //cantity
                                          G_TYPE_DOUBLE, //costo
                                          G_TYPE_INT,    //subtotal
					  G_TYPE_INT,    //tipo producto
                                          G_TYPE_INT,      //id traspaso
					  G_TYPE_INT,      //ID Origen
					  G_TYPE_BOOLEAN,  //modificable
					  G_TYPE_STRING,
					  G_TYPE_BOOLEAN);

      gtk_tree_view_set_model(treeview_details, GTK_TREE_MODEL(store_details));
      //gtk_tree_selection_set_mode (gtk_tree_view_get_selection (treeview_details), GTK_SELECTION_NONE);

      //barcode
      renderer = gtk_cell_renderer_text_new();
      column = gtk_tree_view_column_new_with_attributes("Cod. Barras", renderer,
                                                        "text", 0,
							"foreground", 9,
							"foreground-set", 10,
                                                        NULL);
      gtk_tree_view_append_column (treeview_details, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
      gtk_tree_view_column_set_sort_column_id (column, 0);
      gtk_tree_view_column_set_resizable (column, FALSE);

      //description
      renderer = gtk_cell_renderer_text_new();
      column = gtk_tree_view_column_new_with_attributes("Descripcion", renderer,
                                                        "text", 1,
							"foreground", 9,
							"foreground-set", 10,
                                                        NULL);
      gtk_tree_view_append_column (treeview_details, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
      gtk_tree_view_column_set_sort_column_id (column, 1);
      gtk_tree_view_column_set_resizable (column, FALSE);

      //cantity
      renderer = gtk_cell_renderer_text_new();
      g_object_set (renderer,"editable", TRUE, NULL);
      g_signal_connect (G_OBJECT (renderer), "edited",
			G_CALLBACK (on_mod_et_cantity_cell_renderer_edited), (gpointer)store_details);

      column = gtk_tree_view_column_new_with_attributes("Cantidad", renderer,
                                                        "text", 2,
							"foreground", 9,
							"foreground-set", 10,
                                                        NULL);
      gtk_tree_view_append_column (treeview_details, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
      gtk_tree_view_column_set_sort_column_id (column, 2);
      gtk_tree_view_column_set_resizable (column, FALSE);
      gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)2, NULL);

      //buy price (cost) 
      renderer = gtk_cell_renderer_text_new();

      g_object_set (renderer,"editable", TRUE, NULL);
      g_signal_connect (G_OBJECT (renderer), "edited",
			G_CALLBACK (on_mod_et_buy_price_cell_renderer_edited), (gpointer)store_details);

      column = gtk_tree_view_column_new_with_attributes("Costo", renderer,
                                                        "text", 3,
							"foreground", 9,
							"foreground-set", 10,
                                                        NULL);
      gtk_tree_view_append_column (treeview_details, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
      gtk_tree_view_column_set_sort_column_id (column, 3);
      gtk_tree_view_column_set_resizable (column, FALSE);
      gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)3, NULL);

      //subtotal
      renderer = gtk_cell_renderer_text_new();
      column = gtk_tree_view_column_new_with_attributes("Subtotal", renderer,
                                                        "text", 4,
							"foreground", 9,
							"foreground-set", 10,
                                                        NULL);
      gtk_tree_view_append_column (treeview_details, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
      gtk_tree_view_column_set_sort_column_id (column, 4);
      gtk_tree_view_column_set_resizable (column, FALSE);
      gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)4, NULL);
    }

  /*Combobox sucursales traspaso*/
  GtkWidget *combo_sucursal;
  //GtkWidget *combo_sucursal_edit;
  GtkWidget *combo_traspaso;
  GtkListStore *modelo_sucursal;
  //GtkListStore *modelo_sucursal_edit;
  GtkListStore *modelo_traspaso;

  GtkTreeIter iter;
  gint tuples,i;
  PGresult *res;

  res = EjecutarSQL (g_strdup_printf ("SELECT id, nombre "
				      "FROM bodega "
				      "WHERE estado = true"));
  tuples = PQntuples (res);

  combo_sucursal = GTK_WIDGET (builder_get (builder, "cmb_store_et"));
  modelo_sucursal = GTK_LIST_STORE (gtk_combo_box_get_model (GTK_COMBO_BOX (combo_sucursal)));

  if (modelo_sucursal == NULL)
    {
      GtkCellRenderer *cell;
      modelo_sucursal = gtk_list_store_new (2,
					    G_TYPE_INT,
					    G_TYPE_STRING);

      gtk_combo_box_set_model (GTK_COMBO_BOX (combo_sucursal), GTK_TREE_MODEL (modelo_sucursal));

      cell = gtk_cell_renderer_text_new();
      gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo_sucursal), cell, TRUE);
      gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo_sucursal), cell,
				      "text", 1,
				      NULL);
    }

  gtk_list_store_clear (modelo_sucursal);

  gtk_list_store_append (modelo_sucursal, &iter);
  gtk_list_store_set (modelo_sucursal, &iter,
		      0, 0,
		      1, "TODOS",
		      -1);

  for (i=0 ; i < tuples ; i++)
    {
      gtk_list_store_append (modelo_sucursal, &iter);
      gtk_list_store_set (modelo_sucursal, &iter,
			  0, atoi (PQvaluebycol (res, i, "id")),
			  1, PQvaluebycol (res, i, "nombre"),
			  -1);
    }

  gtk_combo_box_set_active (GTK_COMBO_BOX (combo_sucursal), 0);

  /*tipo traspaso*/
  combo_traspaso = GTK_WIDGET (builder_get (builder, "cmb_transfer_type_et"));
  modelo_traspaso = GTK_LIST_STORE (gtk_combo_box_get_model (GTK_COMBO_BOX (combo_traspaso)));

  if (modelo_traspaso == NULL)
    {
      GtkCellRenderer *cell;
      modelo_traspaso = gtk_list_store_new (1,
  					    G_TYPE_STRING);

      gtk_combo_box_set_model (GTK_COMBO_BOX (combo_traspaso), GTK_TREE_MODEL (modelo_traspaso));

      cell = gtk_cell_renderer_text_new();
      gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo_traspaso), cell, TRUE);
      gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo_traspaso), cell,
  				      "text", 0,
  				      NULL);
    }

  gtk_list_store_clear (modelo_traspaso);

  gtk_list_store_append (modelo_traspaso, &iter);
  gtk_list_store_set (modelo_traspaso, &iter,
  		      0, "TODOS",
  		      -1);

  gtk_list_store_append (modelo_traspaso, &iter);
  gtk_list_store_set (modelo_traspaso, &iter,
  		      0, "ENVIADO",
  		      -1);
  
  gtk_list_store_append (modelo_traspaso, &iter);
  gtk_list_store_set (modelo_traspaso, &iter,
  		      0, "RECIBIDO",
  		      -1);

  gtk_combo_box_set_active (GTK_COMBO_BOX (combo_traspaso), 0);


  //TODO:
  clean_container (GTK_CONTAINER (GTK_WIDGET (builder_get (builder, "wnd_edit_transfers"))));
  gtk_widget_show (GTK_WIDGET (builder_get (builder, "wnd_edit_transfers")));
}

void
on_btn_edit_transfer_search_clicked (GtkButton *button, gpointer data)
{
  /*treeview*/
  GtkTreeView *treeview;
  GtkListStore *store;

  /*combobox*/
  GtkWidget *combo;
  GtkTreeModel *model;
  gint active;
  gint id_sucursal;
  gchar *tipo_traspaso;

  GtkTreeIter iter;

  /*Formulario  busqueda*/
  GDate *date_aux;
  gchar *barcode, *fecha;
  
  /*Obteniendo campos de búsqueda*/
  barcode = g_strdup (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_barcode_et"))));
  fecha = g_strdup (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_nullify_buy_date"))));

  /*fecha*/
  date_aux = g_date_new ();
  if (!g_str_equal (fecha, ""))
    {
      g_date_set_parse (date_aux, fecha);
      fecha = g_strdup_printf("%d-%d-%d",
			      g_date_get_year (date_aux),
			      g_date_get_month (date_aux),
			      g_date_get_day (date_aux));
    }

  /*Filtro sucursal*/
  combo = GTK_WIDGET (builder_get (builder, "cmb_store_et"));
  active = gtk_combo_box_get_active (GTK_COMBO_BOX (combo));

  /*Se obtiene la sucursal elegida*/
  if (active != -1)
    {
      model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));
      gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combo), &iter);

      gtk_tree_model_get (model, &iter,
			  0, &id_sucursal,
                          -1);
    }

  /*tipo traspaso*/
  combo = GTK_WIDGET (builder_get (builder, "cmb_transfer_type_et"));
  active = gtk_combo_box_get_active (GTK_COMBO_BOX (combo));


  /*Se obtiene el tipo de trapaso elegido*/
  if (active != -1)
    {
      model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));
      gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combo), &iter);

      gtk_tree_model_get (model, &iter,
			  0, &tipo_traspaso,
                          -1);
    }

  /*Limpiando Treeview*/
  treeview = GTK_TREE_VIEW (gtk_builder_get_object (builder, "treeview_transfers_et"));
  store = GTK_LIST_STORE (gtk_tree_view_get_model (treeview));
  gtk_list_store_clear (store);

  gchar *q;
  PGresult *res;
  gint tuples, i;

  q = g_strdup_printf ("SELECT t.id, t.monto, t.origen, t.destino, t.vendedor, "
		       "TO_CHAR(fecha,'YYYY-MM-DD') AS fecha, " //YYYY-MM-DD HH24:MI
		       "CASE WHEN (t.origen = 1) THEN 'ENVIADO' ELSE 'RECIBIDO' END AS tipo_traspaso, "
		       "(SELECT nombre FROM bodega WHERE id = t.origen) AS nombre_origen, "
		       "(SELECT nombre FROM bodega WHERE id = t.destino) AS nombre_destino "
		       "FROM traspaso t "
		       "INNER JOIN traspaso_detalle td "
		       "ON t.id = td.id_traspaso");

  /*Condiciones agregadas*/
  if (!g_str_equal (barcode, "") || g_date_valid (date_aux) || 
      id_sucursal != 0 || !g_str_equal (tipo_traspaso, "TODOS"))
    {
      q = g_strdup_printf ("%s WHERE t.origen IS NOT NULL",q);

      if (!g_str_equal (barcode, ""))
	q = g_strdup_printf (" %s AND td.barcode = %s",q, barcode);

      if (g_date_valid (date_aux))
	q = g_strdup_printf (" %s AND c.fecha BETWEEN '%s' AND '%s + 1 days'", q, fecha, fecha);

      if (id_sucursal != 0)
	q = g_strdup_printf (" %s AND (origen = %d OR destino = %d)", q, id_sucursal, id_sucursal);

      if (!g_str_equal (tipo_traspaso, "TODOS"))
	q = g_strdup_printf (" %s AND origen %s 1", q, g_str_equal (tipo_traspaso, "ENVIADO") ? "=": "!=");
    }

  //Se ejecuta la consulta construida
  res = EjecutarSQL (q);
  g_free (q);  
  tuples = PQntuples (res);

  for (i = 0; i < tuples; i++)
    {
      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
			  0, atoi (g_strdup (PQvaluebycol (res, i, "id"))),
			  1, g_strdup (PQvaluebycol (res, i, "fecha")),
			  2, g_strdup (PQvaluebycol (res, i, "nombre_origen")),
			  3, g_strdup (PQvaluebycol (res, i, "nombre_destino")),
			  4, g_strdup (PQvaluebycol (res, i, "tipo_traspaso")),
			  5, atoi (g_strdup (PQvaluebycol (res, i, "monto"))),
			  6, atoi (g_strdup (PQvaluebycol (res, i, "origen"))),
			  7, atoi (g_strdup (PQvaluebycol (res, i, "destino"))),
			  -1);
    }

  PQclear (res);

  //Si existe algo en la estructura "lista_mod_prod" se limpia
  clean_lista_mod_prod ();
}


/**
* Esta funcion guarda (efectúa) todos los cambios
* que se hayan realizado en el traspaso (detallados en lista_mod_prod)
*/
void
on_btn_save_et_clicked (GtkButton *button, gpointer data)
{
  Prods *header;
  Prods *actual;
  gboolean error;

  header = lista_mod_prod->header;
  actual = lista_mod_prod->header;

  if (header == NULL)
    return;

  error = FALSE;
  do
    {
      if (actual->prod->accion == MOD)
	if (mod_to_mod_on_transfer (actual->prod) == FALSE) 
	  error = TRUE;
      actual = actual->next;
    } while (actual != header);

  if (error == FALSE)
    AlertMSG (GTK_WIDGET (builder_get (builder, "btn_edit_transfer_cancel")),
	      "Se realizaron los cambios!");
  else
    ErrorMSG (GTK_WIDGET (builder_get (builder, "btn_edit_transfer_cancel")),
	      "No se logró efecutar la modificacion en traspaso");
  
  //clean_lista_mod_prod (); La funcion que busca limpia la lista
  on_btn_edit_transfer_search_clicked (GTK_BUTTON (builder_get (builder, "btn_edit_transfer_search")), NULL);
  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_save_mod_buy")), FALSE);
}


/**
 * Es llamada por la función "on_button_new_product_clicked"
 * cuando ROPA=1 en el .rizoma.
 *
 * Visualiza la ventana "wnd_new_clothing" y carga los
 * respectivos datos de ésta: los entry's, botones, y combo box'
 *
 * @param void
 */
void
create_new_clothing (void)
{
  GtkListStore *store;
  GtkTreeView *treeview;
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;
  //GtkTreeSelection *selection;

  treeview = GTK_TREE_VIEW (gtk_builder_get_object (builder, "treeview_sizes"));
  store = GTK_LIST_STORE(gtk_tree_view_get_model(treeview));
  if (store == NULL)
    {
      //Tallas
      store = gtk_list_store_new (3,
				  G_TYPE_BOOLEAN, //Para saber si se rellenó desde la BD o se agregó a mano
				  G_TYPE_STRING,  //Codigo de talla
				  G_TYPE_STRING); //Tallas

      gtk_tree_view_set_model (GTK_TREE_VIEW (treeview), GTK_TREE_MODEL (store));


      renderer = gtk_cell_renderer_text_new ();
      g_object_set (renderer,
		    "editable", TRUE,
		    NULL);
      g_signal_connect (G_OBJECT (renderer), "edited",
			G_CALLBACK (on_sizes_code_cell_renderer_edited), (gpointer)store);
      column = gtk_tree_view_column_new_with_attributes ("Cod. Talla", renderer,
							 "text", 1,
							 NULL);
      gtk_tree_view_append_column (treeview, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
      gtk_tree_view_column_set_resizable (column, FALSE);


      renderer = gtk_cell_renderer_text_new ();
      g_object_set (renderer,
		    "editable", TRUE,
		    NULL);
      g_signal_connect (G_OBJECT (renderer), "edited",
			G_CALLBACK (on_sizes_cell_renderer_edited), (gpointer)store);
      column = gtk_tree_view_column_new_with_attributes ("Talla", renderer,
							 "text", 2,
							 NULL);
      gtk_tree_view_append_column (treeview, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
      gtk_tree_view_column_set_resizable (column, FALSE);
    }

  treeview = GTK_TREE_VIEW (gtk_builder_get_object (builder, "treeview_colors"));
  store = GTK_LIST_STORE(gtk_tree_view_get_model(treeview));
  if (store == NULL)
    {
      //Colores
      store = gtk_list_store_new (3,
				  G_TYPE_BOOLEAN, //Para saber si se rellenó desde la BD o se agregó a mano
				  G_TYPE_STRING,  //CODIGO
				  G_TYPE_STRING); //NOMBRE

      gtk_tree_view_set_model (GTK_TREE_VIEW (treeview), GTK_TREE_MODEL (store));

      renderer = gtk_cell_renderer_text_new ();
      g_object_set (renderer,
		    "editable", TRUE,
		    NULL);
      g_signal_connect (G_OBJECT (renderer), "edited",
			G_CALLBACK (on_code_color_cell_renderer_edited), (gpointer)store);
      column = gtk_tree_view_column_new_with_attributes ("Cod. Color", renderer,
							 "text", 1,
							 NULL);
      gtk_tree_view_append_column (treeview, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
      gtk_tree_view_column_set_resizable (column, FALSE);

      renderer = gtk_cell_renderer_text_new ();
      g_object_set (renderer,
		    "editable", TRUE,
		    NULL);
      g_signal_connect (G_OBJECT (renderer), "edited",
			G_CALLBACK (on_color_cell_renderer_edited), (gpointer)store);
      column = gtk_tree_view_column_new_with_attributes ("Color", renderer,
							 "text", 2,
							 NULL);
      gtk_tree_view_append_column (treeview, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
      gtk_tree_view_column_set_resizable (column, FALSE);
    }

  treeview = GTK_TREE_VIEW (gtk_builder_get_object (builder, "treeview_shortcodes"));
  store = GTK_LIST_STORE(gtk_tree_view_get_model(treeview));
  if (store == NULL)
    {
      //Clothes Code
      store = gtk_list_store_new (2,
				  G_TYPE_BOOLEAN, //Para saber si se rellenó desde la BD o se agregó a mano
				  G_TYPE_STRING); //Clothes Code

      gtk_tree_view_set_model (GTK_TREE_VIEW (treeview), GTK_TREE_MODEL (store));

      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes ("Codigo", renderer,
							 "text", 1,
							 NULL);
      gtk_tree_view_append_column (treeview, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
      gtk_tree_view_column_set_resizable (column, FALSE);
    }

  //Limitaciones para los entry
  gtk_entry_set_max_length (GTK_ENTRY (builder_get (builder, "entry_clothes_depto")), 2);
  gtk_entry_set_max_length (GTK_ENTRY (builder_get (builder, "entry_clothes_temp")), 3);
  gtk_entry_set_max_length (GTK_ENTRY (builder_get (builder, "entry_clothes_year")), 2);
  gtk_entry_set_max_length (GTK_ENTRY (builder_get (builder, "entry_clothes_sub_depto")), 3);
  gtk_entry_set_max_length (GTK_ENTRY (builder_get (builder, "entry_clothes_id")), 2);

  //Botones deshabilitados
  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_add_size")), FALSE);
  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_del_size")), FALSE);
  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_add_color")), FALSE);
  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_del_color")), FALSE);
  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_add_new_clothes")), FALSE);

  //Habilitar los widget de construcción de código
  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "entry_clothes_depto")), TRUE);
  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "entry_clothes_temp")), TRUE);
  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "entry_clothes_year")), TRUE);
  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "entry_clothes_sub_depto")), TRUE);
  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "entry_clothes_id")), TRUE);
  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_accept_clothes")), TRUE);

  //widget's deshabilitados
  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "entry_new_clothes_brand")), FALSE);
  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "entry_new_clothes_desc")), FALSE);
  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_create_codes")), FALSE);

  gtk_widget_grab_focus (GTK_WIDGET (builder_get (builder, "entry_clothes_depto")));
  gtk_widget_show_all (GTK_WIDGET (builder_get (builder, "wnd_new_clothing")));
}


/**
 * Esta funcion es llamada cuando el boton "button_new_product" es presionado
 * (signal click).
 *
 * visualiza la ventana "wnd_new_product" o "wnd_new_clothing" 
 * dependindo de la opción "ROPA" en el .rizoma.
 *
 * @param button the button
 * @param user_data the user data
 */
void
on_button_new_product_clicked (GtkButton *button, gpointer data)
{
  if (g_str_equal(rizoma_get_value ("ROPA"),"0"))
    gtk_widget_show (GTK_WIDGET (builder_get (builder, "wnd_new_products_selection")));
  else
    create_new_clothing ();
}

/**
 * Show window to create the merchandise selected
 *
 * @param button the button
 * @param user_data the user data
 */
void 
new_merchandise (GtkButton *button, gpointer data)
{
  gchar *button_name;
  button_name = g_strdup (gtk_buildable_get_name (GTK_BUILDABLE (button)));

  //gtk_widget_hide (GTK_WIDGET (builder_get (builder, "wnd_new_merchandise")));
  gtk_widget_hide (GTK_WIDGET (builder_get (builder, "wnd_new_products_selection")));
  
  if (g_str_equal (button_name, "btn_new_product"))
    create_new_merchandise ("md"); //Mercaderia Corriente
  else if (g_str_equal (button_name, "btn_new_mp"))
    create_new_merchandise ("mp"); //Materia Prima
  else if (g_str_equal (button_name, "btn_make_deriv"))
    create_new_merchandise ("mcd"); //MerCaderia Derivada
  else if (g_str_equal (button_name, "btn_new_offer"))
    create_new_compuesta ();//create_new_merchandise ("mcc"); //MerCaderia Compuesta
}


/**
 * This function is called when the mouse pointer
 * over the button positions (enter-notify-event signal).
 *
 * Update de option description
 *
 * @param the object which received the signal
 * @param the GdkEventCrossing which triggered this signal
 * @param user_data the user data
 *
 * @return TRUE to stop other handlers from being invoked for the event. FALSE to propagate the event further.
 *
 */
gboolean
show_description (GtkWidget *widget, GdkEventCrossing *event, gpointer user_data)
{
  gchar *widget_name;
  widget_name = g_strdup (gtk_buildable_get_name (GTK_BUILDABLE (widget)));

  if (g_str_equal (widget_name, "btn_new_product"))
    gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_option_merchandise")), "<i>Crear una mercadería simple\n"
			  "(abarrotes, alcoholes, bebidas, etc...) </i>");
  else if (g_str_equal (widget_name, "btn_new_offer"))
    gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_option_merchandise")), "<i>Crear ofertas a partir \n"
			  "de otras mercaderías </i>");
  else if (g_str_equal (widget_name, "btn_new_mp"))
    gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_option_merchandise")), "<i>Crear materias primas \n"
			  "(Como Vacunos, Pescados, etc...) </i>");
  else if (g_str_equal (widget_name, "btn_make_service"))
    gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_option_merchandise")), "<i>Crear Servicios \n"
			  "(Trabajos por Horas Hombre (HH)) </i>");

  return FALSE;
}


/**
 * This function is called when the mouse pointer
 * non-over the button positions (enter-notify-event signal).
 *
 * Update de option description
 *
 * @param the object which received the signal
 * @param the GdkEventCrossing which triggered this signal
 * @param user_data the user data
 *
 * @return TRUE to stop other handlers from being invoked for the event. FALSE to propagate the event further.
 *
 */
gboolean
show_default (GtkWidget *widget, GdkEventCrossing *event, gpointer user_data)
{
  gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_option_merchandise")), "<b>Seleccione una opción</b>");
  return FALSE;
}


/**
 * Esta funcion es llamada cuando el boton "btn_accept_clothes" es presionado
 * (signal click).
 *
 * Revisa la existencia del código en la base de datos y rellena los campos
 * correspondientes con la información adquirida, de no existir habilita los
 * campos para la creación de nuevos productos.
 *
 * @param button the button
 * @param user_data the user data
 */
void
on_btn_accept_clothes_clicked (GtkButton *button, gpointer data)
{ 
  //Variables para el treeview
  GtkListStore *store;
  GtkTreeIter iter;

  PGresult *res;
  gchar *clothes_code = g_strdup ("");
  gchar *q;
  gchar *depto, *sub_depto;
  gint i, num_res = 0;
  GObject *clothes_base_code[6];
  GObject *clothes_next_widget[6];

  //Fracciones del codigo de ropa
  clothes_base_code[0] = builder_get (builder, "entry_clothes_depto");
  clothes_base_code[1] = builder_get (builder, "entry_clothes_temp");
  clothes_base_code[2] = builder_get (builder, "entry_clothes_year");
  clothes_base_code[3] = builder_get (builder, "entry_clothes_sub_depto");
  clothes_base_code[4] = builder_get (builder, "entry_clothes_id");
  clothes_base_code[5] = builder_get (builder, "btn_accept_clothes");

  //Los widgets del 2 paso
  clothes_next_widget[0] = builder_get (builder, "btn_add_size");
  clothes_next_widget[1] = builder_get (builder, "btn_del_size");
  clothes_next_widget[2] = builder_get (builder, "btn_add_color");
  clothes_next_widget[3] = builder_get (builder, "btn_del_color");
  clothes_next_widget[4] = builder_get (builder, "btn_create_codes");
  //clothes_next_widget[4] = builder_get (builder, "entry_new_clothes_brand");
  //clothes_next_widget[5] = builder_get (builder, "entry_new_clothes_desc");

  //Valida los campos
  for (i = 0; i < 5; i++)
    {
      if (i==0 || i==2 || i==4)
	{
	  if (strlen (gtk_entry_get_text (GTK_ENTRY (clothes_base_code[i]))) != 2)
	    {
	      ErrorMSG (GTK_WIDGET (clothes_base_code[i]),
			g_strdup_printf ("El campo %s debe contener 2 dígitos", (i==0) ?
					 "departamento":(i==2) ? "año":"id"));
	      return;
	    }
	}
      else
	{
	  if (strlen (gtk_entry_get_text (GTK_ENTRY (clothes_base_code[i]))) != 3)
	    {
	      ErrorMSG (GTK_WIDGET (clothes_base_code[i]),
			g_strdup_printf ("El campo %s debe contener 3 dígitos", (i==1) ?
					 "temporada":"sub departamento"));
	      return;
	    }
	}
    }

  //Deshabilita la escritura en los entry (fracciones de 'clothes code') y el boton
  for (i = 0; i < 6; i++)
    gtk_widget_set_sensitive (GTK_WIDGET (clothes_base_code[i]), FALSE);

  //Concadena todas las partes del "clothes code"
  for (i = 0; i < 5; i++)
    clothes_code = g_strdup_printf ("%s%s", clothes_code, gtk_entry_get_text (GTK_ENTRY (clothes_base_code[i])));

  //Habilita la escritura en los widgets que sigen
  for (i = 0; i < 5; i++)
    gtk_widget_set_sensitive (GTK_WIDGET (clothes_next_widget[i]), TRUE);


  //Deja el foco en el botón para agregar tallas
  gtk_widget_grab_focus (GTK_WIDGET (clothes_next_widget[0]));

  /* == Se rellena el treeview de talla == */
  depto = g_strdup (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_clothes_depto"))));
  q = g_strdup_printf ("SELECT DISTINCT (t.codigo) AS cod_talla, t.nombre AS talla "
  		       "FROM talla t, clothes_code cc "
		       "WHERE t.codigo = cc.talla "
		       "AND cc.depto = '%s'", 
		       depto);
  res = EjecutarSQL(q);
  num_res = PQntuples (res);
  
  //Si encuentra tallas correspondientes al departamento
  if (num_res > 0)
    {
      //Rellena el treeview con las tallas correspondientes al departamento
      store = GTK_LIST_STORE (gtk_tree_view_get_model 
			      (GTK_TREE_VIEW (gtk_builder_get_object (builder, "treeview_sizes"))));
      for (i=0; i<num_res; i++)
	{
	  gtk_list_store_append (store, &iter);
	  gtk_list_store_set (store, &iter,
			      0, TRUE,
			      1, g_strdup (PQvaluebycol (res, i, "cod_talla")),
			      2, g_strdup (PQvaluebycol (res, i, "talla")),
			      -1);
	}
    }

  /* == Se rellena el treeview de colores == */
  q = g_strdup_printf ("SELECT codigo, nombre "
  		       "FROM color");
  res = EjecutarSQL(q);
  num_res = PQntuples (res);
  
  //Si encuentra colores pre-existentes
  if (num_res > 0)
    {
      //Rellena el treeview todos los colores disponibles
      store = GTK_LIST_STORE (gtk_tree_view_get_model 
			      (GTK_TREE_VIEW (gtk_builder_get_object (builder, "treeview_colors"))));
      for (i=0; i<num_res; i++)
	{
	  gtk_list_store_append (store, &iter);
	  gtk_list_store_set (store, &iter,
			      0, TRUE,
			      1, g_strdup (PQvaluebycol(res, i, "codigo")),
			      2, g_strdup (PQvaluebycol(res, i, "nombre")),
			      -1);
	}
    }

  /* == Se rellena el treeview de codigo == */
  q = g_strdup_printf ("SELECT codigo_corto "
  		       "FROM producto "
		       "WHERE codigo_corto like '%s%%' "
		       "AND length (codigo_corto) = 16",
  		       clothes_code);
  res = EjecutarSQL(q);
  num_res = PQntuples (res);
  
  //Si encuentra el codigo base
  if (num_res > 0)
    {
      //Rellena el treeview con los codigos ya existentes
      store = GTK_LIST_STORE (gtk_tree_view_get_model 
			      (GTK_TREE_VIEW (gtk_builder_get_object (builder, "treeview_shortcodes"))));
      for (i=0; i<num_res; i++)
	{
	  gtk_list_store_append (store, &iter);
	  gtk_list_store_set (store, &iter,
			      0, TRUE,
			      1, g_strdup (PQvaluebycol(res, i, "codigo_corto")),
			      -1);
	}
    }

  /* == Se ingresa la descripcion correspondiente al sub_depto == */
  sub_depto = g_strdup (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_clothes_sub_depto"))));
  q = g_strdup_printf ("SELECT nombre "
  		       "FROM sub_depto "
		       "WHERE codigo = '%s'", 
		       sub_depto);

  res = EjecutarSQL(q);
  num_res = PQntuples (res);

  //Si encuentra la descripcion del sub_depto
  if (num_res > 0)
    gtk_entry_set_text (GTK_ENTRY (builder_get (builder, "entry_new_clothes_desc")),
			g_strdup (PQvaluebycol(res, 0, "nombre")));
}


/**
 * Is called by 'btn_add_size' button when is pressed (signal-clicked)
 *
 * make a new row on 'treeview_sizes'
 */
void
on_btn_add_size_clicked ()
{
  GtkTreeView *treeview;
  GtkListStore *store;
  GtkTreeIter iter;

  treeview = GTK_TREE_VIEW (builder_get (builder, "treeview_sizes"));
  store = GTK_LIST_STORE (gtk_tree_view_get_model (treeview));

  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter,
		      0, FALSE,
		      1, "Cod. Talla",
		      2, "Ingrese Talla",
		      -1);
}


/**
 * Is called by 'btn_del_size' button when is pressed (signal-clicked)
 *
 * delete the selected row on 'treeview_sizes'
 */
void
on_btn_del_size_clicked ()
{
  GtkTreeView *treeview;
  GtkListStore *store;
  GtkTreeSelection *selection;
  GtkTreeIter iter;
  gint position;
  gboolean datoExistente;
  gchar *codTalla;

  gboolean base_existente;
  gint i = 0;
  gchar *code_base = g_strdup ("");
  GObject *clothes_base_code[5];
  clothes_base_code[0] = builder_get (builder, "entry_clothes_depto");
  clothes_base_code[1] = builder_get (builder, "entry_clothes_temp");
  clothes_base_code[2] = builder_get (builder, "entry_clothes_year");
  clothes_base_code[3] = builder_get (builder, "entry_clothes_sub_depto");
  clothes_base_code[4] = builder_get (builder, "entry_clothes_id");

  for (i=0; i<5; i++)
    code_base = g_strdup_printf ("%s%s", code_base, gtk_entry_get_text (GTK_ENTRY (clothes_base_code[i])));

  base_existente = DataExist (g_strdup_printf ("SELECT * FROM clothes_code WHERE codigo_corto like '%s%%'",code_base));

  treeview = GTK_TREE_VIEW (builder_get (builder, "treeview_sizes"));
  selection = gtk_tree_view_get_selection (treeview);
  store = GTK_LIST_STORE (gtk_tree_view_get_model (treeview));

  if (gtk_tree_selection_get_selected (selection, NULL, &iter))
    {
      // Comprueba que no sea una fila de una talla pre-existente
      // (Que no se haya agregado a través de una consulta sql)
      gtk_tree_model_get (GTK_TREE_MODEL (store), &iter,
                          0, &datoExistente,
			  1, &codTalla,
                          -1);

      if (datoExistente == FALSE || (datoExistente == TRUE && base_existente == FALSE))
	{
	  position = atoi (gtk_tree_model_get_string_from_iter(GTK_TREE_MODEL(store), &iter));
	  gtk_list_store_remove (store, &iter);
	  select_back_deleted_row("treeview_sizes", position);
	}
      else
	ErrorMSG (GTK_WIDGET (builder_get (builder, "btn_del_size")), 
		  g_strdup_printf ("No puede eliminar la talla %s, pues existen productos con ella", codTalla));
    }
}


/**
 * Is called by 'btn_add_color' button when is pressed (signal-clicked)
 *
 * make a new row on 'treeview_color'
 */
void
on_btn_add_color_clicked ()
{
  GtkTreeView *treeview;
  GtkListStore *store;
  GtkTreeIter iter;

  treeview = GTK_TREE_VIEW (builder_get (builder, "treeview_colors"));
  store = GTK_LIST_STORE (gtk_tree_view_get_model (treeview));

  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter,
		      0, FALSE,
		      1, "Cod",
		      2, "Color",
		      -1);
}


/**
 * Is called by 'btn_del_color' button when is pressed (signal-clicked)
 *
 * delete the selected row on 'treeview_colors'
 */
void
on_btn_del_color_clicked ()
{
  GtkTreeView *treeview;
  GtkListStore *store;
  GtkTreeSelection *selection;
  GtkTreeIter iter;
  gint position;
  gboolean datoExistente;
  gchar *codColor;
  gchar *nombreColor;

  gboolean base_existente;
  gint i = 0;
  gchar *code_base = g_strdup ("");
  GObject *clothes_base_code[5];
  clothes_base_code[0] = builder_get (builder, "entry_clothes_depto");
  clothes_base_code[1] = builder_get (builder, "entry_clothes_temp");
  clothes_base_code[2] = builder_get (builder, "entry_clothes_year");
  clothes_base_code[3] = builder_get (builder, "entry_clothes_sub_depto");
  clothes_base_code[4] = builder_get (builder, "entry_clothes_id");

  for (i=0; i<5; i++)
    code_base = g_strdup_printf ("%s%s", code_base, gtk_entry_get_text (GTK_ENTRY (clothes_base_code[i])));

  base_existente = DataExist (g_strdup_printf ("SELECT * FROM clothes_code WHERE codigo_corto like '%s%%'",code_base));

  treeview = GTK_TREE_VIEW (builder_get (builder, "treeview_colors"));
  selection = gtk_tree_view_get_selection (treeview);
  store = GTK_LIST_STORE (gtk_tree_view_get_model (treeview));

  if (gtk_tree_selection_get_selected (selection, NULL, &iter))
    {
      // Comprueba que no sea una fila de una talla pre-existente
      // (Que no se haya agregado a través de una consulta sql)
      gtk_tree_model_get (GTK_TREE_MODEL (store), &iter,
                          0, &datoExistente,
			  1, &codColor,
			  2, &nombreColor,
                          -1);

      if (datoExistente == FALSE || (datoExistente == TRUE && base_existente == FALSE))
	{
	  position = atoi (gtk_tree_model_get_string_from_iter(GTK_TREE_MODEL(store), &iter));
	  gtk_list_store_remove (store, &iter);
	  select_back_deleted_row("treeview_colors", position);
	}
      else
	ErrorMSG (GTK_WIDGET (builder_get (builder, "btn_del_color")), 
		  g_strdup_printf ("No puede eliminar el color %s, pues existen productos con él", nombreColor));
    }
}


/**
 * Esta funcion es llamada cuando el boton "btn_save_unidad" es presionado
 * (signal click).
 *
 * Esta funcion agregar la nueva unidad que se ingreso en "entry_unidad" a la
 * base datos
 *
 */
void
save_new_unit (GtkButton *button, gpointer user_data)
{
  GtkTreeIter iter;
  GtkComboBox *combo;
  GtkWindow *parent, *win;
  GtkButton *btn;
  GtkListStore *store;
  //GtkCellRenderer *cell;
  PGresult *res2;
  gint i, tuples;
  GtkTreeModel *model;
  gboolean valid;
  gchar *unidad = g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_unidad"))));
  gchar *nombre_boton = g_strdup (gtk_buildable_get_name (GTK_BUILDABLE (button)));

  win = GTK_WINDOW (gtk_builder_get_object(builder, "wnd_new_unit"));

  /*Al editar o agregar una mercadería*/
  if (strcmp (nombre_boton, "btn_save_unidad_amd") == 0) //Mercadería corriente
    {
      parent = GTK_WINDOW (gtk_builder_get_object(builder, "wnd_new_product"));
      gtk_window_set_transient_for(win, parent);
      combo = GTK_COMBO_BOX (gtk_builder_get_object(builder, "cmb_box_new_product_unit"));
      btn = GTK_BUTTON (gtk_builder_get_object(builder, "btn_new_unidad"));
    }
  else if (strcmp (nombre_boton, "btn_save_unidad_amp") == 0) //Materia prima
    {
      parent = GTK_WINDOW (gtk_builder_get_object(builder, "wnd_new_mp"));
      gtk_window_set_transient_for(win, parent);
      combo = GTK_COMBO_BOX (gtk_builder_get_object(builder, "cmb_box_new_mp_unit"));
      btn = GTK_BUTTON (gtk_builder_get_object(builder, "btn_new_mp_unidad"));
    }
  else if (strcmp (nombre_boton, "btn_save_unidad_amc") == 0) //derivada
    {
      parent = GTK_WINDOW (gtk_builder_get_object(builder, "wnd_new_mcd"));
      gtk_window_set_transient_for(win, parent);
      combo = GTK_COMBO_BOX (gtk_builder_get_object(builder, "cmb_box_new_mcd_unit"));
      btn = GTK_BUTTON (gtk_builder_get_object(builder, "btn_new_mcd_unidad"));
    }
  else if (strcmp (nombre_boton, "btn_save_unidad_amcc") == 0) //compuesta
    {
      parent = GTK_WINDOW (gtk_builder_get_object (builder, "wnd_new_compuesta"));
      gtk_window_set_transient_for (win, parent);
      combo = GTK_COMBO_BOX (gtk_builder_get_object (builder, "cmb_new_comp_unit"));
      btn = GTK_BUTTON (gtk_builder_get_object (builder, "btn_new_mcc_unidad"));
    }
  else if (strcmp (nombre_boton, "btn_save_unidad_em") == 0) //Editar Mercadería (cualquiera)
    {
      parent = GTK_WINDOW (gtk_builder_get_object(builder, "wnd_mod_product"));
      gtk_window_set_transient_for(win, parent);
      combo = GTK_COMBO_BOX (gtk_builder_get_object(builder, "cmb_box_edit_product_unit"));
      btn = GTK_BUTTON (gtk_builder_get_object(builder, "btn_edit_unidad"));
    }

  gchar *q = g_strdup_printf("SELECT * FROM unidad_producto where descripcion = upper('%s')", unidad);
  res2 = EjecutarSQL(q);
  tuples = PQntuples (res2);

  if(tuples > 0)
    {
      gtk_widget_hide(GTK_WIDGET(win));
      gtk_entry_set_text(GTK_ENTRY (gtk_builder_get_object (builder, "entry_unidad")), "");
      ErrorMSG (GTK_WIDGET (btn), "La unidad que ingreso ya existe.");
    }
  else
    {
      q = g_strdup_printf("INSERT INTO unidad_producto VALUES (DEFAULT, upper('%s'))", unidad);

      res2 = EjecutarSQL(q);

      res2 = EjecutarSQL ("SELECT * FROM unidad_producto");
      tuples = PQntuples (res2);

      /* metodo parcial para limpiar un combo box  */
      /* TODO: debe haber un metodo mas eficiente */
      i=0;
      do
	{
	  model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));

	  if ((valid = gtk_tree_model_get_iter_first (model, &iter)) == 1 && i<tuples-1)
	    {
	      store = GTK_LIST_STORE(gtk_combo_box_get_model(combo));

	      gtk_list_store_remove(store, &iter);
	      i++;
	    }
	  else
	    valid = 0;
	}
      while(valid == 1);

      /* Ahora repoblar el combo box */
      res2 = EjecutarSQL ("SELECT * FROM unidad_producto");
      tuples = PQntuples (res2);

      if(res2 != NULL)
	{
	  for (i=0 ; i < tuples ; i++)
	    {
	      gtk_list_store_append(GTK_LIST_STORE(model), &iter);
	      gtk_list_store_set(GTK_LIST_STORE(model), &iter,
				 0, atoi(PQvaluebycol(res2, i, "id")),
				 1, PQvaluebycol(res2, i, "descripcion"),
				 -1);
	    }
	}

      model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));

      gtk_combo_box_set_active(combo, tuples-1);
      gtk_widget_hide(GTK_WIDGET(win));
      gtk_entry_set_text(GTK_ENTRY (gtk_builder_get_object (builder, "entry_unidad")), "");
    }
}


void
on_button_buy_clicked (GtkButton *button, gpointer data)
{
  if (compra->header_compra != NULL)
    BuyWindow ();
}


void
on_button_ok_ingress_clicked (GtkButton *button, gpointer data)
{
  GtkWindow *wnd_ingress = GTK_WINDOW (gtk_builder_get_object (builder, "wnd_ingress_buy"));
  gboolean complete = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (gtk_builder_get_object (builder, "radiobutton_ingress_complete")));
  gboolean factura = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (gtk_builder_get_object (builder, "radiobutton_ingress_document")));

  gchar str_date[256];
  GtkEntry *entry;
  GDate *date = g_date_new ();

  gtk_widget_hide_all (GTK_WIDGET (wnd_ingress));

  if (complete)
    {
      if (factura)
        {
          GtkWindow *wnd_invoice = GTK_WINDOW (gtk_builder_get_object (builder, "wnd_ingress_invoice"));

	  // Para que acepte solo numeros //TODO: Hacer esto más limpio...
	  g_signal_connect (G_OBJECT (builder_get (builder, "entry_int_ingress_factura_n")), "insert-text",
			    G_CALLBACK (only_numberi_filter), NULL);

          clean_container (GTK_CONTAINER (wnd_invoice));

          entry = GTK_ENTRY (builder_get (builder, "entry_ingress_factura_date"));

	  /*Total invoice amount*/
          gtk_entry_set_text (GTK_ENTRY (builder_get (builder, "entry_products_invoice_amount")),
			      CutPoints (g_strdup (gtk_label_get_text (GTK_LABEL (builder_get (builder, "label_pending_total"))))));
          //gtk_editable_select_region (GTK_EDITABLE (builder_get (builder, "entry_ingress_factura_amount")), 0, -1);

	  gtk_label_set_text (GTK_LABEL (builder_get (builder, "subtotal_shipping_invoice")), "0") ;

	  /* Suggested amount */
	  gtk_entry_set_text (GTK_ENTRY (builder_get (builder, "entry_int_total_invoice")),
			      CutPoints (g_strdup (gtk_label_get_text (GTK_LABEL (builder_get (builder, "label_pending_total"))))));

	  gtk_widget_grab_focus (GTK_WIDGET (builder_get (builder, "entry_int_ingress_factura_n")));
          gtk_widget_show_all (GTK_WIDGET (wnd_invoice));
        }
      else
        {
          GtkWindow *wnd_guide = GTK_WINDOW (gtk_builder_get_object (builder, "wnd_ingress_guide"));
          clean_container (GTK_CONTAINER (wnd_guide));

          entry = GTK_ENTRY (builder_get (builder, "entry_ingress_guide_date"));

          /* Suggested amount */
          gtk_entry_set_text (GTK_ENTRY (builder_get (builder, "entry_ingress_guide_amount")),
                              CutPoints (g_strdup (gtk_label_get_text (GTK_LABEL (gtk_builder_get_object (builder, "label_pending_total"))))));
          gtk_editable_select_region (GTK_EDITABLE (builder_get (builder, "entry_ingress_guide_amount")), 0, -1);

          gtk_widget_show_all (GTK_WIDGET (wnd_guide));
        }
    }
  else
    {
      if (factura)
        {
          GtkListStore *store;
          GtkTreeView *treeview = GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_partial_invoice"));
          GtkTreeViewColumn *column;
          GtkCellRenderer *renderer;

          if (gtk_tree_view_get_model (treeview) == NULL)
            {
              store = gtk_list_store_new (8,
                                          G_TYPE_STRING,
                                          G_TYPE_STRING,
                                          G_TYPE_STRING,
                                          G_TYPE_DOUBLE,
                                          G_TYPE_DOUBLE,
                                          G_TYPE_STRING,
                                          G_TYPE_STRING,
                                          G_TYPE_BOOLEAN);

              gtk_tree_view_set_model (GTK_TREE_VIEW (treeview), GTK_TREE_MODEL (store));

              renderer = gtk_cell_renderer_text_new ();
              column = gtk_tree_view_column_new_with_attributes ("Codigo", renderer,
                                                                 "text", 0,
                                                                 "foreground", 6,
                                                                 "foreground-set", 7,
                                                                 NULL);
              gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
              gtk_tree_view_column_set_alignment (column, 0.5);
              g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
              gtk_tree_view_column_set_resizable (column, FALSE);

              renderer = gtk_cell_renderer_text_new ();
              column = gtk_tree_view_column_new_with_attributes ("Producto", renderer,
                                                                 "text", 1,
                                                                 "foreground", 6,
                                                                 "foreground-set", 7,
                                                                 NULL);
              gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
              gtk_tree_view_column_set_alignment (column, 0.5);
              gtk_tree_view_column_set_resizable (column, FALSE);

              renderer = gtk_cell_renderer_text_new ();
              column = gtk_tree_view_column_new_with_attributes ("Unit.", renderer,
                                                                 "text", 2,
                                                                 "foreground", 6,
                                                                 "foreground-set", 7,
                                                                 NULL);
              gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
              gtk_tree_view_column_set_alignment (column, 0.5);
              gtk_tree_view_column_set_resizable (column, FALSE);

              renderer = gtk_cell_renderer_text_new ();
              column = gtk_tree_view_column_new_with_attributes ("Cant. Sol.", renderer,
                                                                 "text", 3,
                                                                 "foreground", 6,
                                                                 "foreground-set", 7,
                                                                 NULL);
              gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
              gtk_tree_view_column_set_alignment (column, 0.5);
              g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
              gtk_tree_view_column_set_resizable (column, FALSE);

              gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)3, NULL);

              renderer = gtk_cell_renderer_text_new ();
              g_object_set (renderer,
                            "editable", TRUE,
                            NULL);
              g_signal_connect (G_OBJECT (renderer), "edited",
                                G_CALLBACK (on_partial_cell_renderer_edited), (gpointer)store);

              column = gtk_tree_view_column_new_with_attributes ("Cant. Ing.", renderer,
                                                                 "text", 4,
                                                                 "foreground", 6,
                                                                 "foreground-set", 7,
                                                                 NULL);
              gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
              gtk_tree_view_column_set_alignment (column, 0.5);
              g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
              gtk_tree_view_column_set_resizable (column, FALSE);

              gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)4, NULL);

              renderer = gtk_cell_renderer_text_new ();
              column = gtk_tree_view_column_new_with_attributes ("Sub-Total", renderer,
                                                                 "text", 5,
                                                                 "foreground", 6,
                                                                 "foreground-set", 7,
                                                                 NULL);
              gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
              gtk_tree_view_column_set_alignment (column, 0.5);
              g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
              gtk_tree_view_column_set_resizable (column, FALSE);
            } // if (gtk_tree_view_get_model (treeview) == NULL)

          clean_container (GTK_CONTAINER (builder_get (builder, "wnd_ingress_partial_invoice")));

          entry = GTK_ENTRY (builder_get (builder, "entry_ingress_partial_invoice_date"));

          /* Suggested amount */
          gtk_entry_set_text (GTK_ENTRY (builder_get (builder, "entry_ingress_partial_invoice_amount")),
                              CutPoints (g_strdup (gtk_label_get_text (GTK_LABEL (gtk_builder_get_object (builder, "label_pending_total"))))));
          gtk_editable_select_region (GTK_EDITABLE (builder_get (builder, "entry_ingress_partial_invoice_amount")), 0, -1);


          FillPartialTree (treeview);
          gtk_widget_show_all (GTK_WIDGET (builder_get (builder, "wnd_ingress_partial_invoice")));
        } // if (factura)
      else
        {
          GtkListStore *store;
          GtkTreeView *treeview = GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_partial_guide"));
          GtkTreeViewColumn *column;
          GtkCellRenderer *renderer;

          if (gtk_tree_view_get_model (treeview) == NULL)
            {
              store = gtk_list_store_new (8,
                                          G_TYPE_STRING,
                                          G_TYPE_STRING,
                                          G_TYPE_STRING,
                                          G_TYPE_DOUBLE,
                                          G_TYPE_DOUBLE,
                                          G_TYPE_STRING,
                                          G_TYPE_STRING,
                                          G_TYPE_BOOLEAN);

              gtk_tree_view_set_model (GTK_TREE_VIEW (treeview), GTK_TREE_MODEL (store));

              renderer = gtk_cell_renderer_text_new ();
              column = gtk_tree_view_column_new_with_attributes ("Codigo", renderer,
                                                                 "text", 0,
                                                                 "foreground", 6,
                                                                 "foreground-set", 7,
                                                                 NULL);
              gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
              gtk_tree_view_column_set_alignment (column, 0.5);
              g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
              gtk_tree_view_column_set_resizable (column, FALSE);

              renderer = gtk_cell_renderer_text_new ();
              column = gtk_tree_view_column_new_with_attributes ("Producto", renderer,
                                                                 "text", 1,
                                                                 "foreground", 6,
                                                                 "foreground-set", 7,
                                                                 NULL);
              gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
              gtk_tree_view_column_set_alignment (column, 0.5);
              gtk_tree_view_column_set_resizable (column, FALSE);

              renderer = gtk_cell_renderer_text_new ();
              column = gtk_tree_view_column_new_with_attributes ("Unit.", renderer,
                                                                 "text", 2,
                                                                 "foreground", 6,
                                                                 "foreground-set", 7,
                                                                 NULL);
              gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
              gtk_tree_view_column_set_alignment (column, 0.5);
              gtk_tree_view_column_set_resizable (column, FALSE);

              renderer = gtk_cell_renderer_text_new ();
              column = gtk_tree_view_column_new_with_attributes ("Cant. Sol.", renderer,
                                                                 "text", 3,
                                                                 "foreground", 6,
                                                                 "foreground-set", 7,
                                                                 NULL);
              gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
              gtk_tree_view_column_set_alignment (column, 0.5);
              g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
              gtk_tree_view_column_set_resizable (column, FALSE);

              gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)3, NULL);

              renderer = gtk_cell_renderer_text_new ();
              g_object_set (renderer,
                            "editable", TRUE,
                            NULL);
              g_signal_connect (G_OBJECT (renderer), "edited",
                                G_CALLBACK (on_partial_cell_renderer_edited), (gpointer)store);

              column = gtk_tree_view_column_new_with_attributes ("Cant. Ing.", renderer,
                                                                 "text", 4,
                                                                 "foreground", 6,
                                                                 "foreground-set", 7,
                                                                 NULL);
              gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
              gtk_tree_view_column_set_alignment (column, 0.5);
              g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
              gtk_tree_view_column_set_resizable (column, FALSE);

              gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)4, NULL);

              renderer = gtk_cell_renderer_text_new ();
              column = gtk_tree_view_column_new_with_attributes ("Sub-Total", renderer,
                                                                 "text", 5,
                                                                 "foreground", 6,
                                                                 "foreground-set", 7,
                                                                 NULL);
              gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
              gtk_tree_view_column_set_alignment (column, 0.5);
              g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
              gtk_tree_view_column_set_resizable (column, FALSE);
            } // if (gtk_tree_view_get_model (treeview) == NULL)

          clean_container (GTK_CONTAINER (builder_get (builder, "wnd_ingress_partial_guide")));

          entry = GTK_ENTRY (builder_get (builder, "entry_ingress_partial_guide_date"));

          /* Suggested amount */
          gtk_entry_set_text (GTK_ENTRY (builder_get (builder, "entry_ingress_partial_guide_amount")),
                              CutPoints (g_strdup (gtk_label_get_text (GTK_LABEL (gtk_builder_get_object (builder, "label_pending_total"))))));
          gtk_editable_select_region (GTK_EDITABLE (builder_get (builder, "entry_ingress_partial_guide_amount")), 0, -1);

          FillPartialTree (treeview);
          gtk_widget_show_all (GTK_WIDGET (builder_get (builder, "wnd_ingress_partial_guide")));
        } // else => if (factura)
    } // else => if (complete)

  g_date_set_time_t (date, time (NULL));

  if (g_date_strftime (str_date, sizeof (str_date), "%x", date) == 0)
    strcpy (str_date, "---");

  gtk_entry_set_text (entry, str_date);
} // void on_button_ok_ingress_clicked (GtkButton *button, gpointer data)


void
on_quit_message_response (GtkDialog *dialog, gint response_id, gpointer user_data)
{
  if (response_id == GTK_RESPONSE_YES)
    {
      gtk_main_quit();
    }
  else
    if (response_id == GTK_RESPONSE_NO)
      gtk_widget_hide (GTK_WIDGET (dialog));
}


gboolean
on_wnd_compras_delete_event (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
  GtkWidget *window;
  window = GTK_WIDGET (gtk_builder_get_object (builder, "quit_message"));
  gtk_widget_show_all (window);
  return TRUE;
}


/* void */
/* on_entry_ingress_factura_amount_activate (GtkWidget *btn_ok) */
/* { */
/*   gint total = atoi (CutPoints(g_strdup (gtk_label_get_text (GTK_LABEL (gtk_builder_get_object (builder, "label_pending_total")))))); */
/*   gint total_doc = atoi (g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_ingress_factura_amount"))))); */

/*   CheckMontoIngreso (btn_ok, total, total_doc); */
/* } */


void
on_btn_ok_ingress_invoice_clicked (GtkWidget *widget, gpointer data)
{
  GtkWidget *wnd = GTK_WIDGET (gtk_builder_get_object (builder, "wnd_ingress_invoice"));

  AskElabVenc (wnd, TRUE);
  //gtk_widget_set_sensitive( GTK_WIDGET (gtk_builder_get_object (builder, "btn_ok_ingress_invoice")), FALSE);
}


void
on_entry_ingress_guide_amount_activate (GtkWidget *btn_ok)
{
  gint total = atoi (CutPoints(g_strdup (gtk_label_get_text (GTK_LABEL (gtk_builder_get_object (builder, "label_pending_total"))))));
  gint total_doc = atoi (g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_ingress_guide_amount")))));

  CheckMontoIngreso (btn_ok, total, total_doc);
}


void
on_btn_ok_ingress_guide_clicked (GtkWidget *widget, gpointer data)
{
  GtkWidget *wnd = GTK_WIDGET (gtk_builder_get_object (builder, "wnd_ingress_guide"));

  AskElabVenc (wnd, FALSE);
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
      str_axu = g_strconcat (PQvaluebycol (res, i, "rut"),
			     PQvaluebycol (res, i, "dv"), NULL);

      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
                          0, PQvaluebycol (res, i, "nombre"),
                          1, str_axu,
                          -1);
      g_free (str_axu);
    }
}


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
      gtk_tree_view_column_set_cell_data_func (column, renderer, control_rut, (gpointer)1, NULL);
    }

  window = GTK_WINDOW (gtk_builder_get_object (builder, "wnd_srch_provider"));
  gtk_entry_set_text (GTK_ENTRY (gtk_builder_get_object(builder, "entry_srch_provider")), str_schr);
  on_entry_srch_provider_activate(entry);

  gtk_widget_show_all (GTK_WIDGET (window));
}


void
on_entry_guide_invoice_provider_activate (GtkEntry *entry, gpointer data)
{
  wnd_search_provider (entry, data);
}


void
on_btn_find_srch_provider_clicked (GtkEntry *entry, gpointer data)
{
  on_entry_srch_provider_activate (entry);
}


/**
 * Is called by "btn_nullify_buy_search" (signal click).
 *
 * this function shows the products that meet the search criteria
 * on treeview "treeview_nullify_buy"
 * 
 * @param: GtkButton *button: button from signal is triggered
 * @param: gpointer user_data: user data
 */
void
on_btn_nullify_buy_search_clicked (GtkButton *button, gpointer user_data)
{
  /*Treeview*/
  GtkTreeView *treeview;
  GtkListStore *store;
  GtkTreeIter iter;

  /*Limpiando Treeview*/
  treeview = GTK_TREE_VIEW (gtk_builder_get_object (builder, "treeview_nullify_buy"));
  store = GTK_LIST_STORE (gtk_tree_view_get_model (treeview));
  gtk_list_store_clear (store);

  /*Limpiando Labels*/
  gtk_label_set_text (GTK_LABEL (builder_get (builder, "lbl_nullify_buy_provider_name")), "");
  gtk_label_set_text (GTK_LABEL (builder_get (builder, "lbl_nullify_buy_provider_rut")), "");

  /*Fecha*/
  GDate *date_aux;
  date_aux = g_date_new();

  /*Obteniendo datos de los entry*/
  gchar *id_compra = g_strdup (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_nullify_buy_id"))));
  gchar *num_factura = g_strdup (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_nullify_buy_invoice"))));
  gchar *proveedor = g_strdup (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_nullify_buy_provider"))));
  gchar *str_date = g_strdup (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_nullify_buy_date"))));
  
  if (!g_str_equal (str_date, ""))
    {
      g_date_set_parse (date_aux, str_date);
      str_date = g_strdup_printf("%d-%d-%d",
				 g_date_get_year(date_aux),
				 g_date_get_month(date_aux),
				 g_date_get_day(date_aux));
    }
  
  gchar *q, *q2;
  PGresult *res;
  gint tuples, i;

  q = g_strdup_printf ("SELECT c.id, "
		       "date_part ('year', c.fecha) AS year, "
		       "date_part ('month', c.fecha) AS month, "
		       "date_part ('day', c.fecha) AS day, "
		       "date_part ('hour', c.fecha) AS hour, "
		       "date_part ('minute', c.fecha) AS minute, "
		       "c.rut_proveedor, "
		       "(SELECT dv FROM proveedor WHERE rut = c.rut_proveedor) AS dv_proveedor, "
		       "(SELECT nombre FROM proveedor WHERE rut = c.rut_proveedor) AS nombre_proveedor, "
		       "(SELECT SUM (cantidad * precio) "
		       "FROM factura_compra_detalle fcd "
		       "INNER JOIN factura_compra fc "
		       "ON fcd.id_factura_compra = fc.id "
		       "AND fc.id_compra = c.id) AS monto "
		       "FROM compra c "
		       "INNER JOIN factura_compra fc "
		       "ON c.id = fc.id_compra "
		       "WHERE c.anulada_pi = 'f' ");

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
	  wnd_search_provider (GTK_ENTRY (builder_get (builder, "entry_nullify_buy_provider")), NULL);
	  return;
	}

      else if (tuples == 1)
	q = g_strdup_printf (" %s AND fc.rut_proveedor = %s", q, PQvaluebycol(res,0,"rut"));
      else if (tuples == 0)
	AlertMSG (GTK_WIDGET (builder_get (builder, "entry_nullify_buy_provider")), 
		  "No existen proveedores con el nombre o rut que ha especificado");
    }

  if (g_date_valid (date_aux))
    q = g_strdup_printf (" %s AND c.fecha BETWEEN '%s' AND '%s 23:59:59'", q, str_date, str_date);

  // Se ejecuta la consulta construida
  res = EjecutarSQL (q);
  g_free (q);  
  tuples = PQntuples (res);

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
			  2, atoi (g_strdup (PQvaluebycol (res, i, "monto"))),
			  3, PQvaluebycol (res, i, "nombre_proveedor"),
			  4, g_strconcat (PQvaluebycol (res, i, "rut_proveedor"),
					  PQvaluebycol (res, i, "dv_proveedor"), NULL),
			  -1);
    }

  PQclear (res);

  //Si existe algo en la estructura "lista_mod_prod" se limpia
  clean_lista_mod_prod ();
}

/**
 * Esta funcion guarda (efectúa) todos los cambios
 * que se hayan realizado en la factura (detallados en lista_mod_prod)
 * 
 */
void
on_btn_save_mod_buy_clicked (GtkButton *button, gpointer data)
{
  //TODO: Ahora al momento de comprar, se debe guardar el costo_promedio
  //      Anulacion de compras debe recalcular el costo_promedio de los productos

  Prods *header;
  Prods *actual;
  gboolean error;

  header = lista_mod_prod->header;
  actual = lista_mod_prod->header;

  if (header == NULL)
    return;

  error = FALSE;
  do  /*Recorre los productos modificados y llama a 
	una función dependiendo de la acción asignada*/
    { //TODO: el error debe especificar el producto PEEEEEEE
      if (actual->prod->accion == ADD) {
	if (mod_to_add_on_buy (actual->prod) == FALSE) {
	  error = TRUE;
	}
      } else if (actual->prod->accion == MOD) {
	if (mod_to_mod_on_buy (actual->prod) == FALSE) {
	  error = TRUE;
	}
      } else if (actual->prod->accion == DEL) {
	if (mod_to_del_on_buy (actual->prod) == FALSE) {
	  error = TRUE;
	}
      }
      actual = actual->next;
    } while (actual != header);

  if (error == FALSE)
    AlertMSG (GTK_WIDGET (builder_get (builder, "btn_nullify_buy_cancel")),
	      "Se realizaron los cambios!");
  else
    ErrorMSG (GTK_WIDGET (builder_get (builder, "btn_nullify_buy_cancel")),
	      "No se logró efecutar la modificacion en compras");
  
  //clean_lista_mod_prod (); La funcion que busca limpia la lista
  on_btn_nullify_buy_search_clicked (GTK_BUTTON (builder_get (builder, "btn_nullify_buy_search")), NULL);
  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_save_mod_buy")), FALSE);
}


/**
 *
 *
 */
void
on_btn_ok_srch_provider_clicked (GtkTreeView *tree)
{
  GtkTreeSelection *selection = gtk_tree_view_get_selection (tree);
  GtkTreeModel *model = gtk_tree_view_get_model (tree);
  GtkTreeIter iter;
  gchar *str;
  //gchar **strs;
  //gint tab = gtk_notebook_get_current_page ( GTK_NOTEBOOK (builder_get (builder, "buy_notebook")));
  //gboolean guide;

  if (gtk_tree_selection_get_selected (selection, NULL, &iter) == TRUE)
    {
      gtk_tree_model_get (model, &iter,
                          1, &str,
                          -1);

      str = g_strndup (str, strlen (str)-1);

      if (gtk_widget_get_visible (GTK_WIDGET (builder_get (builder, "wnd_nullify_buy"))))
	{
	  gtk_entry_set_text (GTK_ENTRY (builder_get (builder, "entry_nullify_buy_provider")), str);
	  on_btn_nullify_buy_search_clicked (NULL, NULL);
	}
      else
	{
	  //guide = tab == 2 ? TRUE : FALSE; //Ex page 2 "Ingreso Factura" is disabled
	  FillProveedorData (str, FALSE);
	}

      gtk_widget_hide (GTK_WIDGET (builder_get (builder, "wnd_srch_provider")));
    }
}


void
on_tree_view_srch_provider_row_activated (GtkTreeView *tree)
{
  on_btn_ok_srch_provider_clicked (tree);
}


void
on_btn_guide_invoice_clicked (GtkButton *button, gpointer data)
{
  GtkTreeView *tree_pending_guide = GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_pending_guide"));
  GtkTreeView *tree_guide_invoice = GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_guide_invoice"));
  GtkTreeSelection *selection = gtk_tree_view_get_selection (tree_pending_guide);
  GtkTreeModel *model = gtk_tree_view_get_model (tree_pending_guide);
  GtkTreeIter iter;
  gchar *id;
  gchar *n_guide;
  gchar *monto;

  if (gtk_tree_selection_get_selected (selection, NULL, &iter) == TRUE)
    {
      gtk_tree_model_get (model, &iter,
                          0, &id,
                          1, &n_guide,
                          2, &monto,
                          -1);
      gtk_list_store_remove (GTK_LIST_STORE (model), &iter);


      model = gtk_tree_view_get_model (tree_guide_invoice);

      gtk_list_store_append (GTK_LIST_STORE (model), &iter);
      gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                          0, id,
                          1, n_guide,
                          2, monto,
                          -1);
    }
}


void
on_btn_invoice_guide_clicked (GtkButton *button, gpointer date)
{
  GtkTreeView *tree_pending_guide = GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_pending_guide"));
  GtkTreeView *tree_guide_invoice = GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_guide_invoice"));
  GtkTreeSelection *selection = gtk_tree_view_get_selection (tree_guide_invoice);
  GtkTreeModel *model = gtk_tree_view_get_model (tree_guide_invoice);
  GtkTreeIter iter;
  gchar *id;
  gchar *n_guide;
  gchar *monto;

  if (gtk_tree_selection_get_selected (selection, NULL, &iter) == TRUE)
    {
      gtk_tree_model_get (model, &iter,
                          0, &id,
                          1, &n_guide,
                          2, &monto,
                          -1);
      gtk_list_store_remove (GTK_LIST_STORE (model), &iter);

      model = gtk_tree_view_get_model (tree_pending_guide);

      gtk_list_store_append (GTK_LIST_STORE (model), &iter);
      gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                          0, id,
                          1, n_guide,
                          2, monto,
                          -1);
    }
}


void
on_tree_selection_pending_guide_changed (GtkTreeSelection *selection, gpointer user_data)
{
  GtkTreeView *tree = GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_guide_invoice_detail"));
  GtkTreeModel *model = gtk_tree_view_get_model (gtk_tree_selection_get_tree_view (selection));
  GtkTreeIter iter;
  gchar *n_guide;
  gchar *rut_provider;

  PGresult *res;
  gint tuples, i;
  gchar *q;

  if (gtk_tree_selection_get_selected (selection, NULL, &iter) == TRUE)
    {
      gtk_tree_model_get (model, &iter,
                          1, &n_guide,
                          -1);
      rut_provider = g_strdup (gtk_label_get_text (GTK_LABEL (gtk_builder_get_object (builder, "label_guide_invoice_rut"))));

      q = g_strdup_printf ("SELECT * FROM get_guide_detail(%s, %s)", n_guide, rut_provider);
      res = EjecutarSQL (q);
      g_free (q);

      if (res == NULL) return;

      tuples = PQntuples (res);

      model = gtk_tree_view_get_model (tree);
      gtk_list_store_clear (GTK_LIST_STORE (model));

      for (i = 0; i < tuples; i++)
        {
          gtk_list_store_append (GTK_LIST_STORE (model), &iter);
          gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                              0, PQvaluebycol (res, i, "codigo_corto"),
                              1, g_strdup_printf ("%s %s %s %s", PQvaluebycol (res, i, "descripcion"),
                                                  PQvaluebycol (res, i, "marca"), PQvaluebycol (res, i, "contenido"),
                                                  PQvaluebycol (res, i, "unidad")),
                              2, PQvaluebycol(res, i, "precio"),
                              3, strtod (PUT(PQvaluebycol(res, i, "cantidad")), (char **)NULL),
                              4, (gdouble) (strtod (PUT(PQvaluebycol(res, i, "precio")), (char **)NULL) * strtod (PUT(PQvaluebycol(res, i, "cantidad")), (char **)NULL)),
                              -1);
        }
    }
}


void
on_btn_guide_invoice_ok_clicked (void)
{
  PGresult *res;

  GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_guide_invoice")));
  GtkTreeIter iter;

  gchar *id;
  gchar *guia;
  gint factura;

  gchar *rut = g_strdup (gtk_label_get_text (GTK_LABEL (gtk_builder_get_object (builder, "label_guide_invoice_rut"))));
  gchar *monto = g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_guide_invoice_amount"))));

  gint n_fact = atoi (g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_guide_invoice_n_invoice")))));

  gchar *date_str = g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_guide_invoice_date"))));
  GDate *date = g_date_new ();
  GDate *date_guide = g_date_new ();

  g_date_set_parse (date, date_str);

  if (!g_date_valid (date))
    {
      ErrorMSG (GTK_WIDGET (builder_get (builder, "entry_guide_invoice_date")),
                "Debe ingresar una fecha valida");
      return;
    }

  gtk_tree_model_get_iter_first (model, &iter);

  gtk_tree_model_get (model, &iter,
                      1, &guia,
                      -1);

  res = EjecutarSQL (g_strdup_printf
                     ("SELECT date_part('day', fecha), date_part('month', fecha), "
                      "date_part('year', fecha) FROM compra WHERE id=(SELECT id_compra FROM "
                      "guias_compra WHERE numero=%s AND rut_proveedor='%s')", guia, rut));

  if (res == NULL || PQntuples (res) == 0)
    return;

  g_date_set_dmy (date_guide, atoi (PQgetvalue( res, 0, 0)), atoi (PQgetvalue( res, 0, 1)), atoi (PQgetvalue( res, 0, 2)));

  if (g_date_compare (date_guide, date) > 0)
    {
      ErrorMSG (GTK_WIDGET (builder_get (builder, "entry_guide_invoice_date")),
                "La fecha de emision del documento no puede ser menor al de la guia");
    }

  if (strcmp (rut, "") == 0)
    {
      ErrorMSG (compra->tree_prov, "Debe Seleccionar un proveedor");
      return;
    }
  else if (n_fact == 0)
    {
      ErrorMSG (compra->n_factura, "Debe Ingresar el numero de la factura");
      return;
    }
  else if (strcmp (monto, "") == 0)
    {
      ErrorMSG (compra->fact_monto, "Debe Ingresar el Monto de la Factura");
      return;
    }

  factura = IngresarFactura (n_fact, 0, rut, atoi (monto), g_date_get_day (date), g_date_get_month (date), g_date_get_year (date), atoi (guia), 0);


  gtk_tree_model_get_iter_first (model, &iter);

  do {
    gtk_tree_model_get (model, &iter,
                        0, &id,
                        -1);

    AsignarFactAGuia (atoi (id), factura);
  }  while ((gtk_tree_model_iter_next (model, &iter)) != FALSE);

  ClearFactData ();
}


void
ClearFactData (void)
{
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_guide_invoice_address")), "");
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_guide_invoice_mail")), "");
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_guide_invoice_comuna")), "");
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_guide_invoice_contact")), "");
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_guide_invoice_fono")), "");
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_guide_invoice_rut")), "");
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_guide_invoice_date_emit")), "");
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_guide_invoice_web")), "");

  gtk_entry_set_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_guide_invoice_provider")), "");
  gtk_entry_set_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_guide_invoice_n_invoice")), "");
  gtk_entry_set_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_guide_invoice_amount")), "");

  gtk_list_store_clear (GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (builder_get (builder, "tree_view_pending_guide")))));
  gtk_list_store_clear (GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (builder_get (builder, "tree_view_guide_invoice")))));
  gtk_list_store_clear (GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (builder_get (builder, "tree_view_guide_invoice_detail")))));
}


void
on_entry_invoice_provider_activate (GtkEntry *entry, gpointer data)
{
  //on_entry_srch_provider_activate(entry);
  wnd_search_provider (entry, data);
}


void
on_tree_view_invoice_list_selection_changed (GtkTreeSelection *selection, gpointer data)
{
  gint i, tuples;
  gchar *id_invoice;
  gchar *q;
  GtkTreeView *tree = GTK_TREE_VIEW (builder_get (builder, "tree_view_invoice_detail"));
  GtkTreeModel *model = gtk_tree_view_get_model (gtk_tree_selection_get_tree_view (selection));
  GtkTreeIter iter;

  PGresult *res;

  if (gtk_tree_selection_get_selected (selection, NULL, &iter) == TRUE)
    {
      gtk_tree_model_get (model, &iter,
                          0, &id_invoice,
                          -1);

      q = g_strdup_printf ("SELECT * FROM get_invoice_detail(%s)", id_invoice);
      res = EjecutarSQL (q);
      g_free (q);

      if (res == NULL) return;

      tuples = PQntuples (res);

      model = gtk_tree_view_get_model (tree);
      gtk_list_store_clear (GTK_LIST_STORE (model));

      for (i = 0; i < tuples; i++)
        {
          gtk_list_store_append (GTK_LIST_STORE (model), &iter);
          gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                              0, PQvaluebycol (res, i, "codigo_corto"),
                              1, g_strdup_printf ("%s %s %s %s", PQvaluebycol (res, i, "descripcion"),
                                                  PQvaluebycol (res, i, "marca"), PQvaluebycol (res, i, "contenido"),
                                                  PQvaluebycol (res, i, "unidad")),
                              2, strtod (PUT (PQvaluebycol (res, i, "precio_compra")), (char **)NULL),
                              3, strtod (PUT (PQvaluebycol (res, i, "cantidad")), (char **)NULL),
                              4, lround (strtod (PUT(PQvaluebycol(res, i, "precio_compra")), (char **)NULL) * strtod (PUT(PQvaluebycol(res, i, "cantidad")), (char **)NULL)),
                              -1);
        }
    }
}


void
on_btn_pay_invoice_clicked (void)
{
  GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (builder_get (builder, "tree_view_invoice_list")));
  GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (builder_get (builder, "tree_view_invoice_list")));
  GtkTreeIter iter;
  gchar *id_invoice;
  gchar *rut_provider;
  gint position;

  if (gtk_tree_selection_get_selected (selection, NULL, &iter) == TRUE)
    {
      gtk_tree_model_get (model, &iter,
                          0, &id_invoice,
			  1, &rut_provider,
                          -1);

      rut_provider = g_strndup (rut_provider, strlen (rut_provider)-1);
      position = atoi (gtk_tree_model_get_string_from_iter(model, &iter));
      
      if (PagarFactura (atoi (id_invoice)) == FALSE)
        {
          ErrorMSG (GTK_WIDGET (builder_get (builder, "entry_invoice_provider")), "No se ingreso correctamente");
        }
      else
        {
          FillPagarFacturas (rut_provider);
	  select_back_deleted_row("tree_view_invoice_list", position);
        }
    }
}


void
FillPartialTree (GtkTreeView *tree)
{
  gint i, id, tuples;
  gboolean color;
  GtkTreeIter iter;

  GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model (tree));
  GtkListStore *store_pending_request = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (builder_get (builder, "tree_view_pending_requests"))));
  GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (builder_get (builder, "tree_view_pending_requests")));

  PGresult *res;

  gdouble sol, ing;
  gdouble cantidad;

  if (compra->header != NULL)
    CompraListClean ();

  if (gtk_tree_selection_get_selected (selection, NULL, &iter) == TRUE)
    {
      gtk_tree_model_get (GTK_TREE_MODEL (store_pending_request), &iter,
                          0, &id,
                          -1);

      gtk_list_store_clear (store);

      res = EjecutarSQL ( g_strdup_printf( "SELECT * FROM get_detalle_compra( %d ) ", id ) );

      if (res == NULL)
        return;

      tuples = PQntuples (res);

      if (tuples == 0)
        return;

      for (i = 0; i < tuples; i++)
        {
          sol = strtod (PUT (PQvaluebycol(res, i, "cantidad")), (char **)NULL);
          ing = strtod (PUT (PQvaluebycol(res, i, "cantidad_ingresada")), (char **)NULL);

          if (ing<sol && ing >0)
            color = TRUE;
          else
            color = FALSE;

          gtk_list_store_append (store, &iter);
          gtk_list_store_set (store, &iter,
                              0, PQvaluebycol(res, i, "codigo_corto"),
                              1, g_strdup_printf ("%s %s %s %s", PQvaluebycol(res, i, "descripcion"),
                                                  PQvaluebycol(res, i, "marca"), PQvaluebycol(res, i, "contenido"),
                                                  PQvaluebycol(res, i, "unidad")),
                              2, PQvaluebycol(res, i, "precio"),
                              3, strtod (PUT(PQvaluebycol(res, i, "cantidad")), (char **)NULL),
                              4, strtod (PUT(PQvaluebycol(res, i, "cantidad_ingresada")), (char **)NULL),
                              5, PutPoints (PQvaluebycol(res, i, "costo_ingresado")),
                              6, color ? "Red" : "Black",
                              7, TRUE,
                              -1);

          if (ing != 0)
            cantidad = (gdouble) sol - ing;
          else
            cantidad = (gdouble) sol;

          CompraAgregarALista (PQvaluebycol(res, i, "barcode"), cantidad, atoi (PQvaluebycol(res, i, "precio_venta")),
                               strtod (PUT((PQvaluebycol(res, i, "precio"))), (char **)NULL), atoi (PQvaluebycol(res, i, "margen")), TRUE);
        }
    } // if (gtk_tree_selection_get_selected (selection, NULL, &iter) == TRUE)
} // void FillPartialTree (GtkTreeView *tree)


void
on_partial_cell_renderer_edited (GtkCellRendererText *cell, gchar *path_string, gchar *new_amount, gpointer data)
{
  GtkTreeModel *model = GTK_TREE_MODEL (data);
  GtkTreePath *path = gtk_tree_path_new_from_string (path_string);
  GtkTreeIter iter;
  gdouble new_stock = strtod (PUT (new_amount), (char **)NULL);
  gchar *codigo;
  Productos *products;

  gtk_tree_model_get_iter (model, &iter, path);
  gtk_tree_path_free (path);

  gtk_tree_model_get (model, &iter,
                      0, &codigo,
                      -1);

  products = BuscarPorCodigo (compra->header, codigo);

  if (new_stock > 0 && new_stock <= products->product->cantidad)
    {
      gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                          4, new_stock,
                          -1);
      products->product->cantidad = new_stock;
      CalcularTotales ();

      if (GTK_WIDGET_VISIBLE (GTK_WIDGET (builder_get (builder, "wnd_ingress_partial_invoice"))))
        {
          gtk_entry_set_text (GTK_ENTRY (builder_get (builder, "entry_ingress_partial_invoice_amount")),
                              CutPoints (g_strdup (gtk_label_get_text (GTK_LABEL (gtk_builder_get_object (builder, "label_pending_total"))))));
          gtk_editable_select_region (GTK_EDITABLE (builder_get (builder, "entry_ingress_partial_invoice_amount")), 0, -1);
        }
      else if (GTK_WIDGET_VISIBLE (GTK_WIDGET (builder_get (builder, "wnd_ingress_partial_guide"))))
        {
          gtk_entry_set_text (GTK_ENTRY (builder_get (builder, "entry_ingress_partial_guide_amount")),
                              CutPoints (g_strdup (gtk_label_get_text (GTK_LABEL (gtk_builder_get_object (builder, "label_pending_total"))))));
          gtk_editable_select_region (GTK_EDITABLE (builder_get (builder, "entry_ingress_partial_guide_amount")), 0, -1);
        }
    }
  else
    {
      ErrorMSG (NULL, "El stock a ingresa debe ser mayor a 0 y menor a la cantidad solicitada");
    }
}


void
on_btn_ok_ingress_partial_guide_clicked (GtkWidget *widget, gpointer data)
{
  GtkWidget *wnd = GTK_WIDGET (gtk_builder_get_object (builder, "wnd_ingress_partial_guide"));

  AskElabVenc (wnd, FALSE);
}


void
on_btn_ok_ingress_partial_invoice_clicked (GtkWidget *widget, gpointer data)
{
  GtkWidget *wnd = GTK_WIDGET (gtk_builder_get_object (builder, "wnd_ingress_partial_invoice"));

  AskElabVenc (wnd, TRUE);
}


void
on_entry_ingress_partial_invoice_amount_activate (GtkWidget *btn_ok)
{
  gint total = atoi (CutPoints(g_strdup (gtk_label_get_text (GTK_LABEL (gtk_builder_get_object (builder, "label_pending_total"))))));
  gint total_doc = atoi (g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_ingress_partial_invoice_amount")))));

  CheckMontoIngreso (btn_ok, total, total_doc);
}


void
on_entry_ingress_partial_guide_amount_activate (GtkWidget *btn_ok)
{
  gint total = atoi (CutPoints(g_strdup (gtk_label_get_text (GTK_LABEL (gtk_builder_get_object (builder, "label_pending_total"))))));
  gint total_doc = atoi (g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_ingress_partial_guide_amount")))));

  CheckMontoIngreso (btn_ok, total, total_doc);
}

/**
 * Esta funcion es llamada cuando el boton "btn_add_new_product" es
 * presionado (signal click).
 *
 * Esta funcion obtiene todos los datos del producto que se llenaron en la
 * ventana "wnd_new_product", para ingresar un nuevo producto a la base de datos
 *
 */
void
on_btn_add_new_product_clicked (GtkButton *button, gpointer data)
{
  GtkTreeModel *model;
  GtkTreeIter iter;

  GtkEntry *entry_code = GTK_ENTRY (builder_get (builder, "entry_new_product_code"));
  GtkEntry *entry_barcode = GTK_ENTRY (builder_get (builder, "entry_new_product_barcode"));
  GtkEntry *entry_desc = GTK_ENTRY (builder_get (builder, "entry_new_product_desc"));
  GtkEntry *entry_brand = GTK_ENTRY (builder_get (builder, "entry_new_product_brand"));
  GtkEntry *entry_cont = GTK_ENTRY (builder_get (builder, "entry_new_product_cont"));

  GtkComboBox *combo = GTK_COMBO_BOX (builder_get (builder, "cmbbox_new_product_imp_others"));
  GtkComboBox *combo_unit = GTK_COMBO_BOX (builder_get (builder, "cmb_box_new_product_unit"));
  GtkComboBox *combo_family = GTK_COMBO_BOX (builder_get (builder, "cmb_new_product_family"));

  gboolean iva = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (builder_get (builder, "radio_btn_task_yes")));
  gboolean fraccion = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (builder_get (builder,
										    "radio_btn_fractional_yes")));
  gchar *codigo = g_strdup (gtk_entry_get_text (entry_code));
  gchar *barcode = g_strdup (gtk_entry_get_text (entry_barcode));
  gchar *description = g_strdup (gtk_entry_get_text (entry_desc));
  gchar *marca = g_strdup (gtk_entry_get_text (entry_brand));
  gchar *contenido = g_strdup (gtk_entry_get_text (entry_cont));
  gchar *unidad;
  gchar *tipo;
  gint otros, familia;

  if (strcmp (codigo, "") == 0)
    ErrorMSG (GTK_WIDGET (entry_code), "Debe ingresar un codigo corto");
  else if (strlen (codigo) > 6)
    ErrorMSG (GTK_WIDGET (entry_code), "Código corto debe ser menor a 7 caracteres");
  else if (strcmp (barcode, "") == 0)
    ErrorMSG (GTK_WIDGET (entry_barcode), "Debe Ingresar un Codigo de Barras");
  else if (HaveCharacters (barcode))
    ErrorMSG (GTK_WIDGET (entry_barcode), "Código de barras debe ser un valor numérico");
  else if (strlen (barcode) > 18)
    ErrorMSG (GTK_WIDGET (entry_barcode), "Código de barras debe ser menor a 18 caracteres");
  else if (strlen (barcode) < 6)
    ErrorMSG (GTK_WIDGET (entry_barcode), "Código de barras debe tener 6 dígitos como mínimo");
  else if (strcmp (description, "") == 0)
    ErrorMSG (GTK_WIDGET (entry_desc), "Debe Ingresar una Descripción");
  else if (strcmp (marca, "") == 0)
    ErrorMSG (GTK_WIDGET (entry_brand), "Debe Ingresar al Marca del producto");
  else if (strcmp (contenido, "") == 0)
    ErrorMSG (GTK_WIDGET (entry_cont), "Debe Ingresar el Contenido del producto");
  else if (!is_numeric (contenido))
    ErrorMSG (GTK_WIDGET (entry_cont), "Contenido debe ser un valor numérico");
  else
    {
      if (DataExist (g_strdup_printf ("SELECT codigo_corto FROM producto WHERE codigo_corto like '%s'", codigo)))
        {
          ErrorMSG (GTK_WIDGET (entry_code), "Ya existe un producto con el mismo codigo corto");
          return;
        }
      else if (DataExist (g_strdup_printf ("SELECT codigo_corto FROM producto WHERE barcode = %s", codigo)))
        {
          ErrorMSG (GTK_WIDGET (entry_code), "Ya existe un producto con este codigo corto de barcode");
          return;
        }

      if (DataExist (g_strdup_printf ("SELECT barcode FROM producto WHERE barcode = %s", barcode)))
        {
          ErrorMSG (GTK_WIDGET (entry_barcode), "Ya existe un producto con el mismo codigo de barras");
          return;
        }
      else if (DataExist (g_strdup_printf ("SELECT barcode FROM producto WHERE codigo_corto = '%s'", barcode)))
        {
          ErrorMSG (GTK_WIDGET (entry_barcode), "Ya existe un producto con este barcode de codigo corto");
          return;
        }


      if (gtk_combo_box_get_active (combo) == 0)
        otros = 0;
      else
        {
          model = gtk_combo_box_get_model (combo);
          gtk_combo_box_get_active_iter (combo, &iter);
          gtk_tree_model_get (model, &iter,
                              0, &otros,
                              -1);
        }
      model = gtk_combo_box_get_model (combo_unit);
      gtk_combo_box_get_active_iter (combo_unit, &iter);

      gtk_tree_model_get (model, &iter,
			  1, &unidad,
			  -1);

      /*familia*/
      model = gtk_combo_box_get_model (combo_family);
      gtk_combo_box_get_active_iter (combo_family, &iter);

      gtk_tree_model_get (model, &iter,
			  0, &familia,
			  -1);

      //Tipo producto (corriente)
      tipo = PQvaluebycol (EjecutarSQL ("SELECT id FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'CORRIENTE'"), 0, "id");

      //crear producto
      AddNewProductToDB (codigo, barcode, description, marca, CUT (contenido),
			 unidad, iva, otros, familia, FALSE, fraccion, atoi (tipo));

      gtk_widget_hide (GTK_WIDGET (builder_get (builder, "wnd_new_product")));

      SearchProductHistory (GTK_ENTRY (gtk_builder_get_object (builder, "entry_buy_barcode")), codigo);
    }
  return;
} // void on_btn_add_new_product_clicked (GtkButton *button, gpointer data)


/**
 * Esta funcion es llamada cuando el boton "btn_add_new_product" es
 * presionado (signal click).
 *
 * Esta funcion obtiene todos los datos de la materia prima que se llenaron en la
 * ventana "wnd_new_mp", para ingresar una nueva materia prima a la base de datos
 */
void
on_btn_add_new_mp_clicked (GtkButton *button, gpointer data)
{
  GtkTreeModel *model;
  GtkTreeIter iter;

  GtkEntry *entry_code = GTK_ENTRY (builder_get (builder, "entry_new_mp_code"));
  GtkEntry *entry_barcode = GTK_ENTRY (builder_get (builder, "entry_new_mp_barcode"));
  GtkEntry *entry_desc = GTK_ENTRY (builder_get (builder, "entry_new_mp_desc"));
  GtkEntry *entry_brand = GTK_ENTRY (builder_get (builder, "entry_new_mp_brand"));
  GtkEntry *entry_cont = GTK_ENTRY (builder_get (builder, "entry_new_mp_cont"));
  GtkEntry *entry_buy_price = GTK_ENTRY (builder_get (builder, "entry_new_mp_buy_price"));
  GtkEntry *entry_cantity = GTK_ENTRY (builder_get (builder, "entry_new_mp_cantity"));

  GtkComboBox *combo = GTK_COMBO_BOX (builder_get (builder, "cmbbox_new_mp_imp_others"));
  GtkComboBox *combo_unit = GTK_COMBO_BOX (builder_get (builder, "cmb_box_new_mp_unit"));
  GtkComboBox *combo_family = GTK_COMBO_BOX (builder_get (builder, "cmb_new_mp_family"));

  gboolean iva = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (builder_get (builder, "radio_btn_mp_task_yes")));
  gboolean fraccion = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (builder_get (builder,
										    "radio_btn_mp_fractional_yes")));
  gchar *codigo = g_strdup (gtk_entry_get_text (entry_code));
  gchar *barcode = g_strdup (gtk_entry_get_text (entry_barcode));
  gchar *description = g_strdup (gtk_entry_get_text (entry_desc));
  gchar *marca = g_strdup (gtk_entry_get_text (entry_brand));
  gchar *contenido = g_strdup (gtk_entry_get_text (entry_cont));
  gchar *costo = g_strdup (gtk_entry_get_text (entry_buy_price));
  gchar *cantidad = g_strdup (gtk_entry_get_text (entry_cantity));
  gchar *unidad;
  gchar *tipo;
  gint otros, familia;

  if (strcmp (codigo, "") == 0)
    ErrorMSG (GTK_WIDGET (entry_code), "Debe ingresar un codigo corto");
  else if (strlen (codigo) > 6)
    ErrorMSG (GTK_WIDGET (entry_code), "Código corto debe ser menor a 7 caracteres");
  else if (strcmp (barcode, "") == 0)
    ErrorMSG (GTK_WIDGET (entry_barcode), "Debe Ingresar un Codigo de Barras");
  else if (HaveCharacters (barcode))
    ErrorMSG (GTK_WIDGET (entry_barcode), "Código de barras debe ser un valor numérico");
  else if (strlen (barcode) > 18)
    ErrorMSG (GTK_WIDGET (entry_barcode), "Código de barras debe ser menor a 18 caracteres");
  else if (strlen (barcode) < 6)
    ErrorMSG (GTK_WIDGET (entry_barcode), "Código de barras debe tener 6 dígitos como mínimo");
  else if (strcmp (description, "") == 0)
    ErrorMSG (GTK_WIDGET (entry_desc), "Debe ingresar una Descripción");
  else if (strcmp (marca, "") == 0)
    ErrorMSG (GTK_WIDGET (entry_brand), "Debe ingresar la Marca de la materia prima");
  else if (strcmp (contenido, "") == 0)
    ErrorMSG (GTK_WIDGET (entry_cont), "Debe ingresar el Contenido la materia prima");
  else if (strcmp (costo, "") == 0)
    ErrorMSG (GTK_WIDGET (entry_buy_price), "Debe ingresar el costo neto de la materia prima");
  else if (!is_numeric (costo))
    ErrorMSG (GTK_WIDGET (entry_buy_price), "Costo (neto) debe ser un valor numérico");
  else if (strcmp (cantidad, "") == 0)
    ErrorMSG (GTK_WIDGET (entry_cantity), "Debe ingresar la cantidad a comprar");
  else if (!is_numeric (cantidad))
    ErrorMSG (GTK_WIDGET (entry_cantity), "Cantidad debe ser un valor numérico");
  else if (!is_numeric (contenido))
    ErrorMSG (GTK_WIDGET (entry_cont), "Contenido debe ser un valor numérico");
  else
    {
      if (DataExist (g_strdup_printf ("SELECT codigo_corto FROM producto WHERE codigo_corto like '%s'", codigo)))
        {
          ErrorMSG (GTK_WIDGET (entry_code), "Ya existe un producto con el mismo codigo corto");
          return;
        }
      else if (DataExist (g_strdup_printf ("SELECT codigo_corto FROM producto WHERE barcode = %s", codigo)))
        {
          ErrorMSG (GTK_WIDGET (entry_code), "Ya existe un producto con este codigo corto de barcode");
          return;
        }

      if (DataExist (g_strdup_printf ("SELECT barcode FROM producto WHERE barcode = %s", barcode)))
        {
          ErrorMSG (GTK_WIDGET (entry_barcode), "Ya existe un producto con el mismo codigo de barras");
          return;
        }
      else if (DataExist (g_strdup_printf ("SELECT barcode FROM producto WHERE codigo_corto = '%s'", barcode)))
        {
          ErrorMSG (GTK_WIDGET (entry_barcode), "Ya existe un producto con este barcode de codigo corto");
          return;
        }

      if (gtk_combo_box_get_active (combo) == 0)
        otros = 0;
      else
        {
          model = gtk_combo_box_get_model (combo);
          gtk_combo_box_get_active_iter (combo, &iter);
          gtk_tree_model_get (model, &iter,
                              0, &otros,
                              -1);
        }
      model = gtk_combo_box_get_model (combo_unit);
      gtk_combo_box_get_active_iter (combo_unit, &iter);

      gtk_tree_model_get (model, &iter,
			  1, &unidad,
			  -1);

      /*familia*/
      model = gtk_combo_box_get_model (combo_family);
      gtk_combo_box_get_active_iter (combo_family, &iter);

      gtk_tree_model_get (model, &iter,
			  0, &familia,
			  -1);

      //Tipo producto (corriente)
      tipo = PQvaluebycol (EjecutarSQL ("SELECT id FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'MATERIA PRIMA'"), 0, "id");
      
      //crear producto
      if (AddNewProductToDB (codigo, barcode, description, marca, CUT (contenido),
			     unidad, iva, otros, familia, FALSE, fraccion, atoi (tipo)))
	EjecutarSQL (g_strdup_printf ("UPDATE producto set costo_promedio = %s WHERE barcode = %s", CUT (costo), barcode));

      //Agrega el producto a la lista de compra
      if (CompraAgregarALista (barcode, strtod (PUT (cantidad), (char **)NULL), 0, strtod (PUT (costo), (char **)NULL), 0, FALSE))
	{
	  AddToTree ();
	  gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "button_buy")), TRUE);
	}
      
      gtk_label_set_text (GTK_LABEL (builder_get (builder, "lbl_barcode_mp")), barcode);
      gtk_label_set_text (GTK_LABEL (builder_get (builder, "lbl_mother_price")), costo);
      gtk_label_set_text (GTK_LABEL (builder_get (builder, "lbl_codigo_mp")), codigo);
      gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_descripcion_mp")), 
			    g_strdup_printf ("<span font_weight='bold' size='12500'>%s %s</span>", description, marca));

      gtk_widget_hide (GTK_WIDGET (builder_get (builder, "wnd_new_mp")));
      gtk_widget_show (GTK_WIDGET (builder_get (builder, "wnd_derivar_mp")));
      gtk_widget_grab_focus (GTK_WIDGET (builder_get (builder, "btn_der_mp_yes")));

      //SearchProductHistory (GTK_ENTRY (gtk_builder_get_object (builder, "entry_buy_barcode")), codigo);
    }
  return;
} // void on_btn_add_new_mp_clicked (GtkButton *button, gpointer data)



/**
 *
 *
 */
void
on_btn_edit_derivates_clicked (GtkButton *button, gpointer data)
{
  gchar *barcode, *costo;

  barcode = g_strdup (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_buy_barcode"))));
  costo = g_strdup (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_buy_price"))));

  gtk_label_set_text (GTK_LABEL (builder_get (builder, "lbl_barcode_mp")), barcode);
  gtk_label_set_text (GTK_LABEL (builder_get (builder, "lbl_mother_price")), costo);

  gtk_widget_hide (GTK_WIDGET (builder_get (builder, "wnd_edit_raw_product")));
  on_btn_der_mp_yes_clicked (button, data);
}


/**
 * Esta funcion es llamada cuando el boton "btn_add_new_product" es
 * presionado (signal click).
 *
 * Esta funcion obtiene todos los datos de la mercadería derivada 
 * que se llenaron en la ventana "wnd_new_mcd", 
 * para ingresar la mercadería correspondiente a la base de datos
 */
void
on_btn_add_new_mcd_clicked (GtkButton *button, gpointer data)
{
  GtkTreeModel *model;
  GtkTreeIter iter;

  GtkEntry *entry_code = GTK_ENTRY (builder_get (builder, "entry_new_mcd_code"));
  GtkEntry *entry_barcode = GTK_ENTRY (builder_get (builder, "entry_new_mcd_barcode"));
  GtkEntry *entry_desc = GTK_ENTRY (builder_get (builder, "entry_new_mcd_desc"));
  GtkEntry *entry_brand = GTK_ENTRY (builder_get (builder, "entry_new_mcd_brand"));
  GtkEntry *entry_cont = GTK_ENTRY (builder_get (builder, "entry_new_mcd_cont"));
  GtkEntry *entry_sell_price = GTK_ENTRY (builder_get (builder, "entry_new_mcd_sell_price"));
  GtkEntry *entry_cantity = GTK_ENTRY (builder_get (builder, "entry_new_mcd_cantity_mother"));

  GtkComboBox *combo = GTK_COMBO_BOX (builder_get (builder, "cmbbox_new_mcd_imp_others"));
  GtkComboBox *combo_unit = GTK_COMBO_BOX (builder_get (builder, "cmb_box_new_mcd_unit"));
  GtkComboBox *combo_family = GTK_COMBO_BOX (builder_get (builder, "cmb_new_mcd_family"));
  GtkListStore *store;

  gboolean iva = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (builder_get (builder, "radio_btn_mcd_task_yes")));
  gboolean fraccion = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (builder_get (builder,
										    "radio_btn_mcd_fractional_yes")));
  gchar *codigo = g_strdup (gtk_entry_get_text (entry_code));
  gchar *barcode = g_strdup (gtk_entry_get_text (entry_barcode));
  gchar *description = g_strdup (gtk_entry_get_text (entry_desc));
  gchar *marca = g_strdup (gtk_entry_get_text (entry_brand));
  gchar *contenido = g_strdup (gtk_entry_get_text (entry_cont));
  gchar *precio = g_strdup (gtk_entry_get_text (entry_sell_price));
  gchar *cantidad = g_strdup (gtk_entry_get_text (entry_cantity));
  gchar *unidad;
  gchar *tipo;
  gint otros, familia;

  if (strcmp (codigo, "") == 0)
    ErrorMSG (GTK_WIDGET (entry_code), "Debe ingresar un codigo corto");
  else if (strlen (codigo) > 6)
    ErrorMSG (GTK_WIDGET (entry_code), "Código corto debe ser menor a 7 caracteres");
  else if (strcmp (barcode, "") == 0)
    ErrorMSG (GTK_WIDGET (entry_barcode), "Debe Ingresar un Codigo de Barras");
  else if (HaveCharacters (barcode))
    ErrorMSG (GTK_WIDGET (entry_barcode), "Código de barras debe ser un valor numérico");
  else if (strlen (barcode) > 18)
    ErrorMSG (GTK_WIDGET (entry_barcode), "Código de barras debe ser menor a 18 caracteres");
  else if (strlen (barcode) < 6)
    ErrorMSG (GTK_WIDGET (entry_barcode), "Código de barras debe tener 6 dígitos como mínimo");
  else if (strcmp (description, "") == 0)
    ErrorMSG (GTK_WIDGET (entry_desc), "Debe ingresar una Descripción");
  else if (strcmp (marca, "") == 0)
    ErrorMSG (GTK_WIDGET (entry_brand), "Debe ingresar al Marca de la mercadería derivada");
  else if (strcmp (contenido, "") == 0)
    ErrorMSG (GTK_WIDGET (entry_cont), "Debe ingresar el Contenido la mercadería derivada");
  else if (strcmp (precio, "") == 0)
    ErrorMSG (GTK_WIDGET (entry_sell_price), "Debe ingresar el precio de venta de la mercadería derivada");
  else if (HaveCharacters (precio))
    ErrorMSG (GTK_WIDGET (entry_sell_price), "Precio venta debe ser un valor numérico");
  else if (strcmp (cantidad, "") == 0)
    ErrorMSG (GTK_WIDGET (entry_cantity), "Debe ingresar la cantidad a comprar");
  else if (!is_numeric (cantidad))
    ErrorMSG (GTK_WIDGET (entry_cantity), "Cantidad debe ser un valor numérico");
  else if (!is_numeric (contenido))
    ErrorMSG (GTK_WIDGET (entry_cont), "Contenido debe ser un valor numérico");
  else
    {
      if (DataExist (g_strdup_printf ("SELECT codigo_corto FROM producto WHERE codigo_corto like '%s'", codigo)))
        {
          ErrorMSG (GTK_WIDGET (entry_code), "Ya existe un producto con el mismo codigo corto");
          return;
        }
      else if (DataExist (g_strdup_printf ("SELECT codigo_corto FROM producto WHERE barcode = %s", codigo)))
        {
          ErrorMSG (GTK_WIDGET (entry_code), "Ya existe un producto con este codigo corto de barcode");
          return;
        }

      if (DataExist (g_strdup_printf ("SELECT barcode FROM producto WHERE barcode = %s", barcode)))
        {
          ErrorMSG (GTK_WIDGET (entry_barcode), "Ya existe un producto con el mismo codigo de barras");
          return;
        }
      else if (DataExist (g_strdup_printf ("SELECT barcode FROM producto WHERE codigo_corto = '%s'", barcode)))
        {
          ErrorMSG (GTK_WIDGET (entry_barcode), "Ya existe un producto con este barcode de codigo corto");
          return;
        }

      if (gtk_combo_box_get_active (combo) == 0)
        otros = 0;
      else
        {
          model = gtk_combo_box_get_model (combo);
          gtk_combo_box_get_active_iter (combo, &iter);
          gtk_tree_model_get (model, &iter,
                              0, &otros,
                              -1);
        }
      model = gtk_combo_box_get_model (combo_unit);
      gtk_combo_box_get_active_iter (combo_unit, &iter);

      gtk_tree_model_get (model, &iter,
			  1, &unidad,
			  -1);

      /*familia*/
      model = gtk_combo_box_get_model (combo_family);
      gtk_combo_box_get_active_iter (combo_family, &iter);

      gtk_tree_model_get (model, &iter,
			  0, &familia,
			  -1);

      //Tipo producto (Derivada)
      tipo = PQvaluebycol (EjecutarSQL ("SELECT id FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'DERIVADA'"), 0, "id");
      
      //crear producto
      if (AddNewProductToDB (codigo, barcode, description, marca, CUT (contenido),
			     unidad, iva, otros, familia, FALSE, fraccion, atoi (tipo)))
	EjecutarSQL (g_strdup_printf ("UPDATE producto set precio = %s WHERE barcode = %s", CUT (precio), barcode));

      gtk_widget_hide (GTK_WIDGET (builder_get (builder, "wnd_new_mcd")));

      //Agrega la mercadería derivada al treeview "deriv"
      store = GTK_LIST_STORE (gtk_tree_view_get_model
			      (GTK_TREE_VIEW (gtk_builder_get_object (builder, "treeview_deriv"))));
      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
			  0, barcode,
			  1, codigo,
			  2, g_strdup_printf ("%s %s", description, marca),
			  3, atoi (precio),
			  4, strtod (PUT (cantidad), (char **)NULL),
			  -1);

      //SearchProductHistory (GTK_ENTRY (gtk_builder_get_object (builder, "entry_buy_barcode")), codigo);
    }
  return;
}


/**
 * Callback to edit cantity on 'treeview_components'
 * (editable-event)
 *
 * @param cell
 * @param path_string
 * @param new_cantity
 * @param data -> A GtkListStore
 */
void
on_cantidad_compuesta_renderer_edited (GtkCellRendererText *cell, gchar *path_string, gchar *new_cantity, gpointer data)
{
  // Treeview detalle de la compra
  GtkTreeModel *model;
  GtkTreePath *path;
  GtkTreeIter iter;

  gdouble cantidad;
  gdouble costo;
  gchar *barcode;
  gchar *fraccion;
  gchar *descripcion;

  /*Obtiene el modelo del treeview compra detalle*/
  model = GTK_TREE_MODEL (data);
  path = gtk_tree_path_new_from_string (path_string);

  /* obtiene el iter para poder obtener y setear datos del treeview */
  gtk_tree_model_get_iter (model, &iter, path);

  if (!is_numeric (new_cantity))
    {
      ErrorMSG (GTK_WIDGET (builder_get (builder, "treeview_components")),
		"Cantidad debe ser un valor numérico");
      return;
    }
  
  cantidad = strtod (PUT (new_cantity), (char **)NULL);
  
  if (cantidad <= 0)
    {
      ErrorMSG (GTK_WIDGET (builder_get (builder, "treeview_components")),
		"Cantidad debe ser mayor a cero");
      return;
    }

  // Se obtiene el costo de producto
  gtk_tree_model_get (model, &iter,
		      0, &barcode,
		      2, &descripcion,
		      3, &costo,
		      -1);

  // Se asegura que no se ingresen valores decimales para productos no fraccionarios
  fraccion = PQvaluebycol (EjecutarSQL (g_strdup_printf ("SELECT fraccion FROM producto WHERE barcode = %s", barcode)), 0, "fraccion");
  if (!g_str_equal (fraccion, "t")) 
    {
      cantidad = (cantidad < 1) ? 1 : lround (cantidad);
      statusbar_push (GTK_STATUSBAR (builder_get (builder, "statusbar_asoc_comp")), 
		      g_strdup_printf ("El producto %s no es fraccional, la cantidad ha sido redondeada", descripcion), 
		      5000);
    }

  // Se setean los datos modificados en el treeview
  gtk_list_store_set (GTK_LIST_STORE (model), &iter,
		      4, cantidad,
		      5, costo*cantidad,
		      -1);
  
  // Se actualiza el costo subtotal en el label
  gtk_label_set_text (GTK_LABEL (builder_get (builder, "lbl_subtotal_costo_compuesto")),
		      g_strdup_printf ("%.2f", 
				       sum_treeview_column (GTK_TREE_VIEW (builder_get (builder, "treeview_components")), 
							    5, G_TYPE_DOUBLE) ));

  return;
}

/**
 * Callback from "btn_cancel_asoc_comp" Button
 * (signal click) and "wnd_asoc_comp" (signal delete-event).
 *
 * Hide the "wnd_asoc_comp" windows and set the add_to_comp FLAG to FALSE.
 */
void
close_wnd_asoc_comp (void)
{
  add_to_comp = FALSE;
  gtk_widget_hide (GTK_WIDGET (builder_get (builder, "wnd_asoc_comp")));
}


/**
 * Show and inicialize the 'wnd_asoc_comp' window
 */
void
show_wnd_asoc_comp (void)
{
  GtkTreeView *treeview;
  GtkListStore *store;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkTreeIter iter;
  PGresult *res;
  gchar *barcode;
  gint tuples, i;
  gdouble costo_promedio, costo_total, cantidad;

  add_to_comp = TRUE;
  treeview = GTK_TREE_VIEW (gtk_builder_get_object (builder, "treeview_components"));

  if (gtk_tree_view_get_model (treeview) == NULL)
    {
      store = gtk_list_store_new (9,
				  G_TYPE_STRING,  //Barcode
				  G_TYPE_STRING,  //Codigo Corto
				  G_TYPE_STRING,  //Marca Descripción
				  G_TYPE_DOUBLE,  //Costo
				  G_TYPE_DOUBLE,  //Cantidad
				  G_TYPE_DOUBLE,  //Sub_total costo
				  G_TYPE_INT,     //TIPO
				  G_TYPE_DOUBLE,  //Precio TODO: registrar precio (pero que no se muestre) asdasdasd
				  G_TYPE_DOUBLE );//Sub_total precio

      gtk_tree_view_set_model (GTK_TREE_VIEW (treeview), GTK_TREE_MODEL (store));

      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes ("Codigo", renderer,
							 "text", 1,
							 NULL);
      gtk_tree_view_append_column (treeview, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
      gtk_tree_view_column_set_resizable (column, FALSE);

      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes ("Descripcion", renderer,
							 "text", 2,
							 NULL);
      gtk_tree_view_append_column (treeview, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
      gtk_tree_view_column_set_resizable (column, FALSE);

      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes ("Costo Unit.", renderer,
							 "text", 3,
							 NULL);
      gtk_tree_view_append_column (treeview, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
      gtk_tree_view_column_set_resizable (column, FALSE);
      gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)3, NULL);


      //Editable
      renderer = gtk_cell_renderer_text_new ();
      g_object_set (renderer,"editable", TRUE, NULL);
      g_signal_connect (G_OBJECT (renderer), "edited",
			G_CALLBACK (on_cantidad_compuesta_renderer_edited), (gpointer)store);

      column = gtk_tree_view_column_new_with_attributes ("cantidad", renderer,
							 "text", 4,
							 NULL);
      gtk_tree_view_append_column (treeview, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
      gtk_tree_view_column_set_resizable (column, FALSE);
      gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)4, NULL);

      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes ("SubTotal", renderer,
							 "text", 5,
							 NULL);
      gtk_tree_view_append_column (treeview, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
      gtk_tree_view_column_set_resizable (column, FALSE);
      gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)5, NULL);
    }
  
  //clean_container (GTK_CONTAINER (builder_get (builder, "wnd_asoc_comp")));

  store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (gtk_builder_get_object (builder, "treeview_components"))));
  gtk_list_store_clear (store);

  barcode = g_strdup (gtk_label_get_text (GTK_LABEL (builder_get (builder, "lbl_comp_barcode"))));
  res = get_componentes_compuesto (barcode);
  if (res != NULL)
    {
      tuples = PQntuples (res);
      for (i = 0; i < tuples; i++)
	{
	  costo_promedio = strtod (PUT (g_strdup (PQvaluebycol (res, i, "costo_promedio"))), (char **)NULL);
	  cantidad = strtod (PUT (g_strdup (PQvaluebycol (res, i, "cantidad_mud"))), (char **)NULL);
	  costo_total += costo_promedio*cantidad;

	  gtk_list_store_append (store, &iter);
	  gtk_list_store_set (store, &iter, 
			      0, g_strdup (PQvaluebycol (res, i, "barcode")),
			      1, g_strdup (PQvaluebycol (res, i, "codigo")),
			      2, g_strdup_printf ("%s %s",
						  PQvaluebycol (res, i, "descripcion"),
						  PQvaluebycol (res, i, "marca")),
			      3, costo_promedio,
			      4, cantidad,
			      5, costo_promedio * cantidad,
			      6, atoi (PQvaluebycol (res, i, "tipo")),
			      -1);
	}
    }

  // Se muestra el costo total del compuesto en el label correspondiente
  gtk_label_set_text (GTK_LABEL (builder_get (builder, "lbl_subtotal_costo_compuesto")),
		      g_strdup_printf ("%.2f", costo_total));
  // Se obtiene precio del compuesto y se muestra en el label correspondiente
  gtk_entry_set_text (GTK_ENTRY (builder_get (builder, "entry_int_sell_price_comp")), 
		      GetDataByOne (g_strdup_printf ("SELECT precio FROM producto WHERE barcode = %s", barcode)));
  gtk_widget_show (GTK_WIDGET (builder_get (builder, "wnd_asoc_comp")));

  on_button_clear_clicked (NULL, NULL);
}


/**
 * Esta funcion es llamada cuando el boton "btn_add_new_comp" es
 * presionado (signal click).
 *
 * Esta funcion obtiene todos los datos de la mercadería compuesta 
 * o "bandeo de mercadería" que se llenaron en la ventana "wnd_new_compuesta", 
 * para ingresar la mercadería correspondiente a la base de datos
 */
void
on_btn_add_new_comp_clicked (GtkButton *button, gpointer data)
{
  GtkTreeModel *model;
  GtkTreeIter iter;

  GtkEntry *entry_code = GTK_ENTRY (builder_get (builder, "entry_new_comp_code"));
  GtkEntry *entry_barcode = GTK_ENTRY (builder_get (builder, "entry_new_comp_barcode"));
  GtkEntry *entry_desc = GTK_ENTRY (builder_get (builder, "entry_new_comp_desc"));
  GtkEntry *entry_brand = GTK_ENTRY (builder_get (builder, "entry_new_comp_brand"));
  GtkEntry *entry_cont = GTK_ENTRY (builder_get (builder, "entry_new_comp_cont"));

  GtkComboBox *combo_family = GTK_COMBO_BOX (builder_get (builder, "cmb_new_comp_family"));
  GtkComboBox *combo_unit = GTK_COMBO_BOX (builder_get (builder, "cmb_new_comp_unit"));

  /*gboolean fraccion = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (builder_get (builder, 
    "radio_btn_comp_fractional_yes")));*/
  
  gchar *codigo = g_strdup (gtk_entry_get_text (entry_code));
  gchar *barcode = g_strdup (gtk_entry_get_text (entry_barcode));
  gchar *description = g_strdup (gtk_entry_get_text (entry_desc));
  gchar *marca = g_strdup (gtk_entry_get_text (entry_brand));
  gchar *contenido = g_strdup (gtk_entry_get_text (entry_cont));
  gchar *unidad;
  gint familia;

  gchar *tipo;

  if (g_str_equal (codigo, ""))
    ErrorMSG (GTK_WIDGET (entry_code), "Debe ingresar un codigo corto");
  else if (strlen (codigo) > 6)
    ErrorMSG (GTK_WIDGET (entry_code), "Código corto debe ser menor a 7 caracteres");
  else if (g_str_equal (barcode, ""))
    ErrorMSG (GTK_WIDGET (entry_barcode), "Debe Ingresar un Codigo de Barras");
  else if (HaveCharacters (barcode))
    ErrorMSG (GTK_WIDGET (entry_barcode), "Código de barras debe ser un valor numérico");
  else if (strlen (barcode) > 18)
    ErrorMSG (GTK_WIDGET (entry_barcode), "Código de barras debe ser menor a 18 caracteres");
  else if (strlen (barcode) < 6)
    ErrorMSG (GTK_WIDGET (entry_barcode), "Código de barras debe tener 6 dígitos como mínimo");
  else if (g_str_equal (description, ""))
    ErrorMSG (GTK_WIDGET (entry_desc), "Debe ingresar una Descripción");
  else if (strcmp (marca, "") == 0)
    ErrorMSG (GTK_WIDGET (entry_brand), "Debe ingresar la Marca de la mercadería compuesta");
  else if (strcmp (contenido, "") == 0)
    ErrorMSG (GTK_WIDGET (entry_cont), "Debe ingresar el Contenido la mercadería compuesta");
  else if (!is_numeric (contenido))
    ErrorMSG (GTK_WIDGET (entry_cont), "Contenido debe ser un valor numérico");
  else
    {
      if (DataExist (g_strdup_printf ("SELECT codigo_corto FROM producto WHERE codigo_corto like '%s'", codigo)))
        {
          ErrorMSG (GTK_WIDGET (entry_code), "Ya existe un producto con el mismo codigo corto");
          return;
        }
      else if (DataExist (g_strdup_printf ("SELECT codigo_corto FROM producto WHERE barcode = %s", codigo)))
        {
          ErrorMSG (GTK_WIDGET (entry_code), "Ya existe un producto con este codigo corto de barcode");
          return;
        }

      if (DataExist (g_strdup_printf ("SELECT barcode FROM producto WHERE barcode = %s", barcode)))
        {
          ErrorMSG (GTK_WIDGET (entry_barcode), "Ya existe un producto con el mismo codigo de barras");
          return;
        }
      else if (DataExist (g_strdup_printf ("SELECT barcode FROM producto WHERE codigo_corto = '%s'", barcode)))
        {
          ErrorMSG (GTK_WIDGET (entry_barcode), "Ya existe un producto con este barcode de codigo corto");
          return;
        }

      /*familia*/
      model = gtk_combo_box_get_model (combo_family);
      gtk_combo_box_get_active_iter (combo_family, &iter);

      gtk_tree_model_get (model, &iter,
			  0, &familia,
			  -1);

      /*Unidad*/
      model = gtk_combo_box_get_model (combo_unit);
      gtk_combo_box_get_active_iter (combo_unit, &iter);

      gtk_tree_model_get (model, &iter,
			  1, &unidad,
			  -1);

      //Tipo producto (COMPUESTA)
      tipo = PQvaluebycol (EjecutarSQL ("SELECT id FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'COMPUESTA'"), 0, "id");
      
      //Crear producto - Codigo, barcode, descripcion, marca, contenido, unidad, impuestos, otros, familia, perecible, fraccion, tipo
      if (!AddNewProductToDB (codigo, barcode, description, marca, CUT (contenido), unidad, TRUE, 0, familia, FALSE, FALSE, atoi (tipo)))
	return;

      gtk_widget_hide (GTK_WIDGET (builder_get (builder, "wnd_new_compuesta")));    

      gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_comp_desc")), 
			    g_strdup_printf ("<span font_weight='bold' size='12500'>%s</span>", description));
      gtk_label_set_label (GTK_LABEL (builder_get (builder, "lbl_comp_barcode")), barcode);
      gtk_label_set_label (GTK_LABEL (builder_get (builder, "lbl_comp_code")), codigo);

      show_wnd_asoc_comp ();

      //SearchProductHistory (GTK_ENTRY (gtk_builder_get_object (builder, "entry_buy_barcode")), codigo);
    }

  gtk_entry_set_text (GTK_ENTRY (builder_get (builder, "entry_buy_barcode")), "");
  return;
}


/**
 *
 *
 */
void
on_btn_edit_components_clicked (GtkButton *button, gpointer data)
{
  gchar *codigo;
  gchar *barcode;
  gchar *description;

  codigo = g_strdup (gtk_label_get_text (GTK_LABEL (builder_get (builder, "label_code"))));
  barcode = g_strdup (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_buy_barcode"))));
  description = g_strdup (gtk_label_get_text (GTK_LABEL (builder_get (builder, "label_product_desc"))));

  gtk_widget_hide (GTK_WIDGET (builder_get (builder, "wnd_edit_compuesta")));
  gtk_label_set_markup (GTK_LABEL (builder_get (builder, "lbl_comp_desc")), 
			g_strdup_printf ("<span font_weight='bold' size='12500'>%s</span>", description));
  gtk_label_set_label (GTK_LABEL (builder_get (builder, "lbl_comp_barcode")), barcode);
  gtk_label_set_label (GTK_LABEL (builder_get (builder, "lbl_comp_code")), codigo);
  
  show_wnd_asoc_comp ();
}



/**
 *
 *
 * @param button the button
 * @param user_data the user data
 */
void
on_btn_assoc_comp_deriv_clicked (GtkButton *button, gpointer user_data)
{
  GtkTreeModel *model;
  //GtkListStore *store;
  GtkTreeIter iter;
  gboolean valid;

  gint num_deriv = get_treeview_length (GTK_TREE_VIEW (builder_get (builder, "treeview_deriv")));
  gchar *barcode_deriv[num_deriv];
  gchar *barcode_mp;
  gint tipo_deriv;
  gint tipo_mp;
  gint i;
  gdouble cant_mud[num_deriv];

  /*Obtengo el idde los tipos derivados y materia prima respectivamente*/
  tipo_deriv = atoi (g_strdup
		     (PQvaluebycol 
		      (EjecutarSQL ("SELECT id FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'DERIVADA'"), 0, "id")));

  tipo_mp = atoi (g_strdup
		  (PQvaluebycol 
		   (EjecutarSQL ("SELECT id FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'MATERIA PRIMA'"), 0, "id")));

  
  /*Barcode Materia Prima*/
  barcode_mp = g_strdup (gtk_label_get_text (GTK_LABEL (builder_get (builder, "lbl_barcode_mp"))));

  /*Obteniendo datos de los treeview de las mercaderías DERIVADAS*/
  model = gtk_tree_view_get_model (GTK_TREE_VIEW (builder_get (builder, "treeview_deriv")));
  //store = GTK_LIST_STORE (model);
  valid = gtk_tree_model_get_iter_first (model, &iter); //Obtiene la primera fila del treeview

  i = 0;
  while (valid)
    {
      gtk_tree_model_get (model, &iter,
			  0, &barcode_deriv[i],
			  4, &cant_mud[i],
			  -1);

      //TODO: el treeview debe tener el barcode, para no realizar una búsqueda en la bd por cada iteración
      i++;
      valid = gtk_tree_model_iter_next (model, &iter); /* Me da TRUE si itera a la siguiente */
    }
  
  //Se ingresan de acuerdo a la asociación de mercaderías
  for (i = 0; i<num_deriv; i++) //Se asocia o modifica la relación de acuerdo a lo realizado
    asociar_componente_o_derivado (barcode_deriv[i], tipo_deriv,
				   barcode_mp, tipo_mp,
				   cant_mud[i]);
  
  //Se habilita el botón de compra (cuando se crea la materia prima)
  //gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "button_buy")), TRUE);
  gtk_widget_grab_focus (GTK_WIDGET (gtk_builder_get_object (builder, "entry_buy_barcode")));
  clean_container (GTK_CONTAINER (builder_get (builder, "wnd_comp_deriv")));
  gtk_widget_hide (GTK_WIDGET (builder_get (builder, "wnd_comp_deriv")));
}


/**
 *
 *
 * @param button the button
 * @param user_data the user data
 */
void
on_btn_asoc_comp_clicked (GtkButton *button, gpointer user_data)
{
  GtkTreeModel *model;
  //GtkListStore *store;
  GtkTreeIter iter;
  gboolean valid;

  gint num_componentes = get_treeview_length (GTK_TREE_VIEW (builder_get (builder, "treeview_components")));
  gchar *barcode_componentes [num_componentes];
  gchar *barcode_compuesto;
  gint tipo_componentes [num_componentes];
  gint tipo_compuesto;
  gint i;
  gint sell_price;
  gdouble buy_price;
  gdouble cant_mud[num_componentes];


  //Restricciones iniciales
  if (num_componentes <= 0)
    {
      ErrorMSG (GTK_WIDGET (gtk_builder_get_object (builder, "btn_add_comp")), 
		"Debe haber 1 o más componentes para asociar al compuesto");
      return;
    }

  sell_price = atoi (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_int_sell_price_comp"))));
  buy_price = sum_treeview_column (GTK_TREE_VIEW (builder_get (builder, "treeview_components")), 5, G_TYPE_DOUBLE);

  if (sell_price <= buy_price)
    {
      ErrorMSG (GTK_WIDGET (gtk_builder_get_object (builder, "entry_int_sell_price_comp")), 
		"Debe ingresar un precio de venta mayor al costo total para el compuesto u oferta");
      return;
    }

  tipo_compuesto = atoi (g_strdup (PQvaluebycol 
				   (EjecutarSQL ("SELECT id FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'COMPUESTA'"), 0, "id")));

  /*Barcode Mercadería compuesta*/
  barcode_compuesto = g_strdup (gtk_label_get_text (GTK_LABEL (builder_get (builder, "lbl_comp_barcode"))));

  /*Se actualiza su precio*/
  if (!EjecutarSQL (g_strdup_printf ("UPDATE producto SET precio = %d WHERE barcode = %s", sell_price, barcode_compuesto)))
    printf ("Hubo un error al tratar de actualizar el precio de %s \n", barcode_compuesto);

  /*Obteniendo datos de los treeview de los componentes*/
  model = gtk_tree_view_get_model (GTK_TREE_VIEW (builder_get (builder, "treeview_components")));
  //store = GTK_LIST_STORE (model);
  valid = gtk_tree_model_get_iter_first (model, &iter); //Obtiene la primera fila del treeview

  i = 0;
  while (valid)
    {
      gtk_tree_model_get (model, &iter,
			  0, &barcode_componentes[i],
			  4, &cant_mud[i],
			  6, &tipo_componentes[i],
			  -1);
      i++;
      valid = gtk_tree_model_iter_next (model, &iter); /* Me da TRUE si itera a la siguiente */
    }
  
  //Se ingresan de acuerdo a la asociación de mercadería
  for (i = 0; i<num_componentes; i++) //Se asocia o modifica la relación de acuerdo a lo realizado
    asociar_componente_o_derivado (barcode_compuesto, tipo_compuesto,
				   barcode_componentes[i], tipo_componentes[i],
				   cant_mud[i]);

  // Se setea la bandera de asociación de compuestos a FALSE
  add_to_comp = FALSE;
  //Se coloca el foco en el widget correspondiente y se cierra la ventana de asociación
  gtk_widget_grab_focus (GTK_WIDGET (gtk_builder_get_object (builder, "entry_buy_barcode")));
  gtk_widget_hide (GTK_WIDGET (builder_get (builder, "wnd_asoc_comp")));  
}


void
on_btn_remove_deriv_clicked (GtkButton *button, gpointer user_data)
{
  GtkTreeView *treeview;
  GtkListStore *store;
  GtkTreeSelection *selection;
  GtkTreeIter iter;
  gint position;

  gchar *barcode_derivado, *barcode_mp;
  
  treeview = GTK_TREE_VIEW (builder_get (builder, "treeview_deriv"));
  selection = gtk_tree_view_get_selection (treeview);
  store = GTK_LIST_STORE (gtk_tree_view_get_model (treeview));

  if (gtk_tree_selection_get_selected (selection, NULL, &iter))
    {
      gtk_tree_model_get (GTK_TREE_MODEL (store), &iter,
                          0, &barcode_derivado,
                          -1);
      
      barcode_mp = g_strdup (gtk_label_get_text (GTK_LABEL (builder_get (builder, "lbl_barcode_mp"))));

      /*Se desasocian las meraderías*/
      desasociar_componente_compuesto (barcode_derivado, barcode_mp);

      position = atoi (gtk_tree_model_get_string_from_iter(GTK_TREE_MODEL(store), &iter));
      gtk_list_store_remove (store, &iter);
      select_back_deleted_row ("treeview_deriv", position);
    }
}

/**
 *
 *
 */
void
on_btn_remove_comp_clicked (void)
{
  GtkTreeView *treeview;
  GtkListStore *store;
  GtkTreeModel *model;
  GtkTreeSelection *selection;  
  GtkTreeIter iter;
  gint position;
  gdouble costo_total;
  gchar *barcode_hijo;
  gchar *barcode_madre;

  treeview = GTK_TREE_VIEW (builder_get (builder, "treeview_components"));
  selection = gtk_tree_view_get_selection (treeview);
  store = GTK_LIST_STORE (gtk_tree_view_get_model (treeview));
  model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));

  if (gtk_tree_selection_get_selected (selection, NULL, &iter))
    {
      gtk_tree_model_get (model, &iter,
			  0, &barcode_hijo,
			  -1);

      barcode_madre = g_strdup (gtk_label_get_text (GTK_LABEL (builder_get (builder, "lbl_comp_barcode"))));

      /*Se desasocian las meraderías*/
      desasociar_componente_compuesto (barcode_madre, barcode_hijo);

      position = atoi (gtk_tree_model_get_string_from_iter(GTK_TREE_MODEL(store), &iter));
      gtk_list_store_remove (store, &iter);
      select_back_deleted_row ("treeview_components", position);

      //Actualizo el costo total
      costo_total = sum_treeview_column (treeview, 5, G_TYPE_DOUBLE);
      gtk_label_set_text (GTK_LABEL (builder_get (builder, "lbl_subtotal_costo_compuesto")),
			  g_strdup_printf ("%.2f", costo_total));
    }

  if (get_treeview_length (treeview) == 0)
    gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_asoc_comp")), FALSE);    
}

/**
 * Es llamada cuando el boton "btn_get_request" es presionado (signal click).
 *
 * Esta funcion llama a a funcion "AskIngreso()".
 *
 */
void
on_btn_get_request_clicked (void)
{
  AskIngreso();
}


/**
 * Es llamada cuando el boton "btn_remove_buy_product" es presionado (signal click).
 *
 * Esta funcion elimina el producto seleccionado del treeView y ademas llama
 * a la funcion "DropBuyProduct()".
 *
 */
void
on_btn_remove_buy_product_clicked (void)
{
  GtkTreeView *treeview = GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_products_buy_list"));
  GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (builder_get (builder, "tree_view_products_buy_list")));
  GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (builder_get (builder, "tree_view_products_buy_list"))));
  GtkTreeIter iter;
  gchar *short_code, *barcode;
  gint position;

  if (get_treeview_length(treeview) == 0)
    return;

  if (gtk_tree_selection_get_selected (selection, NULL, &iter))
    {
      gtk_tree_model_get (GTK_TREE_MODEL (store), &iter,
			  0, &barcode,
                          1, &short_code,
                          -1);

      DropBuyProduct (short_code);
      del_to_pedido_temporal (barcode);
      position = atoi (gtk_tree_model_get_string_from_iter(GTK_TREE_MODEL(store), &iter));
      gtk_list_store_remove (store, &iter);

      gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "label_total_buy")),
                            g_strdup_printf ("<span size=\"xx-large\"><b>%s</b></span>",
                                             PutPoints (g_strdup_printf ("%li", lround (CalcularTotalCompra (compra->header_compra))))));
    }

  select_back_deleted_row("tree_view_products_buy_list", position);

  if (get_treeview_length(treeview) == 0)
    {
      //Se libera la variable global
      if (rut_proveedor_global != NULL)
	{
	  g_free (rut_proveedor_global);
	  rut_proveedor_global = NULL;
	}

      on_button_clear_clicked (NULL, NULL);
    }
  else
    {
      gboolean modo_traspaso;
      modo_traspaso = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (builder_get (builder, "rdbutton_transfer_mode")));
      //Si todos los productos son enviables y traspaso enviar esta deshabilitado, éste se habilita
      if (modo_traspaso == TRUE && 
	  !compare_treeview_column (treeview, 7, G_TYPE_BOOLEAN, (void *)FALSE))
	gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_traspaso_enviar")), TRUE);
    }
}


/**
 * Es llamada por la funcion "realizar_traspaso_Env()".
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
  gdouble total = CalcularTotalCompra (compra->header_compra);
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_monto_total")),
                      PUT(g_strdup_printf ("%.2f",total)));
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_origen")),(gchar *)ReturnNegocio());
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "labelID")),g_strdup_printf ("%d",InsertIdTraspaso()+1));
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_vendedor")),"admin");


  res = EjecutarSQL (g_strdup_printf ("SELECT id,nombre FROM bodega "
                                      "WHERE nombre!=(SELECT nombre FROM negocio) AND estado = true"));
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

  gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 0);
}


/**
 * Es llamada por la funcion "realizar_traspaso_Rec()".
 *
 * Esta funcion carga las etiquetas (labels) de la ventana
 * "traspaso_recibir_win", ademas del combobox de origen, con los respectivos valores
 *
 */
void
DatosRecibir (void)
{
  GtkWidget *combo;
  GtkTreeIter iter;
  GtkListStore *modelo;
  gint tuples,i;
  PGresult *res;
  //gint venta_id;

  gtk_widget_grab_focus (GTK_WIDGET (gtk_builder_get_object (builder, "comboboxOrigen")));
  gdouble total = CalcularTotalCompra (compra->header_compra);
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_monto_total1")),
                      PUT(g_strdup_printf ("%.2f",total)));
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_destino")),(gchar *)ReturnNegocio());
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "labelID1")),g_strdup_printf ("%d",InsertIdTraspaso()+1));
  gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_vendedor1")),"admin");

  res = EjecutarSQL (g_strdup_printf ("SELECT id,nombre FROM bodega "
				      "WHERE nombre!=(SELECT nombre FROM negocio) AND estado = true"));
  tuples = PQntuples (res);

  combo = GTK_WIDGET (gtk_builder_get_object(builder, "comboboxOrigen"));
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

  gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 0);
}


/**
 * Es llamada cuando el boton "btn_traspaso_enviar" es presionado (signal click).
 *
 * Esta funcion visualiza la ventana "traspaso_enviar_win" y ademas llama a
 * la funcion "DatosEnviar()".
 *
 * @param widget the widget that emits the signal
 * @param data the user data
 *
 */
void
realizar_traspaso_Env (GtkWidget *widget, gpointer data)
{
  GtkWindow *window;

  if (compra->header_compra == NULL)
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
      gtk_widget_grab_focus (GTK_WIDGET (builder_get (builder, "comboboxDestino")));
      return;
    }
}


/**
 * Es llamada cuando el boton "btn_traspaso_recibir" es presionado (signal click).
 *
 * Esta funcion visualiza la ventana "traspaso_recibir_win" y ademas llama a
 * la funcion "DatosRecibir()".
 *
 * @param widget the widget that emits the signal
 * @param data the user data
 *
 */
void
realizar_traspaso_Rec (GtkWidget *widget, gpointer data)
{
  GtkWindow *window;

  if (compra->header_compra == NULL)
    {
      ErrorMSG (GTK_WIDGET (gtk_builder_get_object (builder, "barcode_entry")), "No hay productos para traspasar");
      return;
    }

  else
    {
      window = GTK_WINDOW (gtk_builder_get_object (builder, "traspaso_recibir_win"));
      clean_container (GTK_CONTAINER (window));
      gtk_widget_show_all (GTK_WIDGET (window));
      DatosRecibir();
      gtk_widget_grab_focus (GTK_WIDGET (builder_get (builder, "comboboxOrigen")));
      return;
    }
}


/**
 * Es llamada cuando el boton "traspaso_button" es presionado (signal click).
 *
 * Esta funcion visualiza la ventana "traspaso_enviar_win" y ademas vuelve a
 * visualizar la opcion "comprar"
 *
 * @param widget the widget that emits the signal
 * @param data the user data
 *
 */
void
on_enviar_button_clicked (GtkButton *button, gpointer data)
{
  gint destino;
  gchar *nombre_origen, *nombre_destino;
  GtkTreeIter iter;
  GtkWidget *combo;
  GtkTreeModel *model;
  gint active;
  gint vendedor = 1; //user_data->user_id;
  gdouble monto = CalcularTotalCompra (compra->header_compra);
  gint id_traspaso;

  combo = GTK_WIDGET (gtk_builder_get_object(builder, "comboboxDestino"));
  active = gtk_combo_box_get_active (GTK_COMBO_BOX (combo));

  nombre_origen = g_strdup (gtk_label_get_text (GTK_LABEL (builder_get (builder, "label_origen"))));

  if (active == -1)
    {
      ErrorMSG (combo, "Debe Seleccionar un Destino");
    }
  else
    {
      model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));
      gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combo), &iter);

      gtk_tree_model_get (model, &iter,
			  0, &destino,
			  1, &nombre_destino,
			  -1);

      id_traspaso = SaveTraspasoCompras (monto,
					 ReturnBodegaID(ReturnNegocio()),
					 vendedor,
					 destino,
					 TRUE);

      PrintValeTraspaso (compra->header_compra, id_traspaso, TRUE, nombre_origen, nombre_destino);

      gtk_widget_hide (GTK_WIDGET (builder_get (builder, "traspaso_enviar_win")));

      gtk_widget_grab_focus (GTK_WIDGET (gtk_builder_get_object (builder, "entry_buy_barcode")));

      ClearAllCompraData ();

      compra->header_compra = NULL;
      compra->products_compra = NULL;

      gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_total_buy")), "");

      gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "rdbutton_buy_mode")), TRUE);
      gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_traspaso_recibir")), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_traspaso_enviar")), FALSE);
    } // else => if (active == -1)
} // void on_enviar_button_clicked (GtkButton *button, gpointer data)


/**
 * Es llamada cuando el boton "traspaso_button1" es presionado (signal click).
 *
 * Esta funcion visualiza la ventana "traspaso_recibir_win"  y ademas vuelve a
 * visualizar la opcion "comprar"
 *
 * @param widget the widget that emits the signal
 * @param data the user data
 *
 */
void
on_recibir_button_clicked (GtkButton *button, gpointer data)
{
  gint origen;
  gchar *nombre_origen, *nombre_destino;
  GtkTreeIter iter;
  GtkWidget *combo;
  GtkTreeModel *model;
  gint active;
  gint vendedor = 1; //user_data->user_id;
  gdouble monto = CalcularTotalCompra (compra->header_compra);
  gint id_traspaso;

  combo = GTK_WIDGET (gtk_builder_get_object(builder, "comboboxOrigen"));
  active = gtk_combo_box_get_active (GTK_COMBO_BOX (combo));

  nombre_destino = g_strdup (gtk_label_get_text (GTK_LABEL (builder_get (builder, "label_destino"))));

  if (active == -1)
    {
      ErrorMSG (combo, "Debe Seleccionar un Origen");
    }
  else
    {
      model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));
      gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combo), &iter);

      gtk_tree_model_get (model, &iter,
			  0, &origen,
			  1, &nombre_origen,
			  -1);

      id_traspaso = SaveTraspasoCompras (monto,
					 origen,
					 vendedor,
					 ReturnBodegaID(ReturnNegocio()),
					 FALSE);

      PrintValeTraspaso (compra->header_compra, id_traspaso, FALSE, nombre_origen, nombre_destino);

      //gtk_widget_hide (gtk_widget_get_toplevel (GTK_WIDGET (button)));
      gtk_widget_hide (GTK_WIDGET(gtk_builder_get_object(builder, "traspaso_recibir_win")));

      gtk_widget_grab_focus (GTK_WIDGET (gtk_builder_get_object (builder, "entry_buy_barcode")));

      ClearAllCompraData ();

      compra->header_compra = NULL;
      compra->products_compra = NULL;

      gtk_label_set_text (GTK_LABEL (gtk_builder_get_object (builder, "label_total_buy")), "");

      gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "rdbutton_buy_mode")), TRUE);
      gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_traspaso_recibir")), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_traspaso_enviar")), FALSE);
    } // else => if (active == -1)
} // void on_recibir_button_clicked (GtkButton *button, gpointer data)


/**
 * Callback connected to the toggle button of the products_providers treeview
 *
 * This function enable or disable the product's selection.
 * @param toggle the togle button
 * @param path_str the path of the selected row
 * @param data the user data
 */

void
ToggleProductSelection (GtkCellRendererToggle *toggle, char *path_str, gpointer data)
{
  GtkWidget *treeview;
  GtkTreeModel *store;
  gboolean enable, valid;
  GtkTreePath *path;
  GtkTreeIter iter;
  gdouble subTotal = 0, precio = 0, sugerido = 0;

  treeview = GTK_WIDGET (gtk_builder_get_object (builder, "tree_view_products_providers"));
  store = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
  path = gtk_tree_path_new_from_string (path_str);

  gtk_tree_model_get_iter (store, &iter, path);
  gtk_tree_model_get (store, &iter,
                      0, &enable,
		      8, &sugerido,
                      -1);

  // Se cambia la seleccion del checkButton
  enable = !(enable);
  gtk_list_store_set (GTK_LIST_STORE(store), &iter,
                      0, enable,
		      8, (sugerido == 0) ? 1 : sugerido,
                      -1);

  // Se calcula el subtotal de todas las filas que fueron seleccionadas
  valid = gtk_tree_model_get_iter_first (store, &iter);
  while (valid)
    {
      gtk_tree_model_get (store, &iter,
			  0, &enable,
			  3, &precio,
			  8, &sugerido,
			  -1);
      if (enable == TRUE)
	subTotal = subTotal + (precio * ((sugerido > 0) ? sugerido : 1));

      // Itero a la siguiente fila --
      valid = gtk_tree_model_iter_next (store, &iter); /* Me da TRUE si itera a la siguiente */
    }

  gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "lbl_sum_selected")),
			g_strdup_printf ("<b>%ld</b>", lround (subTotal)));

  if (subTotal > 0)
    {
      gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_buy_selected")), TRUE);
      gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_print_suggest_buy")), TRUE);
    }
  else
    {
      gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_buy_selected")), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_print_suggest_buy")), FALSE);
    }

  gtk_tree_path_free (path);
}


/**
 * Es llamada cuando se edita "Costo" en el tree_view_products_providers (signal edited).
 *
 * Esta funcion actualiza el campo "Costo" por el valor indicado por el usuario.
 *
 */
void
on_buy_price_edited (GtkCellRendererText *cell, gchar *path_string, gchar *costoT, gpointer data)
{
  GtkTreeModel *model = GTK_TREE_MODEL (data);
  GtkTreePath *path = gtk_tree_path_new_from_string (path_string);
  GtkTreeIter iter;
  gdouble costo = strtod (PUT (costoT), (char **)NULL);
  gdouble costoAnterior;
  gdouble subTotal, precio, cantidad;
  gboolean enable;
  gchar *barcode;

  // Se inicializan en 0
  subTotal = precio = cantidad = 0;

  gtk_tree_model_get_iter (model, &iter, path);
  gtk_tree_path_free (path);

  gtk_tree_model_get (model, &iter,
		      0, &enable,
		      1, &barcode, // Se obtiene el codigo_corto desde el treeview
		      3, &costoAnterior, // Costo antes de la modificacion
		      4, &precio,
		      8, &cantidad,
		      -1);

  if (costo < 0) // costo no puede ser negativo
    costo = 0;

  gtk_list_store_set (GTK_LIST_STORE (model), &iter,
		      3, costo,
		      -1);

  // Recaclula subtotal
  if (enable == TRUE)
    {
      gchar *subTotalActualT = g_strdup (gtk_label_get_text (GTK_LABEL (gtk_builder_get_object (builder, "lbl_sum_selected"))));
      gdouble subTotalActual = strtod (PUT (subTotalActualT), (char **)NULL);

      // Se limpia subtotal restandole costo de la cantidad anterior
      // así se obtiene el subTotal sin contar este producto
      subTotal = subTotalActual - (costoAnterior * ((cantidad > 0) ? cantidad : 1));

      // Agrego al subtotal la cantidad al costo actual (la que se acaba de editar)
      subTotal = subTotal + (costo * ((cantidad > 0) ? cantidad : 1));

      gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "lbl_sum_selected")),
			    g_strdup_printf ("<b>%ld</b>", lround(subTotal)));
    }
}

/**
 * Es llamada cuando se edita "Precio" en el tree_view_products_providers (signal edited).
 *
 * Esta funcion actualiza el campo "precio" por el valor indicado por el usuario.
 *
 */
void
on_sell_price_edited (GtkCellRendererText *cell, gchar *path_string, gchar *precioT, gpointer data)
{
  GtkTreeModel *model = GTK_TREE_MODEL (data);
  GtkTreePath *path = gtk_tree_path_new_from_string (path_string);
  GtkTreeIter iter;
  gdouble precio = strtod (PUT (precioT), (char **)NULL);
  gdouble precioAntes;
  gdouble costo, cantidad; //subTotal
  gboolean enable;
  gchar *barcode, *materia_prima;

  // Obtiene el id de la materia prima
  materia_prima = g_strdup (PQvaluebycol (EjecutarSQL ("SELECT id FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'MATERIA PRIMA'"), 0, "id"));

  // Se inicializan en 0
  costo = cantidad = 0; // subTotal

  gtk_tree_model_get_iter (model, &iter, path);
  gtk_tree_path_free (path);

  gtk_tree_model_get (model, &iter,
		      0, &enable,
		      1, &barcode, // Se obtiene el codigo_corto desde el treeview
		      3, &costo,
		      4, &precioAntes, // precio antes de la modificacion
		      8, &cantidad,
		      -1);

  if (precio < 0) // precio no puede ser negativo
    precio = 0;

  // Se verifica si el producto es una materia prima, de serlo, no se debe modificar su precio de venta
  if (DataExist (g_strdup_printf ("SELECT barcode FROM producto WHERE codigo_corto = '%s' AND tipo = %s", 
				  barcode, materia_prima)))
    {
      AlertMSG (GTK_WIDGET (builder_get (builder, "tree_view_products_providers")), 
		"Esta es una materia prima y no tiene precio de venta. \n"
		"Edite cualquiera de sus mercaderías derivadas");
      return;
    }

  gtk_list_store_set (GTK_LIST_STORE (model), &iter,
		      4, precio,
		      -1);
}


/**
 * Es llamada cuando se edita "sugerido" en el tree_view_products_providers (signal edited).
 *
 * Esta funcion actualiza el campo "sugerido" por el valor indicado por el usuario.
 *
 * TODO: Crear una función que actualice el valor modificado de cualquier treeview
 */

void
on_suggested_amount_edited (GtkCellRendererText *cell, gchar *path_string, gchar *sugeridoT, gpointer data)
{
  GtkTreeModel *model = GTK_TREE_MODEL (data);
  GtkTreePath *path = gtk_tree_path_new_from_string (path_string);
  GtkTreeIter iter;
  gdouble sugerido = strtod (PUT (sugeridoT), (char **)NULL);
  gdouble sugeridoAntes;
  gdouble subTotal = 0, precio = 0;
  gboolean fraccion, enable;
  gchar *barcode;

  gtk_tree_model_get_iter (model, &iter, path);
  gtk_tree_path_free (path);

  gtk_tree_model_get (model, &iter,
		      0, &enable,
		      1, &barcode, // Se obtiene el codigo_corto desde el treeview
		      3, &precio,
		      8, &sugeridoAntes, // Sugerido antes de la modificacion
		      -1);


  if (enable == FALSE)
    {
      AlertMSG (GTK_WIDGET (builder_get (builder, "tree_view_products_providers")), 
		"Debe seleccionar producto a editar");
      return;
    }
  else if (sugerido < 1)
    {
      AlertMSG (GTK_WIDGET (builder_get (builder, "tree_view_products_providers")), 
		"Debe ingresar una cantidad mayor a cero");
      return;
    }
  
  PGresult *res;
  // Se obtiene el barcode a partir del codigo_corto
  barcode = PQvaluebycol (EjecutarSQL
			  (g_strdup_printf ("SELECT barcode FROM codigo_corto_to_barcode('%s')", barcode)),
			  0, "barcode");

  gchar *q = g_strdup_printf ("SELECT fraccion FROM producto WHERE barcode = '%s'", barcode);
  res = EjecutarSQL (q);
  g_free (q);

  if (res == NULL)
    return;

  fraccion = (g_str_equal (PQvaluebycol (res, 0, "fraccion"), "t") == TRUE) ? TRUE : FALSE;

  if (sugerido < 0) // Sugerido no puede ser negativo
    sugerido = 0;

  gtk_list_store_set (GTK_LIST_STORE (model), &iter,
		      8, (fraccion == TRUE) ? sugerido : lround(sugerido),
		      -1);

  // Recaclula subtotal
  if (enable == TRUE)
    {
      gchar *subTotalActualT = g_strdup (gtk_label_get_text (GTK_LABEL (gtk_builder_get_object (builder, "lbl_sum_selected"))));
      gdouble subTotalActual = strtod (PUT (subTotalActualT), (char **)NULL);

      // Se limpia subtotal restandole costo de la cantidad anterior
      // así se obtiene el subTotal sin contar este producto
      subTotal = subTotalActual - (precio * ((sugeridoAntes > 0) ? sugeridoAntes : 1));

      // Agrego al subtotal la cantidad sugerida actual (la que se acaba de editar)
      subTotal = subTotal + (precio * ((sugerido > 0) ? sugerido : 1));

      gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "lbl_sum_selected")),
			    g_strdup_printf ("<b>%ld</b>", lround (subTotal)));
    }
}


/**
 * Es llamada cuando el boton "btn_suggest_buy" es presionado (signal click).
 *
 * Esta funcion despliega una ventana, la cual permite realizar un proceso de compra
 * alternativo, basado en la selección de proveedores
 *
 */

void
on_btn_suggest_buy_clicked (GtkButton *button, gpointer user_data)
{
  GtkWidget *window;

  GtkListStore *store;
  GtkTreeView *treeview;
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;
  //GtkTreeSelection *selection;

  Print *sugerido = (Print *) malloc (sizeof (Print));
  sugerido->son = (Print *) malloc (sizeof (Print));

  treeview = GTK_TREE_VIEW (gtk_builder_get_object (builder, "tree_view_providers"));
  if (gtk_tree_view_get_model (treeview) == NULL )
    {
      // TreeView Providers
      store = gtk_list_store_new (4,
				  G_TYPE_STRING, //Rut
				  G_TYPE_STRING, //Nombre
				  G_TYPE_DOUBLE, //Lapso Reposición
				  G_TYPE_STRING, //Descripción
				  -1);

      treeview = GTK_TREE_VIEW (builder_get (builder, "tree_view_providers"));
      gtk_tree_view_set_model (treeview, GTK_TREE_MODEL (store));
      //selection = gtk_tree_view_get_selection (treeview);

      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes ("Rut", renderer,
							 "text", 0,
							 NULL);
      gtk_tree_view_append_column (treeview, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
      gtk_tree_view_column_set_sort_column_id (column, 0);
      gtk_tree_view_column_set_resizable (column, FALSE);
      gtk_tree_view_column_set_cell_data_func (column, renderer, control_rut, (gpointer)0, NULL);

      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes ("Nombre", renderer,
							 "text", 1,
							 NULL);
      gtk_tree_view_append_column (treeview, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
      gtk_tree_view_column_set_sort_column_id (column, 1);
      gtk_tree_view_column_set_resizable (column, FALSE);

      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes ("Lap. rep.", renderer,
							 "text", 2,
							 NULL);
      gtk_tree_view_append_column (treeview, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
      gtk_tree_view_column_set_sort_column_id (column, 2);
      gtk_tree_view_column_set_resizable (column, FALSE);
      gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)2, NULL);

      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes ("Descripción", renderer,
							 "text", 3,
							 NULL);
      gtk_tree_view_append_column (treeview, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
      gtk_tree_view_column_set_sort_column_id (column, 3);
      gtk_tree_view_column_set_resizable (column, FALSE);

      sugerido->tree = treeview;
      sugerido->title = "Sugerido de compras";
      sugerido->name = "Sugerido";
      sugerido->date_string = NULL;
      sugerido->cols[0].name = "Rut";
      sugerido->cols[0].num = 0;
      sugerido->cols[1].name = "Nombre";
      sugerido->cols[1].num = 1;
      sugerido->cols[2].name = "Lapso Rep.";
      sugerido->cols[2].num = 2;
      sugerido->cols[3].name = "Giro";
      sugerido->cols[3].num = 3;
      sugerido->cols[4].name = NULL;

      // END TreeView Providers


      // TreeView Products Providers (merchandises of each providers)
      store = gtk_list_store_new (9,
				  G_TYPE_BOOLEAN, //Elegir (Para comprar)
				  G_TYPE_STRING,  //Código
				  G_TYPE_STRING,  //Descripción
				  G_TYPE_DOUBLE,  //Costo_promedio
				  G_TYPE_DOUBLE,  //Precio
				  G_TYPE_DOUBLE,  //Vta/Dia
				  G_TYPE_DOUBLE,  //Stock
				  G_TYPE_DOUBLE,  //Dias/Stock
				  G_TYPE_DOUBLE,  //Sugerido
				  -1);

      treeview = GTK_TREE_VIEW (builder_get (builder, "tree_view_products_providers"));
      gtk_tree_view_set_model (treeview, GTK_TREE_MODEL (store));
      //selection = gtk_tree_view_get_selection (treeview);

      renderer = gtk_cell_renderer_toggle_new ();
      column = gtk_tree_view_column_new_with_attributes ("Elegir", renderer,
							 "active", 0,
							 NULL);
      gtk_tree_view_append_column (treeview, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
      //gtk_tree_view_column_set_sort_column_id (column, 0);
      gtk_tree_view_column_set_resizable (column, FALSE);

      g_signal_connect (G_OBJECT (renderer), "toggled",
			G_CALLBACK (ToggleProductSelection), NULL);

      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes ("Código", renderer,
							 "text", 1,
							 NULL);
      gtk_tree_view_append_column (treeview, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
      gtk_tree_view_column_set_sort_column_id (column, 1);
      gtk_tree_view_column_set_resizable (column, FALSE);

      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes ("Descripción Mercadería", renderer,
							 "text", 2,
							 NULL);
      gtk_tree_view_append_column (treeview, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
      gtk_tree_view_column_set_sort_column_id (column, 2);
      gtk_tree_view_column_set_min_width (column, 300);
      gtk_tree_view_column_set_max_width (column, 380);
      gtk_tree_view_column_set_resizable (column, TRUE);

      renderer = gtk_cell_renderer_text_new ();
      g_object_set (renderer, "editable", TRUE, NULL);
      g_signal_connect (G_OBJECT (renderer), "edited", G_CALLBACK (on_buy_price_edited), (gpointer)store);
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
      g_object_set (renderer, "editable", TRUE, NULL);
      g_signal_connect (G_OBJECT (renderer), "edited", G_CALLBACK (on_sell_price_edited), (gpointer)store);
      column = gtk_tree_view_column_new_with_attributes ("Precio", renderer,
							 "text", 4,
							 NULL);
      gtk_tree_view_append_column (treeview, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
      gtk_tree_view_column_set_sort_column_id (column, 4);
      gtk_tree_view_column_set_resizable (column, FALSE);
      gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)4, NULL);


      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes ("Vta/Dia", renderer,
							 "text", 5,
							 NULL);
      gtk_tree_view_append_column (treeview, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
      gtk_tree_view_column_set_sort_column_id (column, 5);
      gtk_tree_view_column_set_resizable (column, FALSE);
      gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)5, NULL);

      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes ("Stock", renderer,
							 "text", 6,
							 NULL);
      gtk_tree_view_append_column (treeview, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
      gtk_tree_view_column_set_sort_column_id (column, 6);
      gtk_tree_view_column_set_resizable (column, FALSE);
      gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)6, NULL);


      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes ("Dias/Stock", renderer,
							 "text", 7,
							 NULL);
      gtk_tree_view_append_column (treeview, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
      gtk_tree_view_column_set_sort_column_id (column, 7);
      gtk_tree_view_column_set_resizable (column, FALSE);
      gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)7, NULL);

      renderer = gtk_cell_renderer_text_new ();
      g_object_set (renderer, "editable", TRUE, NULL);
      g_signal_connect (G_OBJECT (renderer), "edited", G_CALLBACK (on_suggested_amount_edited), (gpointer)store);
      column = gtk_tree_view_column_new_with_attributes ("Sugerido", renderer,
							 "text", 8,
							 NULL);
      gtk_tree_view_append_column (treeview, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
      gtk_tree_view_column_set_sort_column_id (column, 8);
      gtk_tree_view_column_set_resizable (column, FALSE);
      gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)8, NULL);
      // END TreeView Products Providers      

      sugerido->son->tree = treeview;
      sugerido->son->cols[0].name = "Selection";
      sugerido->son->cols[0].num = 0;
      sugerido->son->cols[1].name = "Codigo Corto";
      sugerido->son->cols[1].num = 1;
      sugerido->son->cols[2].name = "Descripción";
      sugerido->son->cols[2].num = 2;
      sugerido->son->cols[3].name = "Costo Promedio";
      sugerido->son->cols[3].num = 3;
      sugerido->son->cols[4].name = "Precio";
      sugerido->son->cols[4].num = 4;
      sugerido->son->cols[5].name = "Venta/Dia";
      sugerido->son->cols[5].num = 5;
      sugerido->son->cols[6].name = "Stock";
      sugerido->son->cols[6].num = 6;
      sugerido->son->cols[7].name = "Dias/Stock";
      sugerido->son->cols[7].num = 7;
      sugerido->son->cols[8].name = "Cantidad";
      sugerido->son->cols[8].num = 8;
      sugerido->son->cols[9].name = NULL;

      g_signal_connect (builder_get (builder, "btn_print_suggest_buy"), "clicked",
			G_CALLBACK (SelectivePrintTwoTree), (gpointer)sugerido);

    }

  window = GTK_WIDGET (gtk_builder_get_object (builder, "wnd_suggest_buy"));
  clean_container (GTK_CONTAINER (window));
  
  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_buy_selected")), FALSE);
  gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_print_suggest_buy")), FALSE);

  gtk_widget_show_all (GTK_WIDGET (window));

  // Rellenando TreeView
  GtkTreeIter iter;
  gint i, tuples;
  PGresult *res;

  gchar *q = "SELECT * FROM proveedor";
  res = EjecutarSQL (q);
  tuples = PQntuples (res);
  store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (builder_get (builder, "tree_view_providers"))));

  if (res == NULL) return;

  for (i = 0; i < tuples; i++)
    {
      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
			  0, g_strconcat (PQvaluebycol (res, i, "rut"),
					  PQvaluebycol (res, i, "dv"), NULL),
                          1, PQvaluebycol (res, i, "nombre"),
			  2, strtod (PUT (PQvaluebycol (res, i, "lapso_reposicion")), (char **)NULL),
			  3, PQvaluebycol (res, i, "giro"),
			  -1);
    }  
}


/**
 * Callback connected to providers treeview "activated signal"
 *
 * This function show on providers_products treeview
 * the products by selected provider.
 *
 * @param tree_view: the object on which the signal is emitted
 * @param path: the GtkTreePath for the activated row
 * @param column: the GtkTreeViewColumn in which the activation occurred
 * @param user_data: user data set when the signal handler was connected.
 */

void
AskProductProvider (GtkTreeView *treeview, GtkTreePath *path_parameter,
		    GtkTreeViewColumn *column, gpointer user_data)
{
  GtkTreePath *path;
  GtkListStore *store;
  GtkTreeModel *model;
  GtkTreeIter iter;
  gchar *rut;
  gdouble lapso_rep;

  path = path_parameter;

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
  store = GTK_LIST_STORE (model);

  gtk_tree_model_get_iter (model, &iter, path);
  gtk_tree_model_get (model, &iter,
                      0, &rut,
		      2, &lapso_rep,
                      -1);
  PGresult *res;
  gint i, tuples;

  rut = g_strndup (rut, strlen (rut)-1);
  res = getProductsByProvider (rut);

  // servirá para elegir el proveedor al confirmar la compra
  rut_proveedor_global = g_strdup (rut);

  if (res == NULL) return;

  tuples = PQntuples (res);

  if (tuples <= 0)
    {
      ErrorMSG (GTK_WIDGET (builder_get (builder, "tree_view_providers")),
		"No existen mercaderías asociadas a este proveedor");
    }
  
  gdouble dias_stock, dias_stock_min, //unidades_rep, 
    cantidad_pedido, stock_actual, //stock_min, 
    ventas_dia, sugerido, stock_reposicion;
  gboolean fraccion;

  //Se obtiene el treeview de productos del proveedor
  model = gtk_tree_view_get_model (GTK_TREE_VIEW (builder_get (builder, "tree_view_products_providers")));
  store = GTK_LIST_STORE (model);
  gtk_list_store_clear (store);
  gtk_label_set_markup (GTK_LABEL (gtk_builder_get_object (builder, "lbl_sum_selected")),
			g_strdup_printf ("<b>0</b>"));

  for (i = 0; i < tuples; i++)
    {
      sugerido = 0;
      stock_actual = strtod (PUT(PQvaluebycol (res, i, "stock")), (char **)NULL),
	dias_stock_min = strtod (PUT(PQvaluebycol (res, i, "dias_stock")), (char **)NULL); //Dias minimos de stock
      dias_stock = strtod (PUT(PQvaluebycol (res, i, "stock_day")), (char **)NULL);     //Dias de stock disponibles
      ventas_dia = strtod (PUT(PQvaluebycol (res, i, "ventas_dia")), (char **)NULL);    //Promedio de ventas
      cantidad_pedido = strtod (PUT(PQvaluebycol (res, i, "cantidad_pedido")), (char **)NULL); //Cantidad Pedida no ingresada
      fraccion = (g_str_equal (PQvaluebycol (res, i, "fraccion"), "t")) ? TRUE : FALSE;
      //stock_min = dias_stock_min * ventas_dia; //Stock minimo
      //unidades_rep = lapso_rep * ventas_dia; //Unidades de reposicion

      /*Algorithm of suggested cantity to buy*/
      stock_reposicion = (dias_stock_min < lapso_rep) ? lapso_rep * ventas_dia : dias_stock_min * ventas_dia;
      sugerido = (((stock_actual + cantidad_pedido) - ventas_dia) > stock_reposicion) ? 0 : stock_reposicion;

      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
			  //0, Toggle Button (Elegir)
                          1, PQvaluebycol (res, i, "codigo_corto"),
			  2, g_strdup_printf ("%s %s %s %s", 
					      PQvaluebycol (res, i, "descripcion"),
					      PQvaluebycol (res, i, "marca"),
					      PQvaluebycol (res, i, "contenido"),
					      PQvaluebycol (res, i, "unidad")),
			  3, strtod (PUT(PQvaluebycol (res, i, "precio_compra")), (char **)NULL),
			  4, strtod (PUT(PQvaluebycol (res, i, "precio")), (char **)NULL),
			  5, ventas_dia,
                          6, stock_actual,
			  7, dias_stock,
			  8, (fraccion == TRUE) ? sugerido : lround (sugerido),
			  -1);
    }
}


/**
 * Callback connected to the entry_filter_providers when is edited
 * (changed signal)
 *
 * This function filters the content (on tree_view_providers) according 
 * to parameters obtained from the entry "entry_filter_providers".
 *
 * @param GtkEditable *editable
 * @param gpointer user_data
 */

void
on_entry_filter_providers_changed (GtkEditable *editable, gpointer user_data)
{
  GtkTreeView *treeview = GTK_TREE_VIEW (builder_get (builder, "tree_view_providers"));
  GtkTreeModel *model = gtk_tree_view_get_model (treeview);
  GtkTreeIter iter;
  GtkWidget *aux_widget;
  gboolean valid;
  gint fila_actual = 0; // Representa el numero de fila en el que se encuetra la iteración
  gint i; // Para la iteración del for

  gchar *rut, *nombre, *descripcion; // Datos que se obtienen del treeview
  gdouble lapRep; // Dato que se obtiene del treeview (lapso de reposición)
  gchar *patron, *textoFila, *textoFilaEliminada;

  /* Contienen las filas del treeview que son borradas */
  if (rutBorrado == NULL || nombreBorrado == NULL || 
      descripcionBorrada == NULL || lapRepBorrado == NULL )
    {
      rutBorrado = g_array_new (FALSE, FALSE, sizeof (gchar*));
      nombreBorrado = g_array_new (FALSE, FALSE, sizeof (gchar*));
      lapRepBorrado = g_array_new (FALSE, FALSE, sizeof (gdouble));
      descripcionBorrada = g_array_new (FALSE, FALSE, sizeof (gchar*));
    }

  /* Obtención del patron */
  aux_widget = GTK_WIDGET (gtk_builder_get_object (builder, "entry_filter_providers"));
  patron = g_strdup (gtk_entry_get_text (GTK_ENTRY (aux_widget))); //El texto obtenido será el patrón de filtrado

  valid = gtk_tree_model_get_iter_first (model, &iter);
  while (valid)
    {
      // Obtengo los valores del treeview --
      gtk_tree_model_get (model, &iter,
			  0, &rut,
                          1, &nombre,
			  2, &lapRep,
                          3, &descripcion,
                          -1);
      
      textoFila = g_strdup_printf ("%s %s %f %s", rut, nombre, lapRep, descripcion);
      //printf ("%s\n", textoFila);

      if (!g_regex_match_simple (patron, textoFila, G_REGEX_CASELESS, 0))
	{
	  /* Se almacenan los datos de las filas eliminadas */
	  g_array_append_val (rutBorrado, rut);
	  g_array_append_val (nombreBorrado, nombre);
	  g_array_append_val (lapRepBorrado, lapRep);
	  g_array_append_val (descripcionBorrada, descripcion);
	  
	  numFilasBorradas++;
	  
	  /* Se elimina la linea correspondiente y se "itera" si Ã©ste es la Ãºltima */
	  gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
	  if (fila_actual == get_treeview_length (treeview))
	    valid = gtk_tree_model_iter_next (model, &iter);
	}
      else
	{
	  valid = gtk_tree_model_iter_next (model, &iter);
	  fila_actual++;
	}
    }

  /* Para comprobar si el patron coincide con las filas eliminadas */  
  for (i = 0; i < numFilasBorradas; i++)
    {
      textoFilaEliminada = g_strdup_printf ("%s %s %f %s",
					    g_array_index (rutBorrado, gchar*, i),
					    g_array_index (nombreBorrado, gchar*, i),
					    g_array_index (lapRepBorrado, gdouble, i),
					    g_array_index (descripcionBorrada, gchar*, i));
	      
      if (g_regex_match_simple (patron, textoFilaEliminada, G_REGEX_CASELESS, 0))
	{
	  /* Se reincorpora la fila borrada al treeview */
	  gtk_list_store_append (GTK_LIST_STORE (model), &iter);
	  gtk_list_store_set (GTK_LIST_STORE (model), &iter,
			      0, g_array_index (rutBorrado, gchar*, i),
			      1, g_array_index (nombreBorrado, gchar*, i),
			      2, g_array_index (lapRepBorrado, gdouble, i),
			      3, g_array_index (descripcionBorrada, gchar*, i),
			      -1);
	  
	  /* Se quita la fila reincorporada al treeview de los datos eliminados */
	  rutBorrado = g_array_remove_index (rutBorrado, i);
	  nombreBorrado = g_array_remove_index (nombreBorrado, i);
	  lapRepBorrado = g_array_remove_index (lapRepBorrado, i);
	  descripcionBorrada = g_array_remove_index (descripcionBorrada, i);
	  numFilasBorradas--;
	  i--;
	}
    }
}


/**
 * Función temporal para facilitar la legibilidad de
 * "addSugestedBuy(void)"
 *
 * @param: void
 */

void
calcularPorcentajeGanancia (void)
{
  gchar *barcode = g_strdup (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_buy_barcode"))));
  gdouble ingresa = strtod (PUT (g_strdup (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_buy_price"))))), (char **)NULL);
  gdouble ganancia = (gdouble) atoi (g_strdup (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_buy_gain")))));
  gdouble precio_final = (gdouble) atoi (g_strdup (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "entry_sell_price")))));
  gdouble precio;
  gdouble porcentaje;

  gdouble iva = GetIVA (barcode);
  gdouble otros = GetOtros (barcode);

  /*------ Se calcula el porcentaje de ganancia ------*/
  if (iva != 0)
    iva = iva / 100;

  // TODO: Revisión (Concenso de la variable)
  if (otros != 0)
    otros = otros / 100;

  /* -- Profit porcent ("ganancia") is calculated here -- */
  if (otros == 0 && iva != 0)
    porcentaje = (((precio_final / (iva+1)) - ingresa) / ingresa) * 100;

  else if (iva != 0 && otros != 0 )
    {
      precio = (gdouble) precio_final / (gdouble)(iva + otros + 1);
      ganancia = (gdouble) precio - ingresa;
      porcentaje = (gdouble)(ganancia / ingresa) * 100;
    }
  else if (iva == 0 && otros == 0 )
    porcentaje = (gdouble) ((precio_final - ingresa) / ingresa) * 100;

  gtk_entry_set_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_buy_gain")),
		      g_strdup_printf ("%ld", lround (porcentaje)));
}


/**
 * Is called by "btn_buy_selected" (signal click).
 *
 * this function add the selected products to
 * "tree_view_products_buy_list"
 * 
 * @param: GtkButton *button: button from signal is triggered
 * @param: gpointer user_data: user data
 */

void
addSugestedBuy (GtkButton *button, gpointer user_data)
{
  /*Variables del treeview*/
  GtkTreeView *treeview = GTK_TREE_VIEW (builder_get (builder, "tree_view_products_providers"));
  GtkTreeModel *model = gtk_tree_view_get_model (treeview);
  //GtkListStore *store;
  GtkTreeIter iter;
  gboolean valid;

  /*Variables de informacion*/
  gboolean enabled;
  gchar *codigo_corto; //Se obtienen desde el treeview
  gdouble costo, precio, cantidad; //Se obtienen desde el treeview
  gchar *barcode; //Se obtienen a través de una consulta sql

  /*Variables consulta*/
  PGresult *res;
  gchar *q = NULL;

  //store = GTK_LIST_STORE (gtk_tree_view_get_model (treeview));
  valid = gtk_tree_model_get_iter_first (model, &iter);
  while (valid)
    {
      // Obtengo los valores del treeview --
      gtk_tree_model_get (model, &iter,
			  0, &enabled,
			  1, &codigo_corto,
			  3, &costo,
			  4, &precio,
			  8, &cantidad,
                          -1);

      if (enabled == TRUE)
	{
	  q = g_strdup_printf ("SELECT barcode, precio FROM producto where codigo_corto = '%s'", codigo_corto);
	  res = EjecutarSQL (q);
	  barcode = PQvaluebycol (res, 0, "barcode");
	  //precio = PQvaluebycol (res, 0, "precio");

	  if (cantidad == 0)
	    cantidad = 1;

	  //Set Buy's Entries
	  gtk_entry_set_text (GTK_ENTRY (builder_get (builder, "entry_buy_barcode")), barcode);
	  gtk_entry_set_text (GTK_ENTRY (builder_get (builder, "entry_buy_price")), g_strdup_printf ("%.2f",costo));
	  gtk_entry_set_text (GTK_ENTRY (builder_get (builder, "entry_sell_price")), g_strdup_printf ("%.2f",precio));
	  calcularPorcentajeGanancia (); // Calcula el porcentaje de ganancia y lo agraga al entry
	  gtk_entry_set_text (GTK_ENTRY (builder_get (builder, "entry_buy_amount")), g_strdup_printf ("%f",cantidad));

	  AddToProductsList(); // Agrega los productos a la lista de compra
	}
      valid = gtk_tree_model_iter_next (model, &iter);
    }    
  
  // Si se selecciona un producto
  if (q != NULL)
    {
      gtk_widget_set_sensitive (GTK_WIDGET (builder_get (builder, "btn_suggest_buy")), FALSE);
      gtk_widget_hide (GTK_WIDGET (gtk_builder_get_object (builder, "wnd_suggest_buy")));
    }
  else
    {
      ErrorMSG (GTK_WIDGET (builder_get (builder, "tree_view_providers")),"Debe elegir a lo menos un producto a comprar");
    }

  /* Se librea la consulta */
  g_free (q);

  /* Se libera la memeria del GArray*/
  if (rutBorrado != NULL || nombreBorrado != NULL || lapRepBorrado != NULL || 
      descripcionBorrada != NULL)
    {
      g_array_free (rutBorrado, TRUE); rutBorrado = NULL;
      g_array_free (nombreBorrado, TRUE); nombreBorrado = NULL;
      g_array_free (lapRepBorrado, TRUE); lapRepBorrado = NULL;
      g_array_free (descripcionBorrada, TRUE); descripcionBorrada = NULL;
    }

  /*El contador de proveedores filtrados retorna a cero*/
  numFilasBorradas = 0;
}


/**
 * Is called by "btn_close_suggest_buy" (signal click).
 *
 * this function hide the "wnd_suggest_buy" windows and
 * free GArray struct of memory.
 * 
 * @param: GtkButton *button: button from signal is triggered
 * @param: gpointer user_data: user data
 */

void 
hide_wnd_suggest_buy (GtkButton *button, gpointer user_data)
{
  if (rutBorrado != NULL || nombreBorrado != NULL || lapRepBorrado != NULL || 
      descripcionBorrada != NULL)
    {
      g_array_free (rutBorrado, TRUE); rutBorrado = NULL;
      g_array_free (nombreBorrado, TRUE); nombreBorrado = NULL;
      g_array_free (lapRepBorrado, TRUE); lapRepBorrado = NULL;
      g_array_free (descripcionBorrada, TRUE); descripcionBorrada = NULL;
    }

  //Se libera la variable global
  if (rut_proveedor_global != NULL)
    {
      g_free (rut_proveedor_global);
      rut_proveedor_global = NULL;
    }

  numFilasBorradas = 0;
  clean_container (GTK_CONTAINER (builder_get (builder, "wnd_suggest_buy")));
  gtk_widget_hide (GTK_WIDGET (gtk_builder_get_object (builder, "wnd_suggest_buy")));
}


/**
 * Callback when "change" event on 'treeview_nullify_buy_invoice' is triggered.
 *
 * This function shows (on the 'treeview_nullify_buy_invoice_details') 
 * the information correspondig to the row selected
 * on the 'treeview_nullify_buy_invoice'. 
 */
void
on_selection_nullify_buy_invoice_change (GtkTreeSelection *selection, gpointer data)
{
  GtkTreeView *treeview = GTK_TREE_VIEW (builder_get (builder, "treeview_nullify_buy_invoice_details"));
  GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model (treeview));
  gtk_list_store_clear (store);

  GtkTreeModel *model = gtk_tree_view_get_model (gtk_tree_selection_get_tree_view (selection));
  GtkTreeIter iter;

  gint i, tuples;
  gchar *q;
  PGresult *res;

  gint id_compra, id_factura_compra;

  if (gtk_tree_selection_get_selected (selection, NULL, &iter) == TRUE)
    {
      gtk_tree_model_get (model, &iter,
			  5, &id_compra,
			  6, &id_factura_compra,
			  -1);

      q = g_strdup_printf ("SELECT fcd.barcode, fcd.cantidad, fcd.precio, (fcd.cantidad * fcd.precio) AS subtotal, "
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
			      2, strtod (PUT (g_strdup (PQvaluebycol (res, i, "cantidad"))), (char **)NULL),
			      3, strtod (PUT (g_strdup (PQvaluebycol (res, i, "precio"))), (char **)NULL),
			      4, atoi (g_strdup (PQvaluebycol (res, i, "subtotal"))),
			      5, id_compra,
			      6, id_factura_compra,
			      -1);
	}
      
      PQclear (res);
    }

  //Se limpian las posibles modificaciones en la edición de compra
  clean_lista_mod_prod ();
}

/**
 * Callback when "change" event on 'treeview_nullify_buy' is triggered.
 *
 * This function shows (on the 'treeview_nullify_buy_invoice') 
 * the information correspondig to the row selected
 * on the 'treeview_nullify_buy'. 
 */
void 
on_selection_nullify_buy_change (GtkTreeSelection *selection, gpointer data)
{  
  GtkTreeView *treeview = GTK_TREE_VIEW (builder_get (builder, "treeview_nullify_buy_invoice"));
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
			  3, &nombre_proveedor,
			  4, &rut_proveedor,
			  -1);

      gtk_label_set_text (GTK_LABEL (builder_get (builder, "lbl_nullify_buy_provider_name")), nombre_proveedor);
      gtk_label_set_text (GTK_LABEL (builder_get (builder, "lbl_nullify_buy_provider_rut")), formato_rut (rut_proveedor));

      q = g_strdup_printf ("SELECT fc.id, fc.num_factura, fc.pagada, fc.id_compra, "
			   "date_part ('year', fc.fecha) AS f_year, "
			   "date_part ('month', fc.fecha) AS f_month, "
			   "date_part ('day', fc.fecha) AS f_day, "

			   "date_part ('year', fc.fecha_pago) AS fp_year, "
			   "date_part ('month', fc.fecha_pago) AS fp_month, "
			   "date_part ('day', fc.fecha_pago) AS fp_day, "

			   "(SELECT SUM (cantidad * precio) "
			   "        FROM factura_compra_detalle fcd "
			   "        WHERE fcd.id_factura_compra = fc.id) AS monto "

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
			      2, g_strdup_printf ("%s", (g_str_equal (PQvaluebycol (res, i, "pagada"), "t")) ? "Si" : "No"),
			      3, g_strdup_printf ("%s/%s/%s",
						  PQvaluebycol (res, i, "fp_day"),
						  PQvaluebycol (res, i, "fp_month"),
						  PQvaluebycol (res, i, "fp_year")),
			      4, atoi (g_strdup (PQvaluebycol (res, i, "monto"))),
			      5, id_compra,
			      6, atoi (g_strdup (PQvaluebycol (res, i, "id"))),
			      -1);
	}
      
      PQclear (res);
    }

  //Se limpian las posibles modificaciones en la edición de compra
  clean_lista_mod_prod ();
}

/**
 * Is called by "btn_nullify_buy_ok" (signal click).
 *
 * this function nullify the buy selected
 * 
 * @param: GtkButton *button: button from signal is triggered
 * @param: gpointer user_data: user data
 */
void
on_btn_nullify_buy_ok_clicked (GtkButton *button, gpointer user_data)
{
  GtkTreeView *treeview = GTK_TREE_VIEW (builder_get (builder, "treeview_nullify_buy"));
  GtkTreeSelection *selection  = gtk_tree_view_get_selection (treeview);
  GtkTreeModel *model = gtk_tree_view_get_model (gtk_tree_selection_get_tree_view (selection));
  GtkTreeIter iter;  

  gint id_compra;

  gchar *q;
  PGresult *res;
  gint tuples, i;


  if (gtk_tree_selection_get_selected (selection, NULL, &iter) == FALSE)
    {
      AlertMSG (GTK_WIDGET (builder_get (builder, "treeview_nullify_buy")),
		"Debe seleccionar la compra que desea anular");
      return;
    }

  gtk_tree_model_get (model, &iter,
		      0, &id_compra,
		      -1);

  q = g_strdup_printf ("SELECT * FROM nullify_buy (%d, %d)", id_compra, atoi (rizoma_get_value ("MAQUINA")));
  res = EjecutarSQL (q);
  g_free (q);
  tuples = PQntuples (res);
  
  for (i = 0; i < tuples; i++)
    {
      gtk_entry_set_text (GTK_ENTRY (builder_get (builder, "entry_buy_barcode")), PQvaluebycol (res, i, "barcode_out"));
      gtk_entry_set_text (GTK_ENTRY (builder_get (builder, "entry_buy_price")), PUT (PQvaluebycol (res, i, "costo_out")));
      gtk_entry_set_text (GTK_ENTRY (builder_get (builder, "entry_sell_price")), PUT (PQvaluebycol (res, i, "precio_out")));
      calcularPorcentajeGanancia (); // Calcula el porcentaje de ganancia y lo agraga al entry
      
      gtk_entry_set_text (GTK_ENTRY (builder_get (builder, "entry_buy_amount")), PQvaluebycol (res, i, "cantidad_anulada_out"));
      
      AddToProductsList(); // Agrega los productos a la lista de compra
    }

  gtk_widget_hide (GTK_WIDGET (builder_get (builder, "wnd_nullify_buy")));

  //Se limpian las posibles modificaciones en la edición de compra
  clean_lista_mod_prod ();
}

/**
 * This function loads the nullify buy dialog.
 *
 * Here must stay all the configuration of the dialog that is needed
 * when the dialog will be showed to the user.
 */
void
nullify_buy_win(void)
{
  GtkWidget *widget;

  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  GtkTreeView *treeview_buy;
  GtkTreeSelection *selection_buy;
  GtkTreeView *treeview_invoice;
  GtkTreeSelection *selection_invoice;
  GtkTreeView *treeview_details;

  GtkListStore *store_buy;
  GtkListStore *store_invoice;
  GtkListStore *store_details;

  // TreeView Compras
  treeview_buy = GTK_TREE_VIEW(gtk_builder_get_object(builder, "treeview_nullify_buy"));
  store_buy = GTK_LIST_STORE(gtk_tree_view_get_model(treeview_buy));
  if (store_buy == NULL)
    {
      store_buy = gtk_list_store_new (5,
				      G_TYPE_INT,     //Id
				      G_TYPE_STRING,  //Date
				      G_TYPE_INT,     //Total amount
				      G_TYPE_STRING,  //Nombre proveedor
				      G_TYPE_STRING); //Rut proveedor

      gtk_tree_view_set_model(treeview_buy, GTK_TREE_MODEL(store_buy));

      selection_buy = gtk_tree_view_get_selection (treeview_buy);
      gtk_tree_selection_set_mode (selection_buy, GTK_SELECTION_SINGLE);
      g_signal_connect (G_OBJECT (selection_buy), "changed",
			G_CALLBACK (on_selection_nullify_buy_change), NULL);

      //ID
      renderer = gtk_cell_renderer_text_new();
      column = gtk_tree_view_column_new_with_attributes("ID", renderer,
                                                        "text", 0,
                                                        NULL);
      gtk_tree_view_append_column (treeview_buy, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
      gtk_tree_view_column_set_sort_column_id (column, 0);
      gtk_tree_view_column_set_resizable (column, FALSE);

      //date
      renderer = gtk_cell_renderer_text_new();
      column = gtk_tree_view_column_new_with_attributes("Fecha", renderer,
                                                        "text", 1,
                                                        NULL);
      gtk_tree_view_append_column (treeview_buy, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
      gtk_tree_view_column_set_sort_column_id (column, 1);
      gtk_tree_view_column_set_resizable (column, FALSE);
      
      //Total amount
      renderer = gtk_cell_renderer_text_new();
      column = gtk_tree_view_column_new_with_attributes("Monto Total", renderer,
                                                        "text", 2,
                                                        NULL);
      gtk_tree_view_append_column (treeview_buy, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
      gtk_tree_view_column_set_sort_column_id (column, 2);
      gtk_tree_view_column_set_resizable (column, FALSE);
      gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)2, NULL);
    }

  // TreeView Facturas
  treeview_invoice = GTK_TREE_VIEW(gtk_builder_get_object(builder, "treeview_nullify_buy_invoice"));
  store_invoice = GTK_LIST_STORE(gtk_tree_view_get_model(treeview_invoice));
  if (store_invoice == NULL)
    {
      store_invoice = gtk_list_store_new (7,
					  G_TYPE_INT,    // N° Factura
					  G_TYPE_STRING, // Fecha Ingreso
					  G_TYPE_STRING, // Pagada
					  G_TYPE_STRING, // Fecha Pago
					  G_TYPE_INT,    // Monto Total
					  G_TYPE_INT,    // ID Compra
					  G_TYPE_INT);   // ID Factura Compra

      gtk_tree_view_set_model(treeview_invoice, GTK_TREE_MODEL(store_invoice));
      
      selection_invoice = gtk_tree_view_get_selection (treeview_invoice);
      gtk_tree_selection_set_mode (selection_invoice, GTK_SELECTION_SINGLE);
      g_signal_connect (G_OBJECT(selection_invoice), "changed",
			G_CALLBACK (on_selection_nullify_buy_invoice_change), NULL);

      //ID
      renderer = gtk_cell_renderer_text_new();
      column = gtk_tree_view_column_new_with_attributes("N° Factura", renderer,
                                                        "text", 0,
                                                        NULL);
      gtk_tree_view_append_column (treeview_invoice, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
      gtk_tree_view_column_set_sort_column_id (column, 0);
      gtk_tree_view_column_set_resizable (column, FALSE);

      //Fecha Ingreso
      renderer = gtk_cell_renderer_text_new();
      column = gtk_tree_view_column_new_with_attributes("Fecha Ing.", renderer,
                                                        "text", 1,
                                                        NULL);
      gtk_tree_view_append_column (treeview_invoice, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
      gtk_tree_view_column_set_sort_column_id (column, 1);
      gtk_tree_view_column_set_resizable (column, FALSE);

      //Pagado
      renderer = gtk_cell_renderer_text_new();
      column = gtk_tree_view_column_new_with_attributes("Pagada", renderer,
                                                        "text", 2,
                                                        NULL);
      gtk_tree_view_append_column (treeview_invoice, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
      gtk_tree_view_column_set_sort_column_id (column, 2);
      gtk_tree_view_column_set_resizable (column, FALSE);

      //Fecha Pago
      renderer = gtk_cell_renderer_text_new();
      column = gtk_tree_view_column_new_with_attributes("Fecha Pago", renderer,
                                                        "text", 3,
                                                        NULL);
      gtk_tree_view_append_column (treeview_invoice, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
      gtk_tree_view_column_set_sort_column_id (column, 3);
      gtk_tree_view_column_set_resizable (column, FALSE);

      //Monto Total
      renderer = gtk_cell_renderer_text_new();
      column = gtk_tree_view_column_new_with_attributes("Monto Total", renderer,
                                                        "text", 4,
                                                        NULL);
      gtk_tree_view_append_column (treeview_invoice, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
      gtk_tree_view_column_set_sort_column_id (column, 4);
      gtk_tree_view_column_set_resizable (column, FALSE);
      gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)4, NULL);
    }

  //TreeView Detalle Factura
  treeview_details = GTK_TREE_VIEW(gtk_builder_get_object(builder, "treeview_nullify_buy_invoice_details"));
  store_details = GTK_LIST_STORE(gtk_tree_view_get_model(treeview_details));
  if (store_details == NULL)
    {
      store_details = gtk_list_store_new (9,
                                          G_TYPE_STRING, //barcode
                                          G_TYPE_STRING, //description
                                          G_TYPE_DOUBLE, //cantity
                                          G_TYPE_DOUBLE, //price
                                          G_TYPE_INT,    //subtotal
                                          G_TYPE_INT,    //Id_compra
                                          G_TYPE_INT,    //id_factura_compra
					  G_TYPE_STRING,
					  G_TYPE_BOOLEAN);

      gtk_tree_view_set_model(treeview_details, GTK_TREE_MODEL(store_details));
      //gtk_tree_selection_set_mode (gtk_tree_view_get_selection (treeview_details), GTK_SELECTION_NONE);

      //barcode
      renderer = gtk_cell_renderer_text_new();
      column = gtk_tree_view_column_new_with_attributes("Cod. Barras", renderer,
                                                        "text", 0,
							"foreground", 7,
							"foreground-set", 8,
                                                        NULL);
      gtk_tree_view_append_column (treeview_details, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
      gtk_tree_view_column_set_sort_column_id (column, 0);
      gtk_tree_view_column_set_resizable (column, FALSE);

      //description
      renderer = gtk_cell_renderer_text_new();
      column = gtk_tree_view_column_new_with_attributes("Descripcion", renderer,
                                                        "text", 1,
							"foreground", 7,
							"foreground-set", 8,
                                                        NULL);
      gtk_tree_view_append_column (treeview_details, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
      gtk_tree_view_column_set_sort_column_id (column, 1);
      gtk_tree_view_column_set_resizable (column, FALSE);

      //cantity
      renderer = gtk_cell_renderer_text_new();
      g_object_set (renderer,"editable", TRUE, NULL);
      g_signal_connect (G_OBJECT (renderer), "edited",
			G_CALLBACK (on_mod_buy_cantity_cell_renderer_edited), (gpointer)store_details);
      column = gtk_tree_view_column_new_with_attributes("Cantidad", renderer,
                                                        "text", 2,
							"foreground", 7,
							"foreground-set", 8,
                                                        NULL);
      gtk_tree_view_append_column (treeview_details, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
      gtk_tree_view_column_set_sort_column_id (column, 2);
      gtk_tree_view_column_set_resizable (column, FALSE);
      gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)2, NULL);

      //buy price (cost) 
      renderer = gtk_cell_renderer_text_new();
      g_object_set (renderer,"editable", TRUE, NULL);
      g_signal_connect (G_OBJECT (renderer), "edited",
			G_CALLBACK (on_mod_buy_price_cell_renderer_edited), (gpointer)store_details);
      column = gtk_tree_view_column_new_with_attributes("Costo", renderer,
                                                        "text", 3,
							"foreground", 7,
							"foreground-set", 8,
                                                        NULL);
      gtk_tree_view_append_column (treeview_details, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
      gtk_tree_view_column_set_sort_column_id (column, 3);
      gtk_tree_view_column_set_resizable (column, FALSE);
      gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)3, NULL);

      //subtotal
      renderer = gtk_cell_renderer_text_new();
      column = gtk_tree_view_column_new_with_attributes("Subtotal", renderer,
                                                        "text", 4,
							"foreground", 7,
							"foreground-set", 8,
                                                        NULL);
      gtk_tree_view_append_column (treeview_details, column);
      gtk_tree_view_column_set_alignment (column, 0.5);
      g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
      gtk_tree_view_column_set_sort_column_id (column, 4);
      gtk_tree_view_column_set_resizable (column, FALSE);
      gtk_tree_view_column_set_cell_data_func (column, renderer, control_decimal, (gpointer)4, NULL);
    }

  widget = GTK_WIDGET (gtk_builder_get_object(builder, "wnd_nullify_buy"));
  clean_container (GTK_CONTAINER (widget));
  gtk_widget_show_all (widget);
}

/**
 * This is a callback from button 'btn_nullify_buy_pi'
 * (signal clicked).
 *
 * Is called by 'f5' key too.
 *
 * Show the 'wnd_nullify_buy' window, to nullify purchases
 * that have already been entered.
 *
 * @param button the button
 * @param user_data the user data
 */
void 
on_btn_nullify_buy_pi_clicked (GtkButton *button, gpointer data)
{
  if (user_data->user_id == 1)
    nullify_buy_win ();
}
