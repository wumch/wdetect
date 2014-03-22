
#pragma once

#include "predef.hpp"
#if CS_DEBUG
#   include <cstdio>
#endif
#include <vector>
#include <algorithm>
#include <utility>
#include <boost/static_assert.hpp>
#include "cvdef.hpp"
#include "facade.hpp"

namespace wdt {

class PositedImageList
{
public:
    ImageList imgs;
    PointList poses;
    isize_t y_mode;

    PositedImageList(const ImageList& imgs_, const BoundList& bounds)
        : imgs(imgs_), poses(extract_tl(bounds)), y_mode(calc_y_mode())
    {}

    PositedImageList()
    {}

    void push_back(const cv::Mat& img, const Point& point)
    {
        imgs.push_back(img);
        poses.push_back(point);
    }

    void pop_back()
    {
        imgs.pop_back();
        poses.pop_back();
    }

    void assemble()
    {
        y_mode = calc_y_mode();
    }

    void reserve(size_t size)
    {
        imgs.reserve(size);
        poses.reserve(size);
    }

protected:
    PointList extract_tl(const BoundList& bounds) const
    {
        PointList points;
        for (BoundList::const_iterator it = bounds.begin(); it != bounds.end(); ++it)
        {
            points.push_back(it->tl());
        }
        return points;
    }

    typedef std::map<isize_t, int32_t> YCount;
    static const class YPairCmper
    {
    public:
        YPairCmper() {}
        bool operator()(const YCount::iterator& left, const YCount::iterator& right) const
        {
            return right->second < left->second;
        }
    } y_pair_cmper;

    isize_t calc_y_mode() const
    {
        CS_RETURN_IF(poses.empty(), Config::invalid_y_mode);
        YCount ycount;
        for (PointList::const_iterator it = poses.begin(); it != poses.end(); ++it)
        {
            YCount::iterator yit = ycount.find(it->y);
            if (yit == ycount.end())
            {
                ycount.insert(std::make_pair(it->y, 1));
            }
            else
            {
                ++yit->second;
            }
        }
        return std::max_element(ycount.begin(), ycount.end(), y_pair_cmper)->first;
    }
};

template<NumDetecterKind kind>
class Divider
{
protected:
    typedef typename NumDetecterTraits<kind>::OptsType OptsType;
    typedef int32_t mark_t;
    BOOST_STATIC_ASSERT(sizeof(mark_t) >= sizeof(pix_t));
    typedef std::vector<mark_t> MarkList;

    static const mark_t invalid_mark = 0;

    const cv::Mat& img;
    const OptsType& opts;
    PositedImageList& pils;

    cv::Mat shadow;

    cv::Mat emap;

    MarkList mark_map;

    // NOTE: the only way to examine whether a `pixel` should be ignored or not is `pixel == bg`.
    mark_t last_mark;
    const Config::BinaryColor fg;
    const Config::BinaryColor bg;

    const isize_t col_idx_max;

public:
    explicit Divider(const cv::Mat& img_, const OptsType& opts_, PositedImageList& res_)
        : img(img_), opts(opts_), pils(res_), last_mark(0),
          fg(Config::fg(opts.inverse)), bg(Config::bg(opts.inverse)),
          col_idx_max(img.cols - 1)
    {}

    void divide()
    {
        if (CS_BLIKELY(img.cols > 1 && img.rows > 1))
        {
            img.copyTo(shadow);
            mark();
            separate();
            delimit();
#if CS_DEBUG
            dump();
#endif
        }
    }

protected:
    typedef std::pair<mark_t, Bound> MarkPos;

    static const class MarkPosCmper
    {
    public:
        bool operator()(const MarkPos& left, const MarkPos& right) const
        {
            return left.second.x < right.second.x;
        }
    } mark_pos_cmper;

