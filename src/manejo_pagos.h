/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*manejo_pagos.h
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

#ifndef MANEJO_PAGOS_H

#define MANEJO_PAGOS_H

ChequesRestaurant * create_new_restaurant_check (gchar *codigo, gchar *fecha_venc, gint monto);
void free_chk_rest (ChequesRestaurant *cheques_restaurant);
gint add_chk_rest_to_list (gchar *codigo, gchar *fecha_venc, gint monto);
gint del_chk_rest_from_list (gchar *codigo);
gdouble calcular_total_cheques (ChequesRestaurant *header);
gint cantidad_cheques (ChequesRestaurant *header);
gint limpiar_lista (void);

#endif
