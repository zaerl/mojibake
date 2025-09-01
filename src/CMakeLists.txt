set(SOURCES
    breaking.c
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

# Link SQLite library
# target_link_libraries(mojibake_lib PRIVATE mojibake_sqlite)

# WASM-specific configuration
if(BUILD_WASM)
    # Create WASM module executable
    add_executable(mojibake mojibake.c)
    target_link_libraries(mojibake mojibake_lib mojibake_sqlite)

    # Set Emscripten-specific flags for the WASM executable
    target_link_options(mojibake PRIVATE
        "SHELL:-s WASM=1"
        "SHELL:-s EXPORT_ES6=1"
        "SHELL:-s MODULARIZE=1"
        "SHELL:-s EXPORTED_FUNCTIONS=[\"_malloc\",\"_free\",\"_mjb_version\"]"
        "SHELL:-s EXPORTED_RUNTIME_METHODS=[\"ccall\",\"cwrap\"]"
        "SHELL:-s ALLOW_MEMORY_GROWTH=1"
    )

    # Set output file extensions for WASM
    set_target_properties(mojibake PROPERTIES
        SUFFIX ".js"
    )

    # Generate example HTML file for WASM
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/wasm.html
        ${CMAKE_CURRENT_BINARY_DIR}/wasm.html
        COPYONLY
    )
endif()
