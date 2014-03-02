
#pragma once

#include "predef.hpp"
#include <vector>
#include <opencv2/core/core.hpp>

namespace wdt {

typedef cv::Point2i Point;
typedef std::vector<Point> PointList;

typedef PointList Contour;  // 轮廓
typedef std::vector<Contour> ContourList;

typedef cv::Rect Bound; // 边界
typedef std::vector<Bound> BoundList;

typedef std::vector<cv::Mat> ImageList;
typedef std::vector<digit_t> DigitList;

typedef BoundList Paragraph;     // 字符片段

}
