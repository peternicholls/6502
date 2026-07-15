---
title: Tech Spec Epic 1
status: DRAFT
type: Spec
version: 1.0.0
owner: TBD
date-created: '2025-11-05'
date-updated: '2025-11-05'
description: 'TODO: Add description'
---

# Epic Technical Specification: Project Foundation & Core Loop

Date: 2025-11-03
Author: BMad
Epic ID: epic-1
Status: Draft

---

## Overview

This epic establishes the foundational infrastructure and initial functionality for the BBC Model B emulator project. It includes repository setup, core render/audio loop, and baseline scaffolding to enable further development.

## Objectives and Scope

### In-Scope
- Repository initialization with CI/CD pipelines.
- Core render loop using Metal at 50 Hz.
- Audio output using AVAudioEngine.
- C API boundary for bridging the core emulator to Swift.
- Status HUD for displaying FPS and buffer stats.

### Out-of-Scope
- Advanced audio/video features (e.g., SN76489 PSG, Mode 7 rendering).
- Full emulator functionality (e.g., CPU, VIA, CRTC integration).

## System Architecture Alignment

This epic aligns with the architecture by establishing the core loop and bridging the C++ emulator core to the SwiftUI app. It ensures deterministic scheduling and stable 50 Hz rendering.

## Detailed Design

### Services and Modules
- **Repository Setup**: Initialize version-controlled repository with README, license, and CI/CD pipelines.
- **Core Loop**: Implement Metal-based render loop and AVAudioEngine for audio output.
- **C API Boundary**: Create `core_api.h` with hooks for initialization, reset, and tick operations.
- **HUD**: Develop a status HUD to display FPS and buffer stats.

### Data Models and Contracts
- No complex data models required for this epic.

### APIs and Interfaces
- `core_api.h`:
  - `void beeb_init();`
  - `void beeb_reset();`
  - `void beeb_tick(int cycles);`

### Workflows and Sequencing
1. Initialize repository.
2. Implement core render loop.
3. Add audio output.
4. Create C API boundary.
5. Develop status HUD.

## Non-Functional Requirements

### Performance
- Maintain 50 Hz rendering with ±0.1 FPS stability.

### Security
- N/A for this epic.

### Reliability/Availability
- Ensure deterministic scheduling for stable rendering.

### Observability
- Provide FPS and buffer stats via HUD.

## Dependencies and Integrations
- Metal for rendering.
- AVAudioEngine for audio output.
- GitHub Actions for CI/CD.

## Acceptance Criteria (Authoritative)
1. Repository initialized with README, license, and CI/CD pipelines.
2. Metal-based render loop runs at 50 Hz for 10 continuous minutes.
3. AVAudioEngine outputs a test tone without underruns (<0.1% frames).
4. `core_api.h` provides initialization, reset, and tick hooks.
5. Status HUD displays FPS and buffer stats.

## Traceability Mapping
| AC | Spec Section | Component | Test Idea |
|----|--------------|-----------|-----------|
| 1  | Repository Setup | GitHub Actions | Verify CI/CD pipeline runs on push |
| 2  | Core Loop | Metal | Measure FPS stability |
| 3  | Audio Output | AVAudioEngine | Check audio underrun rate |
| 4  | C API Boundary | `core_api.h` | Call hooks and verify behavior |
| 5  | HUD | SwiftUI | Validate FPS and buffer stats display |

## Risks, Assumptions, Open Questions

### Risks
- Potential delays in CI/CD pipeline setup.
- Performance issues with Metal rendering on older devices.

### Assumptions
- Metal and AVAudioEngine are supported on target devices.

### Open Questions
- Should the HUD include additional metrics (e.g., memory usage)?

## Test Strategy Summary
- Unit tests for `core_api.h` hooks.
- Integration tests for render loop and audio output.
- Manual tests for HUD display and CI/CD pipeline verification.