import os
import sys
import numpy as np
import matplotlib.pyplot as plt
import time

#FIXME based on P-core and E-core
PCORE_SET = {0, 9, 10, 11, 12, 13}
ECORE_SET = {1, 2, 3, 4, 5, 6, 7, 8}
num_cores = 14
PLOT_TITLE = "Class Heatmap"
PDF_FILE = 'heatmaps_output.pdf'

def calculate_pcore_percentage(filepath):
    with open(filepath, 'r') as file:
        lines = file.readlines()
    
    total_cores = 0
    pcore_count = 0

    for line in lines[400:-100]:
        _, core_value = line.strip().split()  
        core_value = int(core_value)

        # Count as P-core or E-core
        total_cores += 1
        if core_value in PCORE_SET:
            pcore_count += 1

    # Return the percentage of P-cores out of total cores
    return pcore_count / total_cores if total_cores > 0 else 0

def generate_heatmap(base_dir):
    # Iterate over each classX-classY-table directory
    inputs = ["class0", "class1", "class3"]
    for i in range(0,3):
        for j in range(i,3):
            first_class = inputs[i]
            second_class = inputs[j]
            table_path = os.path.join(base_dir, f"{first_class}-{second_class}-table")

            if os.path.isdir(table_path):
                heatmap_data = []
                heatmap_data2 = []

                # Iterate over each subdirectory (e.g., 1-class1-2-class3)
                for subdir in os.listdir(table_path):
                    subdir_path = os.path.join(table_path, subdir)
                    if os.path.isdir(subdir_path):
                        # Initialize counts for input 1 and input 2
                        input_counts = {1: [], 2: []}

                        # Process each .out file in the subdirectory
                        for filename in os.listdir(subdir_path):
                            if filename.endswith('.out'):
                                # Extract the input number from filename (e.g., class1-0-1.out -> input 1)
                                _, thread_num, input_num = filename.replace('.out', '').split('-')
                                input_num = int(input_num)  # Either 1 or 2

                                # Calculate P-core percentage for this file
                                filepath = os.path.join(subdir_path, filename)
                                pcore_percentage = calculate_pcore_percentage(filepath)
                                input_counts[input_num].append(pcore_percentage)

                        # Average percentage for input1 across all files with input number 1 in this subdirectory
                        avg_pcore_input1 = sum(input_counts[1]) / len(input_counts[1]) if input_counts[1] else 0
                        heatmap_data.append(f"{subdir} {avg_pcore_input1:.2f}")

                        if first_class != second_class:
                            first_num, _, second_num, _  = subdir.split('-')
                            subdir2 = f"{second_num}-{second_class}-{first_num}-{first_class}"
                            avg_pcore_input2 = sum(input_counts[2]) / len(input_counts[2]) if input_counts[2] else 0
                            heatmap_data2.append(f"{subdir2} {avg_pcore_input2:.2f}")
                
                heatmaps_dir = os.path.join(base_dir, 'heatmaps')
                os.makedirs(heatmaps_dir, exist_ok=True)
                heatmap_file = os.path.join(heatmaps_dir, f'{first_class}-{second_class}-heatmap.out')
                heatmap_file2 = os.path.join(heatmaps_dir, f'{second_class}-{first_class}-heatmap.out')
                with open(heatmap_file, 'w') as file:
                    file.write("\n".join(heatmap_data))
                if first_class != second_class:
                    with open(heatmap_file2, 'w') as file:
                        file.write("\n".join(heatmap_data2))
                print(f"Generated {heatmap_file}")

def parse_heatmap_file(filepath):
    grid = np.full((num_cores, num_cores), np.nan)  
    with open(filepath, 'r') as file:
        for line in file:
            name, value = line.strip().split()
            value = float(value)
            
            # Extract positions from directory name, format: X-classA-Y-classB
            first_x, _, second_x, _ = name.split('-')
            first_x, second_x = int(first_x), int(second_x)
            
            # Place value in grid at specified coordinates
            if first_x >= 1 and second_x < num_cores:
                grid[first_x-1, second_x] = value
    return grid

def plot_heatmap(base_dir, pdf_file):
    fig, axes = plt.subplots(3, 3, figsize=(12, 12))

    classes = ["class1", "class0", "class3"]

    grids = {}

    for i, row_class in enumerate(classes):
        for j, col_class in enumerate(classes):

            label = f"{row_class}-{col_class}"

            ax = axes[i, j]
            
            heatmap_dir = os.path.join(base_dir, 'heatmaps')
            heatmap_file = os.path.join(heatmap_dir, "class" + str(row_class[-1]) + "-class" + str(col_class[-1]) + "-heatmap.out")
            if not os.path.exists(heatmap_file):
                print(f"Warning: {heatmap_file} does not exist.")
                ax.axis("off")
                continue

            grid = parse_heatmap_file(heatmap_file)
            grids[label] = grid

            cax = ax.imshow(grid, extent=[-0.5, num_cores - 0.5, num_cores + 0.5, 0.5], cmap='gray_r', vmin=0, vmax=1, aspect='auto')

            ax.xaxis.tick_top()

            # Set labels for outer row and column
            if j == 0 and i == 0:
                ax.set_ylabel(row_class)
                ax.xaxis.set_label_position('top')
                ax.set_xlabel(col_class, labelpad=15)
            if j == 0 and i == 1:
                ax.set_ylabel(row_class)
            if j == 0 and i == 2:
                ax.set_ylabel(row_class)

            if j == 1 and i == 0:
                ax.xaxis.set_label_position('top')
                ax.set_xlabel(col_class)
                ax.set_xlabel(col_class, labelpad=15)
            if j == 2 and i == 0:
                ax.xaxis.set_label_position('top')
                ax.set_xlabel(col_class)
                ax.set_xlabel(col_class, labelpad=15)

            ax.set_xticks(range(num_cores))
            ax.set_yticks(range(1,num_cores+1))

            # Place a black "X" in cells where data is missing (NaN values)
            for y in range(num_cores):
                for x in range(num_cores):
                    if np.isnan(grid[y, x]):
                        ax.text(x, y+1, 'X', ha='center', va='center', color='lightgray', fontsize=8, weight='bold')

    fig.tight_layout(rect=[0, 0.03, 1, 0.95])

    output_dir = os.path.join(".", "plot")
    os.makedirs(output_dir, exist_ok=True)

    output_path = os.path.join(output_dir, pdf_file)
    plt.savefig(output_path)
    print(f"Plot saved to {pdf_file}")

def main():

    assert len(sys.argv) == 2, "Specify the table dir"
    BASE_DIR = sys.argv[1]

    # Generate heatmap.out files
    generate_heatmap(BASE_DIR)

    # Plot heatmap from /table/heatmaps
    plot_heatmap(BASE_DIR, PDF_FILE)

if __name__ == "__main__":
    main()
