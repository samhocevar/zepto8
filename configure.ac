dnl
dnl  Configure script for a Lol Engine project
dnl

AC_INIT([zepto8], [0.0])

dnl
dnl  Standard autoconf setup and tools requirements
dnl

AC_PREREQ(2.69)
AC_CONFIG_AUX_DIR(.auto)
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE([subdir-objects no-define tar-ustar silent-rules])

AC_PROG_CXX
LT_INIT
LT_LANG([C++])

AC_CONFIG_HEADERS([config.h])

AC_CONFIG_FILES(
 [Makefile
  src/Makefile
  t/Makefile
  utils/Makefile
  carts/Makefile
])

AC_CHECK_HEADERS(sys/select.h)

ac_cv_have_readline=no
AC_CHECK_LIB(readline, rl_callback_handler_install, [ac_cv_have_readline=yes])
AM_CONDITIONAL(HAVE_READLINE, test "${ac_cv_have_readline}" != "no")

dnl
dnl  Inherit all Lol Engine checks
dnl

LOL_AC_SUBPROJECT(src/3rdparty/lolengine)

dnl
dnl  Perform the actual commands
dnl

AC_OUTPUT

