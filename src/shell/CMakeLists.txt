set(SOURCES
    characters.c
    commands/break.c
    commands/character.c
    commands/codepoint.c
    commands/normalize.c
    commands/string.c
    main.c
    maps.c
    shell.c
)

# Add getopt compatibility for Windows
if(WIN32)
    list(APPEND SOURCES ./getopt/getopt.c)
endif()

add_executable(mojibake ${SOURCES})
target_link_libraries(mojibake PRIVATE mojibake_lib mojibake_sqlite)

# Force C++ compilation if BUILD_CPP is enabled
if(BUILD_CPP)
    set_target_properties(mojibake PROPERTIES LINKER_LANGUAGE CXX)
    set_source_files_properties(${SOURCES} PROPERTIES LANGUAGE CXX)
endif()
