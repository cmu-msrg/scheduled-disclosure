# Covert channel script

# Set the type of receiver and type of sender
$receiver = 0
$senders = @('13')

# Set the number of receivers and senders
$num_receiver = 6
$num_sender = 6

# Set the number of intervals to collect
$num_intervals = 1100

# Set the interval sizes
$intervals = @(3000000000, 1500000000, 1000000000, 750000000, 600000000, 500000000, 428571429, 375000000, 333333333, 300000000, 272727273, 250000000, 230769231, 214285714, 200000000, 187500000, 176470588, 166666667, 157894737, 150000000, 142857143, 136363636, 130434783, 125000000, 120000000, 115384615, 111111111, 107142857, 103448276, 100000000, 96774194, 93750000)

foreach($interval in $intervals) {

    foreach($sender in $senders) {

        if (-not (Test-Path -Path ".\out\$num_sender-sender$sender-$num_receiver-receiver$receiver-$interval")) {
        New-Item -ItemType Directory -Path ".\out\$num_sender-sender$sender-$num_receiver-receiver$receiver-$interval" -Force
        } 

        $procs = $(Start-Process -NoNewWindow ".\bin\sender$sender.exe" -ArgumentList $interval, $num_sender, $num_intervals -PassThru; Start-Process -NoNewWindow ".\bin\receiver$receiver.exe" -ArgumentList $interval, $num_receiver, $num_intervals -PassThru;)
        $procs | Wait-Process

        Get-ChildItem -File -Filter "*.out" -Path ".\out" | Move-Item -Destination ".\out\$num_sender-sender$sender-$num_receiver-receiver$receiver-$interval" -Force

        $targetDir = ".\out\$num_sender-sender$sender-$num_receiver-receiver$receiver\"

        if (-not (Test-Path -Path $targetDir)) {
            New-Item -ItemType Directory -Path $targetDir -Force
        }

        # ignore errors if the directory already exists
        Move-Item -Path ".\out\$num_sender-sender$sender-$num_receiver-receiver$receiver-$interval" -Destination $targetDir -Force 

    }
}
