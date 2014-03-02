
#include "integerdetecter.hpp"
#include <vector>
#include <opencv2/imgproc/imgproc.hpp>
#if CS_DEBUG
#   include <opencv2/highgui/highgui.hpp>
#endif

namespace wdt {

void IntegerDetecter::detect()
{
    BoundList bounds;
    {
        Bound bound;
        CS_RETURN_IF(!adjust(bound));
        WDT_IM_SHOW(img(bound));

        cv::Mat shadow;
        img(bound).copyTo(shadow);
        WDT_IM_SHOW(shadow);

        ContourList contours;
        std::vector<cv::Vec4i> hierarchy;
        cv::findContours(shadow, contours, hierarchy, CV_RETR_LIST, CV_CHAIN_APPROX_NONE, bound.tl());

        CS_DUMP(contours.size());
        for (ContourList::const_iterator it = contours.begin(); it != contours.end(); ++it)
        {
            checkin(bounds, cv::boundingRect(*it));
        }
        CS_DUMP(bounds.size());

        if (CS_BUNLIKELY(bounds.empty()))
        {
            res.code = fo_no_match;
            return;
        }
        sort_bounds(bounds);
    }

    DigitList digits;
    digits.reserve(bounds.size());
    for (BoundList::const_iterator it = bounds.begin(); it != bounds.end(); ++it)
    {
        digit_t digit = recognizer.recognize(crop(*it), opts);
        CS_DUMP(digit);
        if (CS_BUNLIKELY(digit == digit_dot || digit == invalid_digit))
        {
            res.code = fo_recognize;
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
    else
    {
        res.num = digits_to_num(digits);
        res.code = success;
    }
}

bool IntegerDetecter::adjust(Bound& bound) const
{
    return NumDetecter::adjust(opts, bound, res);
}

void IntegerDetecter::checkin(BoundList& bounds, const Bound& bound) const
{
    bounds.push_back(bound);
}

} // namespace wdt
