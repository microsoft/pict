# This script assumes it's being run from a Visual Studio Developer Command Prompt
# with MSBuild.exe and nuget.exe available in the PATH

# Build configurations - we'll use what's actually in the solution file
$configurations = @("Debug", "Release")

# First, let's determine which platforms are actually available in the solution
Write-Host "Analyzing available solution platforms..."
$solutionContent = Get-Content -Path "pict.sln" -Raw

# Extract platform configurations
$platformMatches = [regex]::Matches($solutionContent, 'GlobalSection\(SolutionConfigurationPlatforms\)(.*?)EndGlobalSection', [System.Text.RegularExpressions.RegexOptions]::Singleline)
$platforms = @()

if ($platformMatches.Count -gt 0) {
    $configLines = $platformMatches[0].Groups[1].Value -split "`n" | Where-Object { $_ -match '\s+=\s+' }
    $configPlatforms = $configLines | ForEach-Object {
        if ($_ -match '(\w+)\|(\w+)') {
            $matches[2]  # Extract the platform part
        }
    }
    $platforms = $configPlatforms | Sort-Object -Unique
    
    Write-Host "Found platforms in solution: $($platforms -join ', ')"
}

# If no platforms found, fall back to defaults
if ($platforms.Count -eq 0) {
    $platforms = @("x64", "ARM64")
    Write-Host "Using default platforms: $($platforms -join ', ')"
}

# Build each configuration
foreach ($platform in $platforms) {
    foreach ($configuration in $configurations) {
        Write-Host "Building for $platform - $configuration..."
        & MSBuild.exe pict.sln /p:Configuration=$configuration /p:Platform=$platform /t:Rebuild
        
        # Check if build succeeded
        if ($LASTEXITCODE -ne 0) {
            Write-Error "Build failed for $platform - $configuration"
            exit $LASTEXITCODE
        }
    }
}

# Create NuGet package
Write-Host "Creating NuGet package..."
& nuget pack Microsoft.Test.Pict.nuspec -OutputDirectory .\packages

Write-Host "Build and packaging complete!"