# Plots a subplot for each core showing the number of cycles each thread spends on that core for each interval

import parse
import pandas as pd
import numpy as np
import sys
import os
import matplotlib.pyplot as plt
import datetime
import math
import subprocess

# For plotting the pcore/ecore aggregate plots
def read_from_file_core(filename):
    result_x = []
    result_y = []
    result_z = []

    # Store data from each thread's file
    with open(filename) as f:
        for line in f:
            x, y, z = line.strip().split()
            result_x.append(int(x))
            result_y.append(int(y))
            result_z.append(int(z))

    interval = pd.Series(result_x, name='Interval')
    thread = pd.Series(result_y, name='Thread')
    total_cycles = pd.Series(result_z, name='Total_cycles')
    
    return pd.DataFrame({'Interval': interval, 'Thread': thread, 'Total_cycles': total_cycles})

# Read the output of the parsed data
def read_from_file(filename):
    # final_process.out <core> <interval> <thread> <total_cycles>
    result_w = []
    result_x = []
    result_y = []
    result_z = []

    # Store data from each thread's file
    with open(filename) as f:
        for line in f:
            w, x, y, z = line.strip().split()
            result_w.append(int(w))
            result_x.append(int(x))
            result_y.append(int(y))
            result_z.append(int(z))

    core = pd.Series(result_w, name='Core')
    interval = pd.Series(result_x, name='Interval')
    thread = pd.Series(result_y, name='Thread')
    total_cycles = pd.Series(result_z, name='Total_cycles')
    
    return pd.DataFrame({'Core': core, 'Interval': interval, 'Thread': thread, 'Total_cycles': total_cycles})

def plot_data(final_results, final_results_pcore, final_results_ecore, num_receivers, num_senders, num_intervals, directory):

    out_dir = 'plot'
    try:
        os.makedirs(out_dir)
    except:
        pass

    type = "subset" # or "all"
    start_index = 400
    num_intervals_for_plot = 20
    end_index = start_index + num_intervals_for_plot

    # Create directory and filename
    today = datetime.date.today()
    out_directory = f"./plot"

    if not os.path.exists(out_directory):
        os.makedirs(out_directory)

    out_filename = "/" + os.path.basename(directory) + "-bar-plot.pdf"

    out_name = out_directory + out_filename

    print("OUTNAME: ", out_name)

    receiver_color = 'tab:blue'
    sender_color = 'blue'

    # Modify based on core configuration
    p_core = [0, 9, 10, 11, 12, 13] #FIXME
    e_core = [1, 2, 3, 4, 5, 6, 7, 8] #FIXME

    num_cores = len(p_core) + len(e_core) 

    fig, ax = plt.subplots(1, figsize=(6,2))

    receiver_arr = list(range(num_receivers))

    bottom_zero = np.zeros(num_intervals)

    unique_intervals = range(num_intervals)

    aggregate_cycles = np.zeros(num_intervals_for_plot)
    for thread_id in receiver_arr:
        thread_data_pcore = final_results_pcore[final_results_pcore['Thread'] == thread_id]
        cycles_pcore = np.zeros(num_intervals)
        for interval_index in unique_intervals:
            interval_data = thread_data_pcore[thread_data_pcore['Interval'] == interval_index]
            if not interval_data.empty:
                cycles_pcore[interval_index] = interval_data['Total_cycles'].values[0]

        if type == "subset":
            unique_intervals_subset = np.arange(0, num_intervals_for_plot)
            cycles_pcore_subset = cycles_pcore[start_index:end_index]
            aggregate_cycles += cycles_pcore_subset
            bottom_zero_subset = bottom_zero[0:num_intervals_for_plot]
            ax.bar(unique_intervals_subset, aggregate_cycles, color=receiver_color, label=f'P-core', zorder=10)
            bottom_zero_subset += cycles_pcore_subset
            ax.set_xticks(unique_intervals_subset)
            ax.set_xticklabels(unique_intervals_subset)

        else:
            ax.bar(unique_intervals, cycles_pcore, bottom=bottom_zero, color=receiver_color, label=f'P-core')
            bottom_zero += cycles_pcore
            ax.set_xticks(unique_intervals)
            ax.set_xticklabels(unique_intervals)

    ax.set_xlabel("Interval", fontsize=8)
    ax.set_ylabel("P-core cycles (billions)", fontsize=8)

    ax.set_ylim(0, 2e9)
    #rm 1e9 to plot in billions
    ax.set_yticks(np.arange(0, 2e9 + 1e8, 5e8))
    ax.set_yticklabels([f"{float(x/1e9)}" for x in np.arange(0, 2e9 + 1e8, 5e8)])
    # horizontal grid
    ax.yaxis.grid(True, linestyle='-', linewidth=1, alpha=0.7, zorder=-10)

    fig.tight_layout(pad=0.1)

    fig.savefig(out_name)
    fig.clf()
    plt.close(fig)

def main():

    # Check args
    assert len(sys.argv) == 6, "Specify the directory containing data and the interval <directory> <interval> <receivers> <senders> <intervals>" 

    directory = sys.argv[1]
    filename = directory + "/parsed_data/final_process.out"    
    interval = int(sys.argv[2])
    num_receivers = int(sys.argv[3])
    num_senders = int(sys.argv[4])
    num_intervals = int(sys.argv[5])

    final_results = read_from_file(filename)

    filename_pcore = directory + "/parsed_data/processed_pcore.out"
    final_results_pcore = read_from_file_core(filename_pcore)

    filename_ecore = directory + "/parsed_data/processed_ecore.out"
    final_results_ecore = read_from_file_core(filename_ecore)

    plot_data(final_results, final_results_pcore, final_results_ecore, num_receivers, num_senders, num_intervals, directory)


if __name__ == "__main__":
    main()
