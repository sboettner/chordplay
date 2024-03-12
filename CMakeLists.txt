cmake_minimum_required(VERSION 3.10)
cmake_policy(SET CMP0076 NEW)

project(Chordplay)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(PkgConfig)
pkg_check_modules(POPT popt)
pkg_check_modules(RTMIDI rtmidi)

add_executable(chordplay)
add_subdirectory(src)

target_include_directories(chordplay PUBLIC ${POPT_INCLUDE_DIRS} ${RTMIDI_INCLUDE_DIRS})
target_link_libraries(chordplay ${POPT_LIBRARIES} ${RTMIDI_LIBRARIES})
