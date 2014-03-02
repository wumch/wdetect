
#include "preprocer.hpp"
#include <cstdio>
#include <cmath>
#include <string>
#include <iostream>
extern "C" {
#include <TSRM/tsrm_virtual_cwd.h>
}
#if defined(HAVE_BOOST_FILESYSTEM) && HAVE_BOOST_FILESYSTEM
#   include <boost/filesystem/operations.hpp>
#endif
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "math.hpp"
#include "filesystem.hpp"
#include "facade.hpp"

namespace wdt {

void Preprocer::cal_bounds(const cv::Mat& bimg, BoundList& bounds) const
{
    ContourList contours;
    {
        cv::Mat shadow;
        bimg.copyTo(shadow);
        cv::findContours(shadow, contours, CV_RETR_LIST, CV_CHAIN_APPROX_NONE);
    }
    for (ContourList::iterator it = contours.begin(); it != contours.end(); ++it)
    {
        checkin(cv::boundingRect(*it), bounds);
    }
}

void Preprocer::checkin(const Bound& bound, BoundList& bounds) const
{
    if (valid(bound))
    {
        bounds.push_back(bound);
    }
}

// TODO: strict, no hard-code
bool Preprocer::valid(const Bound& bound) const
{
    static const isize_t min_height = std::min(
        Config::instance()->digit_height,
        std::min(Config::instance()->name_height, Config::instance()->title_height)
    ) - Config::instance()->y_err;
    static const isize_t max_height = std::max(
        Config::instance()->digit_height,
        std::max(Config::instance()->name_height, Config::instance()->title_height)
    ) + Config::instance()->y_err;
    return staging::between<isize_t>(bound.height, min_height, max_height)
        || staging::between<isize_t>(bound.height,
            Config::instance()->chart_min_height, Config::instance()->chart_max_height);
}

cv::Mat Preprocer::binarize(const char* img_file) const
{
    cv::Mat gray_img;
    try
    {
        gray_img = cv::imread(img_file, CV_LOAD_IMAGE_GRAYSCALE);
    }
    catch (const std::exception& e)
    {
        CS_ERR("error occured on cv::imread: " << e.what());
    }
    cv::Mat binary_img(gray_img.size(), CV_8UC1);
    if (!gray_img.empty())
    {
        cv::threshold(gray_img, binary_img, 0, Config::white, CV_THRESH_BINARY | CV_THRESH_OTSU);
    }
    return binary_img;
}

bool Preprocer::check_file_size(const char* img_file) const
{
#if defined(HAVE_BOOST_FILESYSTEM) && HAVE_BOOST_FILESYSTEM
    return boost::filesystem::exists(img_file) &&
        staging::between<ssize_t>(
            boost::filesystem::file_size(img_file),
            Config::instance()->img_file_min_size,
            Config::instance()->img_file_max_size);
#else
    return staging::between<ssize_t>(
        staging::filesize(img_file),
        Config::instance()->img_file_min_size,
        Config::instance()->img_file_max_size);
#endif
}

// See: http://www.cplusplus.com/forum/beginner/45217/ . thank that guy.
bool Preprocer::check_img_size(const char* img_file) const
{
    FILE *fp = VCWD_FOPEN(img_file, "rb");
    if (fp == NULL)
    {
        return false;
    }

    // Strategy:
    // reading GIF dimensions requires the first 10 bytes of the file
    // reading PNG dimensions requires the first 24 bytes of the file
    // reading JPEG dimensions requires scanning through jpeg chunks
    // In all formats, the file is at least 24 bytes big, so we'll read that always
    const int header_size = 24;
    uint8_t buf[header_size];
    if (fread(buf, sizeof(buf[0]), header_size, fp) != header_size)
    {
        fclose(fp);
        return false;
    }

    // For JPEGs, we need to read the first 12 bytes of each chunk.
    // We'll read those 12 bytes at buf+2...buf+14, i.e. overwriting the existing buf.
    if (buf[0] == 0xFF && buf[1] == 0xD8 && buf[2] == 0xFF && buf[3] == 0xE0
            && buf[6] == 'J' && buf[7] == 'F' && buf[8] == 'I'
            && buf[9] == 'F')
    {
        long pos = 2;
        while (buf[2] == 0xFF)
        {
            CS_DUMP((int)buf[3]);
            if (CS_IN(buf[3], 0xc0, 0xc1, 0xc2, 0xc3, 0xc9, 0xca, 0xcb))
            {
                break;
            }
            pos += 2 + (buf[4] << 8) + buf[5];
            fseek(fp, pos, SEEK_SET);
            if (fread(buf + 2, sizeof(uint8_t), 12, fp) != 12)
            {
                return false;
            }
        }
    }
    fclose(fp);

    ssize_t width = 0, height = 0;

    // JPEG: (first two bytes of buf are first two bytes of the jpeg file; rest of buf is the DCT frame
    if (buf[0] == 0xFF && buf[1] == 0xD8 && buf[2] == 0xFF)
    {
        height = (buf[7] << 8) + buf[8];
        width = (buf[9] << 8) + buf[10];
    }

    // GIF: first three bytes say "GIF", next three give version number. Then dimensions
    if (buf[0] == 'G' && buf[1] == 'I' && buf[2] == 'F')
    {
        width = buf[6] + (buf[7] << 8);
        height = buf[8] + (buf[9] << 8);
    }

    // PNG: the first frame is by definition an IHDR frame, which gives dimensions
    if (buf[0] == 0x89 && buf[1] == 'P' && buf[2] == 'N' && buf[3] == 'G'
            && buf[4] == 0x0D && buf[5] == 0x0A && buf[6] == 0x1A
            && buf[7] == 0x0A && buf[12] == 'I' && buf[13] == 'H'
            && buf[14] == 'D' && buf[15] == 'R')
    {
        width = (buf[16] << 24) + (buf[17] << 16) + (buf[18] << 8) + (buf[19] << 0);
        height = (buf[20] << 24) + (buf[21] << 16) + (buf[22] << 8) + (buf[23] << 0);
    }

    return check_size(width, height);
}

bool Preprocer::check_size(ssize_t width, ssize_t height) const
{
    return staging::between<ssize_t>(width, Config::instance()->img_min_width, Config::instance()->img_max_width)
        && staging::between<ssize_t>(height, Config::instance()->img_min_height, Config::instance()->img_max_height);
}

Preprocer::Preprocer()
{}

} // namespace wdt
