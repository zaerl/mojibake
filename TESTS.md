# Test coverage

Mojibake runs a total of **4,647,523** C assertions and **172** JavaScript assertions, including all
the official tests included in the standard:

1. [auxiliary/GraphemeBreakTest.txt](https://www.unicode.org/Public/18.0.0/ucd/auxiliary/GraphemeBreakTest.txt)
2. [auxiliary/LineBreakTest.txt](https://www.unicode.org/Public/18.0.0/ucd/auxiliary/LineBreakTest.txt)
3. [auxiliary/SentenceBreakTest.txt](https://www.unicode.org/Public/18.0.0/ucd/auxiliary/SentenceBreakTest.txt)
4. [auxiliary/WordBreakTest.txt](https://www.unicode.org/Public/18.0.0/ucd/auxiliary/WordBreakTest.txt)
5. [BidiCharacterTest.txt](https://www.unicode.org/Public/18.0.0/ucd/BidiCharacterTest.txt)
6. [BidiTest.txt](https://www.unicode.org/Public/18.0.0/ucd/BidiTest.txt)
7. [CaseFolding.txt](https://www.unicode.org/Public/18.0.0/ucd/CaseFolding.txt)
8. [CollationTest/CollationTest_NON_IGNORABLE.txt](https://www.unicode.org/Public/18.0.0/uca/CollationTest.zip)
9. [CollationTest/CollationTest_SHIFTED.txt](https://www.unicode.org/Public/18.0.0/uca/CollationTest.zip)
10. [emoji-test.txt](https://www.unicode.org/Public/18.0.0/emoji/emoji-test.txt)
11. [IdnaTestV2.txt](https://www.unicode.org/Public/18.0.0/idna/IdnaTestV2.txt)
12. [UTS #39 data (confusables.txt and intentional.txt)](https://www.unicode.org/Public/18.0.0/security/uts39-data-18.0.0.zip)
13. [NormalizationTest.txt](https://www.unicode.org/Public/18.0.0/ucd/NormalizationTest.txt)
14. [SpecialCasing.txt](https://www.unicode.org/Public/18.0.0/ucd/SpecialCasing.txt)

## C

| Test                                       | Coverage    |
| ------------------------------------------ | ----------- |
| `mjb_normalize_into`                       | 2824002     |
| `mjb_bidi_resolve`                         | 582612      |
| `mjb_normalize`                            | 424439      |
| `mjb_collation_compare`                    | 424217      |
| `mjb_bidi_reorder_line`                    | 91723       |
| `mjb_codepoint_info`                       | 82678       |
| `mjb_nfkc_casefold_into`                   | 74799       |
| `mjb_line_break_next`                      | 38699       |
| `mjb_nfkc_casefold`                        | 32099       |
| `mjb_map_case_into`                        | 24757       |
| `mjb_result_free`                          | 14295       |
| `mjb_idna_to_ascii`                        | 6461        |
| `mjb_idna_to_unicode`                      | 6455        |
| `mjb_emoji_sequence_info`                  | 5287        |
| `mjb_word_break_next`                      | 3892        |
| `mjb_map_case`                             | 3514        |
| `mjb_grapheme_break_next`                  | 1734        |
| `mjb_codepoint_emoji_properties`           | 1429        |
| `mjb_sentence_break_next`                  | 1028        |
| `mjb_codepoint_script_extensions`          | 642         |
| `mjb_confusable_match`                     | 362         |
| `mjb_filter`                               | 194         |
| `mjb_convert_encoding`                     | 155         |
| `mjb_collation_key`                        | 139         |
| `mjb_codepoint_count`                      | 135         |
| `mjb_caseless_match`                       | 119         |
| `mjb_confusable_skeleton_into`             | 117         |
| `mjb_codepoint_encode`                     | 106         |
| `mjb_resolved_script_set`                  | 100         |
| `mjb_locale_parse`                         | 92          |
| `mjb_confusable_skeleton`                  | 88          |
| `mjb_terminal_width`                       | 82          |
| `mjb_collation_key_into`                   | 78          |
| `mjb_filter_into`                          | 73          |
| `mjb_idna_to_ascii_into`                   | 63          |
| `mjb_word_count`                           | 51          |
| `mjb_grapheme_count`                       | 47          |
| `mjb_idna_to_unicode_into`                 | 46          |
| `mjb_codepoint_is_valid`                   | 40          |
| `mjb_validate_encoding`                    | 40          |
| `mjb_normalization_quick_check`            | 38          |
| `mjb_codepoint_east_asian_width`           | 35          |
| `mjb_sentence_count`                       | 35          |
| `mjb_utf8_grapheme_snprintf`               | 31          |
| `mjb_decode_next`                          | 28          |
| `mjb_convert_encoding_into`                | 26          |
| `mjb_utf8_snprintf`                        | 21          |
| `mjb_codepoint_numeric_value`              | 20          |
| `mjb_decode_previous`                      | 20          |
| `mjb_hangul_syllable_name`                 | 19          |
| `mjb_is_utf8`                              | 19          |
| `mjb_codepoint_is_cjk_extension_ideograph` | 17          |
| `mjb_for_each_codepoint`                   | 17          |
| `mjb_codepoint_block`                      | 16          |
| `mjb_is_utf16`                             | 16          |
| `mjb_hangul_syllable_compose`              | 14          |
| `mjb_is_identifier`                        | 14          |
| `mjb_bidi_line_runs`                       | 12          |
| `mjb_detect_encoding`                      | 12          |
| `mjb_grapheme_prefix_length`               | 12          |
| `mjb_grapheme_prefix_length_for_width`     | 11          |
| `mjb_is_ascii`                             | 11          |
| `mjb_codepoint_script`                     | 10          |
| `mjb_plane_name`                           | 9           |
| `mjb_word_prefix_length`                   | 9           |
| `mjb_codepoint_is_cjk_ideograph`           | 8           |
| `mjb_word_prefix_length_for_width`         | 8           |
| `mjb_codepoint_is_id_start`                | 7           |
| `mjb_codepoint_property_binary`            | 7           |
| `mjb_codepoint_is_combining`               | 6           |
| `mjb_codepoint_is_graphic`                 | 6           |
| `mjb_codepoint_is_id_continue`             | 6           |
| `mjb_codepoint_property_int`               | 6           |
| `mjb_hangul_syllable_decompose`            | 6           |
| `mjb_locale_code`                          | 6           |
| `mjb_codepoint_is_hangul_jamo`             | 5           |
| `mjb_codepoint_is_hangul_trailing_jamo`    | 5           |
| `mjb_is_rgi_emoji`                         | 5           |
| `mjb_script_code`                          | 5           |
| `mjb_codepoint_is_hangul_leading_jamo`     | 4           |
| `mjb_codepoint_is_hangul_vowel_jamo`       | 4           |
| `mjb_codepoint_is_pattern_syntax`          | 4           |
| `mjb_codepoint_is_pattern_white_space`     | 4           |
| `mjb_codepoint_plane`                      | 4           |
| `mjb_is_emoji_sequence`                    | 4           |
| `mjb_plane_is_valid`                       | 4           |
| `mjb_status_message`                       | 4           |
| `mjb_bidi_paragraph_free`                  | 3           |
| `mjb_category_is_combining`                | 3           |
| `mjb_category_is_graphic`                  | 3           |
| `mjb_codepoint_is_emoji`                   | 3           |
| `mjb_codepoint_is_xid_continue`            | 3           |
| `mjb_codepoint_is_xid_start`               | 3           |
| `mjb_property_name`                        | 3           |
| `mjb_codepoint_is_emoji_component`         | 2           |
| `mjb_codepoint_is_emoji_modifier`          | 2           |
| `mjb_codepoint_is_emoji_modifier_base`     | 2           |
| `mjb_codepoint_is_emoji_presentation`      | 2           |
| `mjb_codepoint_is_extended_pictographic`   | 2           |
| `mjb_codepoint_is_hangul_syllable`         | 2           |
| `mjb_unicode_version`                      | 2           |
| `mjb_utf8_grapheme_vsnprintf`              | 2           |
| `mjb_utf8_vsnprintf`                       | 2           |
| `mjb_version`                              | 2           |
| `mjb_version_number`                       | 2           |
| `mjb_set_allocator`                        | 1           |
| **Total**                                  | **4647523** |

## JavaScript

| Test                                        | Coverage   |
| ------------------------------------------- | ---------- |
| `Mojibake.collationCompare`                 | 18         |
| `Mojibake.codepointCount`                   | 9          |
| `Mojibake.caselessMatch`                    | 8          |
| `Mojibake.mapCase`                          | 8          |
| `Mojibake.collationKey`                     | 6          |
| `Mojibake.convertEncoding`                  | 6          |
| `Mojibake.filter`                           | 6          |
| `Mojibake.normalize`                        | 6          |
| `Mojibake.confusableMatch`                  | 5          |
| `Mojibake.graphemeCount`                    | 5          |
| `Mojibake.resolvedScriptSet`                | 5          |
| `Mojibake.wordCount`                        | 5          |
| `Mojibake.idnaToAscii`                      | 4          |
| `Mojibake.terminalWidth`                    | 4          |
| `Mojibake.codepointPropertyBinary`          | 3          |
| `Mojibake.nfkcCasefold`                     | 3          |
| `Mojibake.normalizationQuickCheck`          | 3          |
| `Mojibake.sentenceCount`                    | 3          |
| `Mojibake.codepointPropertyInt`             | 2          |
| `Mojibake.confusableSkeleton`               | 2          |
| `Mojibake.bidiResolve`                      | 1          |
| `Mojibake.categoryIsCombining`              | 1          |
| `Mojibake.categoryIsGraphic`                | 1          |
| `Mojibake.codepointBlock`                   | 1          |
| `Mojibake.codepointEastAsianWidth`          | 1          |
| `Mojibake.codepointEmojiProperties`         | 1          |
| `Mojibake.codepointEncode`                  | 1          |
| `Mojibake.codepointInfo`                    | 1          |
| `Mojibake.codepointIsCJKExtensionIdeograph` | 1          |
| `Mojibake.codepointIsCjkIdeograph`          | 1          |
| `Mojibake.codepointIsCombining`             | 1          |
| `Mojibake.codepointIsEmoji`                 | 1          |
| `Mojibake.codepointIsEmojiComponent`        | 1          |
| `Mojibake.codepointIsEmojiModifier`         | 1          |
| `Mojibake.codepointIsEmojiModifierBase`     | 1          |
| `Mojibake.codepointIsEmojiPresentation`     | 1          |
| `Mojibake.codepointIsExtendedPictographic`  | 1          |
| `Mojibake.codepointIsGraphic`               | 1          |
| `Mojibake.codepointIsHangulJamo`            | 1          |
| `Mojibake.codepointIsHangulLeadingJamo`     | 1          |
| `Mojibake.codepointIsHangulSyllable`        | 1          |
| `Mojibake.codepointIsHangulTrailingJamo`    | 1          |
| `Mojibake.codepointIsHangulVowelJamo`       | 1          |
| `Mojibake.codepointIsIdContinue`            | 1          |
| `Mojibake.codepointIsIdStart`               | 1          |
| `Mojibake.codepointIsPatternSyntax`         | 1          |
| `Mojibake.codepointIsPatternWhiteSpace`     | 1          |
| `Mojibake.codepointIsValid`                 | 1          |
| `Mojibake.codepointIsXidContinue`           | 1          |
| `Mojibake.codepointIsXidStart`              | 1          |
| `Mojibake.codepointNumericValue`            | 1          |
| `Mojibake.codepointPlane`                   | 1          |
| `Mojibake.codepointScript`                  | 1          |
| `Mojibake.codepointScriptExtensions`        | 1          |
| `Mojibake.create`                           | 1          |
| `Mojibake.detectEncoding`                   | 1          |
| `Mojibake.forEachCodepoint`                 | 1          |
| `Mojibake.graphemeBreakNext`                | 1          |
| `Mojibake.graphemePrefixLength`             | 1          |
| `Mojibake.graphemePrefixLengthForWidth`     | 1          |
| `Mojibake.hangulSyllableName`               | 1          |
| `Mojibake.idnaToUnicode`                    | 1          |
| `Mojibake.isASCII`                          | 1          |
| `Mojibake.isEmojiSequence`                  | 1          |
| `Mojibake.isIdentifier`                     | 1          |
| `Mojibake.isRGIEmoji`                       | 1          |
| `Mojibake.isUTF16`                          | 1          |
| `Mojibake.isUTF8`                           | 1          |
| `Mojibake.lineBreakNext`                    | 1          |
| `Mojibake.localeParse`                      | 1          |
| `Mojibake.planeIsValid`                     | 1          |
| `Mojibake.planeName`                        | 1          |
| `Mojibake.propertyName`                     | 1          |
| `Mojibake.sentenceBreakNext`                | 1          |
| `Mojibake.statusMessage`                    | 1          |
| `Mojibake.unicodeVersion`                   | 1          |
| `Mojibake.version`                          | 1          |
| `Mojibake.versionNumber`                    | 1          |
| `Mojibake.wordBreakNext`                    | 1          |
| `Mojibake.wordPrefixLength`                 | 1          |
| `Mojibake.wordPrefixLengthForWidth`         | 1          |
| `Mojibake.emojiSequenceInfo`                | 0          |
| `Mojibake.localeCode`                       | 0          |
| `Mojibake.scriptCode`                       | 0          |
| **Total**                                   | **172**    |
