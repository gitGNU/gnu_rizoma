/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4;
       c-indentation-style: gnu -*- */
/*manejo_productos.c
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
#include<string.h>
#include<stdlib.h>
#include<math.h>

#include"tipos.h"
#include"postgres-functions.h"
#include"utils.h"

Productos *
CreateNew (gchar *barcode, gdouble cantidad)
{
  Productos *new = NULL;
  PGresult *res;
  gchar *q;

  q = g_strdup_printf ("SELECT codigo_corto, barcode, descripcion, marca, contenido, otros, familia, impuestos, fraccion, "
                       "unidad, COALESCE(precio, 0) AS precio, margen_promedio, tipo, "
		       "(SELECT costo FROM obtener_costo_promedio_desde_barcode (%s)) AS costo_promedio, "
                       "(SELECT monto FROM impuesto WHERE id=1 AND producto.impuestos='t') as impuesto_normal, "
                       "(SELECT monto FROM impuesto WHERE id=producto.otros) as impuesto_otro, "
                       "canje, stock_pro, precio_mayor, cantidad_mayor, mayorista "
                       "FROM select_producto(%s) as producto",
                       barcode, barcode);

  res = EjecutarSQL (q);
  g_free (q);

  new = (Productos *) g_malloc (sizeof (Productos));
  new->product = (Producto *) g_malloc (sizeof (Producto));
  new->product->cantidad = cantidad;
  new->product->codigo = PQvaluebycol (res, 0, "codigo_corto");
  new->product->barcode = PQvaluebycol (res, 0, "barcode");
  new->product->producto = PQvaluebycol (res, 0, "descripcion");
  new->product->marca = PQvaluebycol (res, 0, "marca");
  new->product->contenido = atoi (PQvaluebycol (res, 0, "contenido"));
  new->product->unidad = PQvaluebycol (res, 0, "unidad");
  new->product->precio = strtod (PUT (PQvaluebycol (res, 0, "precio")), (char **)NULL);
  new->product->fifo = strtod (PUT (PQvaluebycol (res, 0, "costo_promedio")), (char **)NULL); //TODO: Corregir nomenclatura. No es fifo
  new->product->precio_compra = GetNeto (barcode);
  new->product->iva = strtod (PUT (PQvaluebycol (res, 0, "impuesto_normal")), (char **)NULL);
  new->product->otros = strtod (PUT (PQvaluebycol (res, 0, "impuesto_otro")), (char **)NULL);
  new->product->otros_id = atoi (PQvaluebycol (res, 0, "otros"));
  new->product->impuestos = g_str_equal (PQvaluebycol (res, 0, "impuestos"), "t") == TRUE ? TRUE : FALSE;
  new->product->fraccion = g_str_equal (PQvaluebycol (res, 0, "fraccion"), "t") == TRUE ? TRUE : FALSE;
  new->product->familia = atoi (PQvaluebycol (res, 0, "familia"));
  new->product->margen = strtod (PUT (PQvaluebycol (res, 0, "margen_promedio")), (char **)NULL);
  new->product->canjeable = atoi (PQvaluebycol (res, 0, "canje"));
  new->product->stock_pro = strtod (PUT (PQvaluebycol (res, 0, "stock_pro")), (char **)NULL);
  new->product->tipo = atoi (PQvaluebycol (res, 0, "tipo"));
  /* Datos Mayoristas */
  new->product->precio_mayor = strtod (PUT (PQvaluebycol (res, 0, "precio_mayor")), (char **)NULL);
  new->product->cantidad_mayorista = strtod (PUT (PQvaluebycol (res, 0, "cantidad_mayor")), (char **)NULL);
  new->product->mayorista = strcmp (PQvaluebycol (res, 0, "mayorista"), "t") == 0 ? TRUE : FALSE;

  return new;
}

/**
 *
 * Libera la memoria de un producto eliminado desde la lista de productos.
 *
 */
void
FreeProduct (Productos *productos)
{
    g_free (productos->product);
    g_free (productos);
}

gint
AgregarALista (gchar *codigo, gchar *barcode, gdouble cantidad)
{
  Productos *new = NULL;

  if (venta->header == NULL)
    {
      new = CreateNew (barcode, cantidad);
      venta->header = new;

      venta->products = new;

      new->back = NULL;
      new->next = venta->header;

      new->product->lugar = 1;
    }
  else
    {
      Productos *end = venta->header;

      new = CreateNew (barcode, cantidad);

      while (end->next != venta->header)
        end = end->next;

      new->back = end;

      end->next = new;

      new->next = venta->header;

      venta->products = new;

      new->product->lugar = new->back->product->lugar+1;
    }

  return 0;
}

