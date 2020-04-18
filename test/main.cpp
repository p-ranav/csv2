#include "doctest.hpp"
#include <csv2/reader.hpp>
using namespace csv2;
using doctest::test_suite;

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

  size_t i = 0;
  row row;
  while(csv.read_row(row)) {
    if (i == 0) {
      REQUIRE(row["a"] == "1");
      REQUIRE(row["b"] == "2");
      REQUIRE(row["c"] == "3");
    } else if (i == 1) {
      REQUIRE(row["a"] == "4");
      REQUIRE(row["b"] == "5");
      REQUIRE(row["c"] == "6");  
    }
    i += 1;
  }
  REQUIRE(csv.rows() == 2);
  REQUIRE(csv.cols() == 3);
}

TEST_CASE("Parse the most basic of CSV buffers with ', ' delimiter using skip_initial_space" * test_suite("reader")) {
  reader csv{
    option::Filename{"inputs/test_02.csv"},
    option::SkipInitialSpace{true}
  };

  size_t i = 0;
  row row;
  while(csv.read_row(row)) {
    if (i == 0) {
      REQUIRE(row["a"] == "1");
      REQUIRE(row["b"] == "2");
      REQUIRE(row["c"] == "3");
    } else if (i == 1) {
      REQUIRE(row["a"] == "4");
      REQUIRE(row["b"] == "5");
      REQUIRE(row["c"] == "6");  
    }
    i += 1;
  }
  REQUIRE(csv.rows() == 2);
  REQUIRE(csv.cols() == 3);
}