#AM_CHECK_PGSQL

AC_DEFUN([AM_CHECK_PGSQL],[

	AC_PATH_PROG(PG_CONFIG, pg_config, no)
	if test "$PG_CONFIG" = "no"; then
		AC_MSG_ERROR(Debes tener instalado pg_config)
	else
		AC_MSG_CHECKING( for PostgreSQL version >= 8.1)

		PGSQL_LIBS="-lpq -lcrypt -lm"
		LIBS="$LIBS $PGSQL_LIBS"
		PGSQL_CFLAGS=`$PG_CONFIG --includedir`
		PGSQL_CFLAGS="-I$PGSQL_CFLAGS"
		CFLAGS="$CFLAGS $PGSQL_CFLAGS"
		AC_MSG_RESULT(yes)		
	fi
	AC_SUBST(PGSQL_LIBS)
	AC_SUBST(PGSQL_FLAGS)
])
