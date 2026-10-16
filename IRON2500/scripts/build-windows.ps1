param([ValidateSet('Debug','Release')][string]$Configuration='Release')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$build=Join-Path $root 'Build/windows'
cmake -S $root -B $build -G 'Visual Studio 17 2022' -A x64
if($LASTEXITCODE -ne 0){throw 'CMake configuration failed'}
cmake --build $build --config $Configuration --parallel 2
if($LASTEXITCODE -ne 0){throw 'Build failed'}
ctest --test-dir $build -C $Configuration --output-on-failure
if($LASTEXITCODE -ne 0){throw 'Tests failed'}
Write-Host "Plugin: $build/IRON2500_artefacts/$Configuration/VST3/IRON 2500.vst3"
Write-Host 'Copy the entire .vst3 folder into C:\Program Files\Common Files\VST3\'
