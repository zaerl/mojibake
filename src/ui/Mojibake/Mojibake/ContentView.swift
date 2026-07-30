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
