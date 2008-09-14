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

#AM_CHECK_PGSQL

AC_DEFUN([AM_CHECK_PGSQL],[

	AC_PATH_PROG(PG_CONFIG, pg_config, no)
	if test "$PG_CONFIG" = "no"; then
		AC_MSG_ERROR(Debes tener instalado pg_config)
	else
		AC_MSG_CHECKING( for PostgreSQL version >= 8.1)

		PGSQL_LIBS="-lpq -lm"
		LIBS="$LIBS $PGSQL_LIBS"
		PGSQL_CFLAGS=`$PG_CONFIG --includedir`
		PGSQL_CFLAGS="-I$PGSQL_CFLAGS"
		CFLAGS="$CFLAGS $PGSQL_CFLAGS"
		AC_MSG_RESULT(yes)
	fi
	AC_SUBST(PGSQL_LIBS)
	AC_SUBST(PGSQL_FLAGS)
])
