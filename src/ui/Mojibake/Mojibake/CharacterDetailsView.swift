//
//  The Mojibake library
//
//  This file is distributed under the MIT License. See LICENSE for details.
//

import SwiftUI

struct CharacterDetailsView: View {
    let details: CharacterDetails
    let onCodepointSelected: (String) -> Void

    @State private var propertiesAreExpanded = false

    var body: some View {
        ScrollView {
            VStack(spacing: 16) {
                CharacterIdentityView(details: details)

                ForEach(details.sections) { section in
                    DetailSectionView(section: section)
                }

                GroupBox {
                    if propertiesAreExpanded {
                        DetailGrid(rows: details.unicodeProperties)
                            .padding(.vertical, 4)
                    }
                } label: {
                    Button {
                        withAnimation {
                            propertiesAreExpanded.toggle()
                        }
                    } label: {
                        HStack {
                            Image(systemName: "chevron.right")
                                .font(.caption)
                                .rotationEffect(.degrees(propertiesAreExpanded ? 90 : 0))

                            Text("Unicode Properties")
                                .font(.headline)

                            Spacer()

                            Text(details.unicodeProperties.count, format: .number)
                                .font(.caption.monospacedDigit())
                                .foregroundStyle(.secondary)
                        }
                        .contentShape(.rect)
                    }
                    .buttonStyle(.plain)
                    .accessibilityValue(
                        String(
                            localized: propertiesAreExpanded ? "Expanded" : "Collapsed"
                        )
                    )
                }
            }
            .frame(maxWidth: 760)
            .frame(maxWidth: .infinity)
            .padding(.vertical)
        }
        .environment(
            \.openURL,
            OpenURLAction { url in
                guard let codepoint = CodepointLink.codepoint(from: url) else {
                    return .systemAction
                }

                onCodepointSelected(codepoint)
                return .handled
            }
        )
    }
}

private struct CharacterIdentityView: View {
    let details: CharacterDetails

    var body: some View {
        VStack(spacing: 6) {
            CodepointText(details.character)
                .font(.system(size: 64))
                .frame(minHeight: 76)

            CodepointText(details.codepoint)
                .font(.title2.monospaced())

            Text(details.name)
                .font(.headline)
                .multilineTextAlignment(.center)
                .textSelection(.enabled)
        }
        .frame(maxWidth: .infinity)
        .padding(.bottom, 4)
    }
}

private struct DetailSectionView: View {
    let section: DetailSection

    var body: some View {
        GroupBox {
            DetailGrid(rows: section.rows)
                .padding(.vertical, 4)
        } label: {
            Text(section.title)
                .font(.headline)
        }
        .frame(maxWidth: .infinity)
    }
}

private struct DetailGrid: View {
    let rows: [DetailRow]

    var body: some View {
        VStack(spacing: 8) {
            ForEach(rows.indices, id: \.self) { index in
                let row = rows[index]

                HStack(alignment: .firstTextBaseline, spacing: 20) {
                    Text(row.label)
                        .font(.callout.weight(.medium))
                        .foregroundStyle(.secondary)
                        .frame(maxWidth: .infinity, alignment: .trailing)

                    CodepointText(row.value)
                        .font(row.monospaced ? .body.monospaced() : .body)
                        .frame(maxWidth: .infinity, alignment: .leading)
                }
            }
        }
        .frame(maxWidth: .infinity)
    }
}

private struct CodepointText: View {
    let value: String

    init(_ value: String) {
        self.value = value
    }

    var body: some View {
        let text = Text(CodepointLink.attributedString(for: value))

        if CodepointLink.containsCodepoint(in: value) {
            text
                .textSelection(.disabled)
                .pointerStyle(.link)
        } else {
            text
                .textSelection(.enabled)
        }
    }
}

private enum CodepointLink {
    private static let scheme = "mojibake"
    private static let host = "codepoint"
    private static let expression = try! NSRegularExpression(
        pattern: #"(?i)\bU\+[0-9A-F]{4,6}\b"#
    )

    static func containsCodepoint(in value: String) -> Bool {
        expression.firstMatch(
            in: value,
            range: NSRange(value.startIndex..., in: value)
        ) != nil
    }

    static func attributedString(for value: String) -> AttributedString {
        var result = AttributedString()
        var currentIndex = value.startIndex
        let matches = expression.matches(
            in: value,
            range: NSRange(value.startIndex..., in: value)
        )

        for match in matches {
            guard let range = Range(match.range, in: value) else {
                continue
            }

            result.append(AttributedString(String(value[currentIndex..<range.lowerBound])))

            let codepoint = String(value[range]).uppercased()
            var link = AttributedString(codepoint)

            if let url = url(for: codepoint) {
                link.link = url
            }

            result.append(link)
            currentIndex = range.upperBound
        }

        result.append(AttributedString(String(value[currentIndex...])))
        return result
    }

    static func codepoint(from url: URL) -> String? {
        guard url.scheme == scheme,
            url.host == host,
            let hexadecimal = url.pathComponents.last,
            (4...6).contains(hexadecimal.count),
            hexadecimal.allSatisfy(\.isHexDigit) else {
            return nil
        }

        return "U+\(hexadecimal.uppercased())"
    }

    private static func url(for codepoint: String) -> URL? {
        let hexadecimal = codepoint.dropFirst(2)
        return URL(string: "\(scheme)://\(host)/\(hexadecimal)")
    }
}
