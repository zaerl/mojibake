set(SOURCES
    array.c
    codepoint.c
    data.c
    db.c
    encoding.c
    memory.c
    normalization.c
    plane.c
    version.c
)

add_library(mojibake_lib STATIC ${SOURCES})