    void separate()
    {
        typedef std::vector<MarkPos> MarkPosList;
        MarkPosList poses;

        for (mark_t cur_mark = last_mark; cur_mark > invalid_mark; --cur_mark)
        {
            if (mark_map[cur_mark] != invalid_mark)
            {
                continue;
            }
            PointList contour;
            for (isize_t row = 0; row < shadow.rows; ++row)
            {
                pix_t* px = shadow.ptr<pix_t>(row);
                for (isize_t col = 0; col < shadow.cols; ++col)
                {
                    if (px[col] == cur_mark)
                    {
                        contour.push_back(Point(col, row));
                    }
                }
            }

            if (contour.size())
            {
                poses.push_back(std::make_pair(cur_mark, cv::boundingRect(contour)));
            }
        }

        pils.reserve(poses.size());
        std::sort(poses.begin(), poses.end(), mark_pos_cmper);

        for (MarkPosList::const_iterator it = poses.begin(); it != poses.end(); ++it)
        {
            const cv::Mat mask = shadow(it->second);
            pils.push_back(cv::Mat(it->second.size(), CV_8UC1), it->second.tl());
            cv::Mat& res_img = *pils.imgs.rbegin();
            const mark_t cur_mark = it->first;
            for (isize_t row = 0; row < mask.rows; ++row)
            {
                const pix_t* px = mask.ptr<pix_t>(row);
                pix_t* res_px = res_img.ptr<pix_t>(row);
                for (const pix_t* const end = px + mask.cols; px != end; ++px, ++res_px)
                {
                    *res_px = (*px == cur_mark) ? fg : bg;
                }
            }
        }
    }

    void delimit()
    {
        for (int32_t i = 0, end = pils.imgs.size() - (kind == percent); i < end; )
        {
            if (pils.imgs[i].cols > opts.digit_max_width)
            {
                delimit(i);
            }
            else
            {
                ++i;
            }
        }
    }

    static const int32_t invalid_delimiter = -1;

    void delimit(const int32_t img_idx)
    {
        const isize_t delimiter = get_delimiter(pils.imgs[img_idx]);
        if (delimiter != invalid_delimiter)
        {
            WDT_IM_SHOW(pils.imgs[img_idx]);
            CS_DUMP(delimiter);

            const isize_t right_begin = delimiter + 1;
            if (0 < delimiter && right_begin < pils.imgs[img_idx].cols)
            {
                {
                    cv::Mat tmp;
                    pils.imgs[img_idx](Bound(right_begin, 0, pils.imgs[img_idx].cols - right_begin, pils.imgs[img_idx].rows)).copyTo(tmp);
                    pils.imgs.insert(pils.imgs.begin() + img_idx + 1, tmp);
                }
                {
                    cv::Mat tmp;
                    pils.imgs[img_idx](Bound(0, 0, delimiter, pils.imgs[img_idx].rows)).copyTo(tmp);
                    pils.imgs[img_idx] = tmp;
                }

                pils.poses.insert(pils.poses.begin() + img_idx + 1, Point(pils.poses[img_idx].x + right_begin, pils.poses[img_idx].y));
                WDT_IM_SHOW(pils.imgs[img_idx + 1]);
                WDT_IM_SHOW(pils.imgs[img_idx]);
            }
        }
    }

    isize_t get_delimiter(const cv::Mat& img) const
    {
        isize_t begin = opts.digit_min_width - 1, end = std::min((opts.digit_max_width + 1) + 1, img.cols);
        CS_DUMP(begin);
        CS_DUMP(end);
        CS_RETURN_IF(!(0 < begin && opts.digit_max_width < end && end - begin >= 3), -1);
        typedef std::vector<isize_t> PixesList;
        PixesList pixes_list(begin, img.rows);
        pixes_list.reserve(end - begin);
        for (isize_t col = begin; col < end; ++col)
        {
            pixes_list.push_back(col_pixes(img, col));
        }

        PixesList::iterator it = std::min_element(pixes_list.begin() + 1, pixes_list.end() - 1);
        for (int32_t i = begin + 1; i < end - 1; ++i)
        {
            if (pixes_list[i] == *it)
            {
                if ((pixes_list[i] == 0)      // reliable
                    || (pixes_list[i] == 1 && (pixes_list[i - 1] > 1 || pixes_list[i + 1] > 1))   // reasonable..
                    || (pixes_list[i] < pixes_list[i - 1] || pixes_list[i] < pixes_list[i + 1])     // TODO: maybe..
                    )
                {
                    return i;
                }
            }
        }
        return invalid_delimiter;
    }

