
#pragma once

#include "predef.hpp"
#if CS_DEBUG
#   include <cstdio>
#endif
#include <vector>
#include <set>
#include <utility>
#include <tr1/unordered_map>
#include <boost/static_assert.hpp>
#include <boost/dynamic_bitset.hpp>
#include "cvdef.hpp"
#include "facade.hpp"

namespace wdt {

template<NumDetecterKind kind>
class Divider
{
protected:
    typedef typename NumDetecterTraits<kind>::OptsType OptsType;
    typedef int32_t mark_t;
    BOOST_STATIC_ASSERT(sizeof(mark_t) >= sizeof(pix_t));
    typedef std::set<mark_t> EquaList;
    typedef std::tr1::unordered_map<mark_t, EquaList> EquaMap;
    typedef std::vector<mark_t> MarkList;
    typedef boost::dynamic_bitset<uint64_t> Check;

    static const mark_t invalid_mark = 0;

    const cv::Mat& img;
    const OptsType& opts;
    ImageList& res;

    cv::Mat shadow;
    EquaMap equa_map;

    mark_t last_mark;
    MarkList marks;
    Check check;

    const Config::BinaryColor fg;
    const Config::BinaryColor bg;

    const isize_t col_idx_max, row_idx_max;

public:
    explicit Divider(const cv::Mat& img_, const OptsType& opts_, ImageList& res_)
        : img(img_), opts(opts_), res(res_), last_mark(0),
          fg(Config::fg(opts.inverse)), bg(Config::bg(opts.inverse)),
          col_idx_max(img.cols - 1), row_idx_max(img.rows - 1)
    {}

    void divide()
    {
        img.copyTo(shadow);
        CS_RETURN_IF(!(shadow.cols > 1 && shadow.rows > 1));
        mark();
        separate();
#if CS_DEBUG
        dump();
#endif
    }

protected:
    typedef std::pair<mark_t, Bound> MarkPos;

    static const class MarkPosCmper
    {
    public:
        bool operator()(const MarkPos& left, const MarkPos& right) const
        {
            return right.second.x < left.second.x;
        }
    } mark_pos_cmper;

    void separate()
    {
        typedef std::vector<MarkPos> MarkPosList;
        MarkPosList poses;
        poses.reserve(marks.size());
        CS_DUMP(marks.size());

        for (int32_t i = 0; i < marks.size(); ++i)
        {
            PointList contour;
            const mark_t cur_mark = marks[i];
            CS_DUMP(cur_mark);
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

            CS_DUMP(contour.size());
            if (contour.size())
            {
                Bound box = cv::boundingRect(contour);
                poses.push_back(std::make_pair(cur_mark, box));
            }
        }

        res.reserve(poses.size());
        std::sort(poses.begin(), poses.end(), mark_pos_cmper);

        for (MarkPosList::const_iterator it = poses.begin(); it != poses.end(); ++it)
        {
            const cv::Mat mask = shadow(it->second);
            res.push_back(cv::Mat(it->second.size(), CV_8UC1));
            cv::Mat& res_img = *res.rbegin();
            const mark_t cur_mark = it->first;
            for (isize_t row = 0; row < mask.rows; ++row)
            {
                const pix_t* px = mask.ptr<pix_t>(row);
                pix_t* res_px = res_img.ptr<pix_t>(row);
                for (const pix_t* const end = px + mask.cols; px != end; ++px)
                {
                    *res_px++ = (*px == cur_mark) ? fg : bg;
                }
            }
        }
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
        merge_equas();

        MarkList map;
        for (EquaMap::const_iterator it = equa_map.begin(); it != equa_map.end(); ++it)
        {
            for (EquaList::const_iterator eit = it->second.begin(); eit != it->second.end(); ++eit)
            {
                if (!(*eit < map.size()))
                {
                    map.resize(*eit + 1, invalid_mark);
                }
                map[*eit] = it->first;
            }
        }

        for (isize_t row = 0; row < shadow.rows; ++row)
        {
            pix_t* px = shadow.ptr<pix_t>(row);
            for (const pix_t* const end = px + shadow.cols; px != end; ++px)
            {
                CS_PREFETCH(px, 1, 1);
                if (*px != bg)
                {
                    if (map[*px] != invalid_mark)
                    {
                        *px = map[*px];
                    }
                }
            }
        }
    }

