
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
        if (!check_x_interact())
        {
            res.code = fo_digit_x_interact;
            return;
        }

        DigitList digits;
        CS_DUMP(pils.imgs.size());
        digits.reserve(pils.imgs.size());
        bool broken = false;
        // ImageList::const_iterator it = pils.imgs.begin(); it != pils.imgs.end(); ++it
        for (uint32_t i = 0; i < pils.imgs.size(); ++i)
        {
        	Sophist::Erode erode = Sophist::no;
        	if (i < pils.break_flags.size())
        	{
        		if (pils.break_flags[i])
        		{
        			erode = Sophist::left;
        		}
        		else if ((i+1) < pils.break_flags.size())
        		{
        			if (pils.break_flags[i+1])
        			{
        				erode = Sophist::right;
        			}
        		}
        	}
            digit_t digit = recognizer.recognize(pils.imgs[i], opts, erode);
            CS_DUMP(digit);
            if (CS_BUNLIKELY(digit == Config::invalid_digit))
            {
                WDT_SHOW_IMG(pils.imgs[i]);
                res.code = fo_recognize;
                broken = true;
                break;
            }
            else
            {
            	if (digit == Config::digit_dot)
            	{
            		digits.push_back(Config::digit_comma);
            	}
            	else
            	{
            		digits.push_back(digit);
            	}
            }
        }
        CS_DUMP(digits.size());

        if (!broken)
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
