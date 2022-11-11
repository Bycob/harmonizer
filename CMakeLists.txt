cmake_minimum_required(VERSION 3.10)

project(Harmonizer)

# User set variables

set(HARMONIZER_Q_PATH CACHE STRING "")

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

add_executable(harmonizer
    src/harmonizer.c
    src/jack_backend.c
    src/pitch_detection.cc
    src/main.c
)

set(Q_INCLUDE "${HARMONIZER_Q_PATH}/q_lib/include/")
message(STATUS "Q headers location is ${Q_INCLUDE}")
include_directories("${Q_INCLUDE}" "${HARMONIZER_Q_PATH}/infra/include/")

target_link_libraries(harmonizer fftw3f m jack)

