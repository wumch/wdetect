
#pragma once

#include "predef.hpp"
#include <vector>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#if CS_DEBUG
#   include <opencv2/highgui/highgui.hpp>
#endif
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
        BoundList bounds;
        {
            Bound bound;
            CS_RETURN_IF(!adjust(bound));
            WDT_IM_SHOW(img(bound));

//            cv::Mat shadow;
//            img(bound).copyTo(shadow);
//            WDT_IM_SHOW(shadow);

//            divide(bound, bounds);

//            ContourList contours;
//            std::vector<cv::Vec4i> hierarchy;
//            cv::findContours(shadow, contours, hierarchy, CV_RETR_LIST, CV_CHAIN_APPROX_NONE, bound.tl());
//
//            CS_DUMP(contours.size());
//            for (ContourList::const_iterator it = contours.begin(); it != contours.end(); ++it)
//            {
//                checkin(bounds, cv::boundingRect(*it));
//            }
//            CS_DUMP(bounds.size());

            if (CS_BUNLIKELY(bounds.empty()))
            {
                res.code = fo_no_match;
                return;
            }
            sort_bounds(bounds);
        }

        res.percent = .0;
        DigitList digits;
        digits.reserve(bounds.size());
        for (BoundList::const_iterator it = bounds.begin(); it != bounds.end(); ++it)
        {
            digit_t digit = recognizer.recognize(crop(*it), opts);
            CS_DUMP(digit);
            if (CS_BUNLIKELY(digit == digit_comma || digit == invalid_digit))
            {
                res.code = fo_recognize;
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
        else
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
