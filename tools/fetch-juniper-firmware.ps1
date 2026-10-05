param([string]$OutDir = "$PSScriptRoot\..\firmware")
$ErrorActionPreference='Stop'
New-Item -ItemType Directory -Force $OutDir | Out-Null
$base='https://kernel.googlesource.com/pub/scm/linux/kernel/git/firmware/linux-firmware/+/f9b926a6e1d67e09e54adc329c4e76be5f24a895/radeon'
# Gitiles raw endpoint returns base64 when ?format=TEXT is used.
foreach($f in 'JUNIPER_pfp.bin','JUNIPER_me.bin','JUNIPER_rlc.bin') {
  $u="$base/$f?format=TEXT"
  $b64=(Invoke-WebRequest -UseBasicParsing $u).Content -replace '\s',''
  [IO.File]::WriteAllBytes((Join-Path $OutDir $f),[Convert]::FromBase64String($b64))
}
Write-Host 'Juniper firmware downloaded. Verify SHA-256 and license provenance before packaging.'
