mingw32-make clean
mingw32-make

$CORE_COUNT = (Get-WmiObject -Class Win32_Processor).NumberOfCores
echo $CORE_COUNT
# stress processor first for reliable results
Start-Process -NoNewWindow ".\bin\stress.exe" -ArgumentList 2, 40 -PassThru
Start-Sleep -Seconds 100
taskkill -f -im stress.exe

for ($cpu_num = 1; $cpu_num -le $CORE_COUNT; $cpu_num+=1) {
    for ($outer = 0; $outer -le 5; $outer+=1) {
        for ($cpu_type = 0; $cpu_type -le 3; $cpu_type+=1) {
            ./run.ps1 $outer $cpu_type $cpu_num
        }
    }
}

python3 plot_score.py data