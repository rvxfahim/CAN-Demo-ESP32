#Requires -Version 5.1
<#
Generates lib\Generated\ from tools\Lecture.dbc using the coderdbc executable.

Run tools\build_coderdbc.ps1 first. This script initializes the
tools\c-coderdbc submodule when it is missing.

Usage:
  powershell -ExecutionPolicy Bypass -File .\tools\generate_code.ps1
#>
[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'

$ToolsDir = $PSScriptRoot
$RepoRoot = Split-Path -Parent $ToolsDir
$SubmoduleDir = Join-Path $ToolsDir 'c-coderdbc'
$DbcFile = Join-Path $ToolsDir 'Lecture.dbc'
$OutDir = Join-Path $RepoRoot 'lib\Generated'

function Test-Tool([string]$Name) {
    return [bool](Get-Command $Name -ErrorAction SilentlyContinue)
}

function Initialize-CoderdbcSubmodule {
    if (Test-Path (Join-Path $SubmoduleDir 'src\CMakeLists.txt')) {
        return
    }

    Write-Host 'tools\c-coderdbc is missing; initializing the git submodule...'
    if (-not (Test-Tool git)) {
        throw 'Git is not installed, so the c-coderdbc submodule cannot be initialized. Install Git and rerun, or clone the repository with --recursive.'
    }
    if (-not (Test-Path (Join-Path $RepoRoot '.git'))) {
        throw 'This does not look like a git checkout, so the submodule cannot be initialized. Clone the repository with: git clone --recursive https://github.com/rvxfahim/CAN-Demo-ESP32.git'
    }

    & git -C $RepoRoot submodule update --init --recursive
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to initialize tools\c-coderdbc. Check your network connection, or run 'git submodule update --init --recursive' manually and rerun."
    }
    if (-not (Test-Path (Join-Path $SubmoduleDir 'src\CMakeLists.txt'))) {
        throw 'tools\c-coderdbc is still empty after git submodule update.'
    }
}

if (-not (Test-Path $DbcFile)) {
    throw "$DbcFile is missing."
}

Initialize-CoderdbcSubmodule

# Support both single-config and multi-config CMake generators.
$candidates = @(
    'build\coderdbc.exe',
    'build\coderdbc',
    'build\Release\coderdbc.exe',
    'build\Release\coderdbc',
    'build\Debug\coderdbc.exe',
    'build\Debug\coderdbc'
)
$binary = $null
foreach ($candidate in $candidates) {
    $fullPath = Join-Path $SubmoduleDir $candidate
    if (Test-Path $fullPath) {
        $binary = $fullPath
        break
    }
}

if (-not $binary) {
    throw 'coderdbc executable not found under tools\c-coderdbc\build\. Run .\tools\build_coderdbc.ps1 first.'
}

Write-Host 'Running coderdbc on Lecture.dbc...'
Push-Location $SubmoduleDir
try {
    & $binary -dbc $DbcFile -out $OutDir -drvname lecture -rw -nodeutils
    if ($LASTEXITCODE -ne 0) {
        throw 'Code generation failed.'
    }
}
finally {
    Pop-Location
}

$generatedFile = Join-Path $OutDir 'lib\lecture.c'
if (-not (Test-Path $generatedFile)) {
    throw "Code generation reported success but $generatedFile is missing."
}

Write-Host "Code generation successful. Files written to $OutDir"
