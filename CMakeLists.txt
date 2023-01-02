cmake_minimum_required(VERSION 3.10)

project(Harmonizer)

# User set variables

set(HARMONIZER_Q_PATH CACHE STRING "")

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Add third party
add_subdirectory(third_party/)

add_executable(harmonizer
    src/harmonizer_dsp.c
    src/harmonizer_app.c
    src/jack_backend.c
    src/pitch_detection.cc
    src/main.c
)

message(STATUS ${THIRD_PARTY_INCLUDES})
include_directories(${THIRD_PARTY_INCLUDES})
add_dependencies(harmonizer ${THIRD_PARTY_DEPENDENCIES})

target_link_libraries(harmonizer
    m
    jack
    ${THIRD_PARTY_LIBRARIES})

