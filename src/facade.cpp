
#include "facade.hpp"
#include <climits>
#include <cstdlib>
#include <cstring>
#include <string>
#include "misc.hpp"
#include "cvdef.hpp"
#include "preprocer.hpp"
#include "chartdetecter.hpp"
#include "textdetecter.hpp"

namespace wdt {

void Config::set_img_file_max_size(const char* size_str, size_t len)
{
    static const int32_t max_bytes = (CS_CONST_STRLEN(CS_STRINGIZE(INT_MAX)) + 1);
    if (len == 0 || len > max_bytes)
    {
        img_file_max_size = 0;
    }
    else
    {
        size_t bit;
        switch (size_str[len-1])
        {
        case 'k':
        case 'K':
            bit = 10;
            break;
        case 'm':
        case 'M':
            bit = 20;
            break;
        case 'g':
        case 'G':
            bit = 30;
            break;
        default:
            bit = 0;
            break;
        }
        char buff[max_bytes];
        memset(buff, 0, max_bytes);
        strncpy(buff, size_str, len - !!bit);
        img_file_max_size = atoi(buff) << bit;
    }
}

DetectResult wdetect(const char* img_file, const char* name, const std::string& title, wdt::DetectResult& res)
{
    static const Preprocer preprocer;

    BoundList* bounds = new BoundList;  // TODO: where is the bug??
    if (preprocer.check(img_file))
    {
        const cv::Mat bimg = preprocer.binarize(img_file);
        CS_DUMP(bimg.rows);

        preprocer.cal_bounds(bimg, *bounds);
        CS_DUMP(bounds->size());

        ChartDetecter cder(bimg, *bounds, res);
        cder.detect();

        CS_DUMP(cder.chart_tl());

        if (res.chart_code == success)
        {
            TextDetecter tder(bimg, name, title, *bounds, cder.chart_tl(), res);
            tder.detect();
        }
    }
#if CS_DEBUG
    else
    {
        CS_DUMP("yun");
        CS_ERR("image file invalid: [" << img_file << "]");
    }
#endif
    return res;
}

}
