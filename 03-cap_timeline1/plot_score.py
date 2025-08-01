import sys, os
import numpy as np
import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt

table_rm_start = 20
table_rm_end = 820
pstate_rm_start = 100
pstate_rm_end = 4100

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

                outer, thread_num, _ = os.path.basename(file)[:-4].split("_")
                if thread_num not in total_data:
                    total_data[thread_num] = {}

                with open(file) as f:
                    value = {}
                    total_perf0_data = []
                    total_eff0_data = []
                    pstates = []
                    total_time = 0
                    for line in f:
                        if ":" not in line and "," not in line:
                            total_time = float(line)
                            break
                        if ":" in line:
                            _, core_info = [x.strip() for x in line.split(":")]
                            core_info = core_info[:-1].split(",")
                            core_info = [int(x) for x in core_info]
                            core_data = np.array(core_info).reshape(-1, 8)
                            total_perf0_data.append(core_data[:,0])
                            total_eff0_data.append(core_data[:,4])
                        else:
                            pstate_info = [x.strip() for x in line.strip()[:-1].split(",")]
                            pstate = [int(x) for x in pstate_info]
                            pstates.append(pstate)
                    total_perf0_data = np.array(total_perf0_data[table_rm_start:table_rm_end])
                    total_eff0_data = np.array(total_eff0_data[table_rm_start:table_rm_end])
                    pstates = np.array(pstates[pstate_rm_start:pstate_rm_end])
                    np.set_printoptions(threshold=sys.maxsize)

                    total_pperf0_data = total_perf0_data[:,0]
                    for i in range(0, len(total_pperf0_data)):
                        if total_pperf0_data[i] == 0:
                            j = i
                            while total_pperf0_data[j] == 0:
                                j += 1
                            total_pperf0_data[i] = total_pperf0_data[j]

                    total_peff0_data = total_eff0_data[:,0]
                    for i in range(0, len(total_peff0_data)):
                        if total_peff0_data[i] == 0:
                            j = i
                            while total_peff0_data[j] == 0:
                                j += 1
                            total_peff0_data[i] = total_peff0_data[j]
                    fig, axs = plt.subplots(figsize = (3,2))
                    df = pd.DataFrame({'Perf': total_pperf0_data, 'EE': total_peff0_data, 'Time(s)': [x/20 for x in range(len(total_pperf0_data))] })
                    df_melted = df.melt(id_vars=["Time(s)"], value_vars=["Perf", "EE"], var_name="Metric", value_name="Value")
                    sns.lineplot(x='Time(s)', y='Value', data=df_melted, hue='Metric')
                    plt.xlabel('Time (s)')
                    plt.ylabel('Capabilities')
                    plt.ylim(20, 70)
                    plt.xlim(-2,42)
                    plt.legend(prop={'size': 8})
                    plt.tight_layout(pad=0.1)
                    plot_output_filename = "./plot/capability.pdf"
                    plt.savefig(plot_output_filename, dpi=300)
                    plt.clf()
                    plt.close()

                    pstates = pstates[:,0]
                    fig, axs = plt.subplots(figsize = (3,2))
                    df = pd.DataFrame({'Average PPstates': pstates, 'Time(s)': [x/100 for x in range(len(pstates))] })
                    sns.lineplot(x='Time(s)', y='Average PPstates', data=df)
                    plt.xlabel('Time (s)')
                    plt.ylabel('P-state')
                    plt.ylim(15, 50)
                    plt.xlim(-2,42)
                    plt.tight_layout(pad=0.1)
                    plot_output_filename = "./plot/pstate.pdf"
                    plt.savefig(plot_output_filename, dpi=300)
                    plt.clf()
                    plt.close()

                print("file " + file + " complete")


if __name__ == "__main__":
    main()
