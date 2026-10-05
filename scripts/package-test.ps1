param([string]$Configuration='Debug')
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$pkg=Join-Path $root "out\$Configuration\package"
New-Item -ItemType Directory -Force -Path $pkg | Out-Null
Copy-Item (Join-Path $root "package\madisonkmd.inf") $pkg -Force
$sys = Join-Path $root "build\x64\$Configuration\MadisonKmd.sys"
if (-not (Test-Path $sys)) { throw "MadisonKmd.sys not found at $sys - run scripts\build.ps1 first" }
Copy-Item $sys $pkg -Force
foreach ($f in 'JUNIPER_pfp.bin','JUNIPER_me.bin','JUNIPER_rlc.bin') {
    $src = Join-Path $root "firmware\$f"
    if (-not (Test-Path $src)) { throw "Missing firmware\$f - run tools\fetch-juniper-firmware.ps1" }
    Copy-Item $src $pkg -Force
}
Write-Host "Package staging: $pkg"
Write-Host 'Generate the catalog with Inf2Cat and sign with a test certificate on the Windows WDK host.'
