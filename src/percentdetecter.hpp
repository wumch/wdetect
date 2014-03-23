
#pragma once

#include "predef.hpp"
#include <vector>
#include <algorithm>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include "numdetecterbase.hpp"
#include "math.hpp"
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
        CS_DUMP(pils.imgs.size());
        rstrip_percent();
        CS_DUMP(pils.imgs.size());

        if (!check_x_interact())
        {
            res.code = fo_digit_x_interact;
            return;
        }

        res.percent.reserve(pils.imgs.size());
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
                res.percent += '.';
            }
            else
            {
                res.percent +=  '0' + digit;
            }
        }

        if (!broken)
        {
            res.code = success;
        }
    }

protected:
    // TODO: more flexible, more graceful.
    void rstrip_percent()
    {
        static const int32_t slash_max_offset_tail = 3, invalid_widest_idx = -1;
        const int32_t percent_parts = std::min<int32_t>(pils.poses.size(), slash_max_offset_tail);
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
        const isize_t y_min = pils.y_mode - 2, y_max = pils.y_mode + 2;
        const bool widest_valid = (widest_idx >= pils.imgs.size() - 2)
                || (pils.imgs[widest_idx].cols > opts.digit_max_width);

        for (int32_t i = pils.poses.size() - 1; i >= first_idx; --i)
        {
            if ((widest_valid && (i == widest_idx || interact(i, widest_left, widest_right)))
                || pils.imgs[i].rows <= pcircle_min_height
                || !staging::between(pils.imgs[i].cols, opts.digit_min_width, opts.digit_max_width)
                || !staging::between(pils.poses[i].y, y_min, y_max))
            {
                CS_DUMP(i);
                CS_DUMP(interact(i, widest_left, widest_right));
                CS_DUMP(y_min);
                CS_DUMP(y_max);
                CS_DUMP(pils.poses[i].y);
                pils.pop_back();
                CS_SAY("striped one for percent");
            }
            else
            {
                break;
            }
        }
    }

    CS_FORCE_INLINE void checkin(BoundList& bounds, const Bound& bound) const
    {
        bounds.push_back(bound);
    }
};

} // namespace wdt
