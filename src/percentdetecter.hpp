
#pragma once

#include "predef.hpp"
#include <vector>
#include <algorithm>
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
        rstrip_percent();

        res.percent = .0;
        DigitList digits;
        digits.reserve(pils.imgs.size());
        bool broken = false;
        for (ImageList::const_iterator it = pils.imgs.begin(); it != pils.imgs.end(); ++it)
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
                res.percent += digits_to_num(digits);
                digits.clear();
            }
            else
            {
                digits.push_back(digit);
            }
        }

        if (digits.empty())
        {
            res.code = fo_no_match;
        }
        else if (!broken)
        {
            res.percent += digits_to_num(digits) / std::pow(10, digits.size());
            res.code = success;
        }
    }

protected:
    // TODO: more flexible, more graceful.
    void rstrip_percent()
    {
        static const int32_t percent_max_parts = 3, invalid_widest_idx = -1;
        const int32_t percent_parts = std::min<int32_t>(pils.poses.size(), percent_max_parts);
        int32_t widest_idx = invalid_widest_idx;
        const int32_t first_idx = pils.poses.size() - percent_parts;
        {
            isize_t widest = -1;
            for (int32_t i = pils.poses.size() - 1; i >= first_idx; --i)
            {
                if (pils.imgs[i].cols > widest)
                {
                    widest = pils.imgs[i].cols;
                    widest_idx = i;
                }
            }
            CS_DUMP(widest_idx);
            CS_DUMP(widest);
            CS_RETURN_IF(widest_idx == invalid_widest_idx);
        }

        const isize_t pcircle_min_height = round(opts.digit_height * 0.6);
        const isize_t widest_left = pils.poses[widest_idx].x;
        const isize_t widest_right = widest_left + pils.imgs[widest_idx].cols;

        for (int32_t i = pils.poses.size() - 1; i >= first_idx; --i)
        {
            if (i == widest_idx || interact(widest_left, widest_right, i)
                || pils.imgs[i].rows < pcircle_min_height)
            {
                pils.pop_back();
                CS_SAY("striped one for percent");
            }
            else
            {
                break;
            }
        }
    }

    bool interact(int32_t other_idx, isize_t widest_left, isize_t widest_right) const
    {
        const isize_t other_left = pils.poses[other_idx].x;
        const isize_t other_right = other_left + pils.imgs[other_idx].cols;
        return !(other_right <= widest_left || widest_right <= other_left);
    }

    CS_FORCE_INLINE void checkin(BoundList& bounds, const Bound& bound) const
    {
        bounds.push_back(bound);
    }
};

} // namespace wdt
