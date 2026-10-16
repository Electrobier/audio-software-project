$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
cmake -S $root -B "$root/Build/dsp" -DIRON_DSP_ONLY=ON
if($LASTEXITCODE -ne 0){throw 'Configuration failed'}
cmake --build "$root/Build/dsp" --config Release
if($LASTEXITCODE -ne 0){throw 'Build failed'}
ctest --test-dir "$root/Build/dsp" -C Release --output-on-failure
if($LASTEXITCODE -ne 0){throw 'Tests failed'}
