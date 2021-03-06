/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*manejo_productos.h
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

#include "tipos.h"

#ifndef MANEJO_PRODUCTOS_H

#define MANEJO_PRODUCTOS_H

gint AgregarALista (gchar *codigo, gchar *barcode, gdouble cantidad);

gint EliminarDeLista (gchar *codigo, gint position);

gdouble CalcularTotal (Productos *header);

gint ReturnTotalProducts (Productos *header);

gchar * ReturnAllProductsCode (Productos *header);

gint ListClean (void);

gint CompraListClean (void);

gboolean CompraAgregarALista (gchar *barcode, gdouble cantidad, gdouble precio_final, gdouble precio_compra,
                              gdouble margen, gboolean ingreso);

void DropBuyProduct (gchar *codigo);

Producto * SearchProductByBarcode (gchar *barcode, gboolean ingreso);

void SetCurrentProductTo (gchar *barcode, gboolean ingreso);

Productos * BuscarPorCodigo (Productos *products, gchar *code);

gdouble CalcularTotalCompra (Productos *header);

gboolean LookCanjeable (Productos *header);

gdouble CalcularSoloAfecto (Productos *header);

gdouble CalcularNetoSoloAfecto (Productos *header);

gdouble CalcularSoloNoAfecto (Productos *header);

gdouble CalcularTotalProporcionAfecta (Productos *header);

gdouble CalcularTotalProporcionNoAfecta (Productos *header);

Productos * CreateNew (gchar *barcode, gdouble cantidad);

Prods * create_prod (gchar *barcode, gboolean con_costo);

gint add_to_mod_prod_list (gchar *code, gboolean con_costo);

Prods * buscar_prod (Prods *header, gchar *barcode);

void free_prods (Prods *prods);

void drop_prod_to_mod_list (gchar *barcode);

gint clean_lista_mod_prod (void);

gint cantidad_total_prods (Prods *header);

gint rmv_prod_from_prod_list (gchar *codigo, gint position);

#endif
