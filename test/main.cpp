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

TEST_CASE("Parse headers with double quotes" * test_suite("reader")) {
  reader csv{
    option::Filename{"inputs/test_06.csv"}
  };
  std::vector<std::string_view> header = csv.header();
  REQUIRE(header.size() == 3);
  REQUIRE(header[0] == "\"Free trip to A,B\"");
  REQUIRE(header[1] == "\"5.89\"");
  REQUIRE(header[2] == "\"Special rate \"\"1.79\"\"\"");
}

// TEST_CASE("Parse headers with pairs of single-quotes" * test_suite("reader")) {
//   reader csv{
//     option::Filename{"inputs/test_07.csv"},
//     option::QuoteCharacter{'\''}
//   };
//   std::vector<std::string_view> header = csv.header();
//   REQUIRE(header.size() == 3);
//   REQUIRE(header[0] == "''Free trip to A,B''");
//   REQUIRE(header[1] == "''5.89''");
//   REQUIRE(header[2] == "''Special rate ''''1.79''''''");
// }

TEST_CASE("Parse the most basic of CSV buffers - No header row" * test_suite("reader")) {
  reader csv{
    option::Filename{"inputs/test_08.csv"},
    option::ColumnNames{std::vector<std::string>{"a", "b", "c"}}
  };

  std::vector<expected> values{
    expected{{"a", "1"}, {"b", "2"}, {"c", "3"}},
    expected{{"a", "4"}, {"b", "5"}, {"c", "6"}},
    expected{{"a", "7"}, {"b", "8"}, {"c", "9"}}
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

TEST_CASE("Parse the most basic of CSV buffers - Space delimiter" * test_suite("reader")) {
  reader csv{
    option::Filename{"inputs/test_09.csv"},
    option::Delimiter{' '}
  };

  std::vector<expected> values{
    expected{{"first_name", "Eric"}, {"last_name", "Idle"}},
    expected{{"first_name", "John"}, {"last_name", "Cleese"}}
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

TEST_CASE("Parse the most basic of CSV buffers and ignore 1 column" * test_suite("reader")) {
  reader csv{
    option::Filename{"inputs/test_01.csv"},
    option::IgnoreColumns{std::vector<std::string>{"a"}}
  };

  std::vector<expected> values{
    expected{{"b", "2"}, {"c", "3"}},
    expected{{"b", "5"}, {"c", "6"}}
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

TEST_CASE("Parse the most basic of CSV buffers and ignore 2 columns" * test_suite("reader")) {
  reader csv{
    option::Filename{"inputs/test_01.csv"},
    option::IgnoreColumns{std::vector<std::string>{"a", "b"}}
  };

  std::vector<expected> values{
    expected{{"c", "3"}},
    expected{{"c", "6"}}
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

TEST_CASE("Parse the most basic of CSV buffers and ignore all columns" * test_suite("reader")) {
  reader csv{
    option::Filename{"inputs/test_01.csv"},
    option::IgnoreColumns{std::vector<std::string>{"a", "b", "c"}}
  };

  std::vector<expected> values{
    expected{},
    expected{}
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

TEST_CASE("Parse the most basic of CSV buffers and ignore age/gender columns" * test_suite("reader")) {
  reader csv{
    option::Filename{"inputs/test_14.csv"},
    option::IgnoreColumns{std::vector<std::string>{"age", "gender"}},
    option::SkipInitialSpace{true}
  };

  std::vector<expected> values{
    expected{{"name", "Mark Johnson"}, {"email", "mark.johnson@gmail.com"}, {"department", "BA"}},
    expected{{"name", "John Stevenson"}, {"email", "john.stevenson@gmail.com"}, {"department", "IT"}},
    expected{{"name", "Jane Barkley"}, {"email", "jane.barkley@gmail.com"}, {"department", "MGT"}}
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

TEST_CASE("Parse CSV with empty lines" * test_suite("reader")) {
  reader csv{
    option::Filename{"inputs/empty_lines.csv"}
  };

  std::vector<expected> values{
    expected{{"a", "1"}, {"b", "2"}, {"c", "3"}},
    expected{{"a", "4"}, {"b", "5"}, {"c", "6"}},
    expected{{"a", "7"}, {"b", "8"}, {"c", "9"}},
    expected{{"a", ""}, {"b", ""}, {"c", ""}},
    expected{{"a", "10"}, {"b", "11"}, {"c", "12"}},
    expected{{"a", ""}, {"b", ""}, {"c", ""}},
    expected{{"a", ""}, {"b", ""}, {"c", ""}},
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

TEST_CASE("Parse CSV with empty lines - skip empty rows" * test_suite("reader")) {
  reader csv{
    option::Filename{"inputs/empty_lines.csv"},
    option::SkipEmptyRows{true}
  };

  std::vector<expected> values{
    expected{{"a", "1"}, {"b", "2"}, {"c", "3"}},
    expected{{"a", "4"}, {"b", "5"}, {"c", "6"}},
    expected{{"a", "7"}, {"b", "8"}, {"c", "9"}},
    expected{{"a", "10"}, {"b", "11"}, {"c", "12"}}
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

TEST_CASE("Parse CSV with missing columns" * test_suite("reader")) {
  reader csv{
    option::Filename{"inputs/missing_columns.csv"}
  };

  std::vector<expected> values{
    expected{{"a", "1"}, {"b", "2"}, {"c", ""}, {"d", "4"}},
    expected{{"a", "5"}, {"b", "6"}, {"c", ""}, {"d", "8"}}
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

TEST_CASE("Parse CSV with missing columns II" * test_suite("reader")) {
  reader csv{
    option::Filename{"inputs/missing_columns_2.csv"},
    option::Delimiter{';'}
  };

  std::vector<expected> values{
    expected{{"a", "1"}, {"b", "2"}, {"c", "3"}},
    expected{{"a", "1"}, {"b", "2"}, {"c", ""}},
    expected{{"a", "1"}, {"b", "2"}, {"c", ""}},
    expected{{"a", "1"}, {"b", ""}, {"c", ""}},
    expected{{"a", "1"}, {"b", ""}, {"c", ""}},
    expected{{"a", "1"}, {"b", ""}, {"c", ""}}
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

TEST_CASE("Parse CSV with too many columns" * test_suite("reader")) {
  reader csv{
    option::Filename{"inputs/too_many_columns.csv"},
    option::SkipInitialSpace{true}
  };

  std::vector<expected> values{
    expected{{"a", "1"}, {"b", "2"}, {"c", "3"}},
    expected{{"a", "6"}, {"b", "7"}, {"c", ""}}
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

TEST_CASE("Parse single row" * test_suite("reader")) {
  reader csv{
    option::Filename{"inputs/single_row.csv"},
    option::SkipInitialSpace{true}
  };

  std::vector<expected> values{
    expected{{"a", "1"}, {"b", "2"}, {"c", "3"}}
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

TEST_CASE("Parse exceptions" * test_suite("reader")) {
  reader csv{
    option::Filename{"inputs/exceptions.csv"},
    option::SkipInitialSpace{true}
  };

  std::vector<expected> values;
  values.push_back(expected{});
  values[0]["Type"] = "0";
  values[0]["Code"] = "1";
  values[0]["Message"] = "My exception 1";
  values[0]["Component"] = "EnergoKodInstrumentyTest";
  values[0]["File"] = "/home/szyk/!-EnergoKod/!-Libs/EnergoKodInstrumenty/Tests/Src/ExceptionsTest.cpp";
  values[0]["Line"] = "54";
  values[0]["Function"] = "virtual void ExceptionsTest::run()";

  values.push_back(expected{});
  values[1]["Type"] = "0";
  values[1]["Code"] = "1";
  values[1]["Message"] = "My exception 2";
  values[1]["Component"] = "EnergoKodInstrumentyTest";
  values[1]["File"] = "/home/szyk/!-EnergoKod/!-Libs/EnergoKodInstrumenty/Tests/Src/ExceptionsTest.cpp";
  values[1]["Line"] = "60";
  values[1]["Function"] = "virtual void ExceptionsTest::run()";

  values.push_back(expected{});
  values[2]["Type"] = "0";
  values[2]["Code"] = "1";
  values[2]["Message"] = "My exception 3";
  values[2]["Component"] = "EnergoKodInstrumentyTest";
  values[2]["File"] = "/home/szyk/!-EnergoKod/!-Libs/EnergoKodInstrumenty/Tests/Src/ExceptionsTest.cpp";
  values[2]["Line"] = "66";
  values[2]["Function"] = "virtual void ExceptionsTest::run()";

  values.push_back(expected{});
  values[3]["Type"] = "1";
  values[3]["Code"] = "2";
  values[3]["Message"] = "My warning 1";
  values[3]["Component"] = "EnergoKodInstrumentyTest";
  values[3]["File"] = "/home/szyk/!-EnergoKod/!-Libs/EnergoKodInstrumenty/Tests/Src/ExceptionsTest.cpp";
  values[3]["Line"] = "70";
  values[3]["Function"] = "virtual void ExceptionsTest::run()";

  values.push_back(expected{});
  values[4]["Type"] = "1";
  values[4]["Code"] = "2";
  values[4]["Message"] = "My warning 2";
  values[4]["Component"] = "EnergoKodInstrumentyTest";
  values[4]["File"] = "/home/szyk/!-EnergoKod/!-Libs/EnergoKodInstrumenty/Tests/Src/ExceptionsTest.cpp";
  values[4]["Line"] = "71";
  values[4]["Function"] = "virtual void ExceptionsTest::run()";

  values.push_back(expected{});
  values[5]["Type"] = "1";
  values[5]["Code"] = "2";
  values[5]["Message"] = "My warning 3";
  values[5]["Component"] = "EnergoKodInstrumentyTest";
  values[5]["File"] = "/home/szyk/!-EnergoKod/!-Libs/EnergoKodInstrumenty/Tests/Src/ExceptionsTest.cpp";
  values[5]["Line"] = "72";
  values[5]["Function"] = "virtual void ExceptionsTest::run()";

  size_t i = 0;
  row row;
  while(csv.read_row(row)) {
    ROWS_ARE_SAME(row, values[i]);
    i += 1;
  }
  REQUIRE(csv.rows() == values.size());
  REQUIRE(csv.cols() == values[0].size());
}