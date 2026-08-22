//
//  The Mojibake library
//
//  This file is distributed under the MIT License. See LICENSE for details.
//

import Foundation

struct DetailRow {
    let label: String
    let value: String
    let monospaced: Bool

    init(_ label: String, _ value: String, monospaced: Bool = false) {
        self.label = label
        self.value = value
        self.monospaced = monospaced
    }
}

struct DetailSection: Identifiable {
    let id: String
    let title: String
    let rows: [DetailRow]

    init(_ title: String, rows: [DetailRow]) {
        id = title
        self.title = title
        self.rows = rows
    }
}

struct CharacterDetails {
    let character: String
    let codepoint: String
    let name: String
    let sections: [DetailSection]
    let unicodeProperties: [DetailRow]

    init(
        character: String,
        codepoint: String,
        name: String,
        sections: [DetailSection],
        unicodeProperties: [DetailRow]
    ) {
        self.character = character
        self.codepoint = codepoint
        self.name = name
        self.sections = sections
        self.unicodeProperties = unicodeProperties
    }

    init(character: mjb_character) {
        self = CharacterDetailsBuilder().details(for: character)
    }

    static func encodedCharacter(_ codepoint: mjb_codepoint) -> String? {
        var buffer = [CChar](repeating: 0, count: 5)
        let length = buffer.withUnsafeMutableBufferPointer {
            mjb_codepoint_encode(codepoint, $0.baseAddress, $0.count, MJB_ENC_UTF_8)
        }

        guard length > 0 else {
            return nil
        }

        let bytes = buffer.prefix(Int(length)).map { UInt8(bitPattern: $0) }
        return String(decoding: bytes, as: UTF8.self)
    }

    static func displayCharacter(
        for character: mjb_character,
        encodedCharacter: String
    ) -> String {
        let codepoint = character.codepoint
        let pictureCodepoint = controlPictureCodepoint(codepoint)

        if pictureCodepoint != codepoint {
            return Self.encodedCharacter(pictureCodepoint)
                ?? MojibakeFormatting.codepoint(codepoint)
        }

        if codepoint == 0x20 {
            return encodedCharacter
        }

        let category = character.category
        let isSeparator = category == MJB_CATEGORY_ZS
            || category == MJB_CATEGORY_ZL
            || category == MJB_CATEGORY_ZP
        let isDefaultIgnorable = hasBinaryProperty(
            MJB_PR_DEFAULT_IGNORABLE_CODE_POINT,
            for: codepoint
        )

        if isSeparator || isDefaultIgnorable || !mjb_codepoint_is_graphic(codepoint) {
            return MojibakeFormatting.codepoint(codepoint)
        }

        // Show combining characters over a U+25CC DOTTED CIRCLE.
        if mjb_codepoint_is_combining(codepoint) {
            let dottedCircle = Self.encodedCharacter(0x25CC) ?? ""
            return dottedCircle + encodedCharacter
        }

        return encodedCharacter
    }

    private static func hasBinaryProperty(
        _ property: mjb_property,
        for codepoint: mjb_codepoint
    ) -> Bool {
        var value = false
        return mjb_codepoint_property_binary(codepoint, property, &value) == MJB_STATUS_OK
            && value
    }

    private static func controlPictureCodepoint(
        _ codepoint: mjb_codepoint
    ) -> mjb_codepoint {
        // Keep this display mapping consistent with mjbsh_control_picture_codepoint.
        if codepoint < 0x20 {
            return codepoint + 0x2400
        }

        if codepoint == 0x7F {
            return 0x2421
        }

        return codepoint
    }
}

private struct CharacterDetailsBuilder {
    func details(for character: mjb_character) -> CharacterDetails {
        let codepoint = character.codepoint
        let encodedCharacter = CharacterDetails.encodedCharacter(codepoint) ?? "N/A"
        let displayedCharacter = CharacterDetails.displayCharacter(
            for: character,
            encodedCharacter: encodedCharacter
        )

        return CharacterDetails(
            character: displayedCharacter,
            codepoint: codepointString(codepoint),
            name: MojibakeFormatting.string(fromCString: character.name),
            sections: [
                DetailSection("Overview", rows: overviewRows(for: character)),
                DetailSection("Encodings", rows: encodingRows(for: codepoint)),
                DetailSection(
                    "Normalization",
                    rows: normalizationRows(for: encodedCharacter)
                ),
                DetailSection(
                    "Character Properties",
                    rows: characterPropertyRows(for: character)
                ),
                DetailSection(
                    "Numeric and Case",
                    rows: numericAndCaseRows(for: character)
                ),
                DetailSection("Emoji", rows: emojiRows(for: codepoint)),
            ],
            unicodeProperties: unicodePropertyRows(for: codepoint)
        )
    }

