param(
    [string]$BuildDir = "build",
    [string]$Config = "Debug"
)

$ErrorActionPreference = "Stop"
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$BuildPath = Join-Path $ProjectRoot $BuildDir
$CovDir = Join-Path $ProjectRoot "coverage"
$OpenCppCoverage = "C:\Program Files\OpenCppCoverage\OpenCppCoverage.exe"

if (-not (Test-Path $OpenCppCoverage)) {
  $OpenCppCoverage = (Get-Command OpenCppCoverage.exe -ErrorAction SilentlyContinue).Source
}

if (-not $OpenCppCoverage) {
  throw "OpenCppCoverage not found. Install with: winget install OpenCppCoverage.OpenCppCoverage"
}

$UnitExe = Join-Path $BuildPath "$Config\rvc_tests.exe"
$SimExe = Join-Path $BuildPath "simulator\server\$Config\rvc_scenario_tests.exe"

foreach ($exe in @($UnitExe, $SimExe)) {
  if (-not (Test-Path $exe)) {
    throw "Missing executable: $exe. Run: cmake --build $BuildPath --config $Config"
  }
}

New-Item -ItemType Directory -Force -Path $CovDir | Out-Null

$commonArgs = @(
  "--quiet"
  "--export_type=html:$CovDir"
  "--export_type=cobertura:$CovDir\coverage.xml"
  "--sources=$ProjectRoot\src"
  "--sources=$ProjectRoot\simulator\server"
  "--excluded_sources=*\build\*"
  "--excluded_sources=*\tests\*"
  "--excluded_sources=*\_deps\*"
  "--excluded_sources=*\googletest\*"
  "--excluded_sources=*\httplib\*"
)

Write-Host "==> Unit test coverage (rvc_tests)"
& $OpenCppCoverage @commonArgs `
  "--export_type=html:$CovDir\unit" `
  "--export_type=cobertura:$CovDir\unit.xml" `
  "--working_dir=$(Split-Path $UnitExe)" `
  "--cover_children" `
  "--" $UnitExe

if ($LASTEXITCODE -ne 0) {
  throw "Unit test or coverage collection failed (exit $LASTEXITCODE)"
}

Write-Host "==> Simulator auto-test coverage (rvc_scenario_tests)"
& $OpenCppCoverage @commonArgs `
  "--export_type=html:$CovDir\simulator" `
  "--export_type=cobertura:$CovDir\simulator.xml" `
  "--working_dir=$(Split-Path $SimExe)" `
  "--" $SimExe

if ($LASTEXITCODE -ne 0) {
  throw "Simulator scenario tests or coverage collection failed (exit $LASTEXITCODE)"
}

python (Join-Path $PSScriptRoot "summarize_coverage.py") `
  (Join-Path $CovDir "unit.xml") `
  (Join-Path $CovDir "simulator.xml") `
  (Join-Path $CovDir "summary.md")

Write-Host ""
Write-Host "Reports:"
Write-Host "  Unit:      $CovDir\unit\index.html"
Write-Host "  Simulator: $CovDir\simulator\index.html"
Write-Host "  Summary:   $CovDir\summary.md"
