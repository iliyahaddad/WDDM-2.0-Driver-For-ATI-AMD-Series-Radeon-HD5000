param(
    [ValidateSet('Debug','Release')][string]$Configuration='Debug',
    [string]$Platform='x64'
)
$ErrorActionPreference='Stop'
$root = Split-Path -Parent $PSScriptRoot
$solution = Join-Path $root 'build\MadisonKmd.sln'
Write-Host "Building Madison KMD: $Configuration|$Platform"
& msbuild $solution /m /p:Configuration=$Configuration /p:Platform=$Platform /t:Build
if ($LASTEXITCODE -ne 0) { throw "MSBuild failed: $LASTEXITCODE" }
Write-Host 'Build succeeded. Run Inf2Cat/SignTool on a Windows WDK host before installation.'
