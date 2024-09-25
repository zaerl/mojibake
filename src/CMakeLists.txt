set(SOURCES
    codepoint.c
    db.c
    encoding.c
    normalization.c
    plane.c
    version.c
)

add_library(mojibake_lib STATIC ${SOURCES})
