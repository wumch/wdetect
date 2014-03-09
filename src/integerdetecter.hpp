
#pragma once

#include "predef.hpp"
#include <vector>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include "numdetecterbase.hpp"
#include "cvdef.hpp"
#include "facade.hpp"

namespace wdt {

template<>
class NumDetecter<integer>: public NumDetecterBase<integer>
{
protected:
    typedef NumDetecterBase<integer> Base;

public:
    NumDetecter(const cv::Mat& img_, const OptsType& opts_, ResType& res_)
        : Base(img_, opts_, res_)
    {}

    void detect()
    {
        prepare();
        DigitList digits;
        CS_DUMP(digit_imgs.size());
        digits.reserve(digit_imgs.size());
        bool broken = false;
        for (ImageList::const_iterator it = digit_imgs.begin(); it != digit_imgs.end(); ++it)
        {
            digit_t digit = recognizer.recognize(*it, opts);
            WDT_IM_SHOW(*it);
            CS_DUMP(digit);
            if (CS_BUNLIKELY(digit == digit_dot || digit == invalid_digit))
            {
                res.code = fo_recognize;
                broken = true;
                break;
            }
            else if (digit != digit_comma)
            {
                digits.push_back(digit);
            }
        }
        CS_DUMP(digits.size());

        if (digits.empty())
        {
            res.code = fo_no_match;
        }
        else if (!broken)
        {
            res.num = digits_to_num(digits);
            res.code = success;
        }
    }


protected:
    CS_FORCE_INLINE void checkin(BoundList& bounds, const Bound& bound) const
    {
        bounds.push_back(bound);
    }
};

} // namespace wdt
