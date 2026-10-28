param([ValidateSet('Debug','Release')][string]$Configuration='Release')
$ErrorActionPreference='Stop'
$Root=$PSScriptRoot
cmake -S $Root -B "$Root/build" -G 'Visual Studio 17 2022' -A x64
if($LASTEXITCODE -ne 0){throw 'CMake-Konfiguration fehlgeschlagen'}
cmake --build "$Root/build" --config $Configuration --parallel 2
if($LASTEXITCODE -ne 0){throw 'Build fehlgeschlagen'}
ctest --test-dir "$Root/build" -C $Configuration --output-on-failure
if($LASTEXITCODE -ne 0){throw 'Tests fehlgeschlagen'}
Write-Host "VST3: $Root/build/Psy303_artefacts/$Configuration/VST3/Psy303 Bassline.vst3"
