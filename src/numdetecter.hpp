
#pragma once

#include "predef.hpp"
#include <algorithm>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include "cvdef.hpp"
#include "facade.hpp"
#include "recognizer.hpp"

namespace wdt {

class NumDetecter
{
protected:
    static const digit_t invalid_digit = -1;
    static const digit_t digit_comma = -2;
    static const digit_t digit_dot = -3;

    static const isize_t separate_max_sway = 2;

    static const Recognizer recognizer;

    const cv::Mat& img;

protected:
    explicit NumDetecter(const cv::Mat& img_)
        : img(img_)
    {}

    static const class BoundCmper
    {
    public:
        bool operator()(const Bound& left, const Bound& right) const
        {
            return right.x < left.x;
        }
    } bound_cmper;

    CS_FORCE_INLINE void sort_bounds(BoundList& bounds) const
    {
        std::sort(bounds.begin(), bounds.end(), bound_cmper);
    }

    template<typename OptsType, typename ResType>
    CS_FORCE_INLINE bool adjust(const OptsType& opts, Bound& bound, ResType& res) const
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
        double result = .0;
        num_t num = 0;
        for (int32_t i = digits.size(); i > 0; --i)
        {
            if (digits[i] == digit_dot)
            {
                result += num / std::pow(10, digits.size() - i);
                num = 0;
            }
            else if (digits[i] != digit_comma)
            {
                num = num * 10 + digits[i];
            }
        }
        return result + num;
    }

    cv::Mat crop(const Bound& bound) const
    {
        cv::Mat shadow;
        img(bound).copyTo(shadow);
        return shadow;
    }

    void devide(const cv::Mat& frag, BoundList& bounds) const;

    bool separated(const cv::Mat& frag, isize_t col, isize_t& left, isize_t& right) const;

    bool separated(const cv::Mat& frag, isize_t col) const;
};

} // namespace wdt
