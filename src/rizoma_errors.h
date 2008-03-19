/* -*- Mode: C; tab-width: 4; ident-tabs-mode: nil; c-basic-offset: 4 -*- */
/*rizoma_errors.h
*
*    Copyright (C) 2006 Rizoma Tecnologia Limitada <info@rizoma.cl>
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

#ifndef _RIZOMA_ERRORS_H

#define _RIZOMA_ERRORS_H

typedef struct __rizoma_errors
{
  gchar *motivo;
  const gchar *funcion;
  gint type;
} RizomaErrors;

RizomaErrors *rizoma_error;

enum {APPLY, ERROR, ALERT};

gint rizoma_errors_set (gchar *error, const gchar *function, gint type);

gint rizoma_error_window (GtkWidget *widget);

#endif
