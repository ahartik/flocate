cmake_minimum_required(VERSION 2.6.0) 

## Project information ##
project(csa CXX C)
ADD_DEFINITIONS(
   -O3
   # -g
  -std=c++0x
  -Wall
  -march=native
  )

set(CMAKE_BUILD_TYPE "Debug")

include_directories(
  )

link_directories(
)

add_executable(build build.cpp)
add_executable(kgram-db_test kgram-db_test.cpp kgram-db.cpp)
add_executable(locate locate.cpp kgram-db.cpp)
