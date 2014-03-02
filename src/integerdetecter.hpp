
#pragma once

#include "predef.hpp"
#include <opencv2/core/core.hpp>
#include "numdetecter.hpp"
#include "cvdef.hpp"
#include "facade.hpp"

namespace wdt
{

class IntegerDetecter: public NumDetecter
{
protected:
    const IntegerOpts& opts;
    IntegerRes& res;

public:
    IntegerDetecter(const cv::Mat& img_, const IntegerOpts& opts_, IntegerRes& res_)
        : NumDetecter(img_), opts(opts_), res(res_)
    {}

    void detect();

protected:
    CS_FORCE_INLINE void checkin(BoundList& bounds, const Bound& bound) const;

    CS_FORCE_INLINE bool adjust(Bound& bound) const;

    CS_FORCE_INLINE bool inside() const;
};

} // namespace wdt
