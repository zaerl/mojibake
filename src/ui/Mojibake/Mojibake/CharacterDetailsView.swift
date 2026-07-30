//
//  The Mojibake library
//
//  This file is distributed under the MIT License. See LICENSE for details.
//

import SwiftUI

struct CharacterDetailsView: View {
    let details: CharacterDetails

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
    }
}

private struct CharacterIdentityView: View {
    let details: CharacterDetails

    var body: some View {
        VStack(spacing: 6) {
            Text(details.character)
                .font(.system(size: 64))
                .frame(minHeight: 76)

            Text(details.codepoint)
                .font(.title2.monospaced())

            Text(details.name)
                .font(.headline)
                .multilineTextAlignment(.center)
        }
        .frame(maxWidth: .infinity)
        .textSelection(.enabled)
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

                    Text(row.value)
                        .font(row.monospaced ? .body.monospaced() : .body)
                        .textSelection(.enabled)
                        .frame(maxWidth: .infinity, alignment: .leading)
                }
            }
        }
        .frame(maxWidth: .infinity)
    }
}
