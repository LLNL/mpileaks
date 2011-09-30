project(mpileaks)
cmake_minimum_required(VERSION 2.8)

find_package(callpath REQUIRED)
find_package(adept_utils REQUIRED)
find_package(MPI REQUIRED)

add_library(mpileaks mpileaks.cpp)
target_link_libraries(mpileaks callpath)

add_executable(example example.c)
target_link_libraries(example mpileaks ${MPI_LIBRARIES})

include_directories(
  ${MPI_INCLUDE_PATH}
  ${adept_utils_INCLUDE_PATH}
  ${callpath_INCLUDE_PATH})
