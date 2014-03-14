
#pragma once

#ifndef HAVE_CONFIG_H
#   define HAVE_CONFIG_H 1
#endif
#include "../config.h"

#ifndef CS_DEBUG
#   ifdef USE_WDETECT_DEBUG
#       define CS_DEBUG 2
#   else
#       define CS_DEBUG 0
#   endif
#endif

#include "meta.hpp"

#define WDT_NO_IM_DISPLAY 0

#if CS_DEBUG && !WDT_NO_IM_DISPLAY
#   include <opencv2/highgui/highgui.hpp>
#   define WDT_IM_SHOW(img)                                         \
    do {                                                            \
        CS_SAY("show image: " << CS_STRINGIZE(img));                \
        cv::namedWindow(CS_STRINGIZE(img), CV_WINDOW_NORMAL);       \
        cv::imshow(CS_STRINGIZE(img), img);                         \
        cv::waitKey();                                              \
    } while (false)
#else
#   define WDT_IM_SHOW(img)
#endif

namespace wdt {

typedef int32_t isize_t;    // image-size type.
typedef int32_t digit_t;
typedef int32_t num_t;      // 各种(整数)数值
typedef double  rate_t;      // 百分比

}
