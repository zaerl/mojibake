/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import {
  Block,
  BreakType,
  CaseType,
  Category,
  Direction,
  EastAsianWidth,
  EmojiQualification,
  EmojiSequenceType,
  Encoding,
  FilterType,
  Locale,
  Mojibake,
  Plane,
  Property,
  QuickCheckResult,
  Script,
  WidthContext
} from '../index.js';
import {
  ATT_ASSERT,
  att_get_total_tests,
  att_get_valid_tests,
  att_set_show_colors,
  att_set_verbose
} from './attractor.js';

const mojibake = await Mojibake.create({
  locateFile: (path, prefix) => `${prefix}${path}`
});

let showColors = true;
let verbosity = 0;

att_set_verbose(verbosity);
att_set_show_colors(showColors);

ATT_ASSERT(mojibake instanceof Mojibake, true, 'create');
ATT_ASSERT(mojibake.codepointCharacter(0x41)?.name, 'LATIN CAPITAL LETTER A', 'codepointCharacter');
ATT_ASSERT(mojibake.normalize('e\u0301')?.output, '\u00E9', 'normalize');
ATT_ASSERT(mojibake.nextCharacter('A')?.[0]?.character.codepoint, 0x41, 'nextCharacter');
ATT_ASSERT(mojibake.stringIsNormalized('abc'), QuickCheckResult.YES, 'stringIsNormalized');
ATT_ASSERT(mojibake.stringFilter('hello    world', FilterType.COLLAPSE_SPACES)?.output, 'hello world',
  'stringFilter');
ATT_ASSERT(mojibake.stringFilter('a\u0300\u0301\u0302\u0303\u0304', FilterType.LIMIT_COMBINING)?.output,
  'a\u0300\u0301\u0302\u0303', 'stringFilterLimitCombining');
ATT_ASSERT(mojibake.codepointPropertyBinary(0x41, Property.ALPHABETIC), true,
  'codepointPropertyBinary true');
ATT_ASSERT(mojibake.codepointPropertyBinary(0x20, Property.ALPHABETIC), false,
  'codepointPropertyBinary false');
ATT_ASSERT(mojibake.codepointPropertyBinary(0x41, Property.SCRIPT), null,
  'codepointPropertyBinary type mismatch');
ATT_ASSERT(mojibake.codepointPropertyInt(0x41, Property.SCRIPT), Script.LATN,
  'codepointPropertyInt');
ATT_ASSERT(mojibake.codepointPropertyInt(0x41, Property.ALPHABETIC), null,
  'codepointPropertyInt type mismatch');
ATT_ASSERT(mojibake.codepointScriptExtensions(0x30FC), [Script.HIRA, Script.KANA],
  'codepointScriptExtensions');
ATT_ASSERT(mojibake.codepointScript(0x41), Script.LATN, 'codepointScript');
ATT_ASSERT(mojibake.stringEncoding('A'), Encoding.ASCII | Encoding.UTF_8, 'stringEncoding');
ATT_ASSERT(mojibake.stringIsUtf8('Hello'), true, 'stringIsUtf8');
ATT_ASSERT(mojibake.stringIsUtf16(new Uint8Array([0x00, 0x48, 0x00, 0x69])), true, 'stringIsUtf16');
ATT_ASSERT(mojibake.stringIsAscii('Hello'), true, 'stringIsAscii');
ATT_ASSERT(mojibake.codepointEncode(0x41)?.output, 'A', 'codepointEncode');
ATT_ASSERT(mojibake.stringConvertEncoding('A', Encoding.UTF_16LE)?.output, 'A', 'stringConvertEncoding');
ATT_ASSERT(mojibake.stringLength('H\u00E9ll\u00F6'), 5, 'stringLength');
ATT_ASSERT(mojibake.stringCompare('hello', 'hello'), 0, 'stringCompare');
ATT_ASSERT((mojibake.collationKey('a')?.length ?? 0) > 0, true, 'collationKey');
ATT_ASSERT(mojibake.case('hello', CaseType.UPPER)?.output, 'HELLO', 'case');
ATT_ASSERT(mojibake.case('\u13A0', CaseType.CASEFOLD)?.output, '\u13A0',
  'casefold uppercase Cherokee');
ATT_ASSERT(mojibake.codepointIsValid(0x41), true, 'codepointIsValid');
ATT_ASSERT(mojibake.codepointIsGraphic(0x23), true, 'codepointIsGraphic');
ATT_ASSERT(mojibake.codepointIsCombining(0x0300), true, 'codepointIsCombining');
ATT_ASSERT(mojibake.codepointIsHangulSyllable(0xAC00), true, 'codepointIsHangulSyllable');
ATT_ASSERT(mojibake.codepointIsCjkIdeograph(0x4E00), true, 'codepointIsCjkIdeograph');
ATT_ASSERT(mojibake.codepointIsCjkExt(0x3400), true, 'codepointIsCjkExt');
ATT_ASSERT(mojibake.categoryIsGraphic(Category.LU), true, 'categoryIsGraphic');
ATT_ASSERT(mojibake.categoryIsCombining(Category.MN), true, 'categoryIsCombining');
ATT_ASSERT(mojibake.codepointNumericValue(0x31), { decimal: 1, digit: 1, numeric: '1' },
  'codepointNumericValue');
