# Don't use Cray compile wrappers
find_program(CMAKE_C_COMPILER gcc)
find_program(CMAKE_CXX_COMPILER g++)

project(LibDLIntercept)
cmake_minimum_required(VERSION 3.1)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall")

if(CMAKE_BUILD_TYPE MATCHES "Debug")
  add_definitions(-DDEBUG=1)
endif()

# Shared split library
add_library(dl-intercept SHARED src/dl-intercept.cpp)
set_target_properties(dl-intercept PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
# Static libraries required as container may not have them
target_link_libraries(dl-intercept -static-libgcc -static-libstdc++)

find_package(Boost)
include_directories(${Boost_INCLUDE_DIRS})

install(TARGETS dl-intercept DESTINATION lib)
