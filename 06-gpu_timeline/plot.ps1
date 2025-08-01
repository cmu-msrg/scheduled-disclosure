mingw32-make clean
mingw32-make

./run.ps1 0 8
Start-Sleep -Seconds 60
./run.ps1 1 8
Start-Sleep -Seconds 60
./run.ps1 2 8

python3 plot_score.py data