#include "consistent_tree.hpp"
#include <iostream>
#include <string>
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

using std::string;

TEST_CASE("basic avl tests") {
    avl_array<int, string> tree;
    REQUIRE(tree.size() == 0);
    tree.insert(4, "four");
    REQUIRE(tree.size() == 1);
}
TEST_CASE("consistent") {

}