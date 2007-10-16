
/*datos.h
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
    {"Ventas", ventas_box, TRUE, GDK_F1},
    {"Compras", compras_box, FALSE, GDK_F2},   
    {"Informes", ventas_stats_box, FALSE, GDK_F3},
    {"Control", MainControl, FALSE, GDK_F4},
    {"Salir", Question, TRUE, GDK_F12},
  };

#endif