gint
EliminarDeLista (gchar *codigo, gint position)
{
  Productos *find = venta->header;
  Productos *end = venta->header;

  /*
    Must be re-write it, to fix and prevent any kind of error
  */

  while (strcmp (find->product->codigo, codigo) != 0)
    find = find->next;

  while (end->next != venta->header)
    end = end->next;

  if (find == venta->header && venta->header->next == venta->header)
    {
      FreeProduct (find);

      venta->header = NULL;
      venta->products = NULL;
    }
  else if (find->back == NULL)
    {
      venta->header = find->next;
      venta->header->back = NULL;
      end->next = venta->header;

      FreeProduct (find);
    }
  else if (find == end)
    {
      find->back->next = venta->header;

      FreeProduct (find);
    }
  else
    {
      find->back->next = find->next;
      find->next->back = find->back;

      FreeProduct (find);
    }

  venta->products = venta->header;

  return 0;
}

gdouble
CalcularTotal (Productos *header)
{
  Productos *cal = header;
  gdouble total = 0;

  if (cal == NULL)
    return total;

  do
    {
      if (cal->product->mayorista == FALSE && cal->product->cantidad < cal->product->cantidad_mayorista)
        total += (gdouble)(cal->product->precio * cal->product->cantidad);
      else if (cal->product->mayorista == TRUE && cal->product->cantidad >= cal->product->cantidad_mayorista)
        total += (gdouble)(cal->product->precio_mayor * cal->product->cantidad);
      else
        total += (gdouble)(cal->product->precio * cal->product->cantidad);

      cal = cal->next;
    }
  while (cal != header);

  return total;
}

gdouble
CalcularSoloAfecto (Productos *header)
{
  Productos *cal = header;
  gdouble total = 0;

  if (cal == NULL)
    return total;

  do
    {
      if (GetIVA (cal->product->barcode) != 0)
	{
	  if (cal->product->mayorista == FALSE && cal->product->cantidad < cal->product->cantidad_mayorista)
	    total += (gdouble)(cal->product->precio * cal->product->cantidad);
	  else if (cal->product->mayorista == TRUE && cal->product->cantidad >= cal->product->cantidad_mayorista)
	    total += (gdouble)(cal->product->precio_mayor * cal->product->cantidad);
	  else
	    total += (gdouble)(cal->product->precio * cal->product->cantidad);
	}
      
      cal = cal->next;
    }
  while (cal != header);

  return total;
}

gdouble
CalcularSoloNoAfecto (Productos *header)
{
  Productos *cal = header;
  gdouble total = 0;

  if (cal == NULL)
    return total;

  do
    {
      if (GetIVA (cal->product->barcode) == 0)
	{
	  if (cal->product->mayorista == FALSE && cal->product->cantidad < cal->product->cantidad_mayorista)
	    total += (gdouble)(cal->product->precio * cal->product->cantidad);
	  else if (cal->product->mayorista == TRUE && cal->product->cantidad >= cal->product->cantidad_mayorista)
	    total += (gdouble)(cal->product->precio_mayor * cal->product->cantidad);
	  else
	    total += (gdouble)(cal->product->precio * cal->product->cantidad);
	}
      
      cal = cal->next;
    }
  while (cal != header);

  return total;
}


gdouble
CalcularTotalProporcionAfecta (Productos *header)
{
  Productos *cal = header;
  gdouble total = 0;

  if (cal == NULL)
    return total;

  do
    { //La proporcion afecta la tienen solo los productos con impuestos
      if (GetIVA (cal->product->barcode) != 0)
	total += (gdouble)(cal->product->proporcion_afecta_imp);

      cal = cal->next;
    }
  while (cal != header);

  return total;
}


gdouble
CalcularTotalProporcionNoAfecta (Productos *header)
{
  Productos *cal = header;
  gdouble total = 0;

  if (cal == NULL)
    return total;

  do
    { //La proporcion no afecta la tienen solo los productos con impuestos
      if (GetIVA (cal->product->barcode) != 0)	
	total += (gdouble)(cal->product->proporcion_no_afecta_imp);
      
      cal = cal->next;
    }
  while (cal != header);

  return total;
}


gint
ReturnTotalProducts (Productos *header)
{
  Productos *cal = header;
  gint total = 0;

  do
    {
      total += 1;

      cal = cal->next;
    }
  while (cal != header);

  return total;
}

