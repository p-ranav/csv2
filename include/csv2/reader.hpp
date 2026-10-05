
#pragma once
#include <cstring>
#if __has_include("sys/mman.h") || __has_include(<sys/mman.h>) || __has_include("windows.h") || __has_include(<windows.h>)
#define __CSV2_HAS_MMAN_H__ 1
#include <csv2/mio.hpp>
#endif
#include <csv2/parameters.hpp>
#include <istream>
#include <string>
#if ((defined(_MSVC_LANG) && _MSVC_LANG >= 201703L) || __cplusplus >= 201703L)
	#include <string_view>
#endif

// Optional SIMD acceleration for scanning cell content. Falls back to a plain
// scalar loop on any platform/compiler where neither intrinsic set is
// available, so this is always correct, just not always vectorized.
#if defined(__SSE2__) || defined(_M_X64) || defined(_M_AMD64) || \
    (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
#define __CSV2_HAS_SSE2__ 1
#include <emmintrin.h>
#elif defined(__ARM_NEON) || defined(__ARM_NEON__)
#define __CSV2_HAS_NEON__ 1
#include <arm_neon.h>
#endif

#if defined(__GNUC__) || defined(__clang__)
#define __CSV2_FORCE_INLINE__ inline __attribute__((always_inline))
#elif defined(_MSC_VER)
#define __CSV2_FORCE_INLINE__ __forceinline
#else
#define __CSV2_FORCE_INLINE__ inline
#endif

namespace csv2 {
namespace detail {

inline int count_trailing_zeros(unsigned int x) {
#if defined(__GNUC__) || defined(__clang__)
  return __builtin_ctz(x);
#elif defined(_MSC_VER)
  unsigned long index;
  _BitScanForward(&index, x);
  return static_cast<int>(index);
#else
  int n = 0;
  while (!(x & 1u)) {
    x >>= 1;
    ++n;
  }
  return n;
#endif
}

inline int count_trailing_zeros(unsigned long long x) {
#if defined(__GNUC__) || defined(__clang__)
  return __builtin_ctzll(x);
#elif defined(_MSC_VER)
  unsigned long index;
  _BitScanForward64(&index, x);
  return static_cast<int>(index);
#else
  int n = 0;
  while (!(x & 1ull)) {
    x >>= 1;
    ++n;
  }
  return n;
#endif
}

inline int popcount(unsigned long long x) {
#if defined(__GNUC__) || defined(__clang__)
  return __builtin_popcountll(x);
#else
  int n = 0;
  while (x) {
    x &= (x - 1);
    ++n;
  }
  return n;
#endif
}

// Returns the index (relative to buffer) of the first occurrence of `a` or
// `b` within buffer[start, end), or `end` if neither appears. Scans 16 bytes
// at a time with SSE2/NEON where available, otherwise falls back to a plain
// byte-by-byte scan. Short spans (below one SIMD chunk, e.g. typical quoted
// fields) skip vector setup entirely since it wouldn't pay for itself.
__CSV2_FORCE_INLINE__ size_t find_first_of_two(const char *buffer, size_t start, size_t end,
                                                char a, char b) {
  size_t i = start;
#if defined(__CSV2_HAS_SSE2__) || defined(__CSV2_HAS_NEON__)
  if (end - start >= 16) {
#endif
#if defined(__CSV2_HAS_SSE2__)
  const __m128i va = _mm_set1_epi8(a);
  const __m128i vb = _mm_set1_epi8(b);
  for (; i + 16 <= end; i += 16) {
    const __m128i chunk = _mm_loadu_si128(reinterpret_cast<const __m128i *>(buffer + i));
    const __m128i hit = _mm_or_si128(_mm_cmpeq_epi8(chunk, va), _mm_cmpeq_epi8(chunk, vb));
    const int mask = _mm_movemask_epi8(hit);
    if (mask)
      return i + static_cast<size_t>(count_trailing_zeros(static_cast<unsigned int>(mask)));
  }
#elif defined(__CSV2_HAS_NEON__)
  const uint8x16_t va = vdupq_n_u8(static_cast<uint8_t>(a));
  const uint8x16_t vb = vdupq_n_u8(static_cast<uint8_t>(b));
  for (; i + 16 <= end; i += 16) {
    const uint8x16_t chunk = vld1q_u8(reinterpret_cast<const uint8_t *>(buffer + i));
    const uint8x16_t hit = vorrq_u8(vceqq_u8(chunk, va), vceqq_u8(chunk, vb));
    const uint64_t lo = vgetq_lane_u64(vreinterpretq_u64_u8(hit), 0);
    const uint64_t hi = vgetq_lane_u64(vreinterpretq_u64_u8(hit), 1);
    if (lo)
      return i + (static_cast<size_t>(count_trailing_zeros(static_cast<unsigned long long>(lo))) >> 3);
    if (hi)
      return i + 8 + (static_cast<size_t>(count_trailing_zeros(static_cast<unsigned long long>(hi))) >> 3);
  }
#endif
#if defined(__CSV2_HAS_SSE2__) || defined(__CSV2_HAS_NEON__)
  }
#endif
  for (; i < end; i++) {
    if (buffer[i] == a || buffer[i] == b)
      return i;
  }
  return end;
}

// Returns the number of occurrences of `needle` in buffer[start, end).
// Used to cheaply check quote parity over a whole row in one bulk pass,
// rather than repeatedly scanning between every individual quote.
__CSV2_FORCE_INLINE__ size_t count_char(const char *buffer, size_t start, size_t end, char needle) {
  size_t count = 0;
  size_t i = start;
#if defined(__CSV2_HAS_SSE2__) || defined(__CSV2_HAS_NEON__)
  if (end - start >= 16) {
#endif
#if defined(__CSV2_HAS_SSE2__)
  const __m128i vneedle = _mm_set1_epi8(needle);
  for (; i + 16 <= end; i += 16) {
    const __m128i chunk = _mm_loadu_si128(reinterpret_cast<const __m128i *>(buffer + i));
    const __m128i eq = _mm_cmpeq_epi8(chunk, vneedle);
    count += static_cast<size_t>(popcount(static_cast<unsigned int>(_mm_movemask_epi8(eq))));
  }
#elif defined(__CSV2_HAS_NEON__)
  const uint8x16_t vneedle = vdupq_n_u8(static_cast<uint8_t>(needle));
  for (; i + 16 <= end; i += 16) {
    const uint8x16_t chunk = vld1q_u8(reinterpret_cast<const uint8_t *>(buffer + i));
    const uint8x16_t eq = vceqq_u8(chunk, vneedle);
    const uint64_t lo = vgetq_lane_u64(vreinterpretq_u64_u8(eq), 0);
    const uint64_t hi = vgetq_lane_u64(vreinterpretq_u64_u8(eq), 1);
    // Each matching lane is a full 0xFF (8 set bits), so popcount/8 gives
    // the number of matching lanes in that half.
    count += (static_cast<size_t>(popcount(lo)) + static_cast<size_t>(popcount(hi))) / 8;
  }
#endif
#if defined(__CSV2_HAS_SSE2__) || defined(__CSV2_HAS_NEON__)
  }
#endif
  for (; i < end; i++) {
    if (buffer[i] == needle)
      ++count;
  }
  return count;
}

// Returns the index of the first '\n' in buffer[start, end) that is not
// inside an (unescaped) quoted field, or `end` if there is none. A row must
// not be split in the middle of a quoted field that spans multiple physical
// lines, so RowIterator uses this instead of a plain memchr for '\n'.
//
// Fast path: find the next '\n' with a plain (single-needle) memchr, then
// check in one bulk pass whether an even or odd number of quotes precede it
// -- an odd count means that candidate falls inside an open quote. Escaped
// "" pairs always contribute an even number of quotes, so this parity check
// is correct without tracking escape state. This avoids repeatedly invoking
// a two-needle scan at every quote boundary, which regresses heavily-quoted
// data (gaps between quotes are often shorter than one SIMD chunk); the
// careful fallback below only runs for a row that genuinely has a quoted
// multi-line field.
inline size_t find_unquoted_newline(const char *buffer, size_t start, size_t end, char quote) {
  size_t search_from = start;
  while (true) {
    const char *ptr =
        static_cast<const char *>(memchr(buffer + search_from, '\n', end - search_from));
    if (!ptr)
      return end;
    const size_t candidate = static_cast<size_t>(ptr - buffer);
    if (count_char(buffer, start, candidate, quote) % 2 == 0)
      return candidate;
    // Candidate falls inside an open quote: carefully scan forward to find
    // where that quoted field actually closes, then resume the cheap search
    // just past it.
    bool quote_opened = true;
    size_t i = candidate + 1;
    while (i < end) {
      if (buffer[i] != quote) {
        ++i;
        continue;
      }
      if (i + 1 < end && buffer[i + 1] == quote) {
        i += 2; // escaped pair, stays open
      } else {
        quote_opened = false;
        ++i;
        break;
      }
    }
    if (quote_opened) // ran off the end still inside an open quote
      return end;
    search_from = i;
  }
}

// If the byte immediately before `end` is a '\r', returns end - 1 so that a
// CRLF line ending (the terminator RFC4180 actually specifies) doesn't leave
// a stray carriage return attached to the row's last cell. `start` bounds
// the check so an empty row isn't underflowed.
inline size_t trim_trailing_cr(const char *buffer, size_t start, size_t end) {
  if (end > start && buffer[end - 1] == '\r')
    return end - 1;
  return end;
}

} // namespace detail
} // namespace csv2

namespace csv2 {

template <class delimiter = delimiter<','>, class quote_character = quote_character<'"'>,
          class first_row_is_header = first_row_is_header<true>,
          class trim_policy = trim_policy::trim_whitespace>
class Reader {
  #if __CSV2_HAS_MMAN_H__
  mio::mmap_source mmap_;          // mmap source
  #endif
  const char *buffer_{nullptr};    // pointer to memory-mapped data
  size_t buffer_size_{0};          // mapped length of buffer
  size_t header_start_{0};         // start index of header (cache)
  size_t header_end_{0};           // end index of header (cache)

public:
  #if __CSV2_HAS_MMAN_H__
  // Use this if you'd like to mmap the CSV file
  template <typename StringType> bool mmap(StringType &&filename) {
    // Use the non-throwing overload: the constructor-based API requires
    // exceptions, which may be disabled (e.g. built with -fno-exceptions).
    std::error_code error;
    mmap_.map(filename, error);
    if (error || !mmap_.is_open() || !mmap_.is_mapped())
      return false;
    buffer_ = mmap_.data();
    buffer_size_ = mmap_.mapped_length();
    return true;
  }
  #endif

  // Use this if you have the CSV contents
  // in an std::string already
  template <typename StringType> bool parse(StringType &&contents) {
    buffer_ = std::forward<StringType>(contents).c_str();
    buffer_size_ = contents.size();
    return buffer_size_ > 0;
  }


  // Use this if you already have the CSV contents
  // in a std::string_view 
#if ((defined(_MSVC_LANG) && _MSVC_LANG >= 201703L) || __cplusplus >= 201703L)
  bool parse_view(std::string_view sv) {
    buffer_ = sv.data();
    buffer_size_ = sv.size();
    return buffer_size_ > 0;
  }
#endif


  class RowIterator;
  class Row;
  class CellIterator;

  class Cell {
    const char *buffer_{nullptr}; // Pointer to memory-mapped buffer
    size_t start_{0};             // Start index of cell content
    size_t end_{0};               // End index of cell content
    bool escaped_{false};         // Does the cell have escaped content?
    friend class Row;
    friend class CellIterator;

  public:
  
	// returns a view on the cell's contents if C++17 available
	#if ((defined(_MSVC_LANG) && _MSVC_LANG >= 201703L) || __cplusplus >= 201703L)
      std::string_view read_view() const {
      const auto new_start_end = trim_policy::trim(buffer_, start_, end_);
      return std::string_view(buffer_ + new_start_end.first, new_start_end.second- new_start_end.first);
      }
	#endif
    // Returns the raw_value of the cell without handling escaped
    // content, e.g., cell containing """foo""" will be returned
    // as is
    template <typename Container> void read_raw_value(Container &result) const {
      if (start_ >= end_)
        return;
      result.insert(result.end(), buffer_ + start_, buffer_ + end_);
    }

    // If cell is escaped, convert and return correct cell contents,
    // e.g., """foo""" => ""foo""
    template <typename Container> void read_value(Container &result) const {
      if (start_ >= end_)
        return;
      const auto new_start_end = trim_policy::trim(buffer_, start_, end_);
      result.insert(result.end(), buffer_ + new_start_end.first, buffer_ + new_start_end.second);
      // An empty quoted field ("") has no content to escape and must resolve
      // to an empty value rather than a single leftover quote character.
      if (result.size() == 2 && result[0] == quote_character::value &&
          result[1] == quote_character::value) {
        result.clear();
        return;
      }
      for (size_t i = 1; i < result.size(); ++i) {
        if (result[i] == quote_character::value && result[i - 1] == quote_character::value) {
          result.erase(i - 1, 1);
        }
      }
    }
  };

  class Row {
    const char *buffer_{nullptr}; // Pointer to memory-mapped buffer
    size_t start_{0};             // Start index of row content
    size_t end_{0};               // End index of row content
    friend class RowIterator;
    friend class Reader;

  public:
    // address of row
    const char *address() const { return buffer_; }
	// returns the char length of the row
	size_t length() const { return end_ - start_; }

    // Returns the raw_value of the row
    template <typename Container> void read_raw_value(Container &result) const {
      if (start_ >= end_)
        return;
      result.insert(result.end(), buffer_ + start_, buffer_ + end_);
    }

    class CellIterator {
      friend class Row;
      const char *buffer_;
      size_t buffer_size_;
      size_t start_;
      size_t current_;
      size_t end_;
      size_t next_; // start of the next cell, or end_ + 1 once iteration is complete

    public:
      CellIterator(const char *buffer, size_t buffer_size, size_t start, size_t end)
          : buffer_(buffer), buffer_size_(buffer_size), start_(start), current_(start),
            end_(end), next_(start) {
        // An iterator with no content ahead of it (e.g. a blank row) has no cells.
        if (start_ >= end_)
          current_ = end_ + 1;
      }

      CellIterator &operator++() {
        current_ = next_;
        return *this;
      }

      CellIterator operator++(int) {
        CellIterator current = *this;
        ++(*this);
        return current;
      }

      Cell operator*() {
        bool escaped{false};
        class Cell cell;
        cell.buffer_ = buffer_;
        cell.start_ = current_;
        cell.end_ = end_;

        size_t i = current_;
        while (i < end_) {
          const char c = buffer_[i];
          if (c == delimiter::value) {
            // actual delimiter: end of cell
            cell.end_ = i;
            cell.escaped_ = escaped;
            next_ = i + 1;
            return cell;
          }
          if (c != quote_character::value) {
            // Plain content: jump ahead in bulk (vectorized where available)
            // to the next delimiter or quote instead of inspecting every byte
            // in between one at a time. Cheap to skip entirely when the very
            // next byte is already special (e.g. quote-first fields), so this
            // never costs anything on data that can't benefit from it.
            i = detail::find_first_of_two(buffer_, i + 1, end_, delimiter::value,
                                           quote_character::value);
            continue;
          }

          // Opening quote: quoted fields are typically short, so a plain
          // scalar scan (no function-call/vector-setup overhead) is used here
          // instead of another vectorized search.
          ++i;
          for (; i < end_; i++) {
            if (buffer_[i] != quote_character::value)
              continue;
            if (i + 1 < end_ && buffer_[i + 1] == quote_character::value) {
              // escaped quote ("") within the quoted field: skip the pair,
              // the field remains open
              escaped = true;
              i++;
            } else {
              // genuine closing quote
              ++i;
              break;
            }
          }
        }
        // Reached the end of the row's content with no trailing delimiter found:
        // this is the last cell and no further (empty) cell follows.
        cell.end_ = i;
        cell.escaped_ = escaped;
        next_ = end_ + 1;
        return cell;
      }

      bool operator!=(const CellIterator &rhs) { return current_ != rhs.current_; }

      // Proxy returned by operator-> so that icell->read_value(...) works.
      class CellProxy {
        Cell cell_;

      public:
        explicit CellProxy(Cell cell) : cell_(cell) {}
        Cell *operator->() { return &cell_; }
        const Cell *operator->() const { return &cell_; }
      };

      CellProxy operator->() { return CellProxy(**this); }
    };

    CellIterator begin() const { return CellIterator(buffer_, end_ - start_, start_, end_); }
    CellIterator end() const { return CellIterator(buffer_, end_ - start_, end_, end_); }
  };

  class RowIterator {
    friend class Reader;
    const char *buffer_;
    size_t buffer_size_;
    size_t start_;
    size_t end_;
    size_t next_; // start of the next row; tracked separately from end_
                  // since end_ may be trimmed of a trailing CRLF '\r'

  public:
    RowIterator(const char *buffer, size_t buffer_size, size_t start)
        : buffer_(buffer), buffer_size_(buffer_size), start_(start), end_(start_), next_(start_) {}

    RowIterator &operator++() {
      start_ = next_;
      end_ = start_;
      return *this;
    }

    Row operator*() {
      Row result;
      result.buffer_ = buffer_;
      result.start_ = start_;
      result.end_ = end_;

      const size_t newline_pos =
          detail::find_unquoted_newline(buffer_, start_, buffer_size_, quote_character::value);
      if (newline_pos < buffer_size_) {
        end_ = detail::trim_trailing_cr(buffer_, start_, newline_pos);
        result.end_ = end_;
        next_ = newline_pos + 1;
      } else {
        // last row
        end_ = detail::trim_trailing_cr(buffer_, start_, buffer_size_);
        result.end_ = end_;
        next_ = buffer_size_ + 1;
      }
      return result;
    }

    bool operator==(const RowIterator &rhs) const {
      // Advancing past a final newline reaches buffer_size_, while end()
      // and a final row without a newline use buffer_size_ + 1. Both are EOF.
      const size_t position = start_ < buffer_size_ ? start_ : buffer_size_;
      const size_t rhs_position =
          rhs.start_ < rhs.buffer_size_ ? rhs.start_ : rhs.buffer_size_;
      return buffer_ == rhs.buffer_ && buffer_size_ == rhs.buffer_size_ &&
             position == rhs_position;
    }

    bool operator!=(const RowIterator &rhs) const { return !(*this == rhs); }
  };

  RowIterator begin() const {
    if (buffer_size_ == 0)
      return end();
    if (first_row_is_header::value) {
      const auto header_indices = header_indices_();
      return RowIterator(buffer_, buffer_size_, header_indices.second  > 0 ? header_indices.second + 1 : 0);
    } else {
      return RowIterator(buffer_, buffer_size_, 0);
    }
  }

  RowIterator end() const { return RowIterator(buffer_, buffer_size_, buffer_size_ + 1); }

private:
  // Returns {0, position of the header's unquoted terminating newline} (or
  // {0, 0} if there is none). The second value is the raw newline position,
  // not CRLF-trimmed, since begin() uses it as-is to find where data rows
  // start; header()'s own Row span is trimmed independently.
  std::pair<size_t, size_t> header_indices_() const {
    size_t start = 0, end = 0;

    const size_t newline_pos = detail::find_unquoted_newline(buffer_, start, buffer_size_, quote_character::value);
    if (newline_pos < buffer_size_)
      end = newline_pos;
    return {start, end};
  }

public:

  Row header() const {
    size_t start = 0, end = 0;
    Row result;
    result.buffer_ = buffer_;
    result.start_ = start;
    result.end_ = end;

    const size_t newline_pos = detail::find_unquoted_newline(buffer_, start, buffer_size_, quote_character::value);
    if (newline_pos < buffer_size_) {
      end = detail::trim_trailing_cr(buffer_, start, newline_pos);
      result.end_ = end;
    }
    return result;
  }

  /**
   * @returns The number of rows (excluding the header)
  */
  size_t rows(bool ignore_empty_lines = false) const {
    size_t result{0};
    if (!buffer_ || buffer_size_ == 0)
      return result;
    
    // Count the first row if not header
    if (not first_row_is_header::value
        and (not ignore_empty_lines
        or *(static_cast<const char*>(buffer_)) != '\r'))
      ++result;

    for (size_t pos = 0; pos < buffer_size_;) {
      const size_t newline_pos = detail::find_unquoted_newline(buffer_, pos, buffer_size_, quote_character::value);
      if (newline_pos >= buffer_size_)
        break;
      if (not (ignore_empty_lines
          and (newline_pos >= buffer_size_ - 1
          or buffer_[newline_pos + 1] == '\r')))
        ++result;
      pos = newline_pos + 1;
    }
    return result;
  }

  size_t cols() const {
    size_t result{0};
    for (const auto cell : header())
      result += 1;
    return result;
  }
};
} // namespace csv2
