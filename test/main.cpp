#include "doctest.hpp"
#include <csv2/reader.hpp>
using namespace csv2;
using doctest::test_suite;

TEST_CASE("Parse an empty CSV" * test_suite("Reader")) {
  Reader<',', '"'> csv;
  bool exception_thrown{false};
  try {
    csv.mmap("input/empty.csv");
  } catch (std::exception& e) {
    exception_thrown = true;
  }
  REQUIRE(exception_thrown);
}

TEST_CASE("Parse file that doesn't exist" * test_suite("Reader")) {
  Reader<',', '"'> csv;
  bool exception_thrown{false};
  try {
    csv.mmap("input/missing.csv");
  } catch (std::exception& e) {
    exception_thrown = true;
  }
  REQUIRE(exception_thrown);
}

TEST_CASE("Parse the most basic of CSV buffers" * test_suite("Reader")) {
  Reader<',', '"'> csv;
  csv.mmap("inputs/test_01.csv");

  std::vector<std::string> expected_cells{"a", "b", "c", "1", "2", "3", "4", "5", "6"};

  size_t rows{0}, cells{0};
  for (auto row : csv) {
    rows += 1;
    for (auto cell : row) {
      REQUIRE(cell.value() == expected_cells[cells++]);
    }
  }
  size_t cols = cells / rows;
  REQUIRE(rows == 3);
  REQUIRE(cols == 3);
}

TEST_CASE("Parse the most basic of CSV headers" * test_suite("Reader")) {
  Reader<',', '"'> csv;
  csv.mmap("inputs/test_01.csv");

  std::vector<std::string> expected_cells{"a", "b", "c"};

  size_t cells{0};
  const auto header = csv.header();
  for (const auto cell : header) {
    REQUIRE(cell.value() == expected_cells[cells++]);
  }
  REQUIRE(cells == 3);
}

TEST_CASE("Parse the most basic of CSV buffers with ', ' delimiter using initial space" *
          test_suite("Reader")) {
  Reader<',', '"'> csv;
  csv.mmap("inputs/test_02.csv");

  std::vector<std::string> expected_cells{"a", " b", " c", "1", " 2", " 3", "4", " 5", " 6"};

  size_t rows{0}, cells{0};
  for (auto row : csv) {
    rows += 1;
    for (auto cell : row) {
      REQUIRE(cell.value() == expected_cells[cells++]);
    }
  }
  size_t cols = cells / rows;
  REQUIRE(rows == 3);
  REQUIRE(cols == 3);
}

TEST_CASE("Parse headers with double quotes" * test_suite("Reader")) {
  Reader<',', '"'> csv;
  csv.mmap("inputs/test_06.csv");

  std::vector<std::string> expected_cells{"\"Free trip to A,B\"", "\"5.89\"",
                                          "\"Special rate \"1.79\"\""};

  size_t rows{0}, cells{0};
  for (auto row : csv) {
    rows += 1;
    for (auto cell : row) {
      REQUIRE(cell.value() == expected_cells[cells++]);
    }
  }
  size_t cols = cells / rows;
  REQUIRE(rows == 1);
  REQUIRE(cols == 3);
}

TEST_CASE("Parse headers with pairs of single-quotes" * test_suite("Reader")) {
  Reader<',', '\''> csv;
  csv.mmap("inputs/test_07.csv");

  std::vector<std::string> expected_cells{"''Free trip to A,B''", "''5.89''",
                                          "'Special rate 1.79'"};

  size_t rows{0}, cells{0};
  for (auto row : csv) {
    rows += 1;
    for (auto cell : row) {
      REQUIRE(cell.value() == expected_cells[cells++]);
    }
  }
  size_t cols = cells / rows;
  REQUIRE(rows == 1);
  REQUIRE(cols == 3);
}

TEST_CASE("Parse row with double quotes" * test_suite("Reader")) {
  Reader<',', '"'> csv;
  csv.mmap("inputs/test_05.csv");

  std::vector<std::string> expected_cells{
      "a", "\"\"b\"\"", "\"c\"", "\"Free trip to A,B\"", "\"5.89\"", "\"Special rate \"1.79\"\""};

  size_t rows{0}, cells{0};
  for (auto row : csv) {
    rows += 1;
    for (auto cell : row) {
      REQUIRE(cell.value() == expected_cells[cells++]);
    }
  }
  size_t cols = cells / rows;
  REQUIRE(rows == 2);
  REQUIRE(cols == 3);
}

TEST_CASE("Parse row with single quotes" * test_suite("Reader")) {
  Reader<',', '\''> csv;
  csv.mmap("inputs/test_04.csv");

  std::vector<std::string> expected_cells{
      "a", "''b''", "'c'", "'Free trip to A,B'", "'5.89'", "'Special rate '1.79''"};

  size_t rows{0}, cells{0};
  for (auto row : csv) {
    rows += 1;
    for (auto cell : row) {
      REQUIRE(cell.value() == expected_cells[cells++]);
    }
  }
  size_t cols = cells / rows;
  REQUIRE(rows == 2);
  REQUIRE(cols == 3);
}

TEST_CASE("Parse line break inside double quotes" * test_suite("Reader")) {
  Reader<',', '"'> csv;
  csv.mmap("inputs/test_03.csv");

  std::vector<std::string> expected_cells{"\"a\"", "\"b\\nc\"", "\"d\"", "1", "2", "3"};

  size_t rows{0}, cells{0};
  for (auto row : csv) {
    rows += 1;
    for (auto cell : row) {
      REQUIRE(cell.value() == expected_cells[cells++]);
    }
  }
  size_t cols = cells / rows;
  REQUIRE(rows == 2);
  REQUIRE(cols == 3);
}

TEST_CASE("Parse the most basic of CSV buffers - Space delimiter" * test_suite("Reader")) {
  Reader<' ', '"'> csv;
  csv.mmap("inputs/test_09.csv");

  std::vector<std::string> expected_cells{"first_name", "last_name", "Eric",
                                          "Idle",       "John",      "Cleese"};

  size_t rows{0}, cells{0};
  for (auto row : csv) {
    rows += 1;
    for (auto cell : row) {
      REQUIRE(cell.value() == expected_cells[cells++]);
    }
  }
  size_t cols = cells / rows;
  REQUIRE(rows == 3);
  REQUIRE(cols == 2);
}

TEST_CASE("Parse CSV with empty lines" * test_suite("Reader")) {
  Reader<',', '"'> csv;
  csv.mmap("inputs/empty_lines.csv");

  std::vector<std::string> expected_cells{"a", "b", "c", "1", "2",  "3",  "4", "5",
                                          "6", "7", "8", "9", "10", "11", "12"};

  size_t rows{0}, cells{0};
  for (auto row : csv) {
    rows += 1;
    for (auto cell : row) {
      REQUIRE(cell.value() == expected_cells[cells++]);
    }
  }
  REQUIRE(rows == 9); // There are rows with empty cells
}

TEST_CASE("Parse CSV with missing columns" * test_suite("Reader")) {
  Reader<',', '"'> csv;
  csv.mmap("inputs/missing_columns.csv");

  std::vector<std::string> expected_cells{"a", "b", "c", "d", "1", "2", "", "4", "5", "6", "", "8"};

  size_t rows{0}, cells{0};
  for (auto row : csv) {
    rows += 1;
    for (auto cell : row) {
      REQUIRE(cell.value() == expected_cells[cells++]);
    }
  }
  size_t cols = cells / rows;
  REQUIRE(rows == 3);
  REQUIRE(cols == 4);
}