ATT_ASSERT(mojibake.codepointBlock(0x41)?.id, Block.BASIC_LATIN, 'codepointBlock');
ATT_ASSERT(mojibake.codepointToLowercase(0x41), 0x61, 'codepointToLowercase');
ATT_ASSERT(mojibake.codepointToUppercase(0x62), 0x42, 'codepointToUppercase');
ATT_ASSERT(mojibake.codepointToTitlecase(0x63), 0x43, 'codepointToTitlecase');
ATT_ASSERT(mojibake.nfkcCasefold('Straße\u00AD')?.output, 'strasse', 'nfkcCasefold');
ATT_ASSERT(mojibake.breakLine('A'), [BreakType.ALLOWED], 'breakLine');
ATT_ASSERT(mojibake.breakWord('A'), [BreakType.ALLOWED], 'breakWord');
ATT_ASSERT(mojibake.truncateWord('Hello World', 1), 5, 'truncateWord');
ATT_ASSERT(mojibake.truncateWordWidth('Hello World', WidthContext.WESTERN, 5), 5,
  'truncateWordWidth');
ATT_ASSERT(mojibake.breakSentence('A'), [BreakType.ALLOWED], 'breakSentence');
ATT_ASSERT(mojibake.breakGraphemeCluster('A'), [BreakType.ALLOWED], 'breakGraphemeCluster');
ATT_ASSERT(mojibake.truncate('ABC', 2), 2, 'truncate');
ATT_ASSERT(mojibake.truncateWidth('ABC', WidthContext.WESTERN, 2), 2, 'truncateWidth');
ATT_ASSERT(mojibake.bidiResolve('ABC', Direction.AUTO)?.direction, Direction.LTR, 'bidiResolve');
ATT_ASSERT(mojibake.codepointPlane(0xFFFD), Plane.BMP, 'codepointPlane');
ATT_ASSERT(mojibake.planeIsValid(Plane.SMP), true, 'planeIsValid');
ATT_ASSERT(mojibake.planeName(Plane.BMP, true), 'BMP', 'planeName');
ATT_ASSERT(mojibake.codepointIsIdStart(0x41), true, 'codepointIsIdStart');
ATT_ASSERT(mojibake.codepointIsIdContinue(0x30), true, 'codepointIsIdContinue');
ATT_ASSERT(mojibake.codepointIsXidStart(0x41), true, 'codepointIsXidStart');
ATT_ASSERT(mojibake.codepointIsXidContinue(0x30), true, 'codepointIsXidContinue');
ATT_ASSERT(mojibake.codepointIsPatternSyntax(0x21), true, 'codepointIsPatternSyntax');
ATT_ASSERT(mojibake.codepointIsPatternWhiteSpace(0x20), true, 'codepointIsPatternWhiteSpace');
ATT_ASSERT(mojibake.stringIsIdentifier('hello'), true, 'stringIsIdentifier');
ATT_ASSERT(mojibake.propertyName(Property.CASED), 'Cased', 'propertyName');
ATT_ASSERT(mojibake.stringIsConfusable('\u0410', 'A'), true, 'stringIsConfusable');
ATT_ASSERT(mojibake.confusableSkeleton('h\u0435llo')?.output, 'hello',
  'confusableSkeleton');
ATT_ASSERT(mojibake.codepointEmoji(0x23)?.component, true, 'codepointEmoji');
ATT_ASSERT(mojibake.codepointIsEmoji(0x23), true, 'codepointIsEmoji');
ATT_ASSERT(mojibake.codepointIsEmojiPresentation(0x23), false, 'codepointIsEmojiPresentation');
ATT_ASSERT(mojibake.codepointIsEmojiModifier(0x1F3FB), true, 'codepointIsEmojiModifier');
ATT_ASSERT(mojibake.codepointIsEmojiModifierBase(0x1F44B), true, 'codepointIsEmojiModifierBase');
ATT_ASSERT(mojibake.codepointIsEmojiComponent(0x23), true, 'codepointIsEmojiComponent');
ATT_ASSERT(mojibake.codepointIsExtendedPictographic(0x1F600), true,
  'codepointIsExtendedPictographic');
ATT_ASSERT(mojibake.stringEmojiSequence('\u263A\uFE0F'), {
  type: EmojiSequenceType.BASIC,
  qualification: EmojiQualification.FULLY_QUALIFIED,
  codepoint_count: 2
}, 'stringEmojiSequence');
ATT_ASSERT(mojibake.stringIsEmojiSequence('\u263A'), true, 'stringIsEmojiSequence');
ATT_ASSERT(mojibake.stringIsRgiEmoji('\u263A\uFE0F'), true, 'stringIsRgiEmoji');
ATT_ASSERT(mojibake.codepointEastAsianWidth(0x20), EastAsianWidth.NARROW,
  'codepointEastAsianWidth');
ATT_ASSERT(mojibake.displayWidth('Hello'), 5, 'displayWidth');
ATT_ASSERT(mojibake.localeParse('sr-Latn-RS').region, 'RS', 'localeParse');
ATT_ASSERT(mojibake.localeSet(Locale.EN), true, 'localeSet');
ATT_ASSERT(mojibake.version(), '0.2.6-WASM', 'version');
ATT_ASSERT(mojibake.versionNumber(), 0x26, 'versionNumber');
ATT_ASSERT(mojibake.unicodeVersion(), '17.0.0', 'unicodeVersion');

const valid = att_get_valid_tests();
const total = att_get_total_tests();
const isValid = valid === total;

const colorCode = showColors ? (isValid ? "\x1B[32m" : "\x1B[31m") : "";

console.log(
  `${verbosity >= 1 ? "\n" : ""}Tests valid/run: ${colorCode}${valid}/${total}` +
    `${showColors ? "\x1B[0m" : ""}`
);
