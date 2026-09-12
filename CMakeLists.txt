cmake_minimum_required(VERSION 3.20)
project(texture_filtering_lab LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(minifb STATIC
    external/minifb/src/MiniFB_common.c
    external/minifb/src/MiniFB_cpp.cpp
    external/minifb/src/MiniFB_internal.c
    external/minifb/src/MiniFB_timer.c
    external/minifb/src/windows/WinMiniFB.c
)
target_compile_definitions(minifb PRIVATE _CRT_SECURE_NO_WARNINGS)
target_include_directories(minifb PUBLIC
    external/minifb/include
    external/minifb/src
    external/minifb/src/windows
)
target_link_libraries(minifb PUBLIC winmm)

add_executable(texture_filtering_lab src/main.cpp)
target_link_libraries(texture_filtering_lab PRIVATE minifb)
