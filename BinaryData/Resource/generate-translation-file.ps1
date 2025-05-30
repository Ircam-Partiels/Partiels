param (
    [string]$keys,
    [string]$translations,
    [string]$output
)

$originalContent = Get-Content -Path "$keys"
$translatedContent = Get-Content -Path "$translations"

if (Test-Path -Path "$output") {
    Clear-Content -Path "$output"
} else {
    New-Item -Path "$output" -ItemType File | Out-Null
}

if($originalContent.Count -ne $translatedContent.Count) {
    Write-Host "Error: The number of keys and translations do not match."
    exit 1
}

Add-Content -Path "$output" -Value "language:"
Add-Content -Path "$output" -Value "countries:"
Add-Content -Path "$output" -Value ""

for ($i = 0; $i -lt $originalContent.Count; $i++) {
    $originalLine = $originalContent[$i].Trim()
    $translatedLine = $translatedContent[$i].Trim()
    Add-Content -Path "$output" -Value """$originalLine"" = ""$translatedLine"""
}
