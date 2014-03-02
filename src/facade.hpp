
#pragma once

#include "predef.hpp"
#include <map>
#include <string>

namespace wdt {

class Config
{
public:
    static const size_t img_file_min_size = 24;
    static const digit_t invalid_digit = -1;
    static const digit_t digit_comma = -2;
    static const digit_t digit_dot = -3;

    typedef enum {
        black = 0, white = 255,
    } BinaryColor;
};

typedef enum {
    locate_chart = 0
} CmdLocate;

typedef enum {
    detect_integer = 0,
    detect_percent,
    detect_textwidth,
} CmdDetect;

typedef enum {
    success = 0,    // 一切正常
    failure = 1,    // 失败，原因未知

    fo_wrong_param, // 参数有误

    fo_img_file_nonexists = 2,
    fo_img_file_unreadable = 3,
    fo_img_file_ext     = 4,    // 图片文件扩展名不被支持
    fo_img_file_size    = 5,    // 图片文件体积不合格
    fo_img_type    = 6,    // 图片类型(按头信息判断)检验失败
    fo_img_content    = 7,    // 图片类型(按头信息判断)检验失败
    fo_img_size         = 8,    // 图片尺寸不合格

    fo_no_match   = 9,    // 没有检测到梯形
    fo_detect      = 10,    // 文字信息检测失败(具体错误看数据)
    fo_recognize      = 11,    // 文字信息检测失败(具体错误看数据)
} ResultCode;

// to resident into php array, it's better to be an ordered map.
typedef std::map<int32_t, rate_t> RateList;

// prepare/locate/detect result.
class Result
{
public:
    ResultCode code;

    Result()
        : code (success)
    {}
};

class PrepareRes: public Result
{};

class LocateRes: public Result
{
public:
    isize_t left, top;
};

// TODO: currently not supported to keep an err-code for every echelon.
class ChartRes: public LocateRes
{
public:
    RateList rates;
};

class DetectRes: public Result
{};

class IntegerRes: public DetectRes
{
public:
    num_t num;
};

class PercentRes: public DetectRes
{
public:
    rate_t percent;
};

// prepare/locate/detect options
class Options
{};

class PrepareOpts: public Options
{
public:
    std::string img_file;
    isize_t img_file_min_size, img_file_max_size;
    isize_t img_min_width, img_max_width,
        img_min_height, img_max_height;

    PrepareOpts()
        : img_file_min_size(Config::img_file_min_size)
    {}
};

class DetectOpts: public Options
{
public:
    int32_t type;
    isize_t x_err, y_err;   // x,y 允许误差
};

class LocateOpts: public DetectOpts
{};

// chart options for `locate()`
class ChartOpts: public LocateOpts
{
public:
    isize_t chart_min_width, chart_max_width,
        chart_min_height, chart_max_height;
    int32_t echelons;    // 梯形最大数量
    isize_t echelon_padding_left;    // 梯形左补白
};

// detect-inside-bound options
class DIBOpts: public DetectOpts
{
public:
    isize_t left, top, right, bottom;
};

// options for numeric value
class NumOpts: public DIBOpts
{
public:
    isize_t digit_height;
    isize_t circle_min_diameter_v;    // 数字形状中圈的最小直径
    isize_t vline_adj, hline_adj;     // 横/竖线允许的邻接宽度
    isize_t vline_max_break, hline_max_break;  // 线最大断点数
    isize_t vline_min_gap, hline_min_gap;      // 线之间最小间隔

    isize_t comma_width, comma_height,
        comma_min_area, comma_protrude;

    isize_t dot_width, dot_height,
        dot_min_area;
};

class IntegerOpts: public NumOpts
{
public:
};

class PercentOpts: public NumOpts
{
public:
    isize_t percent_width;
};

}