    void merge_equas()
    {
        for (EquaMap::iterator it = equa_map.begin(); it != equa_map.end(); ++it)
        {
            for (EquaMap::iterator iit = equa_map.begin(); iit != equa_map.end(); ++iit)
            {
                if (it != iit)
                {
                    for (EquaList::const_iterator eit = iit->second.begin(); eit != iit->second.end(); ++eit)
                    {
                        if (*eit == it->first)
                        {
                            CS_SAY("found " << it->first << " in equivalence-map of " << iit->first);
                            iit->second.insert(it->second.begin(), it->second.end());
                            check[it->first] = false;
                            equa_map.erase(it);
                            return merge_equas();
                        }
                    }
                }
            }
        }

        for (size_t i = 0; i < check.size(); ++i)
        {
            if (check[i])
            {
                marks.push_back(i);
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
            tr = col == d.col_idx_max ? invalid_mark : d.calc_mark(row - 1, col + 1);
        }

        void reset(isize_t row, isize_t col, mark_t left_mark)
        {
            l = d.calc_mark(row, col - 1);
            tl = d.calc_mark(row - 1, col - 1);
            t = d.calc_mark(row - 1, col);
            tr = col == d.col_idx_max ? invalid_mark : d.calc_mark(row - 1, col + 1);
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

    // scan and mark center area.
    void smark_center()
    {
        Around around(*this);
        for (isize_t row = 1; row < shadow.rows; ++row)
        {
            mark_t left_mark = calc_mark(row, 0);
            pix_t* px = px_ptr(row, 1);
            for (isize_t col = 1; col < shadow.cols; ++col, ++px)
            {
                CS_PREFETCH(px, 1, 1);
                if (*px == fg)
                {
                    around.reset(row, col);
//                    CS_SAY("at [" << col << "," << row << "]");
                    mark_center_px(px, around, left_mark);
                }
                else
                {
                    left_mark = invalid_mark;
                }
            }
        }
    }

    void mark_center_px(pix_t* px, const Around& a, mark_t& left_mark)
    {
        int32_t n_mark = a.mark_count();
        if (n_mark == 0)
        {
            *px = left_mark = new_mark();
        }
        else if (n_mark == 1)
        {
            *px = left_mark = a.which();
        }
        else
        {
            bool found = false;
            if (a.l != invalid_mark)
            {
                *px = left_mark = a.l;
                found = true;
            }
            if (a.tl != invalid_mark)
            {
                if (found)
                {
//                    CS_SAY("mark value (" << a.tl << "," << left_mark << ") are equialence.");
                    record_equa(left_mark, a.tl);
                }
                else
                {
                    *px = left_mark = a.tl;
                    found = true;
                }
            }
            if (a.t != invalid_mark)
            {
                if (found)
                {
//                    CS_SAY("mark value (" << a.t << "," << left_mark << ") are equialence.");
                    record_equa(left_mark, a.t);
                }
                else
                {
                    *px = left_mark = a.t;
                    found = true;
                }
            }
            if (a.tr != invalid_mark)
            {
                if (found)
                {
//                    CS_SAY("mark value (" << a.tr << "," << left_mark << ") are equialence.");
                    record_equa(left_mark, a.tr);
                }
                else
                {
                    *px = left_mark = a.tr;
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
//                    CS_SAY("mark value (" << tr << "," << t << ") at [" << 1 << "," << 0 << "] are equialence.");
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
            record_equa_(a, b);
        }
    }

    void record_equa_(mark_t a, mark_t b)
    {
        mark_t key, val;
        if (a < b)
        {
            key = a;
            val = b;
        }
        else
        {
            key = b;
            val = a;
        }
        check[val] = false;
        EquaMap::iterator it = equa_map.find(key);
        if (it == equa_map.end())
        {
            it = equa_map.insert(std::make_pair(key, EquaList())).first;
        }
        it->second.insert(val);
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
        if (!(last_mark < check.size()))
        {
            check.resize(last_mark + 1, false);
        }
        check[last_mark] = true;
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
                    std::printf("  ");
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