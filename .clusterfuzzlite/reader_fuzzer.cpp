#include <csv2/reader.hpp>
#include <string>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  std::string fuzz_payload(reinterpret_cast<const char *>(data), size);

  csv2::Reader<csv2::delimiter<','>, csv2::quote_character<'"'>, csv2::first_row_is_header<true>,
               csv2::trim_policy::trim_whitespace>
      csv;
  bool cont = false;

  try {
    csv.parse(fuzz_payload);
    cont = true;
  } catch (...) {
  }

  if (cont) {
    const auto header = csv.header();
    for (const auto row : csv) {
      for (const auto cell : row) {
        std::string value;
        cell.read_value(value);
      }
    }
  }

  return 0;
}