set(SOURCES
    codepoint.c
    encoding.c
    hangul.c
    mojibake.c
    normalization.c
    plane.c
    sort.c
    version.c
)

add_library(mojibake_lib STATIC ${SOURCES})
