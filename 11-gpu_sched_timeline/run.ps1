param($gpu_type, $cpu_num)

taskkill -f -im run_opencl.exe
echo "START!!"

$app = Start-Process -NoNewWindow "..\utils\opencl\run_opencl.exe" $gpu_type -PassThru
$app.ProcessorAffinity = 0x4000 # FIXME
$app2 = Start-Process -NoNewWindow ".\bin\sample.exe" -ArgumentList $cpu_num -PassThru
Start-Process "..\utils\monitor2.exe" -Verb runAs -WindowStyle hidden -Wait
$app2.WaitForExit()
echo "done!"

taskkill -f -im run_opencl.exe
$dirname="data"
$dirname2="core_data"
mkdir $dirname -ErrorAction SilentlyContinue
mkdir $dirname2 -ErrorAction SilentlyContinue
$files = dir .\out\*.out
$filename = $files[0].name
Copy-Item ".\out\$($filename)" -Destination ".\$($dirname)\$($gpu_type)_$($cpu_num)_$($filename)"
$files = dir .\core_out\*.out
$filename = $files[0].name
Copy-Item ".\core_out\$($filename)" -Destination ".\$($dirname2)\$($gpu_type)_$($filename)"
Start-Sleep -milliseconds 200
rm .\out\* -force
rm .\core_out\* -force

echo "gpu_$($gpu_type)_$($cpu_num)_$($filename)"
Get-Date





