
include(FetchContent)

# if you define dr_libs_SOURCE_DIR ahead of time
# this download step will be skipped
if(NOT DEFINED dr_libs_SOURCE_DIR)
    FetchContent_Declare(
        dr_libs
        GIT_REPOSITORY https://github.com/mackron/dr_libs.git
        GIT_TAG        a4b73d3d423e1cea0652b76d0806e7620337a40f # master 2024 Jan 7
    )
    FetchContent_Populate(dr_libs)
endif()

find_package(portaudio)
message(portaudio_FOUND = ${portaudio_FOUND})
if(NOT ${portaudio_FOUND})
    FetchContent_Declare(
        portaudio
        GIT_REPOSITORY https://github.com/PortAudio/portaudio.git
        GIT_TAG        v19.7.0 # 2021 Apr 6
    )
    FetchContent_MakeAvailable(portaudio)
endif()

add_library(audioplus_wav EXCLUDE_FROM_ALL src/wav.cpp)

target_include_directories(
    audioplus_wav
    PUBLIC include
    PRIVATE ${dr_libs_SOURCE_DIR}
)


add_library(audioplus_audio EXCLUDE_FROM_ALL src/audio.cpp)

target_include_directories(
    audioplus_audio
    PUBLIC include
)
target_link_libraries(
    audioplus_audio
    PRIVATE portaudio
)