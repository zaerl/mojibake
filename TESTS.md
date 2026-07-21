# Test coverage

Mojibake run a total of **1,600,176** C assertions and **79** JavaScript assertions, including all
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

| Test                                     | Coverage    |
| ---------------------------------------- | ----------- |
| `mjb_bidi_resolve`                       | 582573      |
| `mjb_collation_compare`                  | 424486      |
| `mjb_normalize`                          | 424423      |
| `mjb_bidi_reorder_line`                  | 91723       |
| `mjb_nfkc_casefold`                      | 32037       |
| `mjb_break_line`                         | 18800       |
| `mjb_result_free`                        | 10686       |
| `mjb_string_emoji_sequence`              | 5286        |
| `mjb_case`                               | 3532        |
| `mjb_break_word`                         | 1829        |
| `mjb_codepoint_emoji`                    | 1429        |
| `mjb_break_grapheme_cluster`             | 803         |
| `mjb_codepoint_script_extensions`        | 639         |
| `mjb_break_sentence`                     | 459         |
| `mjb_string_filter`                      | 155         |
| `mjb_convert_encoding`                   | 137         |
| `mjb_locale_parse`                       | 121         |
| `mjb_string_is_confusable`               | 120         |
| `mjb_count_codepoints`                   | 111         |
| `mjb_codepoint_encode`                   | 106         |
| `mjb_codepoint_info`                     | 82          |
| `mjb_display_width`                      | 49          |
| `mjb_collation_key`                      | 45          |
| `mjb_hangul_syllable_composition`        | 45          |
| `mjb_codepoint_is_valid`                 | 40          |
| `mjb_codepoint_east_asian_width`         | 35          |
| `mjb_locale_set`                         | 21          |
| `mjb_codepoint_numeric_value`            | 20          |
| `mjb_hangul_syllable_name`               | 19          |
| `mjb_confusable_skeleton`                | 18          |
| `mjb_string_is_utf8`                     | 18          |
| `mjb_codepoint_is_cjk_ext`               | 17          |
| `mjb_codepoint_block`                    | 16          |
| `mjb_string_is_utf16`                    | 15          |
| `mjb_string_is_identifier`               | 13          |
| `mjb_bidi_line_runs`                     | 12          |
| `mjb_truncate`                           | 12          |
| `mjb_detect_encoding`                    | 11          |
| `mjb_normalization_quick_check`          | 11          |
| `mjb_string_is_ascii`                    | 11          |
| `mjb_truncate_width`                     | 11          |
| `mjb_codepoint_script`                   | 10          |
| `mjb_plane_name`                         | 9           |
| `mjb_truncate_word`                      | 9           |
| `mjb_codepoint_is_cjk_ideograph`         | 8           |
| `mjb_shutdown`                           | 8           |
| `mjb_truncate_word_width`                | 8           |
| `mjb_codepoint_is_id_start`              | 7           |
| `mjb_codepoint_property_binary`          | 7           |
| `mjb_string_each_character`              | 7           |
| `mjb_codepoint_is_combining`             | 6           |
| `mjb_codepoint_is_graphic`               | 6           |
| `mjb_codepoint_is_id_continue`           | 6           |
| `mjb_codepoint_property_int`             | 6           |
| `mjb_hangul_syllable_decomposition`      | 6           |
| `mjb_alloc`                              | 5           |
| `mjb_codepoint_is_hangul_jamo`           | 5           |
| `mjb_set_memory_functions`               | 5           |
| `mjb_codepoint_is_hangul_l`              | 4           |
| `mjb_codepoint_is_hangul_t`              | 4           |
| `mjb_codepoint_is_hangul_v`              | 4           |
| `mjb_codepoint_is_pattern_syntax`        | 4           |
| `mjb_codepoint_is_pattern_white_space`   | 4           |
| `mjb_codepoint_plane`                    | 4           |
| `mjb_free`                               | 4           |
| `mjb_plane_is_valid`                     | 4           |
| `mjb_realloc`                            | 4           |
| `mjb_string_is_rgi_emoji`                | 4           |
| `mjb_bidi_free`                          | 3           |
| `mjb_category_is_combining`              | 3           |
| `mjb_category_is_graphic`                | 3           |
| `mjb_codepoint_is_emoji`                 | 3           |
| `mjb_codepoint_is_xid_continue`          | 3           |
| `mjb_codepoint_is_xid_start`             | 3           |
| `mjb_property_name`                      | 3           |
| `mjb_string_is_emoji_sequence`           | 3           |
| `mjb_codepoint_is_emoji_component`       | 2           |
| `mjb_codepoint_is_emoji_modifier`        | 2           |
| `mjb_codepoint_is_emoji_modifier_base`   | 2           |
| `mjb_codepoint_is_emoji_presentation`    | 2           |
| `mjb_codepoint_is_extended_pictographic` | 2           |
| `mjb_codepoint_is_hangul_syllable`       | 2           |
| `mjb_unicode_version`                    | 2           |
| `mjb_version`                            | 2           |
| `mjb_version_number`                     | 2           |
| **Total**                                | **1600176** |

