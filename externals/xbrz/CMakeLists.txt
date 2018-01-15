add_library(xbrz STATIC
    xbrz/config.h
    xbrz/xbrz.cpp
    xbrz/xbrz.h
)

create_target_directory_groups(xbrz)
target_include_directories(xbrz INTERFACE .)
