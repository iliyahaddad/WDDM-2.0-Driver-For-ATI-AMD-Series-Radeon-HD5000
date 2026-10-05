param([string]$ProjectFile = "$PSScriptRoot\..\include\driver.h")
$ErrorActionPreference='Stop'
$s=Get-Content $ProjectFile -Raw
if ($s -notmatch '#define MADISON_ENABLE_PHASE3_CP\s+0') { throw 'MADISON_ENABLE_PHASE3_CP 0 not found (already enabled?)' }
$s=$s -replace '#define MADISON_ENABLE_PHASE3_CP\s+0','#define MADISON_ENABLE_PHASE3_CP      1'
Set-Content -Path $ProjectFile -Value $s -NoNewline
Write-Host 'Phase 3 CP activation enabled. Build, test-sign, install only on a disposable/test Windows 11 system.'
