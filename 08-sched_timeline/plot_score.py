import sys, os
import numpy as np
import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt

cpu_list=['xorl','xorh','str']
rm_start = 1500
rm_end = 51500
table_ms = 50
rm_table_start = rm_start // table_ms
rm_table_end = rm_end // table_ms

def main():
    data_dirs = sys.argv[1:]

    try:
        os.makedirs('plot')
    except:
        pass

    for cur_dir in data_dirs:
        if not os.path.isdir(cur_dir):
            print(cur_dir + " is not a directory")
            continue
        for path, subdirs, files in os.walk(cur_dir):
            total_data = {}
            for name in files:
                file = os.path.join(path, name)
                if not (file.endswith(".out")):
                    continue

                cpu_inst, thread_num, _ = os.path.basename(file)[:-4].split("_")
                cpu_inst = cpu_list[int(cpu_inst)]

                with open(file) as f:
                    value = {}
                    total_data = []
                    total_time = 0
                    for line in f:
                        if ":" not in line and "," not in line:
                            total_time = float(line)
                            break
                        else:
                            _, core_info = [x.strip() for x in line.split(":")]
                            core_info = core_info[:-1].split(",")
                            core_info = [int(x) for x in core_info]
                            core_data = np.array(core_info).reshape(-1, 8)
                            total_data.append(core_data[:6]) #FIXME based on P-core and E-core
                    total_data = np.array(total_data[rm_table_start:rm_table_end])
                    count = total_pcore_count = np.sum(~np.all(total_data == 0,axis=2), axis=1)

                    fig, axs = plt.subplots(figsize = (3,2))
                    df = pd.DataFrame({'idle': 6-count, 'Time(s)': [x/20 for x in range(len(total_data))] }) #FIXME based on P-core and E-core
                    sns.lineplot(x='Time(s)', y='idle', data=df)
                    plt.xlabel('Time (s)')
                    plt.ylabel('Zeroed P-cores')
                    plt.ylim(-0.6, 6.6)
                    plt.xlim(-2,52.5)
                    plt.tight_layout(pad=0.1)
                    plot_output_filename = "./plot/" + cpu_inst + "-idling-hints.pdf"
                    plt.savefig(plot_output_filename, dpi=300)
                    plt.clf()
                    plt.close()

                print("file " + file + " complete")


if __name__ == "__main__":
    main()
