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
