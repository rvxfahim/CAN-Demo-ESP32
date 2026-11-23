#Requires -Version 5.1
<#
Fresh-machine toolchain setup for Windows.

Installs/checks the host toolchain (Git, Python 3, CMake, a C++17 compiler,
pipx, and PlatformIO Core) and reports whether the c-coderdbc submodule and the
generated files are present.

This script does NOT build c-coderdbc, generate lib\Generated\, or build the
firmware. Students run tools\build_coderdbc.ps1 and tools\generate_code.ps1
themselves once the toolchain is ready.

By default it only checks and prints install hints; pass -InstallDeps to
install missing packages with winget/pipx (may require UAC).

Usage:
  powershell -ExecutionPolicy Bypass -File .\tools\setup.ps1
  powershell -ExecutionPolicy Bypass -File .\tools\setup.ps1 -InstallDeps
  powershell -ExecutionPolicy Bypass -File .\tools\setup.ps1 -Docs
#>
[CmdletBinding()]
param(
    [switch]$InstallDeps,
    [switch]$Docs
)

$ErrorActionPreference = 'Stop'
$RepoRoot = Split-Path -Parent $PSScriptRoot
Set-Location $RepoRoot

$missingRequired = $false
$generatedFile = 'lib\Generated\lib\lecture.c'

function Test-Tool([string]$Name) {
    return [bool](Get-Command $Name -ErrorAction SilentlyContinue)
}

function Write-Ok([string]$Text) {
    Write-Host "  [ok]   $Text" -ForegroundColor Green
}

function Write-Missing([string]$Text) {
    Write-Host "  [miss] $Text" -ForegroundColor Yellow
}

function Get-PythonCommand {
    foreach ($candidate in @('python', 'py')) {
        if (Test-Tool $candidate) {
            $null = & $candidate --version 2>&1
            if ($LASTEXITCODE -eq 0) {
                return $candidate
            }
        }
    }
    return $null
}

