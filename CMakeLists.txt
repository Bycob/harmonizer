cmake_minimum_required(VERSION 3.10)

project(Harmonizer)

set(CMAKE_C_STANDARD 11)

add_executable(harmonizer
    src/harmonizer.c
    src/jack_backend.c
    src/main.c
)

target_link_libraries(harmonizer m jack)

