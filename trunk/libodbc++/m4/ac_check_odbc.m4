AC_DEFUN(AC_CHECK_ODBC,
[
AC_LANG_SAVE
AC_LANG_C
AC_ARG_WITH(odbc,
[  --with-odbc[=DIR]       Use unixODBC, optionally installed in DIR],
[
if test "x$withval" != "xyes"
then
	odbc_dir=$withval
else
	odbc_dir="/usr/local"
fi
odbc_libraries_dir="$odbc_dir/lib"
odbc_includes_dir="$odbc_dir/include"
])

AC_ARG_WITH(odbc-includes,
[  --with-odbc-includes=DIR Find unixODBC headers in DIR],
[odbc_includes_dir=$withval]
)

AC_ARG_WITH(odbc-libraries,
[  --with-odbc-libraries=DIR Find unixODBC libraries in DIR],
[odbc_libraries_dir=$withval]
)

save_CPPFLAGS="$CPPFLAGS"
save_LIBS="$LIBS"

if test "x$odbc_includes_dir" != "x" -a "x$odbc_includes_dir" != "x/usr/include"
then
	CPPFLAGS="$CPPFLAGS -I$odbc_includes_dir"
fi

if test "x$odbc_libraries_dir" != "x"
then
	LIBS="$LIBS -L$odbc_libraries_dir"
fi

AC_CHECK_HEADERS([sql.h sqlext.h sqlucode.h],
[odbc_ok=yes; odbc_headers="$odbc_headers $ac_hdr"],
[odbc_ok=no; break]
)

if test "x$odbc_ok" = "xyes"
then
	AC_CHECK_LIB(odbc,SQLConnect,[odbc_ok=yes],[odbc_ok=no])
fi

AC_MSG_CHECKING([whether unixODBC should be used])
if test "x$odbc_ok" = "xyes"
then
	LIBS="$LIBS -lodbc"
	AC_DEFINE(HAVE_LIBODBC,,defined if using unixodbc)
	AC_DEFINE(HAVE_SQL_H,,)
	AC_DEFINE(HAVE_SQLEXT_H,,)
	AC_MSG_RESULT(yes)
else
	CPPFLAGS="$save_CPPFLAGS"
	LIBS="$save_LIBS"
	AC_MSG_RESULT(no)
fi
AC_LANG_RESTORE
])
