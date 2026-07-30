//
//  The Mojibake library
//
//  This file is distributed under the MIT License. See LICENSE for details.
//

import SwiftUI

struct CaselessMatchingView: View {
    @State private var firstInput = ""
    @State private var secondInput = ""
    @State private var results: [CaselessMatchResult] = []

    private let columns = [
        GridItem(.adaptive(minimum: 300), alignment: .top),
    ]

    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 16) {
                ToolHeader(
                    "Caseless Matching",
                    description: "Compare text using every Unicode caseless matching relation."
                )

                ViewThatFits(in: .horizontal) {
                    HStack(alignment: .top, spacing: 12) {
                        inputEditors
                    }

                    VStack(spacing: 12) {
                        inputEditors
                    }
                }

                if hasInput {
                    LazyVGrid(columns: columns, alignment: .leading, spacing: 12) {
                        ForEach(results) { result in
                            CaselessMatchResultView(result: result)
                        }
                    }
                } else {
                    ContentUnavailableView(
                        "Enter Text to Compare",
                        systemImage: "equal.circle",
                        description: Text(
                            "Try “Straße” and “STRASSE” to see full case folding."
                        )
                    )
                    .frame(maxWidth: .infinity, minHeight: 260)
                }
            }
            .frame(minWidth: 360, maxWidth: 900)
            .frame(maxWidth: .infinity)
            .padding()
        }
        .onChange(of: firstInput) {
            compare()
        }
        .onChange(of: secondInput) {
            compare()
        }
    }

    @ViewBuilder
    private var inputEditors: some View {
        ToolTextEditor("First Text", text: $firstInput)
        ToolTextEditor("Second Text", text: $secondInput)
    }

    private var hasInput: Bool {
        !firstInput.isEmpty || !secondInput.isEmpty
    }

    private func compare() {
        guard hasInput else {
            results = []
            return
        }

        results = CaselessMatchMode.allCases.map {
            CaselessMatchResult.compare(firstInput, with: secondInput, mode: $0)
        }
    }
}

private struct CaselessMatchResultView: View {
    let result: CaselessMatchResult

    var body: some View {
        GroupBox {
            VStack(alignment: .leading, spacing: 10) {
                if let matches = result.matches {
                    Label {
                        Text(matches ? "Caseless Match" : "No Match")
                            .font(.headline)
                    } icon: {
                        Image(
                            systemName: matches
                                ? "checkmark.circle.fill"
                                : "xmark.circle.fill"
                        )
                    }
                    .foregroundStyle(matches ? .green : .secondary)
                } else {
                    Label(
                        result.errorMessage ?? "The comparison failed.",
                        systemImage: "exclamationmark.triangle"
                    )
                    .foregroundStyle(.red)
                }

                Divider()

                Text(result.mode.description)
                    .font(.callout)
                    .foregroundStyle(.secondary)
            }
            .frame(maxWidth: .infinity, alignment: .leading)
            .padding(.vertical, 4)
        } label: {
            HStack {
                Text(result.mode.rawValue)
                    .font(.headline)

                Spacer()

                Text(result.mode.constantName)
                    .font(.caption.monospaced())
                    .foregroundStyle(.secondary)
            }
        }
    }
}

private struct CaselessMatchResult: Identifiable {
    let mode: CaselessMatchMode
    let matches: Bool?
    let errorMessage: String?

    var id: CaselessMatchMode {
        mode
    }

    static func compare(
        _ first: String,
        with second: String,
        mode: CaselessMatchMode
    ) -> CaselessMatchResult {
        let firstUTF8 = first.utf8CString
        let secondUTF8 = second.utf8CString

        return firstUTF8.withUnsafeBufferPointer { firstBuffer in
            secondUTF8.withUnsafeBufferPointer { secondBuffer in
                var matches = false
                let status = mjb_caseless_match(
                    firstBuffer.baseAddress,
                    firstBuffer.count - 1,
                    MJB_ENC_UTF_8,
                    secondBuffer.baseAddress,
                    secondBuffer.count - 1,
                    MJB_ENC_UTF_8,
                    mode.value,
                    &matches
                )

                guard status == MJB_STATUS_OK else {
                    return CaselessMatchResult(
                        mode: mode,
                        matches: nil,
                        errorMessage: MojibakeFormatting.statusMessage(
                            operation: "mjb_caseless_match",
                            status: status
                        )
                    )
                }

                return CaselessMatchResult(
                    mode: mode,
                    matches: matches,
                    errorMessage: nil
                )
            }
        }
    }
}

private enum CaselessMatchMode: String, CaseIterable, Identifiable {
    case canonical = "Canonical"
    case unnormalized = "Unnormalized"
    case compatibility = "Compatibility"
    case identifier = "Identifier"

    var id: Self {
        self
    }

    var value: mjb_caseless_mode {
        switch self {
        case .canonical:
            MJB_CASELESS_CANONICAL
        case .unnormalized:
            MJB_CASELESS_UNNORMALIZED
        case .compatibility:
            MJB_CASELESS_COMPATIBILITY
        case .identifier:
            MJB_CASELESS_IDENTIFIER
        }
    }

    var constantName: String {
        switch self {
        case .canonical:
            "MJB_CASELESS_CANONICAL"
        case .unnormalized:
            "MJB_CASELESS_UNNORMALIZED"
        case .compatibility:
            "MJB_CASELESS_COMPATIBILITY"
        case .identifier:
            "MJB_CASELESS_IDENTIFIER"
        }
    }

    var description: String {
        switch self {
        case .canonical:
            "Ignores case and canonical representation differences."
        case .unnormalized:
            "Applies full case folding without normalizing either string."
        case .compatibility:
            "Also treats compatibility-equivalent characters as matching."
        case .identifier:
            "Uses NFKC case folding and removes default-ignorable characters."
        }
    }
}

#Preview {
    CaselessMatchingView()
}
