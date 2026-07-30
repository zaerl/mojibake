//
//  The Mojibake library
//
//  This file is distributed under the MIT License. See LICENSE for details.
//

import SwiftUI

struct CharactersView: View {
    let onCodepointSelected: (String) -> Void

    @State private var input = ""
    @State private var characters: [StringCharacter] = []

    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 16) {
                ToolHeader(
                    "Characters",
                    description: "List the Unicode codepoints in a string and inspect each one."
                )

                ToolTextEditor(
                    "Input",
                    text: $input,
                    minimumHeight: 120
                )

                if characters.isEmpty {
                    ContentUnavailableView(
                        "Enter Text to Inspect",
                        systemImage: "text.quote",
                        description: Text(
                            "Try “hello”, “café”, combining marks, CJK text, or emoji."
                        )
                    )
                    .frame(maxWidth: .infinity, minHeight: 260)
                } else {
                    CharactersList(
                        characters: characters,
                        onCodepointSelected: onCodepointSelected
                    )
                }
            }
            .frame(minWidth: 360, maxWidth: 760)
            .frame(maxWidth: .infinity)
            .padding()
        }
        .onChange(of: input) {
            characters = input.unicodeScalars.enumerated().map {
                StringCharacter(index: $0.offset, scalar: $0.element)
            }
        }
    }
}

private struct CharactersList: View {
    let characters: [StringCharacter]
    let onCodepointSelected: (String) -> Void

    private let columns = [
        GridItem(.adaptive(minimum: 150), spacing: 8),
    ]

    var body: some View {
        GroupBox {
            LazyVGrid(columns: columns, alignment: .leading, spacing: 8) {
                ForEach(characters) { character in
                    Button {
                        onCodepointSelected(character.codepoint)
                    } label: {
                        CharacterTile(character: character)
                    }
                    .buttonStyle(.plain)
                    .pointerStyle(.link)
                    .help(character.helpText)
                    .accessibilityLabel(character.accessibilityLabel)
                    .accessibilityHint("Open codepoint details")
                }
            }
            .padding(.vertical, 4)
        } label: {
            HStack {
                Text("Codepoints")
                    .font(.headline)

                Spacer()

                Text(characters.count, format: .number)
                    .font(.caption.monospacedDigit())
                    .foregroundStyle(.secondary)
            }
        }
    }
}

private struct CharacterTile: View {
    let character: StringCharacter

    var body: some View {
        VStack(spacing: 6) {
            Text(character.displayCharacter)
                .font(.system(size: 34))
                .frame(minHeight: 46)

            Text(character.codepoint)
                .font(.callout.monospaced())

            Text(character.name)
                .font(.caption)
                .foregroundStyle(.secondary)
                .lineLimit(2)
                .multilineTextAlignment(.center)
                .frame(height: 32, alignment: .top)
        }
        .frame(maxWidth: .infinity, minHeight: 122)
        .padding(8)
        .background(.quaternary, in: RoundedRectangle(cornerRadius: 7))
        .overlay {
            RoundedRectangle(cornerRadius: 7)
                .stroke(.secondary.opacity(0.2))
        }
        .contentShape(.rect)
    }
}

private struct StringCharacter: Identifiable {
    let id: Int
    let displayCharacter: String
    let codepoint: String
    let name: String

    var helpText: String {
        "\(codepoint) · \(name)"
    }

    var accessibilityLabel: String {
        "\(displayCharacter), \(codepoint), \(name)"
    }

    init(index: Int, scalar: Unicode.Scalar) {
        id = index
        codepoint = MojibakeFormatting.codepoint(scalar.value)

        var character = mjb_character()
        if mjb_codepoint_info(scalar.value, &character) == MJB_STATUS_OK {
            displayCharacter = CharacterDetails.displayCharacter(
                for: character,
                encodedCharacter: String(scalar)
            )
            name = MojibakeFormatting.string(fromCString: character.name)
        } else {
            displayCharacter = String(scalar)
            name = String(localized: "Unassigned")
        }
    }
}

#Preview {
    CharactersView { _ in }
}
