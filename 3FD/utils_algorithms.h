#ifndef UTILS_ALGORITHMS_H // header guard
#define UTILS_ALGORITHMS_H

#include <chrono>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <iterator>
#include <algorithm>

namespace _3fd
{
namespace utils
{
    template <typename IterType, typename KeyType, typename AnyCallableType>
    class KeyGetter
    {
    private:

        std::function<const KeyType &(typename std::iterator_traits<IterType>::reference) NOEXCEPT> m_wrappedCallable;

    public:

        KeyGetter(AnyCallableType callable)
            : m_wrappedCallable(std::forward<AnyCallableType>(callable)) {}

        KeyGetter(const KeyGetter &ob)
            : m_wrappedCallable(ob.m_wrappedCallable) {}

        constexpr KeyType &operator()(typename std::iterator_traits<IterType>::reference object) const NOEXCEPT
        {
            return m_wrappedCallable(object);
        }
    };

    template <typename IterType, typename AnyCallableType>
    class LessThanComparator
    {
    private:

        std::function<bool (typename std::iterator_traits<IterType>::reference,
            typename std::iterator_traits<IterType>::reference) NOEXCEPT> m_wrappedCallable;

    public:

        LessThanComparator(AnyCallableType callable)
            : m_wrappedCallable(std::forward<AnyCallableType>(callable)) {}

        LessThanComparator(const LessThanComparator &ob)
            : m_wrappedCallable(ob.m_wrappedCallable) {}

        constexpr bool operator()(typename std::iterator_traits<IterType>::reference left,
                                  typename std::iterator_traits<IterType>::reference right) const NOEXCEPT
        {
            return m_wrappedCallable(left, right);
        }
    };

    /// <summary>
    /// Binary search in sub-range of vector containing map cases entries.
    /// </summary>
    /// <param name="begin">An iterator to the first position of the sub-range.
    /// In the end, this parameter keeps the beginning of the last sub-range this
    /// iterative algorithm has delved into.</param>
    /// <param name="begin">An iterator to one past the last position of the sub-range.
    /// In the end, this parameter keeps the end of the last sub-range this iterative
    /// algorithm has delved into.</param>
    /// <param name="searchKey">The key to search for.</param>
    /// <returns>An iterator to the found entry. Naturally, it cannot refer to
    /// the same position in the parameter 'end', unless there was no match.</returns>
    template <typename IterType, typename SearchKeyType, typename GetKeyFnType, typename LessFnType>
    IterType BinarySearch(IterType &begin,
                          IterType &end,
                          SearchKeyType searchKey,
                          KeyGetter<IterType, SearchKeyType, GetKeyFnType> getKey,
                          LessThanComparator<IterType, LessFnType> lessThan) NOEXCEPT
    {
        auto endOfRange = end;
        auto length = std::distance(begin, end);

        if (length > 7)
        {
            while (begin != end)
            {
                auto middle = begin + std::distance(begin, end) / 2;

                if (lessThan(getKey(*middle), searchKey))
                    begin = middle + 1;
                else if (lessThan(searchKey, getKey(*middle)))
                    end = middle;
                else
                    return middle;
            }

            return begin = end = endOfRange;
        }

        // Linear search when container is small:

        while (begin != end)
        {
            if (lessThan(getKey(*begin), searchKey))
                ++begin;
            else if (lessThan(searchKey, getKey(*begin)))
                break;
            else
                return begin;
        }

        return begin = end = endOfRange;
    }

    template <typename IterType, typename SearchKeyType, typename GetKeyFnType, typename LessFnType>
    IterType BinarySearch(IterType &&begin,
                          IterType &&end,
                          SearchKeyType searchKey,
                          KeyGetter<IterType, SearchKeyType, GetKeyFnType> getKey,
                          LessThanComparator<IterType, LessFnType> lessThan) NOEXCEPT
    {
        return BinarySearch(begin,
                            end,
                            searchKey,
                            getKeyFunction,
                            lessThanFunction);
    }

    /// <summary>
    /// Gets the sub range of entries that match the given key (using binary search).
    /// </summary>
    /// <param name="subRangeBegin">An iterator to the first position of
    /// the range to search, and receives the same for the found sub-range.</param>
    /// <param name="subRangeEnd">An iterator to one past the last position of
    /// the range to search, and receives the same for the found sub-range.</param>
    /// <param name="searchKey">The key to search.</param>
    /// <returns>When a sub-range has been found, <c>true</c>, otherwise, <c>false</c>.</returns>
    template <typename IterType, typename SearchKeyType, typename GetKeyFnType, typename LessFnType>
    bool BinSearchSubRange(IterType &subRangeBegin,
                           IterType &subRangeEnd,
                           SearchKeyType searchKey,
                           KeyGetter<IterType, SearchKeyType, GetKeyFnType> getKey,
                           LessThanComparator<IterType, LessFnType> lessThan) NOEXCEPT
    {
        struct {
            IterType begin;
            IterType end;
        } firstMatchRightPart;

        auto endOfRange = subRangeEnd;
        auto end = subRangeEnd;
        auto begin = subRangeBegin;
        auto match = BinarySearch(begin, end, searchKey, getKey, lessThan);

        // match? this is the first, so keep the partition at the right:
        if (match != end)
        {
            firstMatchRightPart.begin = match; // match inclusive
            firstMatchRightPart.end = end;
        }
        else
        {
            subRangeBegin = subRangeEnd = endOfRange;
            return false; // no match found!
        }

        do
        {
            subRangeBegin = end = match; // last match is the smallest entry found!
            match = BinarySearch(begin, end, searchKey, getKey, lessThan); // continue to search in the left partition
        } while (match != end);

        /* Now go back to the partition at the right of the first
        match, and start looking for the end of the range: */

        match = firstMatchRightPart.begin;
        end = firstMatchRightPart.end;

        do
        {
            subRangeEnd = begin = match + 1; // last match is the greatest entry found!
            match = BinarySearch(begin, end, searchKey, getKey, lessThan); // continue to search in the right partition
        } while (match != end);

        return true;
    }

    /// <summary>
    /// Calculates the exponential back off given the attempt and time slot.
    /// </summary>
    /// <param name="attempt">The attempt (base 0).</param>
    /// <param name="timeSlot">The time slot.</param>
    /// <returns>A timespan of how long to wait before the next retry.</returns>
    template <typename RepType, typename PeriodType>
    std::chrono::duration<RepType, PeriodType> CalcExponentialBackOff(unsigned int attempt,
                                                                      std::chrono::duration<RepType, PeriodType> timeSlot)
    {
        static bool first(true);

        if (first)
        {
            srand(time(nullptr));
            first = false;
        }

        auto k = static_cast<unsigned int> (
            std::abs(1.0F * rand() / RAND_MAX) * ((1 << attempt) - 1)
        );

        return timeSlot * k;
    }

}// end of namespace utils
}// end of namespace _3fd

#endif // end of header guard
