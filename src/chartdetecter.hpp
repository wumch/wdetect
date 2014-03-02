
#pragma once

#include "predef.hpp"
#include <vector>
#include <opencv2/core/core.hpp>
#include "cvdef.hpp"

namespace wdt {

class DetectResult;

class ChartDetecter
{
private:
    DetectResult& res;

    const cv::Mat& img;
    const BoundList& bounds;

    Point tl;

public:
    ChartDetecter(const cv::Mat& img_, const BoundList& bounds_, DetectResult& res_)
        : res(res_), img(img_), bounds(bounds_)
    {}

    void detect();

    CS_FORCE_INLINE const Point& chart_tl() const
    {
        return tl;
    }

protected:
    bool detect(const Bound& chart_bound);

private:
    CS_FORCE_INLINE bool valid(const Bound& bound) const;

    CS_FORCE_INLINE bool length(const uchar* pixels, isize_t width, int32_t& len) const;

    CS_FORCE_INLINE bool is_fg(int32_t color) const;

    CS_FORCE_INLINE double cal_rate(const cv::Vec4f& line, double height) const;
    CS_FORCE_INLINE double cal_x(double gradient_inv, double x0, double y0, double y) const;
    CS_FORCE_INLINE double cal_x(const cv::Vec4f& line, double y) const;
};

}
