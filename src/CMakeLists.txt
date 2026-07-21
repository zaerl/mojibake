# The Mojibake library
#
# This file is distributed under the MIT License. See LICENSE for details.

set(SOURCES
    bidi.c
    break-line.c
    break-sentence.c
    break-word.c
    buffer.c
    case.c
    codepoint.c
    collation.c
    cjk.c
    display.c
    encoding.c
    east-asian-width.c
    emoji.c
    hangul.c
    filter.c
    identifier.c
    locales.c
    next.c
    mojibake.c
    normalization.c
    quick-check.c
    plane.c
    properties.c
    security.c
    segmentation.c
    string.c
    unicode-tables.c
    version.c
)

# The standard BUILD_SHARED_LIBS variable selects the library type.
add_library(mojibake_lib ${SOURCES})

add_library(Mojibake::mojibake ALIAS mojibake_lib)

mjb_configure_target(mojibake_lib)

# Consumers need a C11-capable compiler for the public C API.
target_compile_features(mojibake_lib PUBLIC c_std_11)

target_include_directories(
    mojibake_lib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

set_target_properties(mojibake_lib PROPERTIES
    C_VISIBILITY_PRESET hidden
    CXX_VISIBILITY_PRESET hidden
    EXPORT_NAME mojibake
    OUTPUT_NAME mojibake
    VISIBILITY_INLINES_HIDDEN ON
)

if(BUILD_SHARED_LIBS)
    set_target_properties(mojibake_lib PROPERTIES
        SOVERSION ${PROJECT_VERSION_MAJOR}
        VERSION ${PROJECT_VERSION}
    )
    target_compile_definitions(mojibake_lib
        PRIVATE MJB_BUILDING_LIBRARY=1
        PUBLIC MJB_SHARED=1
    )
endif()

# Force C++ compilation if MJB_BUILD_CPP is enabled
if(MJB_BUILD_CPP)
    set_target_properties(mojibake_lib PROPERTIES
        LINKER_LANGUAGE CXX
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
    )
    set_source_files_properties(${SOURCES} PROPERTIES LANGUAGE CXX)
endif()

# Enable embedded NULL support for library if requested
if(MJB_ALLOW_EMBEDDED_NULLS)
    message(STATUS "Building library with embedded NULL support enabled")
    target_compile_definitions(mojibake_lib PRIVATE MJB_DANGEROUSLY_ALLOW_EMBEDDED_NULLS=1)
endif()

if(NOT MJB_FEATURE_CHARACTER_NAMES)
    message(STATUS "Building library without Unicode character name tables")
    target_compile_definitions(mojibake_lib PUBLIC MJB_FEATURE_CHARACTER_NAMES=0)
endif()

# WASM-specific configuration
if(MJB_BUILD_WASM)
    # Emscripten's EM_ASM_* macros require GNU C mode.
    set_target_properties(mojibake_lib PROPERTIES C_EXTENSIONS ON)

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
        "\"_mjb_codepoint_info\""
        "\"_mjb_normalize\""
        "\"_mjb_filter\""
        "\"_mjb_nfkc_casefold\""
        "\"_mjb_normalization_quick_check\""
        "\"_mjb_detect_encoding\""
        "\"_mjb_is_ascii\""
        "\"_mjb_is_utf8\""
        "\"_mjb_is_utf16\""
        "\"_mjb_count_codepoints\""
        "\"_mjb_string_each_character\""
        "\"_mjb_codepoint_property_binary\""
        "\"_mjb_codepoint_property_int\""
        "\"_mjb_codepoint_numeric_value\""
        "\"_mjb_codepoint_block\""
        "\"_mjb_codepoint_script\""
        "\"_mjb_codepoint_script_extensions\""
        "\"_mjb_codepoint_encode\""
        "\"_mjb_convert_encoding\""
        "\"_mjb_collation_compare\""
        "\"_mjb_collation_key\""
        "\"_mjb_map_case\""
        "\"_mjb_codepoint_is_valid\""
        "\"_mjb_codepoint_is_graphic\""
        "\"_mjb_codepoint_is_combining\""
        "\"_mjb_codepoint_is_hangul_leading_jamo\""
        "\"_mjb_codepoint_is_hangul_vowel_jamo\""
        "\"_mjb_codepoint_is_hangul_trailing_jamo\""
        "\"_mjb_codepoint_is_hangul_jamo\""
        "\"_mjb_codepoint_is_hangul_syllable\""
        "\"_mjb_codepoint_is_cjk_ideograph\""
        "\"_mjb_codepoint_is_cjk_extension_ideograph\""
        "\"_mjb_category_is_graphic\""
        "\"_mjb_category_is_combining\""
        "\"_mjb_next_line_break\""
        "\"_mjb_next_word_break\""
        "\"_mjb_next_sentence_break\""
        "\"_mjb_next_grapheme_break\""
        "\"_mjb_truncate_grapheme\""
        "\"_mjb_truncate_grapheme_width\""
        "\"_mjb_truncate_word\""
        "\"_mjb_truncate_word_width\""
        "\"_mjb_bidi_resolve\""
        "\"_mjb_codepoint_is_id_start\""
        "\"_mjb_codepoint_is_id_continue\""
        "\"_mjb_codepoint_is_xid_start\""
        "\"_mjb_codepoint_is_xid_continue\""
        "\"_mjb_codepoint_is_pattern_syntax\""
        "\"_mjb_codepoint_is_pattern_white_space\""
        "\"_mjb_is_identifier\""
        "\"_mjb_property_name\""
        "\"_mjb_confusable_skeleton\""
        "\"_mjb_are_confusable\""
        "\"_mjb_codepoint_emoji_properties\""
        "\"_mjb_codepoint_is_emoji\""
        "\"_mjb_codepoint_is_emoji_presentation\""
        "\"_mjb_codepoint_is_emoji_modifier\""
        "\"_mjb_codepoint_is_emoji_modifier_base\""
        "\"_mjb_codepoint_is_emoji_component\""
        "\"_mjb_codepoint_is_extended_pictographic\""
        "\"_mjb_codepoint_plane\""
        "\"_mjb_plane_is_valid\""
        "\"_mjb_plane_name\""
        "\"_mjb_classify_emoji_sequence\""
        "\"_mjb_is_emoji_sequence\""
        "\"_mjb_is_rgi_emoji\""
        "\"_mjb_hangul_syllable_name\""
        "\"_mjb_codepoint_east_asian_width\""
        "\"_mjb_display_width\""
        "\"_mjb_locale_parse\""
        "\"_mjb_set_locale\""
        "\"_mjb_get_locale\""
        "\"_mjb_version\""
        "\"_mjb_version_number\""
        "\"_mjb_unicode_version\""
        # Core memory functions
        "\"_malloc\""
        "\"_realloc\""
        "\"_free\""
    )

    string(JOIN "," EXPORTED_FUNCTIONS_STRING ${EXPORTED_FUNCTIONS})

    set(EXPORTED_RUNTIME_METHODS
        "\"ccall\""
        "\"cwrap\""
        "\"HEAPU8\""
        "\"HEAPU16\""
        "\"HEAPU32\""
        "\"HEAP32\""
    )

    string(JOIN "," EXPORTED_RUNTIME_METHODS_STRING ${EXPORTED_RUNTIME_METHODS})

    # Create WASM module executable
    add_executable(mojibake mojibake.c)
    target_link_libraries(mojibake mojibake_lib)
    mjb_configure_target(mojibake)

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

endif()

if(MJB_INSTALL AND NOT MJB_BUILD_WASM)
    install(
        TARGETS mojibake_lib
        EXPORT MojibakeTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

    install(
        FILES
            mojibake.h
            unicode.h
            locales.h
            cpp/mojibake.hpp
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
endif()