## JavaScript

| Test                                       | Coverage   |
| ------------------------------------------ | ---------- |
| `Mojibake.codepointPropertyBinary`         | 3          |
| `Mojibake.case`                            | 2          |
| `Mojibake.codepointPropertyInt`            | 2          |
| `Mojibake.stringFilter`                    | 2          |
| `Mojibake.bidiResolve`                     | 1          |
| `Mojibake.breakGraphemeCluster`            | 1          |
| `Mojibake.breakLine`                       | 1          |
| `Mojibake.breakSentence`                   | 1          |
| `Mojibake.breakWord`                       | 1          |
| `Mojibake.categoryIsCombining`             | 1          |
| `Mojibake.categoryIsGraphic`               | 1          |
| `Mojibake.codepointBlock`                  | 1          |
| `Mojibake.codepointEastAsianWidth`         | 1          |
| `Mojibake.codepointEmoji`                  | 1          |
| `Mojibake.codepointEncode`                 | 1          |
| `Mojibake.codepointInfo`                   | 1          |
| `Mojibake.codepointIsCjkExt`               | 1          |
| `Mojibake.codepointIsCjkIdeograph`         | 1          |
| `Mojibake.codepointIsCombining`            | 1          |
| `Mojibake.codepointIsEmoji`                | 1          |
| `Mojibake.codepointIsEmojiComponent`       | 1          |
| `Mojibake.codepointIsEmojiModifier`        | 1          |
| `Mojibake.codepointIsEmojiModifierBase`    | 1          |
| `Mojibake.codepointIsEmojiPresentation`    | 1          |
| `Mojibake.codepointIsExtendedPictographic` | 1          |
| `Mojibake.codepointIsGraphic`              | 1          |
| `Mojibake.codepointIsHangulJamo`           | 1          |
| `Mojibake.codepointIsHangulL`              | 1          |
| `Mojibake.codepointIsHangulSyllable`       | 1          |
| `Mojibake.codepointIsHangulT`              | 1          |
| `Mojibake.codepointIsHangulV`              | 1          |
| `Mojibake.codepointIsIdContinue`           | 1          |
| `Mojibake.codepointIsIdStart`              | 1          |
| `Mojibake.codepointIsPatternSyntax`        | 1          |
| `Mojibake.codepointIsPatternWhiteSpace`    | 1          |
| `Mojibake.codepointIsValid`                | 1          |
| `Mojibake.codepointIsXidContinue`          | 1          |
| `Mojibake.codepointIsXidStart`             | 1          |
| `Mojibake.codepointNumericValue`           | 1          |
| `Mojibake.codepointPlane`                  | 1          |
| `Mojibake.codepointScript`                 | 1          |
| `Mojibake.codepointScriptExtensions`       | 1          |
| `Mojibake.collationCompare`                | 1          |
| `Mojibake.collationKey`                    | 1          |
| `Mojibake.confusableSkeleton`              | 1          |
| `Mojibake.convertEncoding`                 | 1          |
| `Mojibake.countCodepoints`                 | 1          |
| `Mojibake.create`                          | 1          |
| `Mojibake.detectEncoding`                  | 1          |
| `Mojibake.displayWidth`                    | 1          |
| `Mojibake.hangulSyllableName`              | 1          |
| `Mojibake.localeParse`                     | 1          |
| `Mojibake.localeSet`                       | 1          |
| `Mojibake.nfkcCasefold`                    | 1          |
| `Mojibake.normalizationQuickCheck`         | 1          |
| `Mojibake.normalize`                       | 1          |
| `Mojibake.planeIsValid`                    | 1          |
| `Mojibake.planeName`                       | 1          |
| `Mojibake.propertyName`                    | 1          |
| `Mojibake.stringEmojiSequence`             | 1          |
| `Mojibake.stringIsAscii`                   | 1          |
| `Mojibake.stringIsConfusable`              | 1          |
| `Mojibake.stringIsEmojiSequence`           | 1          |
| `Mojibake.stringIsIdentifier`              | 1          |
| `Mojibake.stringIsRgiEmoji`                | 1          |
| `Mojibake.stringIsUtf16`                   | 1          |
| `Mojibake.stringIsUtf8`                    | 1          |
| `Mojibake.truncate`                        | 1          |
| `Mojibake.truncateWidth`                   | 1          |
| `Mojibake.truncateWord`                    | 1          |
| `Mojibake.truncateWordWidth`               | 1          |
| `Mojibake.unicodeVersion`                  | 1          |
| `Mojibake.version`                         | 1          |
| `Mojibake.versionNumber`                   | 1          |
| `Mojibake.stringEachCharacter`             | 0          |
| **Total**                                  | **79**     |
