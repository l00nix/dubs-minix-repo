# MINIX on the HP 200LX — PCMCIA and BIOS INT13 Services

**By Dr. Richard L. Dubs (1998)**

*Archived and preserved for the retrocomputing community*

## Summary

This repository preserves the PCMCIA activation and BIOS INT13 Hard Disk Services developed by Dr. Richard L. Dubs in 1998 for the HP 200LX palmtop computer. These tools allow the HP 200LX to boot and run alternative operating systems from a PCMCIA ATA flash disk.

The original software was hosted at `www.erols.com/rld` and is no longer available online. An archived version of the original web page can be found at the [Wayback Machine](https://web.archive.org/web/19990220003758/http://www.erols.com/rld/).

> *"I have developed PCMCIA and BIOS INT13 Hard Disk Services for the HP200LX that allow someone to boot MINIX from a PCMCIA ATA flash disk. [...] The PCMCIA and BIOS services I have developed should be just as useful to boot and run LINUX-86 (ELKS) on the 200LX."*
> — Dr. Richard L. Dubs, 1998

## Background

The HP 200LX is a palmtop computer with an 80186 processor, CGA-compatible greyscale LCD, and a PCMCIA Type II slot. It boots MS-DOS 5.0 from ROM. Because it boots from ROM rather than from disk, the 200LX does not provide the standard BIOS INT13 service for reading from a hard disk.

Dr. Dubs solved this problem by:

1. Writing **CARDIO.EXE** to activate the PCMCIA card in I/O mode using the 200LX's built-in Socket Services 1.0
2. Writing **INT13.BIN**, a software INT13 handler that provides Read, Write, Reset, and Drive Parameters services via standard ATA I/O
3. Providing a suite of tools to install the handler and boot an operating system from the PCMCIA card

As Dr. Dubs noted, while the HP 200LX's PCMCIA controller is proprietary, Socket Services 1.0 is built into the 200LX ROM and a primitive Card Services is provided as well, shielding the programmer from the proprietary nature of the hardware.

## Repository Contents

### Tools (`tools/`)

| File | Size | Description |
|------|------|-------------|
| `CARDIO.EXE` | 10,351 bytes | Activates PCMCIA ATA card in I/O mode on the HP 200LX |
| `INT13.BIN` | 1,160 bytes | BIOS INT13 hard disk service handler |
| `PUT13.EXE` | 14,707 bytes | Loads INT13.BIN into memory at segment 0x9000 |
| `VECT13.DAT` | 30 bytes | DOS DEBUG script that sets the INT13 interrupt vector to point to 9000:0000 |
| `WINI200.EXE` | 19,850 bytes | Reads a boot sector from a specified partition on the hard disk and executes it |
| `BOOT.BAT` | 167 bytes | Master boot script that installs INT13 and launches the OS |
| `BIOS256.EXE` | 14,879 bytes | Reads and displays the first 256 bytes of any sector (by cylinder, head, sector) |
| `BIOS512.EXE` | 14,879 bytes | Reads and displays a full 512-byte sector |
| `FINDBT2.EXE` | 12,756 bytes | Searches all sectors on the disk for a user-specified byte pattern |
| `NEWBOOT.EXE` | 15,463 bytes | Reads a file and writes it to a specified sector on the flash disk |
| `POKE001.EXE` | 10,796 bytes | Writes a bootable primary partition table to sector (0,0,1) |
| `POKE004.EXE` | 10,735 bytes | Reads a sector, modifies specific bytes, and rewrites it |

### Source Code (`src/`)

| File | Description |
|------|-------------|
| `WINI200.C` | C source code for the hard disk boot sector loader (Borland Turbo C++) |

*Note: Dr. Dubs stated that source code was provided for all tools. If you have the additional source files, please contribute them.*

### Documentation (`docs/`)

| File | Description |
|------|-------------|
| `MINIX.TXT` | Dr. Dubs' complete write-up, including technical details on PCMCIA, INT13, the boot process, and instructions for creating a bootable MINIX filesystem |

### License

| File | Description |
|------|-------------|
| `GNU.DOC` | GNU General Public License |

## Boot Sequence

To boot an alternative OS from a PCMCIA ATA card:

```
1. Boot the HP 200LX to DOS (automatic from ROM)
2. Insert the PCMCIA card
3. Run CARDIO.EXE             → Activates PCMCIA card in I/O mode
4. Run BOOT.BAT               → Installs INT13 and boots the OS:
   a. DEBUG < VECT13.DAT      → Points INT13 vector to 9000:0000
   b. PUT13                   → Loads INT13.BIN to segment 9000
   c. WINI200 1               → Loads partition 1 boot sector, executes it
```

Dr. Dubs recommended booting the 200LX into DOS without the PCMCIA card inserted (to avoid a long boot delay), then turning off the palmtop, inserting the card, and turning it back on.

## INT13 Handler Details

The INT13 handler (`INT13.BIN`) provides four BIOS disk services:

| Function | AH Value | Description |
|----------|----------|-------------|
| Reset | 0x00 | Acknowledges and returns |
| Read Sectors | 0x02 | Reads sectors via ATA PIO from port 0x1F0 |
| Write Sectors | 0x03 | Writes sectors via ATA PIO to port 0x1F0 |
| Drive Parameters | 0x08 | Issues ATA IDENTIFY DEVICE and returns CHS geometry |

The handler auto-detects disk geometry from the card via ATA IDENTIFY DEVICE. It uses standard ATA I/O ports (0x1F0–0x1F7) as configured by CARDIO.EXE.

### VECT13.DAT Explained

The file is a DOS DEBUG script:

```
e 0000:004c 00 00 00 90
q
```

This writes four bytes to the Interrupt Vector Table at address 0000:004C (the INT13 vector), setting it to segment 0x9000, offset 0x0000.

## BOOT.BAT Contents

```batch
echo "installing int13 interrupt vector ..."
debug < vect13.dat
echo "installing bios int13 service ..."
put13
echo "booting from PCMCIA card ..."
wini200 1
```

## Tool Usage Notes

### BIOS256 / BIOS512

Type a cylinder, head, and sector number (spaces between them) and the program will print the requested sector from the PCMCIA card. Press a key to get the next sector and use Ctrl-C to exit.

### FINDBT2

Searches all sectors on the PCMCIA card for a sector that starts with a user-specified set of bytes. Useful for locating boot code within a filesystem. The computer may need to be rebooted after running this program.

### POKE001 / POKE004

**WARNING:** These programs write to the PCMCIA card. If not used properly, they can write to a hard disk and ruin existing sectors and partitions. Make sure you clearly understand the source code (especially the BIOSDISK calls) before using these programs. Running on the palmtop protects against accidentally ruining hard disks on other computers.

### WINI200

Takes a partition number (1–4) as a command line argument. Reads the MBR from the disk, locates the specified partition, loads its boot sector to 0000:7C00, and jumps to it. The `-d` flag displays partition table information without booting.

## Hardware Requirements

- HP 200LX palmtop computer (any RAM configuration)
- PCMCIA ATA flash disk or CompactFlash card with PCMCIA adapter
- The card must allow boot sector (0,0,1) write access

### Known Card Compatibility

- **SimpleTech 20MB ATA Flash Disk** — works (Dr. Dubs' original testing)
- **IBM/SanDisk 10MB** — does not allow master boot sector access (unsuitable)

## MINIX Boot Status (1998)

Dr. Dubs was able to boot MINIX to a login prompt on the HP 200LX, though it was not stable — it would crash after one or two commands. He identified several possible causes:

- Memory layout changes required to accommodate the 200LX's ROM BIOS
- Special HP 200LX functions associated with IRQ2
- Possible spurious signals from the PCMCIA card

He expressed hope that the Internet community would help finish the job of getting a stable Unix-like OS running on the HP 200LX.

## Historical Note

This work was done in 1998 and originally distributed via Dr. Dubs' personal web page at `www.erols.com/rld`. The tools were compiled with Borland Turbo C++ 1.0, which runs natively on the HP 200LX. This repository preserves his contribution for future retrocomputing enthusiasts.

> *"I hope someone takes an interest in this project and figures out how to achieve 'UNIX in your pocket'."*
> — Dr. Richard L. Dubs, 1998

## References

1. *Operating Systems: Design and Implementation*, Second Edition, Andrew S. Tanenbaum and Albert S. Woodhull, Prentice Hall 1998.
2. *The HP100LX/HP200LX Developer's Guide*, Hewlett Packard, October 1994.
3. *PCMCIA Primer*, Larry Levine, M&T Books, 1995.
4. *PCMCIA Programming*, Dana L. Beatty, Steven M. Kipisz, and Brian E. Moore, Peer-to-Peer Communications, 1996.

## Credits

**Dr. Richard L. Dubs** — Original author of all tools and documentation (1998)
**C.M.T. Software Utilities Inc.** — Original Hard Disk Booter (WINI200) base code (1995)

Contact (1998): rld@erols.com
