cmake_minimum_required(VERSION 3.16)
project(ITSOL_2 C)
enable_language(Fortran)


if(NOT JLL_BUILD)
  find_package(LAPACK)
endif()

file(GLOB SRCS src/*.c)

add_library(ITSOL_2 SHARED ${SRCS} src/tools.f)
set_property(TARGET ITSOL_2 PROPERTY C_STANDARD 99)

if(JLL_BUILD)
  if($ENV{target} MATCHES "w64-mingw32$")
    target_link_libraries(ITSOL_2  blastrampoline-5)
  else()
    target_link_libraries(ITSOL_2  blastrampoline)
  endif()
else()
  target_link_libraries(ITSOL_2  ${LAPACK_LIBRARIES})
endif()
target_include_directories(ITSOL_2 PUBLIC include)

install(TARGETS ITSOL_2)
