//
//  The Mojibake library
//
//  This file is distributed under the MIT License. See LICENSE for details.
//

import AppKit
import SwiftUI

struct EmojiView: View {
    @State private var input = ""
    @FocusState private var inputIsFocused: Bool

    private var analysis: EmojiAnalysis? {
        input.isEmpty ? nil : EmojiAnalysis(input: input)
    }

    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 16) {
                ToolHeader(
                    "Emoji",
                    description: "Inspect an emoji sequence and each of its Unicode characters."
                )

                if let analysis {
                    DetailSectionView(
                        title: "Sequence",
                        rows: analysis.sequenceRows,
                        linksCodepoints: false
                    )

                    ForEach(analysis.characters) { character in
                        DetailSectionView(
                            title: "Character \(character.position)",
                            rows: character.rows,
                            linksCodepoints: false
                        )
                    }
                } else {
                    ContentUnavailableView(
                        "Choose an Emoji",
                        systemImage: "face.smiling",
                        description: Text(
                            "Open the macOS emoji selector, or paste an emoji "
                                + "into the search field."
                        )
                    )
                    .frame(maxWidth: .infinity, minHeight: 260)
                }
            }
            .frame(minWidth: 360, maxWidth: 760)
            .frame(maxWidth: .infinity)
            .padding()
        }
        .searchable(text: $input, prompt: "Emoji or emoji sequence")
        .searchFocused($inputIsFocused)
        .toolbar {
            Button {
                showEmojiPicker()
            } label: {
                Label("Choose Emoji", systemImage: "face.smiling")
            }
            .help("Open the macOS emoji selector")
        }
    }

    private func showEmojiPicker() {
        inputIsFocused = true

        DispatchQueue.main.async {
            NSApp.orderFrontCharacterPalette(nil)
        }
    }
}

private struct EmojiAnalysis {
    let sequenceRows: [DetailRow]
    let characters: [EmojiCharacterAnalysis]

    init(input: String) {
        let utf8 = input.utf8CString
        let sequence = utf8.withUnsafeBufferPointer { buffer in
            var metadata = mjb_emoji_sequence()
            let byteLength = buffer.count - 1
            let isEmojiSequence = mjb_is_emoji_sequence(
                buffer.baseAddress,
                byteLength,
                MJB_ENC_UTF_8
            )
            let isRGI = mjb_is_rgi_emoji(
                buffer.baseAddress,
                byteLength,
                MJB_ENC_UTF_8
            )
            let hasMetadata = mjb_classify_emoji_sequence(
                buffer.baseAddress,
                byteLength,
                MJB_ENC_UTF_8,
                &metadata
            ) == MJB_STATUS_OK

            if !hasMetadata {
                metadata.type = MJB_EMOJI_SEQUENCE_NONE
                metadata.qualification = MJB_EMOJI_QUALIFICATION_NONE
                metadata.codepoint_count = 0
            }

            return (isEmojiSequence, isRGI, metadata)
        }

        sequenceRows = [
            DetailRow("Input", input),
            DetailRow(
                "Emoji Sequence",
                MojibakeFormatting.yesOrNo(sequence.0)
            ),
            DetailRow("RGI Emoji", MojibakeFormatting.yesOrNo(sequence.1)),
            DetailRow(
                "Sequence Type",
                MojibakeFormatting.idAndName(
                    Int(sequence.2.type.rawValue),
                    Self.sequenceTypeName(sequence.2.type)
                )
            ),
            DetailRow(
                "Qualification",
                MojibakeFormatting.idAndName(
                    Int(sequence.2.qualification.rawValue),
                    Self.qualificationName(sequence.2.qualification)
                )
            ),
            DetailRow(
                "Sequence Codepoints",
                String(sequence.2.codepoint_count)
            ),
        ]

        characters = input.unicodeScalars.enumerated().map { index, scalar in
            EmojiCharacterAnalysis(position: index + 1, scalar: scalar)
        }
    }

    private static func sequenceTypeName(_ type: mjb_emoji_sequence_type) -> String {
        switch type {
        case MJB_EMOJI_SEQUENCE_NONE:
            "None"
        case MJB_EMOJI_SEQUENCE_BASIC:
            "Basic"
        case MJB_EMOJI_SEQUENCE_KEYCAP:
            "Keycap"
        case MJB_EMOJI_SEQUENCE_FLAG:
            "Flag"
        case MJB_EMOJI_SEQUENCE_TAG:
            "Tag"
        case MJB_EMOJI_SEQUENCE_MODIFIER:
            "Modifier"
        case MJB_EMOJI_SEQUENCE_ZWJ:
            "ZWJ"
        case MJB_EMOJI_SEQUENCE_TEXT_VARIATION:
            "Text variation"
        case MJB_EMOJI_SEQUENCE_EMOJI_VARIATION:
            "Emoji variation"
        default:
            "Unknown"
        }
    }

    private static func qualificationName(
        _ qualification: mjb_emoji_qualification
    ) -> String {
        switch qualification {
        case MJB_EMOJI_QUALIFICATION_NONE:
            "None"
        case MJB_EMOJI_QUALIFICATION_COMPONENT:
            "Component"
        case MJB_EMOJI_QUALIFICATION_FULLY_QUALIFIED:
            "Fully-qualified"
        case MJB_EMOJI_QUALIFICATION_MINIMALLY_QUALIFIED:
            "Minimally-qualified"
        case MJB_EMOJI_QUALIFICATION_UNQUALIFIED:
            "Unqualified"
        default:
            "Unknown"
        }
    }
}

private struct EmojiCharacterAnalysis: Identifiable {
    let position: Int
    let rows: [DetailRow]

    var id: Int {
        position
    }

    init(position: Int, scalar: Unicode.Scalar) {
        let codepoint = mjb_codepoint(scalar.value)
        var character = mjb_character()
        let hasCharacterData = mjb_codepoint_info(codepoint, &character) == MJB_STATUS_OK
        var emoji = mjb_emoji_properties()
        let hasEmojiData =
            mjb_codepoint_emoji_properties(codepoint, &emoji) == MJB_STATUS_OK

        self.position = position
        rows = [
            DetailRow(
                "Codepoint",
                MojibakeFormatting.codepoint(codepoint),
                monospaced: true
            ),
            DetailRow(
                "Name",
                hasCharacterData
                    ? MojibakeFormatting.string(fromCString: character.name)
                    : "N/A"
            ),
            DetailRow("Character", String(scalar)),
            DetailRow("Emoji Data", MojibakeFormatting.yesOrNo(hasEmojiData)),
            DetailRow(
                "Emoji",
                MojibakeFormatting.yesOrNo(hasEmojiData && emoji.emoji)
            ),
            DetailRow(
                "Emoji Presentation",
                MojibakeFormatting.yesOrNo(hasEmojiData && emoji.presentation)
            ),
            DetailRow(
                "Emoji Modifier",
                MojibakeFormatting.yesOrNo(hasEmojiData && emoji.modifier)
            ),
            DetailRow(
                "Emoji Modifier Base",
                MojibakeFormatting.yesOrNo(hasEmojiData && emoji.modifier_base)
            ),
            DetailRow(
                "Emoji Component",
                MojibakeFormatting.yesOrNo(hasEmojiData && emoji.component)
            ),
            DetailRow(
                "Extended Pictographic",
                MojibakeFormatting.yesOrNo(
                    hasEmojiData && emoji.extended_pictographic
                )
            ),
        ]
    }
}

#Preview {
    EmojiView()
}
