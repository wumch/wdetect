
#pragma once

#include "php_wdetect.hpp"
#include <cstring>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/tuple/to_seq.hpp>
#include "misc.hpp"
#include "facade.hpp"
#include "detecter.hpp"

typedef struct {
    zend_object std;
    wdt::Detecter* dectecter;
} DetecterObject;

zend_object_handlers detecter_object_handlers;

#define _WDT_TUPLE_TO_BP_SEQ(...) BOOST_PP_TUPLE_TO_SEQ(CS_PP_NARG(__VA_ARGS__), (__VA_ARGS__))

// NOTE: make sure really assigned to `DEST`.
#define _WDT_GET_OPT_LONG(SRC, DEST, MED, NAME) \
if (zend_hash_find(Z_ARRVAL_P(SRC), CS_STRINGIZE(NAME), CS_CONST_STRLEN(CS_STRINGIZE(NAME)) + 1, reinterpret_cast<void**>(&MED)) == SUCCESS)    \
{                                           \
    convert_to_long_ex(MED);                \
    DEST = Z_LVAL_PP(MED);                  \
}

#define _WDT_FETCH_OPT_LONG(SRC, DEST, MED, NAME) \
if (zend_hash_find(Z_ARRVAL_P(SRC), CS_STRINGIZE(NAME), CS_CONST_STRLEN(CS_STRINGIZE(NAME)) + 1, reinterpret_cast<void**>(&MED)) == SUCCESS)    \
{                                           \
    convert_to_long_ex(MED);                \
    DEST.NAME = Z_LVAL_PP(MED);             \
    CS_DUMP(DEST.NAME);                     \
}

#define _WDT_FETCH_OPT_LONG_OP(_USELESS, _TUPLE, NAME)  \
    _WDT_FETCH_OPT_LONG(BOOST_PP_TUPLE_ELEM(3, 0, _TUPLE), BOOST_PP_TUPLE_ELEM(3, 1, _TUPLE), BOOST_PP_TUPLE_ELEM(3, 2, _TUPLE), NAME)

#define _WDT_FETCH_LONG_OPTS(SRC, DEST, MED, ...)   \
    BOOST_PP_SEQ_FOR_EACH(_WDT_FETCH_OPT_LONG_OP, (SRC, DEST, MED), _WDT_TUPLE_TO_BP_SEQ(__VA_ARGS__))

void delete_detecter(void *obj TSRMLS_DC)
{
    DetecterObject* detecter_obj = reinterpret_cast<DetecterObject*>(obj);
    delete detecter_obj->dectecter;

    zend_hash_destroy(detecter_obj->std.properties);
    FREE_HASHTABLE(detecter_obj->std.properties);
    efree(detecter_obj);
}

zend_object_value create_detecter(zend_class_entry* type TSRMLS_DC)
{
    zval* tmp;
    zend_object_value retval;

    DetecterObject* detecter_obj = reinterpret_cast<DetecterObject*>(emalloc(sizeof(DetecterObject)));
    memset(detecter_obj, 0, sizeof(DetecterObject));
    detecter_obj->std.ce = type;

    ALLOC_HASHTABLE(detecter_obj->std.properties);
    zend_hash_init(detecter_obj->std.properties, 0, NULL, ZVAL_PTR_DTOR, 0);
    zend_hash_copy(detecter_obj->std.properties, &type->default_properties,
        reinterpret_cast<copy_ctor_func_t>(zval_add_ref), static_cast<void*>(&tmp), sizeof(zval*));

    retval.handle = zend_objects_store_put(detecter_obj, NULL, delete_detecter, NULL TSRMLS_CC);
    retval.handlers = &detecter_object_handlers;

    return retval;
}

class Proxy
{
public:
    static void prepare(wdt::Detecter* detecter, const zval* options, zval* return_value);

    static void scale(wdt::Detecter* detecter, const double rate, zval* return_value);

    static void locate(wdt::Detecter* detecter, const zval* options, zval* return_value);

    static void set_origin(wdt::Detecter* detecter, wdt::isize_t left, wdt::isize_t top);

    static void detect(wdt::Detecter* detecter, const zval* options, zval* return_value);

protected:
    template<typename OptsType>
    static OptsType load_opts(const zval* options);

    CS_FORCE_INLINE static void form_retval(zval* return_value, const wdt::PrepareRes& res);
    CS_FORCE_INLINE static void form_retval(zval* return_value, const wdt::ChartRes& rates);
    CS_FORCE_INLINE static void form_retval(zval* return_value, const wdt::IntegerRes& rates);
    CS_FORCE_INLINE static void form_retval(zval* return_value, const wdt::PercentRes& rates);
    CS_FORCE_INLINE static void form_retval(zval* return_value, bool res);

    CS_FORCE_INLINE static void init_retval(zval* return_value, wdt::ResultCode code, wdt::num_t num);
    CS_FORCE_INLINE static void init_retval(zval* return_value, wdt::ResultCode code, wdt::rate_t rate);
    CS_FORCE_INLINE static void init_retval(zval* return_value, wdt::ResultCode code);
    CS_FORCE_INLINE static void init_retval(zval* return_value);
};

template<>
wdt::PrepareOpts Proxy::load_opts<wdt::PrepareOpts>(const zval* options)
{
    wdt::PrepareOpts opts;
    zval** medium;
    _WDT_FETCH_LONG_OPTS(options, opts, medium,
        img_file_min_size, img_file_max_size,
        img_min_width, img_max_width,
        img_min_height, img_max_height
    );

    if (zend_hash_find(Z_ARRVAL_P(options), "img_file", sizeof("img_file"), reinterpret_cast<void**>(&medium)) == SUCCESS)
    {
        opts.img_file = Z_STRVAL_PP(medium);
    }
    return opts;
}

