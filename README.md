# csv2
`csv2` is a fast and lazy CSV Parser written in `C++17`

```cpp
#include <csv2/reader.hpp>
using namespace csv2;

int main() {
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
  // No more rows
}
```

## Highlights
* Single header file - Just include `csv2/reader.hpp`
* Blazing fast - Fast file reader coupled with generous use of `std::string_view` to minimize allocations/copies.
* Single-threaded - No additional worker threads
* Configurable - Set your dialect with `csv2::option`
* MIT License
