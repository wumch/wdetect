
#pragma once

#include "predef.hpp"
#include <map>
#include <string>
#include <functional>
#include <string>

namespace wdt {

class Config
{
public:
	enum {invalid_digit = -1, digit_comma = -2, digit_dot = -3};
    static const size_t img_file_min_size = 24;
    static const isize_t invalid_y_mode = -1;

    typedef enum {
        black = 0, white = 255,
    } BinaryColor;

    static BinaryColor fg(bool inverse = false)
    {
        return inverse ? white : black;
    }

    static BinaryColor bg(bool inverse = false)
    {
        return inverse ? black : white;
    }
};

typedef enum {
	locate_unknown = 0,
    locate_chart,
} CmdLocate;

typedef enum {
	detect_unknown = 0,
    detect_integer,
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

    fo_calc_chart_width = 12,   // 计算图表宽度失败
    fo_calc_chart_height = 13,   // 计算图表高度失败
    fo_digit_x_interact = 14,   // 图像上的数字 横向相交
} ResultCode;

// to resident into php array, it's better to be an ordered map.
typedef std::map<int32_t, rate_t, std::less<int32_t> > RateList;

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
    isize_t chart_width;
    isize_t chart_height;
    RateList rates;
};

class DetectRes: public Result
{};

class IntegerRes: public DetectRes
{
public:
    std::string num;
};

class PercentRes: public DetectRes
{
public:
    std::string percent;
};

class ScanRes: public DetectRes
{
public:
	isize_t pos;		// 推进到
	bool match;			// 是否匹配到
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
    bool inverse;           // 反色
};

class LocateOpts: public DetectOpts
{};

// chart options for `locate()`
class ChartOpts: public LocateOpts
{
public:
    double chart_min_margin_bottom;

    isize_t chart_min_width, chart_max_width,
        chart_min_height, chart_max_height;
    int32_t echelons;    // 梯形最大数量
    isize_t echelon_padding_left;    // 梯形左补白
    isize_t chart_min_margin_right, chart_max_margin_right;
    isize_t chart_min_space_bottom, chart_max_margin_bottom;
    isize_t chart_height_scan_width;
    int32_t chart_margin_max_fg;
    int32_t echelon_max_wrong_row;    // 违反斜率最多行数
    int32_t echelon_gradient_min_continuous;    // 垂直(斜率为1)最少连续相等行数
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
    isize_t digit_height, digit_min_width, digit_max_width;
    isize_t circle_min_diameter_v;    // 数字形状中圈的最小直径
    isize_t vline_adj, hline_adj;     // 横/竖线允许的邻接宽度
    isize_t vline_max_break, hline_max_break;  // 线最大断点数
    isize_t vline_min_gap, hline_min_gap;      // 线之间最小间隔

    isize_t comma_max_width, comma_max_height, comma_min_height,
        comma_min_area, comma_protrude;

    isize_t dot_max_width, dot_max_height,
        dot_min_area;
};

class IntegerOpts: public NumOpts
{};

class PercentOpts: public NumOpts
{
public:
    isize_t percent_width;
};

class ScanOpts: public Options
{
public:
	isize_t low, high;		// boundary.(扫描时逐行/列步进用).
	isize_t begin, end;		// scanning range (扫描时逐像素步进用).
	isize_t min_continuous;	// 最小连续匹配 才算找到。
	isize_t max_miss;		// 最大不匹配次数
	bool match_fg;			// true: 找前景色； false:找背景色
	bool inverse;			// 颜色反转

	ScanOpts()
		: match_fg(true), inverse(false)
	{}
};

class RowScanOpts: public ScanOpts
{};

class ColScanOpts: public ScanOpts
{};

typedef enum {
    integer, percent
} NumDetecterKind;

template<NumDetecterKind kind>
class NumDetecterTraits;

template<>
class NumDetecterTraits<integer>
{
public:
    typedef IntegerOpts OptsType;
    typedef IntegerRes  ResType;
};

template<>
class NumDetecterTraits<percent>
{
public:
    typedef PercentOpts OptsType;
    typedef PercentRes  ResType;
};

}
