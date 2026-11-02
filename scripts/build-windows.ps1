# Granny Delay - Windows build script (Visual Studio 2022 + CMake)
param(
    [string]$JucePath = "JUCE",
    [string]$Configuration = "Release",
    [switch]$Clean
)

$ErrorActionPreference = "Stop"
$projectDir = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Definition)
$buildDir = Join-Path $projectDir "build"
$jucePath = if ([System.IO.Path]::IsPathRooted($JucePath)) { $JucePath } else { Join-Path $projectDir $JucePath }

if (-not (Test-Path (Join-Path $jucePath "CMakeLists.txt"))) {
    Write-Error "JUCE not found at $jucePath`nRun: git clone --depth 1 --branch 7.0.12 https://github.com/juce-framework/JUCE.git JUCE"
}
if ($Clean -and (Test-Path $buildDir)) { Remove-Item $buildDir -Recurse -Force }

& cmake -S $projectDir -B $buildDir -A x64 "-DJUCE_PATH=$jucePath"
if ($LASTEXITCODE -ne 0) { Write-Error "CMake configuration failed!" }

& cmake --build $buildDir --config $Configuration --parallel
if ($LASTEXITCODE -ne 0) { Write-Error "Build failed!" }

Write-Host "Done. VST3: $buildDir\GrannyDelay_artefacts\$Configuration\VST3\GrannyDelay.vst3" -ForegroundColor Green
