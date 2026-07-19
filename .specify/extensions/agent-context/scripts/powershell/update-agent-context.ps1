#!/usr/bin/env pwsh
# update-agent-context.ps1
#
# Refresh the managed Spec Kit section in the coding agent's context file
# (e.g. CLAUDE.md, .github/copilot-instructions.md, AGENTS.md).
#
# Reads `context_file` and `context_markers.{start,end}` from the
# agent-context extension config:
#   .specify/extensions/agent-context/agent-context-config.yml
#
# Usage: update-agent-context.ps1 [plan_path]

[CmdletBinding()]
param(
    [Parameter(Position = 0)]
    [string]$PlanPath
)

function Get-ConfigValue {
    param(
        [AllowNull()][object]$Object,
        [Parameter(Mandatory = $true)][string]$Key
    )

    if ($null -eq $Object) {
        return $null
    }
    if ($Object -is [System.Collections.IDictionary]) {
        return $Object[$Key]
    }
    $prop = $Object.PSObject.Properties[$Key]
    if ($prop) {
        return $prop.Value
    }
    return $null
}

function Test-ConfigObject {
    param(
        [AllowNull()][object]$Object
    )

    if ($null -eq $Object) {
        return $false
    }
    if ($Object -is [System.Collections.IDictionary]) {
        return $true
    }
    if ($Object -is [System.Management.Automation.PSCustomObject]) {
        return $true
    }
    return $false
}

$ErrorActionPreference = 'Stop'
$DefaultStart = '<!-- SPECKIT START -->'
$DefaultEnd   = '<!-- SPECKIT END -->'
$ProjectRoot  = (Get-Location).Path
$ExtConfig    = Join-Path $ProjectRoot '.specify/extensions/agent-context/agent-context-config.yml'

if (-not (Test-Path -LiteralPath $ExtConfig)) {
    Write-Warning "agent-context: $ExtConfig not found; nothing to do."
    exit 0
}

$Options = $null
if (Get-Command ConvertFrom-Yaml -ErrorAction SilentlyContinue) {
    try {
        $Options = Get-Content -LiteralPath $ExtConfig -Raw | ConvertFrom-Yaml -ErrorAction Stop
    } catch {
        # fall through to Python fallback
    }
}

if ($null -eq $Options) {
    # ConvertFrom-Yaml unavailable or failed; fall back to Python+PyYAML.
    $pythonCmd = $null
    foreach ($candidate in @('python3', 'python')) {
        if (Get-Command $candidate -ErrorAction SilentlyContinue) {
            # Verify it is Python 3
            $verOut = & $candidate --version 2>&1
            if ($verOut -match 'Python 3') {
                $pythonCmd = $candidate
                break
            }
        }
    }

    if ($pythonCmd) {
        try {
            $jsonOut = & $pythonCmd -c @'
import json
import sys
try:
    import yaml
except ImportError:
    print(
        "agent-context: PyYAML is required to parse extension config; cannot update context.",
        file=sys.stderr,
    )
    sys.exit(2)

try:
    with open(sys.argv[1], "r", encoding="utf-8") as fh:
        data = yaml.safe_load(fh)
except Exception as exc:
    print(
        f"agent-context: unable to parse {sys.argv[1]} ({exc}); cannot update context.",
        file=sys.stderr,
    )
    sys.exit(2)

if not isinstance(data, dict):
    data = {}

print(json.dumps(data))
'@ $ExtConfig
            if ($LASTEXITCODE -eq 0 -and $jsonOut) {
                $Options = $jsonOut | ConvertFrom-Json -ErrorAction Stop
            }
        } catch {
            $Options = $null
        }
    }

    if (-not $Options) {
        Write-Warning "agent-context: unable to parse $ExtConfig; skipping update."
        exit 0
    }
}

if (-not (Test-ConfigObject -Object $Options)) {
    Write-Warning "agent-context: $ExtConfig must contain a YAML mapping; skipping update."
    exit 0
}

$ContextFile = Get-ConfigValue -Object $Options -Key 'context_file'
if (-not $ContextFile) {
    Write-Warning 'agent-context: context_file not set in extension config; nothing to do.'
    exit 0
}

# Reject absolute paths and '..' path segments in context_file
if ([System.IO.Path]::IsPathRooted($ContextFile)) {
    Write-Warning "agent-context: context_file must be a project-relative path; got '$ContextFile'."
    exit 1
}
$cfSegments = $ContextFile -split '[/\\]'
if ($cfSegments -contains '..') {
    Write-Warning "agent-context: context_file must not contain '..' path segments; got '$ContextFile'."
    exit 1
}

$MarkerStart = $DefaultStart
$MarkerEnd   = $DefaultEnd
$cm = Get-ConfigValue -Object $Options -Key 'context_markers'
if ($cm) {
    $cmStart = Get-ConfigValue -Object $cm -Key 'start'
    if ($cmStart -is [string] -and $cmStart) {
        $MarkerStart = $cmStart
    }
    $cmEnd = Get-ConfigValue -Object $cm -Key 'end'
    if ($cmEnd -is [string] -and $cmEnd) {
        $MarkerEnd = $cmEnd
    }
}

