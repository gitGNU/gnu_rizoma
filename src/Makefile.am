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

bin_PROGRAMS = rizoma-config rizoma-ventas rizoma-compras

rizoma_admin_SOURCES = main.c tipos.h main.h \
datos.h dimentions.h \
postgres-functions.c postgres-functions.h \
errors.c errors.h \
boleta.c boleta.h \
credito.c credito.h \
encriptar.c encriptar.h \
usuario.c usuario.h \
proveedores.c proveedores.h \
control.c control.h \
impuestos.c impuestos.h \
datos_negocio.c datos_negocio.h \
config_file.c config_file.h \
parametros.c parametros.h \
rizoma_errors.c rizoma_errors.h \
utils.c utils.h

rizoma_ventas_SOURCES = ventas.c ventas.h  dimentions.h tipos.h \
credito.c credito.h \
postgres-functions.c postgres-functions.h \
errors.c errors.h \
manejo_productos.c manejo_productos.h \
boleta.c boleta.h \
datos_negocio.c datos_negocio.h \
printing.c printing.h \
config_file.c config_file.h \
parametros.c parametros.h \
rizoma_errors.c rizoma_errors.h \
utils.c utils.h \
factura_more.c factura_more.h \
caja.c caja.h \
vale.c vale.h \
tiempo.c tiempo.h \
encriptar.c encriptar.h \
proveedores.c proveedores.h

rizoma_compras_SOURCES = compras.c compras.h \
administracion_productos.c administracion_productos.h \
postgres-functions.c postgres-functions.h \
errors.c errors.h \
manejo_productos.c manejo_productos.h \
encriptar.c encriptar.h \
usuario.c usuario.h \
proveedores.c proveedores.h \
impuestos.c impuestos.h \
config_file.c config_file.h \
rizoma_errors.c rizoma_errors.h \
credito.c credito.h \
printing.c printing.h \
boleta.c boleta.h \
vale.c vale.h \
tiempo.c tiempo.h \
utils.c utils.h \
factura_more.c factura_more.h

rizoma_config_SOURCES = rizoma-config.c

rizoma_admin_LDADD = $(GTK_LIBS) $(PSLIB_LIBS) $(GKSU_LIBS)
rizoma_admin_LDFLAGS = -Wl,--export-dynamic
INCLUDES = $(CFLAGS) $(PGSQL_CFLAGS) $(GTK_CFLAGS) $(GKSU_CFLAGS) -DDATADIR=\"$(pkgdatadir)\"

rizoma_ventas_LDADD = $(GTK_LIBS) $(PSLIB_LIBS) $(GKSU_LIBS)
rizoma_ventas_LDFLAGS = -Wl,--export-dynamic
INCLUDES = $(CFLAGS) $(PGSQL_CFLAGS) $(GTK_CFLAGS) $(GKSU_CFLAGS) -DDATADIR=\"$(pkgdatadir)\"

rizoma_compras_LDADD = $(GTK_LIBS) $(PSLIB_LIBS) $(GKSU_LIBS)
rizoma_compras_LDFLAGS = -Wl,--export-dynamic
INCLUDES = $(CFLAGS) $(PGSQL_CFLAGS) $(GTK_CFLAGS) $(GKSU_CFLAGS) -DDATADIR=\"$(pkgdatadir)\"

rizoma_config_LDADD = $(CONFUSE_LIBS) $(GTK_LIBS) $(GKSU_LIBS)
rizoma_config_LDFLAGS = -Wl,--export-dynamic
INCLUDES = $(CONFUSE_FLAGS) $(GTK_CFLAGS) $(GKSU_CFLAGS) -DDATADIR=\"$(pkgdatadir)\"

EXTRA_DIST = ChangeLog