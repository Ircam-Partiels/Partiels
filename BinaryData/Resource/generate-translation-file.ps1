param (
    [string]$keys,
    [string]$translations
)

$originalContent = Get-Content -Path "$keys"
$translatedContent = Get-Content -Path "$translations"

for ($i = 0; $i -lt $originalContent.Count; $i++) {
    $left = $originalContent[$i];
    $right = $translatedContent[$i];
    Write-Host """$left"" = ""$right"""
}
