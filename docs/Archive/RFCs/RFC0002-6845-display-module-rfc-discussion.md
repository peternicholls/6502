# 6845+ Display Module RFC — Multi-Agent Discussion
**RFC:** 0002 (referencing RFC-0001)  
**Date:** 2025-11-05  
**Status:** Approved  
**Approved By:** BMad  
**Approval Date:** 2025-11-05  
**Discussion Format:** Architectural Review Board

---

## Participants

**🏗️ Architecture Agent (Arch)** - System design, module boundaries, testability  
**⚡ Performance Agent (Perf)** - Timing, latency, threading, optimization  
**🎯 Emulation Agent (Emu)** - Accuracy, authenticity, hardware fidelity  
**🔧 Implementation Agent (Impl)** - Practicality, Swift patterns, Metal integration  
**�� Testing Agent (Test)** - Verification strategy, test coverage, golden frames

---

## Discussion Topics

### Topic 1: Fixed-Step Timing vs. Cycle-Accurate Emulation

**Arch:** The RFC proposes `tickFrame()` advancing exactly 20ms per call (PAL). This is frame-level granularity. Question: Should we support cycle-accurate CRTC register reads/writes within the frame, or is frame-boundary sufficient for Phase 1?

**Emu:** Historical accuracy perspective: Most BBC Micro software doesn't write CRTC registers mid-frame except for:
- Split-screen effects (change mode/palette between screen areas)
- Raster tricks (cycle-counted writes to create effects)
- Elite's scanner display (changes R12/R13 during frame)

For 90% of software, frame-boundary is sufficient. However, we'll need per-scanline granularity eventually.

**Perf:** Frame-level is vastly simpler and faster. A 20ms tick with 256 scanlines means ~78µs per scanline. If we buffer register writes and apply them at frame boundaries, we avoid complex state tracking. 

**Recommendation:** Phase 1 = frame-boundary. Phase 2 = add per-scanline callback option for register writes.

**Impl:** Agreed. The `CRTCOutputSink` protocol already has `beginScanline()` hooks. We can enhance it with a `writeRegisterMidFrame()` callback later without breaking the API.

---

### Topic 2: Threading Model & Synchronization

**Arch:** The RFC shows two threads: emulation thread (fixed-step) and render thread (Metal vsync). Synchronization via "lock-free ring (2-3 textures)". Let's detail this.

**Perf:** Lock-free ring buffer concerns:
1. **Triple buffering**: Emu writes to texture A, GPU reads texture B, texture C is spare. Classic producer-consumer.
2. **Atomics**: Use `std::atomic<uint32_t>` or Swift's atomic types for ring indices.
3. **Contention**: With 50Hz emu and 60Hz present, we have 5:6 ratio. Ring size of 3 is sufficient; rarely blocks.

**Impl:** Swift doesn't have great lock-free primitives pre-Swift 6. Options:
- `OSAllocatedUnfairLock` (fast, unfair lock) - acceptable for this use case
- `DispatchSemaphore` - heavier but well-tested
- C++ atomic wrapper via bridging header

For Phase 1, I suggest `OSAllocatedUnfairLock` protecting a 3-texture ring. Measure contention; if <0.1ms lock time, it's fine.

**Arch:** Acceptable. Document the lock scope clearly: protect only the ring indices, not the texture upload itself.

---

### Topic 3: BeebMemoryAccess Protocol Design

**Arch:** The RFC shows `protocol BeebMemoryAccess: AnyObject { func readByte(_ addr: UInt16) -> UInt8 }`. This is minimal. Questions:
1. Should it expose bulk reads (e.g., `readBytes(addr: UInt16, count: Int) -> [UInt8]`)?
2. Should it abstract ULA address translation, or is that RasterBuilder's job?

**Emu:** The ULA (Video ULA chip) has complex address mapping:
- Modes 0-6: Linear addressing with wraparound at 32KB boundary
- Mode 7: Teletext has different layout (1KB at 0x7C00)
- Hardware scrolling: R12/R13 registers offset the start address

The ULA translation logic should be in `RasterBuilder`, not the memory protocol. The protocol should remain "read physical RAM address X".

