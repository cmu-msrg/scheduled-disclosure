import warnings
warnings.filterwarnings("ignore", category=DeprecationWarning)

import numpy as np
import multiprocessing as mp
from collections import namedtuple
import sys
import os
import matplotlib.pyplot as plt

import pandas as pd

# This code is for the training / offline phase
ParseParams = namedtuple('ParseParams', 'interval offset contention_frac score directory')
interval = None
pcore_data = None
ecore_data = None
SCORE_MAX = 9999999999999
directory = None

# The first 10 intervals are discarded
# The following 100 intervals are the training set (offline phase)
# The rest of the intervals are the testing set (online phase)
discard_intervals = 10
train_intervals_no = 100

discard_intv_start = 0
discard_intv_end = discard_intervals
train_intv_start = discard_intervals
train_intv_end = train_intv_start + train_intervals_no
test_intv_start = discard_intervals + train_intervals_no


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


def diff_letters(a, b):
    return sum(a[i] != b[i] for i in range(len(a)))


def parse_intervals_into_bits(intervals_p, intervals_e, min_contention_frac):

    # Parse the intervals into bits
    # If more than $(min_contention_frac)% of the measurements in an interval
    # are greater than the threshold, then we classify
    # the interval as a 1.
    result = ""
    min_interval = min(intervals_p['Interval'].min(), intervals_e['Interval'].min())
    max_interval = max(intervals_p['Interval'].max(), intervals_e['Interval'].max())
    all_intervals = pd.DataFrame({'Interval': range(min_interval, max_interval + 1)})
    merged_pe = pd.merge(all_intervals, intervals_p, on='Interval', how='outer', suffixes=('_p', ''))
    merged_pe = pd.merge(merged_pe, intervals_e, on='Interval', how='outer', suffixes=('_p', '_e'))
    merged_pe.fillna(0, inplace=True)
    for _, row in merged_pe.iterrows():
        interval = row['Interval']
        p_count = row['Total_cycles_p'] 
        e_count = row['Total_cycles_e'] 
        total_count = p_count + e_count
        if p_count > min_contention_frac * total_count:
            result += "1"
        else:
            result += "0"

    return result


