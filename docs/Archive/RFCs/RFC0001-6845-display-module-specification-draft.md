# 6845+ Display Module — Spec & Skeleton (v1.1) RFC

**Project:** BBC Model B (iOS/macOS)  
**RFC:** 0001  
**Author:** Peter + GPT-5 Thinking  
**Date:** 2025‑11‑05  
**Version:** v1.1  
**Status:** Approved  
**Approved By:** BMad  
**Approval Date:** 2025-11-05  
**Discussion:** RFC0002  
**Scope:** Faithful 6845 (CRTC) timing + modern host presentation (Metal). No PAL field simulation by default; keep emulation timing correct and let the host repeat frames.

---

## 0) Why this exists (recap of our discussion)
- Original Beeb (UK PAL) outputs **50 Hz fields** but content is **non‑interlaced**: the same 312‑ish lines repeated each field → effectively **50 Hz progressive** within PAL.
- We do **not** need to emulate PAL fields. We emulate **CRTC timing (20 ms per frame)** and produce a **256‑line progressive texture** (modes 0–6; Mode 7 mapped), then let the host (60/120 Hz) repeat frames as needed.
- Exposes an authentic raster buffer (256 visible lines for UK modes; optional NTSC 200-line variant).
- Lets the host decide refresh (60/120 Hz, etc.) while repeating frames between emulated updates.
- Avoids simulating PAL interlace fields unless explicitly requested
- Faithful where it matters: 6845 timing and vblank cadence match the Beeb; software timing is correct.
- Pragmatic where it helps: no field emulation, no forced sync to 50 Hz; we respect the host display’s cadence.
- Clean interfaces: you can unit-test 6845 and RasterBuilder without Metal; swap presenters (e.g., CAMetalLayer, offscreen, video export).

**Goals:** software behaves authentically (timers, vblank cadence, raster semantics) while the renderer exploits modern display refresh without hacks.

---

## 1) Design goals
- **Timing correctness:** 50.000 Hz PAL / 60.000 Hz NTSC emulated cadence; accurate HSYNC/VSYNC positions, char rows, scanlines.
- **Deterministic fixed‑step:** one `tickFrame()` advances exactly one emulated frame (20 ms PAL, 16.666… ms NTSC).
- **Low latency, no jitter:** stable pacing to host; no per-frame logging in hot paths.
- **Separation of concerns:**
  - `CRTC6845` = counters + registers + sync; no pixels.
  - `RasterBuilder` = pixels from ULA/RAM using CRTC addresses.
  - `HostPresenter` = Metal upload + repeat on host vsync.
- **No field emulation:** progressive 256‑line output by default (interlace optional/off).
- **Introspectable:** signposts; HUD metrics; zero console spam in Release.

---

## 2) High‑level architecture

### Option 1
```
CPU/VIA/ULA ──► CRTC6845 (pure timing) ──► RasterBuilder (pixels) ──► FrameBuffer (MTLTexture)
                                                        │                           │
                                                        └── emu RAM access          └── HostPresenter (Metal)
```

### Option 2
```
CPU & ULA (emulated) ──► 6845Core  ──► RasterBuilder ──► FrameBuffer (texture)
                               │             │                   │
                               │             └─► Mode rules      └─► HostPresenter
                               │
                               └─► CRTC signals (HSYNC, VSYNC, MA/RA counters)
```

- **6845Core:** pure timing+address-gen model (registers R0–R17; counters; sync).
- **RasterBuilder:** consumes CRTC addresses & mode info, fetches pixels from emulated RAM/ULA, builds a 1-frame pixel buffer.
- **HostPresenter:** uploads the last completed buffer to Metal; repeats between emulated updates to match host vsync.

---

## 3) Timing model
- **PAL:** Δt = **20.000 ms** per emulated frame (historically a field); 256 visible lines within ~312 total.
- **NTSC:** Δt = **16.6667 ms**; ~200 visible lines; teletext 20 rows.
- Interlace bit (R8) **off** by default; optional later.

**Register set (supported):** R0–R7, R8 (modes 0/1/3), R9, R12–R17.

**Mid‑frame writes:** Phase‑1 applies changes from the **next frame** (simple, deterministic). Phase‑2 option: per‑scanline apply.

