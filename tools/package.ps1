# Package a standalone build that classmates can run without installing anything.
# Release + statically linked MSVC runtime => the exe needs no VC++ redistributable.
#
# Notes:
#  - Keep this file pure ASCII. PowerShell 5.1 reads UTF-8 without a BOM as GBK,
#    and non-ASCII bytes inside comments would break parsing.
#  - cmake is called with an argument array and "2>&1 | Out-Null": passing args as
#    an array avoids quote-escaping pain, and merging stderr keeps warnings from
#    turning into terminating errors under $ErrorActionPreference = 'Stop'.
param(
    [string]$Name = 'plant-battlegrounds'
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot

$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere)) { throw "vswhere.exe not found; install VS Build Tools" }
$vs = & $vswhere -latest -products * -property installationPath
$cmake = Join-Path $vs "Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
$buildDir = Join-Path $root 'build-dist'

# Reuse the dependency sources already fetched under build/ to avoid re-cloning.
$deps = Join-Path $root 'build\_deps'
$cacheArgs = @()
foreach ($dep in @(@('RAYLIB', 'raylib-src'), @('IMGUI', 'imgui-src'), @('RLIMGUI', 'rlimgui-src'))) {
    $src = Join-Path $deps $dep[1]
    if (Test-Path $src) {
        $cacheArgs += ('-DFETCHCONTENT_SOURCE_DIR_' + $dep[0] + '=' + $src)
    }
}

Write-Output '=== 1/4 configure (static runtime) ==='
$ErrorActionPreference = 'Continue'
& $cmake -S $root -B $buildDir -G 'Visual Studio 17 2022' -A x64 `
    -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded @cacheArgs 2>&1 | Out-Null
$code = $LASTEXITCODE
$ErrorActionPreference = 'Stop'
if ($code -ne 0) { throw "configure failed (exit $code)" }

Write-Output '=== 2/4 build Release ==='
$ErrorActionPreference = 'Continue'
$out = & $cmake --build $buildDir --config Release 2>&1
$code = $LASTEXITCODE
$ErrorActionPreference = 'Stop'
$bad = $out | Select-String -Pattern 'error C|error MSB|error LNK'
if ($bad) { $bad | Select-Object -First 15; throw 'build failed' }
if ($code -ne 0) { throw "build failed (exit $code)" }

Write-Output '=== 3/4 assemble folder ==='
$dist = Join-Path $root "dist\$Name"
Remove-Item (Join-Path $root 'dist') -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path (Join-Path $dist 'assets\plants'), (Join-Path $dist 'assets\gardeners') | Out-Null
Copy-Item (Join-Path $buildDir 'Release\plant_battlegrounds.exe') $dist -Force
Copy-Item (Join-Path $root 'assets\plants\*.png') (Join-Path $dist 'assets\plants') -Force
Copy-Item (Join-Path $root 'assets\gardeners\*.png') (Join-Path $dist 'assets\gardeners') -Force
if (Test-Path (Join-Path $PSScriptRoot 'dist_readme.txt')) {
    Copy-Item (Join-Path $PSScriptRoot 'dist_readme.txt') (Join-Path $dist 'README.txt') -Force
}

Write-Output '=== 4/4 zip ==='
$zip = Join-Path $root "dist\$Name.zip"
Compress-Archive -Path $dist -DestinationPath $zip -Force
Write-Output ('done: ' + $zip + '  (' + [math]::Round((Get-Item $zip).Length / 1KB) + ' KB)')
