mingw32-make clean
mingw32-make

# warmup 
./run.ps1 100 5

# default frequency and power limit
for($i=0; $i -lt 6; $i++){
    for($j=0; $j -lt 20; $j++){
        ./run.ps1 $j $i
    }
}

mv data default_data

# limited frequency and power limit
Start-Process "..\utils\msr-utility\msr-cmd.exe" -Verb runAs -ArgumentList "rmw","0x610","14","0","0xa0"   -WindowStyle hidden -Wait
powercfg /setACvalueindex scheme_current SUB_PROCESSOR PROCFREQMAX1 1000
powercfg /setACvalueindex scheme_current SUB_PROCESSOR PROCFREQMAX 800
powercfg /setactive scheme_current 

for($i=0; $i -lt 6; $i++){
    for($j=0; $j -lt 20; $j++){
        ./run.ps1 $j $i
    }
}

# set back to default
powercfg /setACvalueindex scheme_current SUB_PROCESSOR PROCFREQMAX1 0
powercfg /setACvalueindex scheme_current SUB_PROCESSOR PROCFREQMAX 0 
powercfg /setactive scheme_current 
Start-Process "..\utils\msr-utility\msr-cmd.exe" -Verb runAs -ArgumentList "rmw","0x610","14","0","0xe0"   -WindowStyle hidden -Wait

mv data limited_data

python3 plot_pstate.py default_data
python3 plot_pstate.py limited_data
