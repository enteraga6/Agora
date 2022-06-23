cmake_minimum_required(VERSION 2.8.12)
project(udp_client)
set(CMAKE_CXX_FLAGS "-std=c++17 -Wall -g -march=native -m64")
add_executable(udp_client client.cc)
target_link_libraries(udp_client gflags)

