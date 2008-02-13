#Makefile.am
#
#    Copyright (C) 2004 Rizoma Tecnologia Limitada <info@rizoma.cl>
#
#    This file is part of rizoma.
#
#    Rizoma is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program; if not, write to the Free Software
#    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

bin_PROGRAMS = rizoma rizoma-config fakebuy

rizoma_SOURCES = main.c tipos.h main.h datos.h dimentions.h \
ventas.c ventas.h \
administracion_productos.c administracion_productos.h \
postgres-functions.c postgres-functions.h \
errors.c errors.h \
manejo_productos.c manejo_productos.h \
ventas_stats.c ventas_stats.h \
credito.c credito.h \
encriptar.c encriptar.h \
usuario.c usuario.h \
compras.c compras.h \
boleta.c boleta.h \
caja.c caja.h \
tiempo.c tiempo.h \
proveedores.c proveedores.h \
control.c control.h \
impuestos.c impuestos.h \
datos_negocio.c datos_negocio.h \
printing.c printing.h \
config_file.c config_file.h \
parametros.c parametros.h \
factura_more.c factura_more.h \
comisiones.c comisiones.h \
vale.c vale.h \
rizoma_errors.c rizoma_errors.h

fakebuy_SOURCES = fake_buy.c \
ventas.c ventas.h \
administracion_productos.c administracion_productos.h \
postgres-functions.c postgres-functions.h \
errors.c errors.h \
manejo_productos.c manejo_productos.h \
ventas_stats.c ventas_stats.h \
credito.c credito.h \
encriptar.c encriptar.h \
usuario.c usuario.h \
compras.c compras.h \
boleta.c boleta.h \
caja.c caja.h \
tiempo.c tiempo.h \
proveedores.c proveedores.h \
control.c control.h \
impuestos.c impuestos.h \
datos_negocio.c datos_negocio.h \
printing.c printing.h \
config_file.c config_file.h \
parametros.c parametros.h \
factura_more.c factura_more.h \
comisiones.c comisiones.h \
vale.c vale.h \
rizoma_errors.c rizoma_errors.h

rizoma_config_SOURCES = rizoma-config.c

rizoma_LDADD = $(GTK_LIBS) $(PSLIB_LIBS)
INCLUDES = $(CFLAGS) $(PGSQL_CFLAGS) $(GTK_CFLAGS)

fakebuy_LDADD = $(GTK_LIBS) $(PSLIB_LIBS)
INCLUDES = $(CFLAGS) $(PGSQL_CFLAGS) $(GTK_CFLAGS)

rizoma_config_LDADD = $(CONFUSE_LIBS) $(GTK_LIBS) $(GKSU_LIBS)
rizoma_config_LDFLAGS = -Wl,--export-dynamic
INCLUDES = $(CONFUSE_FLAGS) $(GTK_CFLAGS) $(GKSU_CFLAGS) -DDATADIR=\"$(pkgdatadir)\"

EXTRA_DIST = ChangeLog