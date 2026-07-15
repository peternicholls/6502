---
title: Xcode Cloud Setup
status: DRAFT
type: Guide
version: 1.0.0
owner: TBD
date-created: '2025-11-05'
date-updated: '2025-11-05'
description: 'TODO: Add description'
---

# Xcode Cloud CI/CD Setup

**Last Updated:** 2025-11-05  
**Story:** 1-2-hotfix-1-xcode-cloud-workflow-setup  
**Status:** Configuration Guide

## Overview

This document provides the configuration guide for Xcode Cloud workflows that enforce code quality gates and ensure reliable archiving in the cloud.

## Project Configuration

### Core Settings

- **Workspace:** `BBC Model B.xcworkspace`
- **Scheme:** `BBC Model B`
- **Team:** `P52T58LHR7`
- **Bundle Identifier:** `uk.co.peternicholls.BBC-Model-B`
- **Xcode Version:** 16.0 (match local version)

### Archivable Products

The scheme supports two archivable products:
1. **iOS App** - Destination: Any iOS Device (platform iOS)
2. **macOS App** - Destination: Any Mac (platform macOS)

## Workflow 1: PR Validation (Lean)

**Purpose:** Fast quality gates for pull requests

### Start Conditions
- **Trigger:** Pull Request Changes
- **Target Branches:** `develop`, `master`
- **Auto-cancel:** Enabled (cancel superseded builds)

### Files & Folders Filter
Include only relevant code changes to minimize unnecessary builds:
```
*.swift
*.metal
*.h
*.cpp
*.c
*.plist
*.json
*.xcconfig
CMakeLists.txt
External/BeebCore/**
```

### Actions
1. **Build** - Debug configuration
2. **Test** - Run unit tests on latest iOS Simulator
3. **Analyze** - Static analysis

### Test Configuration
- **Unit Tests:** `BBC Model BTests` (all tests)
- **UI Tests:** Optional or moved to nightly (to keep PR workflow fast)
- **Destination:** Any iOS Simulator (latest)

### Environment
- **Xcode Version:** 16.0
- **Clean Build:** No (for speed)

## Workflow 2: Archive/Release

**Purpose:** Create distribution archives for TestFlight/App Store

### Start Conditions
- **Trigger (Iteration):** Manual Start
- **Trigger (Production):** Tag Changes matching `v*` OR merges to `master`

### Actions
1. **Archive - iOS**
   - Configuration: Release
   - Destination: Any iOS Device (generic iOS device)
   - Signing: Automatic
   - Team: `P52T58LHR7`
   
2. **Archive - macOS** (Optional)
   - Configuration: Release
   - Destination: Any Mac
   - Signing: Automatic
   - Team: `P52T58LHR7`

### Environment
- **Xcode Version:** 16.0 (match local)
- **Clean Build:** Yes (for release)

### Post-Actions (Optional)
- Distribute to TestFlight (configure after initial setup validated)

## Metal Toolchain Guard

**Status:** ✅ Already implemented in shared scheme

The Metal Toolchain Guard is implemented as an Archive Pre-action in the shared scheme (`BBC Model B.xcscheme`). This prevents the "cannot execute tool 'metal' due to missing Metal Toolchain" error in Xcode Cloud.

### Implementation
```xml
<PreActions>
  <ExecutionAction ActionType="Xcode.IDEStandardExecutionActionsCore.ExecutionActionType.ShellScriptAction">
    <ActionContent title="Run Script" scriptText="...">
      set -euxo pipefail
      if ! xcrun -find metal >/dev/null 2>&1; then
        echo "Installing Metal Toolchain…"
        xcodebuild -downloadComponent MetalToolchain || true
      fi
      xcodebuild -version
      xcrun -find metal || true
    </ActionContent>
  </ExecutionAction>
</PreActions>
```

This script:
1. Checks if Metal toolchain is available
2. Downloads it if missing (with error tolerance)
3. Logs Xcode version and Metal path for debugging

## Branch Protection (GitHub)

**Status:** Requires GitHub repository settings configuration

### Protection Rules for `develop` and `master`

#### Required Status Checks
- ✅ Xcode Cloud: Build
- ✅ Xcode Cloud: Test
- ✅ Xcode Cloud: Analyze

