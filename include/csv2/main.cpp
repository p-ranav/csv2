#include <csv2/reader.hpp>
using namespace csv2;

int main(int argc, char **argv) {
  reader csv{
    option::Filename{std::string(argv[1])},
    option::Delimiter{' '},
    option::TrimCharacters{std::vector<char>{'\n', '\r'}},
    option::TrimPolicy{Trim::trailing},
    option::IgnoreColumns{std::vector<std::string>{}},
    option::SkipEmptyRows{true},
  };
  row next;
  while(csv.read_row(next)) {
    for (auto& [k, v]: next)
        std::cout << k << ":" << v << ";";
    std::cout << "\n";
  }
  std::cout << "Loaded " << csv.rows() << "x" << csv.cols() << " entries\n";
}