### 3.1 Fixed timestep
- PAL: Δt = 20.000 ms per emulated frame (i.e., one “field” in Beeb terms; we treat it as a full progressive frame).
- NTSC: Δt = 16.666 ms (60 Hz).
- Each tick advances 6845 counters from top border → active → bottom border → VSYNC, then emits VBlank event.

### 3.2 CRTC registers (supported)
- Horizontal: R0 (HTotal), R1 (HDisplay), R2 (HSyncPos), R3 (SyncWidth)
- Vertical: R4 (VTotal), R5 (VTotalAdjust), R6 (VDisplay), R7 (VSyncPos), R9 (MaxScanline)
- Interlace: R8 (modes 0/1/3 supported; default 0 = non-interlaced)
- Addressing: R12–R13 (Start Addr), R14–R15 (Cursor Addr), R16–R17 (Light Pen)
- Writes are cycle-accurate within the tick boundary: writes before BeginFrame affect the frame; writes mid-frame affect subsequent scanlines (optional advanced path).

### 3.3 Visible geometry (defaults)
- PAL/UK modes (0–6): 256 visible lines (32 rows × 8 scanlines), borders variable (total ~312 lines).
- Mode 7: logical 25×40 rows; rendered as 256-line progressive texture (ULA/teletext path can map rows onto scanlines).
- NTSC option: ~200 visible lines (20 Mode 7 rows).

---

## 4) Host presentation
- Host decides refresh (60/120 Hz). Presenter repeats last completed emu frame until a new one is ready.
- Cadence hints: 50→60 Hz = **5:6** repeat; 50→120 Hz = **12:5** mapping.
- Triple‑buffer pool avoids GPU stalls. No sleeps on the render thread.
- Decoupled refresh: host (Metal) vsyncs at 60/120 Hz; HostPresenter repeats last completed emulated frame until a new one is ready.
- Frame pacing:
    - If host = 60 Hz and emu = 50 Hz → 5:6 repeat (every 6 host frames, show 5 emu frames).
    - If host = 120 Hz and emu = 50 Hz → 12:5 mapping (very smooth).
- No forced sleeps in render thread. The emulation clock drives updates; presenter just shows the newest texture.
- Latency control: optional “present on next vsync after upload” flag; optional triple-buffered texture pool to avoid stalls.

---

## 5) Public API (Swift) (suggestions)

```swift
public enum VideoStandard { case pal50, ntsc60 }

public struct CRTCConfig {
    public var standard: VideoStandard = .pal50
    public var interlaceEnabled: Bool = false // default off
    public var mode: BeebMode // 0..7
}

public protocol CRTCOutputSink: AnyObject {
    func beginFrame(info: CRTCFrameInfo)
    func beginScanline(y: Int, counters: CRTCCounters)
    func endScanline(y: Int)
    func endFrame(info: CRTCFrameInfo)
}

public final class CRTC6845 {
    public init(config: CRTCConfig, sink: CRTCOutputSink)
    public func writeRegister(_ index: UInt8, _ value: UInt8)
    public func readRegister(_ index: UInt8) -> UInt8
    public func tickFrame() // advances exactly one 20 ms (PAL) or 16.666 ms (NTSC) frame
    public var signals: CRTCSignals { get } // HSYNC, VSYNC, MA, RA, cursor state, etc.
}

public final class RasterBuilder: CRTCOutputSink {
    public var frameTexture: MTLTexture? { get } // last completed
    public func attachMemory(_ mem: BeebMemoryAccess) // ULA fetches
}

public final class HostPresenter {
    public init(device: MTLDevice, view: MTKView)
    public func present(_ texture: MTLTexture, timestamp: CFTimeInterval)
    public func updateHostVsyncRate(_ hz: Double) // optional runtime adjust
}
```

