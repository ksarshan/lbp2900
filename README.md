# Canon LBP2900 Driver for Mac (macOS, Apple Silicon M1/M2/M3/M4 & Intel)

**Free, open-source Canon LBP2900 / LBP2900B printer driver for macOS.** Canon never released a Mac driver for the LBP2900 that works on modern macOS or Apple Silicon. This one does. One command installs it, and you can print from any app.

🇬🇧 English &nbsp;|&nbsp; [🇮🇳 മലയാളം](README.ml.md)

| | |
|---|---|
| **Printer** | Canon LBP2900, LBP2900B, LBP-2900, LASER SHOT LBP2900, i-SENSYS LBP2900 / LBP2900B |
| **Mac** | Apple Silicon (M1, M2, M3, M4) and Intel |
| **macOS** | Sonoma (14), Sequoia (15), Tahoe (26) |
| **Connection** | USB |
| **Status** | ✅ Verified printing on macOS 26, Apple Silicon |
| **Price** | Free (GPLv3) |

```bash
git clone https://github.com/ksarshan/lbp2900.git
cd lbp2900
./setup.sh
```

---

## Contents

- [Why there is no official Canon LBP2900 Mac driver](#why-there-is-no-official-canon-lbp2900-mac-driver)
- [Features](#features)
- [Requirements](#requirements)
- [How to install the Canon LBP2900 on a Mac](#how-to-install-the-canon-lbp2900-on-a-mac)
- [How to print](#how-to-print)
- [Print progress and printer status](#print-progress-and-printer-status)
- [Troubleshooting](#troubleshooting)
- [FAQ](#faq)
- [How it works](#how-it-works)
- [Build from source](#build-from-source)
- [Uninstall](#uninstall)
- [Roadmap](#roadmap) · [Credits](#credits) · [License](#license)

## Why there is no official Canon LBP2900 Mac driver

The Canon LBP2900 is a **host-based (GDI) laser printer**. It has no PostScript or PCL interpreter and only understands Canon's proprietary **CAPT** protocol (Canon Advanced Printing Technology, with Hi-SCoA compression). The computer has to do all the rendering.

Canon's CAPT driver for macOS only covers the LBP3000 and newer models, and was never updated for Apple Silicon. So on a current Mac, the LBP2900 shows up but won't print, or macOS adds a "Generic" queue that does nothing.

This project fills that gap with a **CUPS filter** (`rastertocapt`) that speaks CAPT, plus a **PPD** for the printer. It's built on the open-source [captdriver](https://github.com/mounaiban/captdriver) project (GPLv3), with [extra fixes](#the-fix) that the LBP2900 needs to actually print on macOS.

## Features

- ✅ **Prints from any Mac app**: Word, Excel, Pages, Preview, Safari, Chrome, PDFs
- ✅ 600 DPI, all standard paper sizes (A4, Letter, Legal, A5, B5, envelopes), plain / heavy / envelope media, toner save, manual duplex
- ✅ **Native Apple Silicon (arm64)** prebuilt binary, no Rosetta needed
- ✅ **No kernel extension, no SIP changes**: uses the standard CUPS USB backend
- ✅ **Survives macOS updates**: a self-healing service reinstalls the driver if an update removes it
- ✅ **Menu-bar app** showing live page progress (`🖨 2/3`)
- ✅ **Out-of-paper** warning and notification; printing resumes when paper is reloaded
- ✅ Offline / unplugged printer reported in the macOS print queue
- ⚠️ Not yet: telling *paper jam* apart from *cover open* (see [Roadmap](#roadmap))

## Requirements

- A Mac running a modern macOS (CUPS is built in to every macOS).
- Canon LBP2900 / LBP2900B connected by **USB** and powered on.
- **Apple Silicon (M1/M2/M3/M4):** nothing else, since a prebuilt filter is included.
- **Intel Mac:** the installer builds from source, so you need the Xcode Command Line Tools (`xcode-select --install`) and autotools (`brew install autoconf automake`).

## How to install the Canon LBP2900 on a Mac

1. Plug in the printer with a USB cable and switch it on.
2. Open **Terminal** and run:

   ```bash
   git clone https://github.com/ksarshan/lbp2900.git
   cd lbp2900
   ./setup.sh
   ```

3. Enter your Mac password when asked. Run `setup.sh` **without** `sudo`; it asks for admin rights only when it needs them.

That's it. `setup.sh`:

1. installs the CAPT filter into `/usr/libexec/cups/filter/` (works with SIP enabled) and the PPD into `/Library/Printers/PPDs/…`,
2. detects the USB printer and creates a print queue named `Canon_LBP2900`,
3. sets up a **self-healing LaunchDaemon** so the driver survives macOS updates,
4. installs the **menu-bar progress app**.

If the printer isn't connected yet, everything still installs. Re-run `./setup.sh` once it's plugged in to add the queue.

<details><summary>Prefer to install the pieces separately?</summary>

```bash
sudo ./install.sh                    # driver + queue + self-heal
cd menubar && ./install-menubar.sh   # menu-bar app (no sudo)
```
</details>

### Surviving macOS updates

A major macOS update can wipe third-party CUPS filters out of `/usr/libexec/cups/filter/`. To prevent that, `install.sh` keeps a copy of the filter and PPD in `/Library/Application Support/CanonLBP2900/` (on the data volume, which updates don't touch). It also installs a LaunchDaemon, `com.lbp2900.heal`, that runs at every boot and **puts the filter, PPD and queue back if they went missing**. After an OS update the printer keeps working with no reinstall. This was verified by simulating a wipe: the daemon restored the filter on its next run.

## How to print

Print from any app and choose **Canon LBP2900**, or use Terminal:

```bash
lpr -P Canon_LBP2900 some-file.pdf
lpstat -p Canon_LBP2900        # queue status
```

## Print progress and printer status

Canon's floating status window is a Windows-only program. On the Mac, this driver reports status into the **native macOS print queue** instead (System Settings ▸ Printers & Scanners ▸ Canon LBP2900 ▸ *Open Print Queue*, or the Dock icon while printing):

- **Progress**: "Printing page N" (`PAGE:` messages)
- **Out of paper**: a warning badge and notification (`STATE: +media-empty`); reload paper and the job continues
- **Offline / not connected**: reported by the CUPS USB backend when the cable is unplugged or the printer is off

### Multi-page jobs (Word, Excel, PDFs)

The driver sends one page at a time and waits for it to physically print before sending the next, reporting progress as it goes. To watch it:

- **Menu-bar app (recommended):** shows `🖨 2/3` in the menu bar while printing. See [`menubar/`](menubar/), or install with `cd menubar && ./install-menubar.sh` (no sudo).
- **Terminal:** run [`progress.sh`](progress.sh):

  ```bash
  ./progress.sh          # e.g.  job #12 "Report.xlsx": printed 4/10 pages
  ```

  It reads CUPS' `job-media-sheets-completed`, which the driver updates in real time. Jobs from the macOS print dialog usually know their total, so you get `printed X / Y`; a plain `lpr file.txt` may only show `printed X`.

> **Why macOS' "Printing N of M" text stays at "1 of M"**
>
> On modern macOS this counter is frozen for the LBP2900 and for every non-AirPrint, host-based CUPS printer. The driver can't fix it:
>
> - The CUPS job counters the filter drives (`job-media-sheets-completed` and `job-impressions-completed`) were measured advancing correctly (`0→1→2→3`) as each page printed.
> - Apple's UI ignores them. It shows the correct total "of 3" even while the CUPS `job-impressions` attribute is unset, which shows Apple's print proxy takes its page count from the spool document, not from the CUPS job.
> - Live progress in that widget only works for **AirPrint / IPP Everywhere** printers, which report back over IPP. The LBP2900 is a raw USB CAPT device with no IPP.
>
> Use the menu-bar app or `progress.sh` for an accurate live count.

## Troubleshooting

Turn on verbose logging and watch it while you print:

```bash
sudo cupsctl LogLevel=debug
tail -f /var/log/cups/error_log      # lines tagged "CAPT:" are from this driver
sudo cupsctl LogLevel=warn           # turn it back off afterwards
```

- **"Filter failed"**: check the filter is present and executable with
  `ls -l /usr/libexec/cups/filter/rastertocapt` (it should be `-rwxr-xr-x root wheel`). Re-run `sudo ./install.sh` if it's missing.
- **Job stuck on "Now printing" forever**: this usually happens after you **cancel a job mid-print**. The cancelled job never sends a clean end-of-job, so the printer's page counters drift and the next job waits forever. Run `cancel -a Canon_LBP2900`, then **power-cycle the printer** (off for about 10 s, on, wait for a steady green light) and print again. This is a [known captdriver behavior](https://github.com/agalakhov/captdriver/issues/7).
- **macOS added a "Generic" or "AirPrint" LBP2900 queue**: the LBP2900 has no AirPrint and that queue won't work. Use the `Canon_LBP2900` queue created by the installer, which has the correct PPD, and delete the other one.
- **Printer not detected**: check the USB cable, switch the printer on, and look for it in `lpinfo -v | grep -i canon`. Then re-run `./setup.sh`.
- **Blank or shifted output**: pick the correct paper size (A4/Letter) in the print dialog. The printer needs margins of about 5 mm.

## FAQ

### Does the Canon LBP2900 work on a Mac?
Yes. With this driver the LBP2900 and LBP2900B print on modern macOS, on both Apple Silicon and Intel Macs.

### Does the Canon LBP2900 work on an M1, M2, M3 or M4 Mac?
Yes. A native arm64 build is included, so it runs without Rosetta.

### Does it work on macOS Sonoma, Sequoia or Tahoe?
Yes. It's tested on macOS 26 (Tahoe) on Apple Silicon and targets macOS 14 and later.

### Is there an official Canon LBP2900 driver for macOS?
No. Canon's Mac CAPT driver doesn't support the LBP2900 on current macOS or on Apple Silicon. This is an unofficial, community driver.

### Is this the same printer as the LBP2900B, LASER SHOT LBP2900 or i-SENSYS LBP2900?
Yes. Those are regional names for the same printer, and this driver supports all of them.

### Do I need to disable SIP or install a kernel extension?
No. The driver is a normal CUPS filter and uses the standard USB backend.

### Will a macOS update break it?
No. The self-healing service reinstalls the driver automatically if an update removes it. See [Surviving macOS updates](#surviving-macos-updates).

### Does it work over Wi-Fi or the network?
The printer itself is USB-only. You can share it from the Mac it's plugged into using macOS printer sharing (System Settings ▸ General ▸ Sharing ▸ Printer Sharing).

### Is it free?
Yes. It's free and open source under the GPLv3.

## How it works

CUPS filter chain: `app → PDF → cgpdftoraster → rastertocapt → usb backend → printer`.

`rastertocapt` rasterizes each page into CAPT / Hi-SCoA compressed bands and streams them to the printer. It uses CUPS' standard **back-channel** (`cupsBackChannelRead`) and **side-channel** (`cupsSideChannelDoRequest`) APIs for the two-way status handshake, so it needs no kernel extension and no privileges beyond `sudo` at install time.

### The fix

Upstream captdriver registers the LBP2900 with a *conditional* status strategy (`capt_get_xstatus` / `capt_wait_ready`). That only fetches the **extended** printer status (CAPT command `0xA0A8`) when the `XSTATUS_CHNG` flag is set, and this printer never sets it. The per-page counters (`page_decoding` / `page_out` / `page_completed`) live **only** in the extended status record, so they were never refreshed. The stale values hung the end-of-page wait loops: the familiar endless `0xE0A0` status poll in [captdriver#3](https://github.com/mounaiban/captdriver/issues/3).

The patch switches the LBP2900 to the strategy used by the working LBP3010: **always** read the extended status (`capt_get_xstatus_only` / `capt_wait_xready_only`). With the counters read correctly they converge (`1/1/1/1`) and the page prints. The same patch adds the `PAGE:` / `STATE:` reporting described above. See [`patches/lbp2900-macos.patch`](patches/lbp2900-macos.patch).

This matches the reverse-engineered CAPT protocol spec ([`captdriver/SPECS`](captdriver/SPECS)): the extended-status record (the reply to `0xA0A8`) carries the page counters at bytes 14–21, and paper-out is STATUS0 bit 1 / STATUS1 bit 14.

## Build from source

The prebuilt arm64 filter is in `prebuilt/arm64/`. To rebuild it (for Intel, or a universal binary), see [`docs/BUILD.md`](docs/BUILD.md). In short:

```bash
cd captdriver
autoreconf -fi && ./configure
make CFLAGS="-D_DARWIN_C_SOURCE -std=gnu99 -O2"     # -> src/rastertocapt
```

The `captdriver/` tree here is upstream captdriver **with the macOS/LBP2900 patch already applied**. It's included so you can build with one command, and to comply with the GPLv3 source requirements.

## Uninstall

```bash
sudo ./uninstall.sh
cd menubar && ./uninstall-menubar.sh
```

## Roadmap

- Decode the cover-open / paper-jam status bits (STATUS2 bit 7 = "Problem" in the spec) to show specific errors instead of a generic stall.
- Universal (arm64 + x86_64) prebuilt binary.

## Credits

- [captdriver](https://github.com/mounaiban/captdriver) by Moses Chong and contributors: the CAPT CUPS filter this is built on.
- The original CAPT reverse engineering by **Alexey Galakhov**, Nicolas Boichat, Benoit Bolsee and others (see [`captdriver/AUTHORS`](captdriver/AUTHORS) and [`captdriver/SPECS`](captdriver/SPECS)).
- macOS port, LBP2900 status-strategy fix and native status reporting: this repository.

This is unofficial software, not endorsed by or affiliated with Canon Inc. "Canon", "LASER SHOT" and "i-SENSYS" are trademarks of Canon Inc.

## License

**GNU General Public License v3**: see [`LICENSE`](LICENSE). captdriver is GPLv3, and this derivative work (patches and built binary) is distributed under the same terms.

---

*Keywords: Canon LBP2900 Mac driver, Canon LBP2900B macOS driver, Canon 2900 driver for MacBook, LBP2900 Apple Silicon, LBP2900 M1 / M2 / M3 / M4, Canon LBP2900 macOS Sonoma / Sequoia / Tahoe, Canon CAPT driver Mac, LBP 2900 printer not working on Mac, install Canon LBP2900 on MacBook.*
