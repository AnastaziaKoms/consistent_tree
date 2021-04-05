#include "consistent_tree.hpp"
#define CATCH_CONFIG_MAIN
#include "catch.hpp"


TEST_CASE("basic avl tests") {


}
TEST_CASE("consistent") {
    avl_array<int, int> tree;
    for (int i = 1; i <= 15; ++i) {
        tree.insert(i,i);
    }
    auto it = tree.find(20);
    REQUIRE(it == tree.end());
    tree.insert(20, 20);
    it = tree.find(9);
    REQUIRE(it.val() == 9);
    tree.erase(it);
    auto it1 = tree.find(10);
    tree.erase(it1);
    it++;
    it--;
    it++;
    int i = 0;
}