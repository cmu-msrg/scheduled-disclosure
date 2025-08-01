param($outer, $inst)

echo "START!!"

$app2 = Start-Process -NoNewWindow ".\bin\stress.exe" -ArgumentList $inst -PassThru
Start-Process "..\utils\monitor_pstate.exe" -Verb runAs -WindowStyle hidden -Wait
taskkill -f -im stress.exe

echo "DONE!!"
Start-Sleep -milliseconds 200

$dirname="data"
mkdir $dirname -ErrorAction SilentlyContinue
$files = dir .\out\*.out
$filename = $files[0].name

Copy-Item ".\out\$($filename)" -Destination ".\$($dirname)\$($outer)_$($inst)_$($filename)"
Start-Sleep -milliseconds 200
rm .\out\* -force

echo "$($filename)"
Get-Date






