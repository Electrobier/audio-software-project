param([ValidateSet('Debug','Release')][string]$Configuration='Release')
$ErrorActionPreference='Stop'
$Source=Join-Path $PSScriptRoot "build/Psy303_artefacts/$Configuration/VST3/Psy303 Bassline.vst3"
if(!(Test-Path (Join-Path $Source 'Contents/x86_64-win/Psy303 Bassline.vst3'))){throw 'Zuerst Windows-Build ausführen. Ein Linux-VST3 funktioniert unter Windows nicht.'}
$Target=Join-Path $env:CommonProgramFiles 'VST3'
New-Item -ItemType Directory -Force -Path $Target | Out-Null
Copy-Item -LiteralPath $Source -Destination $Target -Recurse -Force
Write-Host "Installiert in $Target. Cubase neu starten oder VST-Plug-in-Manager aktualisieren."
