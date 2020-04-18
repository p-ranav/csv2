# csv2
`csv2` is a fast and lazy CSV Parser written in `C++17`

```cpp
#include <csv2/reader.hpp>
using namespace csv2;

int main() {

  Reader csv{
    option::Filename{std::string("foo.csv")},
    option::Delimiter{','},
    option::TrimCharacters{std::vector<char>{'\n', '\r'}},
    option::TrimPolicy{Trim::trailing}
    // ...
  };
  
  // By this point, the file is fully loaded into memory and
  // split into lines (std::vector<std::string>)
  
  // Row is a std::unordered_map<std::string_view, std::string_view>
  // of key-value pairs. Use `read_row` to iterate over rows in the csv
  Row next;
  while (csv.read_row(next)) {
    // Do something with row
  }
}
```

## Highlights
* Single header file - Just include `csv2/reader.hpp`
* Blazing fast - Fast file reader coupled with generous use of `std::string_view` to minimize allocations/copies.
* Single-threaded - No additional worker threads.
* Lazy evaluated - Tokenization is not performed until `read_row()` is called.
* Configurable - Set your dialect with `csv2::option`.
* MIT License.
