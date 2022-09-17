cmake_minimum_required(VERSION 3.23)
project(lab2 C)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/mem_trace1.txt
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
set(CMAKE_C_STANDARD 11)

add_executable(lab2
        cache_sim.c)
