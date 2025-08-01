import sys, os
import numpy as np
import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt


gpu_list=['fmul','fmal','fmah']
rm_start = 1500
rm_end = 51500
sample_ms = 5
rm_sample_start = rm_start // sample_ms
rm_sample_end = rm_end // sample_ms

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

                gpu_inst, thread_num, _ = os.path.basename(file)[:-4].split("_")
                gpu_inst = gpu_list[int(gpu_inst)]

                with open(file) as f:
                    core_data = []
                    for line in f:
                        core_info = line.strip().split(" ")
                        core_info = [int(x) for x in core_info]
                        core_data.append(core_info[rm_sample_start:rm_sample_end])
                    core_data = np.array(core_data)
                    timeline_coredata = core_data.T
                    timeline_pcount1 = np.sum(timeline_coredata == 0, axis=1)
                    timeline_pcount1 = np.where(timeline_pcount1 > 0, 1, 0)
                    timeline_pcount2 = np.sum(timeline_coredata == 9, axis=1)
                    timeline_pcount2 = np.where(timeline_pcount2 > 0, 1, 0)
                    timeline_pcount3 = np.sum(timeline_coredata == 10, axis=1)
                    timeline_pcount3 = np.where(timeline_pcount3 > 0, 1, 0)
                    timeline_pcount4 = np.sum(timeline_coredata == 11, axis=1)
                    timeline_pcount4 = np.where(timeline_pcount4 > 0, 1, 0)
                    timeline_pcount5 = np.sum(timeline_coredata == 12, axis=1)
                    timeline_pcount5 = np.where(timeline_pcount5 > 0, 1, 0)
                    timeline_pcount6 = np.sum(timeline_coredata == 13, axis=1)
                    timeline_pcount6 = np.where(timeline_pcount6 > 0, 1, 0)
                    timeline_pcount = timeline_pcount1 + timeline_pcount2 + timeline_pcount3 + timeline_pcount4 + timeline_pcount5 + timeline_pcount6
                    timeline_pcount = timeline_pcount

                    fig, axs = plt.subplots(figsize = (3,2))
                    df = pd.DataFrame({'Pcore usage': timeline_pcount, 'Time(s)': [x/200 for x in range(len(timeline_pcount))] })
                    sns.lineplot(x='Time(s)', y='Pcore usage', data=df)
                    plt.xlabel('Time (s)')
                    plt.ylabel('Used P-cores')
                    plt.ylim(-0.6, 6.6)
                    plt.xlim(-2,47)
                    plt.tight_layout(pad=0.1)
                    plot_output_filename = "./plot/" + gpu_inst + "-pcore-usage.pdf"
                    plt.savefig(plot_output_filename, dpi=300)
                    plt.clf()
                    plt.close()
                print("file " + file + " complete")


if __name__ == "__main__":
    main()
