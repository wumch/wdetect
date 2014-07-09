
#include "detecter.hpp"
#include <cmath>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "preprocer.hpp"
#include "locater.hpp"
#include "integerdetecter.hpp"
#include "percentdetecter.hpp"
#include "scanner.hpp"

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
        CS_DUMP(rate);
        img.release();

        // 加载
        cv::Mat colored_img;
        colored_img = cv::imread(img_file, CV_LOAD_IMAGE_UNCHANGED);
        CS_DUMP(colored_img.cols);
        CS_DUMP(colored_img.rows);

        // 缩放
        cv::Mat scaled_img;
        cv::resize(
			colored_img, scaled_img,
			cv::Size2i(round(colored_img.cols * rate), round(colored_img.rows * rate)),
			0, 0,
//			CV_INTER_AREA
//			CV_INTER_CUBIC
			CV_INTER_LANCZOS4
			);
        colored_img.release();
        CS_DUMP(scaled_img.cols);
        CS_DUMP(scaled_img.rows);

        // 转灰度图
        cv::Mat gray_img(scaled_img.size(), CV_8UC1);
        cv::cvtColor(scaled_img, gray_img, cv::COLOR_BGR2GRAY);

        // 二值化
        if (!gray_img.empty())
        {
            cv::threshold(gray_img, img, 0, Config::white, CV_THRESH_BINARY | CV_THRESH_OTSU);
        }
        CS_DUMP(img.cols);
        CS_DUMP(img.rows);
#if CS_DEBUG
        cv::imwrite("/dev/shm/scaled.png", img);
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
        	img_file = opts.img_file;
            img = preprocer.binarize(img_file.c_str());
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


void Detecter::row_scan(const RowScanOpts& opts, ScanRes& res) const
{
	Scanner scanner(img);
	scanner.row_scan(opts, res);
}

void Detecter::col_scan(const ColScanOpts& opts, ScanRes& res) const
{
	Scanner scanner(img);
	scanner.col_scan(opts, res);
}

} // namespace wdt
