# ``BeebKit``

Own and drive a deterministic BBC Model B core from Swift.

## Overview

``BeebMachine`` is the supported Swift entry point. It owns one emulator core,
submits every operation to the C++ runtime owner, validates host data before
crossing the C ABI, and maps successful outputs into Swift-owned values. The
runtime FIFO is safe across Swift concurrency domains; `BeebMachine` adds no
second lock or mirrored lifecycle state. A machine needs a lawful 16 KiB
operating-system ROM before it can execute useful BBC software.

Use ``BeebMachine/loadOSROM(_:)`` to install that ROM, then advance emulated time
with ``BeebMachine/run(cycles:)`` or ``BeebMachine/runToNextFrame(maximumCycles:)``.
For sustained execution, use ``BeebMachine/start()`` and
``BeebMachine/pause()`` and observe ``BeebMachine/state``. Read
``BeebMachine/cpuState()`` for a value snapshot and
``BeebMachine/videoFrame()`` for a stable RGBA copy. All fallible operations
throw ``BeebError``; a C failure becomes
``BeebError/coreStatus(_:_:)`` without losing its category or diagnostic.

Reset is the recovery path from ``BeebRuntimeState/faulted``. Fault detail stays
available through ``BeebMachine/fault()`` until reset succeeds. Concurrent
tasks must retain the machine through their calls; after the final strong
reference is released, deinitialization blocks while the runtime drains and
joins.

The generated documentation landing page also links the C/C++ reference and the
project's architecture, timing, host-boundary, and evidence guides.

## Topics

### Machine lifecycle

- ``BeebMachine``
- ``BeebVersion``
- ``BeebRuntimeState``

### Values

- ``BeebCPUState``
- ``BeebVideoFrame``
- ``BeebSafePoint``
- ``BeebRuntimeFault``

### Errors

- ``BeebError``
- ``BeebStatusCategory``
