
#pragma once

#include "predef.hpp"
#include <vector>
#include <opencv2/core/core.hpp>
#include "cvdef.hpp"
#include "facade.hpp"

namespace wdt {

class DetectResult;

class Locater
{
private:
    const cv::Mat& img;
    const ChartOpts& opts;
    ChartRes& res;

    const Config::BinaryColor fg, bg;

public:
    Locater(const cv::Mat& img_, const ChartOpts& opts_, ChartRes& res_)
        : img(img_), opts(opts_), res(res_),
          fg(Config::fg(opts.inverse)), bg(Config::bg(opts.inverse))
    {}

    void locate();

protected:
    bool detect(const Bound& chart_bound);

    void calc_width(const Bound& box);
    void calc_height(const Bound& box);

private:
    CS_FORCE_INLINE bool valid(const Bound& bound) const;
    CS_FORCE_INLINE void checkin(BoundList& bounds, const Bound& bound) const;

    CS_FORCE_INLINE bool length(const uchar* pixels, isize_t width, int32_t& len) const;

    CS_FORCE_INLINE bool is_fg(int32_t color) const;

    CS_FORCE_INLINE double calc_rate(const cv::Vec4f& line, double height) const;
    CS_FORCE_INLINE double calc_x(double gradient_inv, double x0, double y0, double y) const;
    CS_FORCE_INLINE double calc_x(const cv::Vec4f& line, double y) const;

    bool is_margin_col(isize_t col, isize_t top, isize_t bottom) const;
    bool is_margin_row(isize_t row, isize_t left, isize_t right) const;
};

}
