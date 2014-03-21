
#include "locater.hpp"
#include <cmath>
#include <vector>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "math.hpp"
#include "preprocer.hpp"
#include "facade.hpp"

namespace wdt {

void Locater::locate()
{
    BoundList bounds;
    {
        cv::Mat shadow;
        img.copyTo(shadow);

        ContourList contours;
        cv::findContours(shadow, contours, CV_RETR_LIST, CV_CHAIN_APPROX_NONE);

        for (ContourList::const_iterator it = contours.begin(); it != contours.end(); ++it)
        {
            checkin(bounds, cv::boundingRect(*it));
        }

        if (CS_BUNLIKELY(bounds.empty()))
        {
            res.code = fo_no_match;
            return;
        }
    }

    bool detected = false;
    Bound box;
    for (BoundList::const_iterator it = bounds.begin(); it != bounds.end(); ++it)
    {
        if (valid(*it))
        {
            if (detect(*it))
            {
                detected = true;
                box = *it;
                break;
            }
        }
    }

    if (detected)
    {

    }
    else
    {
        res.code = fo_no_match;
    }
}

bool Locater::detect(const Bound& chart_bound)
{
    typedef std::vector<isize_t> RopeList;
    typedef std::vector<RopeList> EchelonList;

    const cv::Mat chart = img(chart_bound);
    isize_t first_echelon_y = -1;
    bool prev_valid = false;
    EchelonList echelons;
    echelons.reserve(opts.echelons);
    EchelonList::reverse_iterator echelon;

    isize_t rope, prev_rope;
    isize_t continuous_equal = 0;
    enum gradient_sign { negative = -1, zero_or_pending = 0, positive = 1, invalid = -2};
    int32_t gradient_sign = invalid; // 斜率符号（-1/0/1有效, -2无效)
    for (isize_t row = 0; row < chart.rows; ++row)
    {
        if (length(chart.ptr<uchar>(row), chart.cols, rope))
        {
            if (!prev_valid)
            {
                if (first_echelon_y == negative)
                {
                    first_echelon_y = row;
                }
                CS_SAY("new echelon begans at [" << row << "]");
                prev_valid = true;
                continuous_equal = 0;
                gradient_sign = invalid;
                echelons.push_back(RopeList());
                echelon = echelons.rbegin();
            }
            else
            {
                if (gradient_sign == invalid)
                {
                    if (rope > prev_rope)
                    {
                        gradient_sign = negative;
                    }
                    else if (rope < prev_rope)
                    {
                        gradient_sign = positive;
                    }
                    else
                    {
                        if (++continuous_equal > 3)
                        {
                            gradient_sign = zero_or_pending;
                        }
                    }
                }
                else
                {
                    if (!((rope >= prev_rope && gradient_sign == negative)
                        || (rope <= prev_rope && gradient_sign == positive)
                        || (rope == prev_rope && gradient_sign == zero_or_pending)))
                    {
                        return false;
                    }
                }
            }
            echelon->push_back(rope);
            prev_rope = rope;
        }
        else if (prev_valid)
        {
            prev_valid = false;
        }
    }

    const isize_t eche_min_height = opts.chart_min_height - 2;
    for (EchelonList::iterator it = echelons.begin(); it != echelons.end(); )
    {
        if (it->size() < eche_min_height)
        {
            it = echelons.erase(it);
        }
        else
        {
            ++it;
        }
    }

    CS_DUMP(chart.cols);
    CS_DUMP(chart.rows);
    CS_DUMP(echelons.size());
#if CS_DEBUG
    if (!echelons.empty())
    {
        CS_SAY("first echelon height: [" << CS_OC_RED(echelons.begin()->size()) << "]");
        for (RopeList::const_iterator it = echelons.begin()->begin(); it != echelons.begin()->end(); ++it)
        {
            CS_STDOUT << *it << ",";
        }
        CS_STDOUT << std::endl;
    }
#endif
    CS_RETURN_IF(echelons.empty() || echelons.size() > opts.echelons, false);
    res.left = chart_bound.x;
    res.top = first_echelon_y + chart_bound.y;

    int32_t turn = 0;
    for (EchelonList::const_iterator eche_it = echelons.begin(); eche_it != echelons.end(); ++eche_it)
    {
        PointList points;
        int32_t row_index = eche_it->size();
        points.reserve(eche_it->size());

        for (RopeList::const_iterator rope_it = eche_it->begin(); rope_it != eche_it->end(); ++rope_it)
        {
            points.push_back(Point(*rope_it, row_index--));
        }

        cv::Vec4f line;
        cv::Mat data(points);
        cv::fitLine(data, line, CV_DIST_L2, 1, 0.001, 0.001);
        CS_DUMP(line[0]);
        CS_DUMP(line[1]);
        CS_DUMP(line[2]);
        CS_DUMP(line[3]);
        CS_DUMP(cal_x(line, -1));
        CS_DUMP(cal_x(line, 0));
        CS_DUMP(eche_it->size());
        CS_DUMP(cal_x(line, eche_it->size() - 1));
        CS_DUMP(cal_x(line, eche_it->size()));
        CS_DUMP(cal_x(line, eche_it->size() + 1));

        res.rates.insert(std::make_pair(turn, cal_rate(line, eche_it->size())));
        CS_DUMP(res.rates.rbegin()->second);
        ++turn;
    }
    return true;
}

CS_FORCE_INLINE double Locater::cal_rate(const cv::Vec4f& line, double height) const
{
    return (cal_x(line, 0) - opts.echelon_padding_left)
        / (cal_x(line, height) - opts.echelon_padding_left);
}

double Locater::cal_x(double gradient_inv, double x0, double y0, double y) const
{
    return (y - y0) * gradient_inv + x0;
}

double Locater::cal_x(const cv::Vec4f& line, double y) const
{
    return (line[0] * (y - line[3]) / line[1]) + line[2];
}

bool Locater::length(const uchar* rope, isize_t width, int32_t& len) const
{
    len = 0;
    bool began = false, end = false;
    for (isize_t i = 0; i < width; ++i)
    {
        if (is_fg(rope[i]))
        {
            if (CS_BUNLIKELY(end))
            {
                return false;
            }
            else
            {
                if (!began)
                {
                    began = true;
                }
                ++len;
            }
        }
        else
        {
            if (began)
            {
                end = true;
            }
        }
    }
    return began;
}

void Locater::checkin(BoundList& bounds, const Bound& bound) const
{
    if (valid(bound))
    {
        bounds.push_back(bound);
    }
}

bool Locater::valid(const Bound& bound) const
{
    return staging::between<isize_t>(bound.width, opts.chart_min_width, opts.chart_max_width)
        && staging::between<isize_t>(bound.height, opts.chart_min_height, opts.chart_max_height);
}

bool Locater::is_fg(int32_t color) const
{
    return color == Config::black;
}

}
