**msr-cmd** on Windows is something like **wrmsr** from Intel msr-tools in Linux world, which can be used in batch/shell scripts.
Program compiled from [this repo](https://github.com/cocafe/msr-utility)

## Commands
```sh
# Run in Powershell administrator mode
# power limit
.\msr-cmd.exe -l rmw 0x610 14 0 0xe0 # default 28 W
.\msr-cmd.exe -l rmw 0x610 14 0 0xa0 # 20 W

# Turbo Boost
.\msr-cmd.exe -l setbit 0x1a0 38 1 # disable
.\msr-cmd.exe -l setbit 0x1a0 38 0 # enable
```