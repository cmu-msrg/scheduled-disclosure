param($gpu_type, $cpu_num)
taskkill -f -im run_opencl.exe
echo "START!!"

$app = Start-Process -NoNewWindow "..\utils\opencl\run_opencl.exe" $gpu_type -PassThru
$app.ProcessorAffinity = 0x8 # set to E-core #FIXME based on P-core and E-core
$app2 = Start-Process -NoNewWindow ".\bin\stress.exe" -ArgumentList $cpu_num -PassThru
Start-Process "..\utils\monitor1.exe" -Verb runAs -WindowStyle hidden -Wait
echo "done!"
taskkill -f -im run_opencl.exe
taskkill -f -im stress.exe

$dirname="data"
mkdir $dirname -ErrorAction SilentlyContinue
$files = dir .\out\*.out
$filename = $files[0].name
Copy-Item ".\out\$($filename)" -Destination ".\$($dirname)\gpu_$($gpu_type)_$($cpu_num)_$($filename)"
Start-Sleep -milliseconds 100
rm .\out\* -force

echo "gpu_$($gpu_type)_$($cpu_num)_$($filename)"
Get-Date





