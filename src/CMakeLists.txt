set(SOURCES
    codepoint.c
    cjk.c
    encoding.c
    hangul.c
    next.c
    mojibake.c
    normalization.c
    plane.c
    string.c
    version.c
)

add_library(mojibake_lib STATIC ${SOURCES})
