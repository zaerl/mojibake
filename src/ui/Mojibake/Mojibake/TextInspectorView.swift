//
//  The Mojibake library
//
//  This file is distributed under the MIT License. See LICENSE for details.
//

import SwiftUI

struct TextInspectorView: View {
    let onCodepointSelected: (String) -> Void

    @State private var input = ""
    @State private var scalars: [InspectedScalar] = []

    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 16) {
                ToolHeader(
                    "Text Inspector",
                    description: "Inspect every Unicode scalar in a string."
                )

                ToolTextEditor(
                    "Input",
                    text: $input,
                    minimumHeight: 120
                )

                if scalars.isEmpty {
                    ContentUnavailableView(
                        "Enter Text to Inspect",
                        systemImage: "text.quote",
                        description: Text(
                            "Try “hello”, “café”, combining marks, CJK text, or emoji."
                        )
                    )
                    .frame(maxWidth: .infinity, minHeight: 260)
                } else {
                    ScalarList(
                        scalars: scalars,
                        onCodepointSelected: onCodepointSelected
                    )
                }
            }
            .frame(minWidth: 360, maxWidth: 760)
            .frame(maxWidth: .infinity)
            .padding()
        }
        .onChange(of: input) {
            scalars = input.unicodeScalars.enumerated().map {
                InspectedScalar(index: $0.offset, scalar: $0.element)
            }
        }
    }
}

private struct ScalarList: View {
    let scalars: [InspectedScalar]
    let onCodepointSelected: (String) -> Void

    private let columns = [
        GridItem(.adaptive(minimum: 76, maximum: 100), spacing: 8),
    ]

    var body: some View {
        GroupBox {
            LazyVGrid(columns: columns, alignment: .leading, spacing: 8) {
                ForEach(scalars) { scalar in
                    Button {
                        onCodepointSelected(scalar.codepoint)
                    } label: {
                        ScalarTile(scalar: scalar)
                    }
                    .buttonStyle(.plain)
                    .pointerStyle(.link)
                    .help(scalar.helpText)
                    .accessibilityLabel(scalar.accessibilityLabel)
                    .accessibilityHint("Open codepoint details")
                }
            }
            .padding(.vertical, 4)
        } label: {
            HStack {
                Text("Codepoints")
                    .font(.headline)

                Spacer()

                Text(scalars.count, format: .number)
                    .font(.caption.monospacedDigit())
                    .foregroundStyle(.secondary)
            }
        }
    }
}

private struct ScalarTile: View {
    let scalar: InspectedScalar

    var body: some View {
        VStack(spacing: 3) {
            Text(scalar.displayCharacter)
                .font(.title2)
                .frame(minHeight: 30)

            Text(scalar.codepoint)
                .font(.caption2.monospaced())

            Text(scalar.name)
                .font(.caption2)
                .foregroundStyle(.secondary)
                .lineLimit(1)
                .truncationMode(.tail)
                .multilineTextAlignment(.center)
        }
        .frame(maxWidth: .infinity)
        .padding(6)
        .background(.quaternary, in: RoundedRectangle(cornerRadius: 7))
        .overlay {
            RoundedRectangle(cornerRadius: 7)
                .stroke(.secondary.opacity(0.2))
        }
        .contentShape(.rect)
    }
}

private struct InspectedScalar: Identifiable {
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
    TextInspectorView { _ in }
}
