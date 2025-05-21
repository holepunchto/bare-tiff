cmake_minimum_required(VERSION 3.31)

find_package(cmake-bare REQUIRED PATHS node_modules/cmake-bare)
find_package(cmake-fetch REQUIRED PATHS node_modules/cmake-fetch)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

project(bare_tiff C)

fetch_package("gitlab:libtiff/libtiff@4.7.0")

add_bare_module(bare_tiff)

target_sources(
  ${bare_tiff}
  PRIVATE
    binding.c
)

target_link_libraries(
  ${bare_tiff}
  PUBLIC
    tiff
)
