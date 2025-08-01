mingw32-make clean
mingw32-make

./run.ps1 0 6
Start-Sleep -seconds 60
./run.ps1 1 6
Start-Sleep -seconds 60
./run.ps1 2 6

python3 plot_core.py core_data
python3 plot_score.py data