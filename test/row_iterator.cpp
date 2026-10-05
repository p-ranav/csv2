#include "doctest.hpp"
#ifdef CSV2_SINGLE_HEADER
#include <csv2/csv2.hpp>
#else
#include <csv2/reader.hpp>
#endif
#include <string>
#include <vector>

namespace {
using DataReader = csv2::Reader<csv2::delimiter<','>, csv2::quote_character<'"'>,
                                csv2::first_row_is_header<false>>;
using HeaderReader = csv2::Reader<csv2::delimiter<','>, csv2::quote_character<'"'>,
                                  csv2::first_row_is_header<true>>;
}

TEST_CASE("Copied row positions compare equal before and after dereferencing") {
  const std::string input = "one,two\nthree,four\n";
  DataReader reader;
  REQUIRE(reader.parse(input));
  auto first = reader.begin();
  const auto copy = first;
  CHECK(first == copy);
  CHECK(copy == first);
  CHECK_FALSE(first != copy);
  CHECK_FALSE(copy != first);

  (void)*first; // Populating cached row bounds must not change its position.
  CHECK(first == copy);
  CHECK_FALSE(first != copy);
  ++first;
  CHECK(first != copy);
  CHECK(copy != first);
  CHECK_FALSE(first == copy);
  CHECK_FALSE(copy == first);
}

TEST_CASE("Row end comparisons are symmetric for each line-ending convention") {
  const std::vector<std::string> inputs = {
      "one,two\nthree,four", "one,two\nthree,four\n",
      "one,two\r\nthree,four\r\n", "one,two\r\nthree,four",
      "\"one\ncontinued\",two\nthree,four\n"};
  for (const auto &input : inputs) {
    CAPTURE(input);
    DataReader reader;
    REQUIRE(reader.parse(input));
    auto it = reader.begin();
    const auto end = reader.end();
    CHECK(end == reader.end());
    CHECK_FALSE(end != reader.end());
    std::size_t rows = 0;
    while (it != end && rows < 3) {
      CHECK(end != it);
      CHECK_FALSE(it == end);
      CHECK_FALSE(end == it);
      (void)*it;
      ++it;
      ++rows;
    }
    REQUIRE(rows == 2);
    CHECK(it == end);
    CHECK(end == it);
    CHECK_FALSE(it != end);
    CHECK_FALSE(end != it);
  }
}

TEST_CASE("Empty readers have equal begin and end iterators") {
  DataReader reader;
  CHECK(reader.begin() == reader.end());
  CHECK_FALSE(reader.begin() != reader.end());
  const std::string empty;
  REQUIRE_FALSE(reader.parse(empty));
  CHECK(reader.begin() == reader.end());
  CHECK(reader.end() == reader.begin());
  CHECK_FALSE(reader.end() != reader.begin());
}

TEST_CASE("Header skipping preserves row-position comparisons") {
  const std::vector<std::string> inputs = {
      "name,value\n", "name,value\r\n", "name,value\nfirst,1",
      "name,value\r\nfirst,1\r\n"};
  for (std::size_t i = 0; i < inputs.size(); ++i) {
    CAPTURE(inputs[i]);
    HeaderReader reader;
    REQUIRE(reader.parse(inputs[i]));
    auto it = reader.begin();
    const auto end = reader.end();
    if (i < 2) {
      CHECK(it == end);
      CHECK(end == it);
    } else {
      CHECK(it != end);
      CHECK(end != it);
      (void)*it;
      ++it;
      CHECK(it == end);
      CHECK(end == it);
    }
  }
}

TEST_CASE("Matching offsets in distinct CSV buffers do not compare equal") {
  const std::string first_input = "a,b\n";
  const std::string second_input = "a,b\n";
  DataReader first_reader;
  DataReader second_reader;
  REQUIRE(first_reader.parse(first_input));
  REQUIRE(second_reader.parse(second_input));
  CHECK(first_reader.begin() != second_reader.begin());
  CHECK_FALSE(first_reader.begin() == second_reader.begin());
  CHECK(first_reader.end() != second_reader.end());
  CHECK_FALSE(first_reader.end() == second_reader.end());
}

#if __cplusplus >= 201703L
TEST_CASE("Different extents of the same buffer identify distinct row ranges") {
  const std::string input = "a,b\nc,d\n";
  DataReader short_reader;
  DataReader full_reader;
  REQUIRE(short_reader.parse_view(std::string_view(input.data(), 4)));
  REQUIRE(full_reader.parse_view(std::string_view(input)));
  CHECK(short_reader.begin() != full_reader.begin());
  CHECK_FALSE(short_reader.begin() == full_reader.begin());
  CHECK(short_reader.end() != full_reader.end());
}
#endif