**Perf:** Bulk read optimization is important. Fetching 256 bytes per scanline (Mode 0) as 256 individual calls is slow. Suggest:
```swift
protocol BeebMemoryAccess {
    func readByte(_ addr: UInt16) -> UInt8
    func readBytes(_ addr: UInt16, count: Int, into: UnsafeMutableBufferPointer<UInt8>)
}
```
RasterBuilder calls `readBytes()` for entire scanlines when possible.

**Impl:** Agreed, but careful with Swift bridging overhead. If `BeebMemoryAccess` is implemented in C++, the bulk API should use raw pointers, not Swift arrays.

---

### Topic 4: Mode 7 (Teletext) Integration

**Arch:** Mode 7 uses the SAA5050 teletext chip. The RFC says "render via SAA5050 emulation → line-mapped into 256-line progressive buffer". Architecture question: Is SAA5050 a separate module, or part of RasterBuilder?

**Emu:** SAA5050 is logically separate:
- Has its own character ROM (not in main RAM)
- Generates 12×10 pixel characters from 7-bit codes
- Has display/graphics modes, foreground/background colors, flashing, etc.
- 25 rows × 40 columns = 1000 bytes at 0x7C00-0x7FFF

Suggest: `SAA5050Renderer` module that RasterBuilder delegates to when mode == .mode7.

**Impl:** Agreed. API sketch:
```swift
protocol TeletextRenderer {
    func renderScanline(y: Int, chars: [UInt8]) -> [UInt32] // BGRA pixels
}

class SAA5050Renderer: TeletextRenderer {
    // Implements BBC Master / Model B variant
}
```

RasterBuilder calls this for Mode 7 scanlines, otherwise uses ULA path.

**Arch:** Good separation. Add this to the RFC's "Roadmap" section as a Phase 2 dependency.

---

### Topic 5: Host Presentation & Frame Pacing

**Perf:** The RFC proposes frame repeat strategy: 50Hz emu → 60Hz host = 5:6 repeat pattern. Let's validate this:

**Math:**
- Emu period: 20.000ms (50Hz)
- Host period: 16.667ms (60Hz)
- Ratio: 20/16.667 = 1.2 = 6/5

So every 5 emu frames should span 6 host frames. Pattern: `A A B B C C D D E E F` (show A twice, B twice, etc., except one frame shows 3x per 6-frame cycle).

Actually, more precise pattern over 30 host frames (500ms):
- Emu produces: 25 frames
- Host shows: 30 frames
- Difference: 5 repeats across 25 frames

**Impl:** Practical algorithm: track `lastEmuTimestamp` and `currentHostTime`. When presenting, pick the emu frame whose timestamp is closest to `currentHostTime`. This naturally handles irregular host vsync (e.g., 59.94Hz NTSC displays).

**Arch:** Agreed. No hardcoded repeat pattern; use timestamp comparison. This also handles pause/resume gracefully.

**Perf:** Latency concern: If we always pick "closest timestamp", we might introduce 10ms average latency (half a frame). Alternative: pick "most recent emu frame not in the future". This minimizes latency at the cost of occasional judder if emu runs slow.

**Recommendation:** Configurable policy. Default = "most recent", option for "closest" (smoother but higher latency).

---

### Topic 6: Testing Strategy

**Test:** The RFC mentions golden-frame CRCs. Let's define the testing layers:

**Layer 1: Unit Tests (Isolated)**
- `CRTC6845` alone: Feed register values, assert counters/signals after N scanlines
- `RasterBuilder` alone: Mock memory + mock CRTC signals → verify texture pixels
- `HostPresenter` alone: Mock textures + mock vsync → verify repeat logic

**Layer 2: Integration Tests**
- CRTC + RasterBuilder: Load a known RAM pattern (e.g., test card), verify full-frame CRC
- Full chain: Emu thread + Render thread, run for 1 second, verify frame count ± tolerance

**Layer 3: Golden Frame Tests**
- Capture reference images from:
  - BeebEm emulator (Windows; gold standard)
  - B-Em (Linux)
  - JSBeeb (browser; known good)
- Compare CRC or perceptual hash (to allow minor dither differences)

