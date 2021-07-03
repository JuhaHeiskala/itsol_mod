cmake_minimum_required(VERSION 3.16)
project(ITSOL_2 C)
enable_language(Fortran)


if(JLL_BUILD)
  include($ENV{prefix}/lib/cmake/lapack-3.9.0/lapack-config.cmake)
else()
  find_package(LAPACK)
endif()

file(GLOB SRCS src/*.c)

add_library(ITSOL_2 SHARED ${SRCS} src/tools.f)
set_property(TARGET ITSOL_2 PROPERTY C_STANDARD 99)

target_link_libraries(ITSOL_2  ${LAPACK_LIBRARIES})
target_include_directories(ITSOL_2 PUBLIC include)

install(TARGETS ITSOL_2)
