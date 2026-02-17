set(SOURCES
    breaking.c
    buffer.c
    case.c
    codepoint.c
    cjk.c
    display.c
    encoding.c
    east-asian-width.c
    emoji.c
    hangul.c
    filter.c
    next.c
    mojibake.c
    normalization.c
    quick-check.c
    plane.c
    properties.c
    segmentation.c
    string.c
    version.c
)

# Build as shared or static library based on BUILD_SHARED option
if(BUILD_SHARED)
    add_library(mojibake_lib SHARED ${SOURCES})
else()
    add_library(mojibake_lib STATIC ${SOURCES})
endif()

# Link SQLite library
target_link_libraries(mojibake_lib PRIVATE mojibake_sqlite)

# Force C++ compilation if BUILD_CPP is enabled
if(BUILD_CPP)
    set_target_properties(mojibake_lib PROPERTIES
        LINKER_LANGUAGE CXX
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
    )
    set_source_files_properties(${SOURCES} PROPERTIES LANGUAGE CXX)
endif()

# Add embedded database support
if(USE_EMBEDDED_DB)
    target_compile_definitions(mojibake_lib PRIVATE MJB_USE_EMBEDDED_DB)
endif()

# Enable embedded NULL support for library if requested
if(ALLOW_EMBEDDED_NULLS)
    message(STATUS "Building library with embedded NULL support enabled")
    target_compile_definitions(mojibake_lib PRIVATE MJB_DANGEROUSLY_ALLOW_EMBEDDED_NULLS=1)
endif()

# WASM-specific configuration
if(BUILD_WASM)
    # EM_ASM_* macros
    target_compile_options(
        mojibake_lib
        PRIVATE
        -Wno-variadic-macros
        -Wno-variadic-macro-arguments-omitted
        -Wno-dollar-in-identifier-extension
    )

    # This is auto-generated, do not edit this array.
    set(EXPORTED_FUNCTIONS
        "\"_mjb_codepoint_character\""
        "\"_mjb_normalize\""
        "\"_mjb_next_character\""
        "\"_mjb_string_is_normalized\""
        "\"_mjb_string_filter\""
        "\"_mjb_codepoint_has_property\""
        "\"_mjb_codepoint_properties\""
        "\"_mjb_string_encoding\""
        "\"_mjb_string_is_utf8\""
        "\"_mjb_string_is_utf16\""
        "\"_mjb_string_is_ascii\""
        "\"_mjb_codepoint_encode\""
        "\"_mjb_string_convert_encoding\""
        "\"_mjb_strnlen\""
        "\"_mjb_string_compare\""
        "\"_mjb_case\""
        "\"_mjb_codepoint_is_valid\""
        "\"_mjb_codepoint_is_graphic\""
        "\"_mjb_codepoint_is_combining\""
        "\"_mjb_codepoint_is_hangul_syllable\""
        "\"_mjb_codepoint_is_cjk_ideograph\""
        "\"_mjb_category_is_graphic\""
        "\"_mjb_category_is_combining\""
        "\"_mjb_codepoint_block\""
        "\"_mjb_codepoint_to_lowercase\""
        "\"_mjb_codepoint_to_uppercase\""
        "\"_mjb_codepoint_to_titlecase\""
        "\"_mjb_break_line\""
        "\"_mjb_segmentation\""
        "\"_mjb_codepoint_plane\""
        "\"_mjb_plane_is_valid\""
        "\"_mjb_plane_name\""
        "\"_mjb_codepoint_emoji\""
        "\"_mjb_codepoint_east_asian_width\""
        "\"_mjb_display_width\""
        "\"_mjb_version\""
        "\"_mjb_version_number\""
        "\"_mjb_unicode_version\""
        "\"_mjb_initialize_v2\""
        # Core memory functions
        "\"_malloc\""
        "\"_realloc\""
        "\"_free\""
    )

    string(JOIN "," EXPORTED_FUNCTIONS_STRING ${EXPORTED_FUNCTIONS})

    set(EXPORTED_RUNTIME_METHODS
        "\"ccall\""
        "\"cwrap\""
        "\"UTF8ToString\""
        "\"UTF16ToString\""
        "\"UTF32ToString\""
        "\"stringToUTF16\""
        "\"stringToUTF32\""
        "\"HEAPU8\""
        "\"HEAPU16\""
        "\"HEAPU32\""
        "\"HEAP32\""
    )

    string(JOIN "," EXPORTED_RUNTIME_METHODS_STRING ${EXPORTED_RUNTIME_METHODS})

    # Create WASM module executable
    add_executable(mojibake mojibake.c)
    target_link_libraries(mojibake mojibake_lib mojibake_sqlite)

    # Set Emscripten-specific flags for the WASM executable
    target_link_options(mojibake PRIVATE
        "SHELL:-s WASM=1"
        "SHELL:-s EXPORT_ES6=1"
        "SHELL:-s MODULARIZE=1"
        "SHELL:-s EXPORTED_FUNCTIONS=[${EXPORTED_FUNCTIONS_STRING}]"
        "SHELL:-s EXPORTED_RUNTIME_METHODS=[${EXPORTED_RUNTIME_METHODS_STRING}]"
        "SHELL:-s ALLOW_MEMORY_GROWTH=1"
    )

    # Set output file extensions for WASM
    set_target_properties(mojibake PROPERTIES
        SUFFIX ".js"
    )

    # Copy entire site directory for WASM assets
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/site/
         DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

    # Generate database file for WASM
    configure_file(
        ${CMAKE_SOURCE_DIR}/mojibake.db
        ${CMAKE_CURRENT_BINARY_DIR}/mojibake.db
        COPYONLY
    )
endif()