gint
ListClean (void)
{
  Productos *alter = venta->header;
  Productos *tofree;
  gint total;
  gint i;

  if (venta->header == NULL) return 0;

  total = ReturnTotalProducts (venta->header);

  for (i = 0; i < total; i++)
    {
      tofree = alter;
      alter = tofree->next;
      FreeProduct (tofree);
    }

  venta->header = NULL;
  venta->products = NULL;

  return 0;
}

gint
CompraListClean (void)
{
  Productos *alter = compra->header;
  Productos *tofree;
  //  gint total = ReturnTotalProducts (compra->header);
  //  gint i;

  /*
    for (i = 0; i < total; i++)
    {
    tofree = alter;
    alter = tofree->next;
    FreeProduct (tofree);
    }
  */

  do {

    tofree = alter;

    FreeProduct (tofree);

    tofree = alter->next;

    /* g_free (alter); */

    alter = tofree;

  } while (alter != compra->header);

  compra->header = NULL;
  compra->products_list = NULL;
  compra->current = NULL;

  return 0;
}

Productos *
CompraCreateNew (gchar *barcode, gdouble cantidad, gdouble precio_final, gdouble precio_compra, gdouble margen)
{
  Productos *new = NULL;
  PGresult *res;
  gchar *q;

  q = g_strdup_printf ("SELECT codigo_corto, barcode, descripcion, marca, contenido, unidad, "
                       "perecibles, canje, stock, stock_pro, tasa_canje, precio_mayor, costo_promedio, "
                       "cantidad_mayor, mayorista FROM select_producto (%s)",
                       barcode);
  res = EjecutarSQL (q);
  g_free (q);

  if (PQntuples (res) == 0)
    return NULL;

  new = (Productos *) g_malloc (sizeof (Productos));

  new->product = (Producto *) g_malloc (sizeof (Producto));

  new->product->cantidad = cantidad;
  new->product->codigo = g_strdup (PQvaluebycol (res, 0, "codigo_corto"));
  new->product->barcode = PQvaluebycol (res, 0, "barcode");
  new->product->producto = PQvaluebycol (res, 0, "descripcion");
  new->product->marca = PQvaluebycol (res, 0, "marca");
  new->product->contenido = atoi (PQvaluebycol (res, 0, "contenido"));
  new->product->unidad = PQvaluebycol (res, 0, "unidad");
  //  new->product->precio = atoi (PQvaluebycol (res, 0, 8));
  new->product->margen = margen;
  new->product->iva = GetIVA (barcode);
  new->product->otros = GetOtros (barcode);
  new->product->precio = precio_final;
  new->product->precio_neto = (double)precio_compra * ((double)(margen / 100) + 1); //Precio venta neto
  new->product->precio_compra = precio_compra;
  new->product->fifo = strtod (PUT (PQvaluebycol (res, 0, "costo_promedio")), (char **)NULL); //TODO: Corregir nomenclatura. No es fifo
  new->product->ingresar = TRUE;
  new->product->perecible = strcmp (PQvaluebycol (res, 0, "perecibles"), "t") ? FALSE : TRUE;
  new->product->canjeable = strcmp (PQvaluebycol (res, 0, "canje"), "t") ? FALSE : TRUE;
  new->product->stock = strtod (PUT(PQvaluebycol (res, 0, "stock")), (char **) NULL);
  new->product->stock_pro = strtod (PUT(PQvaluebycol (res, 0, "stock_pro")), (char **) NULL);
  new->product->tasa_canje = strtod (PUT(PQvaluebycol (res, 0, "tasa_canje")), (char **) NULL);
  /* Datos Mayoristas */
  new->product->precio_mayor = strtod (PUT(PQvaluebycol (res, 0, "precio_mayor")), (char **) NULL);
  new->product->cantidad_mayorista = strtod (PUT(PQvaluebycol (res, 0, "cantidad_mayor")), (char **) NULL);
  new->product->mayorista = atoi (PQvaluebycol (res, 0, "mayorista"));

  new->product->canjear = FALSE;

  return new;
}

