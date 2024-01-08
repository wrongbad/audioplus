

add_library(
    drcpp
    src/wav.cpp
)
target_include_directories(
    drcpp
    PUBLIC include
    PRIVATE src
)