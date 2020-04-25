#include "doctest.hpp"
#include <csv2/reader.hpp>
#include <string>
#include <vector>
using namespace csv2;
using doctest::test_suite;

TEST_CASE("Parse an empty CSV" * test_suite("Reader")) {
  Reader<delimiter<','>, quote_character<'"'>, first_row_is_header<false>> csv;
  bool exception_thrown{false};
  try {
    csv.mmap("input/empty.csv");
  } catch (std::exception &e) {
    exception_thrown = true;
  }
  REQUIRE(exception_thrown);
}

TEST_CASE("Parse file that doesn't exist" * test_suite("Reader")) {
  Reader<delimiter<','>, quote_character<'"'>, first_row_is_header<false>> csv;
  bool exception_thrown{false};
  try {
    csv.mmap("input/missing.csv");
  } catch (std::exception &e) {
    exception_thrown = true;
  }
  REQUIRE(exception_thrown);
}

TEST_CASE("Parse the most basic of CSV buffers" * test_suite("Reader")) {
  Reader<delimiter<','>, quote_character<'"'>, first_row_is_header<false>> csv;
  csv.mmap("inputs/test_01.csv");

  const std::vector<std::string> expected_cells{"a", "b", "c", "1", "2", "3", "4", "5", "6"};

  size_t rows{0}, cells{0};
  for (auto row : csv) {
    rows += 1;
    for (auto cell : row) {
      std::string value;
      cell.read_value(value);
      REQUIRE(value == expected_cells[cells++]);
    }
  }
  size_t cols = cells / rows;
  REQUIRE(rows == 3);
  REQUIRE(cols == 3);
}

TEST_CASE("Parse the most basic of CSV buffers ignoring the first row" * test_suite("Reader")) {
  Reader<delimiter<','>, quote_character<'"'>, first_row_is_header<true>> csv;
  csv.mmap("inputs/test_01.csv");

  const std::vector<std::string> expected_header{"a", "b", "c"};
  const auto header = csv.header();
  size_t h = 0;
  for (const auto cell: header) {
    std::string value;
    cell.read_value(value);
    REQUIRE(value == expected_header[h++]);
  }
  REQUIRE(h == 3);

  const std::vector<std::string> expected_cells{"1", "2", "3", "4", "5", "6"};

  size_t rows{0}, cells{0};
  for (auto row : csv) {
    rows += 1;
    for (auto cell : row) {
      std::string value;
      cell.read_value(value);
      REQUIRE(value == expected_cells[cells++]);
    }
  }
  size_t cols = cells / rows;
  REQUIRE(rows == 2);
  REQUIRE(cols == 3);
}

TEST_CASE("Parse the most basic of CSV headers" * test_suite("Reader")) {
  Reader<delimiter<','>, quote_character<'"'>, first_row_is_header<false>> csv;
  csv.mmap("inputs/test_01.csv");

  const std::vector<std::string> expected_cells{"a", "b", "c"};

  size_t cells{0};
  const auto header = csv.header();
  for (const auto cell : header) {
    std::string value;
    cell.read_value(value);
    REQUIRE(value == expected_cells[cells++]);
  }
  REQUIRE(cells == 3);
}

TEST_CASE("Parse the most basic of CSV buffers with ', ' delimiter using initial space" *
          test_suite("Reader")) {
  Reader<delimiter<','>, quote_character<'"'>, first_row_is_header<false>, trim_policy::no_trimming> csv;
  csv.mmap("inputs/test_02.csv");

  const std::vector<std::string> expected_cells{"a", " b", " c", "1", " 2", " 3", "4", " 5", " 6"};

  size_t rows{0}, cells{0};
  for (auto row : csv) {
    rows += 1;
    for (auto cell : row) {
      std::string value;
      cell.read_value(value);
      REQUIRE(value == expected_cells[cells++]);
    }
  }
  size_t cols = cells / rows;
  REQUIRE(rows == 3);
  REQUIRE(cols == 3);
}

TEST_CASE("Parse the most basic of CSV buffers with ', ' delimiter using initial space - Trimming "
          "enabled" *
          test_suite("Reader")) {
  Reader<delimiter<','>, quote_character<'"'>, first_row_is_header<false>, trim_policy::trim_whitespace> csv;
  csv.mmap("inputs/test_02.csv");

  const std::vector<std::string> expected_cells{"a", "b", "c", "1", "2", "3", "4", "5", "6"};

  size_t rows{0}, cells{0};
  for (auto row : csv) {
    rows += 1;
    for (auto cell : row) {
      std::string value;
      cell.read_value(value);
      REQUIRE(value == expected_cells[cells++]);
    }
  }
  size_t cols = cells / rows;
  REQUIRE(rows == 3);
  REQUIRE(cols == 3);
}

