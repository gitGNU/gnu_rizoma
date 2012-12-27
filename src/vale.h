/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*vale.c
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
*    You should have received a 1copy of the GNU General Public License
*    along with this program; if not, write to the Free Software
*    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef VALE_H

#define VALE_H

int PrintVale (Productos *header, gint venta_id, gchar *rut_cliente, gint boleta, gint total, gint tipo_pago, gint tipo_documento);
void PrintValeContinuo (Productos *header, gint venta_id, gchar *rut_cliente, gint boleta, gint total,
                        gint tipo_pago, gint tipo_documento, Productos *prod);
void PrintValeMesa (Productos *header, gint num_mesa);
void print_cash_box_info (gint cash_id, gint monto_ingreso, gint monto_egreso, gchar *motivo);
void PrintValeTraspaso (Productos *header, gint traspaso_id, gboolean traspaso_envio, gchar *origen, gchar *destino);
void PrintValeCompra (Productos *header, gint compra_id, gint n_document, gchar *nombre_proveedor);
void abrirGaveta (void);

#endif
