#include <string>
#include <fstream>
#include <iostream>
#include <csv2/task_system.hpp>

namespace csv2 {

class reader {
    task_system t{1};
public:
    reader(std::string filename) {
        t.start();
        std::ifstream file(std::move(filename));
        std::string line;
        unsigned line_no = 1;
        while (std::getline(file, line)) {
            t.async_(std::make_pair(line_no, line));
            line_no += 1;
        }
        t.stop();
        std::cout << ">>>>>>>>> Stopped\n";
    }
};

}