param($outer, $cpu_type, $cpu_num)

$app2 = Start-Process -NoNewWindow ".\bin\stress.exe" -ArgumentList $cpu_type, $cpu_num -PassThru
Start-Process "..\utils\monitor1.exe" -Verb runAs -WindowStyle hidden -Wait
echo "done!"
taskkill -f -im stress.exe

$dirname="data"
mkdir $dirname -ErrorAction SilentlyContinue
$files = dir .\out\*.out
$filename = $files[0].name
Copy-Item ".\out\$($filename)" -Destination ".\$($dirname)\$($outer)_$($cpu_type)_$($cpu_num)_$($filename)"
Start-Sleep -milliseconds 200
rm .\out\* -force

echo "$($outer)_$($cpu_type)_$($cpu_num)_$($filename)"
Get-Date





