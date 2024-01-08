

if(NOT DEFINED dr_libs_SOURCE_DIR)
    include(FetchContent)
    FetchContent_Declare(
        dr_libs
        GIT_REPOSITORY https://github.com/mackron/dr_libs.git
        GIT_TAG        a4b73d3d423e1cea0652b76d0806e7620337a40f # master 2024 Jan 7
    )
    FetchContent_Populate(dr_libs)
endif()


add_library(
    audioplus
    src/wav.cpp
)

target_include_directories(
    audioplus
    PUBLIC include
    PRIVATE ${dr_libs_SOURCE_DIR}
)