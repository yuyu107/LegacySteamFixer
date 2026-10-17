$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
Push-Location $root
try {
    $csc = Join-Path $env:SystemRoot 'Microsoft.NET/Framework/v4.0.30319/csc.exe'
    $tempBase = if ($env:RUNNER_TEMP) { $env:RUNNER_TEMP } else { [IO.Path]::GetTempPath() }
    $testRoot = Join-Path $tempBase ('fixer-tests-' + [guid]::NewGuid().ToString('N'))
    New-Item -ItemType Directory -Path "$testRoot/stub" -Force | Out-Null
    @'
using System.Reflection;
[assembly:AssemblyVersion("999.0.0.0")]
[assembly:AssemblyInformationalVersion("999.0.0")]
class Fixture {static void Main(){}}
'@ | Set-Content "$testRoot/stub/fixture.cs" -Encoding UTF8
    & $csc /nologo "/out:$testRoot/stub/LegacySteamFixer.exe" "$testRoot/stub/fixture.cs"
    if ($LASTEXITCODE -ne 0) { throw 'Update fixture compilation failed' }
    python tests/self_update_fixtures.py $root "$testRoot/stub/LegacySteamFixer.exe" $testRoot
    if ($LASTEXITCODE -ne 0) { throw 'ZIP fixture generation failed' }
    $sources = Get-ChildItem src/*.cs | ForEach-Object { $_.FullName }
    foreach ($name in @('CoreTests', 'UpdateTests', 'ZstdSupportTests', 'SteamLaunchModesTests', 'LaunchOptionsTests', 'SelfUpdateTests')) {
        $entry = if ($name -eq 'CoreTests') { 'Tests' } else { $name }
        # Run beside modules and native assets, matching the runtime layout.
        $exe = Join-Path $root "$name.exe"
        & $csc /nologo /target:exe "/main:$entry" "/out:$exe" /reference:System.Windows.Forms.dll /reference:System.Drawing.dll /reference:System.Web.Extensions.dll @sources "tests/$name.cs"
        if ($LASTEXITCODE -ne 0) { throw "Test compilation failed: $name" }
        if ($name -eq 'SelfUpdateTests') { & $exe $testRoot } else { & $exe }
        if ($LASTEXITCODE -ne 0) { throw "Test failed: $name" }
        Remove-Item $exe
    }
} finally {
    Pop-Location
}
