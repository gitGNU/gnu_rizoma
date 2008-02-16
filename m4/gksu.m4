#AM_CHECK_GKSU

AC_DEFUN([AM_CHECK_GKSU],[

	AC_PATH_PROG(PKG_CONFIG, pkg-config, no)
	if test "$PKG_CONFIG" = "no"; then
		AC_MSG_ERROR(Debes tener instalado pkg-config)
	else
		AC_MSG_CHECKING( for libgksu >= 2 )

		if `$PKG_CONFIG --exists libgksu2` ; then
		   GKSU_LIBS=`$PKG_CONFIG --libs libgksu2`
		   GKSU_CFLAGS=`$PKG_CONFIG --cflags libgksu2`
		else 
		AC_MSG_ERROR( Debes tener instalado libgksu >= 2.0 )
		fi
	fi
	AC_SUBST(GKSU_LIBS)
	AC_SUBST(GKSU_CFLAGS)
])