#### Additional Settings (Recommended)
- ✅ Require a pull request before merging
- ✅ Require approvals: 1 (for team environments)
- ✅ Dismiss stale pull request approvals when new commits are pushed
- ⚠️ Do not allow bypassing the above settings

### Configuration Steps
1. Navigate to: Repository → Settings → Branches
2. Add branch protection rule for `develop`:
   - Branch name pattern: `develop`
   - Enable "Require status checks to pass before merging"
   - Add required checks: Xcode Cloud workflows
   - Enable "Require branches to be up to date before merging"
3. Repeat for `master` branch

## Setup Instructions

### Step 1: Connect Repository to Xcode Cloud

1. Open `BBC Model B.xcworkspace` in Xcode
2. Select the project in the navigator
3. Navigate to: Product → Xcode Cloud → Create Workflow
4. Select "BBC Model B" scheme
5. Authenticate with Apple Developer account (Team `P52T58LHR7`)

### Step 2: Configure PR Workflow

1. In Xcode Cloud settings, create "PR Validation" workflow:
   - **Name:** PR Validation
   - **Start Conditions:** Pull Request Changes on `develop`, `master`
   - **Files & Folders:** Add filters listed above
   - **Actions:** Build → Test → Analyze
   - **Environment:** Xcode 16.0
   - **Auto-cancel:** Enabled

### Step 3: Configure Archive Workflow

1. Create "Archive/Release" workflow:
   - **Name:** Archive/Release
   - **Start Conditions:** Manual Start (later: Tag Changes `v*`)
   - **Actions:** Archive (iOS), Archive (macOS - optional)
   - **Configuration:** Release
   - **Signing:** Automatic with Team `P52T58LHR7`
   - **Environment:** Xcode 16.0

### Step 4: Verify Metal Toolchain Guard

✅ Already implemented in shared scheme - no action needed

The Archive Pre-action is committed in `BBC Model B.xcodeproj/xcshareddata/xcschemes/BBC Model B.xcscheme`

### Step 5: Enable Branch Protection

Follow GitHub Branch Protection steps above to require Xcode Cloud checks before merge.

## Verification Checklist

- [ ] Manual Start of Archive workflow succeeds in Xcode Cloud (no Metal errors)
- [ ] Test PR triggers the PR Validation workflow
- [ ] PR workflow reports status back to GitHub PR
- [ ] Xcode Cloud logs show Metal toolchain check executing
- [ ] Xcode Cloud logs show two archivable products (iOS + macOS)
- [ ] Branch protection blocks PRs until checks pass
- [ ] Scheme is shared and visible to Cloud in `xcshareddata/xcschemes/`

## Troubleshooting

### Metal Compilation Errors
- **Symptom:** "cannot execute tool 'metal' due to missing Metal Toolchain"
- **Solution:** Verify Archive Pre-action is present in shared scheme (already added)

### Workflow Not Triggering
- **Symptom:** PR created but no Xcode Cloud workflow runs
- **Solution:** Check Files & Folders filters; ensure changed files match patterns

### Archive Signing Errors
- **Symptom:** "Automatic signing is unable to resolve an issue with the target's entitlements"
- **Solution:** Verify Team `P52T58LHR7` has valid provisioning profiles and certificates

### Test Failures in Cloud but Passing Locally
- **Symptom:** Tests pass locally but fail in Xcode Cloud
- **Solution:** Check environment differences; ensure test data/resources are included in test target

## References

- [Xcode Cloud Documentation](https://developer.apple.com/documentation/xcode/xcode-cloud)
- [Configuring Workflows](https://developer.apple.com/documentation/xcode/configuring-your-first-xcode-cloud-workflow)
- [Branch Protection Rules](https://docs.github.com/en/repositories/configuring-branches-and-merges-in-your-repository/managing-protected-branches/about-protected-branches)

## Next Steps

After completing the configuration:
1. ✅ Run Manual Start of Archive workflow
2. ✅ Create test PR to validate PR workflow
3. ✅ Verify status checks appear in GitHub PR
4. ✅ Enable branch protection once workflows are stable
5. Configure TestFlight distribution (future story)
