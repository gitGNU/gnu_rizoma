/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*ventas.h
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

#ifndef VENTAS_H

#define VENTAS_H

void ventas_box (MainBox *module_box);

gboolean SearchProductByCode (void);

gboolean AgregarProducto (GtkButton *button, gpointer data);

void EliminarProducto (GtkButton *button, gpointer data);

void on_sell_button_clicked (GtkButton *button, gpointer data);

void AumentarCantidad (GtkEntry *entry, gpointer data);

void CleanSellLabels (void);

void CleanEntryAndLabelData (void);

void TipoVenta (GtkWidget *widget, gpointer data);

void CambiarTipoVenta (GtkToggleButton *button, gpointer user_data);

void CalcularVuelto (void);

gint AddProduct (void);

void LooksForClient (void);

gboolean SearchClient (void);

void WindowClientSelection (GtkWidget *widget, gpointer data);

void SeleccionarCliente (GtkWidget *nothing, gpointer data);

gint SearchBarcodeProduct (GtkWidget *widget, gpointer data);

void BuscarProducto (GtkWidget *widget, gpointer data);

void SearchAndFill (void);

void FillSellData (GtkTreeView *treeview, GtkTreePath *arg1, GtkTreeViewColumn *arg2, gpointer user_data);

void Descuento (GtkWidget *widget, gpointer data);

gchar * ModifieBarcode (gchar *barcode);

gboolean CalcularVentas (Productos *header);

void CloseChequeWindow (void);

void PagoCheque (void);

void SelectClient (GtkWidget *widget, gpointer data);

void AddDataEmisor (GtkTreeView *treeview, GtkTreePath *path, GtkTreeViewColumn *column,
		    gpointer data);

void TiposVenta (GtkWidget *widget, gpointer data);

void CancelWindow (GtkWidget *widget, gpointer data);

void WindowChangeSeller ();

void MoveFocus (GtkEntry *entry, gpointer data);

gboolean on_delete_ventas_gui (GtkWidget *widget, GdkEvent  *event, gpointer data);

void exit_response (GtkDialog *dialog, gint response_id, gpointer data);

void clean_credit_data ();

void nullify_sale_win (void);

void on_selection_nullify_sales_change (GtkTreeSelection *treeselection, gpointer data);

void close_nullify_sale_dialog(void);

#endif
