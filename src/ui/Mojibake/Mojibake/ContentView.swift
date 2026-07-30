//
//  The Mojibake library
//
//  This file is distributed under the MIT License. See LICENSE for details.
//

import SwiftUI

struct ContentView: View {
    private enum Section: String, CaseIterable, Identifiable {
        case codepoint
        case emoji
        case security
        case idna = "IDNA"
        case collation
        case caseless = "Caseless Match"
        case `case`
        case breaks
        case bidi
        case terminalWidth = "Terminal Width"
        case normalize
        case filter

        var id: Self {
            self
        }

        var title: String {
            rawValue.capitalized
        }

        var systemImage: String {
            switch self {
            case .codepoint:
                "character.cursor.ibeam"
            case .emoji:
                "face.smiling"
            case .security:
                "lock.shield"
            case .idna:
                "globe"
            case .collation:
                "arrow.up.arrow.down"
            case .caseless:
                "equal.circle"
            case .case:
                "textformat.abc"
            case .breaks:
                "text.word.spacing"
            case .bidi:
                "arrow.left.arrow.right"
            case .terminalWidth:
                "ruler"
            case .normalize:
                "textformat"
            case .filter:
                "line.3.horizontal.decrease"
            }
        }
    }

    @State private var selection: Section? = .codepoint

    var body: some View {
        NavigationSplitView {
            List(Section.allCases, selection: $selection) { section in
                Label(section.title, systemImage: section.systemImage)
                    .tag(section)
            }
            .navigationSplitViewColumnWidth(min: 160, ideal: 180)
        } detail: {
            switch selection {
            case .codepoint:
                CodepointView()
            case .emoji:
                EmojiView()
            case .security:
                SecurityView()
            case .idna:
                IDNAView()
            case .collation:
                CollationView()
            case .caseless:
                CaselessMatchingView()
            case .case:
                CaseView()
            case .breaks:
                BreaksView()
            case .bidi:
                BidiView()
            case .terminalWidth:
                TerminalWidthView()
            case .normalize:
                NormalizationView()
            case .filter:
                FilterView()
            case nil:
                ContentUnavailableView("Select a tool", systemImage: "sidebar.left")
            }
        }
    }
}

#Preview {
    ContentView()
}
