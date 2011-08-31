/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*compras.h
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

#ifndef COMPRAS_H

#define COMPRAS_H

void SearchProductHistory (GtkEntry *entry, gchar *barcode);

void Save (GtkWidget *widget, gpointer data);

void CalcularPrecioFinal (void);

void AddToProductsList (void);

void AddNewProduct (void);

void AddNew (GtkWidget *widget, gpointer data);

void FillDataProduct (gchar *barcode);

void AddToTree (void);

void BuyWindow (void);

void SearchByName ();

void Comprar (GtkWidget *widget, gpointer data);

void ShowProductHistory (void);

void ClearAllCompraData (void);

void InsertarCompras (void);

void IngresoDetalle (GtkTreeSelection *selection, gpointer data);

void IngresarCompra (gboolean invoice, gint n_document, gchar *monto, GDate *date);

void SelectProveedores (GtkWidget *widget, gpointer data);

void FillProveedores (void);

void AddProveedor (GtkWidget *widget, gpointer data);

void AddProveedorWindow (GtkWidget *widget, gpointer user_data);

void Seleccionado (GtkTreeSelection *selection, gpointer data);

void Ingresar (GtkCellRendererToggle *cellrenderertoggle, gchar *arg1, gpointer data);

void IngresoParcial (void);

void ChangeIngreso (GtkEntry *entry, gpointer data);

void CleanStatusProduct (gint option); // 0 = todo, cualquier otro numero = parcial

void AskForCurrentPrice (gchar *barcode);

gboolean IngresarProductoSeleccionado (GtkWidget *widget, gpointer data);

void CloseDocumentoIngreso (void);

void DocumentoIngreso (void);

void DocumentoFactura (GtkWidget *widget, gpointer data);

void DocumentoGuia (GtkWidget *widget, gpointer data);

gboolean CheckDocumentData (GtkWidget *wnd, gboolean factura, gchar *rut_proveedor, gint id, gchar *n_documento, gchar *monto, GDate *date);

void FillPagarFacturas (gchar *rut_proveedor);

void FillGuias (gchar *rut_proveedor);

void FillDetGuias (GtkTreeSelection *selection, gpointer data);

void AddGuia (GtkWidget *widget, gpointer data);

void DelGuia (GtkWidget *widget, gpointer data);

void SelectProveedor (GtkWidget *widget, gpointer data);

void FoundProveedor (GtkWidget *widget, gpointer dawta);

void FillProveedorData (gchar *str_rut, gboolean guias);

void AddFactura (void);

void ClearFactData (void);

void CallBacksTabs (GtkNotebook *notebook, GtkNotebookPage *page, guint page_num, gpointer user_data);

void CalcularTotales (void);

void CalcularTotalesGuias (void);

void CheckMontoIngreso (GtkWidget *btn_ok, gint total, gint total_doc);

void CheckMontoGuias (void);

void AskIngreso ();

void SetElabVenc (GtkWidget *widget, gpointer data);

void AskElabVenc (GtkWidget *wnd, gboolean invoice);

void PagarDocumento (GtkWidget *widget, gpointer data);

void on_tree_selection_pending_guide_changed (GtkTreeSelection *selection, gpointer user_data);

void on_tree_view_invoice_list_selection_changed (GtkTreeSelection *selection, gpointer data);

void FillPartialTree (GtkTreeView *tree);

void on_partial_cell_renderer_edited (GtkCellRendererText *cell, gchar *path_string, gchar *new_amount, gpointer data);

#endif