gboolean
CompraAgregarALista (gchar *barcode, gdouble cantidad, gdouble precio_final, gdouble precio_compra,
                     gdouble margen, gboolean ingreso)
{
  Productos *new = NULL;

  if (ingreso == TRUE)
    {
      if (compra->header == NULL)
        {
          new = CompraCreateNew (barcode, cantidad, precio_final, precio_compra, margen);

          if (new == NULL)
            return FALSE;

          compra->header = new;

          compra->products_list = new;

          new->back = NULL;
          new->next = compra->header;
        }
      else
        {
          Productos *end = compra->header;

          new = CompraCreateNew (barcode, cantidad, precio_final, precio_compra, margen);

          if (new == NULL)
            return FALSE;

          while (end->next != compra->header)
            end = end->next;

          new->back = end;

          end->next = new;

          new->next = compra->header;

          compra->products_list = new;
        }
    }
  else if (ingreso == FALSE)
    {
      if (compra->header_compra == NULL)
        {
          new = CompraCreateNew (barcode, cantidad, precio_final, precio_compra, margen);

          if (new == NULL)
            return FALSE;

          compra->header_compra = new;

          compra->products_compra = new;

          new->back = NULL;
          new->next = compra->header_compra;
          compra->current = new->product;
        }
      else
        {
          Productos *end = compra->header_compra;

          new = CompraCreateNew (barcode, cantidad, precio_final, precio_compra, margen);

          if (new == NULL)
            return FALSE;

          while (end->next != compra->header_compra)
            end = end->next;

          new->back = end;

          end->next = new;

          new->next = compra->header_compra;

          compra->products_compra = new;
          compra->current = new->product;
        }
    }

  return TRUE;
}


/**
 *
 * Esta funcion elimina el producto seleccionado, de  la lista de productos a
 * comprar.
 *
 */
void
DropBuyProduct (gchar *codigo)
{
  Productos *find = compra->header_compra;
  Productos *end = compra->header_compra;

  while (strcmp (find->product->codigo, codigo) != 0)
    find = find->next;

  while (end->next != compra->header_compra)
    end = end->next;

  if (find == compra->header_compra && compra->header_compra->next == compra->header_compra)
    {
      FreeProduct (find);

      compra->header_compra = NULL;
      compra->products_compra = NULL;
    }
  else if (find->back == NULL)
    {
      compra->header_compra = find->next;
      end->next = compra->header_compra;
      compra->header_compra->back = NULL;

      FreeProduct (find);
    }
  else if (find == end)
    {
      find->back->next = compra->header_compra;

      FreeProduct (find);
    }
  else
    {
      find->back->next = find->next;
      find->next->back = find->back;

      FreeProduct (find);
    }

  return;
}

Producto *
SearchProductByBarcode (gchar *barcode, gboolean ingreso)
{
  Productos *find;
  Productos *control;

  if (ingreso == TRUE)
    {
      find = compra->header;
      control = compra->header;
    }
  else
    {
      find = compra->header_compra;
      control = compra->header_compra;
    }

  do
    {
      if (strcmp (find->product->barcode, barcode) == 0)
        return find->product;
      else
        find = find->next;
    }
  while (find != control);

  return NULL;
}

void
SetCurrentProductTo (gchar *barcode, gboolean ingreso)
{
  compra->current = SearchProductByBarcode (barcode, ingreso);
}

Productos *
BuscarPorCodigo (Productos *header, gchar *code)
{
  Productos *find = header;

  if (header == NULL)
    return NULL;

  do
    {
      if (strcmp (find->product->codigo, code) == 0)
        return find;
      else
        find = find->next;
    }
  while (find != header);

  return NULL;
}

gdouble
CalcularTotalCompra (Productos *header)
{
  Productos *cal = header;
  gdouble total = 0;

  if (cal == NULL)
    return total;

  do
    {
      //      if (cal->product->precio_compra == 0)
      total += (cal->product->precio_compra * cal->product->cantidad);
      /*      else
              total += (cal->product->precio_compra * cal->product->cantidad);
      */
      cal = cal->next;
    }
  while (cal != header);

  return total;
}

gboolean
LookCanjeable (Productos *header)
{
  Productos *look = header;

  do
    {
      if (look->product->canjeable == TRUE)
        return TRUE;

      look = look->next;
    }
  while (look != header);

  return FALSE;
}


/*Lista de productos de una factura a modificar*/

Prods *
create_prod (gchar *barcode, gboolean con_costo)
{
  Prods *new = NULL;
  PGresult *res;
  gchar *q;

  q = g_strdup_printf ("SELECT barcode, descripcion "
		       "FROM producto "
                       "WHERE barcode = %s "
		       "OR codigo_corto = '%s'",
                       barcode, barcode);

  res = EjecutarSQL (q);
  g_free (q);

  new = (Prods *) g_malloc (sizeof (Prods));
  new->prod = (Prod *) g_malloc (sizeof (Prod));

  new->prod->barcode = g_strdup (PQvaluebycol (res, 0, "barcode"));
  new->prod->descripcion = g_strdup (PQvaluebycol (res, 0, "descripcion"));

  new->prod->id_factura_compra = 0;

  new->prod->costo_original = 0;
  new->prod->costo_nuevo = 0;
  new->prod->cantidad_original = 0;
  new->prod->cantidad_nueva = 0;

  if (con_costo)
    new->prod->costo_nuevo = get_last_buy_price (barcode);
  
  return new;
}

