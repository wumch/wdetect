
#include "scanner.hpp"

namespace wdt {

void Scanner::row_scan(const RowScanOpts& opts, ScanRes& res) const
{
	res.pos = row_scan(opts);
	res.match = res.pos != invalid_pos;
	res.code = res.match ? wdt::success : wdt::failure;
}

isize_t Scanner::row_scan(const RowScanOpts& opts) const
{
	const isize_t
		row_begin = adjust_row(opts.low),
		row_end = adjust_row(opts.high),
		col_begin = adjust_col(opts.begin),
		col_end = adjust_col(opts.end);
	CS_DUMP(row_begin);
	CS_DUMP(row_end);
	CS_DUMP(col_begin);
	CS_DUMP(col_end);

	isize_t row = row_begin;
	const bool backward = (row_begin > row_end);
	const int step = backward ? -1 : 1;
	CS_DUMP(backward);

	bool is_margin;
	while ((!backward && row < row_end)
		|| (backward && row > row_end))
	{
		is_margin = is_margin_row(opts, row, col_begin, col_end);
		if ((opts.match_fg && is_margin)
			|| (!opts.match_fg && !is_margin))
		{
			row += step;
		}
		else
		{
			isize_t nm_end = adjust_row(row + step * opts.min_continuous);	// non-margin
			if (!(nm_end < img.cols))
			{
				return invalid_pos;
			}

			// check continuous.
			bool broken = false;
			bool _is_margin;

			row += step;
			isize_t nm_row = row;
			while ((!backward && nm_row < nm_end)
				|| (backward && nm_row > nm_end))
			{
				_is_margin = is_margin_row(opts, nm_row, col_begin, col_end);
				CS_SAY("checking: " << nm_row << " is " << (_is_margin ? "margin" : "not margin"));
				if ((opts.match_fg && _is_margin)
					|| (!opts.match_fg && !_is_margin))
				{
					row = nm_row + step;
					broken = true;
					break;
				}
				nm_row += step;
			}
			if (!broken)
			{
				return row - step;
			}
		}
	}
	return invalid_pos;
}

void Scanner::col_scan(const ColScanOpts& opts, ScanRes& res) const
{
	res.pos = col_scan(opts);
	res.match = res.pos != invalid_pos;
	res.code = res.match ? wdt::success : wdt::failure;
	CS_DUMP(res.pos);
	CS_DUMP(res.match);
	CS_DUMP(res.code);
}

isize_t Scanner::col_scan(const ColScanOpts& opts) const
{
	const isize_t
		col_begin = adjust_col(opts.low),
		col_end = adjust_col(opts.high),
		row_begin = adjust_row(opts.begin),
		row_end = adjust_row(opts.end);

	isize_t col = col_begin;
	const bool backward = (col_begin > col_end);
	const int step = backward ? -1 : 1;

	bool is_margin;
	while ((!backward && col < col_end)
		|| (backward && col > col_end))
	{
		is_margin = is_margin_col(opts, col, row_begin, row_end);
		if ((opts.match_fg && is_margin)
			|| (!opts.match_fg && !is_margin))
		{
			col += step;
		}
		else
		{
			// check continuous.
			bool broken = false;
			bool _is_margin;

			isize_t nm_end = adjust_col(col + step * opts.min_continuous);	// non-margin
			if (!(nm_end < img.cols))
			{
				return invalid_pos;
			}

			isize_t nm_col = col + step;
			while ((!backward && nm_col < nm_end)
				|| (backward && nm_col > nm_end))
			{
				_is_margin = is_margin_col(opts, nm_col, row_begin, row_end);
				CS_SAY("checking: " << nm_col << " is " << (_is_margin ? "margin" : "not margin"));
				if ((opts.match_fg && _is_margin)
					|| (!opts.match_fg && !_is_margin))
				{
					col = nm_col + step;
					broken = true;
					break;
				}
				nm_col += step;
			}
			if (!broken)
			{
				return col;
			}

			col += step;
		}
	}
	return invalid_pos;
}

bool Scanner::is_margin_row(const RowScanOpts& opts, isize_t row, isize_t left, isize_t right) const
{
//	const Config::BinaryColor margin_color = opts.match_fg ?
//		Config::bg(opts.inverse) : Config::fg(opts.inverse);
	const Config::BinaryColor margin_color = Config::bg(opts.inverse);
    int32_t fgs = 0;
    const uint8_t* pixels = img.ptr<uint8_t>(row) + left;
    for (const uint8_t* const end = pixels + (right - left); pixels != end; ++pixels)
    {
        if (CS_BUNLIKELY(*pixels != margin_color))
        {
            if (++fgs > opts.max_miss)
            {
                return false;
            }
        }
    }
    return true;
}

bool Scanner::is_margin_col(const ColScanOpts& opts, isize_t col, isize_t top, isize_t bottom) const
{
//	const Config::BinaryColor margin_color = opts.match_fg ?
//		Config::bg(opts.inverse) : Config::fg(opts.inverse);
	const Config::BinaryColor margin_color = Config::bg(opts.inverse);
    int32_t fgs = 0;
    for (isize_t row = top; row < bottom; ++row)
    {
        if (CS_BUNLIKELY(img.ptr<uint8_t>(row)[col] != margin_color))
        {
            if (++fgs > opts.max_miss)
            {
                return false;
            }
        }
    }
    return true;
}

} /* namespace wdt */
