# MSX PicoVerse 2040 USB FDD

The USB FDD firmware turns the PicoVerse 2040 into a **USB floppy disk interface** and a **USB flash drive disk image (DSK) interface** for MSX computers (a matching [PicoVerse 2350 version](./msx-picoverse-2350-msxusbfdd.md) is also available in `2350/software/msxusbfdd`). It is a port of the MSXUSB USB FDD BIOS (based on the Rookie Drive BIOS by Konamiman, which used a CH376S USB host chip) where the CH376S was replaced by the USB host of the RP2040 on the cartridge.

The BIOS is a complete MSX-DOS 1 disk ROM: it adds two drives (A: and B: when there is no other disk interface) that work with

- a **USB floppy drive** (UFI command set, the common 720K/1.44MB drives): read, write and FORMAT, or
- a **disk image file** (`.DSK`) stored in a USB flash drive formatted with FAT12, FAT16 or FAT32. The image is mounted as the disk of the drive, files can be browsed, created and mounted from a boot menu or with the `CALL USB*` commands.

## Connecting the drive

> **⚠️ WARNING — DO NOT CONNECT A USB FLOPPY DRIVE DIRECTLY TO THE PICOVERSE USB-C PORT!**
>
> Differently from the MSX-USB / RookieDrive, the PicoVerse 2040 cartridge **cannot supply the electrical current required to run USB floppy drives**. Connecting a USB floppy drive directly to the cartridge's USB-C port can overload the power path and **damage the cartridge**.
>
> **Always connect USB floppy drives through an externally powered (5V) USB hub** connected to the cartridge's USB-C port (via an OTG adapter or cable). Single-tier and compound 2-tier hubs are supported, and the boot screen shows `USB hub found`.

- USB flash drives (for `.DSK` images) can be connected either through the powered USB hub or via a USB-C OTG adapter.
- Only one device is used at a time: a floppy drive or a flash drive, the first one that is connected.
- Floppy drives must support the UFI command set with the CBI (with or without interrupt endpoint), CB or Bulk-Only transport, which is the case of all the common drives (validated in hardware with drives such as the **TEAC FD-05PUB** and the **TEAC USB UF000x**). Drives whose interrupt endpoint never reports the status are automatically detected and handled as CB drives. If a drive can't be queried at boot the BIOS prints a `Diag:` line followed by an `HW:` line and four lines of USB controller register words: please include them when reporting problems. Flash drives must be SCSI Bulk-Only mass storage with 512 byte sectors and a FAT12/16/32 file system. exFAT and long file names are not supported: the files are seen with their short 8.3 names.

## Building

Requirements are the same of the other PicoVerse 2040 firmwares: the Raspberry Pi Pico SDK 2.2.0 (with CMake and Ninja), GNU Make and gcc (MSYS2). The Z80 code is assembled with the included `sjasm`.

```
cd 2040/software/msxusbfdd
make
```

The top-level Makefile builds, in this order, the MSX ROM (`msx/`), the Pico firmware (`pico/msxusbfdd/`) and the PC tool (`tool/`). Individual targets are available: `make msx`, `make pico`, `make tool`. `make clean` removes the build output and `make VERSION=v1.01` sets the version shown by the BIOS and the tool.

The result is `tool/dist/msxusbfdd.exe`.

## Flashing

```
msxusbfdd.exe
```

creates `msxusbfdd.uf2` with the firmware and the 32KB BIOS ROM. Put the Pico in BOOTSEL mode and copy the UF2 file to the `RPI-RP2` drive.

Options:

| Option | Description |
| --- | --- |
| `-o <file>` | Set the UF2 output file name (default `msxusbfdd.uf2`). |
| `-r <file>` | Use another 32KB BIOS ROM instead of the embedded one (BIOS development). |
| `-h` | Show the help. |

## Using it

Power the MSX with the drive connected. The BIOS shows the device that was found:

```
PicoVerse USB FDD BIOS v1.00
...
USB FDD found: TEAC     FD-05PUB        1026
```

or `USB storage device found: ...` for a flash drive, in which case the BIOS starts its disk image boot process (default image, boot menu, boot directory...), exactly as the MSXUSB BIOS does. The keys at boot time (SHIFT, CTRL, GRAPH...), the `_USB` configuration directory and file names, the DSK formats and the FORMAT command are the ones documented for the MSXUSB BIOS and were not changed.

The BASIC `CALL` commands (`CALL USBHELP` shows them):

| Command | Description |
| --- | --- |
| `CALL USBRESET` | Initialize the device again |
| `CALL USBERROR` | Show the last ASC/ASCQ error (floppy drive only) |
| `CALL USBMENU` | Show the file navigation menu (flash drive only) |
| `CALL USBCD` / `CALL USBCD("dir")` | Show or change the current directory (flash drive only) |
| `CALL USBFILES` | List the files of the current directory (flash drive only) |
| `CALL USBMOUNT` / `CALL USBMOUNT("file.dsk")` / `CALL USBMOUNT(n)` / `CALL USBMOUNT(-1)` | Show, mount by name or number, or unmount the disk image (flash drive only) |
| `CALL USBMOUNTR(...)` | Mount and reset the computer (flash drive only) |

Pressing **CAPS + ESC** while the interface is waiting for the drive aborts the operation and resets the device (the "panic button").

Hot plugging is supported: the BIOS checks for connections and disconnections every time the disk is accessed.

## Differences with the CH376S version

