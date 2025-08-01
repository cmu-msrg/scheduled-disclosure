mingw32-make clean
mingw32-make

# disable turbo
Start-Process "..\utils\msr-utility\msr-cmd.exe" -Verb runAs -ArgumentList "-l", "setbit", "0x1a0","38","1" -WindowStyle hidden -Wait

# Runs class0.exe, class1.exe, class3.exe to create tables comparing the P/E-core ratio 
$input = @("class0", "class1", "class3")
$core_num = 14 #FIXME based on P-core and E-core

for($outer_i = 0; $outer_i -le 2; $outer_i++) {
    for($outer_j = $outer_i; $outer_j -le 2; $outer_j++) {
        $first_class = $input[$outer_i]
        $second_class = $input[$outer_j]

        for ($j = 0; $j -le $core_num; $j++) {
            for ($i = 0; $i -le $core_num; $i++) {
                if($j + $i -le $core_num -and $j + $i -ge 1) {

                    if (-not (Test-Path -Path ".\out\$i-$first_class-$j-$second_class")) {
                        New-Item -ItemType Directory -Path ".\out\$i-$first_class-$j-$second_class" -Force
                    }

                    $procs = $(Start-Process -NoNewWindow ".\bin\$first_class.exe" -ArgumentList $i, 1 -PassThru; Start-Process -NoNewWindow ".\bin\$second_class.exe" -ArgumentList $j, 2 -PassThru;)

                    $procs | Wait-Process

                    Get-ChildItem -Path ".\out" -File | Move-Item -Destination ".\out\$i-$first_class-$j-$second_class" -Force
                } 
            }
        }

        # Move all the run directories to a single directory
        $destinationDir = ".\out\$first_class-$second_class-table"
        if (-not (Test-Path -Path $destinationDir)) {
            New-Item -ItemType Directory -Path $destinationDir -Force
        }
        Get-ChildItem -Directory -Path ".\out" | Where-Object { $_.Name -like "*-$first_class-*-*$second_class" } | Move-Item -Destination $destinationDir

    }
}

python3 .\heatmap.py .\out\

# enable turbo
Start-Process "..\utils\msr-utility\msr-cmd.exe" -Verb runAs -ArgumentList "-l", "setbit", "0x1a0","38","0" -WindowStyle hidden -Wait
