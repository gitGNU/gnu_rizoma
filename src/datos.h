/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*datos.h
*
*    Copyright (C) 2004,2008 Rizoma Tecnologia Limitada <info@rizoma.cl>
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

#ifndef DATOS_H

#define DATOS_H

static struct _modulos
{
  char *name;
  void (*func) (MainBox *module_box);
  gboolean normal_user;
  guint accel_key;
}
modulos[] =
  {
    {"Compras", compras_box, FALSE, GDK_F2},
    {"Control", MainControl, FALSE, GDK_F4},
    {"Salir", Question, TRUE, GDK_F12},
  };

#endif
