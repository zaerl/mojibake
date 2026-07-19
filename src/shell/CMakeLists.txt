# The Mojibake library
#
# This file is distributed under the MIT License. See LICENSE for details.

set(SOURCES
    characters.c
    commands/bidi.c
    commands/break.c
    commands/character.c
    commands/codepoint.c
    commands/emoji.c
    commands/filter.c
    commands/locale.c
    commands/normalize.c
    commands/string.c
    main.c
    maps.c
    screen.c
    shell.c
)

# Add getopt compatibility for Windows
if(WIN32)
    list(APPEND SOURCES ./getopt/getopt.c)
endif()

add_executable(mojibake ${SOURCES})
target_link_libraries(mojibake PRIVATE mojibake_lib)
mjb_configure_target(mojibake)

# The shell uses strdup and strsep, which strict C11 hides on glibc/musl.
target_compile_definitions(mojibake PRIVATE _DEFAULT_SOURCE)

# Force C++ compilation if MJB_BUILD_CPP is enabled
if(MJB_BUILD_CPP)
    set_target_properties(mojibake PROPERTIES LINKER_LANGUAGE CXX)
    set_source_files_properties(${SOURCES} PROPERTIES LANGUAGE CXX)
endif()

if(MSVC)
    # Suppress warnings from the Microsoft compiler
    target_compile_options(mojibake
        PRIVATE
        /wd4996 # Suppress 'the POSIX name for this item is deprecated'
    )
endif()
