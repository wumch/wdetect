
#pragma once

#include "predef.hpp"
#include <opencv2/core/core.hpp>
#include "cvdef.hpp"
#include "facade.hpp"

namespace wdt {

class Preprocer
{
public:
    Preprocer();

    CS_FORCE_INLINE bool check(const PrepareOpts& opts, PrepareRes& res) const
    {
        return check_file_size(opts, res) && check_img_size(opts, res);
    }

    cv::Mat binarize(const char* img_file) const;

protected:
    bool check_file_size(const PrepareOpts& opts, PrepareRes& res) const;

    bool check_img_size(const PrepareOpts& opts, PrepareRes& res) const;

    bool check_size(const PrepareOpts& opts, ssize_t width, ssize_t height) const;
};

} // namespace wdt
