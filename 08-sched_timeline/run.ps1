param($is_high, $cpu_num)
echo "START!!"

$app2 = Start-Process -NoNewWindow ".\bin\sample.exe" -ArgumentList $is_high, $cpu_num -PassThru
Start-Process "..\utils\monitor2.exe" -Verb runAs -WindowStyle hidden -Wait
echo "done!"

$dirname="data"
$dirname2="core_data"
mkdir $dirname -ErrorAction SilentlyContinue
mkdir $dirname2 -ErrorAction SilentlyContinue
$files = dir .\out\*.out
$filename = $files[0].name
Copy-Item ".\out\$($filename)" -Destination ".\$($dirname)\$($is_high)_$($cpu_num)_$($filename)"
$files = dir .\core_out\*.out
$filename = $files[0].name
Copy-Item ".\core_out\$($filename)" -Destination ".\$($dirname2)\$($filename)"
Start-Sleep -milliseconds 200
rm .\out\* -force
rm .\core_out\* -force

echo "$($filename)"
Get-Date





