
#pragma once

#include "predef.hpp"
#include <cstdio>
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
    static const digit_t invalid_digit = -1;
    static const digit_t digit_comma = -2;
    static const digit_t digit_dot = -3;

    static const isize_t separate_max_sway = 2;

    static const Recognizer recognizer;

    const cv::Mat& img;
    const OptsType& opts;
    mutable ImageList digit_imgs;
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

    num_t digits_to_num(const DigitList& digits) const
    {
        num_t result = 0;
        for (int32_t i = digits.size() - 1; i >= 0; --i)
        {
            // the Qin Jiushao Algorithm, created in Chinese Song Dynasty.
            result = (result << 3) + (result << 1) + digits[i];
        }
        return result;
    }

    cv::Mat crop(const Bound& bound) const
    {
        cv::Mat shadow;
        img(bound).copyTo(shadow);
        return shadow;
    }

    void _devide(const Bound& bound, BoundList& bounds) const
    {
        _devide(img(bound), bounds);
        for (BoundList::iterator it = bounds.begin(); it != bounds.end(); ++it)
        {
            it->x += bound.x;
            it->y += bound.y;
        }
    }

    void divide(const cv::Mat& frag) const
    {
        Divider<kind> divider(frag, opts, digit_imgs);
        divider.divide();
    }

    void _devide(const cv::Mat& frag, BoundList& bounds) const
    {
        isize_t fg_begin = -1;
        bool prev_is_fg = false;
        for (isize_t col = 0; col < frag.cols; ++col)
        {
            if (separated(frag, col))
            {
                if (prev_is_fg)
                {
                    prev_is_fg = false;
                    record_frag(frag, bounds, fg_begin, col);
                }
            }
            else
            {
                if (!prev_is_fg)
                {
                    prev_is_fg = true;
                    fg_begin = col;
                }
            }
        }

        for (BoundList::iterator it = bounds.begin(); it != bounds.end(); ++it)
        {

        }
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

    bool separated(const cv::Mat& frag, isize_t col, isize_t& left, isize_t& right) const
    {

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

        if (CS_BUNLIKELY(digit_imgs.empty()))
        {
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
