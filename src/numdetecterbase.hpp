
#pragma once

#include "predef.hpp"
#include <cstdio>
#include <map>
#include <algorithm>
#include <boost/static_assert.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include "cvdef.hpp"
#include "facade.hpp"
#include "divider.hpp"
#include "recognizer.hpp"

namespace wdt {

template<NumDetecterKind kind>
class NumDetecterBase
{
protected:
    typedef NumDetecterTraits<kind> Traits;
    typedef typename Traits::OptsType OptsType;
    typedef typename Traits::ResType ResType;

protected:
    static const isize_t separate_max_sway = 2;

    static const Recognizer recognizer;

    const cv::Mat& img;
    const OptsType& opts;
    mutable PositedImageList pils;
    ResType& res;

    BOOST_STATIC_ASSERT(sizeof(uchar) == sizeof(uint8_t));
    const Config::BinaryColor fg_color;

protected:
    explicit NumDetecterBase(const cv::Mat& img_, const OptsType& opts_, ResType& res_)
        : img(img_), opts(opts_), res(res_), fg_color(opts.inverse ? Config::white : Config::black)
    {}

    CS_FORCE_INLINE bool adjust(Bound& bound) const
    {
        isize_t left, top, right, bottom;
        left = std::max(opts.left, 0);
        top = std::max(opts.top, 0);
        right = std::min(opts.right, img.cols);
        bottom = std::min(opts.bottom, img.rows);

        isize_t width = right - left, height = bottom - top;
        if (!(width > 0 && height > 0))
        {
            res.code = fo_wrong_param;
            return false;
        }

        bound.x = left;
        bound.y = top;
        bound.width = width;
        bound.height = height;

        return true;
    }

    std::string digits_to_num(const DigitList& digits) const
    {
        std::string result;
        result.reserve(digits.size());
        for (DigitList::const_iterator it = digits.begin(); it != digits.end(); ++it)
        {
            result.append(1, *it == Config::digit_comma ? ',' : ('0' + *it));
        }
        return result;
    }

    num_t _digits_to_num(const DigitList& digits) const
    {
        num_t result = 0;
        for (DigitList::const_iterator it = digits.begin(); it != digits.end(); ++it)
        {
            // the Qin Jiushao Algorithm, created in Chinese Song Dynasty.
            result = (result << 3) + (result << 1) + *it;
        }
        return result;
    }

    cv::Mat crop(const Bound& bound) const
    {
        cv::Mat shadow;
        img(bound).copyTo(shadow);
        return shadow;
    }

    bool check_x_interact() const
    {
        CS_RETURN_IF(pils.imgs.empty(), true);

        isize_t prev_left = pils.poses[0].x, prev_right = prev_left + pils.imgs[0].cols;
        isize_t cur_left, cur_right;
        for (uint32_t i = 1; i < pils.imgs.size(); ++i)
        {
            cur_left = pils.poses[i].x;
            cur_right = cur_left + pils.imgs[i].cols;
            if (interact(cur_left, cur_right, prev_left, prev_right))
            {
            	if (i < pils.break_flags.size() && !pils.break_flags[i])
            	{
            		return false;
            	}
            }
            prev_left = cur_left;
            prev_right = cur_right;
        }
        return true;
    }

    bool interact(int32_t other_idx, isize_t widest_left, isize_t widest_right) const
    {
        const isize_t other_left = pils.poses[other_idx].x;
        const isize_t other_right = other_left + pils.imgs[other_idx].cols;
        return interact(widest_left, widest_right, other_left, other_right);
    }

    bool interact(isize_t a_left, isize_t a_right, isize_t b_left, isize_t b_right) const
    {
        return !(a_right <= b_left || b_right <= a_left);
    }

    void divide(const cv::Mat& frag) const
    {
        Divider<kind> divider(frag, opts, pils);
        divider.divide();
        pils.assemble();
    }

    void record_frag(const cv::Mat& frag, BoundList& bounds, isize_t left, isize_t right) const
    {
        isize_t top, bottom;
        clip(frag(Bound(left, 0, right - left, frag.rows)), top, bottom);
        CS_DUMP(top);
        CS_DUMP(bottom);
        bounds.push_back(Bound(left, top, right - left, bottom - top));
    }

    void clip(const cv::Mat& sheet, isize_t& top, isize_t& bottom) const
    {
        WDT_IM_SHOW(sheet);
        top = 0;
        for ( ; top < sheet.rows; ++top)
        {
            const uint8_t* pixels = sheet.ptr<uint8_t>(top);
            if (!is_bg_row(pixels, pixels + sheet.cols))
            {
                break;
            }
        }
        bottom = 0;
        for (isize_t row = sheet.rows - 1; row > 0; --row)
        {
            const uint8_t* pixels = sheet.ptr<uint8_t>(row);
            if (!is_bg_row(pixels, pixels + sheet.cols))
            {
                bottom = row + 1;
                break;
            }
        }
    }

    bool separated(const cv::Mat& frag, isize_t col) const
    {
        for (isize_t row = 0; row < frag.rows; ++row)
        {
            if (is_fg(frag.ptr<uint8_t>(row)[col]))
            {
                return false;
            }
        }
        return true;
    }

    CS_FORCE_INLINE bool is_bg_row(const uint8_t* begin, const uint8_t* end) const
    {
        while (begin != end)
        {
            if (is_fg(*begin))
            {
                return false;
            }
            ++begin;
        }
        return true;
    }

    bool is_fg(uint8_t color) const
    {
        return color == fg_color;
    }

    bool prepare()
    {
        Bound bound;
        CS_RETURN_IF(!adjust(bound), false);
        WDT_IM_SHOW(img(bound));

        divide(img(bound));

        if (CS_BUNLIKELY(pils.imgs.empty()))
        {
            WDT_SHOW_IMG(img(bound));
            res.code = fo_no_match;
            return false;
        }
        return true;
    }
};

template<NumDetecterKind kind>
const Recognizer NumDetecterBase<kind>::recognizer;

template<NumDetecterKind kind>
class NumDetecter;

} // namespace wdt
