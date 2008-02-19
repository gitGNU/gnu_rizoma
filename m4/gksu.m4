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

#AM_CHECK_GKSU

AC_DEFUN([AM_CHECK_GKSU],[

	AC_PATH_PROG(PKG_CONFIG, pkg-config, no)
	if test "$PKG_CONFIG" = "no"; then
		AC_MSG_ERROR(Debes tener instalado pkg-config)
	fi

	AC_MSG_CHECKING( for libgksu >= 2 )

	if `$PKG_CONFIG --exists libgksu2` ; then
	   GKSU_VERSION=`$PKG_CONFIG --modversion libgksu2`
	   GKSU_LIBS=`$PKG_CONFIG --libs libgksu2`
	   GKSU_CFLAGS=`$PKG_CONFIG --cflags libgksu2`

	   AC_MSG_RESULT( yes (version $GKSU_VERSION));
	else
	AC_MSG_ERROR( Debes tener instalado libgksu >= 2.0 )
	fi

	AC_SUBST(GKSU_LIBS)
	AC_SUBST(GKSU_CFLAGS)
])
