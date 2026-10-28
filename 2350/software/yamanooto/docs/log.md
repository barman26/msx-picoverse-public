# PicoVerse 2350 Yamanooto v1.19

- Version bumped to v1.19 (top-level and tool Makefiles, and the tool's fallback `APP_VERSION`).
- Moved the RP2350 firmware build from Pico SDK 2.2.0 to Pico SDK 2.3.1 (including its bundled TinyUSB), from the Arm GNU Toolchain 14.2.Rel1 to 15.2.Rel1 (GCC 15.2.1) and from picotool 2.2.0-a4 to 2.3.1. `pico/yamanooto/CMakeLists.txt` now sets `sdkVersion 2.3.1`, `toolchainVersion 15_2_Rel1` and `picotoolVersion 2.3.1`, and `pico/yamanooto/pico_extras_import.cmake` fetches pico-extras at tag `sdk-2.3.1` (it was `sdk-2.1.1`) so the audio library matches the SDK. The SDK, toolchain, picotool and pioasm are expected under `~/.pico-sdk/sdk/2.3.1`, `~/.pico-sdk/toolchain/15_2_Rel1`, `~/.pico-sdk/picotool/2.3.1` and `~/.pico-sdk/tools/2.3.1`. The old `pico/yamanooto/build` directory must be removed once so CMake reconfigures and refetches pico-extras (`make clean` does this).
- Clean `make all` build succeeds with no source changes; the firmware went from 65248 to 65872 bytes. The embedded firmware header and `tool/dist/yamanooto.exe` were regenerated, and the rebuilt tool writes both plain and `-f` (FM-PAC) UF2 images. Not yet tested on hardware; the MSX bus, expanded-slot/FM-PAC, flash-save and audio paths need a check on a real MSX with the new SDK and compiler (including the Pampas & Selene boot and the Aleste Collection) before release.

# PicoVerse 2350 Yamanooto v1.18

- Made the FM-PAC / expanded slot optional and OFF by default. The cartridge is now presented as a plain, non-expanded slot (exactly like the real Yamanooto hardware and openMSX) unless the FM-PAC is explicitly requested, restoring compatibility with games that mis-handle an expanded cartridge slot (e.g. Aleste Gaiden in the Aleste Collection, which was showing "Syntax error in 0").
- Firmware: added `yamanooto_run_simple()`, a plain-slot loop with no secondary-slot register (`0xFFFF`) and no FM-PAC sub-slot; `main()` now dispatches on the config-record `type` byte — `YAMA_MSX_MUSIC_FLAG` (`0x20`) set runs the expanded FM-PAC loop, clear runs the plain loop. No expanded-slot cold-boot bootstrap is used in plain mode.
- Tool: added the `-f` / `--fmpac` option that sets `YAMANOOTO_MSX_MUSIC_FLAG` in the config-record `type` byte to enable the expanded slot + FM-PAC BIOS for the image; without it the image is a plain slot. Updated the usage text and the build summary (Audio/Slot lines). The FM-PAC BIOS is still embedded/appended so a flag-only change can toggle it.
- Diagnosis (openMSX MCP + ROM disassembly): the Aleste Collection uses pure software banking, and openMSX runs the cartridge as a simple slot (`EXPTBL` bit clear). Only Aleste Gaiden relocates itself into RAM using heavy expanded-slot management (`RDSLT`/`ENASLT`/`CALSLT`, direct `0xFFFF` writes, `EXPTBL` scans), so our PicoVerse-added FM-PAC expansion broke its slot logic while the other games were unaffected. Added `devtools/z80dis.py` (minimal Z80 disassembler) used for the analysis.
- Version bumped to v1.18 (top-level and tool Makefiles). Firmware and tool verified building; both plain (`type=0x01`) and FM-PAC (`type=0x21`) UF2 images verified.

# PicoVerse 2350 Yamanooto v1.17

- Completed the persistent S29GL064N flash-save emulation behind `ENAR.WREN`: AMD single-byte and 32-byte buffered programming, sector/chip erase, CFI query, autoselect IDs, and reset-to-read mode are supported.
- Stores mutable save pages in PSRAM and journals them in unused board flash with safe multicore flash writes; repeated saves compact to their latest changed pages when the journal fills.
- Preserved reliable Pampas & Selene boot behavior by keeping all unmodified ROM reads on the original XIP image. PSRAM is a lazy overlay used only for restored, programmed, or erased flash pages; the expanded-slot bootstrap remains available immediately at power-on.
- Flash operations complete synchronously, so polling code observes the completed byte without stalling the MSX bus. The journal is intended for save-state workloads, not a full 8 MB on-MSX cartridge rewrite.
- Updated the Yamanooto implementation guide and tool manual for the full game-save command interface and persistence boundary.
- Version bumped to v1.17 (top-level and tool Makefiles).

# PicoVerse 2350 Yamanooto v1.16

- Added persistent S29GL064 flash-save emulation: the 8 MB cartridge image is now mutable in external PSRAM and modified pages/sectors are replayed from a journal in the unused upper board flash on boot.
- Implemented the AMD program, sector-erase, reset and autoselect command path gated by `ENAR.WREN`, including Pampas & Selene's observed `0x4AAA` / `0x4555` x16 unlock sequence. Normal mapper, SCC and FM writes remain unchanged while `WREN` is clear.
- Added Pico SDK flash-safe multicore coordination so save-journal writes never execute from the XIP flash being programmed.
- Updated the Yamanooto implementation guide and tool manual with the persistent game-save behavior and the remaining bulk-reflash limitation.
- Version bumped to v1.16 (top-level and tool Makefiles). Firmware and tool verified building.

# PicoVerse 2350 Yamanooto v1.15

- Added Explorer-style USB CDC stdio debug support for Yamanooto and trace points for boot, subslot selection, Yamanooto registers, SCC raw-bank/mode activation, SCC register writes, FM writes, and audio engine switching.
- Added `utils/capture-yamanooto-debug.ps1` to capture the USB CDC trace into timestamped log files for SCC/FMPAC diagnosis.
- Fixed SCC audio gating found from USB CDC logs: SCC rendering now latches after SCC register writes and is no longer disabled when the game pages bank 2 away from `0x3F` after the write; FM writes still take ownership for FM-PAC games.
- Disabled USB CDC stdio debug for the production build while keeping the compile-time debug switch and capture helper available for future diagnostics.
- Version bumped to v1.15 (top-level and tool Makefiles). Firmware and tool verified building.

# PicoVerse 2350 Yamanooto v1.14

- Fixed SCC/SCC+ playback selection in Yamanooto images with always-on FM-PAC by switching the audio engine from recent SCC register writes instead of letting an idle/keyed OPLL keep priority.
- Matched SCC/SCC+ register read/write routing to Yamanooto raw-bank visibility rules, including mirrored SCC register windows before offset-adjusted flash bank mapping.
- Kept SCC rendering active whenever Yamanooto's raw SCC/SCC+ enable state is active, matching Explorer's SCC mapper behaviour instead of timing out on quiet/sustained SCC passages.
- Added an explicit volatile SCC render request from the mapper write path so core 1 renders SCC as soon as core 0 enables or writes the SCC chip.
- Moved SCC/SCC+ enable and mode handling into the Yamanooto mapper layer, matching openMSX: the ROM offset only affects flash bank selection, while raw banks plus the SCC mode register decide the SCC window and `emu2212` receives mirrored SCC register bytes; CFGR changes also resync SCC render state immediately.
- Version bumped to v1.14 (top-level and tool Makefiles). Firmware and tool verified building.

# PicoVerse 2350 Yamanooto v1.13

- Removed the software PSG DC blocker to fix PSG noise under SCC (matches Explorer's clean SCC + PSG).
- Version bumped to v1.13 (top-level and tool Makefiles). Firmware and tool verified building.

# PicoVerse 2350 Yamanooto v1.12

- Applied the FM path's audible-gated PSG handling to the SCC path too, removing PSG noise under SCC.
- Version bumped to v1.12 (top-level and tool Makefiles). Firmware and tool verified building.

# PicoVerse 2350 Yamanooto v1.11

- Fixed FM noise reintroduced by v1.10's always-render PSG, using context-aware PSG mixing.
- PSG mixing is now context-aware, matching the two Explorer paths:
  * FM primary  -> audible-gated PSG (`psg_mix_gated`): idle PSGs skip `PSG_calc`, keeping core 1
    within budget; the gate transitions are masked by the FM and the DC blocker.
  * PSG / SCC primary -> full PSG (`psg_mix_sample`): both PSGs clocked and mixed every sample so
    there are no mute/unmute clicks (the pure-PSG behaviour fixed in v1.10 is preserved).
- Version bumped to v1.11 (top-level and tool Makefiles). Firmware and tool verified building.
- Added `docs/msx-picoverse-2350-yamanooto.md` (implementation guide) and
  `docs/msx-picoverse-2350-yamanooto-tool-manual.en-us.md` (PC tool manual).
- Updated `docs/msx-picoverse-public-readme.md` (highlights, 2350 section, documentation links,
  Yamanooto tool manual link, and credits for the Genami Yamanooto / openMSX Yamanooto.cc references),
  `docs/msx-picoverse-2350-features.md` (Yamanooto flash-cartridge emulation feature entry),
  and `docs/presentation/msx-cartridge-comparison.md` (Yamanooto listed as a PicoVerse 2350 exclusive).

# PicoVerse 2350 Yamanooto v1.10

- Removed the remaining mute/unmute clicking on PSG by always rendering the PSG (like Explorer).
- Version bumped to v1.10 (top-level and tool Makefiles). Firmware and tool verified building.

# PicoVerse 2350 Yamanooto v1.09

- Fixed the constant beep on pure-PSG games (was the idle OPLL being rendered).
- Version bumped to v1.09 (top-level and tool Makefiles). Firmware and tool verified building.

# PicoVerse 2350 Yamanooto v1.08

- Fixed the constant mute/unmute artifact on PSG-only games.
- The PSG is now clocked AND mixed every sample while in use, exactly like the Explorer standalone
  PSG path: its output falls to the idle level naturally during a rest, so there is no on/off step.
  A PSG is only dropped from the mix once it has been silent for the whole ~200 ms keep-alive
  window, so idle PSGs still add no buzz under FM/SCC.
- Version bumped to v1.08 (top-level and tool Makefiles). Firmware and tool verified building.

# PicoVerse 2350 Yamanooto v1.07

- Fixed PSG-mirror noise (crackle) while keeping FM and SCC clean.
- Version bumped to v1.07 (top-level and tool Makefiles). Firmware and tool verified building.

# PicoVerse 2350 Yamanooto v1.06

- Dual PSG is now only rendered when audible, added a main-PSG mirror, and reduced the residual
noise heard under FM music.
- PSG audible gating: a PSG channel is only mixed when it has a non-zero fixed volume (or a
  running envelope) AND its tone or noise is enabled in the mixer. Idle PSGs (all volumes 0 with
  every channel enabled after reset) previously added a low-level buzz that was audible under the
  quieter FM music; they now contribute nothing. This applies to both PSGs and also skips the
  `PSG_calc` work when silent. Mirrors the Explorer PSG behaviour.
- Added a PSG mirror: writes to the main MSX PSG ports (`0xA0`/`0xA1`) now drive a second PSG
  instance that is mixed (audible-gated) into the cartridge DAC, so the system PSG is heard
  through the same output as SCC/FM. Echo mode still additionally drives the secondary PSG.
- The FM + PSG mix is soft-limited (compressed) instead of hard-clipped; the SCC + PSG path keeps
  its original hard clamp.
- Version bumped to v1.06 (top-level and tool Makefiles). Firmware and tool verified building.

# PicoVerse 2350 Yamanooto v1.05

- MSX-MUSIC is now always enabled, and the SCC-vs-FM audio engine is chosen at runtime so a
single flash image can hold a mix of SCC games and FM games.
- The cartridge is always presented as the expanded slot with the FM-PAC BIOS in sub-slot 3;
  the OPLL is always created. The plain (non-expanded) run path was removed and `main()` always
  runs the FM-PAC loop.
- On-the-fly engine selection: the SCC and FM engines are never used together by one game but a
  multi-ROM image can contain both, so the audio core now picks the engine per buffer from recent
  sound-register write activity (`scc_activity` bumped on SCC register-window writes while the SCC
  is enabled; `fm_activity` bumped on OPLL writes) and renders only that engine (+ the secondary
  PSG). This keeps the core-1 per-sample budget within range while supporting both game types.
- Removed the `-f` / `--fmpac` / `--msx-music` option: the FM-PAC BIOS is always embedded and
  appended after the ROM image.
- Version bumped to v1.05 (top-level and tool Makefiles). Firmware and tool verified building.

# PicoVerse 2350 Yamanooto v1.04

- Further fix for robotic MSX-MUSIC audio: reduced the core-1 per-sample workload.
- Version bumped to v1.04 (top-level and tool Makefiles). Firmware and tool verified building.

# PicoVerse 2350 Yamanooto v1.03

- Fixed the "robotic" MSX-MUSIC audio, following the Explorer FM path.
- Version bumped to v1.03 (top-level and tool Makefiles). Firmware and tool verified building.

# PicoVerse 2350 Yamanooto v1.02

- Fixed MSX-MUSIC games not playing music: the OPLL is now presented with the **FM-PAC BIOS**
so games can detect the FM chip (previously only the raw I/O ports were emulated, which is
insufficient for BIOS-detecting games).
- Version bumped to v1.02 (top-level and tool Makefiles). Firmware and tool verified building;
  the tool packages a UF2 with the FM-PAC BIOS appended when `-f` is selected.

# PicoVerse 2350 Yamanooto v1.01

- Added optional **MSX-MUSIC** (YM2413 / OPLL) emulation, modelled on the MSX-MUSIC work in
`2350/software/explorer.pio` / `loadrom.pio`.
- Added the `emu2413` (YM2413/OPLL) engine to the build.
- When enabled, an OPLL instance is created (`OPLL_2413_TONE`, 3.579545 MHz, 44.1 kHz) and
  MSX-MUSIC I/O writes on ports `0x7C`/`0x7D` (captured on the PIO1 I/O bus) are dispatched to
  it; the FM output is mixed into the core-1 audio stream alongside the SCC and secondary PSG.
- MSX-MUSIC is off by default and activated by the `0x20` flag in the config-record `type`
  byte; the OPLL is serviced/rendered on core 1 only, so no cross-core locking is required.
- Added the `-f` / `--fmpac` / `--msx-music` flag which sets the `0x20` MSX-MUSIC bit in the
  config-record `type` byte, and reports the selection.
- Version bumped to v1.01 (top-level and tool Makefiles). Firmware and tool verified building;
  the tool packages a UF2 with and without `-f`.

# PicoVerse 2350 Yamanooto v1.0

- Initial implementation of the **Yamanooto** flash cartridge emulation for the
PicoVerse 2350, modelled on the openMSX `Yamanooto` device and reusing the SCC/SCC+ and
dual-PSG work from `2350/software/explorer.pio` / `loadrom.pio`.
- Added `yamanooto.c` / `yamanooto.h`: PIO-based emulation of the Yamanooto Konami-SCC
  compatible flash cartridge.
- Konami-SCC mapper (default) and Konami-4 mapper (CFGR `K4` bit): 10-bit bank registers
  with the global bank offset `(OFFR << 2) | (CFGR SUBOFF >> 4)`; `MDIS` disables banking.
- Memory-mapped register window at `0x7FFC-0x7FFF` (not mirrored), gated by `ENAR` `REGEN`:
  `ENAR` (`REGEN`/`WREN`), `OFFR`, `CFGR` (`MDIS`/`ECHO`/`ROMDIS`/`K4`/`SUBOFF`/`FPGA_EN`),
  and the FPGA channel (`FPGA_REG`) with the minimal read-ID handshake returning
  `{0xFF,0x1F,0x23,0x00,0x00}` and `FPGA_WAIT` always ready.
- SCC / SCC+ audio via `emu2212` (type `SCC_ENHANCED`): every mapper-region write is fed to
  the SCC engine, which auto-detects SCC (bank2 == `0x3F`), SCC+ (bank3 bit7) and the mode
  register at `0xBFFE`; SCC register reads decode `base_adr + 0x800`. Disabled in K4 mode.
- Secondary (dual) PSG via `emu2149` on I/O ports `0x10`/`0x11`, echoed to `0xA0`/`0xA1` when
  `CFGR ECHO` is set (captured on PIO1); SCC and PSG are mixed into the I2S DAC on core 1.
- ROM image served directly from flash XIP through `getFlashAddr`, guarded by the image size
  (unprogrammed reads return `0xFF`); `ROMDIS` returns `0xFF`. Address mirroring 0x4000<->0xC000
  and 0x8000<->0x0000 reproduced.
- Flash programming (`WREN` + AmdFlash command sequences) is intentionally not emulated;
  write cycles with `WREN` set are ignored so the emulated banks are never corrupted. The
  firmware runs pre-flashed images.
- Reused `emu2212`, `emu2149`, `msx_bus.pio`, and the CMake import scripts from `loadrom.pio`.
- `CMakeLists.txt` targets `pimoroni_pga2350` (RP2350B), 210 MHz, I2S on PIO0 SM2, MSX memory
  bus on PIO0 SM0/SM1 and the PSG I/O bus on PIO1 SM0/SM1.
- Added `src/yamanooto.c`: Windows console UF2 creator. Packs the embedded firmware, a 59-byte
  config record (name + type + size + offset) and the user ROM image (up to 8 MB) into a
  UF2 (RP2350 family, 256-byte payloads).
- `Makefile` regenerates the embedded firmware header (`src/yamanooto.h`) with `xxd` and builds
  `dist/yamanooto.exe`.
- Top-level `Makefile` builds the firmware (CMake/Ninja) and the tool.
- Verified: firmware links (`yamanooto.uf2`/`.bin` produced) and the tool packages a UF2 from a
  128 KB test image.
