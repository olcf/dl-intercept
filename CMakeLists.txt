project(LibDLIntercept)
cmake_minimum_required(VERSION 3.1)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_BUILD_TYPE MATCHES "Debug")
  add_definitions(-DDEBUG=1)
endif()

# Shared split library
add_library(dl-intercept SHARED src/dl-intercept.cpp)
set_target_properties(dl-intercept PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

find_package(Boost COMPONENTS regex REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})
target_link_libraries(dl-intercept ${Boost_LIBRARIES})

install(TARGETS dl-intercept DESTINATION lib)
