param([string]$Destination = "Build/Release", [string]$JuceSource = "")
$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest
$root = Split-Path $PSScriptRoot -Parent
Push-Location $root
try {
  foreach ($tool in @("cmake", "git")) {
    if (-not (Get-Command $tool -ErrorAction SilentlyContinue)) { throw "Fehlt: $tool. Siehe README.md." }
    & $tool --version
    if ($LASTEXITCODE -ne 0) { throw "Versionspruefung fehlgeschlagen: $tool" }
  }
  $build = Join-Path $root "Build/msvc"
  $options = @("-S", $root, "-B", $build, "-G", "Visual Studio 17 2022", "-A", "x64")
  if ($JuceSource) { $options += "-DJUCE_SOURCE_DIR=$JuceSource" }
  & cmake @options
  if ($LASTEXITCODE -ne 0) { throw "CMake-Konfiguration fehlgeschlagen" }
  & cmake --build $build --config Release --parallel 2
  if ($LASTEXITCODE -ne 0) { throw "Build fehlgeschlagen" }
  $dest = [System.IO.Path]::GetFullPath((Join-Path $root $Destination))
  if (Test-Path $dest) { throw "Zielordner existiert bereits. Zum Schutz vorhandener Dateien ein neues Ziel angeben: $dest" }
  New-Item -ItemType Directory -Path $dest | Out-Null
  & "$build/Release/white224_tests.exe" "$dest/Messungen" 2>&1 | Tee-Object "$dest/DSP-Tests.txt"
  if ($LASTEXITCODE -ne 0) { throw "DSP-Tests fehlgeschlagen" }
  & "$build/Release/white224_state_tests.exe" "$dest/Oberflaeche.png" 2>&1 | Tee-Object "$dest/State-Tests.txt"
  if ($LASTEXITCODE -ne 0) { throw "State-Tests fehlgeschlagen" }
  $bundle = "$build/White224_artefacts/Release/VST3/White224 Vintage Reverb.vst3"
  $exe = "$build/White224_artefacts/Release/Standalone/White224 Vintage Reverb.exe"
  if (-not (Test-Path "$bundle/Contents/x86_64-win/White224 Vintage Reverb.vst3")) { throw "VST3-Bundle fehlt" }
  if (-not (Test-Path $exe)) { throw "Standalone fehlt" }
  Copy-Item -Recurse $bundle $dest
  Copy-Item $exe $dest
  Copy-Item "$root/README.md", "$root/LICENSE", "$root/THIRD_PARTY.md" $dest
  Copy-Item -Recurse "$root/docs" $dest
  $process = Start-Process -FilePath (Join-Path $dest "White224 Vintage Reverb.exe") -PassThru
  Start-Sleep -Seconds 5
  if ($process.HasExited) { throw "Standalone vorzeitig beendet; ExitCode $($process.ExitCode)" }
  $null = $process.CloseMainWindow()
  if (-not $process.WaitForExit(5000)) { $process.Kill() }
  "Windows x64 Build und automatisierte Tests erfolgreich. Standalone-Prozess blieb beim Start 5 s aktiv. Kein Cubase-/Audiohardware-Nachweis." | Set-Content "$dest/WINDOWS-BUILD.txt"
  # Ship corresponding project and JUCE sources with AGPL-based binaries.
  $source = Join-Path $dest "Sources/White224"
  New-Item -ItemType Directory -Path $source -Force | Out-Null
  foreach ($item in @("src", "tests", "scripts", "docs", "assets", "reference", "CMakeLists.txt", "README.md", "LICENSE", "THIRD_PARTY.md")) {
    Copy-Item -Recurse (Join-Path $root $item) $source
  }
  $juceArchive = Join-Path $dest "Sources/JUCE-8.0.6.tar.gz"
  Invoke-WebRequest -Uri "https://codeload.github.com/juce-framework/JUCE/tar.gz/refs/tags/8.0.6" -OutFile $juceArchive
  $cmakeText = Get-Content (Join-Path $root "CMakeLists.txt") -Raw
  $expectedJuce = [regex]::Match($cmakeText, 'SHA256=([a-f0-9]{64})').Groups[1].Value
  if (-not $expectedJuce -or (Get-FileHash $juceArchive -Algorithm SHA256).Hash.ToLowerInvariant() -ne $expectedJuce) { throw "JUCE-Quellarchiv-Pruefsumme ungueltig" }
  $jucePath = if ($JuceSource) { $JuceSource } elseif (Test-Path "$root/third_party/JUCE") { "$root/third_party/JUCE" } else { "$build/_deps/juce-src" }
  Copy-Item (Join-Path $jucePath "LICENSE.md") (Join-Path $dest "JUCE-LICENSE.md")
  # Large raw measurement IRs are development artifacts, not part of the installer.
  Get-ChildItem "$dest/Messungen" -Filter *.f32 | Remove-Item
  $zip = "$dest.zip"
  Compress-Archive -Path "$dest/*" -DestinationPath $zip
  Add-Type -AssemblyName System.IO.Compression.FileSystem
  $archive = [System.IO.Compression.ZipFile]::OpenRead($zip)
  try {
    if ($archive.Entries.Count -lt 10) { throw "ZIP unvollstaendig" }
    foreach ($entry in $archive.Entries) {
      if ($entry.Length -gt 0) { $stream=$entry.Open();try { $stream.CopyTo([System.IO.Stream]::Null) } finally { $stream.Dispose() } }
    }
  } finally { $archive.Dispose() }
  (Get-FileHash $zip -Algorithm SHA256).Hash | Set-Content "$zip.sha256"
  Write-Host "FERTIG: $zip"
} finally { Pop-Location }