function Get-CppCompiler {
    foreach ($name in @('cl', 'g++', 'clang++')) {
        if (Test-Tool $name) {
            return $name
        }
    }

    # Visual Studio / Build Tools keep cl.exe off the PATH; CMake finds the
    # compiler through the Visual Studio generator, so treat an installed VC++
    # toolset as present.
    $vsWhere = $null
    if (${env:ProgramFiles(x86)}) {
        $vsWhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    }
    if ($vsWhere -and (Test-Path $vsWhere)) {
        $installPath = & $vsWhere -latest -products * `
            -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
            -property installationPath
        if ($LASTEXITCODE -eq 0 -and $installPath) {
            return 'Visual Studio C++ tools'
        }
    }

    return $null
}

function Add-KnownPaths {
    $candidates = @(
        (Join-Path $env:USERPROFILE '.platformio\penv\Scripts'),
        (Join-Path $env:USERPROFILE '.local\bin')
    )
    if ($env:ProgramFiles) {
        $candidates += Join-Path $env:ProgramFiles 'CMake\bin'
        $candidates += Join-Path $env:ProgramFiles 'Git\cmd'
    }
    if (${env:ProgramFiles(x86)}) {
        $candidates += Join-Path ${env:ProgramFiles(x86)} 'CMake\bin'
    }
    if ($env:LOCALAPPDATA) {
        $candidates += Join-Path $env:LOCALAPPDATA 'Microsoft\WindowsApps'
    }

    foreach ($candidate in $candidates) {
        if ((Test-Path $candidate) -and (($env:Path -split ';') -notcontains $candidate)) {
            $env:Path = "$candidate;$env:Path"
        }
    }

    # Python installed by winget lives under LOCALAPPDATA\Programs\Python\Python*.
    $pythonRoot = $null
    if ($env:LOCALAPPDATA) {
        $pythonRoot = Join-Path $env:LOCALAPPDATA 'Programs\Python'
    }
    if ($pythonRoot -and (Test-Path $pythonRoot)) {
        Get-ChildItem $pythonRoot -Directory -ErrorAction SilentlyContinue | ForEach-Object {
            foreach ($dir in @($_.FullName, (Join-Path $_.FullName 'Scripts'))) {
                if ((Test-Path $dir) -and (($env:Path -split ';') -notcontains $dir)) {
                    $env:Path = "$dir;$env:Path"
                }
            }
        }
    }
}

function Install-Deps {
    if (-not (Test-Tool winget)) {
        Write-Host 'winget is not available on this machine.'
        Write-Host 'Install Git, Python 3, CMake, and Visual Studio Build Tools manually, then rerun.'
        return $false
    }

    $accept = @('--accept-source-agreements', '--accept-package-agreements')

    if (-not (Test-Tool git)) {
        Write-Host 'Installing Git with winget...'
        winget install --id Git.Git -e --silent @accept
        if ($LASTEXITCODE -ne 0) {
            Write-Warning 'winget returned an error for Git.'
        }
        Add-KnownPaths
    }

    if (-not (Get-PythonCommand)) {
        Write-Host 'Installing Python 3.12 with winget...'
        winget install --id Python.Python.3.12 -e --silent @accept
        if ($LASTEXITCODE -ne 0) {
            Write-Warning 'winget returned an error for Python.'
        }
        Add-KnownPaths
    }

    if (-not (Test-Tool cmake)) {
        Write-Host 'Installing CMake with winget...'
        winget install --id Kitware.CMake -e --silent @accept
        if ($LASTEXITCODE -ne 0) {
            Write-Warning 'winget returned an error for CMake.'
        }
        Add-KnownPaths
    }

    if (-not (Get-CppCompiler)) {
        Write-Host 'Installing Visual Studio 2022 Build Tools (large, UAC-elevated)...'
        winget install --id Microsoft.VisualStudio.2022.BuildTools -e @accept `
            --override '--wait --quiet --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended'
        if ($LASTEXITCODE -ne 0) {
            Write-Warning 'winget returned an error for Visual Studio Build Tools.'
        }
    }

    Add-KnownPaths
    $pythonCmd = Get-PythonCommand
    if (-not $pythonCmd) {
        Write-Warning 'Python is still not available in this shell.'
        return $false
    }

    if (-not (Test-Tool pipx)) {
        Write-Host 'Installing pipx...'
        & $pythonCmd -m pip install --user pipx
        if ($LASTEXITCODE -ne 0) {
            Write-Warning 'pipx installation failed.'
        }
        Add-KnownPaths
    }

    if (-not (Test-Tool pio)) {
        Write-Host 'Installing PlatformIO Core with pipx...'
        & $pythonCmd -m pipx install platformio
        if ($LASTEXITCODE -ne 0) {
            Write-Host 'pipx install failed; trying pip --user as a fallback...'
            & $pythonCmd -m pip install --user platformio
            if ($LASTEXITCODE -ne 0) {
                Write-Warning 'PlatformIO installation failed.'
            }
        }
        if (Test-Tool pipx) {
            & $pythonCmd -m pipx ensurepath | Out-Null
        }
        Add-KnownPaths
    }

    Write-Host 'Dependency installation finished.'
    Write-Host 'Open a new terminal before running tools directly; this script re-probes known install locations.'
    return $true
}

Add-KnownPaths

if ($InstallDeps) {
    Write-Host 'Installing missing dependencies...'
    if (-not (Install-Deps)) {
        Write-Host 'Dependency installation did not complete; continuing with checks.' -ForegroundColor Yellow
    }
    Add-KnownPaths
    Write-Host ''
}

$python = Get-PythonCommand
$compiler = Get-CppCompiler

Write-Host 'CAN-Demo-ESP32 setup check'
Write-Host "Repository: $RepoRoot"
Write-Host ''

Write-Host 'Required tools:'
if (Test-Tool git) {
    $gitVersion = (& git --version | Select-Object -First 1)
    Write-Ok "git ($gitVersion)"
}
else {
    Write-Missing 'git'
    $missingRequired = $true
}

if (Test-Tool pio) {
    $pioVersion = (& pio --version 2>$null | Select-Object -First 1)
    Write-Ok "PlatformIO Core ($pioVersion)"
}
else {
    Write-Missing 'PlatformIO Core CLI (pio)'
    $missingRequired = $true
}

if ($python) {
    $pythonVersion = (& $python --version 2>&1 | Select-Object -First 1)
    Write-Ok "$pythonVersion"
}
else {
    Write-Missing 'Python 3 (needed for the PlatformIO CLI and docs)'
    # PlatformIO can be installed by the VS Code extension with its own Python;
    # only make this fatal when the CLI itself is missing too.
    if (-not (Test-Tool pio)) {
        $missingRequired = $true
    }
}
Write-Host ''

