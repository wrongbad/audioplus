
include(FetchContent)

# if you define dr_libs_SOURCE_DIR ahead of time
# this download step will be skipped
if(NOT DEFINED dr_libs_SOURCE_DIR)
    FetchContent_Declare(
        dr_libs
        GIT_REPOSITORY https://github.com/mackron/dr_libs.git
        GIT_TAG        a4b73d3d423e1cea0652b76d0806e7620337a40f # 2023 Dec 1
        GIT_SUBMODULES "" # skip broken submodule links
    )
    FetchContent_Populate(dr_libs)
endif()


find_package(portaudio)
if(NOT ${portaudio_FOUND})
    FetchContent_Declare(
        portaudio
        GIT_REPOSITORY https://github.com/PortAudio/portaudio.git
        GIT_TAG        v19.7.0 # 2021 Apr 6
    )
    FetchContent_MakeAvailable(portaudio)
endif()


find_package(portmidi)
if(NOT ${portmidi_FOUND})
    FetchContent_Declare(
        portmidi
        GIT_REPOSITORY https://github.com/PortMidi/portmidi.git
        GIT_TAG        v2.0.4 # 2022 Oct 7
    )
    FetchContent_MakeAvailable(portmidi)
endif()


add_library(audioplus_wav EXCLUDE_FROM_ALL src/wav.cpp)
target_include_directories(
    audioplus_wav
    PUBLIC include
    PRIVATE ${dr_libs_SOURCE_DIR}
)

add_library(audioplus_audio EXCLUDE_FROM_ALL src/audio.cpp)
target_include_directories(audioplus_audio PUBLIC include)
target_link_libraries(audioplus_audio PRIVATE portaudio)

add_library(audioplus_midi EXCLUDE_FROM_ALL src/midi.cpp)
target_include_directories(audioplus_midi PUBLIC include)
target_link_libraries(audioplus_midi PRIVATE portmidi)