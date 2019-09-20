# GnGeo
Neo Geo Emulator for M68K Amiga Computers.

This is a deep port of the GNGEO emulator for the M68K platform suitable for 68060 and Vampire equiopped Amiga computers. Due to substantial deviations from the original Amiga-GnGeo fork, I've de-forked the whole repository.

Done:
- removed requirement with ixemul; ixemul made a lot of tasks in GNGEO slower than they needed to be (especially with file handling).
- removed all traces of AHI; the audio engine presently uses Paula directly to make 14-bit audio
- change start-up to use Amiga ReadArgs
- replaced keyboard with lowlevel.library calls
- CD32 gamepad handled via lowlevel.library
- file requester when no game name is given
- convert ROM images from ZIP to LZ4W
- removed sin and exp tables from audio
- compressed EXE to bring it under 200KB
- rewrite fixed and tile rendering to use M68K and AMMX code
- supports audio at different sample depth (8/14-bit) and 16-bit on Vampire "Pamela" sound
- args may now come from gngeo.config globally, the name.info file or via command line
- frameskip simplified to half-rate (30/25Hz for NTSC/PAL) mode
- removed interleaved mode as we're able to hit 50Hz with just frameskip
- added M68K emulation detuning (80-90% seems to improve performance nicely)
- made gamepad support option and added non-CD32 3-button support

To-do:
- fix vsync and triple buffer timing to SAGA or AGA (vsync should lock updates to the retrace and should have nothing to do with the speed of the game and/or whether or not frame-skip is enabled).
- reimplement user-configurable key mapping
- rewrite the audio engine with M68K and (optional) AMMX code
- reimplement save states using ~~LZ4W~~ raw blobs (LZ4W is too slow at compressing)

Maybe To-do:
- replace M68K emulator ~~with Musashi (presently does not work) and maybe eventually~~ with assembly code. It should be possible to contain the entire assembler state in the address registers and leave all the data registers as well as MOST of the flags to be used directly. For example, the instruction move.l d0,d1 would be emulated exactly as move.l d0,d1.
- replace Z80 with assembler version. it should be possible to exceed the Z80 on cycle-to-cycle performance (e.g., if an instruction took eight cycles on a Z80, we should be able to do it in fewer -- at least on the 68060/68080).
- ~~kill the operating system and poke hardware directly; this would be done more for simplicity than for performance. AmigaOS makes somethings (like triple buffering) more difficult than it needs to be.~~ this doesn't work that well and doesn't gain any useful performance.
- implement AGA HAM6 C2P mode for 68060 users without RTG (might be too slow).
- ~~use Amiga sprites as the NEOGEO "fixed" layer to avoid some work (might help speed it up).~~ Fix layer has too many colours -- meh
