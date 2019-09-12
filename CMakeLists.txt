cmake_minimum_required(VERSION 3.9)
project(mc833_concurrent_tcp)

set(CMAKE_CXX_STANDARD 11)

add_executable(mc833_concurrent_tcp
        server.c
        cliente.c)
