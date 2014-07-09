
#include "recognizer.hpp"
#include "math.hpp"
#include "facade.hpp"

namespace wdt {

void Recognizer::detect_vline(Sophist& sop) const
{
    int32_t col = 0, prev_line = -1;
    while (col < sop.cols)
    {
        if (is_vline(sop, col))
        {
            if (prev_line == -1 || col - prev_line >= sop.opts.vline_min_gap)
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
        col += 1;
    }
    if (sop.vline == Sophist::unknown_num)
    {
        sop.vline = 0;
    }
}

bool Recognizer::is_vline(const Sophist& sop, int32_t col) const
{
    int32_t gap = 0;
    for (int32_t row = 0, end = sop.rows; row < end; ++row)
    {
        if (!sop.is_fg(col, row))
        {
            // 被相邻列连接 也可以(遵设置)
            if (sop.opts.vline_adj)
            {
                if (CS_BLIKELY(!is_fg(sop, col - 1, row)))
                {
                    if (!is_fg(sop, col + 1, row))
                    {
                        if (++gap > sop.opts.vline_max_break)
                        {
                            return false;
                        }
                    }
                }
            }
            else
            {
                return false;
            }
        }
    }
    return true;
}

void Recognizer::detect_hline(Sophist& sop) const
{
    int32_t row = 0, prev_line = -1;
    while (row < sop.rows)
    {
        if (is_hline(sop, row))
        {
            if (prev_line == -1 || row - prev_line >= sop.opts.hline_min_gap)
            {
                if (sop.hline == Sophist::unknown_num)
                {
                    Sophist::Pos pos = sop.calc_hline_pos(row + 1);
                    if (pos != Sophist::unknown)
                    {
                        sop.hline = 1;
                        sop.hline_pos = pos;
                    }
                }
                else
                {
                    ++sop.hline;
                }
                prev_line = row;
            }
        }
        row += 1;
    }
    if (sop.hline == Sophist::unknown_num)
    {
        sop.hline = 0;
    }
}

bool Recognizer::is_hline(const Sophist& sop, int32_t row) const
{
    int32_t gap = 0;
    for (int32_t col = 0; col < sop.cols; ++col)
    {
        if (!sop.is_fg(col, row))
        {
            // 被相邻行连接 也可以(遵设置)
            if (sop.opts.hline_adj)
            {
                if (CS_BLIKELY(!is_fg(sop, col, row - 1)))
                {
                    if (!is_fg(sop, col, row + 1))
                    {
                        if (++gap > sop.opts.hline_max_break)
                        {
                            return false;
                        }
                    }
                }
            }
            else
            {
                return false;
            }
        }
    }
    return true;
}

void Recognizer::detect_circle(Sophist& sop) const
{
	static const int32_t margin = 2;
	static const int32_t width_threshold = (margin << 1) + 1;
	if (width_threshold < sop.cols)
	{
		CircleResult best_res, res;
		for (int32_t col = margin, end = sop.cols - margin; col < end; ++col)
		{
			res = detect_circle_incol(sop, col);
			if (res.total_diameter > best_res.total_diameter)
			{
				best_res = res;
			}
		}
		sop.circle = (best_res.num == Sophist::unknown_num ? 0 : best_res.num);
		sop.circle_pos = best_res.first_pos;
	}
	else
	{
		CircleResult res = detect_circle_incol(sop, (sop.cols - 1) >> 1);
		sop.circle = res.num;
		sop.circle_pos = res.first_pos;
	}
	CS_DUMP((int)sop.circle);
	CS_DUMP((int)sop.circle_pos);
}

Recognizer::CircleResult Recognizer::detect_circle_incol(const Sophist& sop, int32_t col) const
{
    Point point(col, 0);

    bool prev_in_circle = false;
    int32_t continuous_begin = 0, continuous = 0;

    CircleResult res;
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
                if (continuous >= sop.opts.circle_min_diameter_v)
                {
                    if (res.num == Sophist::unknown_num)
                    {
                        Sophist::Pos pos = Sophist::calculate_circle_pos(sop, continuous_begin + 1, point.y);
                        if (pos != Sophist::unknown)
                        {
                        	res.total_diameter += point.y - continuous_begin;
                            res.num = 1;
                            res.first_pos = pos;
                        }
                    }
                    else
                    {
                    	res.total_diameter += point.y - continuous_begin;
                        ++res.num;
                    }
                }
            }
            prev_in_circle = false;
            continuous = 0;
        }
        ++point.y;
    }
    if (res.num == Sophist::unknown_num)
    {
        res.num = 0;
    }

    return res;
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

digit_t Recognizer::recognize(Sophist& sop, Sophist::Erode erode) const
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
    	return recognize_digit(sop, erode);
    }
    return Config::invalid_digit;
}

digit_t Recognizer::recognize_digit(Sophist& sop, Sophist::Erode erode) const
{
	digit_t digit = recognize_digit(sop);
	if (digit == Config::invalid_digit)
	{
		if (crop(sop, erode))
		{
			digit = recognize_digit(sop);
		}
	}
	return digit;
}

bool Recognizer::crop(Sophist& sop, Sophist::Erode erode) const
{
	if (sop.img.cols > 1)
	{
		if (erode == Sophist::left)
		{
			CS_DUMP(sop.img.cols);
			sop.img = sop.img(cv::Rect(1, 0, sop.img.cols - 1, sop.img.rows));
			CS_DUMP(sop.img.cols);
			return true;
		}
		else if (erode == Sophist::right)
		{
			CS_DUMP(sop.img.cols);
			sop.img = sop.img(cv::Rect(0, 0, sop.img.cols - 1, sop.img.rows));
			CS_DUMP(sop.img.cols);
			return true;
		}
	}
	return false;
}

