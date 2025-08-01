# Scheduled Disclosure

This repository contains the source code for the IEEE S&P 2025 paper, [_Scheduled Disclosure: Turning Power Into Timing Without Frequency Scaling_](https://www.hertzbleed.com/scheduled-disclosure-sp2025.pdf).

### Experimental Setup

The code was tuned to work on a bare-metal ASRock NUCS BOX mini PC with an Intel Core Ultra 7 155H processor, running Windows 11 (version 24H2).
SMT was set off for the reverse engineering experiments.
__Running this code on other CPUs or machines will likely require changing configurations, such as the CPU core indices, within the code.__

## Content
The subdirectories contain the source code to create the figures in [the paper](https://www.hertzbleed.com/scheduled-disclosure-sp2025.pdf). Each subdirectory name is formatted as `[fig#]-[content]/`

### How to run
- Install `mingw32-make`, `python3`
- `pip3 install -r requirements.txt`
- Fix `FIXME`s in the code if you are running on a different machine setup (may not be exhaustive)
- Run `plot_all.ps1`


__Note:__ The SIKE key recovery and pixel stealing attack POCs in the paper were implemented by modifying code from [Hertzbleed](https://github.com/FPSG-UIUC/hertzbleed/tree/main/06-circl) and [GPU.zip](https://github.com/UT-Security/gpu-zip/tree/main/04-chrome-poc).

## Citation

If you make use of this code, please cite the paper:

```
@inproceedings {chun2025scheduled,
   author = {Inwhan Chun and Isabella Siu and Riccardo Paccagnella},
   booktitle = {Proceedings of the IEEE Symposium on Security and Privacy (S\&P)},
   title = {Scheduled Disclosure: Turning Power Into Timing Without Frequency Scaling},
   year = {2025},
}
```