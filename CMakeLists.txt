cmake_minimum_required(VERSION 3.10)
cmake_policy(SET CMP0076 NEW)

project(Chordplay VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include(GNUInstallDirs)

find_package(PkgConfig)
pkg_check_modules(POPT popt)
pkg_check_modules(RTMIDI rtmidi)

configure_file(
    ${PROJECT_SOURCE_DIR}/src/config.h.in
    ${PROJECT_BINARY_DIR}/config.h
)

add_executable(chordplay)
add_subdirectory(src)

target_include_directories(chordplay PUBLIC ${POPT_INCLUDE_DIRS} ${RTMIDI_INCLUDE_DIRS} ${PROJECT_BINARY_DIR})
target_link_libraries(chordplay ${POPT_LIBRARIES} ${RTMIDI_LIBRARIES})

install(TARGETS chordplay DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY ensembles DESTINATION ${CMAKE_INSTALL_DATADIR}/chordplay)

set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "stb")
include(CPack)
