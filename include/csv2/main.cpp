#include <csv2/reader.hpp>
using namespace csv2;

int main(int argc, char **argv) {
  reader csv{
    option::Filename{std::string(argv[1])},
    option::Delimiter{','},
    option::TrimCharacters{std::vector<char>{'\n', '\r'}},
    option::IgnoreColumns{std::vector<std::string>{}},
    option::SkipEmptyRows{true}
  };
  row_t row;
  while(csv.read_row(row)) {
    for (auto& [k, v]: row)
        std::cout << k << ":" << v << ";";
    std::cout << "\n";
  }
  std::cout << "Loaded " << csv.rows() << "x" << csv.cols() << " entries\n";
}
