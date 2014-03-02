
#include "recognizer.hpp"
#include "math.hpp"
#include "facade.hpp"
#if CS_DEBUG
#   include <opencv2/highgui/highgui.hpp>
#endif

namespace wdt {

void Recognizer::detect_vline(Sophist& sop) const
{
    int32_t col = 0, prev_line = -1;
    while (col < sop.cols)
    {
        if (is_vline(sop, col))
        {
            if (prev_line == -1 || col - prev_line >= Config::instance()->vline_min_gap)
            {
                if (sop.vline == Sophist::unknown_num)
                {
                    sop.vline = 1;
                }
                else
                {
                    ++sop.vline;
                }
                prev_line = col;
            }
        }
        col += 1;   // 一条线可能有2像素粗
    }
    CS_DUMP((int)sop.vline);
    if (sop.vline == Sophist::unknown_num)
    {
        sop.vline = 0;
    }
}

bool Recognizer::is_vline(const Sophist& sop, int32_t col) const
{
    int32_t gap = 0;
    for (int32_t row = 0, end = sop.rows; row < end - 1; ++row)
    {
        if (!sop.is_fg(col, row))
        {
            // 被相邻列连接 也可以(遵设置)
            if (Config::instance()->vline_adj)
            {
                if (CS_BLIKELY(!is_fg(sop, col - 1, row)))
                {
                    if (!is_fg(sop, col + 1, row))
                    {
                        if (++gap > Config::instance()->vline_max_break)
                        {
                            return false;
                        }
                    }
                }
            }
        }
    }
    return true;
}

void Recognizer::detect_hline(Sophist& sop) const
{
    WDT_IM_SHOW(sop.img);
    int32_t row = 0, prev_line = -1;
    while (row < sop.rows)
    {
        if (is_hline(sop, row))
        {
            if (prev_line == -1 || row - prev_line >= Config::instance()->hline_min_gap)
            {
                if (sop.hline == Sophist::unknown_num)
                {
                    sop.hline = 1;
                }
                else
                {
                    ++sop.hline;
                }
                prev_line = row;
            }
        }
        row += 1;   // 一条线可能有2像素粗
    }
    CS_DUMP((int)sop.hline);
    if (sop.hline == Sophist::unknown_num)
    {
        sop.hline = 0;
    }
}

bool Recognizer::is_hline(const Sophist& sop, int32_t row) const
{
    int32_t gap = 0;
    for (int32_t col = 0, end = sop.cols; col < end - 1; ++col)
    {
        if (!sop.is_fg(col, row))
        {
            // 被相邻行连接 也可以(遵设置)
            if (Config::instance()->hline_adj)
            {
                if (CS_BLIKELY(!is_fg(sop, col, row - 1)))
                {
                    if (!is_fg(sop, col, row + 1))
                    {
                        if (++gap > Config::instance()->hline_max_break)
                        {
                            return false;
                        }
                    }
                }
            }
        }
    }
    return true;
}

void Recognizer::detect_circle(Sophist& sop) const
{
    Point point((sop.cols >> 1) - 1, 0);

    bool prev_in_circle = false;
    int32_t continuous_begin = 0, continuous = 0;

    int32_t end = sop.rows;
    while (point.y < end)
    {
        if (in_circle(sop, point))
        {
            ++continuous;
            if (!prev_in_circle)
            {
                continuous_begin = point.y;
                prev_in_circle = true;
            }
        }
        else if (prev_in_circle)
        {
            if (is_fg(sop, point))
            {
                if (continuous >= Config::instance()->circle_min_diameter_v)
                {
                    if (sop.circle == Sophist::unknown_num)
                    {
                        Sophist::Pos pos = sop.cal_circle_pos(continuous_begin, point.y - 1);
                        if (pos != Sophist::unknown)
                        {
                            sop.circle = 1;
                            sop.circle_pos = pos;
                        }
                    }
                    else
                    {
                        ++sop.circle;
                    }
                }
            }
            prev_in_circle = false;
            continuous = 0;
        }
        ++point.y;
    }
    CS_DUMP((int)sop.circle);
    CS_DUMP(sop.circle_pos);
    if (sop.circle == Sophist::unknown_num)
    {
        sop.circle = 0;
    }
}

bool Recognizer::in_circle(const Sophist& sop, const Point& center) const
{
    if (is_fg(sop, center))
    {
        return false;
    }
    PointList::const_iterator it = directions.begin();
    directions_loop:
    while (it != directions.end())
    {
        if (hit(sop, center, *it))
        {
            ++it;
        }
        else
        {
            Point point(center + *it);
            while (inside(sop, point))
            {
                if (hit(sop, point, *it))
                {
                    ++it;
                    goto directions_loop;
                }
                point += *it;
            }
            return false;
        }
    }
    return true;
}

bool Recognizer::hit(const Sophist& sop, const Point& point, const Point& dir) const
{
    if (sop.is_fg(point))
    {
        return true;
    }
    Point next(point + dir);
    if (inside(sop, next))
    {
        if (sop.is_fg(next))
        {
            return true;
        }
        else if (dir.x != 0 && dir.y != 0)
        {
            if (sop.is_fg(next.x, point.y) && sop.is_fg(point.x, next.y))
            {
                return true;
            }
        }
    }
    return false;
}

bool Recognizer::inside(const Sophist& sop, const Point& point) const
{
    return point.x >= 0 && point.x < sop.cols
        && point.y >= 0 && point.y < sop.rows;
}

bool Recognizer::inside(const Sophist& sop, int32_t col, int32_t row) const
{
    return col >= 0 && col < sop.cols
        && row >= 0 && row < sop.rows;
}

bool Recognizer::is_fg(const Sophist& sop, const Point& point) const
{
    return CS_BLIKELY(inside(sop, point)) && sop.is_fg(point);
}

bool Recognizer::is_fg(const Sophist& sop, int32_t col, int32_t row) const
{
    return CS_BLIKELY(inside(sop, col, row)) && sop.is_fg(col, row);
}

// currently no need of this strict.
bool Recognizer::contain_island(const Sophist& sop) const
{
    return false;
}

digit_t Recognizer::recognize(Sophist sop) const
{
    if (is_dot(sop))
    {
        return Config::digit_dot;
    }
    else if (is_comma(sop))
    {
        return Config::digit_comma;
    }
    else if (CS_BLIKELY(!contain_island(sop)))
    {
        detect_hline(sop);
        if (sop.hline == 0)
        {
            detect_vline(sop);
            if (sop.vline == 0)
            {
                detect_circle(sop);
                if (sop.circle == 0)
                {
                    return 3;
                }
                else if (sop.circle == 1)
                {
                    switch (sop.circle_pos)
                    {
                    case Sophist::top:
                        return 9;
                        break;
                    case Sophist::middle:
                        return 0;
                        break;
                    case Sophist::bottom:
                        return 6;
                        break;
                    case Sophist::unknown:
                        break;
                    }
                }
                else if (sop.circle == 2)
                {
                    return 8;
                }
            }
            else if (sop.vline == 1)
            {
                return 1;
            }
        }
        else if (sop.hline == 1)
        {
            detect_vline(sop);
            if (sop.vline == 0)
            {
                if (sop.hline_pos == Sophist::bottom)
                {
                    return 2;
                }
                else if (sop.hline_pos == Sophist::top)     // 5,7
                {
                    return recognize_5_and_7(sop);
                }
            }
            else if (sop.vline == 1)
            {
                if (sop.hline_pos == Sophist::middle)
                {
                    return 4;
                }
                else if (sop.hline_pos == Sophist::bottom)
                {
                    return 1;
                }
            }
        }
    }
    return Config::invalid_digit;
}

bool Recognizer::is_comma(const Sophist& sop) const
{
    if (sop.rows == Config::instance()->comma_height && sop.cols == Config::instance()->comma_width)
    {
        int32_t area = 0;
        for (int32_t row = 0; row < sop.rows; ++row)
        {
            for (int32_t col = 0; col < sop.cols; ++col)
            {
                area += sop.is_fg(col, row);
            }
        }
        return area >= Config::instance()->comma_min_area;
    }
    return false;
}

bool Recognizer::is_dot(const Sophist& sop) const
{
    if (sop.rows == Config::instance()->dot_height && sop.cols == Config::instance()->dot_width)
    {
        int32_t area = 0;
        for (int32_t row = 0; row < sop.rows; ++row)
        {
            for (int32_t col = 0; col < sop.cols; ++col)
            {
                area += sop.is_fg(col, row);
            }
        }
        return area >= Config::instance()->dot_min_area;
    }
    return false;
}

digit_t Recognizer::recognize_5_and_7(const Sophist& sop) const
{
    bool blank_began = false;
    int32_t fg_threshold = round(sop.cols * 0.31);
    int32_t lr_delimiter = round(sop.cols * 0.48);
    int32_t maybe_5 = 0, maybe_7 = 0;
    for (int32_t row = 1, end = round(sop.rows * 0.5); row < end; ++row)
    {
        int32_t fgs = 0;
        int32_t col = 0, fg_col = -1;
        for ( ; col < sop.cols; ++col)
        {
            if (sop.is_fg(col, row))
            {
                ++fgs;
                fg_col = col;
            }
        }
        if (fgs <= fg_threshold)
        {
            if (blank_began)
            {
                if (fg_col == -1)
                {
                    return Config::invalid_digit;
                }
                else if (fg_col < lr_delimiter)
                {
                    ++maybe_5;
                }
                else
                {
                    ++maybe_7;
                }
            }
            else
            {
                blank_began = true;
            }
        }
    }
    if (maybe_5 > 1 && maybe_7 <= 1)
    {
        return 5;
    }
    else if (maybe_5 <= 1 && maybe_7 > 1)
    {
        return 7;
    }
    return Config::invalid_digit;
}

Recognizer::Recognizer()
{
    directions.reserve(8);
    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            if (!(x == 0 && y == 0))
            {
                directions.push_back(Point(x, y));
            }
        }
    }
}

Recognizer::~Recognizer()
{

}

} // namespace fdt
