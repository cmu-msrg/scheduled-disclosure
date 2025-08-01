mingw32-make clean
mingw32-make

Start-Process "..\utils\msr-utility\msr-cmd.exe" -Verb runAs -ArgumentList "-l", "setbit", "0x1a0","38","1" -WindowStyle hidden -Wait
./run.ps1
Start-Process "..\utils\msr-utility\msr-cmd.exe" -Verb runAs -ArgumentList "-l", "setbit", "0x1a0","38","0" -WindowStyle hidden -Wait

$rootDirectory = ".\out\6-sender13-6-receiver0"
$dirName = Split-Path -Path $rootDirectory -Leaf
$dirName = "./plot/$dirName"
New-Item -ItemType Directory -Path $dirName -Force

$subdirectories = Get-ChildItem -Path $rootDirectory -Directory 

$errorFilePath = Join-Path -Path $rootDirectory -ChildPath "errors.out"

if (Test-Path -Path $errorFilePath) {
    Remove-Item -Path $errorFilePath -Force
}

foreach ($subdirectory in $subdirectories) {
    $basenameParts = $subdirectory -split '-'
    $interval = $basenameParts[-1]
    $subdirPath = $subdirectory.FullName
    Write-Host "Subdirectory: $subdirPath"
    Write-Host "Last Item: $interval"
    $parseDataOutput = python3 ./parse.py $subdirPath $interval
    $parseDataValues = $parseDataOutput -split ' '
    Write-Host "Parse Data Values: " $parseDataValues
    $num_receivers = $parseDataValues[12]
    Write-Host "Num Receivers: " $num_receivers
    $num_senders = $parseDataValues[16]
    $num_intervals = $parseDataValues[5]

    python3 ./get_errors.py $subdirPath $interval
    python3 ./processed_data_plot.py $subdirPath $interval $num_receivers $num_senders $num_intervals
}

$error_file = $rootDirectory + "/errors.out"
python3 ./plot_errors.py $error_file

$files = Get-ChildItem -Path "./plot" -File
Write-Host "Files: " $files
foreach ($file in $files) {
    Write-Host $file.Name
    Move-Item -Path $file.FullName -Destination $dirName
}