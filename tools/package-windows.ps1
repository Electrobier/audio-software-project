$ErrorActionPreference='Stop'
$Root=Split-Path $PSScriptRoot -Parent
$Stage=Join-Path $Root 'dist/Psy303-Windows-x64'
$Artifacts=Join-Path $Root 'build/Psy303_artefacts/Release'
$Binary=Join-Path $Artifacts 'VST3/Psy303 Bassline.vst3/Contents/x86_64-win/Psy303 Bassline.vst3'
if(!(Test-Path -LiteralPath $Binary)){throw 'Windows VST3 binary missing'}
$Bytes=[System.IO.File]::ReadAllBytes($Binary)
if($Bytes[0] -ne 77 -or $Bytes[1] -ne 90){throw 'Not a Windows PE binary'}
New-Item -ItemType Directory -Force -Path $Stage | Out-Null
Copy-Item -LiteralPath (Join-Path $Artifacts 'VST3') -Destination $Stage -Recurse -Force
Copy-Item -LiteralPath (Join-Path $Artifacts 'Standalone') -Destination $Stage -Recurse -Force
Copy-Item -LiteralPath (Join-Path $Root 'RenderedLibrary') -Destination (Join-Path $Stage 'Library') -Recurse -Force
Copy-Item -LiteralPath (Join-Path $Root 'docs') -Destination $Stage -Recurse -Force
Copy-Item -LiteralPath (Join-Path $Root 'README.md') -Destination $Stage
Copy-Item -LiteralPath (Join-Path $Root 'LICENSE') -Destination $Stage
@'
PSY303 BASSLINE - WINDOWS x64

1. ZIP vollstaendig entpacken.
2. Cubase schliessen.
3. Den gesamten Ordner VST3\Psy303 Bassline.vst3 nach
   C:\Program Files\Common Files\VST3\ kopieren.
4. Cubase starten und im VST-Plug-in-Manager neu scannen.
5. Instrumentenspur mit Psy303 Bassline anlegen.

Alternativ Install-VST3.ps1 in einer PowerShell als Administrator ausfuehren.
Standalone\Psy303 Bassline.exe kann ohne Cubase gestartet werden.
Standalone dient zum separaten Spielen; das VST3-Bundle ist fuer Cubase.
Library enthaelt 32 MIDI-Patterns, 32 Bassline-WAVs und 10 Kicks.

Host starten: Sequencer synchronisiert mit Cubase. Fuer MIDI-Spiel EXTERNAL MIDI.
Bei gestopptem Host aktiviert PLAY / STOP den Freilauf.
Cubase, Audiohardware und native DAW-Bedienung wurden nicht manuell getestet.
Die automatisch ausgefuehrten Windows-Tests stehen unter docs.
'@ | Set-Content -LiteralPath (Join-Path $Stage 'INSTALLATION.txt') -Encoding utf8
@'
$ErrorActionPreference='Stop'
$Source=Join-Path $PSScriptRoot 'VST3/Psy303 Bassline.vst3'
$Target=Join-Path $env:CommonProgramFiles 'VST3'
if(!(Test-Path -LiteralPath (Join-Path $Source 'Contents/x86_64-win/Psy303 Bassline.vst3'))){throw 'VST3-Bundle fehlt. ZIP zuerst vollstaendig entpacken.'}
New-Item -ItemType Directory -Force -Path $Target | Out-Null
Copy-Item -LiteralPath $Source -Destination $Target -Recurse -Force
Write-Host "Psy303 Bassline installiert in $Target. Cubase jetzt neu starten."
'@ | Set-Content -LiteralPath (Join-Path $Stage 'Install-VST3.ps1') -Encoding utf8
$SHA=Get-FileHash -LiteralPath $Binary -Algorithm SHA256
"$($SHA.Hash.ToLower())  VST3/Psy303 Bassline.vst3/Contents/x86_64-win/Psy303 Bassline.vst3" | Set-Content -LiteralPath (Join-Path $Stage 'SHA256.txt') -Encoding ascii
Write-Host "Windows package staged: $Stage"
