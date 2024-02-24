
include(ExternalProject)

set(THIRD_PARTY_LIBRARIES "")
set(THIRD_PARTY_LIB_DIRS "")
set(THIRD_PARTY_INCLUDES "")
set(THIRD_PARTY_DEPENDENCIES "")

# TinyWav
add_library(tinywav STATIC tinywav/tinywav.c)
list(APPEND THIRD_PARTY_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/tinywav")
list(APPEND THIRD_PARTY_LIBRARIES tinywav)

# Q library
set(HARMONIZER_Q_PATH "${CMAKE_CURRENT_SOURCE_DIR}/Q")
set(Q_INCLUDE "${HARMONIZER_Q_PATH}/q_lib/include/")
message(STATUS "Q headers location is ${Q_INCLUDE}")
list(APPEND THIRD_PARTY_INCLUDES
    "${Q_INCLUDE}"
    "${HARMONIZER_Q_PATH}/infra/include/")

# FFTW3
set(FFTW3_INSTALL_DIR "${CMAKE_BINARY_DIR}/third_party/fftw3/install")
message(STATUS "Installing FFTW3 in ${FFTW3_INSTALL_DIR}")
ExternalProject_Add(
    fftw3
    PREFIX fftw3
    UPDATE_DISCONNECTED 1
    URL https://www.fftw.org/fftw-3.3.10.tar.gz
    CONFIGURE_COMMAND ./configure --enable-float --prefix "${FFTW3_INSTALL_DIR}"
    BUILD_COMMAND make -j4
    BUILD_IN_SOURCE 1
)
list(APPEND THIRD_PARTY_INCLUDES ${FFTW3_INSTALL_DIR}/include)
list(APPEND THIRD_PARTY_LIBRARIES "${FFTW3_INSTALL_DIR}/lib/libfftw3f.a")
list(APPEND THIRD_PARTY_DEPENDENCIES fftw3)

# RTmidi
set(RTMIDI_INSTALL_DIR "${CMAKE_BINARY_DIR}/third_party/rtmidi/install")
message(STATUS "Installing RTMidi in ${RTMIDI_INSTALL_DIR}")
ExternalProject_Add(
    rtmidi
    PREFIX rtmidi
    UPDATE_DISCONNECTED 1
    URL http://www.music.mcgill.ca/~gary/rtmidi/release/rtmidi-5.0.0.tar.gz
    CONFIGURE_COMMAND ./configure --prefix "${RTMIDI_INSTALL_DIR}"
    BUILD_COMMAND make -j4
    BUILD_IN_SOURCE 1
)
# https://github.com/thestk/rtmidi/archive/refs/tags/5.0.0.tar.gz
list(APPEND THIRD_PARTY_DEPENDENCIES rtmidi)
list(APPEND THIRD_PARTY_INCLUDES ${RTMIDI_INSTALL_DIR}/include)
list(APPEND THIRD_PARTY_LIBRARIES ${RTMIDI_INSTALL_DIR}/lib/librtmidi.a)

# Midi files
set(MIDIFILE_INSTALL_DIR "${CMAKE_BINARY_DIR}/third_party/midifile/src/midifile")
message(STATUS "Installing MidiFile in ${MIDIFILE_INSTALL_DIR}")
ExternalProject_Add(
    midifile
    PREFIX midifile
    UPDATE_DISCONNECTED 1
    URL https://github.com/craigsapp/midifile/tarball/0015eada0a315fbd663e728e7b1088acd9877f19
    CONFIGURE_COMMAND ""
    BUILD_COMMAND make -j6 library
    INSTALL_COMMAND ""
    BUILD_IN_SOURCE 1
)
list(APPEND THIRD_PARTY_DEPENDENCIES midifile)
list(APPEND THIRD_PARTY_INCLUDES ${MIDIFILE_INSTALL_DIR}/include)
list(APPEND THIRD_PARTY_LIBRARIES ${MIDIFILE_INSTALL_DIR}/lib/libmidifile.a)

# SDL2
find_package(SDL2)
if (SDL2_FOUND)
    list(APPEND THIRD_PARTY_LIBRARIES ${SDL2_LIBRARIES})
endif(SDL2_FOUND)
set(SDL2_FOUND ${SDL2_FOUND} PARENT_SCOPE)

# Pytorch
find_package(Torch HINTS "${Torch_DIR}/share/cmake/Torch")
if(Torch_FOUND)
    list(APPEND THIRD_PARTY_LIBRARIES ${TORCH_LIBRARIES})
    list(APPEND THIRD_PARTY_INCLUDES ${TORCH_INCLUDE_DIRS})
endif(Torch_FOUND)

set(THIRD_PARTY_LIBRARIES ${THIRD_PARTY_LIBRARIES} PARENT_SCOPE)
set(THIRD_PARTY_LIB_DIRS ${THIRD_PARTY_LIB_DIRS} PARENT_SCOPE)
set(THIRD_PARTY_INCLUDES ${THIRD_PARTY_INCLUDES} PARENT_SCOPE)
set(THIRD_PARTY_DEPENDENCIES ${THIRD_PARTY_DEPENDENCIES} PARENT_SCOPE)