TEST_CASE("Parse headers with double quotes" * test_suite("Reader")) {
  Reader<delimiter<','>, quote_character<'"'>, first_row_is_header<false>> csv;
  csv.mmap("inputs/test_06.csv");

  const std::vector<std::string> expected_cells{"\"Free trip to A,B\"", "\"5.89\"",
                                          "\"Special rate \"1.79\"\""};

  size_t rows{0}, cells{0};
  for (auto row : csv) {
    rows += 1;
    for (auto cell : row) {
      std::string value;
      cell.read_value(value);
      REQUIRE(value == expected_cells[cells++]);
    }
  }
  size_t cols = cells / rows;
  REQUIRE(rows == 1);
  REQUIRE(cols == 3);
}

TEST_CASE("Parse headers with pairs of single-quotes" * test_suite("Reader")) {
  Reader<delimiter<','>, quote_character<'\''>, first_row_is_header<false>> csv;
  csv.mmap("inputs/test_07.csv");

  const std::vector<std::string> expected_cells{"''Free trip to A,B''", "''5.89''",
                                          "'Special rate 1.79'"};

  size_t rows{0}, cells{0};
  for (auto row : csv) {
    rows += 1;
    for (auto cell : row) {
      std::string value;
      cell.read_value(value);
      REQUIRE(value == expected_cells[cells++]);
    }
  }
  size_t cols = cells / rows;
  REQUIRE(rows == 1);
  REQUIRE(cols == 3);
}

TEST_CASE("Parse row with double quotes" * test_suite("Reader")) {
  Reader<delimiter<','>, quote_character<'"'>, first_row_is_header<false>> csv;
  csv.mmap("inputs/test_05.csv");

  const std::vector<std::string> expected_cells{
      "a", "\"\"b\"\"", "\"c\"", "\"Free trip to A,B\"", "\"5.89\"", "\"Special rate \"1.79\"\""};

  size_t rows{0}, cells{0};
  for (auto row : csv) {
    rows += 1;
    for (auto cell : row) {
      std::string value;
      cell.read_value(value);
      REQUIRE(value == expected_cells[cells++]);
    }
  }
  size_t cols = cells / rows;
  REQUIRE(rows == 2);
  REQUIRE(cols == 3);
}

TEST_CASE("Parse row with single quotes" * test_suite("Reader")) {
  Reader<delimiter<','>, quote_character<'\''>, first_row_is_header<false>> csv;
  csv.mmap("inputs/test_04.csv");

  const std::vector<std::string> expected_cells{
      "a", "''b''", "'c'", "'Free trip to A,B'", "'5.89'", "'Special rate '1.79''"};

  size_t rows{0}, cells{0};
  for (auto row : csv) {
    rows += 1;
    for (auto cell : row) {
      std::string value;
      cell.read_value(value);
      REQUIRE(value == expected_cells[cells++]);
    }
  }
  size_t cols = cells / rows;
  REQUIRE(rows == 2);
  REQUIRE(cols == 3);
}

TEST_CASE("Parse line break inside double quotes" * test_suite("Reader")) {
  Reader<delimiter<','>, quote_character<'"'>, first_row_is_header<false>> csv;
  csv.mmap("inputs/test_03.csv");

  const std::vector<std::string> expected_cells{"\"a\"", "\"b\\nc\"", "\"d\"", "1", "2", "3"};

  size_t rows{0}, cells{0};
  for (auto row : csv) {
    rows += 1;
    for (auto cell : row) {
      std::string value;
      cell.read_value(value);
      REQUIRE(value == expected_cells[cells++]);
    }
  }
  size_t cols = cells / rows;
  REQUIRE(rows == 2);
  REQUIRE(cols == 3);
}

TEST_CASE("Parse the most basic of CSV buffers - Space delimiter" * test_suite("Reader")) {
  Reader<delimiter<' '>, quote_character<'"'>, first_row_is_header<false>> csv;
  csv.mmap("inputs/test_09.csv");

  const std::vector<std::string> expected_cells{"first_name", "last_name", "Eric",
                                          "Idle",       "John",      "Cleese"};

  size_t rows{0}, cells{0};
  for (auto row : csv) {
    rows += 1;
    for (auto cell : row) {
      std::string value;
      cell.read_value(value);
      REQUIRE(value == expected_cells[cells++]);
    }
  }
  size_t cols = cells / rows;
  REQUIRE(rows == 3);
  REQUIRE(cols == 2);
}

