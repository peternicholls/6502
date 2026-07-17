# Legacy design decision register

**Status:** Canonical interpretation of historical material  
**Updated:** 2026-07-17

The files under `../Archive/` came from an abandoned version
of the project. They capture useful product discovery, but their implementation
details and apparent approvals are not binding on the current codebase.

Disposition meanings:

- **Retained:** still directs current product or architecture work.
- **Refined:** the intent remains, but the current project defines it more
  precisely.
- **Superseded:** replaced by the current implementation or architecture.
- **Open:** requires evidence or a product decision.
- **Deferred:** valid possibility, but not on the committed roadmap.

## Product decisions

| Legacy position | Disposition | Current direction |
| --- | --- | --- |
| The product combines Machine, Media and Editor experiences. | Retained | This is the central product model in `VISION.md`; the emulator is its foundation. |
| Primary users are creators, collectors, educators and technically curious users. | Retained | Reframed around creator, preservation and exploration jobs. |
| The application is native to iPhone, iPad and macOS. | Retained | SwiftUI-first with platform-specific adaptation where necessary. |
| Authenticity and accessibility should coexist with modern convenience. | Retained | Modern tools may assist input and editing but must preserve authentic execution. |
| Fixed 4:3 presentation, nearest-neighbor scaling and no default CRT effects. | Retained | This is a host-presentation rule; raw core frame geometry is not forced to 4:3. |
| Media is imported through system pickers and remains local. | Retained | Import-only and no downloaded executable content are product/legal boundaries. |
| Tape, including live microphone capture, is a differentiator. | Retained | File decoding precedes microphone DSP; reliability claims require a recording corpus. |
| A label-aware editor round-trips BBC BASIC through RAM. | Retained | Transactional memory APIs and deterministic tokenization are prerequisites. |
| Clean-room firmware is available by default. | Open | The current product requires user-supplied firmware. Redistributable firmware needs feasibility, legal and value validation. |
| Freemium conversion, download and retention targets. | Open | Treat as hypotheses until a beta, analytics policy and monetization decision exist. |
| Specific 20–26 week milestone dates. | Superseded | `ROADMAP.md` uses evidence-based capability horizons without schedule promises. |
| BBC Model B wording is suitable as the final store name. | Open | Final naming requires trademark-safe product review. |

## Core and integration decisions

| Legacy position | Disposition | Current direction |
| --- | --- | --- |
| C++ core behind a stable C API and Swift wrapper. | Retained | Implemented as `BeebCore` → `beeb_c.h` → `BeebKit`; C++ exceptions are contained at the ABI. |
| Reuse and adapt the old davepoo-derived fork. | Superseded | The current repository contains a new dependency-free C++20 implementation with its own tests and documented legal boundary. |
| Separate submodule, Xcode project or prebuilt XCFramework. | Refined | C2 implements a committed top-level Xcode project as an Apple host/build surface over the monorepo. Its shared macOS, iOS Simulator and test schemes consume the same local package products and demo source; Swift Package Manager and the portable Makefile remain authoritative, and binary distribution remains deferred. |
| C++17 and Google Test/CMake are required. | Superseded | The core uses C++20 and a dependency-free test executable. New dependencies require an explicit need. |
| Host file, window and audio concerns remain outside the deterministic core. | Retained | Bytes enter explicitly; frames, samples and state leave through stable data contracts. |
| Swift wrapper failures use `throws`; C failures use error codes. | Refined | C returns structured operation-owned statuses and success/explicit-partial outputs; Swift preserves typed categories, diagnostics and recoverable audio pressure with owned partial samples. |
| Auto-save uses a core state blob plus host metadata. | Refined | The concept is retained, but a versioned snapshot contract has not yet been designed. |
| The Swift wrapper's single lock is the final threading model. | Superseded | `BeebMachine` adds no lock. The C++ runtime owner is the sole serialization authority and owns the measured bounded frame/audio queues behind its FIFO. |

## Timing, display and audio decisions

