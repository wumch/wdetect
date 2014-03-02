
#pragma once

#include "predef.hpp"
#include <opencv2/core/core.hpp>
#include "cvdef.hpp"

namespace wdt {

class Preprocer
{
public:
    Preprocer();

    CS_FORCE_INLINE bool check(const char* img_file) const
    {
        return check_file_size(img_file) && check_img_size(img_file);
    }

    cv::Mat binarize(const char* img_file) const;

    void cal_bounds(const cv::Mat& bimg, BoundList& bounds) const;

protected:
    bool check_file_size(const char* img_file) const;

    bool check_img_size(const char* img_file) const;

    bool check_size(ssize_t width, ssize_t height) const;

    CS_FORCE_INLINE void checkin(const Bound& bound, BoundList& bounds) const;
    CS_FORCE_INLINE bool valid(const Bound& bound) const;
};

} // namespace wdt
