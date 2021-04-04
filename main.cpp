#include "consistent_tree.hpp"
#include <iostream>
#include <string>
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

using std::string;

TEST_CASE("basic avl tests") {
    avl_array<int, int> tree;
    REQUIRE(tree.size() == 0);
    for (int i = 1; i <= 15; ++i) {
        tree.insert(i,i);
    }
    {
        auto it = tree.find(20);
        REQUIRE(it == tree.end());
        tree.insert(20, 20);
        it = tree.find(20);
        REQUIRE(it.val() == 20);
        tree.erase(it);
    }
}
TEST_CASE("consistent") {

}