//
//  The Mojibake library
//
//  This file is distributed under the MIT License. See LICENSE for details.
//

import AppKit
import SwiftUI

@main
struct MojibakeApp: App {
    init() {
        // Tool inputs must reach the library exactly as typed, so disable the
        // automatic substitutions text views read from the app's defaults.
        let substitutionDefaults = [
            "NSAutomaticQuoteSubstitutionEnabled",
            "NSAutomaticDashSubstitutionEnabled",
            "NSAutomaticPeriodSubstitutionEnabled",
            "NSAutomaticTextReplacementEnabled",
            "NSAutomaticSpellingCorrectionEnabled",
        ]

        for key in substitutionDefaults {
            UserDefaults.standard.set(false, forKey: key)
        }
    }

    private var libraryVersion: String {
        versionString(mjb_version())
    }

    private var unicodeVersion: String {
        versionString(mjb_unicode_version())
    }

    private var windowTitle: String {
        "Mojibake v\(libraryVersion)"
    }

    var body: some Scene {
        WindowGroup(windowTitle) {
            ContentView()
        }
        .commands {
            CommandGroup(replacing: .appInfo) {
                Button("About Mojibake") {
                    showAboutPanel()
                }
            }
        }
    }

    private func showAboutPanel() {
        let credits = """
        Mojibake Library Version: \(libraryVersion)
        Unicode Version: \(unicodeVersion)
        """

        NSApp.orderFrontStandardAboutPanel(
            options: [
                .credits: NSAttributedString(string: credits),
            ]
        )
    }

    private func versionString(_ version: UnsafePointer<CChar>?) -> String {
        version.map(String.init(cString:)) ?? "Unknown"
    }
}
