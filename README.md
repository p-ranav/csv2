# csv2
Fast and Lazy CSV Parser for Modern C++


```cpp
#include <csv2/reader.hpp>
using namespace csv2;

int main(int argc, char **argv) {
  reader csv{
    option::Filename{std::string("foo.csv")},
    option::Delimiter{','},
    option::TrimCharacters{std::vector<char>{'\n', '\r'}},
    option::TrimPolicy{Trim::trailing}
  };

  row next;
  while (csv.read_row(next)) {
    // Do something with row
  }
}
```
