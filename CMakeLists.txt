cmake_minimum_required(VERSION 3.10)

project(se_solver LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(se_solver
    src/main.cpp
    src/SolvingManager.cpp
)

if(MSVC)
    target_compile_options(se_solver PRIVATE /W4 /WX)
else()
    target_compile_options(se_solver PRIVATE -Wall -Wextra -Werror -Wpedantic)
endif()
