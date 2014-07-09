dnl $Id$
dnl config.m4 for extension wdetect

PHP_ARG_ENABLE(wdetect, [wdetect support], 
[  --enable-wdetect     Include wdetect support])

PHP_ARG_WITH(staging, [wumengchun's staging of ccpp support],
[  --with-staging=/path/to/staging/ccpp Include wumengchun's staging of ccpp])

AC_ARG_ENABLE([debug], [enable debug mode for wdetect],
  [AC_DEFINE(CS_DEBUG, 2, [debug level of wdetect: 0/1/2])
    CXX=clang++
    EXTRA_LDFLAGS="$EXTRA_LDFLAGS -g3"
    CXXFLAGS="$CXXFLAGS -g3 -O0 -DDEBUG -Wall"], 
  [AC_DEFINE(NDEBUG, , standard NDEBUG, will also disable debug for wdetect)
    CXX=g++
    EXTRA_LDFLAGS="$EXTRA_LDFLAGS -Wl,-O1"
    CXXFLAGS="$CXXFLAGS -O2 -finline -finline-small-functions -Wall -Wno-write-strings"]
)

if test "$PHP_WDETECT" != "no"; then
  dnl Write more examples of tests here...
  PHP_REQUIRE_CXX()
  AC_LANG_PUSH([C++])
  
  # --with-wdetect -> check with-path
  SEARCH_PATH=". /usr /usr/local"     # you might want to change this
  SEARCH_FOR="/src/php_wdetect.hpp"  # you most likely want to change this
  if test -r $PHP_WDETECT/$SEARCH_FOR; then # path given as parameter
    WDETECT_DIR=$PHP_WDETECT
  else # search default path list
    AC_MSG_CHECKING([for wdetect files in default path])
    for i in $SEARCH_PATH ; do
      if test -r $i/$SEARCH_FOR; then
        WDETECT_DIR=$i
        AC_MSG_RESULT(found in $i)
      fi
    done
  fi

  if test -z "$PHP_STAGING"; then
    STAGING_HEADERS="meta.hpp math.hpp"
  else
    STAGING_HEADERS="$PHP_STAGING/meta.hpp $PHP_STAGING/math.hpp"
  fi
  
  if test -z "$WDETECT_DIR"; then
    AC_MSG_RESULT([not found])
    AC_MSG_ERROR([Please reinstall the wdetect distribution])
  fi
  
  WDETECT_SOURCE_DIR=src
  if test -r $WDETECT_DIR/$WDETECT_SOURCE_DIR; then
    PHP_ADD_BUILD_DIR($WDETECT_DIR/$WDETECT_SOURCE_DIR)
  else
    WDETECT_SOURCE_DIR=$WDETECT_DIR
  fi

  # --with-wdetect -> add include path
  if test -r $WDETECT_DIR/include; then
    PHP_ADD_INCLUDE($WDETECT_DIR/include)
  fi
  
  PHP_ADD_INCLUDE(/usr/include/php5)
  
  # opencv headers and libs, currently only opencv2 is supported.
  AC_CHECK_HEADERS(
    [opencv2/core/core.hpp opencv2/imgproc/imgproc.hpp opencv2/highgui/highgui.hpp], [
    PHP_CHECK_LIBRARY(opencv_core, cvCreateImage, [
      PHP_ADD_LIBRARY_WITH_PATH(opencv_core, , WDETECT_SHARED_LIBADD)
      PHP_ADD_LIBRARY_WITH_PATH(opencv_imgproc, , WDETECT_SHARED_LIBADD)
      PHP_ADD_LIBRARY_WITH_PATH(opencv_highgui, , WDETECT_SHARED_LIBADD)
    ], [
      AC_MSG_ERROR([opencv libs not found])
    ], [-lopencv_core])], [
    AC_MSG_ERROR([header files of opencv not found])
  ])
  
  # ImageMagick headers and libs
  #AC_CHECK_HEADERS([ImageMagick/Magick++/Image.h], [
  #    PHP_ADD_INCLUDE(`Magick++-config --ldflags --libs`)
  #    AC_DEFINE(HAVE_IMAGE_MAGICK, 1, [Whether have ImageMagick support or not])
  #  ], [
  #  AC_MSG_ERROR([header files of ImageMagick not found])
  #])
  
  # wumengchun's staging of ccpp.
  #   c++ still does not know -I$PHP_STAGING...
  AC_CHECK_HEADERS([$STAGING_HEADERS], [
    PHP_ADD_INCLUDE($PHP_STAGING)], [
    AC_MSG_ERROR([wumengchun's staging of ccpp not found])
  ])
  
  # currently no way to add boost with shared-object checking...
  AC_CHECK_HEADERS([boost/filesystem/path.hpp], [
    PHP_ADD_LIBRARY_WITH_PATH(boost_system, , WDETECT_SHARED_LIBADD)
    PHP_ADD_LIBRARY_WITH_PATH(boost_filesystem, , WDETECT_SHARED_LIBADD)
    AC_DEFINE(HAVE_BOOST_FILESYSTEM, 1, [Whether have boost-filesystem or not])], [
    AC_MSG_ERROR([header files of boost-filesystem not found])
  ])
  
  AC_DEFINE(HAVE_WDETECTLIB,1,[Whether wdetect support is present and requested])
  
  EXTRA_LDFLAGS="$EXTRA_LDFLAGS -L$WDETECT_DIR/lib -lm `Magick++-config --ldflags --libs`"
  INCLUDES="$INCLUDES `Magick++-config --cppflags --cxxflags`"
  CXXFLAGS="$CFLAGS $CXXFLAGS"
  # CPPFILES="$WDETECT_SOURCE_DIR/wdetect.cpp 
  #   $WDETECT_SOURCE_DIR/facade.cpp 
  #   $WDETECT_SOURCE_DIR/utility.cpp 
  #   $WDETECT_SOURCE_DIR/preprocer.cpp 
  #   $WDETECT_SOURCE_DIR/locater.cpp
  #   $WDETECT_SOURCE_DIR/recognizer.cpp
  #   $WDETECT_SOURCE_DIR/chartdetecter.cpp
  #   $WDETECT_SOURCE_DIR/textdetecter.cpp"
  CPPFILES="$WDETECT_SOURCE_DIR/wdetect.cpp
    $WDETECT_SOURCE_DIR/detecter.cpp
    $WDETECT_SOURCE_DIR/preprocer.cpp
    $WDETECT_SOURCE_DIR/locater.cpp
    $WDETECT_SOURCE_DIR/recognizer.cpp"
  PHP_NEW_EXTENSION(wdetect, $CPPFILES, $ext_shared, , $CXXFLAGS, "yes")
  PHP_SUBST(WDETECT_SHARED_LIBADD)
fi
