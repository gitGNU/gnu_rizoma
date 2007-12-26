#AM_CHECK_PSLIB

AC_DEFUN([AM_CHECK_PSLIB],[

	AC_PATH_PROG(PKG_CONFIG, pkg-config, no)
	if test "$PKG_CONFIG" = "no"; then
		AC_MSG_ERROR(Debes tener instalado pkg-config)
	else
		AC_MSG_CHECKING( for PSLib )

		if `$PKG_CONFIG --exists libps` ; then
		   PSLIB_LIBS="-lps"
		else 
		AC_MSG_ERROR( Debes tener instalado libps )
		fi
	fi
	AC_SUBST(PSLIB_LIBS)
])