**Emu:** Important: Golden frames must be captured with identical CRTC register settings. Many emulators use slightly different defaults. Suggest we bootstrap from actual hardware screenshots or BeebEm with documented registers.

**Test:** Agreed. Also test edge cases:
- Mode switches (0→7→2 rapidly)
- Mid-frame palette changes (once we support them)
- Unusual CRTC register combinations (non-standard screen sizes)

**Impl:** For Swift Testing, we can use:
```swift
@Test func goldenFrameMode0() async throws {
    let crtc = CRTC6845(config: .init(mode: .mode0))
    let raster = RasterBuilder(device: metalDevice)
    // ... load test pattern into mock RAM
    crtc.tickFrame()
    let texture = raster.frameTexture!
    let crc = computeCRC32(texture)
    #expect(crc == 0x12345678) // known good value
}
```

---

### Topic 7: Performance Budgets & Profiling

**Perf:** The RFC states:
- Build 256-line texture: <2.0ms (Mode 0 worst-case)
- Upload/blit: <1.0ms
- Present jitter: <0.5ms at 120Hz

Let's validate these are achievable and define measurement strategy.

**Mode 0 Analysis (worst case):**
- 640×256 pixels = 163,840 pixels
- Each pixel requires: ULA memory fetch (1 byte → 8 pixels), palette lookup, format conversion
- Rough CPU budget: 163,840 / 2.0ms = 81.9M pixels/sec
- Apple Silicon (M1): ~10 GHz effective, 81.9M pixels = ~122 cycles/pixel

**Impl:** 122 cycles/pixel is tight but achievable if we:
1. Batch memory reads (avoid per-byte boundary checks)
2. Use SIMD for palette lookup (NEON intrinsics via Accelerate)
3. Pre-compute mode scanline byte counts (avoid per-pixel arithmetic)

**Perf:** Profiling strategy:
- Instruments: `os_signpost` intervals for `CRTC_Frame`, `Raster_Build`, `Presenter_Upload`
- Micro-benchmarks: Render 1000 frames of Mode 0 test pattern, measure p50/p95/p99
- Real-world: Run "The Sentinel" (known demanding title), verify <5% frame drops

**Arch:** Add to acceptance criteria: "p95 frame build time <2.5ms on M1 (Mode 0), <1.0ms on M2 (Mode 0)".

---

### Topic 8: API Refinements & Swift Conventions

**Impl:** Reviewing the proposed Swift API, some suggestions:

**Issue 1: Weak sink reference**
```swift
public weak var sink: CRTCOutputSink?
```
Problem: If sink is deallocated mid-frame, `tickFrame()` will silently no-op. Better:
```swift
public unowned let sink: CRTCOutputSink
```
Or require non-optional initialization: `init(config:, sink:)` with `sink` as non-optional param.

**Issue 2: Register write thread safety**
If CPU emulation thread calls `writeRegister()` while `tickFrame()` is executing, we have a data race. Options:
- Document: "All calls must be from emulation thread"
- Add: `@MainActor` or `DispatchQueue` enforcement
- Use: atomics or lock per register array

**Arch:** Phase 1 = document single-threaded usage. Phase 2 = add thread safety if multi-threaded CPU emulation becomes necessary.

**Issue 3: Signal exposure**
```swift
public var signals: CRTCSignals { get }
```
Should this return a copy (value type) or a reference? If value, it's snapshot-safe but causes copying overhead in hot paths. If reference, caller can cache but must handle mutation.

**Perf:** Signals are read ~50,000 times/sec (once per scanline in detailed mode). Make `CRTCSignals` a `struct` (value type) to avoid reference counting overhead.

---

### Topic 9: Error Handling & Diagnostics

**Arch:** The RFC emphasizes "zero console output in Release". How do we handle errors?

**Scenarios:**
1. Metal device unavailable (unlikely but possible on old hardware)
2. Texture allocation failure (out of VRAM)
3. Invalid CRTC register values (e.g., R1 > R0)

**Impl:** Swift error handling:
```swift
public enum CRTCError: Error {
    case invalidRegisterValue(register: UInt8, value: UInt8)
    case textureAllocationFailed
}

public func writeRegister(_ index: UInt8, _ value: UInt8) throws {
    guard index < 18 else { throw CRTCError.invalidRegisterValue(...) }
    // ...
}
```

