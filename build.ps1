[CmdletBinding()]
param(
    [Parameter(Position = 0)]
    [string]$Action = 'make'
)

$ErrorActionPreference = 'Stop'
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $scriptDir

$msysRoot = 'C:\msys64'
$mingwBin = Join-Path $msysRoot 'mingw64\bin'
$makeExe = Join-Path $mingwBin 'mingw32-make.exe'
$cmakeExe = Join-Path $mingwBin 'cmake.exe'

if (-not (Test-Path $mingwBin)) {
    Write-Error "MSYS2 MinGW toolchain was not found at $msysRoot. Install MSYS2 and the required packages first."
    exit 1
}

$env:PATH = "$mingwBin;$env:PATH"

function Show-Help {
    Write-Host 'Insurance Management System — Windows build wrapper'
    Write-Host ''
    Write-Host 'Usage:'
    Write-Host '  .\build.ps1            Build with make (default)'
    Write-Host '  .\build.ps1 cmake      Build with CMake'
    Write-Host '  .\build.ps1 clean      Remove build artefacts'
    Write-Host '  .\build.ps1 run        Build and run the application'
    Write-Host '  .\build.ps1 help       Show this help message'
    Write-Host ''
    Write-Host 'If ./build.sh fails with a cygheap base mismatch on Windows, use this wrapper instead.'
}

switch ($Action.ToLowerInvariant()) {
    'make' {
        & $makeExe all
    }
    'cmake' {
        if (-not (Test-Path $cmakeExe)) {
            Write-Error 'cmake.exe was not found. Install it with: pacman -S mingw-w64-x86_64-cmake'
            exit 1
        }
        & $cmakeExe -S . -B build -G 'MinGW Makefiles'
        & $cmakeExe --build build
    }
    'clean' {
        if (Test-Path $makeExe) {
            & $makeExe clean 2>$null
        }
        if (Test-Path 'build') {
            Remove-Item 'build' -Recurse -Force
        }
        if (Test-Path 'src') {
            Get-ChildItem 'src' -Filter *.o -File | Remove-Item -Force
        }
        if (Test-Path 'insurance_system.exe') {
            Remove-Item 'insurance_system.exe' -Force
        }
    }
    'run' {
        & $makeExe all
        if (Test-Path 'insurance_system.exe') {
            & .\insurance_system.exe
        }
        else {
            Write-Error 'Executable was not produced.'
            exit 1
        }
    }
    'help' {
        Show-Help
    }
    default {
        Write-Error "Unknown action: '$Action'"
        Show-Help
        exit 1
    }
}
