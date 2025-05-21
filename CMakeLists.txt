cmake_minimum_required(VERSION 3.31)

find_package(cmake-bare REQUIRED PATHS node_modules/cmake-bare)
find_package(cmake-fetch REQUIRED PATHS node_modules/cmake-fetch)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_MACOSX_BUNDLE OFF)

project(bare_tiff C)

fetch_package(
  "github:madler/zlib@1.3.1"
  SOURCE_DIR zlib_SOURCE_DIR
  BINARY_DIR zlib_BINARY_DIR
)

set(ZLIB_INCLUDE_DIR ${zlib_SOURCE_DIR} ${zlib_BINARY_DIR})
set(ZLIB_LIBRARY $<TARGET_FILE:zlibstatic>)

add_library(ZLIB::ZLIB ALIAS zlibstatic)

set(zlib ON CACHE BOOL "Enable zlib" FORCE)
set(libdeflate OFF CACHE BOOL "Enable libdeflate" FORCE)
set(jbig OFF CACHE BOOL "Enable jbig" FORCE)
set(jpeg OFF CACHE BOOL "Enable jpeg" FORCE)
set(old-jpeg OFF CACHE BOOL "Enable old-jpeg" FORCE)
set(jpeg12 OFF CACHE BOOL "Enable jpeg12" FORCE)
set(lerc OFF CACHE BOOL "Enable lerc" FORCE)
set(lzma OFF CACHE BOOL "Enable lzma" FORCE)
set(webp OFF CACHE BOOL "Enable webp" FORCE)
set(zstd OFF CACHE BOOL "Enable zstd" FORCE)

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