Problem: `tickFrame()` is called 50x/sec; we can't `throw` every frame without massive overhead.

**Alternative:** Fail-safe defaults
- Invalid register? Clamp to valid range, log once per session (or never in Release)
- Texture alloc fail? Keep rendering to previous texture
- Metal unavailable? Return black texture

**Arch:** Agreed. Reserve `throw` for initialization failures only. Runtime issues use fail-safe + optional signpost logging.

---

### Topic 10: Roadmap Prioritization

**Arch:** The RFC lists 6 roadmap items. Let's prioritize for story planning:

**Phase 1 (MVP - Epic 2?):**
1. ✅ CRTC6845 skeleton with frame-boundary register handling
2. ✅ RasterBuilder with Mode 0-6 basic rendering (no Mode 7 yet)
3. ✅ HostPresenter with texture repeat logic
4. ✅ Single-threaded proof of concept
5. ⚠️ Golden frame test for Mode 1 (simplest)

**Phase 2 (Polish):**
1. Triple-buffered threading with ring buffer
2. Mode 7 integration via SAA5050Renderer
3. Mid-frame register write path (per-scanline)
4. 120Hz tuning + present-age metrics
5. Interlace toggle (R8 support)

**Phase 3 (Advanced):**
1. Raster effects (demos like "Inifinity Intro")
2. ULA fetch optimization (SIMD)
3. Dirty-rect optimization (skip unchanged regions)
4. Video export (offscreen rendering → QuickTime)

**Emu:** One caution: Don't defer Mode 7 too long. Many popular titles (e.g., Ceefax, teletext adventures) require it. Suggest Mode 7 in Phase 1.5.

**Impl:** Agreed. Mode 7 is ~200 lines of code if we use a simple character ROM lookup. Full SAA5050 emulation (graphics mode, doubleheight, etc.) can wait for Phase 2.

---

## Consensus & Action Items

### ✅ Approved Design Decisions

1. **Timing:** Frame-boundary register handling for Phase 1; per-scanline optional for Phase 2
2. **Threading:** Triple-buffer with `OSAllocatedUnfairLock`; measure contention
3. **Memory API:** Add bulk read method; keep protocol minimal
4. **Mode 7:** Separate `SAA5050Renderer` module, integrate in Phase 1.5
5. **Frame Pacing:** Timestamp-based repeat selection; configurable policy
6. **Testing:** 3-layer strategy (unit, integration, golden frames)
7. **Diagnostics:** Signposts only in Release; fail-safe error handling

### 📋 Action Items for RFC v1.2

1. **Arch:** Add threading synchronization details (lock scope, ring buffer algorithm)
2. **Perf:** Document performance measurement strategy and tooling
3. **Emu:** Add register default tables for each mode/standard (Appendix)
4. **Impl:** Refine Swift API (unowned sink, signal types, error handling)
5. **Test:** Define golden frame capture process and CRC validation approach

### ⚠️ Open Questions (APPROVED — 2025-11-05)

1. ✅ **Mode 7 Timing:** **Phase 1.5** (approved by BMad)
2. ✅ **Latency vs. Smoothness:** **"Most recent" as default** (approved by BMad)
3. ✅ **NTSC Support:** **Phase 3 (deferred)** — may not be needed (approved by BMad)

---

## Next Steps

**RFC Status:** ✅ **APPROVED** — All design decisions and open questions resolved (2025-11-05 14:32 UTC)

**For Implementation:**
Once approved, create Epic 2 stories:
- Story 2.1: CRTC6845 Core (registers, counters, frame iteration)
- Story 2.2: RasterBuilder (Modes 0-6, basic rendering)
- Story 2.3: HostPresenter (Metal integration, frame repeat)
- Story 2.4: Golden Frame Testing (Mode 1 test card)
- Story 2.5: Mode 7 Renderer (SAA5050 basic character mode)

**Estimated Effort:** Epic 2 = ~8-12 developer-days (assuming no major blocks)

---

**Discussion Status:** ✅ Complete — Awaiting author review and approval
