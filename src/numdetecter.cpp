
#include "numdetecter.hpp"
#include <boost/static_assert.hpp>
#include "facade.hpp"

namespace wdt {

const Recognizer NumDetecter::recognizer;

void NumDetecter::devide(const cv::Mat& frag, BoundList& bounds) const
{
    isize_t fg_begin = -1;
    bool prev_is_fg = false;
    for (isize_t col = 0; col < frag.cols; ++col)
    {
        if (separated(frag, col))
        {
            if (prev_is_fg)
            {
                prev_is_fg = false;
                bounds.push_back(Bound(fg_begin, 0, col - fg_begin, frag.rows));
            }
        }
        else
        {
            if (!prev_is_fg)
            {
                prev_is_fg = true;
                fg_begin = col;
            }
        }
    }

    //
    for (BoundList::iterator it = bounds.begin(); it != bounds.end(); ++it)
    {

    }
}

// TODO: is_fg...
bool NumDetecter::separated(const cv::Mat& frag, isize_t col) const
{
    BOOST_STATIC_ASSERT(sizeof(uchar) == sizeof(uint8_t));
    for (isize_t row = 0; row < frag.rows; ++row)
    {
        if (frag.ptr<uint8_t>(row)[col] != Config::white)
        {
            return false;
        }
    }
    return true;
}

bool NumDetecter::separated(const cv::Mat& frag, isize_t col, isize_t& left, isize_t& right) const
{

}

} // namespace wdt
