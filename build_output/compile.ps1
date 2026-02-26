$ErrorActionPreference = 'Stop'
$solutionPath = 'c:\Users\roman\Dropbox\Engine\Engine.sln'
$config = 'Development'
$platform = 'x64'
$outputFile = 'c:\Users\roman\Dropbox\Engine\build_output\build_output.txt'

$vsRunning = Get-Process devenv -ErrorAction SilentlyContinue
if ($vsRunning) {
    Write-Host 'Visual Studio is running, building via COM automation...'
    try {
        $dte = [Runtime.InteropServices.Marshal]::GetActiveObject('VisualStudio.DTE.17.0')
        if ($dte.Solution.FullName -ieq $solutionPath) {
            # Activate the desired configuration
            foreach ($sc in $dte.Solution.SolutionBuild.SolutionConfigurations) {
                if ($sc.Name -eq $config) {
                    $sc.Activate()
                    break
                }
            }
            $dte.Solution.SolutionBuild.Build($true)
            $failed = $dte.Solution.SolutionBuild.LastBuildInfo

            # Try to capture build output from VS Error List
            try {
                $errorItems = $dte.ToolWindows.ErrorList.ErrorItems
                $output = @()
                for ($i = 1; $i -le $errorItems.Count; $i++) {
                    $item = $errorItems.Item($i)
                    $output += "$($item.FileName)($($item.Line)): $($item.Description)"
                }
                if ($output.Count -gt 0) {
                    $output -join "`n" | Out-File -FilePath $outputFile -Encoding utf8
                }
            } catch {
                # Error list not accessible, not critical
            }

            if ($failed -gt 0) {
                Write-Host "BUILD FAILED: $failed project(s) with errors"
                if (Test-Path $outputFile) {
                    Write-Host "Build errors saved to $outputFile"
                }
                exit 1
            } else {
                Write-Host 'BUILD SUCCEEDED (via Visual Studio)'
                exit 0
            }
        } else {
            Write-Host 'VS is open but with a different solution, falling back to MSBuild...'
        }
    } catch {
        Write-Host "COM automation failed: $($_.Exception.Message)"
        Write-Host 'Falling back to MSBuild...'
    }
}

Write-Host 'Building via MSBuild...'
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' $solutionPath /p:Configuration=$config /p:Platform=$platform /m:1 /v:minimal 2>&1 | Tee-Object -FilePath $outputFile
exit $LASTEXITCODE
