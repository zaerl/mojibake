//
//  The Mojibake library
//
//  This file is distributed under the MIT License. See LICENSE for details.
//

import SwiftUI

struct IDNAView: View {
    @State private var input = ""
    @State private var resolution: IDNAResolution?

    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 16) {
                ToolHeader(
                    "IDNA",
                    description: "Convert and validate domain names using Unicode UTS #46."
                )

                GroupBox {
                    TextField("bücher.example", text: $input)
                        .font(.body.monospaced())
                        .textFieldStyle(.roundedBorder)
                        .accessibilityLabel("Domain name")
                } label: {
                    HStack {
                        Text("Domain")
                            .font(.headline)

                        Spacer()

                        Text(inputSummary)
                            .font(.caption.monospacedDigit())
                            .foregroundStyle(.secondary)
                    }
                }

                if let resolution {
                    IDNAConversionView(conversion: resolution.ascii)
                    IDNAConversionView(conversion: resolution.unicode)
                } else {
                    ContentUnavailableView(
                        "Enter a Domain",
                        systemImage: "globe",
                        description: Text(
                            "Try “bücher.example” or its Punycode form “xn--bcher-kva.example”."
                        )
                    )
                    .frame(maxWidth: .infinity, minHeight: 260)
                }
            }
            .frame(minWidth: 360, maxWidth: 800)
            .frame(maxWidth: .infinity)
            .padding()
        }
        .onChange(of: input) {
            convert()
        }
    }

    private var inputSummary: String {
        let scalarCount = input.unicodeScalars.count
        let scalarLabel = scalarCount == 1 ? "scalar" : "scalars"
        return "\(input.utf8.count) bytes · \(scalarCount) \(scalarLabel)"
    }

    private func convert() {
        resolution = input.isEmpty ? nil : IDNAResolution.convert(input)
    }
}

private struct IDNAConversionView: View {
    let conversion: IDNAConversion

    var body: some View {
        GroupBox {
            VStack(alignment: .leading, spacing: 12) {
                if let output = conversion.output {
                    Text(output.isEmpty ? "Empty output" : output)
                        .font(.title3.monospaced())
                        .foregroundStyle(output.isEmpty ? .secondary : .primary)
                        .textSelection(.enabled)
                        .frame(maxWidth: .infinity, alignment: .leading)

                    Divider()

                    Label {
                        Text(conversion.validationSummary)
                    } icon: {
                        Image(systemName: conversion.validationSystemImage)
                    }
                    .foregroundStyle(conversion.validationColor)

                    if conversion.direction == .ascii && !conversion.isValid {
                        Text("Do not use this ToASCII result for DNS lookup.")
                            .font(.callout)
                            .foregroundStyle(.red)
                    }

                    if !conversion.issues.isEmpty {
                        VStack(alignment: .leading, spacing: 8) {
                            ForEach(conversion.issues) { issue in
                                IDNAIssueView(issue: issue)
                            }
                        }
                    }
                } else {
                    Label(
                        conversion.failureMessage ?? "The conversion failed.",
                        systemImage: "exclamationmark.triangle"
                    )
                    .foregroundStyle(.red)
                    .frame(maxWidth: .infinity, alignment: .leading)
                }
            }
            .padding(.vertical, 4)
        } label: {
            HStack {
                Text(conversion.direction.title)
                    .font(.headline)

                Spacer()

                Text(conversion.direction.subtitle)
                    .font(.caption)
                    .foregroundStyle(.secondary)
            }
        }
    }
}

private struct IDNAIssueView: View {
    let issue: IDNAValidationIssue

    var body: some View {
        HStack(alignment: .top, spacing: 8) {
            Image(systemName: "exclamationmark.circle.fill")
                .foregroundStyle(.orange)

            VStack(alignment: .leading, spacing: 2) {
                Text(issue.title)
                    .font(.callout.weight(.medium))

                Text(issue.description)
                    .font(.caption)
                    .foregroundStyle(.secondary)
            }
        }
    }
}

private struct IDNAResolution {
    let ascii: IDNAConversion
    let unicode: IDNAConversion

    static func convert(_ input: String) -> IDNAResolution {
        IDNAResolution(
            ascii: IDNAConversion.convert(input, direction: .ascii),
            unicode: IDNAConversion.convert(input, direction: .unicode)
        )
    }
}

private struct IDNAConversion {
    enum Direction {
        case ascii
        case unicode

        var title: String {
            switch self {
            case .ascii:
                "ASCII"
            case .unicode:
                "Unicode"
            }
        }

        var subtitle: String {
            switch self {
            case .ascii:
                "Punycode · ToASCII"
            case .unicode:
                "Display form · ToUnicode"
            }
        }
    }

    let direction: Direction
    let output: String?
    let errors: UInt32
    let failureMessage: String?

