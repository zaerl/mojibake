/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import {
  Block,
  BreakType,
  CaselessMode,
  CaseType,
  Category,
  CollationStrength,
  CollationVariableWeighting,
  Direction,
  EastAsianWidth,
  EmojiQualification,
  EmojiSequenceType,
  Encoding,
  FilterFlags,
  IdnaError,
  Locale,
  Mojibake,
  Normalization,
  Plane,
  Property,
  QuickCheckResult,
  Script,
  ScriptSetKind,
  Status,
  TerminalWidthProfile
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
ATT_ASSERT(mojibake.codepointInfo(0x41)?.name, 'LATIN CAPITAL LETTER A', 'codepointCharacter');
ATT_ASSERT(mojibake.normalize('e\u0301')?.output, '\u00E9', 'normalize');
ATT_ASSERT(mojibake.idnaToAscii('bücher.example')?.output, 'xn--bcher-kva.example', 'idnaToAscii');
ATT_ASSERT(mojibake.idnaToUnicode('xn--bcher-kva.example')?.output, 'bücher.example', 'idnaToUnicode');
ATT_ASSERT(mojibake.idnaToAscii('a..b')?.valid, false, 'idnaToAscii validation');
ATT_ASSERT((mojibake.idnaToAscii('a..b')?.errors ?? 0) & IdnaError.EMPTY_LABEL,
  IdnaError.EMPTY_LABEL, 'idnaToAscii error flags');
ATT_ASSERT(mojibake.forEachCodepoint('A')?.[0]?.character.codepoint, 0x41, 'forEachCodepoint');
ATT_ASSERT(mojibake.normalizationQuickCheck('abc'), QuickCheckResult.YES, 'normalizationQuickCheck');
ATT_ASSERT(mojibake.normalizationQuickCheck('\u00E9', Normalization.NFD), QuickCheckResult.NO,
  'normalizationQuickCheck negative result');
ATT_ASSERT(mojibake.normalizationQuickCheck(new Uint8Array([0x80])), null,
  'normalizationQuickCheck malformed input');
ATT_ASSERT(mojibake.filter('hello    world',
  FilterFlags.COLLAPSE_SPACES | FilterFlags.CONTROLS)?.output, 'hello world', 'filter');
ATT_ASSERT(mojibake.filter('a\u0300\u0301\u0302\u0303\u0304', FilterFlags.LIMIT_COMBINING)?.output,
  'a\u0300\u0301\u0302\u0303', 'filter LIMIT_COMBINING');
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
ATT_ASSERT(mojibake.resolvedScriptSet('hello123!'),
  { kind: ScriptSetKind.RESOLVED, scripts: [Script.LATN] }, 'resolvedScriptSet Latin');
ATT_ASSERT(mojibake.resolvedScriptSet('h\u0435llo'),
  { kind: ScriptSetKind.EMPTY, scripts: [] }, 'resolvedScriptSet mixed');
ATT_ASSERT(mojibake.resolvedScriptSet('123!'),
  { kind: ScriptSetKind.ALL, scripts: [] }, 'resolvedScriptSet ALL');
ATT_ASSERT(mojibake.resolvedScriptSet('\u306D\u30AC'),
  { kind: ScriptSetKind.RESOLVED, scripts: [Script.JPAN] }, 'resolvedScriptSet Japanese');
ATT_ASSERT(mojibake.resolvedScriptSet(new Uint8Array([0x80])), null,
  'resolvedScriptSet malformed');
ATT_ASSERT(mojibake.detectEncoding('A'), Encoding.ASCII | Encoding.UTF_8, 'detectEncoding');
ATT_ASSERT(mojibake.isUTF8('Hello'), true, 'isUTF8');
ATT_ASSERT(mojibake.isUTF16(new Uint8Array([0x00, 0x48, 0x00, 0x69])), true, 'isUTF16');
ATT_ASSERT(mojibake.isASCII('Hello'), true, 'isASCII');
ATT_ASSERT(mojibake.codepointEncode(0x41)?.output, 'A', 'codepointEncode');
ATT_ASSERT(mojibake.convertEncoding('A', Encoding.UTF_16LE)?.output, 'A', 'convertEncoding');
ATT_ASSERT(mojibake.codepointCount('H\u00E9ll\u00F6'), 5, 'codepointCount');
ATT_ASSERT(mojibake.codepointCount(''), 0, 'codepointCount empty');
ATT_ASSERT(mojibake.caselessMatch('Straße', 'STRASSE'), true, 'caselessMatch');
ATT_ASSERT(mojibake.caselessMatch('\u00C5', 'A\u030A', CaselessMode.UNNORMALIZED), false,
  'caselessMatch unnormalized');
ATT_ASSERT(mojibake.caselessMatch('\u00C5', 'A\u030A', CaselessMode.CANONICAL), true,
  'caselessMatch canonical');
ATT_ASSERT(mojibake.caselessMatch('ab\u00AD', 'ab', CaselessMode.IDENTIFIER), true,
  'caselessMatch identifier');
ATT_ASSERT(mojibake.caselessMatch(new Uint8Array([0x80]), 'a'), null,
  'caselessMatch malformed input');
ATT_ASSERT(mojibake.collationCompare('hello', 'hello'), 0, 'collationCompare');
ATT_ASSERT((mojibake.collationCompare('a', 'b') ?? 0) < 0, true,
  'collationCompare negative order');
ATT_ASSERT(mojibake.collationCompare('A', 'a', CollationVariableWeighting.NON_IGNORABLE,
  CollationStrength.SECONDARY), 0, 'collationCompare secondary ignores case');
ATT_ASSERT((mojibake.collationCompare('A', 'a', CollationVariableWeighting.NON_IGNORABLE,
  CollationStrength.TERTIARY) ?? 0) !== 0, true, 'collationCompare tertiary compares case');
ATT_ASSERT(mojibake.collationCompare('a', '\u00E1', CollationVariableWeighting.NON_IGNORABLE,
  CollationStrength.PRIMARY), 0, 'collationCompare primary ignores accents');
ATT_ASSERT((mojibake.collationCompare('a', '\u00E1', CollationVariableWeighting.NON_IGNORABLE,
  CollationStrength.SECONDARY) ?? 0) !== 0, true, 'collationCompare secondary compares accents');
ATT_ASSERT(mojibake.collationCompare('ab', 'a-b', CollationVariableWeighting.SHIFTED,
  CollationStrength.TERTIARY), 0, 'collationCompare shifted tertiary ignores punctuation');
ATT_ASSERT((mojibake.collationCompare('ab', 'a-b', CollationVariableWeighting.SHIFTED,
  CollationStrength.QUATERNARY) ?? 0) !== 0, true,
  'collationCompare shifted quaternary compares punctuation');
ATT_ASSERT(mojibake.collationCompare('', '\u200B'), 0,
  'collationCompare completely ignorable equals empty');
ATT_ASSERT(mojibake.collationCompare('\u200B', ''), 0,
  'collationCompare completely ignorable equals empty in reverse order');
ATT_ASSERT(mojibake.collationCompare('', '\u0301',
  CollationVariableWeighting.NON_IGNORABLE, CollationStrength.PRIMARY), 0,
  'collationCompare primary-ignorable accent equals empty');
ATT_ASSERT((mojibake.collationCompare('', '\u0301',
  CollationVariableWeighting.NON_IGNORABLE, CollationStrength.SECONDARY) ?? 0) !== 0, true,
  'collationCompare secondary accent differs from empty');
ATT_ASSERT(mojibake.collationCompare('', '-', CollationVariableWeighting.SHIFTED,
  CollationStrength.TERTIARY), 0,
  'collationCompare shifted punctuation equals empty below quaternary');
ATT_ASSERT((mojibake.collationCompare('', '-', CollationVariableWeighting.SHIFTED,
  CollationStrength.QUATERNARY) ?? 0) !== 0, true,
  'collationCompare shifted punctuation differs from empty at quaternary');
ATT_ASSERT(mojibake.collationCompare(new Uint8Array([0x80]), 'a'), null,
  'collationCompare malformed input');
ATT_ASSERT((mojibake.collationKey('a')?.length ?? 0) > 0, true, 'collationKey');
ATT_ASSERT(mojibake.collationKey('\u200B'), mojibake.collationKey(''),
  'collationKey completely ignorable equals empty');
ATT_ASSERT(mojibake.collationKey('\u0301', CollationVariableWeighting.NON_IGNORABLE,
  CollationStrength.PRIMARY), mojibake.collationKey('',
  CollationVariableWeighting.NON_IGNORABLE, CollationStrength.PRIMARY),
  'collationKey primary-ignorable accent equals empty');
ATT_ASSERT(mojibake.collationKey('A', CollationVariableWeighting.NON_IGNORABLE,
  CollationStrength.SECONDARY), mojibake.collationKey('a',
  CollationVariableWeighting.NON_IGNORABLE, CollationStrength.SECONDARY),
  'collationKey secondary ignores case');
ATT_ASSERT(mojibake.mapCase('hello', CaseType.UPPER)?.output, 'HELLO', 'mapCase');
ATT_ASSERT(mojibake.mapCase('\u13A0', CaseType.CASEFOLD)?.output, '\u13A0',
  'mapCase casefold uppercase Cherokee');
ATT_ASSERT(mojibake.codepointIsValid(0x41), true, 'codepointIsValid');
ATT_ASSERT(mojibake.codepointIsGraphic(0x23), true, 'codepointIsGraphic');
ATT_ASSERT(mojibake.codepointIsCombining(0x0300), true, 'codepointIsCombining');
ATT_ASSERT(mojibake.codepointIsHangulLeadingJamo(0x1100), true, 'codepointIsHangulLeadingJamo');
ATT_ASSERT(mojibake.codepointIsHangulVowelJamo(0x1161), true, 'codepointIsHangulVowelJamo');
ATT_ASSERT(mojibake.codepointIsHangulTrailingJamo(0x11A8), true, 'codepointIsHangulTrailingJamo');
ATT_ASSERT(mojibake.codepointIsHangulJamo(0x1100), true, 'codepointIsHangulJamo');
ATT_ASSERT(mojibake.codepointIsHangulSyllable(0xAC00), true, 'codepointIsHangulSyllable');
ATT_ASSERT(mojibake.codepointIsCjkIdeograph(0x4E00), true, 'codepointIsCjkIdeograph');
ATT_ASSERT(mojibake.codepointIsCJKExtensionIdeograph(0x3400), true,
'codepointIsCJKExtensionIdeograph');
ATT_ASSERT(mojibake.categoryIsGraphic(Category.LU), true, 'categoryIsGraphic');
ATT_ASSERT(mojibake.categoryIsCombining(Category.MN), true, 'categoryIsCombining');
ATT_ASSERT(mojibake.codepointNumericValue(0x31), { decimal: 1, digit: 1, numeric: '1' },
  'codepointNumericValue');
ATT_ASSERT(mojibake.codepointBlock(0x41)?.id, Block.BASIC_LATIN, 'codepointBlock');
ATT_ASSERT(mojibake.nfkcCasefold('Straße\u00AD')?.output, 'strasse', 'nfkcCasefold');
ATT_ASSERT(mojibake.nextLineBreak('A'), [BreakType.ALLOWED], 'nextLineBreak');
ATT_ASSERT(mojibake.nextWordBreak('A'), [BreakType.ALLOWED], 'nextWordBreak');
ATT_ASSERT(mojibake.nextSentenceBreak('A'), [BreakType.ALLOWED], 'nextSentenceBreak');
ATT_ASSERT(mojibake.nextGraphemeBreak('A'), [BreakType.ALLOWED], 'nextGraphemeBreak');
ATT_ASSERT(mojibake.truncateWord('Hello World', 1), 5, 'truncateWord');
ATT_ASSERT(mojibake.truncateWordWidth('Hello World', TerminalWidthProfile.NARROW, 5), 5,
  'truncateWordWidth');
ATT_ASSERT(mojibake.truncateGrapheme('ABC', 2), 2, 'truncateGrapheme');
ATT_ASSERT(mojibake.graphemeCount('ABC'), 3, 'graphemeCount');
ATT_ASSERT(mojibake.graphemeCount(''), 0, 'graphemeCount empty');
ATT_ASSERT(mojibake.graphemeCount('🇮🇹'), 1, 'graphemeCount flag emoji');
ATT_ASSERT(mojibake.graphemeCount('👨‍👩‍👦'), 1, 'graphemeCount ZWJ sequence');
ATT_ASSERT(mojibake.sentenceCount('Hello. How are you? Fine!'), 3, 'sentenceCount');
ATT_ASSERT(mojibake.sentenceCount(''), 0, 'sentenceCount empty');
ATT_ASSERT(mojibake.wordCount('Hello, world! It works.'), 4, 'wordCount');
ATT_ASSERT(mojibake.wordCount('state-of-the-art'), 4, 'wordCount hyphenated');
ATT_ASSERT(mojibake.wordCount('...'), 0, 'wordCount punctuation only');
ATT_ASSERT(mojibake.wordCount(''), 0, 'wordCount empty');
ATT_ASSERT(mojibake.truncateGraphemeWidth('ABC', TerminalWidthProfile.NARROW, 2), 2,
  'truncateGraphemeWidth');
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
ATT_ASSERT(mojibake.isIdentifier('hello'), true, 'isIdentifier');
ATT_ASSERT(mojibake.propertyName(Property.CASED), 'Cased', 'propertyName');
ATT_ASSERT(mojibake.confusableMatch('\u0410', 'A'), true, 'areConfusable');
ATT_ASSERT(mojibake.confusableMatch('a', 'b'), false, 'areConfusable false result');
ATT_ASSERT(mojibake.confusableMatch(new Uint8Array([0x80]), 'A'), null,
  'areConfusable malformed input');
ATT_ASSERT(mojibake.confusableSkeleton('h\u0435llo')?.output, 'hello',
  'confusableSkeleton');
ATT_ASSERT(mojibake.codepointEmojiProperties(0x23)?.component, true, 'codepointEmojiProperties');
ATT_ASSERT(mojibake.codepointIsEmoji(0x23), true, 'codepointIsEmoji');
ATT_ASSERT(mojibake.codepointIsEmojiPresentation(0x23), false, 'codepointIsEmojiPresentation');
ATT_ASSERT(mojibake.codepointIsEmojiModifier(0x1F3FB), true, 'codepointIsEmojiModifier');
ATT_ASSERT(mojibake.codepointIsEmojiModifierBase(0x1F44B), true, 'codepointIsEmojiModifierBase');
ATT_ASSERT(mojibake.codepointIsEmojiComponent(0x23), true, 'codepointIsEmojiComponent');
ATT_ASSERT(mojibake.codepointIsExtendedPictographic(0x1F600), true,
  'codepointIsExtendedPictographic');
ATT_ASSERT(mojibake.classifyEmojiSequence('\u263A\uFE0F'), {
  type: EmojiSequenceType.BASIC,
  qualification: EmojiQualification.FULLY_QUALIFIED,
  codepoint_count: 2
}, 'classifyEmojiSequence');
ATT_ASSERT(mojibake.isEmojiSequence('\u263A'), true, 'isEmojiSequence');
ATT_ASSERT(mojibake.isRGIEmoji('\u263A\uFE0F'), true, 'stringIsRgiEmoji');
ATT_ASSERT(mojibake.hangulSyllableName(0xAC01), 'HANGUL SYLLABLE GAG', 'hangulSyllableName');
ATT_ASSERT(mojibake.codepointEastAsianWidth(0x20), EastAsianWidth.NARROW,
  'codepointEastAsianWidth');
ATT_ASSERT(mojibake.terminalWidth('Hello'), 5, 'terminalWidth');
ATT_ASSERT(mojibake.terminalWidth('👨🏻‍❤️‍💋‍👨🏻'), 2, 'terminalWidth emoji sequence');
ATT_ASSERT(mojibake.terminalWidth('line\nbreak'), null, 'terminalWidth rejects controls');
ATT_ASSERT(mojibake.localeParse('sr-Latn-RS').region, 'RS', 'localeParse');
ATT_ASSERT(mojibake.setLocale(Locale.IT), true, 'setLocale');
ATT_ASSERT(mojibake.getLocale(), Locale.IT, 'getLocale');
ATT_ASSERT(mojibake.setLocale(Locale.EN), true, 'restore locale');
ATT_ASSERT(mojibake.version(), '0.3.6-WASM', 'version');
ATT_ASSERT(mojibake.versionNumber(), 0x36, 'versionNumber');
ATT_ASSERT(mojibake.unicodeVersion(), '18.0.0', 'unicodeVersion');
ATT_ASSERT(mojibake.statusMessage(Status.OK), 'The operation completed successfully',
  'statusMessage');

const valid = att_get_valid_tests();
const total = att_get_total_tests();
const isValid = valid === total;

const colorCode = showColors ? (isValid ? "\x1B[32m" : "\x1B[31m") : "";

console.log(
  `${verbosity >= 1 ? "\n" : ""}Tests valid/run: ${colorCode}${valid}/${total}` +
    `${showColors ? "\x1B[0m" : ""}`
);

if(!isValid) {
  process.exitCode = 1;
}
