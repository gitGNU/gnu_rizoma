/*manejo_productos.c
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
#include<string.h>
#include<stdlib.h>
#include<math.h>

#include"tipos.h"
#include"postgres-functions.h"

Productos *
CreateNew (gchar *barcode, gdouble cantidad)
{
  Productos *new = NULL;
  //  PGresult *res = EjecutarSQL (g_strdup_printf ("SELECT productos* FROM producto WHERE barcode='%s'", barcode));
  PGresult *res = EjecutarSQL
    (g_strdup_printf
     ("SELECT codigo, barcode, descripcion, marca, contenido, unidad, precio, fifo, margen_promedio, (SELECT monto FROM impuestos WHERE id=0 AND "
      "productos.impuestos='t'), (SELECT monto FROM impuestos WHERE id=productos.otros), canje , stock_pro, precio_mayor, cantidad_mayor, mayorista FROM producto WHERE barcode='%s'", barcode));

  new = (Productos *) g_malloc (sizeof (Productos));

  new->product = (Producto *) g_malloc (sizeof (Producto));

  new->product->cantidad = cantidad;
  new->product->codigo = PQgetvalue (res, 0, 0);
  new->product->barcode = PQgetvalue (res, 0, 1);
  new->product->producto = PQgetvalue (res, 0, 2);
  new->product->marca = PQgetvalue (res, 0, 3);
  new->product->contenido = atoi (PQgetvalue (res, 0, 4));
  new->product->unidad = PQgetvalue (res, 0, 5);
  new->product->precio = atoi (PQgetvalue (res, 0, 6));
  new->product->fifo = atoi (PQgetvalue (res, 0, 7));
  new->product->precio_compra = GetNeto (barcode);
  new->product->iva = atoi (PQgetvalue (res, 0, 9));
  new->product->otros = atoi (PQgetvalue (res, 0, 10));
  new->product->margen = atoi (PQgetvalue (res, 0, 8));
  new->product->canjeable = atoi (PQgetvalue (res, 0, 11));
  new->product->stock_pro = strtod (PUT (PQgetvalue (res, 0, 12)), (char **) NULL);
  /* Datos Mayoristas */
  new->product->precio_mayor = atoi (PQgetvalue (res, 0, 13));
  new->product->cantidad_mayorista = atoi (PQgetvalue (res, 0, 14));
  new->product->mayorista = strcmp (PQgetvalue (res, 0, 15), "t") == 0 ? TRUE : FALSE;

  return new;
}

void
FreeProduct (Productos *productos)
{
  /*
  Producto *producto = productos->product;

  free (producto->codigo);

  g_free (producto->producto);
  g_free (producto->marca);
  g_free (producto->unidad);
  g_free (producto->barcode);
  g_free (producto);

  g_free (productos->product);
  g_free (productos);
  */
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

/*gchar *
ReturnAllProductsCode (Productos *header)
{
  Productos *cal = header;
  gchar *codes = (gchar *) g_malloc0 (600);
  gchar *alter = (gchar *) g_malloc0 (600);

  do
    {
      if (strcmp (codes, "") == 0)
	{
	  g_snprintf (codes, 600, "%s_% ", cal->product->codigo, cal->product->cantidad);
	  g_snprintf (alter, 600, "%s", codes);
	}
      else
	{
	  g_snprintf (codes, 600, "%s%s_%d ", alter, cal->product->codigo, cal->product->cantidad);
	  g_snprintf (alter, 600, "%s", codes);
	}


      cal = cal->next;
    }
  while (cal != header);

  return codes;
}
*/
gint
ListClean (void)
{
  Productos *alter = venta->header;
  Productos *tofree;
  gint total = ReturnTotalProducts (venta->header);
  gint i;


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

    g_free (alter);

    alter = tofree;

  } while (alter != compra->header);

  compra->header = NULL;
  compra->products_list = NULL;
  compra->current = NULL;

  return 0;
}

Productos *
CompraCreateNew (gchar *barcode, double cantidad, gint precio_final, gdouble precio_compra, gint margen)
{
  Productos *new = NULL;
  PGresult *res = EjecutarSQL
    (g_strdup_printf ("SELECT codigo, barcode, descripcion, marca, contenido, unidad, perecibles, canje, stock_pro, tasa_canje, precio_mayor, cantidad_mayor, mayorista FROM producto "
		      "WHERE barcode='%s'", barcode));

  new = (Productos *) g_malloc (sizeof (Productos));

  new->product = (Producto *) g_malloc (sizeof (Producto));

  new->product->cantidad = cantidad;
  new->product->codigo = g_strdup (PQgetvalue (res, 0, 0));
  new->product->barcode = PQgetvalue (res, 0, 1);
  new->product->producto = PQgetvalue (res, 0, 2);
  new->product->marca = PQgetvalue (res, 0, 3);
  new->product->contenido = atoi (PQgetvalue (res, 0, 4));
  new->product->unidad = PQgetvalue (res, 0, 5);
  //  new->product->precio = atoi (PQgetvalue (res, 0, 8));
  new->product->margen = margen;
  new->product->iva = GetIVA (barcode);
  new->product->otros = GetOtros (barcode);
  new->product->precio = precio_final;
  new->product->precio_neto = (double)precio_compra * ((double)(margen / 100) + 1);
  new->product->precio_compra = precio_compra;
  new->product->ingresar = TRUE;
  new->product->perecible = strcmp (PQgetvalue (res, 0, 6), "t") ? FALSE : TRUE;
  new->product->canjeable = strcmp (PQgetvalue (res, 0, 7), "t") ? FALSE : TRUE;
  new->product->stock_pro = strtod (PUT(PQgetvalue (res, 0, 8)), (char **) NULL);
  new->product->tasa_canje = strtod (PUT(PQgetvalue (res, 0, 9)), (char **) NULL);
  /* Datos Mayoristas */
  new->product->precio_mayor = atoi (PQgetvalue (res, 0, 10));
  new->product->cantidad_mayorista = atoi (PQgetvalue (res, 0, 11));
  new->product->mayorista = atoi (PQgetvalue (res, 0, 12));

  new->product->canjear = FALSE;

  return new;
}

gint
CompraAgregarALista (gchar *barcode, gdouble cantidad, gint precio_final, gdouble precio_compra,
		     gint margen, gboolean ingreso)
{
  Productos *new = NULL;

  if (ingreso == TRUE)
    {
      if (compra->header == NULL)
	{
	  new = CompraCreateNew (barcode, cantidad, precio_final, precio_compra, margen);
	  compra->header = new;

	  compra->products_list = new;

	  new->back = NULL;
	  new->next = compra->header;
	}
      else
	{
	  Productos *end = compra->header;

	  new = CompraCreateNew (barcode, cantidad, precio_final, precio_compra, margen);

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

	  while (end->next != compra->header_compra)
	    end = end->next;

	  new->back = end;

	  end->next = new;

	  new->next = compra->header_compra;

	  compra->products_compra = new;
	  compra->current = new->product;
	}
    }

  return 0;
}

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
  guint64 total = 0;

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
