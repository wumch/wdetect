
#pragma once

#include "predef.hpp"
#include <opencv2/core/core.hpp>
#include "numdetecterbase.hpp"
#include "integerdetecter.hpp"
#include "percentdetecter.hpp"
#include "facade.hpp"

namespace wdt {

class Detecter
{
protected:
	std::string img_file;
    cv::Mat img;
    isize_t left, top;
    bool offsets;

public:
    Detecter();

    void prepare(const PrepareOpts& opts, PrepareRes& res);

    void scale(double rate);

    void locate(const ChartOpts& opts, ChartRes& res) const;

    void row_scan(const RowScanOpts& opts, ScanRes& res) const;
    void col_scan(const ColScanOpts& opts, ScanRes& res) const;

    void set_origin(isize_t left, isize_t top);

    template<NumDetecterKind kind>
    CS_FORCE_INLINE void detect(
        const typename NumDetecterTraits<kind>::OptsType& opts,
        typename NumDetecterTraits<kind>::ResType& res) const
    {
        NumDetecter<kind> detecter(img, opts, res);
        detecter.detect();
    }
};

} // namespace wdt