    private func overviewRows(for character: mjb_character) -> [DetailRow] {
        let codepoint = character.codepoint
        let category = Int(character.category.rawValue)
        let plane = mjb_codepoint_plane(codepoint)
        let planeID = Int(plane.rawValue)
        let planeName = mjb_plane_name(plane, false).map(String.init(cString:)) ?? "Unknown"
        let script = mjb_codepoint_script(codepoint)
        var rows = [
            DetailRow(
                "Category",
                MojibakeFormatting.idAndName(category, categoryName(category))
            ),
            DetailRow("Plane", MojibakeFormatting.idAndName(planeID, planeName)),
            DetailRow("Script", String(script.rawValue)),
        ]

        var block = mjb_block_info()
        if mjb_codepoint_block(codepoint, &block) == MJB_STATUS_OK {
            let blockID = Int(block.id.rawValue)
            rows.append(
                DetailRow(
                    "Block",
                    MojibakeFormatting.idAndName(
                        blockID,
                        MojibakeFormatting.string(fromCString: block.name)
                    )
                )
            )
        }

        return rows
    }

    private func encodingRows(for codepoint: mjb_codepoint) -> [DetailRow] {
        let encodings: [(String, mjb_encoding)] = [
            ("Hex UTF-8", MJB_ENC_UTF_8),
            ("Hex UTF-16LE", MJB_ENC_UTF_16LE),
            ("Hex UTF-16BE", MJB_ENC_UTF_16BE),
            ("Hex UTF-32LE", MJB_ENC_UTF_32LE),
            ("Hex UTF-32BE", MJB_ENC_UTF_32BE),
        ]

        return encodings.map { label, encoding in
            DetailRow(label, hexEncoding(codepoint, as: encoding), monospaced: true)
        }
    }

    private func normalizationRows(for character: String) -> [DetailRow] {
        let normalizations: [(String, mjb_normalization)] = [
            ("NFD", MJB_NORMALIZATION_NFD),
            ("NFC", MJB_NORMALIZATION_NFC),
            ("NFKD", MJB_NORMALIZATION_NFKD),
            ("NFKC", MJB_NORMALIZATION_NFKC),
        ]
        var rows: [DetailRow] = []

        for (label, form) in normalizations {
            guard let normalized = normalized(character, form: form) else {
                rows.append(DetailRow(label, "N/A"))
                rows.append(DetailRow("\(label) normalization", "N/A"))
                continue
            }

            let normalizationCodepoints = normalized.unicodeScalars
                .map { codepointString($0.value) }
                .joined(separator: " ")
            rows.append(DetailRow(label, normalized))
            rows.append(
                DetailRow(
                    "\(label) normalization",
                    normalizationCodepoints,
                    monospaced: true
                )
            )
        }

        return rows
    }

    private func characterPropertyRows(for character: mjb_character) -> [DetailRow] {
        let codepoint = character.codepoint
        let combining = Int(character.combining.rawValue)
        let bidirectional = Int(character.bidirectional)
        let decomposition = Int(character.decomposition.rawValue)
        var eastAsianWidth = MJB_EAW_NOT_SET
        var rows = [
            DetailRow(
                "Combining",
                MojibakeFormatting.idAndName(combining, combiningName(combining))
            ),
            DetailRow(
                "Bidirectional",
                MojibakeFormatting.idAndName(
                    bidirectional,
                    bidirectionalName(bidirectional)
                )
            ),
            DetailRow(
                "Decomposition",
                MojibakeFormatting.idAndName(
                    decomposition,
                    decompositionName(decomposition)
                )
            ),
            DetailRow("Mirrored", MojibakeFormatting.yesOrNo(character.mirrored)),
        ]

        if mjb_codepoint_east_asian_width(codepoint, &eastAsianWidth) == MJB_STATUS_OK {
            let width = Int(eastAsianWidth.rawValue)
            rows.append(
                DetailRow(
                    "East Asian Width",
                    MojibakeFormatting.idAndName(width, eastAsianWidthName(width))
                )
            )
        } else {
            rows.append(DetailRow("East Asian Width", "N/A"))
        }

        return rows
    }

