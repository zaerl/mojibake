# Test coverage

Mojibake run a total of **4,524,062** C assertions and **88** JavaScript assertions, including all
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
11. [UTS #39 data (confusables.txt and intentional.txt)](https://www.unicode.org/Public/draft/security/uts39-data-18.0.0.zip)
12. [NormalizationTest.txt](https://www.unicode.org/Public/18.0.0/ucd/NormalizationTest.txt)
13. [SpecialCasing.txt](https://www.unicode.org/Public/18.0.0/ucd/SpecialCasing.txt)

## C

| Test                                       | Coverage    |
| ------------------------------------------ | ----------- |
| `mjb_normalize_into`                       | 2824113     |
| `mjb_bidi_resolve`                         | 582573      |
| `mjb_collation_compare`                    | 424502      |
| `mjb_normalize`                            | 424423      |
| `mjb_bidi_reorder_line`                    | 91723       |
| `mjb_nfkc_casefold_into`                   | 74749       |
| `mjb_nfkc_casefold`                        | 32037       |
| `mjb_map_case_into`                        | 24756       |
| `mjb_next_line_break`                      | 18800       |
| `mjb_result_free`                          | 10686       |
| `mjb_classify_emoji_sequence`              | 5286        |
| `mjb_map_case`                             | 3492        |
| `mjb_next_word_break`                      | 1829        |
| `mjb_codepoint_emoji_properties`           | 1429        |
| `mjb_next_grapheme_break`                  | 803         |
| `mjb_codepoint_script_extensions`          | 639         |
| `mjb_next_sentence_break`                  | 459         |
| `mjb_are_confusable`                       | 241         |
| `mjb_filter`                               | 161         |
| `mjb_convert_encoding`                     | 137         |
| `mjb_locale_parse`                         | 121         |
| `mjb_count_codepoints`                     | 111         |
| `mjb_codepoint_encode`                     | 106         |
| `mjb_codepoint_info`                       | 82          |
| `mjb_filter_into`                          | 56          |
| `mjb_confusable_skeleton_into`             | 50          |
| `mjb_display_width`                        | 49          |
| `mjb_hangul_syllable_composition`          | 45          |
| `mjb_collation_key`                        | 41          |
| `mjb_codepoint_is_valid`                   | 40          |
| `mjb_codepoint_east_asian_width`           | 35          |
| `mjb_normalization_quick_check`            | 34          |
| `mjb_collation_key_into`                   | 21          |
| `mjb_codepoint_numeric_value`              | 20          |
| `mjb_confusable_skeleton`                  | 20          |
| `mjb_hangul_syllable_name`                 | 19          |
| `mjb_is_utf8`                              | 18          |
| `mjb_codepoint_is_cjk_extension_ideograph` | 17          |
| `mjb_set_locale`                           | 17          |
| `mjb_codepoint_block`                      | 16          |
| `mjb_convert_encoding_into`                | 16          |
| `mjb_is_utf16`                             | 15          |
| `mjb_is_identifier`                        | 13          |
| `mjb_bidi_line_runs`                       | 12          |
| `mjb_truncate_grapheme`                    | 12          |
| `mjb_detect_encoding`                      | 11          |
| `mjb_is_ascii`                             | 11          |
| `mjb_truncate_grapheme_width`              | 11          |
| `mjb_codepoint_script`                     | 10          |
| `mjb_plane_name`                           | 9           |
| `mjb_truncate_word`                        | 9           |
| `mjb_codepoint_is_cjk_ideograph`           | 8           |
| `mjb_reset`                                | 8           |
| `mjb_truncate_word_width`                  | 8           |
| `mjb_codepoint_is_id_start`                | 7           |
| `mjb_codepoint_property_binary`            | 7           |
| `mjb_for_each_character`                   | 7           |
| `mjb_codepoint_is_combining`               | 6           |
| `mjb_codepoint_is_graphic`                 | 6           |
| `mjb_codepoint_is_id_continue`             | 6           |
| `mjb_codepoint_property_int`               | 6           |
| `mjb_hangul_syllable_decomposition`        | 6           |
| `mjb_alloc`                                | 5           |
| `mjb_codepoint_is_hangul_jamo`             | 5           |
| `mjb_get_locale`                           | 5           |
| `mjb_set_memory_functions`                 | 5           |
| `mjb_codepoint_is_hangul_leading_jamo`     | 4           |
| `mjb_codepoint_is_hangul_trailing_jamo`    | 4           |
| `mjb_codepoint_is_hangul_vowel_jamo`       | 4           |
| `mjb_codepoint_is_pattern_syntax`          | 4           |
| `mjb_codepoint_is_pattern_white_space`     | 4           |
| `mjb_codepoint_plane`                      | 4           |
| `mjb_free`                                 | 4           |
| `mjb_is_rgi_emoji`                         | 4           |
| `mjb_plane_is_valid`                       | 4           |
| `mjb_realloc`                              | 4           |
| `mjb_bidi_paragraph_free`                  | 3           |
| `mjb_category_is_combining`                | 3           |
| `mjb_category_is_graphic`                  | 3           |
| `mjb_codepoint_is_emoji`                   | 3           |
| `mjb_codepoint_is_xid_continue`            | 3           |
| `mjb_codepoint_is_xid_start`               | 3           |
| `mjb_is_emoji_sequence`                    | 3           |
| `mjb_property_name`                        | 3           |
| `mjb_codepoint_is_emoji_component`         | 2           |
| `mjb_codepoint_is_emoji_modifier`          | 2           |
| `mjb_codepoint_is_emoji_modifier_base`     | 2           |
| `mjb_codepoint_is_emoji_presentation`      | 2           |
| `mjb_codepoint_is_extended_pictographic`   | 2           |
| `mjb_codepoint_is_hangul_syllable`         | 2           |
| `mjb_unicode_version`                      | 2           |
| `mjb_version`                              | 2           |
| `mjb_version_number`                       | 2           |
| **Total**                                  | **4524062** |

## JavaScript

| Test                                        | Coverage   |
| ------------------------------------------- | ---------- |
| `Mojibake.areConfusable`                    | 3          |
| `Mojibake.codepointPropertyBinary`          | 3          |
| `Mojibake.collationCompare`                 | 3          |
| `Mojibake.normalizationQuickCheck`          | 3          |
| `Mojibake.codepointPropertyInt`             | 2          |
| `Mojibake.filter`                           | 2          |
| `Mojibake.mapCase`                          | 2          |
| `Mojibake.setLocale`                        | 2          |
| `Mojibake.bidiResolve`                      | 1          |
| `Mojibake.categoryIsCombining`              | 1          |
| `Mojibake.categoryIsGraphic`                | 1          |
| `Mojibake.classifyEmojiSequence`            | 1          |
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
| `Mojibake.collationKey`                     | 1          |
| `Mojibake.confusableSkeleton`               | 1          |
| `Mojibake.convertEncoding`                  | 1          |
| `Mojibake.countCodepoints`                  | 1          |
| `Mojibake.create`                           | 1          |
| `Mojibake.detectEncoding`                   | 1          |
| `Mojibake.displayWidth`                     | 1          |
| `Mojibake.forEachCharacter`                 | 1          |
| `Mojibake.getLocale`                        | 1          |
| `Mojibake.hangulSyllableName`               | 1          |
| `Mojibake.isASCII`                          | 1          |
| `Mojibake.isEmojiSequence`                  | 1          |
| `Mojibake.isIdentifier`                     | 1          |
| `Mojibake.isRGIEmoji`                       | 1          |
| `Mojibake.isUTF16`                          | 1          |
| `Mojibake.isUTF8`                           | 1          |
| `Mojibake.localeParse`                      | 1          |
| `Mojibake.nextGraphemeBreak`                | 1          |
| `Mojibake.nextLineBreak`                    | 1          |
| `Mojibake.nextSentenceBreak`                | 1          |
| `Mojibake.nextWordBreak`                    | 1          |
| `Mojibake.nfkcCasefold`                     | 1          |
| `Mojibake.normalize`                        | 1          |
| `Mojibake.planeIsValid`                     | 1          |
| `Mojibake.planeName`                        | 1          |
| `Mojibake.propertyName`                     | 1          |
| `Mojibake.truncateGrapheme`                 | 1          |
| `Mojibake.truncateGraphemeWidth`            | 1          |
| `Mojibake.truncateWord`                     | 1          |
| `Mojibake.truncateWordWidth`                | 1          |
| `Mojibake.unicodeVersion`                   | 1          |
| `Mojibake.version`                          | 1          |
| `Mojibake.versionNumber`                    | 1          |
| **Total**                                   | **88**     |
