mingw32-make clean
mingw32-make

./run.ps1 0 14
Start-Sleep -seconds 60
./run.ps1 1 14
Start-Sleep -seconds 60
./run.ps1 2 14

python3 plot_core.py core_data
python3 plot_score.py data