TEST_CASE("Parse CSV with empty lines" * test_suite("Reader")) {
  Reader<delimiter<','>, quote_character<'"'>, first_row_is_header<false>> csv;
  csv.mmap("inputs/empty_lines.csv");

  const std::vector<std::string> expected_cells{"a", "b", "c", "1", "2",  "3",  "4", "5",
                                          "6", "7", "8", "9", "10", "11", "12"};

  size_t rows{0}, cells{0};
  for (auto row : csv) {
    rows += 1;
    for (auto cell : row) {
      std::string value;
      cell.read_value(value);
      REQUIRE(value == expected_cells[cells++]);
    }
  }
  REQUIRE(rows == 9); // There are rows with empty cells
}

TEST_CASE("Parse CSV with missing columns" * test_suite("Reader")) {
  Reader<delimiter<','>, quote_character<'"'>, first_row_is_header<false>> csv;
  csv.mmap("inputs/missing_columns.csv");

  const std::vector<std::string> expected_cells{"a", "b", "c", "d", "1", "2", "", "4", "5", "6", "", "8"};

  size_t rows{0}, cells{0};
  for (auto row : csv) {
    rows += 1;
    for (auto cell : row) {
      std::string value;
      cell.read_value(value);
      REQUIRE(value == expected_cells[cells++]);
    }
  }
  size_t cols = cells / rows;
  REQUIRE(rows == 3);
  REQUIRE(cols == 4);
}

TEST_CASE("Parse the most basic of CSV buffers from string" * test_suite("Reader")) {
  Reader<delimiter<','>, quote_character<'"'>, first_row_is_header<false>> csv;
  const std::string buffer = "a,b,c\n1,2,3\n4,5,6";
  csv.parse(buffer);

  const std::vector<std::string> expected_cells{"a", "b", "c", "1", "2", "3", "4", "5", "6"};

  size_t rows{0}, cells{0};
  for (auto row : csv) {
    rows += 1;
    for (auto cell : row) {
      std::string value;
      cell.read_value(value);
      REQUIRE(value == expected_cells[cells++]);
    }
  }
  size_t cols = cells / rows;
  REQUIRE(rows == 3);
  REQUIRE(cols == 3);
}

TEST_CASE("Parse the most basic of CSV buffers with whitespace trimming enabled" *
          test_suite("Reader")) {
  Reader<delimiter<','>, quote_character<'"'>, first_row_is_header<false>> csv;
  csv.mmap("inputs/test_08.csv");

  const std::vector<std::string> expected_cells{"1", "2", "3", "4", "5", "6", "7", "8", "9"};

  size_t rows{0}, cells{0};
  for (auto row : csv) {
    rows += 1;
    for (auto cell : row) {
      std::string value;
      cell.read_value(value);
      REQUIRE(value == expected_cells[cells++]);
    }
  }
  size_t cols = cells / rows;
  REQUIRE(rows == 3);
  REQUIRE(cols == 3);
}

TEST_CASE("Parse the most basic of CSV buffers with double quotes with embedded delimiters" *
          test_suite("Reader")) {
  Reader<delimiter<','>, quote_character<'"'>, first_row_is_header<false>> csv;
  const std::string buffer = "a,\"b,c\",d\n\"1,2,3\",\"4,5,6\", \"7,8,9\"";
  csv.parse(buffer);

  const std::vector<std::string> expected_cells{"a", "\"b,c\"", "d", "\"1,2,3\"", "\"4,5,6\"", "\"7,8,9\""};

  size_t rows{0}, cells{0};
  for (auto row : csv) {
    rows += 1;
    for (auto cell : row) {
      std::string value;
      cell.read_value(value);
      REQUIRE(value == expected_cells[cells++]);
    }
  }
  size_t cols = cells / rows;
  REQUIRE(rows == 2);
  REQUIRE(cols == 3);
}

TEST_CASE("Parse the most basic of CSV buffers with double quotes with just delimiters" *
          test_suite("Reader")) {
  Reader<delimiter<','>, quote_character<'"'>, first_row_is_header<false>> csv;
  const std::string buffer = "hello,\",\",\" \",world,1,\"!\"";
  csv.parse(buffer);

  const std::vector<std::string> expected_cells{"hello", "\",\"", "\" \"", "world", "1", "\"!\""};

  size_t rows{0}, cells{0};
  for (auto row : csv) {
    rows += 1;
    for (auto cell : row) {
      std::string value;
      cell.read_value(value);
      REQUIRE(value == expected_cells[cells++]);
    }
  }
  size_t cols = cells / rows;
  REQUIRE(rows == 1);
  REQUIRE(cols == 6);
}