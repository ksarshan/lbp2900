// LBP2900Progress — a tiny macOS menu-bar app that shows live print progress
// for the Canon LBP2900 print queue.
//
// Why this exists: Apple's own "Printing N of M" widget is frozen at "1" for
// non-AirPrint / host-based CUPS printers (it polls the printer over IPP, which
// a raw USB CAPT device can't answer). The CUPS job counter the driver updates
// *is* correct, so this app reads that and shows it in the menu bar instead.
//
// How it works: every `Config.pollInterval` seconds it runs
// `/usr/bin/ipptool` against the local CUPS server with a Get-Jobs request and
// parses the active job's `job-media-sheets-completed` (pages printed) and
// `job-impressions` (total pages, when known). The icon is only visible while
// a job is active.
//
// Build:  swiftc -O LBP2900Progress.swift -o LBP2900Progress
// Runs as an accessory (no Dock icon). Quit from its menu.

import Cocoa

/// App-wide settings.
enum Config {
    /// CUPS queue to watch (created by install.sh).
    static let queueName = "Canon_LBP2900"
    /// Seconds between CUPS queries.
    static let pollInterval: TimeInterval = 1.5
    /// Kill an `ipptool` run that takes longer than this (CUPS not responding).
    static let queryTimeout: TimeInterval = 10
}

/// Progress of the active print job, as reported by CUPS.
struct JobProgress {
    /// Document name, if CUPS reports one.
    var name: String?
    /// Sheets printed so far (`job-media-sheets-completed`).
    var pagesDone: Int?
    /// Total pages (`job-impressions`); often unknown for plain `lpr` jobs.
    var totalPages: Int?
}

/// Owns the status-bar item and polls CUPS for the active job.
final class ProgressWatcher: NSObject {
    private let statusItem = NSStatusBar.system.statusItem(withLength: NSStatusItem.variableLength)
    private let statusLine = NSMenuItem(title: "Idle", action: nil, keyEquivalent: "")
    private var timer: Timer?
    /// True while a query is running, so slow CUPS replies don't pile up.
    /// Only touched on the main thread.
    private var isPolling = false
    /// ipptool test file holding the Get-Jobs request.
    private let requestFile: String

    override init() {
        requestFile = NSTemporaryDirectory() + "lbp2900-progress.test"
        super.init()
        writeRequestFile()
        buildMenu()

        let timer = Timer(timeInterval: Config.pollInterval, target: self,
                          selector: #selector(poll), userInfo: nil, repeats: true)
        RunLoop.main.add(timer, forMode: .common)
        self.timer = timer
        poll()
    }

    /// Write the IPP Get-Jobs request once. `$uri` is filled in by ipptool.
    private func writeRequestFile() {
        let request = """
        { OPERATION Get-Jobs
          GROUP operation-attributes-tag
          ATTR charset attributes-charset utf-8
          ATTR naturalLanguage attributes-natural-language en
          ATTR uri printer-uri $uri
          ATTR keyword which-jobs not-completed
          ATTR keyword requested-attributes job-id,job-name,job-media-sheets-completed,job-impressions
        }
        """
        do {
            try request.write(toFile: requestFile, atomically: true, encoding: .utf8)
        } catch {
            NSLog("LBP2900Progress: cannot write %@: %@", requestFile, error.localizedDescription)
        }
    }

    /// Create the (initially hidden) status item and its menu.
    private func buildMenu() {
        statusItem.button?.title = "🖨"
        statusItem.isVisible = false   // hidden until a print job is active
        let menu = NSMenu()
        let header = NSMenuItem(title: "Canon LBP2900", action: nil, keyEquivalent: "")
        header.isEnabled = false
        menu.addItem(header)
        menu.addItem(statusLine)
        menu.addItem(.separator())
        let quit = NSMenuItem(title: "Quit", action: #selector(quitApp), keyEquivalent: "q")
        quit.target = self
        menu.addItem(quit)
        statusItem.menu = menu
    }

    @objc private func quitApp() { NSApp.terminate(nil) }

    /// Ask CUPS for the queue's active job. Blocking; call off the main thread.
    /// Returns nil when there is no active job or the query failed.
    private func queryActiveJob() -> JobProgress? {
        let task = Process()
        task.executableURL = URL(fileURLWithPath: "/usr/bin/ipptool")
        task.arguments = ["-tv", "ipp://localhost/printers/\(Config.queueName)", requestFile]
        let pipe = Pipe()
        task.standardOutput = pipe
        task.standardError = FileHandle.nullDevice
        do { try task.run() } catch { return nil }

        // Never let a hung ipptool block polling forever.
        let watchdog = DispatchWorkItem { if task.isRunning { task.terminate() } }
        DispatchQueue.global().asyncAfter(deadline: .now() + Config.queryTimeout, execute: watchdog)
        let data = pipe.fileHandleForReading.readDataToEndOfFile()
        task.waitUntilExit()
        watchdog.cancel()

        guard let output = String(data: data, encoding: .utf8) else { return nil }
        return Self.parse(ipptoolOutput: output)
    }

    /// Parse `ipptool -tv` output: one `name (type) = value` line per attribute.
    /// Only the first job (the one printing) is used.
    static func parse(ipptoolOutput output: String) -> JobProgress? {
        var haveJob = false
        var progress = JobProgress()
        for line in output.split(separator: "\n") {
            guard let eq = line.range(of: " = ") else { continue }
            let value = String(line[eq.upperBound...])
            if line.contains("job-id (integer)") {
                if haveJob { break }   // start of a second job: stop
                haveJob = true
            } else if line.contains("job-name (name") {
                progress.name = value
            } else if line.contains("job-media-sheets-completed (integer)") {
                progress.pagesDone = Int(value)
            } else if line.contains("job-impressions (integer)") {
                progress.totalPages = Int(value)
            }
        }
        return haveJob ? progress : nil
    }

    /// Timer callback (main thread): start a background query unless one is running.
    @objc private func poll() {
        guard !isPolling else { return }
        isPolling = true
        DispatchQueue.global(qos: .background).async {
            let progress = self.queryActiveJob()
            DispatchQueue.main.async {
                self.isPolling = false
                self.render(progress)
            }
        }
    }

    /// Update the menu-bar title and the status line in the menu.
    private func render(_ progress: JobProgress?) {
        guard let progress = progress else {
            statusItem.isVisible = false   // vanish from the menu bar when idle
            statusLine.title = "Idle"
            return
        }
        statusItem.isVisible = true        // appear only while a job is active

        let done = progress.pagesDone ?? 0
        let title: String
        var line: String
        if done == 0 {
            title = "🖨 …"
            line = "Preparing…"
        } else if let total = progress.totalPages, done <= total {
            title = "🖨 \(done)/\(total)"
            line = "Printing page \(done) of \(total)"
        } else {
            title = "🖨 \(done)"
            line = "Printing page \(done)"
        }
        if let name = progress.name, !name.isEmpty { line += " — \(name)" }
        statusItem.button?.title = title
        statusLine.title = line
    }
}

let app = NSApplication.shared
app.setActivationPolicy(.accessory)   // menu-bar only, no Dock icon
let watcher = ProgressWatcher()
app.run()
