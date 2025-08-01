import numpy as np
import matplotlib.pyplot as plt
import sys
from datetime import datetime
import os

assert len(sys.argv) == 2, "Specify the file containing error rate, bps, and interval data <filename>" 

file_path = sys.argv[1] 

# Initialize lists to store data
intervals = []
error_rates = []
bps_values = []

# Read the file
with open(file_path, 'r') as file:
    for line in file:
        parts = line.split()
        print(parts)
        intervals.append(float(parts[0]))
        error_rates.append(float(parts[1]))
        bps_values.append(float(parts[2]))

# Convert lists to numpy arrays
intervals = np.array(intervals)
error_rates = np.array(error_rates)
bps_values = np.array(bps_values)

# Replace zero error rates to avoid log2(0)
error_rates[error_rates == 0] = 0.00000001

sorted_indices = np.argsort(intervals)
intervals = intervals[sorted_indices]
error_rates = error_rates[sorted_indices]
bps_values = bps_values[sorted_indices]

H = -error_rates * np.log2(error_rates) - (1 - error_rates) * np.log2(1 - error_rates)
capacity =  bps_values * (1 - H)
print(capacity)
# Create a single subplot for capacity
fig, ax4 = plt.subplots(figsize=(8, 2.5))

# Plot Capacity (bps) on the subplot with bps on the x-axis
ax4.set_xlabel('Raw bandwidth (bps)')
ax4.set_ylabel('Capacity (bps)', color='tab:blue')
line3 = ax4.scatter(bps_values, capacity, color='tab:blue', label='Capacity', linewidths=2)
ax4.plot(bps_values, capacity, color='tab:blue')
ax4.tick_params(axis='y', labelcolor='tab:blue')

# Create a second y-axis to plot error rate on the subplot
ax5 = ax4.twinx()
plt.tight_layout(pad=0.1)
ax5.set_ylabel('Error Rate', color='tab:red')
line4 = ax5.scatter(bps_values, error_rates, color='tab:red', label='Error probability', marker='x', linewidths=2)
ax5.plot(bps_values, error_rates, color='tab:red')
ax5.tick_params(axis='y', labelcolor='tab:red')
ax5.yaxis.grid(True, which='both', color='gray', linestyle='-', linewidth=0.5)  


lines = [line3, line4]
labels = [line.get_label() for line in lines]
ax4.legend(lines, labels, loc='upper left')

plt.tight_layout(pad=0.1)

output_directory = f'./plot'

if not os.path.exists(output_directory):
    os.makedirs(output_directory)

parent_directory = os.path.basename(os.path.dirname(file_path))
plt.savefig(f'{output_directory}/{parent_directory}/{parent_directory}.pdf')

