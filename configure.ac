# configure.in
# Copyright (C) 2008 Rizoma Tecnologia Limitada <info@rizoma.cl>

# This file is part of rizoma.

# Rizoma is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA


AC_INIT(rizoma, 1.0-git, info@rizoma.cl)
AM_INIT_AUTOMAKE(dist-bzip2)
AC_CONFIG_HEADERS(config.h)

AC_ARG_ENABLE(debug,
	     [	     --enable-debug	enable debugging information (default=no)],
	     debug=$enableval, debug=no)

if test x"$enableval" = xyes; then
   CFLAGS="-ggdb -O0 -Wall -DDEBUG"
else
   CFLAGS=""
fi

AM_CHECK_PGSQL

AC_PROG_CC

AM_PATH_GTK_2_0

AM_CHECK_PSLIB

AC_HEADER_STDC

AC_GCC_TRADITIONAL
AC_FUNC_MALLOC

AC_OUTPUT([Makefile
	   src/Makefile
	   m4/Makefile
	   db/Makefile
	   ui/Makefile
	   data/rizoma-ventas.desktop
	   data/rizoma-compras.desktop
	   data/rizoma-admin.desktop
	   data/rizoma-informes.desktop
	   data/Makefile
	   data/icons/Makefile
	   ])
