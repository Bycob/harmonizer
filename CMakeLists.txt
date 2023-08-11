cmake_minimum_required(VERSION 3.10)

project(Harmonizer)

# User set variables
# ...

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

# Add third party
add_subdirectory(third_party/)

set(HARMONIZER_SOURCES
    src/harmonizer_dsp.c
    src/pitch_detection.cc
    src/fourier.c
    src/math_utils.c

    src/harmonizer_app.cc
    src/harmonizer_midi.cc
    src/jack_backend.c
    src/main.cc
)

# Visualizer
if (SDL2_FOUND)
    message(STATUS "SDL2 detected, building visualizer")
    add_definitions(-DVISUALIZER)
    list(APPEND HARMONIZER_SOURCES
        src/visualizer.c)
endif(SDL2_FOUND)

add_executable(harmonizer
    ${HARMONIZER_SOURCES}
)

include_directories(${THIRD_PARTY_INCLUDES})
add_dependencies(harmonizer ${THIRD_PARTY_DEPENDENCIES})

target_link_libraries(harmonizer
    m
    jack
    asound
    ${THIRD_PARTY_LIBRARIES})

# Alsa
find_package(ALSA REQUIRED)
if (ALSA_FOUND)
    target_link_libraries(harmonizer ${ALSA_LIBRARIES})
endif(ALSA_FOUND)

