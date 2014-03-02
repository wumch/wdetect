
#pragma once

#include "predef.hpp"

namespace wdt {

class TextAssit
{
public:
    // 计算文本宽度
    static int32_t width(const char* text, const std::string& font, int32_t size);
};

} // namespace wdt
