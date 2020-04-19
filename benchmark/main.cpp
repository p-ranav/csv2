#include <csv2/reader.hpp>
using namespace csv2;

int main(int argc, char **argv) {

  Reader csv{
    option::Filename{std::string(argv[1])},
    option::Delimiter{','},
    option::SkipInitialSpace{true},
    option::TrimCharacters{std::vector<char>{'\n', '\r'}},
    option::TrimPolicy{Trim::trailing}
    // ...
  };
  
  Row next;
  while (csv.read_row(next)) {
    // Iterate over all rows in CSV
  }
}