    private func numericAndCaseRows(for character: mjb_character) -> [DetailRow] {
        let numeric = MojibakeFormatting.string(fromCString: character.numeric)

        return [
            DetailRow("Decimal", numberOrNotAvailable(character.decimal)),
            DetailRow("Digit", numberOrNotAvailable(character.digit)),
            DetailRow("Numeric", numeric.isEmpty ? "N/A" : numeric),
            DetailRow(
                "Simple Uppercase Mapping",
                mappedCodepoint(character.uppercase)
            ),
            DetailRow(
                "Simple Lowercase Mapping",
                mappedCodepoint(character.lowercase)
            ),
            DetailRow(
                "Simple Titlecase Mapping",
                mappedCodepoint(character.titlecase)
            ),
        ]
    }

    private func emojiRows(for codepoint: mjb_codepoint) -> [DetailRow] {
        var emoji = mjb_emoji_properties()
        let labelsAndValues: [(String, KeyPath<mjb_emoji_properties, Bool>)] = [
            ("Emoji", \.emoji),
            ("Emoji Presentation", \.presentation),
            ("Emoji Modifier", \.modifier),
            ("Emoji Modifier Base", \.modifier_base),
            ("Emoji Component", \.component),
            ("Extended Pictographic", \.extended_pictographic),
        ]

        guard mjb_codepoint_emoji_properties(codepoint, &emoji) == MJB_STATUS_OK else {
            return labelsAndValues.map { label, _ in
                DetailRow(label, "N/A")
            }
        }

        return labelsAndValues.map { label, keyPath in
            DetailRow(label, MojibakeFormatting.yesOrNo(emoji[keyPath: keyPath]))
        }
    }

    private func unicodePropertyRows(for codepoint: mjb_codepoint) -> [DetailRow] {
        var rows: [DetailRow] = []

        for index in 0..<Int(MJB_PR_COUNT) {
            let property = mjb_property(rawValue: UInt32(index))

            if property == MJB_PR_SCRIPT {
                continue
            }

            guard let propertyName = mjb_property_name(property) else {
                continue
            }

            let label = String(cString: propertyName)

            if isBinaryProperty(property) {
                var value = false
                let status = mjb_codepoint_property_binary(codepoint, property, &value)

                if status == MJB_STATUS_OK && value {
                    rows.append(DetailRow(label, MojibakeFormatting.yesOrNo(value)))
                }
            } else {
                var value: Int32 = 0
                let status = mjb_codepoint_property_int(codepoint, property, &value)

                if status == MJB_STATUS_OK && value != 0 {
                    rows.append(DetailRow(label, String(value)))
                }
            }
        }

        return rows
    }

    private func isBinaryProperty(_ property: mjb_property) -> Bool {
        property == MJB_PR_NFD_QUICK_CHECK
            || property == MJB_PR_NFKD_QUICK_CHECK
            || property.rawValue >= MJB_PR_ASCII_HEX_DIGIT.rawValue
    }

    private func hexEncoding(
        _ codepoint: mjb_codepoint,
        as encoding: mjb_encoding
    ) -> String {
        var buffer = [CChar](repeating: 0, count: 5)
        let length = buffer.withUnsafeMutableBufferPointer {
            mjb_codepoint_encode(codepoint, $0.baseAddress, $0.count, encoding)
        }

        return buffer.prefix(Int(length))
            .map { String(format: "%02X", UInt8(bitPattern: $0)) }
            .joined(separator: " ")
    }

    private func normalized(_ value: String, form: mjb_normalization) -> String? {
        MojibakeString.transform(value) { input, byteLength, result in
            mjb_normalize(
                input,
                byteLength,
                MJB_ENC_UTF_8,
                MJB_MALFORMED_STOP,
                form,
                MJB_ENC_UTF_8,
                result,
                nil
            )
        }
    }