digit_t Recognizer::recognize_digit(Sophist& sop) const
{
	detect_vline(sop);
	detect_hline(sop);
	detect_circle(sop);

	CS_DUMP((int)sop.circle);
	CS_DUMP((int)sop.circle_pos);
	CS_DUMP((int)sop.hline);
	CS_DUMP((int)sop.hline_pos);
	CS_DUMP((int)sop.vline);

	if (sop.circle == 2)
	{
		return 8;
	}

	if (sop.circle == 1)
	{
        switch (sop.circle_pos)
        {
        case Sophist::top:
        	if (sop.hline == 1 && sop.vline == 1)
        	{
        		return 4;
        	}
        	else
        	{
        		return 9;
        	}
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

	if (sop.circle == 0)
	{
		if (sop.hline == 1)
		{
			if (sop.hline_pos == Sophist::top)
			{
				return recognize_5_and_7(sop);
			}
			else if (sop.hline_pos == Sophist::middle)
			{
				return 4;
			}
			else if (sop.hline_pos == Sophist::bottom)
			{
				if (sop.vline == 1)
				{
					return 1;
				}
				else if (sop.vline == 0)
				{
					return 2;
				}
			}
		}
		else if (sop.hline == 0)
		{
			if (sop.vline == 0)
			{
				return 3;
			}
		}
		else if (sop.hline == 2)
		{
			if (sop.hline_pos == Sophist::top)
			{
				switch (recognize_5_and_7(sop))
				{
				case 5:
					return 5;
					break;
				case 7:
					return 2;
					break;
				}
			}
		}
	}

	// for scaled images, sometimes "3" contains a vertical line.
	if (sop.vline == 1)
	{
		return 3;
	}

	return Config::invalid_digit;
}

digit_t Recognizer::recognize_useless(Sophist& sop) const
{
    if (is_comma(sop))
    {
        return Config::digit_comma;
    }
    else if (is_dot(sop))
    {
        return Config::digit_dot;
    }
    else if (!CS_BUNLIKELY(contain_island(sop)))
    {
        detect_hline(sop);
        CS_DUMP((int)sop.hline);
        CS_DUMP((int)sop.hline_pos);
        if (sop.hline == 0)
        {
            detect_vline(sop);
            CS_DUMP((int)sop.vline);
            detect_circle(sop);
            CS_DUMP((int)sop.circle);
            if (sop.vline == 0)
            {
                CS_DUMP((int)sop.circle);
                CS_DUMP((int)sop.circle_pos);
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
            if (sop.circle == 1) {
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
//            else if (sop.vline == 1)
//            {
//                return 1;
//            }
        }
        else if (sop.hline == 1)
        {
            detect_vline(sop);
            CS_DUMP((int)sop.vline);
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
                else
                {
                    detect_circle(sop);
                    if (sop.circle == 1)
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
        else if (sop.hline == 2)
        {
            if (sop.hline_pos == Sophist::top)
            {
                if (recognize_5_and_7(sop) == 5)
                {
                    return 5;
                }
                else
                {
                	return 3;
                }
            }
        }
    }
    CS_SAY("recognize failed:");
    WDT_SHOW_IMG(sop.img);
    return Config::invalid_digit;
}

bool Recognizer::is_comma(const Sophist& sop) const
{
    if (sop.rows && sop.rows <= sop.opts.comma_max_height &&
    		sop.rows >= sop.opts.comma_min_height &&
        sop.cols && sop.cols <= sop.opts.comma_max_width)
    {
        int32_t area = 0;
        for (int32_t row = 0; row < sop.rows; ++row)
        {
            for (int32_t col = 0; col < sop.cols; ++col)
            {
                area += sop.is_fg(col, row);
            }
        }
        return area >= sop.opts.comma_min_area;
    }
    return false;
}

bool Recognizer::is_dot(const Sophist& sop) const
{
    if (sop.rows && sop.rows <= sop.opts.dot_max_height &&
        sop.cols && sop.cols <= sop.opts.dot_max_width)
    {
        int32_t area = 0;
        for (int32_t row = 0; row < sop.rows; ++row)
        {
            for (int32_t col = 0; col < sop.cols; ++col)
            {
                area += sop.is_fg(col, row);
            }
        }
        return area >= sop.opts.dot_min_area;
    }
    return false;
}

digit_t Recognizer::recognize_5_and_7(const Sophist& sop) const
{
    bool blank_began = false;
    const int32_t fg_threshold = round(sop.cols * 0.31);
    const int32_t lr_delimiter = round(sop.cols * 0.48);
    CS_DUMP(fg_threshold);
    CS_DUMP(lr_delimiter);
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
        CS_DUMP(fgs);
        CS_DUMP(fg_col);
        if (fgs <= fg_threshold)
        {
            blank_began = true;
            if (fg_col != -1)
            {
				if (fg_col < lr_delimiter)
				{
					++maybe_5;
				}
				else
				{
					++maybe_7;
				}
            }
        }
        else
        {
            if (blank_began)
            {
                break;
            }
        }
    }
    CS_DUMP(maybe_5);
    CS_DUMP(maybe_7);
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
