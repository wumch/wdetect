
#pragma once

#include "predef.hpp"
#include <cmath>
#include <vector>
#include <opencv2/core/core.hpp>
#include "cvdef.hpp"
#include "facade.hpp"

namespace wdt {

class Sophist
{
public:
    static const int8_t unknown_num = -1;
    enum Pos {
        top, middle, bottom, unknown
    };

    const cv::Mat& img;
    const NumOpts& opts;
    const int32_t cols, rows;

    int8_t hline, vline;
    int8_t circle, top_circle, bottom_circle;

    Pos hline_pos;
    Pos circle_pos;

    const Config::BinaryColor fg, bg;

public:
    Sophist(const cv::Mat& bimg, const NumOpts& opts_)
        : img(bimg), opts(opts_), cols(img.size().width), rows(img.size().height),
          hline(unknown_num), vline(unknown_num),
          circle(unknown_num), top_circle(unknown_num), bottom_circle(unknown_num),
          hline_pos(unknown), circle_pos(unknown),
          fg(Config::fg(opts.inverse)), bg(Config::bg(opts.inverse))
    {}

    CS_FORCE_INLINE bool is_fg(int32_t col, int32_t row) const
    {
        return is_fg_color(img.ptr<uint8_t>(row)[col]);
    }

    CS_FORCE_INLINE bool is_fg(const Point& point) const
    {
        return is_fg(point.x, point.y);
    }

    CS_FORCE_INLINE Pos cal_circle_pos(int32_t row_begin, int32_t row_end) const
    {
        int32_t top_quantile = round(rows * 0.3), bottom_quantile = round(rows * 0.65);
        if (row_begin <= top_quantile)
        {
            if (row_end >= bottom_quantile)
            {
                return middle;
            }
            else
            {
                return top;
            }
        }
        else if (row_end >= bottom_quantile)
        {
            return bottom;
        }
        return unknown;
    }

    CS_FORCE_INLINE Pos cal_hline_pos(int32_t row) const
    {
        if (row <= round(rows * 0.4))
        {
            return top;
        }
        else if (row >= round(rows * 0.75))
        {
            return bottom;
        }
        else
        {
            return middle;
        }
    }

protected:
    CS_FORCE_INLINE bool is_fg_color(int32_t color) const
    {
        return color == fg;
    }
};

// 数字识别器。 NOTE: 参数设置仅适用于线条较细(<=2px)的数字。
class Recognizer
{
private:
    PointList directions;

public:
    Recognizer();

    digit_t recognize(const cv::Mat& img, const NumOpts& opts) const
    {
        Sophist sop(img, opts);
        return recognize(sop);
    }

    virtual ~Recognizer();

protected:
    digit_t recognize(Sophist& sop) const;

    void detect_hline(Sophist& sop) const;
    void detect_vline(Sophist& sop) const;
    void detect_circle(Sophist& sop) const;

    bool contain_island(const Sophist& sop) const;

protected:
    bool in_circle(const Sophist& sop, const Point& point) const;

    bool hit(const Sophist& sop, const Point& point, const Point& dir) const;

    CS_FORCE_INLINE bool inside(const Sophist& sop, const Point& point) const;
    CS_FORCE_INLINE bool inside(const Sophist& sop, int32_t col, int32_t row) const;

    CS_FORCE_INLINE bool is_fg(const Sophist& sop, const Point& point) const;
    CS_FORCE_INLINE bool is_fg(const Sophist& sop, int32_t col, int32_t row) const;

    digit_t recognize_5_and_7(const Sophist& sop) const;

    digit_t recognize_comma_and_dot(const Sophist& sop) const;
    bool is_dot(const Sophist& sop) const;
    bool is_comma(const Sophist& sop) const;

private:
    CS_FORCE_INLINE bool is_vline(const Sophist& sop, int32_t col) const;
    CS_FORCE_INLINE bool is_hline(const Sophist& sop, int32_t col) const;
};

} // namespace fdt
