cmake_minimum_required(VERSION 3.17)
project(consistent_list)

set(CMAKE_CXX_STANDARD 17)

add_executable(consistent_list main.cpp consistent_tree.hpp smart_ptr.hpp)