| Legacy position | Disposition | Current direction |
| --- | --- | --- |
| The core advances in fixed 20 ms `tickFrame()` calls. | Superseded | CPU execution drives machine time. The current instruction-boundary device tick will evolve toward bus-cycle micro-operations. |
| CRTC register writes are buffered to frame boundaries in phase one. | Superseded | Register writes occur through the emulated bus. Fidelity improvements should preserve their emulated timing rather than impose a host-frame abstraction. |
| CRTC timing, pixel construction and host presentation are separate responsibilities. | Retained | The current core already separates CRTC/ULA/rendering; Metal presentation remains a host concern. |
| A triple-buffer protected by `OSAllocatedUnfairLock` is mandatory. | Superseded | C2 implements a capacity-three owner-only frame FIFO with oldest-drop pressure accounting. It needs no internal lock because all publication and dequeue operations run on `MachineRuntime`; the 10,000-item and sustained measurements enforce the bound. |
| Host presentation repeats 50 Hz frames on 60/120 Hz displays using timestamps. | Retained | Presentation must not alter emulated time; frame-age and pacing metrics should validate the policy. |
| Metal and AVAudioEngine are the Apple presentation/output technologies. | Retained | They are planned host integrations, not core dependencies. |
| “Cycle accurate” is an achieved binary property. | Refined | Accuracy is a set of evidence-backed behaviors. Current aggregate instruction cycles are correct; bus events are not yet scheduled per cycle. |
| Mode 7 should use an embedded character ROM lookup. | Superseded | The repository uses a clean-room host font. Exact SAA5050 behavior remains a fidelity track without copying proprietary glyph data. |
| Golden frames compare output with a reference emulator. | Refined | Retain layered unit/integration/golden testing, but fixtures must have documented provenance and legally usable inputs. |
| Low audio latency and zero underruns can be assumed from AVAudioEngine. | Refined | C2 now supplies a measured capacity-4,096 continuous ring with exact demand, overrun and underrun accounting. Device latency and zero-underrun claims still require sustained AVAudioEngine integration tests. |

## UX and platform decisions

| Legacy position | Disposition | Current direction |
| --- | --- | --- |
| Modern Native with restrained retro accents. | Retained | Native semantics and accessibility take priority over decorative nostalgia. |
| Machine, Media and Editor use platform-appropriate navigation. | Retained | Exact layouts remain design work for each device class. |
| Keyboard overlay, key help and safe input capture are required. | Retained | A graphical keyboard remains optional research. |
| Full-screen is opt-in and always has a discoverable exit. | Retained | Preserve platform shortcuts and never trap keyboard focus. |
| Swift Testing is mandatory and XCTest is deprecated. | Open | Current Swift boundary tests use XCTest. Standardize only when the chosen toolchain, UI testing needs and migration value justify it. |
| Xcode Cloud is the primary CI system. | Superseded | GitHub Actions validates the Linux core, Swift package, shared macOS/iOS Simulator app builds and Xcode test scheme. Xcode Cloud may supplement device/archive workflows later. |
| iOS 17/macOS 14 are fixed product minimums. | Open | Current package declarations are build baselines, not a final release support decision. |
| Combine is the required state-management framework. | Superseded | Prefer the simplest current Swift concurrency/observation tools; introduce a framework only for a concrete requirement. |

## Historical document disposition

| Document | Use now |
| --- | --- |
| `product-brief-BBC Model B-2025-11-03.md` | Product discovery reference. User problems, experience pillars and legal concerns informed the current vision. Commercial numbers and implementation assumptions are unvalidated. |
| `PRD.md` | Requirements reference. Superseded by the current product vision and roadmap for planning authority. |
| `ux-design-specification.md` | Durable UX principles reference. Exact components and layouts remain proposals. |
| `epics.md` | Historical backlog. Do not implement story numbers or milestone ordering directly. |
| `architecture.md` | Historical architecture. Current technical authority is `../ARCHITECTURE.md`. |
| `tech-spec-epic-1.md` | Historical foundation plan; its current equivalents are already implemented differently. |
| Display RFCs | Design research. Separation and host-pacing conclusions remain useful; fixed-frame core timing is rejected. |
| Xcode Cloud setup documents | Retired operational guidance. GitHub Actions is current CI. |

## Adoption rule

When historical material suggests a useful feature or constraint that is not
listed here, add it to this register with a disposition before updating the
current roadmap or architecture. Historical “approved” labels describe the
abandoned project and do not bypass current review.
