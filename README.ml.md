# Mac-നുള്ള Canon LBP2900 ഡ്രൈവർ (macOS, Apple Silicon M1/M2/M3/M4, Intel)

**macOS-നുള്ള സൗജന്യ, ഓപ്പൺ സോഴ്സ് Canon LBP2900 / LBP2900B പ്രിന്റർ ഡ്രൈവർ.** ആധുനിക macOS-ലോ Apple Silicon-ലോ പ്രവർത്തിക്കുന്ന ഒരു Mac ഡ്രൈവർ Canon LBP2900-ന് പുറത്തിറക്കിയിട്ടില്ല. ഇത് പ്രവർത്തിക്കും. ഒരൊറ്റ കമാൻഡ് കൊണ്ട് ഇൻസ്റ്റാൾ ചെയ്യാം, പിന്നെ ഏത് app-ൽ നിന്നും പ്രിന്റ് ചെയ്യാം.

[🇬🇧 English](README.md) &nbsp;|&nbsp; 🇮🇳 മലയാളം

| | |
|---|---|
| **പ്രിന്റർ** | Canon LBP2900, LBP2900B, LBP-2900, LASER SHOT LBP2900, i-SENSYS LBP2900 / LBP2900B |
| **Mac** | Apple Silicon (M1, M2, M3, M4), Intel |
| **macOS** | Sonoma (14), Sequoia (15), Tahoe (26) |
| **കണക്ഷൻ** | USB |
| **നില** | ✅ macOS 26, Apple Silicon-ൽ പ്രിന്റിംഗ് പരിശോധിച്ച് ഉറപ്പാക്കി |
| **വില** | സൗജന്യം (GPLv3) |

```bash
git clone https://github.com/ksarshan/lbp2900.git
cd lbp2900
./setup.sh
```

---

## ഉള്ളടക്കം

