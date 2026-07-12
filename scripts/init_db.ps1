param(
    [string]$DatabasePath = "insurance.db"
)

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RepoRoot = Resolve-Path "$ScriptDir\.."
$DbFile = Join-Path $RepoRoot $DatabasePath

Set-Location $RepoRoot

if (Test-Path $DbFile) {
    Write-Host "Removing existing database: $DbFile"
    Remove-Item $DbFile -Force
}

if (-not (Test-Path "build")) {
    New-Item -ItemType Directory -Path "build" | Out-Null
}

cmake -S . -B build
cmake --build build --config Release

Write-Host "Running migration mode..."
& "$(Join-Path $RepoRoot 'build\insurance_system.exe')" --migrate

Write-Host "Database initialized at $DbFile."
