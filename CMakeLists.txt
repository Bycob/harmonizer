cmake_minimum_required(VERSION 3.10)

project(Harmonizer)

# User set variables
# ...

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

# Add third party
add_subdirectory(third_party/)

add_executable(harmonizer
    src/harmonizer_dsp.c
    src/pitch_detection.cc

    src/harmonizer_app.c
    src/harmonizer_midi.cc
    src/jack_backend.c
    src/main.c
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
    target_link_libraries (harmonizer ${ALSA_LIBRARIES})
endif(ALSA_FOUND)