$FeatureDir = ''
$featurePointer = Join-Path $ProjectRoot '.specify/feature.json'
if (Test-Path -LiteralPath $featurePointer) {
    try {
        $featureData = Get-Content -LiteralPath $featurePointer -Raw | ConvertFrom-Json -ErrorAction Stop
        if ($featureData.feature_directory) {
            $FeatureDir = [string]$featureData.feature_directory
        }
    } catch {
        Write-Warning "agent-context: unable to read $featurePointer; refusing to update context."
        exit 1
    }
}

if ($FeatureDir) {
    $featureSegments = $FeatureDir -split '/'
    if ([System.IO.Path]::IsPathRooted($FeatureDir) -or
        $featureSegments.Count -ne 2 -or
        $featureSegments[0] -ne 'specs' -or
        $featureSegments[1] -notmatch '^[A-Za-z0-9][A-Za-z0-9._-]*$' -or
        $featureSegments -contains '..') {
        Write-Warning "agent-context: invalid feature_directory '$FeatureDir'."
        exit 1
    }
    $expectedPlan = "$FeatureDir/plan.md"
    if ($PlanPath -and $PlanPath.Replace('\','/') -ne $expectedPlan) {
        Write-Warning "agent-context: plan '$PlanPath' does not belong to active feature '$FeatureDir'."
        exit 1
    }
    if (-not $PlanPath -and (Test-Path -LiteralPath (Join-Path $ProjectRoot $expectedPlan))) {
        $PlanPath = $expectedPlan
    }
} elseif ($PlanPath) {
    Write-Warning "agent-context: no active feature; refusing explicit plan '$PlanPath'."
    exit 1
}

$CtxPath = Join-Path $ProjectRoot $ContextFile
$CtxDir  = Split-Path -Parent $CtxPath
if ($CtxDir -and -not (Test-Path -LiteralPath $CtxDir)) {
    New-Item -ItemType Directory -Path $CtxDir -Force | Out-Null
}

$lines = @($MarkerStart)
if (-not $FeatureDir) {
    $lines += 'No Spec Kit feature is currently active.'
    $lines += 'docs/product/MACHINE_DELIVERY_PLAN.md is the sole forward programme authority.'
    $lines += 'Select a named bounded slice from it, then create its feature branch and verify'
    $lines += '.specify/feature.json before running plan, tasks or implementation workflows.'
    $lines += 'Do not select work independently from supporting catalogues, archived documents,'
    $lines += 'or completed feature artifacts.'
} else {
    $lines += "Active Spec Kit feature: $FeatureDir"
    $lines += 'Its scope must trace to a named row or gate in'
    $lines += 'docs/product/MACHINE_DELIVERY_PLAN.md, the sole forward programme authority.'
    if ($PlanPath) {
        $lines += "Read the current implementation plan at $PlanPath"
    } else {
        $lines += 'No implementation plan exists yet; complete specification before planning.'
    }
}
$lines += $MarkerEnd
$Section = ($lines -join "`n") + "`n"

if (Test-Path -LiteralPath $CtxPath) {
    $rawBytes = [System.IO.File]::ReadAllBytes($CtxPath)
    # Strip UTF-8 BOM if present
    if ($rawBytes.Length -ge 3 -and $rawBytes[0] -eq 0xEF -and $rawBytes[1] -eq 0xBB -and $rawBytes[2] -eq 0xBF) {
        $content = [System.Text.Encoding]::UTF8.GetString($rawBytes, 3, $rawBytes.Length - 3)
    } else {
        $content = [System.Text.Encoding]::UTF8.GetString($rawBytes)
    }

    $s = $content.IndexOf($MarkerStart)
    $e = if ($s -ge 0) { $content.IndexOf($MarkerEnd, $s) } else { $content.IndexOf($MarkerEnd) }

    if ($s -ge 0 -and $e -ge 0 -and $e -gt $s) {
        $endOfMarker = $e + $MarkerEnd.Length
        if ($endOfMarker -lt $content.Length -and $content[$endOfMarker] -eq "`r") { $endOfMarker++ }
        if ($endOfMarker -lt $content.Length -and $content[$endOfMarker] -eq "`n") { $endOfMarker++ }
        $newContent = $content.Substring(0, $s) + $Section + $content.Substring($endOfMarker)
    } elseif ($s -ge 0) {
        $newContent = $content.Substring(0, $s) + $Section
    } elseif ($e -ge 0) {
        $endOfMarker = $e + $MarkerEnd.Length
        if ($endOfMarker -lt $content.Length -and $content[$endOfMarker] -eq "`r") { $endOfMarker++ }
        if ($endOfMarker -lt $content.Length -and $content[$endOfMarker] -eq "`n") { $endOfMarker++ }
        $newContent = $Section + $content.Substring($endOfMarker)
    } else {
        if ($content -and -not $content.EndsWith("`n")) { $content += "`n" }
        if ($content) { $newContent = $content + "`n" + $Section } else { $newContent = $Section }
    }
} else {
    $newContent = $Section
}

$newContent = $newContent.Replace("`r`n", "`n").Replace("`r", "`n")
[System.IO.File]::WriteAllText($CtxPath, $newContent, (New-Object System.Text.UTF8Encoding($false)))

Write-Host "agent-context: updated $ContextFile"
