dnl $Id$
dnl config.m4 for extension wdetect

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

dnl PHP_ARG_WITH(wdetect, for wdetect support,
dnl Make sure that the comment is aligned:
dnl [  --with-wdetect             Include wdetect support])

dnl Otherwise use enable:

dnl PHP_ARG_ENABLE(wdetect, whether to enable wdetect support,
dnl Make sure that the comment is aligned:
dnl [  --enable-wdetect           Enable wdetect support])

if test "$PHP_WDETECT" != "no"; then
  dnl Write more examples of tests here...

  dnl # --with-wdetect -> check with-path
  dnl SEARCH_PATH="/usr/local /usr"     # you might want to change this
  dnl SEARCH_FOR="/include/wdetect.h"  # you most likely want to change this
  dnl if test -r $PHP_WDETECT/$SEARCH_FOR; then # path given as parameter
  dnl   WDETECT_DIR=$PHP_WDETECT
  dnl else # search default path list
  dnl   AC_MSG_CHECKING([for wdetect files in default path])
  dnl   for i in $SEARCH_PATH ; do
  dnl     if test -r $i/$SEARCH_FOR; then
  dnl       WDETECT_DIR=$i
  dnl       AC_MSG_RESULT(found in $i)
  dnl     fi
  dnl   done
  dnl fi
  dnl
  dnl if test -z "$WDETECT_DIR"; then
  dnl   AC_MSG_RESULT([not found])
  dnl   AC_MSG_ERROR([Please reinstall the wdetect distribution])
  dnl fi

  dnl # --with-wdetect -> add include path
  dnl PHP_ADD_INCLUDE($WDETECT_DIR/include)

  dnl # --with-wdetect -> check for lib and symbol presence
  dnl LIBNAME=wdetect # you may want to change this
  dnl LIBSYMBOL=wdetect # you most likely want to change this 

  dnl PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  dnl [
  dnl   PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $WDETECT_DIR/lib, WDETECT_SHARED_LIBADD)
  dnl   AC_DEFINE(HAVE_WDETECTLIB,1,[ ])
  dnl ],[
  dnl   AC_MSG_ERROR([wrong wdetect lib version or lib not found])
  dnl ],[
  dnl   -L$WDETECT_DIR/lib -lm
  dnl ])
  dnl
  dnl PHP_SUBST(WDETECT_SHARED_LIBADD)

  PHP_NEW_EXTENSION(wdetect, wdetect.c, $ext_shared)
fi