    private func codepointString(_ codepoint: mjb_codepoint) -> String {
        MojibakeFormatting.codepoint(codepoint)
    }

    private func mappedCodepoint(_ codepoint: mjb_codepoint) -> String {
        codepoint == 0 ? "N/A" : codepointString(codepoint)
    }

    private func numberOrNotAvailable(_ value: Int32) -> String {
        value == MJB_NUMBER_NOT_VALID ? "N/A" : String(value)
    }

    private func categoryName(_ category: Int) -> String {
        let names = [
            "Other, not assigned",
            "Letter, uppercase",
            "Letter, lowercase",
            "Letter, titlecase",
            "Letter, modifier",
            "Letter, other",
            "Mark, non-spacing",
            "Mark, spacing combining",
            "Mark, enclosing",
            "Number, decimal digit",
            "Number, letter",
            "Number, other",
            "Punctuation, connector",
            "Punctuation, dash",
            "Punctuation, open",
            "Punctuation, close",
            "Punctuation, initial quote",
            "Punctuation, final quote",
            "Punctuation, other",
            "Symbol, math",
            "Symbol, currency",
            "Symbol, modifier",
            "Symbol, other",
            "Separator, space",
            "Separator, line",
            "Separator, paragraph",
            "Other, control",
            "Other, format",
            "Other, surrogate",
            "Other, private use",
        ]

        return names.indices.contains(category) ? names[category] : "Unknown"
    }

    private func combiningName(_ combining: Int) -> String {
        switch combining {
        case 0:
            return "Not Reordered"
        case 1:
            return "Overlay"
        case 2...5:
            return "Unknown"
        case 6:
            return "Han Reading"
        case 7:
            return "Nukta"
        case 8:
            return "Kana Voicing"
        case 9:
            return "Virama"
        case 10...36, 84, 91, 103, 107, 118, 122, 129, 130, 132:
            return "CCC\(combining)"
        case 200:
            return "Attached Below Left"
        case 202:
            return "Attached Below"
        case 214:
            return "Attached Above"
        case 216:
            return "Attached Above Right"
        case 218:
            return "Below Left"
        case 220:
            return "Below"
        case 222:
            return "Below Right"
        case 224:
            return "Left"
        case 226:
            return "Right"
        case 228:
            return "Above Left"
        case 230:
            return "Above"
        case 232:
            return "Above Right"
        case 233:
            return "Double Below"
        case 234:
            return "Double Above"
        case 240:
            return "Iota_Subscript"
        case 0...240:
            return "Not Reordered"
        default:
            return "Unknown"
        }
    }

    private func bidirectionalName(_ bidirectional: Int) -> String {
        let names = [
            "None",
            "Left-to-right",
            "Right-to-left",
            "Right-to-left arabic",
            "European number",
            "European number separator",
            "European number terminator",
            "Arabic number",
            "Common number separator",
            "Nonspacing mark",
            "Boundary neutral",
            "Paragraph separator",
            "Segment separator",
            "Whitespace",
            "Other neutrals",
            "Left-to-right embedding",
            "Left-to-right override",
            "Right-to-left embedding",
            "Right-to-left override",
            "Pop directional format",
            "Left-to-right isolate",
            "Right-to-left isolate",
            "First strong isolate",
            "Pop directional isolate",
        ]

        return names.indices.contains(bidirectional) ? names[bidirectional] : "Unknown"
    }

    private func decompositionName(_ decomposition: Int) -> String {
        let names = [
            "None",
            "Canonical",
            "Circle",
            "Compatibility",
            "Final",
            "Font",
            "Fraction",
            "Initial",
            "Isolated",
            "Medial",
            "Narrow",
            "No break",
            "Small",
            "Square",
            "Sub",
            "Super",
            "Vertical",
            "Wide",
        ]

        return names.indices.contains(decomposition) ? names[decomposition] : "Unknown"
    }

    private func eastAsianWidthName(_ width: Int) -> String {
        let names = [
            "Not set",
            "Ambiguous",
            "Full-width",
            "Half-width",
            "Neutral",
            "Narrow",
            "Wide",
        ]

        return names.indices.contains(width) ? names[width] : "Unknown"
    }
}
