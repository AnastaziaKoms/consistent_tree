#include "consistent_tree.hpp"
#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include <string>
#include <thread>
#include <vector>

using std::string;
using std::thread;
using std::vector;

TEST_CASE("basic avl tests") {
    avl_tree<char, string> tree;
    tree.insert('0', "abc");
    tree.insert('1', "zxc");
    tree.insert('2', "klj");

    avl_tree<char, string> copy(tree);

    REQUIRE(copy['0'] == "abc");
    REQUIRE(tree.size() == copy.size());

    REQUIRE(tree.find('2').val() == "klj");
    REQUIRE(tree.size() == 3);

    tree.clear();
    REQUIRE(tree.find('0') == tree.end());
    REQUIRE(tree.find('1') == tree.end());
    REQUIRE(tree.find('2') == tree.end());
    REQUIRE(tree.size() == 0);
    REQUIRE(tree.empty());


    REQUIRE(tree.begin() == tree.end());
    copy['0'] = "hgy";
    REQUIRE(copy['0'] == "hgy");
}

TEST_CASE("Consistency")
{
    auto tree = avl_tree<int, int>();
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

TEST_CASE("Consistency_correct_end")
{
    int n = 500;
    auto tree = avl_tree<int, int>();
    for (int i = 0; i < n; ++i)
    {
        auto key = std::rand() % 5000;
        auto value = std::rand();
        tree.insert(key, value);
    }

    auto its = vector<avl_tree<int, int>::iterator>();
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




TEST_CASE("Coarse-grained common") {
    int n = 9;

    avl_tree<char, char> tree;
    vector<thread> threads(n-1);
    string s = "123456789";
    size_t block_size = s.size() / n;
    string::iterator block_start = s.begin();
    auto f =  [&tree](std::string::iterator first, std::string::iterator last) {
        std::for_each(first, last, [&tree](char& c) { tree[c] = c; });
    };

    for (size_t i = 0; i < n - 1; i++) {
        string::iterator block_end = block_start;
        std::advance(block_end, block_size);
        threads[i] = thread(f, block_start, block_end);
        block_start = block_end;
    }

    f(block_start, s.end());
    for (auto& t : threads)
        t.join();
    REQUIRE(tree.begin().val() == '1');
}

TEST_CASE("Coarse-grained in(de)crement") {

}