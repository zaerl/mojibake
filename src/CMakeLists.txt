set(SOURCES
    codepoint.c
    encoding.c
    mojibake.c
    normalization.c
    plane.c
    version.c
)

add_library(mojibake_lib STATIC ${SOURCES})
