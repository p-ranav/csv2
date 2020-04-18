#include "doctest.hpp"
#include <csv2/reader.hpp>
using namespace csv2;
using doctest::test_suite;

using expected = std::unordered_map<std::string, std::string>;

void ROWS_ARE_SAME(row r1, expected r2) {
  REQUIRE(r1.size() == r2.size());
  for(auto& [k, v]: r1) {
    REQUIRE(r2[k.data()] == v);
  }
}

TEST_CASE("Parse an empty CSV" * test_suite("reader")) {
  reader csv {
    option::Filename{"inputs/empty.csv"}
  };
  auto rows = csv.rows();
  auto cols = csv.cols();
  REQUIRE(rows == 0);
  REQUIRE(cols == 0);
}

TEST_CASE("Parse file that doesn't exist" * test_suite("reader")) {
  bool exception_thrown = false;
  try {
    reader csv {
      option::Filename{"inputs/missing.csv"}
    };
  }
  catch (std::exception&) {
    exception_thrown = true;
  }
  REQUIRE(exception_thrown);
}

TEST_CASE("Parse the most basic of CSV buffers" * test_suite("reader")) {
  reader csv{
    option::Filename{"inputs/test_01.csv"}
  };

  std::vector<expected> values{
    expected{{"a", "1"}, {"b", "2"}, {"c", "3"}},
    expected{{"a", "4"}, {"b", "5"}, {"c", "6"}}
  };

  size_t i = 0;
  row row;
  while(csv.read_row(row)) {
    ROWS_ARE_SAME(row, values[i]);
    i += 1;
  }
  REQUIRE(csv.rows() == values.size());
  REQUIRE(csv.cols() == values[0].size());
}

TEST_CASE("Parse the most basic of CSV buffers with ', ' delimiter using skip_initial_space" * test_suite("reader")) {
  reader csv{
    option::Filename{"inputs/test_02.csv"},
    option::SkipInitialSpace{true}
  };

  std::vector<expected> values{
    expected{{"a", "1"}, {"b", "2"}, {"c", "3"}},
    expected{{"a", "4"}, {"b", "5"}, {"c", "6"}}
  };

  size_t i = 0;
  row row;
  while(csv.read_row(row)) {
    ROWS_ARE_SAME(row, values[i]);
    i += 1;
  }
  REQUIRE(csv.rows() == values.size());
  REQUIRE(csv.cols() == values[0].size());
}