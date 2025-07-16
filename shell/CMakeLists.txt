set(SOURCES
    main.c
    maps.c
)

add_executable(mojibake ${SOURCES})
target_link_libraries(mojibake PRIVATE mojibake_lib mojibake_sqlite)
