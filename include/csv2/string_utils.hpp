#include <string>
#include <algorithm>
#include <cctype>
#include <locale>

// trim from start (in place)
// static inline void ltrim(std::string &s) {
//     s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
//         return !std::isspace(ch);
//     }));
// }

// trim from end (in place)
static inline void rtrim(std::string &s, const std::vector<char>& t) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [&t](int ch) {
        return std::find(t.begin(), t.end(), ch) == t.end();
    }).base(), s.end());
}

// // trim from both ends (in place)
// static inline void trim(std::string &s) {
//     ltrim(s);
//     rtrim(s);
// }