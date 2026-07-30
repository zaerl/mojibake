//
//  The Mojibake library
//
//  This file is distributed under the MIT License. See LICENSE for details.
//

import SwiftUI

struct CodepointView: View {
    private enum LookupState {
        case initial
        case found(CharacterDetails)
        case notFound(String)
        case invalid(String)
    }

    @State private var codepoint = ""
    @State private var lookupState = LookupState.initial

    var body: some View {
        Group {
            switch lookupState {
            case .initial:
                ContentUnavailableView(
                    "Look Up a Codepoint",
                    systemImage: "character.cursor.ibeam",
                    description: Text(
                        "Enter a codepoint in the search field, such as U+0041."
                    )
                )
            case .found(let details):
                CharacterDetailsView(
                    details: details,
                    onCodepointSelected: showCodepoint
                )
                    .id(details.codepoint)
            case .notFound(let query):
                ContentUnavailableView.search(text: query)
            case .invalid(let message):
                ContentUnavailableView(
                    "Invalid Codepoint",
                    systemImage: "exclamationmark.triangle",
                    description: Text(message)
                )
            }
        }
        .frame(minWidth: 360, maxWidth: .infinity, maxHeight: .infinity)
        .padding()
        .searchable(text: $codepoint, prompt: "Codepoint, such as U+0041")
        .onSubmit(of: .search) {
            search()
        }
        .onChange(of: codepoint) { _, newValue in
            if newValue.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty {
                lookupState = .initial
            }
        }
    }

    private func search() {
        search(codepoint)
    }

    private func showCodepoint(_ codepoint: String) {
        self.codepoint = codepoint
        search(codepoint)
    }

    private func search(_ query: String) {
        let input = query.trimmingCharacters(in: .whitespacesAndNewlines)
        let hexadecimal: Substring

        guard !input.isEmpty else {
            lookupState = .initial
            return
        }

        if input.lowercased().hasPrefix("u+") || input.lowercased().hasPrefix("0x") {
            hexadecimal = input.dropFirst(2)
        } else {
            hexadecimal = input[...]
        }

        guard !hexadecimal.isEmpty,
            hexadecimal.allSatisfy(\.isHexDigit),
            let value = mjb_codepoint(hexadecimal, radix: 16) else {
            lookupState = .invalid("Enter a hexadecimal codepoint such as U+0041.")
            return
        }

        var character = mjb_character()
        let status = mjb_codepoint_info(value, &character)

        guard status == MJB_STATUS_OK else {
            if status == MJB_STATUS_NOT_FOUND {
                lookupState = .notFound(String(format: "U+%04X", value))
            } else {
                lookupState = .invalid(
                    String(format: "U+%04X is not a valid Unicode codepoint.", value)
                )
            }
            return
        }

        lookupState = .found(CharacterDetails(character: character))
    }
}

#Preview {
    CodepointView()
}
