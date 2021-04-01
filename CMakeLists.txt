cmake_minimum_required(VERSION 3.17)
project(consistent_list)

set(CMAKE_CXX_STANDARD 17)

add_executable(consistent_list ../../CLionProjects/consistent_list/main.cpp ../../CLionProjects/consistent_list/consistent_tree.hpp ../../CLionProjects/consistent_list/smart_ptr.hpp)