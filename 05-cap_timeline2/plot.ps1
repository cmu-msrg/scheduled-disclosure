mingw32-make clean
mingw32-make

./run.ps1 0 0 12
Start-Sleep -seconds 60
./run.ps1 0 1 12

python3 plot_score.py data