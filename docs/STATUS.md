# Implementation status

## Verified

| Area | Evidence | Status |
| --- | --- | --- |
| NMOS 6502 legal instruction set | All 151 opcodes decode; targeted semantics and cycle tests | Green |
| Independent CPU conformance | Klaus functional image stable success trap `$3469`, 96,241,664 cycles | Green |
| Binary arithmetic | Exhaustive `256 × 256 × carry` ADC and SBC | Green |
| Valid BCD arithmetic | Exhaustive `100 × 100 × carry` ADC and SBC | Green |
| NMOS decimal flags | Visual6502 difficult ADC/SBC vectors | Green |
| Interrupt/stack quirks | IRQ/NMI, BRK/RTI, JSR/RTS, pushed status, NMOS D preservation | Green |
| Basic BBC boot | OS 1.20 + BASIC II smoke test reaches the BASIC idle loop and renders startup | Green smoke test |
| Core build | GCC 13, C++20, warnings promoted to errors | Green |
| Swift package | BeebKit tests and full package build on Apple Swift 6.2 | Green |

## Hardware fidelity

| Device | Implemented | Still required |
| --- | --- | --- |
| 6502 | Legal opcodes, flags, decimal mode, interrupts, cycles | Per-bus-cycle micro-operations; optional undocumented opcodes |
| Memory | Model B RAM, OS ROM, 16 sideways banks, I/O overlay | Sideways RAM write policy; 1 MHz bus wait-state phasing |
| 6522 | Ports/DDRs, T1/T2, IFR/IER, CA1/CB1, PB7 timer output | Shift modes, complete CA2/CB2 handshake/pulse modes, latches |
| 6845 | Register masks, raster/frame counters, display geometry | Sync widths, interlace, cursor output, exact variants and rupture effects |
| Video ULA | Serializer modes, palette and flash | Exact cursor/clock edge behaviour |
| Bitmap video | 1/2/4-bit interleave, mode-sized output, wrap | Cycle raster, mid-line changes, rigorous hardware-scroll cases |
| Mode 7 | Text, colours, flash, contiguous/separated mosaics | Double-height pairs, hold/release, conceal, precise control-code latency |
| SN76489 | Tone/noise registers and sample output | Band-limiting, exact LFSR variant confirmation, host audio queue |
| 8271 | SSD/DSD sector read/write, seek, status, special registers, NMI bytes | Full command/timing/error model, formatting, deleted sectors, flux |
| Keyboard | Matrix injection through System VIA | Complete host-to-BBC key map and all IC32 nuances |
| Cassette | — | 6850, Serial ULA, UEF chunks, WAV edge decoder, motor timing |

## Highest-value next steps

1. Run the Swift package in Xcode and fix any Apple importer/UI differences.
2. Replace instruction-end peripheral ticking with a CPU bus-cycle sequencer.
3. Add a deterministic boot test that uses a redistributable test ROM rather
   than proprietary OS/BASIC images.
4. Connect `SN76489::render` to an AVAudioEngine ring buffer.
5. Implement ACIA + Serial ULA and UEF `0x0100`, carrier and gap chunks.
6. Add debugger breakpoints, memory watchpoints, disassembly and save states to
   the public C/Swift API.
7. Compare screen address sequences and VIA timers against hardware traces or a
   transistor/logic reference before labelling the core cycle-accurate.
