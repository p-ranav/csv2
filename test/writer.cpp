#include "doctest.hpp"
#ifdef CSV2_SINGLE_HEADER
#include <csv2/csv2.hpp>
#else
#include <csv2/writer.hpp>
#endif
#include <array>
#include <sstream>
#include <string>
#include <vector>

namespace {
// The Writer owns closing its stream; keep that API while inspecting output.
struct BufferStream : std::ostringstream {
  void close() {}
};

template <char Delimiter = ','>
std::string write(const std::vector<std::string> &row) {
  BufferStream stream;
  csv2::Writer<csv2::delimiter<Delimiter>, BufferStream> writer(stream);
  writer.write_row(row);
  return stream.str();
}
} // namespace

TEST_CASE("Writer preserves ordinary fields" * doctest::test_suite("Writer")) {
  CHECK(write({"a", "b", "c"}) == "a,b,c\n");
  CHECK(write({"", "b", ""}) == ",b,\n");
  CHECK(write({" leading ", "trailing "}) == " leading ,trailing \n");
  CHECK(write<';'>({"a,b", "c"}) == "a,b;c\n");
}

TEST_CASE("Writer quotes the active delimiter" * doctest::test_suite("Writer")) {
  CHECK(write({"a,,,,,", "b", "c"}) == "\"a,,,,,\",b,c\n");
  CHECK(write({"a", "b,c", "d,e"}) == "a,\"b,c\",\"d,e\"\n");
  CHECK(write<';'>({"a;b", "c"}) == "\"a;b\";c\n");
  CHECK(write<'\t'>({"a\tb", "c"}) == "\"a\tb\"\tc\n");
}

TEST_CASE("Writer doubles quotes inside quoted fields" * doctest::test_suite("Writer")) {
  CHECK(write({"\"hello\"", "b"}) == "\"\"\"hello\"\"\",b\n");
  CHECK(write({"a\"b", "\""}) == "\"a\"\"b\",\"\"\"\"\n");
  CHECK(write({"\"\"", "b"}) == "\"\"\"\"\"\",b\n");
}

TEST_CASE("Writer quotes embedded line endings" * doctest::test_suite("Writer")) {
  CHECK(write({"first\nsecond", "b"}) == "\"first\nsecond\",b\n");
  CHECK(write({"first\rsecond", "b"}) == "\"first\rsecond\",b\n");
  CHECK(write({"first\r\nsecond", "b"}) == "\"first\r\nsecond\",b\n");
  CHECK(write({"a,\"b\"\r\nc", "d"}) == "\"a,\"\"b\"\"\r\nc\",d\n");
}

TEST_CASE("Writer applies escaping to every row" * doctest::test_suite("Writer")) {
  BufferStream stream;
  csv2::Writer<csv2::delimiter<','>, BufferStream> writer(stream);
  const std::vector<std::vector<std::string>> rows{{"a,b", "c"}, {"a\"b", "d"}};
  writer.write_rows(rows);
  CHECK(stream.str() == "\"a,b\",c\n\"a\"\"b\",d\n");
}

TEST_CASE("Writer accepts existing string containers" * doctest::test_suite("Writer")) {
  BufferStream stream;
  csv2::Writer<csv2::delimiter<','>, BufferStream> writer(stream);
  const std::array<std::string, 2> array{{"a,b", "c"}};
  writer.write_row(array);
  writer.write_row(std::vector<std::string>{"d", "e\"f"});
  const std::vector<const char *> strings{"g,h", "i"};
  writer.write_row(strings);
  CHECK(stream.str() == "\"a,b\",c\nd,\"e\"\"f\"\n\"g,h\",i\n");
}
