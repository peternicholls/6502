---
title: Xcode Cloud Setup Human Actions
status: DRAFT
type: Guide
version: 1.0.0
owner: TBD
date-created: '2025-11-05'
date-updated: '2025-11-05'
description: 'TODO: Add description'
---

# Xcode Cloud Setup - Human Collaboration Required

**Story:** 1-2-hotfix-1-xcode-cloud-workflow-setup  
**Status:** Partial automation complete, manual steps required  
**Date:** 2025-11-04

## What Has Been Completed (Automated)

✅ **Metal Toolchain Guard** - Archive Pre-action added to shared scheme  
✅ **Documentation** - Complete setup guide created at `docs/xcode-cloud-setup.md`  
✅ **Shared Scheme** - Committed to version control for Xcode Cloud visibility  

## What Requires Your Action (Manual)

The following tasks require manual configuration through Xcode and GitHub web interfaces:

### 1. Connect Repository to Xcode Cloud

**Where:** Xcode → Product → Xcode Cloud → Create Workflow

**Steps:**
1. Open `BBC Model B.xcworkspace` in Xcode
2. Select the project in the navigator
3. Navigate to: **Product → Xcode Cloud → Create Workflow**
4. Select **"BBC Model B"** scheme
5. Authenticate with Apple Developer account (Team `P52T58LHR7`)

### 2. Configure PR Validation Workflow

**Where:** Xcode Cloud settings (in Xcode or App Store Connect)

**Configuration:**
- **Name:** PR Validation
- **Start Conditions:** 
  - Pull Request Changes
  - Branches: `develop`, `master`
  - Auto-cancel: ✅ Enabled
- **Files & Folders Filters:**
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
- **Actions:** 
  1. Build (Debug configuration)
  2. Test (iOS Simulator, latest)
  3. Analyze
- **Environment:** Xcode 16.0
- **Test Targets:**
  - ✅ BBC Model BTests (all unit tests)
  - ⚪ BBC Model BUITests (optional/nightly)

### 3. Configure Archive/Release Workflow

**Where:** Xcode Cloud settings

**Configuration:**
- **Name:** Archive/Release
- **Start Conditions:**
  - Manual Start (for now)
  - Later: Tag Changes matching `v*`
- **Actions:**
  1. Archive - iOS
     - Configuration: Release
     - Destination: Any iOS Device
     - Signing: Automatic, Team `P52T58LHR7`
  2. Archive - macOS (optional)
     - Configuration: Release
     - Destination: Any Mac
     - Signing: Automatic, Team `P52T58LHR7`
- **Environment:** Xcode 16.0

### 4. Test the Workflows

**Archive Workflow Test:**
1. In Xcode Cloud, trigger Manual Start of Archive workflow
2. Monitor build logs for:
   - Metal toolchain check executing (PreAction script)
   - Successful Metal compilation of `Shaders.metal`
   - Both iOS and macOS archives created
3. ✅ Verify: No "cannot execute tool 'metal'" errors

**PR Workflow Test:**
1. Create a test branch with a trivial change (e.g., update README)
2. Open a Pull Request to `develop`
3. Monitor GitHub PR for Xcode Cloud status checks
4. ✅ Verify: Build/Test/Analyze checks appear and pass

### 5. Enable GitHub Branch Protection

**Where:** GitHub Repository → Settings → Branches

**For `develop` branch:**
1. Navigate to: **Repository → Settings → Branches**
2. Click **Add branch protection rule**
3. Branch name pattern: `develop`
4. Enable **"Require status checks to pass before merging"**
5. Search and add required checks:
   - Xcode Cloud: Build
   - Xcode Cloud: Test
   - Xcode Cloud: Analyze
6. Enable **"Require branches to be up to date before merging"**
7. (Optional) Enable **"Require a pull request before merging"**
8. Save changes

**Repeat for `master` branch:**
- Same configuration as `develop`

## Verification Checklist

Use this checklist to confirm all acceptance criteria are met:

- [ ] **AC1:** PR Workflow configured for PRs to develop/master with extension/path filters
- [ ] **AC2:** Archive workflow created with Manual Start, Release config, Team P52T58LHR7
- [ ] **AC3:** Metal Toolchain guard present (✅ already in shared scheme)
- [ ] **AC4:** Branch protection enabled on develop and master requiring Cloud checks
- [ ] **AC5:** Checklist documented (✅ this document + xcode-cloud-setup.md)
- [ ] **AC6:** Config values captured in documentation (✅ in xcode-cloud-setup.md)

**Verification Sub-tasks:**
- [ ] Manual Start of Archive workflow succeeds (no Metal errors)
- [ ] Test PR triggers PR workflow
- [ ] PR workflow reports status to GitHub
- [ ] Xcode Cloud logs show Metal toolchain check
- [ ] Xcode Cloud logs show two archivable products (iOS + macOS)
- [ ] Branch protection blocks PRs until checks pass
- [ ] Scheme is shared and committed (✅ done)

## Troubleshooting Reference

If you encounter issues, see the **Troubleshooting** section in `docs/xcode-cloud-setup.md` for:
- Metal compilation errors
- Workflow not triggering
- Archive signing errors
- Test failures in Cloud

## Once Complete

After completing the manual steps and verification:

1. **Update the story file** (`docs/stories/1-2-hotfix-1-xcode-cloud-workflow-setup.md`):
   - Mark all subtasks as `[x]` completed
   - Add verification results to Dev Agent Record → Debug Log References
   - Update status to `review`

2. **Commit remaining changes:**
   ```bash
   git add docs/stories/1-2-hotfix-1-xcode-cloud-workflow-setup.md
   git commit -m "Complete Xcode Cloud CI/CD workflow setup"
   git push origin feature/1-2-hotfix-1-xcode-cloud-workflow-setup
   ```

3. **Create Pull Request:**
   - Target: `develop`
   - This will test your PR workflow!

4. **Run code review workflow** if desired:
   - `*code-review` in dev agent

## Questions?

Refer to the comprehensive guide at `docs/xcode-cloud-setup.md` for detailed configuration instructions and references.