- No USB host chip is needed. All the USB work (enumeration, hubs, transport of the commands) and the FAT file system access are done by the RP2040, so the MSX side only has to send commands.
- The diagnostics of the endpoint 0 transfers and the CH376 specific messages were removed (a device is enumerated by TinyUSB, not by the Z80).
- The "alternative ports" option (two interfaces in the same computer) does not exist, the interface does not use I/O ports.
- Long file names are not shown (the CH376S neither did). Directories and files with names that begin with `_` are hidden from the menus, as before.

## How it works (developer notes)

### Memory map

The MSX sees a 32KB ROM at 4000h-7FFFh mapped with the Konami SCC mapper: the bank register at 5000h selects the 8KB bank at 4000h-5FFFh and the register at 7000h the bank at 6000h-7FFFh. The two 16KB halves of the ROM image (bank 0: kernel; bank 1: driver) are selected with `CALL_BANK` (7FD2h), which is the same code at the same address in both banks.

The firmware overlays the **mailbox** on 7D00h-7FCFh, an area that is free in both banks (the BIOS checks at build time that the code of each bank ends before 7D00h):

| Address | Access | Description |
| --- | --- | --- |
| 7D00h-7DFFh | R/W | Data window: 256 bytes of the 8KB data buffer, selected by the page register |
| 7E00h | R | Status: bit 7 = BUSY |
| 7E01h | R | Result of the last command (`USB_ERR_*` or file result) |
| 7E02h | R/W | Page of the data window (0-31) |
| 7E03h | W | Command: writing it starts the execution (FFh = abort the running command) |
| 7E04h-7E13h | R/W | 16 argument bytes |
| 7E14h-7E23h | R | 16 response bytes |
| 7E30h-7E33h | R | Signature `PVFD` |
| 7E34h | R | Protocol version (1) |

The MSX writes the arguments and the data, writes the command and polls the status until BUSY is clear. The commands are listed in `msx/src/pico_regs.asm` and `pico/msxusbfdd/mailbox.h`:

- `RESET`, `DEV_CHANGE`, `DEV_INFO`, `BUS_RESET`, `LONG_TIMEOUT`: device detection and recovery.
- `UFI_EXEC`: execute a 12 byte UFI command with an optional data stage of up to 8KB (the 16 sectors that DSKIO transfers at once). The firmware returns the amount of data, ASC and ASCQ (it sends REQUEST SENSE if the command failed).
- `FS_*`: mount, open file or directory, create file or directory, close, delete, enumerate (raw 32 byte directory entries, with patterns), seek, read, write and attributes. They keep the behavior of the CH376 file commands that the BIOS was written for.

### RP2040 USB host transport details

- **High-rate `epx` bulk engine (`ufi_bulk_epx`):** TinyUSB's default RP2040 host driver places Bulk endpoints in the controller's periodic `INT_EP` slots, which only poll once per 1 ms SOF frame and do not retry on `NAK` until the next frame. While drives with a full-track buffer (such as the TEAC FD-05PUB) tolerate 1 ms polling, streaming drives without a track buffer (such as the TEAC UF000x, which streams from the FDC at 64 bytes per 1.024 ms) overflow their internal FDC FIFO if a `NAK` waits a full 1 ms. The PicoVerse UFI driver executes Bulk IN and Bulk OUT stages directly on the RP2040's main asynchronous `epx` engine in SRAM, retrying `NAK` every 16 µs and transferring multiple 64-byte packets per 1 ms frame.
- **Per-device `EP0` max packet size and 2-tier hubs:** `bMaxPacketSize0` is tracked per USB address (for example 8 bytes on hubs and the TEAC FD-05PUB vs. 64 bytes on the TEAC UF000x) and restored before every control stage, with up to two cascaded hub tiers (`CFG_TUH_HUB = 2`) supported.

### Source layout

| Folder | Content |
| --- | --- |
| `msx/` | MSX BIOS: `src/` (assembly), `tools/sjasm.exe`, `dist/msxusbfdd.rom`, its own Makefile |
| `pico/msxusbfdd/` | RP2040 firmware: `msxusbfdd.c` (bus loop, mailbox), `usb_host.c` (TinyUSB host, UFI class driver, device commands), `fs_service.c` (file commands), `diskio_usb.c` (FatFs on top of the TinyUSB MSC driver), `fatfs/` (FatFs R0.15, ChaN, see `fatfs/LICENSE.txt`) |
| `tool/` | PC tool that embeds the firmware and the ROM and writes the UF2 file |
| `tests/` | Host tests |

### Tests

```
cd 2040/software/msxusbfdd
make test
```

`make test` runs both tests below (`make -C tests test` or `make -C tests z80test` run them separately):

- the file service (`fs_service.c`) on FAT12, FAT16 and FAT32 RAM disks (needs gcc).
- the real BIOS ROM in a Z80 emulator (`pip install z80`, Python) against the real file service and an emulated mailbox and floppy drive.

## Credits

- **Original Rookie Drive USB FDD BIOS / USBHOST NestorBIOS 2.2:** Konamiman (Néstor Soriano), 2018–2022.
- **MSXUSB adaptation and PicoVerse 2040 implementation (RP2040 PIO bus, mailbox architecture, UFI/MSC USB host stack, FatFs integration, and UF2 builder tool):** Cristiano Goncalves (The Retro Hacker), 2026.
- **FatFs — Generic FAT Filesystem Module (R0.15):** ChaN.

