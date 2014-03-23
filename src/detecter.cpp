
#include "detecter.hpp"
#include <cmath>
#include "preprocer.hpp"
#include "locater.hpp"
#include "integerdetecter.hpp"
#include "percentdetecter.hpp"

namespace wdt
{

Detecter::Detecter()
    : left(0), top(0), offsets(false)
{}

void Detecter::set_origin(isize_t left_, isize_t top_)
{
    left = left_;
    top = top_;
    offsets = !(left == 0 && top == 0);
}

void Detecter::scale(double rate)
{
    if (rate != 1.0)
    {
        CS_DUMP(img.cols);
        CS_DUMP(img.rows);
        cv::Mat scaled;
        CS_DUMP(rate);
        cv::resize(img, scaled, cv::Size2i(round(img.cols * rate), round(img.rows * rate)), 0, 0, CV_INTER_LINEAR);
        img.release();
        cv::threshold(scaled, img, 0, Config::white, CV_THRESH_BINARY | CV_THRESH_OTSU);
        CS_DUMP(img.cols);
        CS_DUMP(img.rows);
#if CS_DEBUG
        cv::imwrite("/dev/shm/a.png", img);
#endif
    }
}

void Detecter::prepare(const PrepareOpts& opts, PrepareRes& res)
{
    static const Preprocer preprocer;

    if (preprocer.check(opts, res))
    {
        try
        {
            img = preprocer.binarize(opts.img_file.c_str());
            res.code = success;
        }
        catch (const std::exception& e)
        {
            res.code = fo_img_content;
        }
    }
}

void Detecter::locate(const ChartOpts& opts, ChartRes& res) const
{
    Locater locater(img, opts, res);
    locater.locate();
}

} // namespace wdt
