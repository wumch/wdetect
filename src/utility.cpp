
#include "utility.hpp"
#include <cmath>
#include <Magick++/Image.h>

namespace wdt {

int32_t TextAssit::width(const char* name, const std::string& font, int32_t font_size)
{
    Magick::Image img;
    img.font(font);
    img.fontPointsize(font_size);
    Magick::TypeMetric metric;
    img.fontTypeMetrics(name, &metric);
    return std::ceil(metric.textWidth());
}

} // namespace wdt