Or:
```swift
import Foundation
import Metal
import MetalKit
import os

public enum VideoStandard { case pal50, ntsc60 }

public enum BeebMode: UInt8 { case mode0, mode1, mode2, mode3, mode4, mode5, mode6, mode7 }

public struct CRTCConfig {
    public var standard: VideoStandard = .pal50
    public var interlaceEnabled: Bool = false // default off
    public var mode: BeebMode = .mode1
    public init(standard: VideoStandard = .pal50, interlaceEnabled: Bool = false, mode: BeebMode = .mode1) {
        self.standard = standard
        self.interlaceEnabled = interlaceEnabled
        self.mode = mode
    }
}

public struct CRTCSignals {
    public var hsync: Bool = false
    public var vsync: Bool = false
    public var ma: UInt16 = 0  // Memory Address counter
    public var ra: UInt8 = 0   // Row Address (scanline within character)
}

public struct CRTCCounters {
    public var hChar: UInt16 = 0    // horizontal character position
    public var vRow: UInt16 = 0     // vertical character row
    public var scanline: UInt8 = 0  // scanline in char (0..R9)
}

public protocol CRTCOutputSink: AnyObject {
    func beginFrame(info: CRTCFrameInfo)
    func beginScanline(y: Int, counters: CRTCCounters)
    func endScanline(y: Int)
    func endFrame(info: CRTCFrameInfo)
}

public struct CRTCFrameInfo {
    public let widthPixels: Int
    public let heightPixels: Int
    public let standard: VideoStandard
    public let mode: BeebMode
}

public final class CRTC6845 {
    public private(set) var config: CRTCConfig
    public weak var sink: CRTCOutputSink?

    // 18 registers (R0..R17). Keep them as bytes for fidelity.
    private var regs: [UInt8] = Array(repeating: 0, count: 18)

    // Derived counters/state
    private var signals = CRTCSignals()

    // Signposting
    private let sp = OSSignposter()
    private let spid: OSSignpostID
    private let log = Logger(subsystem: "com.beeb.BBC-Model-B", category: "crtc")

    public init(config: CRTCConfig, sink: CRTCOutputSink?) {
        self.config = config
        self.sink = sink
        self.spid = sp.makeSignpostID()
        bootstrapDefaults(for: config)
    }

    public func writeRegister(_ index: UInt8, _ value: UInt8) {
        let i = Int(index & 0x1F)
        if i < regs.count { regs[i] = value }
        // Phase‑1: apply on next frame. (Optional Phase‑2: allow mid‑frame effects.)
    }

    public func readRegister(_ index: UInt8) -> UInt8 {
        let i = Int(index & 0x1F)
        return i < regs.count ? regs[i] : 0
    }

    /// Advance exactly one emulated frame (PAL=20ms, NTSC=16.666ms).
    /// Iterates character rows × scanlines, emitting sink callbacks.
    public func tickFrame() {
        let state = CRTCFrameInfo(widthPixels: visibleWidth(), heightPixels: visibleHeight(), standard: config.standard, mode: config.mode)
        let spTok = sp.beginInterval("CRTC_Frame", id: spid)
        sink?.beginFrame(info: state)

        // Simplified Phase‑1 skeleton: iterate visible scanlines only (borders omitted here for clarity).
        let visibleH = visibleWidth()
        let visibleV = visibleHeight()
        let maxScan = Int(regs[9]) // R9: max scanline (0..N)
        var counters = CRTCCounters()

        for y in 0..<visibleV {
            counters.vRow = UInt16(y / (maxScan + 1))
            counters.scanline = UInt8(y % (maxScan + 1))
            sink?.beginScanline(y: y, counters: counters)
            // In Phase‑2, compute MA/RA, HSYNC windows, borders, etc.
            sink?.endScanline(y: y)
        }

        sink?.endFrame(info: state)
        sp.endInterval("CRTC_Frame", id: spid, state: spTok)
    }

    // MARK: - Helpers (derived geometry)
    private func visibleWidth() -> Int {
        switch config.mode {
        case .mode0: return 640
        case .mode1: return 320
        case .mode2: return 160
        case .mode3: return 640 // text alias of 1 with different mapping
        case .mode4: return 320
        case .mode5: return 160
        case .mode6: return 320 // 40×25 text
        case .mode7: return 480 // rendered as 40×25 chars mapped into 480×250 (12×10) logical; adjustable
        }
    }

    private func visibleHeight() -> Int {
        if config.standard == .ntsc60 { return 200 } // US machines ~200 lines
        return 256 // PAL 256 lines
    }

    private func bootstrapDefaults(for config: CRTCConfig) {
        // Seed reasonable defaults per standard/mode. (Exact tables TODO v1.2)
        // Horizontal totals/positions here are placeholders for skeleton wiring.
        regs[0] = 127 // H total (chars)
        regs[1] = config.mode == .mode0 ? 80 : 40 // H displayed (chars)
        regs[2] = 96  // HSYNC pos
        regs[3] = 8   // sync width
        regs[4] = 38  // V total (rows)
        regs[5] = 0   // V total adjust
        regs[6] = 32  // V displayed (rows) -> 32×8 = 256 lines
        regs[7] = 2   // VSYNC pos
        regs[8] = config.interlaceEnabled ? 1 : 0
        regs[9] = 7   // max scanline (8 lines/char)
    }
}
```

