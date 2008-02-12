/*administracion_productos.h
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
*    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef INGRESO_PRODUCTOS_H

#define INGRESO_PRODUCTOS_H

void admini_box (GtkWidget *main_box);

void Ingresar_Producto (gpointer data);

void ListarProductos (GtkWidget *button, gpointer data);

gint ReturnProductsStore (GtkListStore *store);

void FillEditFields (GtkTreeSelection *selection, gpointer data);

void FillFields (GtkTreeSelection *selection, gpointer data);

void EliminarProductoDB (GtkButton *button, gpointer data);

void CloseProductWindow (void);

void SaveChanges (void);

void BuscarProductosParaListar (void);

void ModificarProducto (void);

#endif
