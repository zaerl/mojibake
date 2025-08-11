set(SOURCES
    buffer.c
    case.c
    codepoint.c
    cjk.c
    encoding.c
    hangul.c
    next.c
    mojibake.c
    normalization.c
    quick-check.c
    plane.c
    segmentation.c
    string.c
    version.c
)

add_library(mojibake_lib STATIC ${SOURCES})
