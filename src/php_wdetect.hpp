/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2013 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author:                                                              |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifndef PHP_WDETECT_H
#define PHP_WDETECT_H

#ifdef HAVE_CONFIG_H
extern "C" {
#   include "config.h"
}
#endif

#include "predef.hpp"
extern "C" {
#include "php5/main/php.h"
#include "php5/main/php_ini.h"
#include "php5/ext/standard/info.h"
}

extern "C" zend_module_entry wdetect_module_entry;
#define phpext_wdetect_ptr &wdetect_module_entry

#ifdef PHP_WIN32
#	define PHP_WDETECT_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#	define PHP_WDETECT_API __attribute__ ((visibility("default")))
#else
#	define PHP_WDETECT_API
#endif

#ifdef ZTS
BEGIN_EXTERN_C()
#include "php5/TSRM/TSRM.h"
}
END_EXTERN_C()
#endif

PHP_MINIT_FUNCTION(wdetect);
//PHP_MSHUTDOWN_FUNCTION(wdetect);
//PHP_RINIT_FUNCTION(wdetect);
//PHP_RSHUTDOWN_FUNCTION(wdetect);
PHP_MINFO_FUNCTION(wdetect);

/* In every utility function you add that needs to use variables 
   in php_wdetect_globals, call TSRMLS_FETCH(); after declaring other 
   variables used by that function, or better yet, pass in TSRMLS_CC
   after the last function argument and declare your utility function
   with TSRMLS_DC after the last declared argument.  Always refer to
   the globals in your function as WDETECT_G(variable).  You are 
   encouraged to rename these macros something shorter, see
   examples in any other php module directory.
*/

#ifdef ZTS
#   define WDETECT_G(v) TSRMG(wdetect_globals_id, zend_wdetect_globals *, v)
#else
#   define WDETECT_G(v) (wdetect_globals.v)
#endif

PHP_METHOD(WDetecter, __construct);
PHP_METHOD(WDetecter, __destruct);
PHP_METHOD(WDetecter, prepare);
PHP_METHOD(WDetecter, locate);
PHP_METHOD(WDetecter, setOrigin);
PHP_METHOD(WDetecter, detect);

#endif	/* PHP_WDETECT_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
