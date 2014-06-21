
#include "predef.hpp"
#include <opencv2/core/core.hpp>
#include "facade.hpp"

namespace wdt {

class Scanner
{
private:
    const cv::Mat& img;

    static const isize_t invalid_pos = -1;

public:
	Scanner(const cv::Mat& img_)
		: img(img_)
	{}

    void row_scan(const RowScanOpts& opts, ScanRes& res) const;

    void col_scan(const ColScanOpts& opts, ScanRes& res) const;

protected:
    bool is_margin_row(const RowScanOpts& opts, isize_t row, isize_t left, isize_t right) const;

    bool is_margin_col(const ColScanOpts& opts, isize_t col, isize_t top, isize_t bottom) const;

    isize_t row_scan(const RowScanOpts& opts) const;
    isize_t col_scan(const ColScanOpts& opts) const;

    isize_t adjust_col(isize_t pos) const
    {
    	if (CS_BUNLIKELY(pos < 0))
    	{
    		pos = 0;
    	}
    	if (CS_BUNLIKELY(!(pos < img.cols)))
    	{
    		pos = img.cols - 1;
    	}
    	return pos;
    }

    isize_t adjust_row(isize_t pos) const
    {
    	if (CS_BUNLIKELY(pos < 0))
    	{
    		pos = 0;
    	}
    	if (CS_BUNLIKELY(!(pos < img.rows)))
    	{
    		pos = img.rows - 1;
    	}
    	return pos;
    }
};

} /* namespace wdt */