def per_offset_worker(args):

    parse_params = args

    # Parse trace into intervals
    offset = parse_params.offset
    interval = parse_params.interval
    directory = parse_params.directory

    file_paths = [os.path.join(directory, 'parsed_data', f) for f in os.listdir(os.path.join(directory, 'parsed_data')) if f.endswith('parsed.out')]

    begin_interval_file = os.path.join(directory, 'parsed_data', 'begin_interval.out')
    with open(begin_interval_file, 'r') as f:
        begin_interval = int(f.readline().strip())

    results = pd.DataFrame(columns=['Core', 'Interval', 'Thread', 'Total_cycles'])    

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
                start_time = int(start_time) - offset
                end_time = int(end_time) - offset
                duration = int(duration) 
                core = int(core)

                # Determine the start and end interval of the data point (row)
                start_interval = start_time // interval
                end_interval = end_time // interval

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

    final_result_by_pcore = results[results['Core'].isin(pcore_arr)]
    final_result_by_ecore = results[results['Core'].isin(ecore_arr)]

    pcore_data = final_result_by_pcore.groupby(['Interval'], as_index=False)['Total_cycles'].sum()
    ecore_data = final_result_by_ecore.groupby(['Interval'], as_index=False)['Total_cycles'].sum()

    train_intervals_p = pcore_data[train_intv_start:train_intv_end]
    train_intervals_e = ecore_data[train_intv_start:train_intv_end]

    # We use the training set to find the best threshold
    # for this interval (offline) and the remaining parsed data
    # to get the error rate (online). We try different thresholds
    # because, depending on the CC interval, the best threshold
    # to use is different (e.g., with larger intervals we can use a
    # lower threshold than with shorter intervals).
    # We try different fractions of contention samples observed (e.g. 10% of
    # the samples must show contention for the bit to be counted as a 1)
    contention_fracs = range(0, 100, 1)  # test from 0.03 to 1.00

    # Save best scores
    best_contention_frac = None
    best_score = SCORE_MAX

    # Test various contention fractions
    for min_contention_frac in contention_fracs:
            # Parse the intervals into bits
            result = parse_intervals_into_bits(train_intervals_p, train_intervals_e, min_contention_frac / 100)
            if len(result) % 2 != 0:
                result = result[:-1]

            # Compare these bits with the ground truth
            # The ground truth is either 0101... or 1010...
            candidate_1 = "01" * (len(result) // 2)
            candidate_2 = "10" * (len(result) // 2)

            # Get the number of bit flips between the
            # decoded stream and the (correct) ground truth
            score_1 = diff_letters(result, candidate_1)
            score_2 = diff_letters(result, candidate_2)
            score = min(score_1, score_2)

            # Pick the best score (lowest error)
            if (score < best_score):
                best_contention_frac = min_contention_frac / 100
                best_score = score

    return ParseParams(interval, offset, best_contention_frac, best_score, directory), pcore_data, ecore_data

def main():
    global interval, directory

    # Check args
    assert len(sys.argv) == 3, "Specify the files with the results as argument, and the interval"
    directory = sys.argv[1]
    interval = int(sys.argv[2])
    num_receivers = int(os.path.basename(directory).split("-")[2])
    receiver_arr = range(0, num_receivers)  
    num_senders = int(os.path.basename(directory).split("-")[0])

    ################################ Train ################################ 

    # Train using different offsets
    pool = mp.Pool(processes=8)
    offsets = range(0, interval//2, interval // 20)
    params = [ParseParams(interval, o, None, None, directory) for o in offsets]
    results = pool.map(per_offset_worker, params)
    best_params_per_offset, pcore_datasets, ecore_datasets = zip(*results)
    for params in best_params_per_offset:
        print(params)

    # Select the best parameters
    best_params_index = min(range(len(best_params_per_offset)), key=lambda i: best_params_per_offset[i].score)
    best_params = best_params_per_offset[best_params_index]
    pcore_data = pcore_datasets[best_params_index]
    ecore_data = ecore_datasets[best_params_index]
    best_offset = best_params.offset
    best_contention_frac = best_params.contention_frac


    ################################ Test ################################ 

    max_interval = max(pcore_data['Interval'].max(), ecore_data['Interval'].max())
    test_intervals_no = max_interval - test_intv_start
    test_intervals_no = min(1000, test_intervals_no)
    test_intv_end = test_intv_start + test_intervals_no

    test_intervals_p = pcore_data[test_intv_start:test_intv_end]

    test_intervals_e = ecore_data[test_intv_start:test_intv_end]

    if len(test_intervals_p) < test_intervals_no and len(test_intervals_e) < test_intervals_no:
        print("Not enough test intervals")
        exit(-1)

    print('Best params: {}'.format(best_params))

    # Parse the intervals into bits
    result = parse_intervals_into_bits(test_intervals_p, test_intervals_e, best_contention_frac)

    # Compare these bits with the ground truth
    # The ground truth is either 0101... or 1010...
    candidate_1 = "01" * ((len(result) // 2) + 1)
    candidate_2 = "10" * ((len(result) // 2) + 1)

    # Print the number of bit flips between the
    # decoded stream and the (correct) ground truth
    score_1 = diff_letters(result, candidate_1[:len(result)])
    score_2 = diff_letters(result, candidate_2[:len(result)])
    score = min(score_1, score_2)

    print(score, "out of", test_intervals_no, "->", score / test_intervals_no)
    bps = 1 / ((interval/30000000)*0.01)
    print("BPS: ", bps) 

    out_file = os.path.join(os.path.dirname(directory), "errors.out")
    print(f"Output file: {out_file}")
    if not os.path.exists(out_file):
        with open(out_file, "w") as f:
            pass

    with open(out_file, "a") as f:
        f.write(f"{interval} {score / test_intervals_no} {bps} {num_receivers} {num_senders} \n")

if __name__ == "__main__":
    main()