```swift
// RasterBuilder.swift (skeleton)
import Foundation
import Metal

public protocol BeebMemoryAccess: AnyObject {
    func readByte(_ addr: UInt16) -> UInt8
}

public final class RasterBuilder: CRTCOutputSink {
    private let device: MTLDevice
    private let pixelFormat: MTLPixelFormat = .bgra8Unorm
    private var texture: MTLTexture?
    private weak var mem: BeebMemoryAccess?

    public init(device: MTLDevice) { self.device = device }

    public func attachMemory(_ mem: BeebMemoryAccess) { self.mem = mem }

    public var frameTexture: MTLTexture? { texture }

    // MARK: - CRTCOutputSink
    public func beginFrame(info: CRTCFrameInfo) {
        ensureTexture(width: info.widthPixels, height: info.heightPixels)
    }

    public func beginScanline(y: Int, counters: CRTCCounters) {
        // Phase‑1: Fill from a simple background or debug pattern.
        // Phase‑2: Fetch from emu RAM/ULA mapping using counters (MA/RA) + mode rules.
    }

    public func endScanline(y: Int) { }

    public func endFrame(info: CRTCFrameInfo) {
        // Finalize if needed. (Nothing required for immutable blit textures.)
    }

    private func ensureTexture(width: Int, height: Int) {
        if let tex = texture, tex.width == width, tex.height == height { return }
        let desc = MTLTextureDescriptor.texture2DDescriptor(pixelFormat: pixelFormat, width: width, height: height, mipmapped: false)
        desc.usage = [.shaderRead, .shaderWrite, .renderTarget]
        texture = device.makeTexture(descriptor: desc)
    }
}
```

```swift
// HostPresenter.swift (skeleton)
import Foundation
import Metal
import MetalKit
import os

public final class HostPresenter {
    private let device: MTLDevice
    private unowned let view: MTKView
    private var lastTexture: MTLTexture?
    private let sp = OSSignposter()
    private let spid: OSSignpostID

    public init(device: MTLDevice, view: MTKView) {
        self.device = device
        self.view = view
        self.spid = sp.makeSignpostID()
        self.view.framebufferOnly = true
        self.view.isPaused = false
        self.view.enableSetNeedsDisplay = false
    }

    public func present(_ texture: MTLTexture, timestamp: CFTimeInterval) {
        lastTexture = texture
        // MTKView delegate or draw loop should sample lastTexture each vsync and blit it.
        // This class holds the latest frame; the host repeats until a new one arrives.
        // (Wire into your existing MTKView draw(in:) implementation.)
    }

    public func drawableSizeWillChange(_ size: CGSize) {
        // No-op: we stretch/center as needed in the render pipeline.
    }
}
```
Notes
- tickFrame() is the fixed-step engine you call from your emulation loop; it internally iterates scanlines and invokes the sink callbacks.
- RasterBuilder builds one progressive texture per tickFrame(); if nothing changed, it can short-circuit and reuse prior texture (dirty-rect optional).
- HostPresenter is fed whenever RasterBuilder finalizes a frame; it repeats until a new one arrives.
    
---

## 6) Pseudocode: fixed‑step emu + decoupled present

```swift
// Emulation thread (fixed step)
let dt: TimeInterval = (config.standard == .pal50) ? 0.020 : (1.0/60.0)
var next = monotonicNow()
while running {
    waitUntil(next)
    next += dt
    crtc.tickFrame() // builds/updates raster.texture via sink
    if let tex = raster.frameTexture { presenter.present(tex, timestamp: monotonicNow()) }
}

// Render thread (MTKView)
func draw(in view: MTKView) {
    guard let drawable = view.currentDrawable else { return }
    let commandBuffer = commandQueue.makeCommandBuffer()
    // Blit/quad render lastTexture → drawable.texture
    commandBuffer?.present(drawable)
    commandBuffer?.commit()
}
```

