# Parses raw receiver/sender data in the format <duration> <global time> <core> into <core> <interval> <thread> <total_cycles> 
# Called before processed_data_plot.py to create bar chart plot

import os
import sys 
import pandas as pd
import math
import re

# Sort files by receiver/sender and by thread number
def extract_key(filename):
    base_name = os.path.basename(filename)
    match = re.search(r'(\d+)', base_name)
    number = int(match.group(1)) if match else float('inf')
    return number

# Sort files by receiver/sender and by thread number
def custom_sort_key(x):
    base_name = os.path.basename(x)
    category = '1' if 'receiver' in base_name else '2' 
    number = extract_key(base_name)
    return(category, number)

# Output only overlapping data for receiver and senders
def filter_overlap(directory, interval):
    min_time_arr = []
    max_time_arr = []

    output_filename_arr = []  
    # print(directory)

    for filename in os.listdir(directory):
        if filename.endswith('.out'):
            file_path = os.path.join(directory, filename)
            # print(file_path)
            with open(file_path, 'r') as f:
                output_filename = filename.replace('.out', '-filtered.out')
                output_filename_arr.append(os.path.join(directory + '\\parsed_data', output_filename))
                first_line = f.readline()
                last_line = f.readlines()[-1]
                min_cycles, min_core = first_line.strip().split()
                max_cycles, max_core = last_line.strip().split()
                min_time_arr.append(int(min_cycles))
                max_time_arr.append(int(max_cycles))

    time_lower_bound = math.ceil(max(min_time_arr) / interval) * interval
    time_upper_bound = math.floor(min(max_time_arr) / interval) * interval

    # In the case, only one interval is plotted
    if time_lower_bound == time_upper_bound:
        time_upper_bound += interval

    start_interval = time_lower_bound // interval
    print("start interval: " + str(start_interval))
    num_intervals = (time_upper_bound - time_lower_bound) // interval
    print("num intervals: " + str(num_intervals))

    index = 0
    for filename in os.listdir(directory):
        if filename.endswith('.out'):
            file_path = os.path.join(directory, filename)
            start_time = 0
            with open(file_path, 'r') as infile, open(output_filename_arr[index], 'w') as outfile:
                first_end_time, first_cycles = infile.readline().strip().split()
                start_time = (int(first_end_time)// interval) * interval
                for line in infile:
                    cycles, core = line.strip().split()
                    cycles = int(cycles)
                    core = int(core)

                    if (time_lower_bound <= start_time <= time_upper_bound):
                        outfile.write(line)
                    start_time = cycles
            index += 1

 
    return num_intervals, start_interval

# Calculate duration and output file in format <start_time> <duration> <core>
def parse_data(file_paths, directory, interval):

    parsed_data_dir = directory + '/parsed_data'

    for filename in file_paths:
        with open(filename, 'r') as file:
            output_filename = filename.replace('filtered.out', 'parsed.out')
            output_file = os.path.join(parsed_data_dir, output_filename)
            processed_data = []
            current_core = None
            duration = 0
            start_time = None
            sample_start_time = 0
            first_end_time, first_core = file.readline().strip().split()
            sample_start_time = (int(first_end_time) // interval) * interval
            for line in file:
                end_time, core = line.strip().split()
                end_time = int(end_time)
                core = int(core)   

                # Intialize a different core value
                if current_core == None:
                    current_core = core
                    start_time = sample_start_time
                    duration = 0
                elif core == current_core:
                    duration = end_time - start_time
                else:
                    processed_data.append((start_time, sample_start_time, sample_start_time-start_time, current_core))
                    current_core = core
                    start_time = sample_start_time
                    duration = 0

                sample_start_time = end_time

            if current_core != None:
                processed_data.append((start_time, end_time, duration, current_core))

            with open(output_file, 'w') as outfile:
                for start_time, end_time, duration, value in processed_data:
                    outfile.write(f"{start_time} {end_time} {duration} {value}\n")

    return

def process_data(file_paths, directory, results, interval, begin_interval):

    # Modify based on core configuration
    pcore_arr = [0, 9, 10, 11, 12, 13] #FIXME
    ecore_arr = [1, 2, 3, 4, 5, 6, 7, 8] #FIXME

    # For each thread, calculate the cycles spent on each core for each interval
    for thread_id, file in enumerate(file_paths):

        results_temp = pd.DataFrame(columns=['Core', 'Interval', 'Thread', 'Total_cycles'])


        output_filename = file.replace('parsed.out', 'processed.out')
        output_file = os.path.join(directory + '/parsed_data', output_filename)

        with open(file, 'r') as file:
            for line in file:
                start_time, end_time, duration, core = line.strip().split() 
                start_time = int(start_time)
                end_time = int(end_time) 
                duration = int(duration) 
                core = int(core)

                # Determine the start and end interval of the data point (row)
                start_interval = start_time // interval
                end_interval = end_time // interval
                if(start_interval == 0):
                    print("interval 0 time: " + start_time)

                # Determine the total cycles the thread spent on each core for each interval
                for interval_index in range(start_interval, end_interval+1):
                    interval_start_time = interval_index * interval
                    interval_end_time = interval_start_time + interval

                    if interval_index == start_interval:
                        # Sample starts and ends in the same interval
                        if interval_index == end_interval:
                            time_in_interval = duration
                        # Sample starts in interval but ends in another
                        else: 
                            time_in_interval = interval_end_time - start_time
                    # Sample starts in another interval but ends in the current interval
                    elif interval_index == end_interval:
                        time_in_interval = end_time - interval_start_time
                    # Sample spans the entire interval
                    else:
                        time_in_interval = interval

                    results_temp = results_temp._append({'Core': core, 'Interval': interval_index - begin_interval, 'Thread': thread_id, 'Total_cycles': time_in_interval}, ignore_index=True)
                start_time = end_time


        results = results._append(results_temp, ignore_index=True)
        with open(output_file, 'w') as outfile:
            for _, row in results_temp.iterrows():
                outfile.write(f"{row['Core']} {row['Interval']} {row['Thread']} {row['Total_cycles']}\n")

    final_results = results.groupby(['Core', 'Interval', 'Thread'], as_index=False).sum()
    final_result_by_pcore = results[results['Core'].isin(pcore_arr)].groupby(['Interval', 'Thread'], as_index=False).sum()
    final_result_by_ecore = results[results['Core'].isin(ecore_arr)].groupby(['Interval', 'Thread'], as_index=False).sum()

    output_file_pcore_name = os.path.join(directory + '/parsed_data', 'processed_pcore.out')
    with open(output_file_pcore_name, 'w') as outfile:
        for _, row in final_result_by_pcore.iterrows():
            outfile.write(f"{row['Interval']} {row['Thread']} {row['Total_cycles']}\n")
    output_file_ecore_name = os.path.join(directory + '/parsed_data', 'processed_ecore.out')
    with open(output_file_ecore_name, 'w') as outfile:
        for _, row in final_result_by_ecore.iterrows():
            outfile.write(f"{row['Interval']} {row['Thread']} {row['Total_cycles']}\n")

    output_file_total_name = os.path.join(directory + '/parsed_data', 'final_process.out')
    with open(output_file_total_name, 'w') as outfile:
        for _, row in final_results.iterrows():
            outfile.write(f"{row['Core']} {row['Interval']} {row['Thread']} {row['Total_cycles']}\n")

    return final_results

def main():

    # Check args
    assert len(sys.argv) == 3, "Specify the files with the results as argument, and the interval"
    directory = sys.argv[1]
    interval = int(sys.argv[2])

    parsed_data_dir = directory + '/parsed_data'
    os.makedirs(parsed_data_dir, exist_ok=True)

    # 1) Filter out overlapping data
    num_intervals, begin_interval = filter_overlap(directory, interval)
    print("Begin interval: " + str( begin_interval))
    # Output: <global time (end_time)> <core> in .out file in /parse_data

    # Get file_paths in correct order (first by receiver then by sender, ordered numerically by thread number)
    file_paths_filtered = [os.path.join(directory + '/parsed_data', file) for file in os.listdir(directory + '/parsed_data') if file.endswith('filtered.out')]
    file_paths_filtered = sorted(file_paths_filtered, key=custom_sort_key)
    
    # Counting number of receivers and senders
    num_receivers = 0
    num_senders = 0
    for file in file_paths_filtered:
        if 'receiver' in os.path.basename(file):
            num_receivers += 1
        elif 'sender' in os.path.basename(file):
            num_senders += 1

    # 3) Calculate duration and output newly formatted data
    parse_data(file_paths_filtered, directory, interval)   
    # Output: <start_time> <end_time> <duration> <core>

    # Order file paths
    file_paths_parsed = [os.path.join(directory + '/parsed_data', file) for file in os.listdir(directory + '/parsed_data') if file.endswith('parsed.out')]
    file_paths_parsed = sorted(file_paths_parsed, key=custom_sort_key)

    results = pd.DataFrame(columns=['Core', 'Interval', 'Thread', 'Total_cycles'])    
    # 4) Sum the total number of cycles for each <core> <thread> <interval> combination
    final_results = process_data(file_paths_parsed, directory, results, interval, begin_interval)
    # <core> <interval> <thread> <total_cycles> 
    print(f"Number of receivers: {num_receivers}")
    print(f"Number of senders: {num_senders}")
    print(f"Number of intervals: {num_intervals}")
    
    begin_interval_file = os.path.join(parsed_data_dir, 'begin_interval.out')
    with open(begin_interval_file, 'w') as outfile:
        outfile.write(f"{begin_interval}\n")

    return num_receivers, num_senders, num_intervals
 
                                
if __name__ == "__main__":
    main()


