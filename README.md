# csv2
`csv2` is a fast and lazy CSV Parser written in `C++17`

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

  row next; // unordered_map<string_view, string_view>
  while (csv.read_row(next)) {
    // Do something with row
  }
}
```

## Highlights
* 
