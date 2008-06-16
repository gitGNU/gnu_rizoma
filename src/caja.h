/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*caja.h
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

#ifndef CAJA_H

#define CAJA_H

/* We need to see this var from the venta_stats.c file */
GtkWidget *caja_button;

gint ArqueoCajaLastDay (void);

gint ReturnSaldoCaja (void);

void VentanaIngreso (gint monto);

void VentanaEgreso (gint monto);

void CloseVentanaEgreso (void);

void FillCajaData (void);

void CajaTab (GtkWidget *main_box);

gboolean InicializarCaja (gint monto);

gboolean CerrarCaja (gint monto);

gint CalcularPerdida (void);

gboolean check_caja (void);

void InicializarCajaWin (gint proposed_amount);

void IniciarLaCaja (GtkWidget *widget, gpointer data);

void CerrarCajaWin (void);

void CerrarLaCaja (GtkWidget *widget, gpointer data);

void prepare_caja (void);

void open_caja (gboolean automatic_mode);

gint caja_get_last_amount (void);

#endif
