param (
    [string]$address = "LCM1.local",
    [int]$port = 2112,
    [string]$data = ""
)

# Append a null termination to the end of the string
$data = $data + "`0"

echo $data | .\nmap-7.01-win32\nmap-7.01\ncat.exe $address $port 2> $null
Write-Host ""
Write-Host ""