---

## 8) Threading model
- Emulation thread: runs CPU, VIA, ULA, and CRTC6845.tickFrame() at fixed intervals (wall-clock paced or “catch-up” when resuming).
- Render thread (Metal): uploads textures and presents on host vsync; no blocking waits on emulation.
- Synchronization via a lock-free ring (2–3 textures) or a slim mutex; textures are immutable after finalize.

---

## 9) Diagnostics & policies
- **Signposts only** in hot paths (`CRTC_Frame`, `Raster_Build`, `Presenter_Present`).
- **Release builds:** zero console output. Debug warnings behind `Diagnostics.consoleEnabled` (off by default).
- FPS deviation warnings **HUD‑only** (no console spam) if re‑enabled.

---

## 11) Performance targets
- Build 256-line progressive texture < 2.0 ms on Apple Silicon (Mode 0 worst-case).
- Upload (blit) < 1.0 ms typical with persistent textures.
- Present jitter < 0.5 ms at 120 Hz host.

---

## 12) Edge cases & correctness
- Mid-frame register writes: (phase 2) optionally apply from next scanline; initially, apply on next frame boundary.
- Raster effects: (phase 2) add callback at HSYNC to re-latch select registers for the following line.
- Interlace: default off; enable only when a title expects it (flicker risk).
- Mode 7: render via SAA5050 emulation → line-mapped into 256-line progressive buffer.

## 13) Testing
- Golden-frame tests: known titles (e.g., test cards, modes 0–7) compare CRCs.
- Timing tests: verify 50.000/60.000 Hz cadence against a monotonic clock (±0.05%).
- Stress: rapid mode switches; window resizes; host at 59.94/60/120 Hz.
- User-visible: no tearing; stable cadence on 60 Hz and 120 Hz displays.

## 14) Acceptance criteria
- PAL default: 256-line progressive output at logical 50 Hz; host shows smooth output with frame repeats (no console spam).
- NTSC option: 200-line variant at logical 60 Hz; Mode 7 → 20 rows.
- Deterministic emulation clock: titles relying on vblank timing behave correctly.
- Host-decoupled rendering: identical emulation behavior on 60/120 Hz displays.

---

## 15) Pseudocode (core loop) (suggestion)

```swift
// Emulation thread
let dt = (config.standard == .pal50) ? 0.020 : 1.0/60.0
var nextFrameTime = monotonicNow()

while running {
    // Pace to fixed-step; allow small catch-up if behind
    waitUntil(nextFrameTime)
    nextFrameTime += dt

    // Advance 6845 one frame (progressive)
    crtc.tickFrame() // invokes raster.begin/end callbacks and builds a texture

    if let tex = raster.frameTexture {
        presenterQueue.enqueue(tex, timestamp: monotonicNow())
    }
}

// Render thread (Metal)
while showing {
    if let item = presenterQueue.dequeueNewest() {
        hostPresenter.present(item.texture, timestamp: item.timestamp)
    } else {
        hostPresenter.present(lastTexture, timestamp: lastTimestamp) // repeat
    }
    waitForNextHostVsync()
}
```

---

## Roadmap (next increments)
1) **Register tables per mode/standard** (exact R0–R9 defaults; borders; HSYNC windows).  
2) **Mid‑frame register write path** (optional) for raster effects.  
3) **ULA fetch mapping** (bytes‑per‑line per mode, palette, Mode 7 with SAA5050).  
4) **Triple‑buffered presenter** with blit pipeline + scaling shader.  
5) **120 Hz tuning** and present‑age metrics.  
6) **Interlace support** behind a toggle (off by default).

---

## Checklist vs our ambition
- No field emulation ✅
- 20 ms PAL timing ✅
- 256/200 visible lines ✅
- Decoupled host vsync ✅
- Clean API, testable core ✅
- Zero‑console Release, signposts ✅
- Room for per‑scanline effects later ✅
