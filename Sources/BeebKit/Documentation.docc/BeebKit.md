# ``BeebKit``

Own and drive a deterministic BBC Model B core from Swift.

## Overview

``BeebMachine`` is the supported Swift entry point. It owns one emulator core,
serializes access to that core, validates host data before crossing the C ABI,
and copies borrowed C buffers into Swift-owned values. A machine needs a lawful
16 KiB operating-system ROM before it can execute useful BBC software.

Use ``BeebMachine/loadOSROM(_:)`` to install that ROM, then advance emulated time
with ``BeebMachine/run(cycles:)`` or ``BeebMachine/runToNextFrame(maximumCycles:)``.
Read ``BeebMachine/cpuState`` for a value snapshot and
``BeebMachine/videoFrame()`` for a stable RGBA copy.

The generated documentation landing page also links the C/C++ reference and the
project's architecture, timing, host-boundary, and evidence guides.

## Topics

### Machine lifecycle

- ``BeebMachine``
- ``BeebVersion``

### Values

- ``BeebCPUState``
- ``BeebVideoFrame``

### Errors

- ``BeebError``
