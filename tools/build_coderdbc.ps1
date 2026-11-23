#Requires -Version 5.1
<#
Builds the c-coderdbc DBC code generator.

Initializes the tools\c-coderdbc git submodule when it is missing, then runs
CMake. Run tools\generate_code.ps1 afterwards to generate lib\Generated\.

Usage:
  powershell -ExecutionPolicy Bypass -File .\tools\build_coderdbc.ps1
#>
[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'

$ToolsDir = $PSScriptRoot
$RepoRoot = Split-Path -Parent $ToolsDir
$SubmoduleDir = Join-Path $ToolsDir 'c-coderdbc'

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

Initialize-CoderdbcSubmodule

if (-not (Test-Tool cmake)) {
    throw 'CMake is not on PATH. Run: powershell -ExecutionPolicy Bypass -File .\tools\setup.ps1 -InstallDeps'
}

Write-Host 'Building c-coderdbc...'
Push-Location $SubmoduleDir
try {
    cmake -S src -B build -DCMAKE_BUILD_TYPE=Release
    if ($LASTEXITCODE -ne 0) {
        throw 'CMake configure failed.'
    }

    cmake --build build --config Release
    if ($LASTEXITCODE -ne 0) {
        throw 'CMake build failed.'
    }
}
finally {
    Pop-Location
}

# Different CMake generators put the binary in different places:
#   - Single-config (MinGW Makefiles, Ninja): build\coderdbc.exe
#   - Multi-config (Visual Studio):           build\<Config>\coderdbc.exe
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
    throw 'coderdbc executable not found under build\. Expected build\coderdbc.exe or build\Release\coderdbc.exe.'
}

Write-Host "coderdbc built successfully: $binary"