- [ഔദ്യോഗിക ഡ്രൈവർ ഇല്ലാത്തത് എന്തുകൊണ്ട്](#ഔദ്യോഗിക-ഡ്രൈവർ-ഇല്ലാത്തത്-എന്തുകൊണ്ട്)
- [പ്രവർത്തിക്കുന്നവ](#പ്രവർത്തിക്കുന്നവ)
- [ആവശ്യകതകൾ](#ആവശ്യകതകൾ)
- [ഇൻസ്റ്റാൾ ചെയ്യുന്ന വിധം](#ഇൻസ്റ്റാൾ-ചെയ്യുന്ന-വിധം)
- [പ്രിന്റ് ചെയ്യുന്ന വിധം](#പ്രിന്റ്-ചെയ്യുന്ന-വിധം)
- [പ്രിന്റ് പുരോഗതിയും നിലയും](#പ്രിന്റ്-പുരോഗതിയും-നിലയും)
- [പ്രശ്നപരിഹാരം](#പ്രശ്നപരിഹാരം)
- [പതിവ് ചോദ്യങ്ങൾ (FAQ)](#പതിവ്-ചോദ്യങ്ങൾ-faq)
- [ഇത് എങ്ങനെ പ്രവർത്തിക്കുന്നു](#ഇത്-എങ്ങനെ-പ്രവർത്തിക്കുന്നു)
- [Source-ൽ നിന്ന് build ചെയ്യൽ](#source-ൽ-നിന്ന്-build-ചെയ്യൽ)
- [അൺഇൻസ്റ്റാൾ](#അൺഇൻസ്റ്റാൾ)
- [ഭാവി പദ്ധതികൾ](#ഭാവി-പദ്ധതികൾ) · [കടപ്പാട്](#കടപ്പാട്) · [ലൈസൻസ്](#ലൈസൻസ്)

## ഔദ്യോഗിക ഡ്രൈവർ ഇല്ലാത്തത് എന്തുകൊണ്ട്

Canon LBP2900 ഒരു **host-based (GDI) ലേസർ പ്രിന്റർ** ആണ്. ഇതിന് PostScript/PCL interpreter ഇല്ല. Canon-ന്റെ സ്വന്തം **CAPT** protocol (Canon Advanced Printing Technology, Hi-SCoA compression) മാത്രമേ ഇതിന് മനസ്സിലാകൂ. പേജ് തയ്യാറാക്കുന്ന ജോലി മുഴുവൻ കമ്പ്യൂട്ടർ ചെയ്യണം.

Canon-ന്റെ macOS CAPT ഡ്രൈവർ LBP3000-ഉം അതിനു ശേഷമുള്ള മോഡലുകളും മാത്രമേ പിന്തുണയ്ക്കുന്നുള്ളൂ, Apple Silicon-നായി അത് ഒരിക്കലും അപ്ഡേറ്റ് ചെയ്തിട്ടുമില്ല. അതുകൊണ്ട് പുതിയ Mac-ൽ LBP2900 കാണിക്കുമെങ്കിലും പ്രിന്റ് ചെയ്യില്ല, അല്ലെങ്കിൽ ഒന്നും ചെയ്യാത്ത ഒരു "Generic" queue macOS ചേർക്കും.

ഈ കുറവ് നികത്താൻ ഈ പ്രോജക്ട് CAPT സംസാരിക്കുന്ന ഒരു **CUPS filter** (`rastertocapt`), പ്രിന്ററിനുള്ള ഒരു **PPD** എന്നിവ നൽകുന്നു. ഓപ്പൺ സോഴ്സ് [captdriver](https://github.com/mounaiban/captdriver) പ്രോജക്ടിനെ (GPLv3) അടിസ്ഥാനമാക്കിയാണ് ഇത് നിർമ്മിച്ചത്. LBP2900 macOS-ൽ യഥാർത്ഥത്തിൽ പ്രിന്റ് ചെയ്യാൻ ആവശ്യമായ [അധിക തിരുത്തലുകളും](#പരിഹാരം) ഇതിലുണ്ട്.

## പ്രവർത്തിക്കുന്നവ

- ✅ **ഏത് Mac app-ൽ നിന്നും പ്രിന്റ് ചെയ്യാം**: Word, Excel, Pages, Preview, Safari, Chrome, PDF-കൾ
- ✅ 600 DPI, എല്ലാ സാധാരണ പേപ്പർ വലുപ്പങ്ങളും (A4, Letter, Legal, A5, B5, envelope), plain / heavy / envelope media, toner save, manual duplex
- ✅ **Native Apple Silicon (arm64)** prebuilt binary, Rosetta വേണ്ട
- ✅ **Kernel extension വേണ്ട, SIP മാറ്റങ്ങൾ വേണ്ട**: സാധാരണ CUPS USB backend ഉപയോഗിക്കുന്നു
- ✅ **macOS അപ്ഡേറ്റുകൾക്ക് ശേഷവും പ്രവർത്തിക്കും**: അപ്ഡേറ്റ് ഡ്രൈവർ നീക്കം ചെയ്താൽ ഒരു self-healing service അത് വീണ്ടും ഇൻസ്റ്റാൾ ചെയ്യും
- ✅ തത്സമയ പേജ് പുരോഗതി (`🖨 2/3`) കാണിക്കുന്ന **menu-bar app**
- ✅ **പേപ്പർ തീർന്നാൽ** മുന്നറിയിപ്പും അറിയിപ്പും; പേപ്പർ വീണ്ടും ഇട്ടാൽ പ്രിന്റിംഗ് തുടരും
- ✅ പ്രിന്റർ offline ആകുമ്പോഴോ കേബിൾ ഊരുമ്പോഴോ macOS print queue-ൽ അറിയിക്കും
- ⚠️ ഇതുവരെ ഇല്ല: *paper jam*, *cover open* എന്നിവ വേർതിരിച്ചറിയൽ ([ഭാവി പദ്ധതികൾ](#ഭാവി-പദ്ധതികൾ) കാണുക)

## ആവശ്യകതകൾ

- ആധുനിക macOS ഉള്ള ഒരു Mac (എല്ലാ macOS-ലും CUPS ഉണ്ട്).
- **USB** വഴി കണക്ട് ചെയ്ത് ഓൺ ചെയ്ത Canon LBP2900 / LBP2900B.
- **Apple Silicon (M1/M2/M3/M4):** മറ്റൊന്നും വേണ്ട, prebuilt filter ഉൾപ്പെടുത്തിയിട്ടുണ്ട്.
- **Intel Mac:** installer source-ൽ നിന്ന് build ചെയ്യും, അതിനാൽ Xcode Command Line Tools (`xcode-select --install`), autotools (`brew install autoconf automake`) എന്നിവ വേണം.

## ഇൻസ്റ്റാൾ ചെയ്യുന്ന വിധം

1. USB കേബിൾ ഉപയോഗിച്ച് പ്രിന്റർ കണക്ട് ചെയ്ത് ഓൺ ചെയ്യുക.
2. **Terminal** തുറന്ന് ഇത് പ്രവർത്തിപ്പിക്കുക:

   ```bash
   git clone https://github.com/ksarshan/lbp2900.git
   cd lbp2900
   ./setup.sh
   ```

3. ചോദിക്കുമ്പോൾ Mac password നൽകുക. `setup.sh` `sudo` **ഇല്ലാതെ** പ്രവർത്തിപ്പിക്കുക; admin അവകാശം ആവശ്യമുള്ളപ്പോൾ മാത്രം അത് ചോദിക്കും.

ഇത്രയേ ഉള്ളൂ. `setup.sh`:

1. CAPT filter `/usr/libexec/cups/filter/`-ലേക്കും (SIP ഓണായിരിക്കുമ്പോഴും പ്രവർത്തിക്കും) PPD `/Library/Printers/PPDs/…`-ലേക്കും ഇൻസ്റ്റാൾ ചെയ്യുന്നു,
2. USB പ്രിന്റർ കണ്ടെത്തി `Canon_LBP2900` എന്ന പേരിൽ ഒരു print queue ഉണ്ടാക്കുന്നു,
3. macOS അപ്ഡേറ്റുകൾക്ക് ശേഷവും ഡ്രൈവർ നിലനിൽക്കാൻ ഒരു **self-healing LaunchDaemon** സജ്ജമാക്കുന്നു,
4. **menu-bar progress app** ഇൻസ്റ്റാൾ ചെയ്യുന്നു.

പ്രിന്റർ ഇതുവരെ കണക്ട് ചെയ്തിട്ടില്ലെങ്കിലും എല്ലാം ഇൻസ്റ്റാൾ ആകും. പ്രിന്റർ കണക്ട് ചെയ്ത ശേഷം queue ചേർക്കാൻ `./setup.sh` ഒരിക്കൽ കൂടി പ്രവർത്തിപ്പിക്കുക.

<details><summary>ഭാഗങ്ങൾ വെവ്വേറെ ഇൻസ്റ്റാൾ ചെയ്യണോ?</summary>

```bash
sudo ./install.sh                    # ഡ്രൈവർ + queue + self-heal
cd menubar && ./install-menubar.sh   # menu-bar app (sudo വേണ്ട)
```
</details>

### macOS അപ്ഡേറ്റുകൾക്ക് ശേഷവും പ്രവർത്തിക്കാൻ

ഒരു വലിയ macOS അപ്ഡേറ്റ് `/usr/libexec/cups/filter/`-ൽ നിന്ന് മറ്റ് കമ്പനികളുടെ CUPS filter-കൾ നീക്കം ചെയ്തേക്കാം. ഇത് തടയാൻ `install.sh` filter-ന്റെയും PPD-യുടെയും ഒരു പകർപ്പ് `/Library/Application Support/CanonLBP2900/`-ൽ (അപ്ഡേറ്റുകൾ തൊടാത്ത data volume-ൽ) സൂക്ഷിക്കുന്നു. ഓരോ boot-ലും പ്രവർത്തിക്കുന്ന `com.lbp2900.heal` എന്ന LaunchDaemon-ഉം ഇൻസ്റ്റാൾ ചെയ്യുന്നു; **filter, PPD, queue എന്നിവ നഷ്ടപ്പെട്ടാൽ അത് അവ തിരികെ വയ്ക്കും**. OS അപ്ഡേറ്റിന് ശേഷവും വീണ്ടും ഇൻസ്റ്റാൾ ചെയ്യാതെ പ്രിന്റർ പ്രവർത്തിക്കും. filter നീക്കം ചെയ്തത് അനുകരിച്ച് ഇത് പരിശോധിച്ചു: അടുത്ത തവണ daemon filter പുനഃസ്ഥാപിച്ചു.

## പ്രിന്റ് ചെയ്യുന്ന വിധം

ഏത് app-ൽ നിന്നും പ്രിന്റ് ചെയ്ത് **Canon LBP2900** തിരഞ്ഞെടുക്കുക, അല്ലെങ്കിൽ Terminal ഉപയോഗിക്കുക:

```bash
lpr -P Canon_LBP2900 some-file.pdf
lpstat -p Canon_LBP2900        # queue നില
```

## പ്രിന്റ് പുരോഗതിയും നിലയും

Canon-ന്റെ floating status window Windows-ൽ മാത്രമുള്ള ഒരു program ആണ്. Mac-ൽ ഈ ഡ്രൈവർ നില വിവരങ്ങൾ **macOS-ന്റെ സ്വന്തം print queue-ലേക്ക്** നൽകുന്നു (System Settings ▸ Printers & Scanners ▸ Canon LBP2900 ▸ *Open Print Queue*, അല്ലെങ്കിൽ പ്രിന്റ് ചെയ്യുമ്പോൾ Dock-ലെ ഐക്കൺ):

- **പുരോഗതി**: "Printing page N" (`PAGE:` സന്ദേശങ്ങൾ)
- **പേപ്പർ തീർന്നു**: ഒരു മുന്നറിയിപ്പ് badge-ഉം അറിയിപ്പും (`STATE: +media-empty`); പേപ്പർ വീണ്ടും ഇട്ടാൽ ജോലി തുടരും
- **Offline / കണക്ട് ചെയ്തിട്ടില്ല**: കേബിൾ ഊരുമ്പോഴോ പ്രിന്റർ ഓഫാകുമ്പോഴോ CUPS USB backend അറിയിക്കുന്നു

### ഒന്നിലധികം പേജുകളുള്ള ജോലികൾ (Word, Excel, PDF)

ഡ്രൈവർ ഒരു സമയം ഒരു പേജ് അയയ്ക്കുകയും, അത് യഥാർത്ഥത്തിൽ പ്രിന്റ് ആയ ശേഷം മാത്രം അടുത്തത് അയയ്ക്കുകയും, ഒപ്പം പുരോഗതി അറിയിക്കുകയും ചെയ്യുന്നു. ഇത് കാണാൻ:

- **Menu-bar app (ശുപാർശ ചെയ്യുന്നു):** പ്രിന്റ് ചെയ്യുമ്പോൾ menu bar-ൽ `🖨 2/3` കാണിക്കും. [`menubar/`](menubar/) കാണുക, അല്ലെങ്കിൽ `cd menubar && ./install-menubar.sh` (sudo വേണ്ട) ഉപയോഗിച്ച് ഇൻസ്റ്റാൾ ചെയ്യുക.
- **Terminal:** [`progress.sh`](progress.sh) പ്രവർത്തിപ്പിക്കുക:

  ```bash
  ./progress.sh          # ഉദാ.  job #12 "Report.xlsx": printed 4/10 pages
  ```

  ഡ്രൈവർ തത്സമയം പുതുക്കുന്ന CUPS-ന്റെ `job-media-sheets-completed` ഇത് വായിക്കുന്നു. macOS print dialog-ൽ നിന്നുള്ള ജോലികൾക്ക് സാധാരണ ആകെ പേജുകളുടെ എണ്ണം അറിയാം, അതിനാൽ `printed X / Y` ലഭിക്കും; ഒരു സാധാരണ `lpr file.txt` `printed X` മാത്രം കാണിച്ചേക്കാം.

> **macOS-ന്റെ "Printing N of M" എഴുത്ത് "1 of M"-ൽ നിൽക്കുന്നത് എന്തുകൊണ്ട്**
>
> ആധുനിക macOS-ൽ LBP2900-നും AirPrint അല്ലാത്ത, host-based ആയ എല്ലാ CUPS പ്രിന്ററുകൾക്കും ഈ കൗണ്ടർ മരവിച്ചു നിൽക്കും. ഡ്രൈവറിന് ഇത് ശരിയാക്കാൻ കഴിയില്ല:
>
> - filter പുതുക്കുന്ന CUPS job കൗണ്ടറുകൾ (`job-media-sheets-completed`, `job-impressions-completed`) ഓരോ പേജ് പ്രിന്റ് ചെയ്യുമ്പോഴും ശരിയായി മുന്നോട്ട് പോകുന്നതായി (`0→1→2→3`) അളന്നു.
> - Apple-ന്റെ UI അവയെ അവഗണിക്കുന്നു. CUPS `job-impressions` attribute സജ്ജമാക്കാതിരിക്കുമ്പോഴും അത് ശരിയായ ആകെ "of 3" കാണിക്കുന്നു. Apple-ന്റെ print proxy പേജ് എണ്ണം എടുക്കുന്നത് CUPS job-ൽ നിന്നല്ല, spool document-ൽ നിന്നാണ് എന്ന് ഇത് കാണിക്കുന്നു.
> - ആ widget-ലെ തത്സമയ പുരോഗതി IPP വഴി വിവരം തിരികെ അയയ്ക്കുന്ന **AirPrint / IPP Everywhere** പ്രിന്ററുകൾക്ക് മാത്രമേ പ്രവർത്തിക്കൂ. LBP2900 IPP ഇല്ലാത്ത ഒരു raw USB CAPT ഉപകരണമാണ്.
>
> കൃത്യമായ തത്സമയ എണ്ണത്തിന് menu-bar app അല്ലെങ്കിൽ `progress.sh` ഉപയോഗിക്കുക.

## പ്രശ്നപരിഹാരം

വിശദമായ logging ഓണാക്കി പ്രിന്റ് ചെയ്യുമ്പോൾ അത് നിരീക്ഷിക്കുക:

```bash
sudo cupsctl LogLevel=debug
tail -f /var/log/cups/error_log      # "CAPT:" എന്ന് അടയാളപ്പെടുത്തിയ വരികൾ ഈ ഡ്രൈവറിൽ നിന്നുള്ളതാണ്
sudo cupsctl LogLevel=warn           # പിന്നീട് ഇത് ഓഫാക്കുക
```

- **"Filter failed"**: filter ഉണ്ടെന്നും പ്രവർത്തിപ്പിക്കാവുന്നതാണെന്നും
  `ls -l /usr/libexec/cups/filter/rastertocapt` ഉപയോഗിച്ച് പരിശോധിക്കുക (`-rwxr-xr-x root wheel` ആയിരിക്കണം). ഇല്ലെങ്കിൽ `sudo ./install.sh` വീണ്ടും പ്രവർത്തിപ്പിക്കുക.
- **ജോലി എപ്പോഴും "Now printing"-ൽ കുടുങ്ങുന്നു**: സാധാരണയായി **പ്രിന്റിംഗിനിടയിൽ ഒരു ജോലി റദ്ദാക്കിയ ശേഷം** ഇത് സംഭവിക്കും. റദ്ദാക്കിയ ജോലി ശരിയായ end-of-job അയയ്ക്കാത്തതിനാൽ പ്രിന്ററിന്റെ പേജ് കൗണ്ടറുകൾ തെറ്റുകയും അടുത്ത ജോലി എപ്പോഴും കാത്തിരിക്കുകയും ചെയ്യും. `cancel -a Canon_LBP2900` പ്രവർത്തിപ്പിക്കുക, തുടർന്ന് **പ്രിന്റർ ഓഫാക്കി ഓണാക്കുക** (ഏകദേശം 10 സെക്കൻഡ് ഓഫ്, ഓൺ, സ്ഥിരമായ പച്ച ലൈറ്റിനായി കാത്തിരിക്കുക), വീണ്ടും പ്രിന്റ് ചെയ്യുക. ഇത് ഒരു [അറിയപ്പെടുന്ന captdriver സ്വഭാവമാണ്](https://github.com/agalakhov/captdriver/issues/7).
- **macOS ഒരു "Generic" അല്ലെങ്കിൽ "AirPrint" LBP2900 queue ചേർത്തു**: LBP2900-ന് AirPrint ഇല്ല, ആ queue പ്രവർത്തിക്കില്ല. installer ഉണ്ടാക്കിയ, ശരിയായ PPD ഉള്ള `Canon_LBP2900` queue ഉപയോഗിക്കുക, മറ്റേത് ഡിലീറ്റ് ചെയ്യുക.
- **പ്രിന്റർ കണ്ടെത്തുന്നില്ല**: USB കേബിൾ പരിശോധിക്കുക, പ്രിന്റർ ഓൺ ചെയ്യുക, `lpinfo -v | grep -i canon` ഉപയോഗിച്ച് അത് കാണുന്നുണ്ടോ എന്ന് നോക്കുക. തുടർന്ന് `./setup.sh` വീണ്ടും പ്രവർത്തിപ്പിക്കുക.
- **ശൂന്യമായ അല്ലെങ്കിൽ സ്ഥാനം മാറിയ പ്രിന്റ്**: print dialog-ൽ ശരിയായ പേപ്പർ വലുപ്പം (A4/Letter) തിരഞ്ഞെടുക്കുക. പ്രിന്ററിന് ഏകദേശം 5 mm margin വേണം.

## പതിവ് ചോദ്യങ്ങൾ (FAQ)

### Canon LBP2900 Mac-ൽ പ്രവർത്തിക്കുമോ?
അതെ. ഈ ഡ്രൈവർ ഉപയോഗിച്ച് LBP2900, LBP2900B എന്നിവ ആധുനിക macOS-ൽ, Apple Silicon-ലും Intel Mac-ലും, പ്രിന്റ് ചെയ്യും.

### M1, M2, M3, M4 Mac-ൽ Canon LBP2900 പ്രവർത്തിക്കുമോ?
അതെ. Native arm64 build ഉൾപ്പെടുത്തിയിട്ടുണ്ട്, Rosetta ഇല്ലാതെ പ്രവർത്തിക്കും.

### macOS Sonoma, Sequoia, Tahoe എന്നിവയിൽ പ്രവർത്തിക്കുമോ?
അതെ. Apple Silicon-ൽ macOS 26 (Tahoe)-ൽ പരീക്ഷിച്ചു; macOS 14-ഉം അതിനു ശേഷമുള്ളവയും ലക്ഷ്യമിടുന്നു.

### macOS-നുള്ള ഔദ്യോഗിക Canon LBP2900 ഡ്രൈവർ ഉണ്ടോ?
ഇല്ല. Canon-ന്റെ Mac CAPT ഡ്രൈവർ നിലവിലെ macOS-ലോ Apple Silicon-ലോ LBP2900 പിന്തുണയ്ക്കുന്നില്ല. ഇത് അനൗദ്യോഗികമായ, community ഡ്രൈവർ ആണ്.

### LBP2900B, LASER SHOT LBP2900, i-SENSYS LBP2900 എന്നിവ ഇതേ പ്രിന്റർ ആണോ?
അതെ. ഒരേ പ്രിന്ററിന്റെ വിവിധ രാജ്യങ്ങളിലെ പേരുകളാണ് ഇവ. ഈ ഡ്രൈവർ എല്ലാത്തിനെയും പിന്തുണയ്ക്കുന്നു.

### SIP ഓഫാക്കുകയോ kernel extension ഇൻസ്റ്റാൾ ചെയ്യുകയോ വേണോ?
വേണ്ട. ഡ്രൈവർ ഒരു സാധാരണ CUPS filter ആണ്, സാധാരണ USB backend ഉപയോഗിക്കുന്നു.

### macOS അപ്ഡേറ്റ് ചെയ്താൽ ഇത് പ്രവർത്തിക്കാതാകുമോ?
ഇല്ല. അപ്ഡേറ്റ് ഡ്രൈവർ നീക്കം ചെയ്താൽ self-healing service അത് സ്വയമേവ വീണ്ടും ഇൻസ്റ്റാൾ ചെയ്യും. [macOS അപ്ഡേറ്റുകൾക്ക് ശേഷവും പ്രവർത്തിക്കാൻ](#macos-അപ്ഡേറ്റുകൾക്ക്-ശേഷവും-പ്രവർത്തിക്കാൻ) കാണുക.

### Wi-Fi വഴിയോ network വഴിയോ പ്രവർത്തിക്കുമോ?
പ്രിന്റർ USB മാത്രമേ പിന്തുണയ്ക്കൂ. അത് കണക്ട് ചെയ്ത Mac-ൽ നിന്ന് macOS printer sharing (System Settings ▸ General ▸ Sharing ▸ Printer Sharing) ഉപയോഗിച്ച് മറ്റുള്ളവരുമായി പങ്കിടാം.

### ഇത് സൗജന്യമാണോ?
അതെ. GPLv3 പ്രകാരം സൗജന്യവും ഓപ്പൺ സോഴ്സും ആണ്.

## ഇത് എങ്ങനെ പ്രവർത്തിക്കുന്നു

CUPS filter ശൃംഖല: `app → PDF → cgpdftoraster → rastertocapt → usb backend → printer`.

`rastertocapt` ഓരോ പേജും CAPT / Hi-SCoA രീതിയിൽ compress ചെയ്ത band-കളാക്കി പ്രിന്ററിലേക്ക് അയയ്ക്കുന്നു. ഇരുദിശാ status handshake-നായി CUPS-ന്റെ സാധാരണ **back-channel** (`cupsBackChannelRead`), **side-channel** (`cupsSideChannelDoRequest`) API-കൾ ഉപയോഗിക്കുന്നു. അതുകൊണ്ട് kernel extension-ഓ ഇൻസ്റ്റാൾ സമയത്തെ `sudo`-ക്കപ്പുറം അധിക അവകാശങ്ങളോ ആവശ്യമില്ല.

### പരിഹാരം

Upstream captdriver LBP2900-നെ ഒരു *സോപാധിക* status strategy-യിലാണ് (`capt_get_xstatus` / `capt_wait_ready`) രജിസ്റ്റർ ചെയ്യുന്നത്. `XSTATUS_CHNG` flag സജ്ജമാകുമ്പോൾ മാത്രമേ അത് **വിപുലീകൃത** printer status (CAPT command `0xA0A8`) എടുക്കൂ; ഈ പ്രിന്റർ ആ flag ഒരിക്കലും സജ്ജമാക്കുന്നില്ല. പേജ് കൗണ്ടറുകൾ (`page_decoding` / `page_out` / `page_completed`) വിപുലീകൃത status record-ൽ **മാത്രമാണ്** ഉള്ളത്, അതിനാൽ അവ ഒരിക്കലും പുതുക്കിയില്ല. പഴയ മൂല്യങ്ങൾ കാരണം end-of-page കാത്തിരിപ്പ് loop-കൾ നിലച്ചു: [captdriver#3](https://github.com/mounaiban/captdriver/issues/3)-ലെ അവസാനമില്ലാത്ത `0xE0A0` status poll.

Patch LBP2900-നെ പ്രവർത്തിക്കുന്ന LBP3010 ഉപയോഗിക്കുന്ന strategy-യിലേക്ക് മാറ്റുന്നു: **എപ്പോഴും** വിപുലീകൃത status വായിക്കുക (`capt_get_xstatus_only` / `capt_wait_xready_only`). കൗണ്ടറുകൾ ശരിയായി വായിക്കുമ്പോൾ അവ ഒത്തുചേരുന്നു (`1/1/1/1`), പേജ് പ്രിന്റ് ആകുന്നു. മുകളിൽ വിവരിച്ച `PAGE:` / `STATE:` റിപ്പോർട്ടിംഗും ഇതേ patch ചേർക്കുന്നു. [`patches/lbp2900-macos.patch`](patches/lbp2900-macos.patch) കാണുക.

ഇത് reverse-engineer ചെയ്ത CAPT protocol spec-മായി ([`captdriver/SPECS`](captdriver/SPECS)) യോജിക്കുന്നു: വിപുലീകൃത status record (`0xA0A8`-നുള്ള മറുപടി) byte 14–21-ൽ പേജ് കൗണ്ടറുകൾ വഹിക്കുന്നു, പേപ്പർ തീർന്നത് STATUS0 bit 1 / STATUS1 bit 14 ആണ്.

## Source-ൽ നിന്ന് build ചെയ്യൽ

Prebuilt arm64 filter `prebuilt/arm64/`-ൽ ഉണ്ട്. വീണ്ടും build ചെയ്യാൻ (Intel-നോ universal binary-ക്കോ) [`docs/BUILD.md`](docs/BUILD.md) കാണുക. ചുരുക്കത്തിൽ:

```bash
cd captdriver
autoreconf -fi && ./configure
make CFLAGS="-D_DARWIN_C_SOURCE -std=gnu99 -O2"     # -> src/rastertocapt
```

ഇവിടെയുള്ള `captdriver/` upstream captdriver ആണ്, **macOS/LBP2900 patch ഇതിനകം പ്രയോഗിച്ചത്**. ഒറ്റ കമാൻഡിൽ build ചെയ്യാനും GPLv3 source നിബന്ധനകൾ പാലിക്കാനുമാണ് ഇത് ഉൾപ്പെടുത്തിയത്.

## അൺഇൻസ്റ്റാൾ

```bash
sudo ./uninstall.sh
cd menubar && ./uninstall-menubar.sh
```

## ഭാവി പദ്ധതികൾ

- പൊതുവായ നിശ്ചലതയ്ക്ക് പകരം വ്യക്തമായ പിശകുകൾ കാണിക്കാൻ cover-open / paper-jam status bit-കൾ decode ചെയ്യുക (spec പ്രകാരം STATUS2 bit 7 = "Problem").
- Universal (arm64 + x86_64) prebuilt binary.

## കടപ്പാട്

- [captdriver](https://github.com/mounaiban/captdriver), Moses Chong-ഉം സംഭാവകരും: ഇത് നിർമ്മിച്ചിരിക്കുന്ന CAPT CUPS filter.
- യഥാർത്ഥ CAPT reverse engineering: **Alexey Galakhov**, Nicolas Boichat, Benoit Bolsee തുടങ്ങിയവർ ([`captdriver/AUTHORS`](captdriver/AUTHORS), [`captdriver/SPECS`](captdriver/SPECS) കാണുക).
- macOS port, LBP2900 status-strategy പരിഹാരം, native status റിപ്പോർട്ടിംഗ്: ഈ repository.

ഇത് അനൗദ്യോഗിക സോഫ്റ്റ്‌വെയർ ആണ്, Canon Inc.-ന്റെ അംഗീകാരമോ ബന്ധമോ ഇല്ല. "Canon", "LASER SHOT", "i-SENSYS" എന്നിവ Canon Inc.-ന്റെ trademark-കളാണ്.

## ലൈസൻസ്

**GNU General Public License v3**: [`LICENSE`](LICENSE) കാണുക. captdriver GPLv3 ആണ്, ഈ derivative work-ഉം (patch-കളും build ചെയ്ത binary-യും) അതേ നിബന്ധനകൾ പ്രകാരം വിതരണം ചെയ്യുന്നു.

---

*Keywords: Canon LBP2900 Mac ഡ്രൈവർ, Canon LBP2900B macOS ഡ്രൈവർ, MacBook-നുള്ള Canon 2900 ഡ്രൈവർ, LBP2900 Apple Silicon, LBP2900 M1 / M2 / M3 / M4, Mac-ൽ Canon LBP2900 എങ്ങനെ ഇൻസ്റ്റാൾ ചെയ്യാം, Canon CAPT driver Mac, LBP 2900 printer not working on Mac, Canon LBP2900 macOS Sonoma / Sequoia / Tahoe.*
