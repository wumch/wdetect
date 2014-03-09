
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
class NumDetecter<percent>: public NumDetecterBase<percent>
{
protected:
    typedef NumDetecterBase<percent> Base;

public:
    NumDetecter(const cv::Mat& img_, const OptsType& opts_, ResType& res_)
        : Base(img_, opts_, res_)
    {}

    void detect()
    {
        prepare();
        res.percent = .0;
        DigitList digits;
        digits.reserve(digit_imgs.size());
        bool broken = false;
        for (ImageList::const_iterator it = digit_imgs.begin(); it != digit_imgs.end(); ++it)
        {
            digit_t digit = recognizer.recognize(*it, opts);
            CS_DUMP(digit);
            if (CS_BUNLIKELY(digit == digit_comma || digit == invalid_digit))
            {
                res.code = fo_recognize;
                broken = true;
                break;
            }
            else if (digit == digit_dot)
            {
                res.percent += digits_to_num(digits) / std::pow(10, digits.size());
                digits.clear();
            }
            else
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
            res.percent += digits_to_num(digits);
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
