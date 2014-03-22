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

#include "php_wdetect.hpp"
#include "wdetect_impl.hpp"
#include "facade.hpp"
#include "detecter.hpp"

zend_class_entry* wdetect_class_entry;

zend_module_entry wdetect_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
	STANDARD_MODULE_HEADER,
#endif
	"wdetect",
	NULL,
	PHP_MINIT(wdetect),
	NULL,
	NULL,
	NULL,
	PHP_MINFO(wdetect),
#if ZEND_MODULE_API_NO >= 20010901
	"0.1", /* Replace with version number for your extension */
#endif
	STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_WDETECT
BEGIN_EXTERN_C()
ZEND_GET_MODULE(wdetect)
END_EXTERN_C()
#endif

function_entry wdetecter_methods[] = {
    PHP_ME(WDetecter, __construct, NULL, ZEND_ACC_PUBLIC | ZEND_ACC_CTOR)
    PHP_ME(WDetecter, __destruct,  NULL, ZEND_ACC_PUBLIC | ZEND_ACC_DTOR)
    PHP_ME(WDetecter, prepare,     NULL, ZEND_ACC_PUBLIC)
    PHP_ME(WDetecter, scale,       NULL, ZEND_ACC_PUBLIC)
    PHP_ME(WDetecter, setOrigin,   NULL, ZEND_ACC_PUBLIC)
    PHP_ME(WDetecter, locate,      NULL, ZEND_ACC_PUBLIC)
    PHP_ME(WDetecter, detect,      NULL, ZEND_ACC_PUBLIC)
    {NULL, NULL, NULL}
};

#define _WDT_CLASS_CONST_LONG(NAME, VALUE)  \
    zend_declare_class_constant_long(wdetect_class_entry, NAME, CS_CONST_STRLEN(NAME), VALUE);

PHP_MINIT_FUNCTION(wdetect)
{
	{
        zend_class_entry class_entry;
        INIT_CLASS_ENTRY(class_entry, "WDetecter", wdetecter_methods);
        wdetect_class_entry = zend_register_internal_class(&class_entry TSRMLS_CC);

        wdetect_class_entry->create_object = create_detecter;
        memcpy(&detecter_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
        detecter_object_handlers.clone_obj = NULL;
	}

	_WDT_CLASS_CONST_LONG("CMD_LOCATE_CHART", wdt::locate_chart);

	_WDT_CLASS_CONST_LONG("CMD_DETECT_INTEGER", wdt::detect_integer);
	_WDT_CLASS_CONST_LONG("CMD_DETECT_PERCENT", wdt::detect_percent);
	_WDT_CLASS_CONST_LONG("CMD_DETECT_TEXT_WIDTH", wdt::detect_percent);

	_WDT_CLASS_CONST_LONG("OK", wdt::success);
	_WDT_CLASS_CONST_LONG("ERR_UNKNOWN", wdt::failure);
	_WDT_CLASS_CONST_LONG("ERR_WRONG_PARAM", wdt::fo_wrong_param);

	_WDT_CLASS_CONST_LONG("ERR_IMG_FILE_NONEXISTS", wdt::fo_img_file_nonexists);
	_WDT_CLASS_CONST_LONG("ERR_IMG_FILE_UNREADABLE", wdt::fo_img_file_unreadable);
	_WDT_CLASS_CONST_LONG("ERR_IMG_FILE_EXT", wdt::fo_img_file_ext);
	_WDT_CLASS_CONST_LONG("ERR_IMG_FILE_SIZE", wdt::fo_img_file_size);
	_WDT_CLASS_CONST_LONG("ERR_IMG_TYPE", wdt::fo_img_type);
	_WDT_CLASS_CONST_LONG("ERR_IMG_CONTENT", wdt::fo_img_content);
	_WDT_CLASS_CONST_LONG("ERR_IMG_SIZE", wdt::fo_img_size);

	_WDT_CLASS_CONST_LONG("ERR_NO_MATCH", wdt::fo_no_match);
	_WDT_CLASS_CONST_LONG("ERR_DETECT_FAILED", wdt::fo_detect);
	_WDT_CLASS_CONST_LONG("ERR_RECOGNIZE_FAILED", wdt::fo_recognize);

	_WDT_CLASS_CONST_LONG("ERR_CALC_CHART_WIDTH", wdt::fo_calc_chart_width);

	return SUCCESS;
}

#undef _WDT_CLASS_CONST_LONG

PHP_MINFO_FUNCTION(wdetect)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "wdetect support", "enabled");
	php_info_print_table_end();
}
/* }}} */

PHP_METHOD(WDetecter, __construct)
{
    zval* obj = getThis();
    reinterpret_cast<DetecterObject*>(zend_object_store_get_object(obj TSRMLS_CC))->dectecter = new wdt::Detecter;
}

PHP_METHOD(WDetecter, __destruct)
{
}

PHP_METHOD(WDetecter, prepare)
{
    zval* options;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "a", &options) == FAILURE)
    {
        php_error_docref(NULL TSRMLS_CC, E_ERROR, "<Wdetecter>.setOrigin requires one arguments to be array.");
        RETURN_FALSE;
    }

    wdt::Detecter* detecter = reinterpret_cast<DetecterObject*>(
        zend_object_store_get_object(getThis() TSRMLS_CC))->dectecter;
    if (detecter == NULL)
    {
        RETURN_FALSE;
    }

    Proxy::prepare(detecter, options, return_value);
}

PHP_METHOD(WDetecter, scale)
{
    double rate;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "d", &rate) == FAILURE)
    {
        php_error_docref(NULL TSRMLS_CC, E_ERROR, "<Wdetecter>.scale requires one arguments to be double.");
        RETURN_FALSE;
    }

    wdt::Detecter* detecter = reinterpret_cast<DetecterObject*>(
        zend_object_store_get_object(getThis() TSRMLS_CC))->dectecter;
    if (detecter == NULL)
    {
        RETURN_FALSE;
    }

    Proxy::scale(detecter, rate, return_value);
}

PHP_METHOD(WDetecter, locate)
{
    zval* options;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "a", &options) == FAILURE)
    {
        RETURN_FALSE;
    }

    wdt::Detecter* detecter = reinterpret_cast<DetecterObject*>(
        zend_object_store_get_object(getThis() TSRMLS_CC))->dectecter;
    if (detecter == NULL)
    {
        RETURN_FALSE;
    }

    Proxy::locate(detecter, options, return_value);
}

PHP_METHOD(WDetecter, setOrigin)
{
    wdt::isize_t left, top;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ll", &left, &top) == FAILURE)
    {
        RETURN_FALSE;
    }

    wdt::Detecter* detecter = reinterpret_cast<DetecterObject*>(
        zend_object_store_get_object(getThis() TSRMLS_CC))->dectecter;
    if (detecter == NULL)
    {
        RETURN_FALSE;
    }

    Proxy::set_origin(detecter, left, top);
    RETURN_TRUE;
}

PHP_METHOD(WDetecter, detect)
{
    zval* options;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "a", &options) == FAILURE)
    {
        RETURN_FALSE;
    }

    wdt::Detecter* detecter = reinterpret_cast<DetecterObject*>(
        zend_object_store_get_object(getThis() TSRMLS_CC))->dectecter;
    if (detecter == NULL)
    {
        RETURN_FALSE;
    }

    Proxy::detect(detecter, options, return_value);
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
