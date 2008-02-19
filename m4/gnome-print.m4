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

AC_DEFUN([AM_CHECK_GNOME_PRINT],[
	AC_SUBST(GNOMEPRINT_LIBS)
	AC_SUBST(GNOMEPRINT_FLAGS)


	AC_PATH_PROG(PKG_CONFIG, pkg-config, no)
	if test "$PKG_CONFIG" = "no"; then
		AC_MSG_ERROR(You must have pkg-config installed)
	else
		AC_MSG_CHECKING( for Gnome Print+ version >= 2.2)

		if $PKG_CONFIG --print-errors --exists gtk+-2.0; then
			GNOMEPRINT_LIBS=`$PKG_CONFIG libgnomeprint-2.2 --libs`
			GNOMEPRINT_FLAGS=`$PKG_CONFIG libgnomeprint-2.2 --cflags`
			LIBS="$LIBS $GNOMEPRINT_LIBS"
			CFLAGS="$CFLAGS $GNOMEPRINT_FLAGS"
			AC_MSG_RESULT(yes)
		else
			AC_MSG_ERROR(No tienes Gnome Print+ version 2.0)
		fi
	fi
])
