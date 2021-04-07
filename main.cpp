#include "consistent_tree.hpp"
#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include <string>

using std::string;

TEST_CASE("basic avl tests") {


}

TEST_CASE("Consistency", "First")
{
    auto tree = avl_array<int, int>();
    tree.insert(1, 2);
    tree.insert(3, 4);
    tree.insert(5, 6);

    auto it = tree.begin();

    ++it;

    REQUIRE(it.key() == 3);
    REQUIRE(it.val() == 4);

    tree.erase(3);

    ++it;

    REQUIRE(it.key() == 5);
    REQUIRE(it.val() == 6);
}

TEST_CASE("Consistency_2", "Second")
{
    int n = 5000;
    auto tree = avl_array<int, int>();
    for (int i = 0; i < n; ++i)
    {
        auto key = std::rand() % 5000;
        auto value = std::rand();
        tree.insert(key, value);
    }

    auto its = std::vector<avl_array<int, int>::iterator>();
    for (int i = 0; i < n; ++i)
    {
        auto it = tree.begin();
        auto steps = std::rand() % 2000;
        for (int j = 1; j < steps; ++j)
        {
            ++it;
            if (it == tree.end()) break;
        }
        its.push_back(it);
    }

    for (int i = 0; i < n; ++i)
    {
        auto key = std::rand() % 5000;
        tree.erase(key);
    }

    for (auto it : its)
    {
        while (it != tree.end())
        {
            ++it;
        }
    }
}