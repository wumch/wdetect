
#pragma once

#ifdef HAVE_CONFIG_H
#	include "../config.h"
#endif

#ifndef CS_DEBUG
#   ifdef USE_WDETECT_DEBUG
#       define CS_DEBUG 2
#   else
#       define CS_DEBUG 0
#   endif
#endif

#include "meta.hpp"

#define WDT_NO_IM_DISPLAY 1

#if CS_DEBUG
#   include <opencv2/highgui/highgui.hpp>
#   define WDT_SHOW_IMG(img)                                         \
    do {                                                            \
        CS_SAY("show image: " << CS_STR_LITER(img));                \
        cv::namedWindow(CS_STR_LITER(img), CV_WINDOW_NORMAL);       \
        cv::imshow(CS_STR_LITER(img), img);                         \
        cv::waitKey();                                              \
    } while (false)
#else
#   define WDT_SHOW_IMG(img)
#endif
#if !WDT_NO_IM_DISPLAY
#   define WDT_IM_SHOW(img) WDT_SHOW_IMG(img)
#else
#   define WDT_IM_SHOW(img)
#endif

namespace wdt {

typedef int32_t isize_t;    // image-size type.
typedef int32_t digit_t;
typedef int32_t num_t;      // 各种(整数)数值
typedef double  rate_t;      // 百分比

}
