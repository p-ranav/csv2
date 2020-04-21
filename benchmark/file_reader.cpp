#include <iostream>
#include <fstream>
#include <string>
#include <cstdint>
#include <chrono>

template <typename LineHandler>
void read_file_fast_(std::ifstream &file, LineHandler &&line_handler) {
  int64_t buffer_size = 40000;
  file.seekg(0, std::ios::end);
  std::ifstream::pos_type p = file.tellg();
#ifdef WIN32
  int64_t file_size = *(int64_t *)(((char *)&p) + 8);
#else
  int64_t file_size = p;
#endif
  file.seekg(0, std::ios::beg);
  buffer_size = std::min(buffer_size, file_size);
  char *buffer = new char[buffer_size];
  auto buffer_length = buffer_size;
  file.read(buffer, buffer_length);

  int string_end = -1;
  int string_start{0};
  int64_t buffer_position_in_file = 0;
  while (buffer_length > 0) {
    int i = string_end + 1;
    string_start = string_end;
    string_end = -1;
    for (; i < buffer_length && i + buffer_position_in_file < file_size; i++) {
      if (buffer[i] == '\n') {
	string_end = i;
	break;
      }
    }

    if (string_end == -1) { // scroll buffer
      if (string_start == -1) {
	line_handler(buffer + string_start + 1, buffer_length);
	buffer_position_in_file += buffer_length;
	buffer_length = std::min(buffer_length, file_size - buffer_position_in_file);
	delete[] buffer;
	buffer = new char[buffer_length];
	file.read(buffer, buffer_length);
      } else {
	int moved_length = buffer_length - string_start - 1;
	memmove(buffer, buffer + string_start + 1, moved_length);
	buffer_position_in_file += string_start + 1;
	int read_size = std::min(buffer_length - moved_length,
				 file_size - buffer_position_in_file - moved_length);

	if (read_size != 0)
	  file.read(buffer + moved_length, read_size);
	if (moved_length + read_size < buffer_length) {
	  char *temp_buffer = new char[moved_length + read_size];
	  memmove(temp_buffer, buffer, moved_length + read_size);
	  delete[] buffer;
	  buffer = temp_buffer;
	  buffer_length = moved_length + read_size;
	}
	string_end = -1;
      }
    } else {
      line_handler(buffer + string_start + 1, string_end - string_start);
    }
  }
  delete[] buffer;
  line_handler(nullptr, 0); // eof
}

int main(int argc, char **argv) {
  using timepoint = std::chrono::time_point<std::chrono::high_resolution_clock>;

  auto print_exec_time = [](timepoint start, timepoint stop) {
    auto duration_us = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
    auto duration_s = std::chrono::duration_cast<std::chrono::seconds>(stop - start);

    std::cout << duration_us.count() << " us | " << duration_ms.count() << " ms | "
              << duration_s.count() << " s\n";
  };
    // Measurement 1: Loading file
  auto m1_start = std::chrono::high_resolution_clock::now();

  // Large I/O buffer to speed up ifstream read
  const uint64_t stream_buffer_size = 1000000;
  char stream_buffer[stream_buffer_size];
  
  std::ios_base::sync_with_stdio(false);
  std::ifstream infile;
  infile.rdbuf()->pubsetbuf(stream_buffer, stream_buffer_size);
  infile.open(argv[1]);
  size_t lines{0};
  read_file_fast_(infile, [&](char *buffer, int length) -> void {
			    lines += 1;
			  });
  auto m1_stop = std::chrono::high_resolution_clock::now();
  print_exec_time(m1_start, m1_stop);
  std::cout << "Rows: " << lines << std::endl;
}
