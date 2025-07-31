set(SOURCES
    characters.c
    commands/character.c
    commands/normalize.c
    main.c
    maps.c
    shell.c
)

add_executable(mojibake ${SOURCES})
target_link_libraries(mojibake PRIVATE mojibake_lib mojibake_sqlite)