template<>
wdt::ChartOpts Proxy::load_opts<wdt::ChartOpts>(const zval* options)
{
    wdt::ChartOpts opts;
    zval** medium;
    _WDT_FETCH_LONG_OPTS(options, opts, medium,
        x_err, y_err, inverse,
        chart_min_width, chart_max_width,
        chart_min_height, chart_max_height,
        echelons, echelon_padding_left
    );
    return opts;
}

template<>
wdt::IntegerOpts Proxy::load_opts<wdt::IntegerOpts>(const zval* options)
{
    wdt::IntegerOpts opts;
    zval** medium;
    _WDT_FETCH_LONG_OPTS(options, opts, medium,
        x_err, y_err, inverse,
        digit_height, digit_min_width, digit_max_width,
        circle_min_diameter_v,
        vline_adj, hline_adj,
        vline_max_break, hline_max_break,
        vline_min_gap, hline_min_gap,
        comma_max_width, comma_max_height,
        comma_min_area, comma_protrude,
        dot_max_width, dot_max_height,
        dot_min_area,
        left, top, right, bottom
    );
    return opts;
}

template<>
wdt::PercentOpts Proxy::load_opts<wdt::PercentOpts>(const zval* options)
{
    wdt::PercentOpts opts;
    zval** medium;
    _WDT_FETCH_LONG_OPTS(options, opts, medium,
        x_err, y_err, inverse,
        digit_height, digit_min_width, digit_max_width,
        circle_min_diameter_v,
        vline_adj, hline_adj,
        vline_max_break, hline_max_break,
        vline_min_gap, hline_min_gap,
        comma_max_width, comma_max_height,
        comma_min_area, comma_protrude,
        dot_max_width, dot_max_height,
        dot_min_area,
        percent_width,
        left, top, right, bottom
    );
    return opts;
}

void Proxy::detect(wdt::Detecter* detecter, const zval* options, zval* return_value)
{
    int32_t command;
    {
        zval** medium;
        _WDT_GET_OPT_LONG(options, command, medium, type);
    }
    switch (command)
    {
    case wdt::detect_integer:
    {
        wdt::IntegerOpts opts = load_opts<wdt::IntegerOpts>(options);
        wdt::IntegerRes res;
        detecter->detect<wdt::integer>(opts, res);
        form_retval(return_value, res);
    }
    break;

    case wdt::detect_percent:
    {
        wdt::PercentOpts opts = load_opts<wdt::PercentOpts>(options);
        wdt::PercentRes res;
        detecter->detect<wdt::percent>(opts, res);
        form_retval(return_value, res);
    }
    break;

    default:
        form_retval(return_value, false);
    break;
    }
}

void Proxy::prepare(wdt::Detecter* detecter, const zval* options, zval* return_value)
{
    wdt::PrepareOpts opts = load_opts<wdt::PrepareOpts>(options);
    wdt::PrepareRes res;
    detecter->prepare(opts, res);
    form_retval(return_value, res);
}

void Proxy::scale(wdt::Detecter* detecter, const double rate, zval* return_value)
{
    detecter->scale(rate);
    form_retval(return_value, true);
}

void Proxy::locate(wdt::Detecter* detecter, const zval* options, zval* return_value)
{
    int32_t command;
    {
        zval** medium;
        _WDT_GET_OPT_LONG(options, command, medium, type);
    }
    switch (command)
    {
    case wdt::locate_chart:
    {
        wdt::ChartOpts opts = load_opts<wdt::ChartOpts>(options);
        wdt::ChartRes res;
        detecter->locate(opts, res);
        form_retval(return_value, res);
    }
    break;

    default:
        form_retval(return_value, false);
    break;
    }
}

void Proxy::form_retval(zval* return_value, const wdt::PercentRes& res)
{
    init_retval(return_value, res.code, res.percent);
}

void Proxy::form_retval(zval* return_value, const wdt::IntegerRes& res)
{
    init_retval(return_value, res.code, res.num);
}

void Proxy::form_retval(zval* return_value, const wdt::PrepareRes& res)
{
    init_retval(return_value, res.code);
}

void Proxy::form_retval(zval* return_value, const wdt::ChartRes& res)
{
    init_retval(return_value, res.code);
    {
        zval* array;
        MAKE_STD_ZVAL(array);
        array_init(array);
        for (wdt::RateList::const_iterator it = res.rates.begin(); it != res.rates.end(); ++it)
        {
            add_index_double(array, it->first, it->second);
        }
        add_next_index_zval(return_value, array);
    }
    {
        zval* array;
        MAKE_STD_ZVAL(array);
        array_init(array);
        add_next_index_long(array, res.left);
        add_next_index_long(array, res.top);
        add_next_index_zval(return_value, array);
    }
}

void Proxy::init_retval(zval* return_value, wdt::ResultCode code, wdt::rate_t rate)
{
    init_retval(return_value, code);
    add_next_index_double(return_value, rate);
}

void Proxy::init_retval(zval* return_value, wdt::ResultCode code, wdt::num_t num)
{
    init_retval(return_value, code);
    add_next_index_long(return_value, num);
}

void Proxy::init_retval(zval* return_value, wdt::ResultCode code)
{
    init_retval(return_value);
    add_next_index_long(return_value, code);
}

void Proxy::form_retval(zval* return_value, bool res)
{
    RETURN_BOOL(res);
}

void Proxy::init_retval(zval* return_value)
{
    array_init(return_value);
}

void Proxy::set_origin(wdt::Detecter* detecter, wdt::isize_t left, wdt::isize_t top)
{}