    var isValid: Bool {
        output != nil && errors == UInt32(MJB_IDNA_ERROR_NONE.rawValue)
    }

    var issues: [IDNAValidationIssue] {
        IDNAValidationIssue.issues(for: errors)
    }

    var validationSummary: String {
        if isValid {
            return direction == .ascii
                ? "Valid for DNS lookup"
                : "No validation issues"
        }

        let issueCount = issues.count
        return "\(issueCount) validation \(issueCount == 1 ? "issue" : "issues")"
    }

    var validationSystemImage: String {
        isValid ? "checkmark.shield.fill" : "exclamationmark.triangle.fill"
    }

    var validationColor: Color {
        isValid ? .green : .orange
    }

    static func convert(_ input: String, direction: Direction) -> IDNAConversion {
        var info = mjb_idna_info()
        var status = MJB_STATUS_OK
        let output = MojibakeString.transform(input) { buffer, byteLength, result in
            switch direction {
            case .ascii:
                status = mjb_idna_to_ascii(
                    buffer,
                    byteLength,
                    MJB_ENC_UTF_8,
                    MJB_ENC_UTF_8,
                    &info,
                    result
                )
            case .unicode:
                status = mjb_idna_to_unicode(
                    buffer,
                    byteLength,
                    MJB_ENC_UTF_8,
                    MJB_ENC_UTF_8,
                    &info,
                    result
                )
            }

            return status
        }

        return IDNAConversion(
            direction: direction,
            output: output,
            errors: info.errors,
            failureMessage: output == nil
                ? MojibakeFormatting.statusMessage(
                    operation: direction == .ascii
                        ? "mjb_idna_to_ascii"
                        : "mjb_idna_to_unicode",
                    status: status
                )
                : nil
        )
    }
}

private struct IDNAValidationIssue: Identifiable {
    let flag: UInt32
    let title: String
    let description: String

    var id: UInt32 {
        flag
    }

    nonisolated static func issues(for errors: UInt32) -> [IDNAValidationIssue] {
        allIssues.filter { errors & $0.flag != 0 }
    }

    private nonisolated static var allIssues: [IDNAValidationIssue] {
        [
            IDNAValidationIssue(
                flag: UInt32(MJB_IDNA_ERROR_PUNYCODE.rawValue),
                title: "Invalid Punycode",
                description: "An ACE label could not be decoded or did not round-trip."
            ),
            IDNAValidationIssue(
                flag: UInt32(MJB_IDNA_ERROR_EMPTY_LABEL.rawValue),
                title: "Empty Label",
                description: "The domain contains an empty label, such as two adjacent dots."
            ),
            IDNAValidationIssue(
                flag: UInt32(MJB_IDNA_ERROR_HYPHEN.rawValue),
                title: "Invalid Hyphen Placement",
                description: "A label violates the UTS #46 hyphen restrictions."
            ),
            IDNAValidationIssue(
                flag: UInt32(MJB_IDNA_ERROR_NOT_NFC.rawValue),
                title: "Not NFC",
                description: "A label is not in Unicode Normalization Form C."
            ),
            IDNAValidationIssue(
                flag: UInt32(MJB_IDNA_ERROR_LEADING_MARK.rawValue),
                title: "Leading Combining Mark",
                description: "A label starts with a combining mark."
            ),
            IDNAValidationIssue(
                flag: UInt32(MJB_IDNA_ERROR_DISALLOWED.rawValue),
                title: "Disallowed Character",
                description: "The domain contains a codepoint disallowed by UTS #46."
            ),
            IDNAValidationIssue(
                flag: UInt32(MJB_IDNA_ERROR_STD3.rawValue),
                title: "STD3 Violation",
                description: "An ASCII character violates the STD3 domain-name rules."
            ),
            IDNAValidationIssue(
                flag: UInt32(MJB_IDNA_ERROR_CONTEXTJ.rawValue),
                title: "Invalid Joiner Context",
                description: "A zero-width joiner or non-joiner appears in an invalid context."
            ),
            IDNAValidationIssue(
                flag: UInt32(MJB_IDNA_ERROR_BIDI.rawValue),
                title: "Bidirectional Rule Violation",
                description: "A right-to-left label violates the IDNA bidi rules."
            ),
            IDNAValidationIssue(
                flag: UInt32(MJB_IDNA_ERROR_LABEL_LENGTH.rawValue),
                title: "Label Too Long",
                description: "A Punycode label exceeds the DNS length limit."
            ),
            IDNAValidationIssue(
                flag: UInt32(MJB_IDNA_ERROR_DOMAIN_LENGTH.rawValue),
                title: "Domain Too Long",
                description: "The complete ASCII domain exceeds the DNS length limit."
            ),
        ]
    }
}

#Preview {
    IDNAView()
}