    isize_t col_pixes(const cv::Mat& img, isize_t col) const
    {
        isize_t pixes = 0;
        for (isize_t row = 0; row < img.rows; ++row)
        {
            pixes += (img.ptr<uint8_t>(row)[col] == fg);
        }
        return pixes;
    }

    void mark()
    {
        smark_top();
        smark_left();
        smark_center();
        merge();
    }

    void merge()
    {
        calc_map();
        for (isize_t row = 0; row < shadow.rows; ++row)
        {
            pix_t* px = shadow.ptr<pix_t>(row);
            for (const pix_t* const end = px + shadow.cols; px != end; ++px)
            {
                CS_PREFETCH(px, 1, 1);
                if (*px != bg)
                {
                    if (mark_map[*px] != invalid_mark)
                    {
                        *px = mark_map[*px];
                    }
                }
            }
        }
    }

    class Around
    {
    public:
        const Divider& d;
        mark_t l, tl, t, tr;

        Around(const Divider& divider)
            : d(divider)
        {}

        Around(const Divider& divider, isize_t row, isize_t col)
            : d(divider),
              l(d.calc_mark(row, col - 1)),
              tl(d.calc_mark(row - 1, col - 1)),
              t(d.calc_mark(row - 1, col)),
              tr(col == d.col_idx_max ? invalid_mark : d.calc_mark(row - 1, col + 1))
        {}

        Around(const Divider& divider, isize_t row, isize_t col, mark_t left_mark)
            : d(divider),
              l(d.calc_mark(row, col - 1)),
              tl(d.calc_mark(row - 1, col - 1)),
              t(d.calc_mark(row - 1, col)),
              tr(col == d.col_idx_max ? invalid_mark : d.calc_mark(row - 1, col + 1))
        {}

        void reset(isize_t row, isize_t col)
        {
            l = d.calc_mark(row, col - 1);
            tl = d.calc_mark(row - 1, col - 1);
            t = d.calc_mark(row - 1, col);
            tr = (col == d.col_idx_max ? invalid_mark : d.calc_mark(row - 1, col + 1));
        }

        void reset(isize_t row, isize_t col, mark_t left_mark)
        {
            l = d.calc_mark(row, col - 1);
            tl = d.calc_mark(row - 1, col - 1);
            t = d.calc_mark(row - 1, col);
            tr = (col == d.col_idx_max ? invalid_mark : d.calc_mark(row - 1, col + 1));
        }

        int32_t mark_count() const
        {
            return (l != invalid_mark) + (tl != invalid_mark) + (t != invalid_mark) + (tr != invalid_mark);
        }

        mark_t which() const
        {
            if (l != invalid_mark)
            {
                return l;
            }
            if (tl != invalid_mark)
            {
                return tl;
            }
            if (t != invalid_mark)
            {
                return t;
            }
            assert(tr != invalid_mark);
            return tr;
        }
    };

#define _WDT_DUMP_PX(COL, ROW) CS_SAY("pixel (" << COL << "," << ROW << "): " << (int)*px_ptr(ROW, COL));

    // scan and mark center area.
    void smark_center()
    {
        Around around(*this);
        _WDT_DUMP_PX(79, 11);
        for (isize_t row = 1; row < shadow.rows; ++row)
        {
            pix_t* px = px_ptr(row, 1);
            for (isize_t col = 1; col < shadow.cols; ++col, ++px)
            {
                CS_PREFETCH(px, 1, 1);
                if (*px == fg)
                {
                    around.reset(row, col);
                    mark_center_px(px, around);
                }
            }
        }
    }

    void mark_center_px(pix_t* px, const Around& a)
    {
        mark_t mark_val;
        const int32_t n_mark = a.mark_count();
        if (n_mark == 0)
        {
            *px = mark_val = new_mark();
        }
        else if (n_mark == 1)
        {
            *px = mark_val = a.which();
        }
        else
        {
            bool found = false;
            if (a.l != invalid_mark)
            {
                *px = mark_val = a.l;
                found = true;
            }
            if (a.tl != invalid_mark)
            {
                if (found)
                {
                    record_equa(mark_val, a.tl);
                    mark_val = a.tl;
                }
                else
                {
                    *px = mark_val = a.tl;
                    found = true;
                }
            }
            if (a.t != invalid_mark)
            {
                if (found)
                {
                    record_equa(mark_val, a.t);
                    mark_val = a.t;
                }
                else
                {
                    *px = mark_val = a.t;
                    found = true;
                }
            }
            if (a.tr != invalid_mark)
            {
                if (found)
                {
                    record_equa(mark_val, a.tr);
                }
                else
                {
                    *px = a.tr;
                }
            }
        }
    }

