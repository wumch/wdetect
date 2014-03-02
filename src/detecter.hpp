
#pragma once

#include "predef.hpp"
#include <opencv2/core/core.hpp>
#include "facade.hpp"

namespace wdt
{

class Detecter
{
protected:
    cv::Mat img;
    isize_t left, top;
    bool offsets;

public:
    Detecter();

    void prepare(const PrepareOpts& opts, PrepareRes& res);

    void locate(const ChartOpts& opts, ChartRes& res) const;

    void set_origin(isize_t left, isize_t top);

    void detect(const IntegerOpts& opts, IntegerRes& res) const;
    void detect(const PercentOpts& opts, PercentRes& res) const;
};

} // namespace wdt
