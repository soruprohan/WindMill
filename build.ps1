# Windmill Farm - build script (MSYS2 MinGW-w64 64-bit toolchain).
#
#   .\build.ps1          build only
#   .\build.ps1 -Run     build, then run
#   .\build.ps1 -Clean   delete build/ and rebuild
#
# Compiles every .cpp and .c in src/ (so glad.c and each new class from later
# phases are picked up automatically - no editing this file when you add one).

param(
    [switch]$Run,
    [switch]$Clean
)

$ErrorActionPreference = 'Stop'

$mingwBin = 'C:\msys64\mingw64\bin'
# MSYS2 first, so the program loads its DLLs instead of any older C:\MinGW ones.
$env:Path = "$mingwBin;$env:Path"

$root     = $PSScriptRoot
$srcDir   = Join-Path $root 'src'
$buildDir = Join-Path $root 'build'
$exe      = Join-Path $buildDir 'WindmillFarm.exe'

$gxx = Join-Path $mingwBin 'g++.exe'
$gcc = Join-Path $mingwBin 'gcc.exe'

$includeFlags = @("-I$root\include", "-I$root\libs", "-I$mingwBin\..\include")
$cxxFlags     = @('-std=c++17', '-Wall') + $includeFlags
$cFlags       = $includeFlags
$libs         = @('-lglfw3', '-lopengl32', '-lgdi32')

if ($Clean -and (Test-Path -LiteralPath $buildDir)) {
    Remove-Item -LiteralPath $buildDir -Recurse -Force
}
if (-not (Test-Path -LiteralPath $buildDir)) {
    New-Item -ItemType Directory -Path $buildDir | Out-Null
}

# --- compile ------------------------------------------------------------
$sources = Get-ChildItem -LiteralPath $srcDir -File |
           Where-Object { $_.Extension -in '.cpp', '.c' }

if (-not $sources) { Write-Host 'No sources in src/' -ForegroundColor Red; exit 1 }

$objects = @()
$compiled = 0

foreach ($s in $sources) {
    $obj = Join-Path $buildDir ($s.BaseName + '.o')
    $objects += $obj

    # Recompile only when the source is newer than its object file.
    # Headers change often in this project, so a header edit forces a full pass.
    $newestHeader = (Get-ChildItem -LiteralPath $srcDir -File -Filter '*.h' -ErrorAction SilentlyContinue |
                     Measure-Object LastWriteTime -Maximum).Maximum
    $upToDate = (Test-Path -LiteralPath $obj) -and
                ((Get-Item -LiteralPath $obj).LastWriteTime -gt $s.LastWriteTime) -and
                (-not $newestHeader -or (Get-Item -LiteralPath $obj).LastWriteTime -gt $newestHeader)
    if ($upToDate) { continue }

    Write-Host "  CC  $($s.Name)" -ForegroundColor DarkGray
    if ($s.Extension -eq '.c') { & $gcc @cFlags   -c $s.FullName -o $obj }
    else                       { & $gxx @cxxFlags -c $s.FullName -o $obj }
    if ($LASTEXITCODE -ne 0) { Write-Host "Compile failed: $($s.Name)" -ForegroundColor Red; exit 1 }
    $compiled++
}

# --- link ---------------------------------------------------------------
if ($compiled -gt 0 -or -not (Test-Path -LiteralPath $exe)) {
    Write-Host "  LD  WindmillFarm.exe" -ForegroundColor DarkGray
    & $gxx @objects -o $exe @libs
    if ($LASTEXITCODE -ne 0) { Write-Host 'Link failed' -ForegroundColor Red; exit 1 }
}

Write-Host "Build OK -> build\WindmillFarm.exe" -ForegroundColor Green

# --- run ----------------------------------------------------------------
# Run from the project root so "shaders/..." and "textures/..." resolve.
if ($Run) {
    Push-Location -LiteralPath $root
    try { & $exe } finally { Pop-Location }
    exit $LASTEXITCODE
}
