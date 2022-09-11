cmake_minimum_required(VERSION 3.20)
project(Cache_Rottenberg_L1_l2)

set(CMAKE_CXX_STANDARD 11)
add_library(Shloks_rob_sim_cache sim_cache.cpp sim_cache.h)
add_executable(Cache_Rottenberg_L1_l2  main.cpp )
target_link_libraries(Cache_Rottenberg_L1_l2 Shloks_rob_sim_cache)