Write-Host 'Code generation toolchain (used by tools\build_coderdbc.ps1):'
if (Test-Tool cmake) {
    $cmakeVersion = (& cmake --version | Select-Object -First 1)
    Write-Ok "cmake ($cmakeVersion)"
}
else {
    Write-Missing 'cmake'
    $missingRequired = $true
}

if ($compiler) {
    Write-Ok "C++ compiler ($compiler)"
}
else {
    Write-Missing 'C++ compiler (Visual Studio Build Tools with the C++ workload, g++, or clang++)'
    $missingRequired = $true
}
Write-Host ''

Write-Host 'Project status:'
if (Test-Path 'tools\c-coderdbc\src\CMakeLists.txt') {
    Write-Ok 'tools\c-coderdbc submodule'
}
else {
    Write-Missing 'tools\c-coderdbc submodule (tools\build_coderdbc.ps1 will initialize it)'
}

if (Test-Path $generatedFile) {
    Write-Ok "generated code ($generatedFile)"
}
else {
    Write-Missing "$generatedFile (run the student workflow below)"
}
Write-Host ''

if ($missingRequired) {
    Write-Host 'Install hints (Windows):'
    Write-Host '  Git:       https://git-scm.com/download/win'
    Write-Host '  Python:    https://www.python.org/downloads/windows/ (enable "Add python.exe to PATH")'
    Write-Host '  PlatformIO:'
    Write-Host '    py -m pip install --user pipx'
    Write-Host '    py -m pipx ensurepath'
    Write-Host '    py -m pipx install platformio'
    Write-Host '  Official guide: https://docs.platformio.org/en/latest/core/installation/index.html'
    Write-Host '  Alternative: install the PlatformIO IDE extension in VS Code.'
    Write-Host '  Code generation: CMake + Visual Studio Build Tools with the'
    Write-Host '    "Desktop development with C++" workload (used by build_coderdbc.ps1).'
    Write-Host '  USB-UART driver: install the driver for your board''s bridge chip'
    Write-Host '    (CP210x, FTDI, or CH340 are common).'
    Write-Host '  Or rerun with: powershell -ExecutionPolicy Bypass -File .\tools\setup.ps1 -InstallDeps'
    Write-Host ''
}

# ---------------------------------------------------------------------------
# Serial ports
# ---------------------------------------------------------------------------
if (Test-Tool pio) {
    Write-Host 'Serial ports:'
    pio device list
    Write-Host ''
}

Write-Host @'
Student workflow (run once the required tools above are available; this script
stops after toolchain setup):
  1. Build the DBC code generator:
       powershell -ExecutionPolicy Bypass -File .\tools\build_coderdbc.ps1
  2. Generate the CAN code from tools\Lecture.dbc:
       powershell -ExecutionPolicy Bypass -File .\tools\generate_code.ps1
  3. Implement the application layer using the generated API in
     lib\Generated\lib\lecture.h. On the TX board, complete
     SendClusterFrame() in src\tx\main.cpp (the lecture exercise).

See README.md ("Build, flash, and monitor") when your implementation is ready.
'@

# ---------------------------------------------------------------------------
# Optional documentation build
# ---------------------------------------------------------------------------
if ($Docs) {
    Write-Host ''
    Write-Host 'Building documentation...'
    if ($python) {
        & $python -m venv .venv-docs
        if ($LASTEXITCODE -ne 0) {
            throw 'Could not create .venv-docs.'
        }
        & .\.venv-docs\Scripts\python.exe -m pip install --upgrade pip
        & .\.venv-docs\Scripts\python.exe -m pip install -r docs\requirements.txt
        & .\.venv-docs\Scripts\python.exe -m sphinx -b html docs docs\_build\html
        if ($LASTEXITCODE -ne 0) {
            throw 'Sphinx documentation build failed.'
        }
        Write-Host 'Documentation written to docs\_build\html'
    }
    else {
        Write-Host 'Skipping documentation: Python not found.' -ForegroundColor Yellow
    }
}

if ($missingRequired) {
    exit 1
}
exit 0