    // scan and mark the first column.
    void smark_left()
    {
        mark_t above_mark;
        pix_t* px = px_ptr(1, 0);
        CS_PREFETCH(px, 1, 1);
        if (*px == fg)
        {
            const mark_t t = *px_ptr(0, 0);
            const mark_t tr = *px_ptr(0, 1);
            if (t != bg)
            {
                *px = above_mark = t;
                if (tr != bg && tr != t)
                {
                    record_equa(tr, t);
                }
            }
            else if (tr != bg)
            {
                *px = above_mark = tr;
            }
            else
            {
                *px = above_mark = new_mark();
            }
        }
        else
        {
            above_mark = invalid_mark;
        }

        for (isize_t row = 2; row < shadow.rows; ++row)
        {
            pix_t* px = px_ptr(row, 0);
            CS_PREFETCH(px, 1, 1);
            if (*px == fg)
            {
                *px = above_mark = (above_mark == invalid_mark ? new_mark() : above_mark);
            }
            else
            {
                above_mark = invalid_mark;
            }
        }
    }

    // scan and mark the first row.
    void smark_top()
    {
        pix_t* px = shadow.ptr<pix_t>(0);
        CS_PREFETCH(px, 1, 1);

        mark_t left_mark;
        if (*px == fg)
        {
            *px = left_mark = new_mark();
        }
        else
        {
            left_mark = invalid_mark;
        }
        ++px;
        for (const pix_t* const end = px + shadow.cols; px != end; ++px)
        {
            CS_PREFETCH(px, 1, 1);
            if (*px == fg)
            {
                *px = left_mark = (left_mark == invalid_mark ? new_mark() : left_mark);
            }
            else
            {
                left_mark = invalid_mark;
            }
        }
    }

    CS_FORCE_INLINE mark_t calc_mark(isize_t row, isize_t col) const
    {
        return calc_mark(*px_ptr(row, col));
    }

    // NOTE: @pix_color_or_mark must be a marked pixel-value.
    CS_FORCE_INLINE mark_t calc_mark(pix_t marked_pixel) const
    {
        return marked_pixel == bg ? invalid_mark : marked_pixel;
    }

    CS_FORCE_INLINE void record_equa(mark_t a, mark_t b)
    {
        if (a != b)
        {
            CS_SAY("recording (" << a << "," << b << ")");
            if (a < b)
            {
                record_equa_(a, b);
            }
            else
            {
                record_equa_(b, a);
            }
        }
    }

    MarkList calc_map()
    {
        CS_DUMP(emap.cols);
        CS_DUMP(emap.rows);
        CS_DUMP(last_mark);
        mark_map.resize(last_mark + 1, invalid_mark);
        mark_t m;
        for (mark_t mark = min_valid_mark; mark <= last_mark; ++mark)
        {
            m = calc_min_equa(mark);
            if (m != mark)
            {
                mark_map[mark] = m;
            }
        }
        return mark_map;
    }

    static const mark_t min_valid_mark = invalid_mark + 1;

    mark_t calc_min_equa(mark_t mark) const
    {
        mark_t min = mark;
        calc_min_equa_in_col(mark, invalid_mark, min);
        return min;
    }

    void calc_min_equa_in_col(mark_t mark, mark_t skip, mark_t& min) const
    {
        bool is_min = true;
        for (mark_t row = min_valid_mark; row <= last_mark; ++row)
        {
            if (emap.ptr<uint8_t>(row)[mark])
            {
                is_min = false;
                if (row < min)
                {
                    min = row;
                }
                if (row != skip)
                {
                    calc_min_equa_in_row(row, mark,  min);
                }
            }
        }
        if (is_min)
        {
            if (mark < min)
            {
                min = mark;
            }
        }
    }

