AC_DEFUN([AM_CHECK_GNOME_UI],[
	AC_SUBST(GNOMEUI_LIBS)
	AC_SUBST(GNOMEUI_FLAGS)


	AC_PATH_PROG(PKG_CONFIG, pkg-config, no)
	if test "$PKG_CONFIG" = "no"; then
		AC_MSG_ERROR(You must have pkg-config installed)
	else
		AC_MSG_CHECKING( for GnomeUI version >= 2.2)

		if $PKG_CONFIG --print-errors --exists libgnomeui-2.0; then
			GNOMEUI_LIBS=`$PKG_CONFIG libgnomeui-2.0 --libs`
			GNOMEUI_FLAGS=`$PKG_CONFIG libgnomeui-2.0 --cflags`
			AC_MSG_RESULT(yes)
		else
			AC_MSG_ERROR(You don't have lib gnomeui >= 2.0)
		fi
	fi
])