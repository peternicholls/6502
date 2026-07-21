# 004-machine-target-profile: Code Quality & Architecture Review

This document provides a detailed review of the `004-machine-target-profile` implementation across the C++ Core, C ABI, Swift Wrapper, and SwiftUI Application boundaries. It evaluates the code based on clean code practices, documentation alignment with the project constitution, and broader architectural considerations.

## 1. Clean Code Practices

Overall, the code is highly disciplined, demonstrating strong adherence to strict boundaries and memory safety.

**The Good:**
*   **Strong Encapsulation & Immutability**: `ProfileComponentIdentity` and `MachineTargetProfile` in C++ only provide getters (`[[nodiscard]] constexpr`). They don't have setters, preventing accidental mid-flight mutation. This makes the code exceptionally easy to reason about.
*   **Self-Documenting Helpers**: In `profile.cpp`, extracting logic into small named helpers like `isKnownBaseIdentifier`, `identifierText`, `malformed`, and `unknown` keeps the main `validateMachineTargetProfile` function readable.
*   **RAII in C/C++ boundary**: The `ActiveCall` wrapper class in `beeb_c.cpp` is a fantastic use of RAII (Resource Acquisition Is Initialization). It completely hides the complexity of registry locking, handle reference counting, and C++ exception containment, allowing the `operation(...)` template to remain clean.

**Areas for Improvement:**
*   **Repetitive Translation Boilerplate**: In `beeb_c.cpp`, there is a lot of boilerplate required to translate structs field-by-field. While this is the safest way to cross the ABI without assuming memory layouts, it is verbose and could become a maintenance burden as structs grow.
*   **Cyclomatic Complexity in Validation**: `validateMachineTargetProfile` in `profile.cpp` has a very long sequence of `if/return` checks. While it perfectly follows the precedence order defined in the spec, breaking it down into `validateStructuralIntegrity(profile)` and `classifySemanticSupport(profile)` might make the function shorter and more expressive.

## 2. Documentation & Constitution Alignment

The project constitution and specs (`AGENTS.md`, `plan.md`) demand rigorous traceability, preventing "documentation debt," and explicitly requiring that internal states do not claim features they don't have.

**The Good:**
*   **Mandatory Rationale Links**: You have `// C0-DOC-RATIONALE: docs/code/target-profile.md owns...` at the top of files like `profile.hpp`. This is an excellent, constitution-aligned practice. It stops developers from duplicating architectural philosophy in code comments where it might drift from the true spec.
*   **Honest Diagnostics**: The error strings returned by the system are aggressively honest. In `main.swift`, `BeebError.machineProfileUnavailable` explicitly translates to `[Profile] is recognised, but machine support is not yet available`. This aligns perfectly with the spec's mandate to not "claim or simulate completed Model B+ machine behavior."
*   **Doxygen/DocC Integration**: The headers (`beeb_c.h`) are richly decorated with Doxygen commands (`/// @param`, `/// @return`), explicitly listing the `BEEB_STATUS_...` codes each function can return. The Swift code uses native DocC triple-slash formatting.

**Areas for Improvement:**
*   **Magic Numbers in Documentation**: In `BeebMachine.swift`, the error for `.invalidOSROM` is hardcoded as `"A BBC Model B OS ROM must be exactly 16 KiB."` If later B+ profiles require different ROM sizes, this string becomes inaccurate. Pushing the "truth" (and the error string) entirely into the C++ layer would be cleaner.

## 3. Architecture Issues & Considerations

The architecture handles the immediate requirements flawlessly, but looking ahead to the rest of the roadmap (e.g., M2 Continuity/Snapshots and M3 Expansions), there are a few structural friction points to watch.

**The Good:**
*   **Zero "Enum Fallback"**: The architecture forces the host to deal with `unknown` and `recognisedUnavailable` states natively. By creating a 16-slot expansion aggregate now, you have future-proofed the ABI against changing sizes when expansions are added later.
*   **Strict Concurrency Serialization**: The C++ core is not thread-safe by default, but the C boundary (`beeb_c.cpp`) enforces strict serialization through `registryMutex` and `HandleState::mutex`. This guarantees that if the Swift UI requests a frame while another thread resets the machine, the system will not tear.

**Architectural Friction Points to Watch:**
*   **The Swift "Fat Interface" Problem**: `BeebMachine.swift` acts as a monolithic God Object for the emulator. It handles everything: profile queries, lifecycle (start/pause), ROM loading, disc mounting, keyboard input, and audio/video draining. As the roadmap expands to include tape loading, debugger bridging (C6), and snapshots (C3), this class will become massive. 
    *   *Recommendation*: Consider splitting the Swift API into cohesive facades, e.g., `BeebMachine.Storage`, `BeebMachine.AudioVideo`, `BeebMachine.Debugger`, which all hold a reference to the shared C-handle.
*   **C ABI C-String Constraints**: The `BEEB_STATUS_MESSAGE_CAPACITY` is fixed at 256 bytes. In `translateProfile`, if an error message exceeds 255 characters, it is silently truncated. If a future validation error needs to list *which* expansions conflict (e.g., "Econet is incompatible with X, Y, and Z"), you might hit this boundary and lose valuable diagnostic information. 
    *   *Recommendation*: Ensure that core C++ error generators understand this 256-byte limit so they format concise strings.