    void calc_min_equa_in_row(mark_t mark, mark_t skip, mark_t& min) const
    {
        bool is_min = true;
        for (mark_t col = min_valid_mark; col <= last_mark; ++col)
        {
            if (emap.ptr<uint8_t>(mark)[col])
            {
                is_min = false;
                if (col < min)
                {
                    min = col;
                }
                if (col != skip)
                {
                    calc_min_equa_in_col(col, mark, min);
                }
            }
        }
        if (is_min)
        {
            if (mark < min)
            {
                min = mark;
            }
        }
    }

    mark_t get_min_equa_(mark_t mark) const
    {
        mark_t parent = mark, prev = parent;
        while (parent != invalid_mark)
        {
            prev = parent;
            parent = parent_in_col(parent);
            if (parent != invalid_mark)
            {
                prev = parent;
                parent = parent_in_row(parent);
            }
        }
        return prev;
    }

    MarkList get_min_equas() const
    {
        MarkList mark_list;
        for (mark_t mark = invalid_mark + 1; mark <= last_mark; ++mark)
        {
            if (is_min_equa(mark))
            {
                mark_list.push_back(mark);
            }
        }
        return mark_list;
    }

    bool is_min_equa(mark_t mark) const
    {
        return parent_in_col(mark) == invalid_mark;
    }

    mark_t parent_in_row(mark_t from) const
    {
        const uint8_t* signs = emap.ptr<uint8_t>(from);
        for (int32_t col = 0; col < from; ++col)
        {
            if (signs[col])
            {
                return col;
            }
        }
        return invalid_mark;
    }

    mark_t parent_in_col(mark_t from) const
    {
        for (int32_t row = 0; row < from; ++row)
        {
            if (emap.ptr<uint8_t>(row)[from])
            {
                return row;
            }
        }
        return invalid_mark;
    }

    void record_equa_(mark_t to, mark_t from)
    {
        if (!(emap.cols > from))
        {
            if (emap.empty())
            {
                emap.create(from << 1, from << 1, CV_8UC1);
                emap.setTo(static_cast<uint8_t>(false));
            }
            else
            {
                cv::Mat tmp;
                emap.copyTo(tmp);
                emap.create(from << 1, from << 1, CV_8UC1);
                emap.setTo(static_cast<uint8_t>(false));
                cv::Mat dest = emap(cv::Rect(0, 0, tmp.cols, tmp.rows));
                tmp.copyTo(dest);
            }
        }
        emap.ptr<uint8_t>(to)[from] = emap.ptr<uint8_t>(from)[to] = static_cast<uint8_t>(true);
    }

    Bound bounding(const PointList& points) const
    {
        isize_t left = shadow.cols, top = shadow.rows, right = 0, bottom = 0;
        for (PointList::const_iterator it = points.begin(); it != points.end(); ++it)
        {
            if (it->x < left)
            {
                left = it->x;
            }
            if (it->y < top)
            {
                top = it->y;
            }
            if (right < it->x)
            {
                right = it->x;
            }
            if (bottom < it->y)
            {
                bottom = it->y;
            }
        }
        return Bound(left, top, right - left + 1, bottom - top + 1);
    }

    CS_FORCE_INLINE mark_t new_mark()
    {
        ++last_mark;
        if (CS_BUNLIKELY(last_mark == bg))
        {
            ++last_mark;
        }
        return last_mark;
    }

    pix_t* px_ptr(isize_t row)
    {
        return shadow.ptr<pix_t>(row);
    }

    const pix_t* px_ptr(isize_t row) const
    {
        return shadow.ptr<pix_t>(row);
    }

    pix_t* px_ptr(isize_t row, isize_t col)
    {
        return shadow.ptr<pix_t>(row) + col;
    }

    const pix_t* px_ptr(isize_t row, isize_t col) const
    {
        return shadow.ptr<pix_t>(row) + col;
    }

#if CS_DEBUG
    void dump()
    {
        std::printf("\n");
        for (isize_t row = 0; row < shadow.rows; ++row)
        {
            pix_t* px = shadow.ptr<pix_t>(row);
            for (isize_t col = 0; col < shadow.cols; ++col)
            {
                if (px[col] == bg)
                {
                    std::printf("%2s", px[col] == bg ? "_" : (px[col] == fg ? "." : "*"));
                }
                else
                {
                    std::printf("%2d", px[col]);
                }
            }
            std::printf("\n");
        }
        std::printf("\n");
    }
#endif
};

}
