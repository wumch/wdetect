
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
                if (detected)
                {
                    if (it->y < box.y)
                    {
                        box = *it;
                    }
                }
                else
                {
                    detected = true;
                    box = *it;
                }
            }
        }
    }

    if (detected)
    {
        calc_width(box);
        calc_height(box);
    }
    else
    {
        res.code = fo_no_match;
    }
}

void Locater::calc_height(const Bound& box)
{
    static const int32_t min_continuous_fg_rows = 4;
    const isize_t left = box.x;
    int32_t fg_cols = 0;
    bool found = false;
    CS_DUMP(box.x);
    CS_DUMP(box.y);
    CS_DUMP(box.width);
    CS_DUMP(box.height);
    const isize_t top = std::max(box.y + opts.chart_min_margin_bottom, 0),
        bottom = std::min(box.y + opts.chart_max_margin_bottom + min_continuous_fg_rows + 2, img.rows);

    for (isize_t row = top; row < bottom; ++row)
    {
        CS_DUMP(row);
        if (is_margin_row(row, left, opts.chart_height_scan_width))
        {
            CS_DUMP(fg_cols);
            if (++fg_cols >= min_continuous_fg_rows)
            {
                found = true;
                res.chart_height = row - fg_cols - box.y;
                break;
            }
        }
        else
        {
            if (fg_cols)
            {
                fg_cols = 0;
            }
        }
    }
    if (!found)
    {
        res.code = fo_calc_chart_height;
    }
}

bool Locater::is_margin_row(isize_t row, isize_t left, isize_t width) const
{
    int32_t fgs = 0;
    const uint8_t* pixels = img.ptr<uint8_t>(row) + left;
    for (const uint8_t* const end = pixels + width; pixels != end; ++pixels)
    {
        if (CS_BUNLIKELY(is_fg(*pixels)))
        {
            if (++fgs > opts.chart_margin_max_fg)
            {
                return false;
            }
        }
    }
    return true;
}

void Locater::calc_width(const Bound& box)
{
    static const int32_t min_continuous_fg_cols = 4;
    isize_t top = box.y, bottom = box.y + box.height;
    CS_DUMP(top);
    CS_DUMP(bottom);
    CS_DUMP(box.x);
    CS_DUMP(box.width);
    int32_t fg_cols = 0;
    bool found = false;
    for (isize_t right = box.x + box.width,
            col = right + opts.chart_min_margin_right,
            end = std::min(right + opts.chart_max_margin_right + min_continuous_fg_cols + 2, img.cols);
        col < end; ++col)
    {
        CS_DUMP(col);
        if (is_margin_col(col, top, bottom))
        {
            if (fg_cols)
            {
                fg_cols = 0;
            }
        }
        else
        {
            CS_DUMP(fg_cols);
            if (++fg_cols >= min_continuous_fg_cols)
            {
                found = true;
                res.chart_width = col - fg_cols - box.x;
                break;
            }
        }
    }
    if (!found)
    {
        res.code = fo_calc_chart_width;
    }
}

bool Locater::is_margin_col(isize_t col, isize_t top, isize_t bottom) const
{
    int32_t fgs = 0;
    for (isize_t row = top; row < bottom; ++row)
    {
        if (CS_BUNLIKELY(is_fg(img.ptr<uint8_t>(row)[col])))
        {
            if (++fgs > opts.chart_margin_max_fg)
            {
                return false;
            }
        }
    }
    return true;
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

    isize_t rope, prev_rope = 0;
    isize_t gradient_conform = 0;
    enum gradient_sign_t { negative = -1, zero_or_pending = 0, positive = 1, invalid = -2};
    gradient_sign_t gradient_sign = invalid, gradient_sign_prob = invalid; // 斜率符号（-1/0/1有效, -2无效)
    int32_t wrong_row = 0;
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
                CS_SAY("new echelon began at [" << row << "]");
                prev_valid = true;
                gradient_sign_prob = invalid;
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
                        if (gradient_sign_prob == negative)
                        {
                            ++gradient_conform;
                        }
                        else
                        {
                            gradient_sign_prob = negative;
                            gradient_conform = 1;
                        }
                    }
                    else if (rope < prev_rope)
                    {
                        if (gradient_sign_prob == positive)
                        {
                            ++gradient_conform;
                        }
                        else
                        {
                            gradient_sign_prob = positive;
                            gradient_conform = 1;
                        }
                    }
                    else
                    {
                        if (gradient_sign_prob == zero_or_pending)
                        {
                            ++gradient_conform;
                        }
                        else if (gradient_sign_prob == invalid)
                        {
                            gradient_sign_prob = zero_or_pending;
                            gradient_conform = 1;
                        }   // 斜率不为0时也允许相邻行长度相同。
                    }
                    if (gradient_conform >= opts.echelon_gradient_min_continuous)
                    {
                        gradient_sign = gradient_sign_prob;
                    }
                }
                else
                {
                    if (CS_BUNLIKELY(!((rope >= prev_rope && gradient_sign == negative)
                        || (rope <= prev_rope && gradient_sign == positive)
                        || (rope == prev_rope && gradient_sign == zero_or_pending))))
                    {
                        CS_DUMP(rope);
                        CS_DUMP(prev_rope);
                        CS_DUMP(gradient_sign);
                        CS_DUMP(wrong_row);
                        if (++wrong_row > opts.echelon_max_wrong_row)
                        {
                            return false;
                        }
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
        if (static_cast<ssize_t>(it->size()) < eche_min_height)
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
    CS_RETURN_IF(echelons.empty() || static_cast<ssize_t>(echelons.size()) > opts.echelons, false);
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
    return color == fg;
}

}
