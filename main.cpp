#include "consistent_tree.hpp"
#include <iostream>
#include <string>

using std::string;

int main() {
    // create a 2048 node tree with <int> as key and value types and <std::uint16_t> as size type in 'Fast' mode
    avl_array<int, string> avl;
    avl.insert(4, "four");
    avl.insert(5, "five");
    avl.insert(6, "six");
    return 0;
}
