param(
	[Parameter(Mandatory = $true)]
	[string]$CMakeListsPath
)

if (-not (Test-Path -LiteralPath $CMakeListsPath)) {
	Write-Error "CMakeLists not found: $CMakeListsPath"
	exit 1
}

$content = Get-Content -LiteralPath $CMakeListsPath -Raw
if ($content -match 'project\s*\(\s*\S+\s+VERSION\s+([0-9]+(?:\.[0-9]+)*)') {
	Write-Output $matches[1]
	exit 0
}

Write-Error "Could not parse project VERSION from: $CMakeListsPath"
exit 1