/**
 * Add prods to list 
 *
 * @param: code = The barcode or short code
 * @param: cant = The cantity with start 
 */
gint
add_to_mod_prod_list (gchar *code, gboolean con_costo)
{
  Prods *new = NULL;

  if (lista_mod_prod->header == NULL)
    {
      new = create_prod (code, con_costo);
      lista_mod_prod->header = new;

      lista_mod_prod->prods = new;

      new->back = NULL;
      new->next = lista_mod_prod->header;

      new->prod->lugar = 1;
    }
  else
    {
      Prods *end = lista_mod_prod->header;

      new = create_prod (code, con_costo);

      while (end->next != lista_mod_prod->header)
        end = end->next;

      new->back = end;

      end->next = new;

      new->next = lista_mod_prod->header;

      lista_mod_prod->prods = new;

      new->prod->lugar = new->back->prod->lugar+1;
    }

  return 0;
}


Prods *
buscar_prod (Prods *header, gchar *barcode)
{
  Prods *find = header;

  if (header == NULL)
    return NULL;

  do
    {
      if (strcmp (find->prod->barcode, barcode) == 0)
        return find;
      else
        find = find->next;
    }
  while (find != header);

  return NULL;
}


void
free_prods (Prods *prods)
{
    g_free (prods->prod);
    g_free (prods);
}


void
drop_prod_to_mod_list (gchar *barcode)
{
  Prods *find = lista_mod_prod->header;
  Prods *end = lista_mod_prod->header;

  while (strcmp (find->prod->barcode, barcode) != 0)
    find = find->next;

  while (end->next != lista_mod_prod->header)
    end = end->next;

  if (find == lista_mod_prod->header && lista_mod_prod->header->next == lista_mod_prod->header)
    {
      free_prods (find);

      lista_mod_prod->header = NULL;
      lista_mod_prod->prods = NULL;
    }
  else if (find->back == NULL)
    {
      lista_mod_prod->header = find->next;
      end->next = lista_mod_prod->header;
      lista_mod_prod->header->back = NULL;

      free_prods (find);
    }
  else if (find == end)
    {
      find->back->next = lista_mod_prod->header;

      free_prods (find);
    }
  else
    {
      find->back->next = find->next;
      find->next->back = find->back;

      free_prods (find);
    }

  return;
}

gint
clean_lista_mod_prod (void)
{
  Prods *alter = lista_mod_prod->header;
  Prods *to_free;

  if (lista_mod_prod->header == NULL)
    return 0;

  do {
    to_free = alter;
    alter = alter->next;
    free_prods (to_free);
  } while (alter != lista_mod_prod->header);

  lista_mod_prod->header = NULL;
  lista_mod_prod->prods = NULL;
  lista_mod_prod->check = NULL;

  return 0;
}

gint
cantidad_total_prods (Prods *header)
{
  Prods *cal = header;
  int total = 0;

  if (cal == NULL)
    return total;

  do {
    total++;
    cal = cal->next;
  } while (cal != header);

  return total;
}


gint
rmv_prod_from_prod_list (gchar *codigo, gint position)
{
  Prods *find = lista_mod_prod->header;
  Prods *end = lista_mod_prod->header;

  /*
    Must be re-write it, to fix and prevent any kind of error
  */

  while (strcmp (find->prod->barcode, codigo) != 0)
    find = find->next;

  while (end->next != lista_mod_prod->header)
    end = end->next;

  if (find == lista_mod_prod->header && 
      lista_mod_prod->header->next == lista_mod_prod->header)
    {
      free_prods (find);

      lista_mod_prod->header = NULL;
      lista_mod_prod->prods = NULL;
    }
  else if (find->back == NULL)
    {
      lista_mod_prod->header = find->next;
      lista_mod_prod->header->back = NULL;
      end->next = lista_mod_prod->header;

      free_prods (find);
    }
  else if (find == end)
    {
      find->back->next = lista_mod_prod->header;
      free_prods (find);
    }
  else
    {
      find->back->next = find->next;
      find->next->back = find->back;

      free_prods (find);
    }

  lista_mod_prod->prods = lista_mod_prod->header;

  return 0;
}
