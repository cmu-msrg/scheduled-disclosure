import sys, os
import numpy as np
import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt

inst_list=[r'${\rm xor_l}$', r'${\rm xor_h}$', r'${\rm str}$', r'${\rm xor_l}$+LP', r'${\rm xor_h}$+LP', r'${\rm str}$+LP']
print_labels = [r'$\mathrm{P}_0$', r'$\mathrm{P}_1$', r'$\mathrm{P}_2$', r'$\mathrm{P}_3$', r'$\mathrm{P}_4$', r'$\mathrm{P}_5$', r'$\mathrm{E}_{0\!-\!3}$', r'$\mathrm{E}_{4\!-\!7}$']
rm_start = 1500
rm_end = 51500
pstate_ms = 10
table_ms = 50
rm_table_start = rm_start // table_ms
rm_table_end = rm_end // table_ms
rm_pstate_start = rm_start // pstate_ms
rm_pstate_end = rm_end // pstate_ms

def main():
    data_dirs = sys.argv[1:]

    try:
        os.makedirs('plot')
    except:
        pass

    data = {}
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

                outer, cpu_inst, _ = os.path.basename(file)[:-4].split("_")
                if outer == "100":
                    continue
                cpu_inst = int(cpu_inst)

                with open(file) as f:
                    value = {}
                    total_data = []
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
                            total_data.append(core_data[:14]) #FIXME based on P-core and E-core
                        else:
                            pstate_info = [x.strip() for x in line.strip()[:-1].split(",")]
                            pstate = [int(x) for x in pstate_info]
                            pstates.append(pstate)
                    total_data = np.array(total_data[rm_table_start:rm_table_end])
                    count = total_pcore_count = np.sum(~np.all(total_data == 0, axis=2), axis=1)
                    pstates = np.array(pstates[rm_pstate_start:rm_pstate_end])
                    ppstates = np.array(pstates)
                    if cpu_inst not in data:
                        data[cpu_inst] = ppstates
                    else:
                        data[cpu_inst] = np.concatenate((data[cpu_inst], ppstates), axis=0)

    data = dict(sorted(data.items()))
    total_len = 0
    dfs = []
    for cpu_inst, ppstates in data.items():
        total_len = len(ppstates)
        #FIXME based on P-core and E-core
        p0_pstates = ppstates[:, 0][ppstates[:, 0] != 0]
        p1_pstates = ppstates[:, 1][ppstates[:, 1] != 0]
        p2_pstates = ppstates[:, 2][ppstates[:, 2] != 0]
        p3_pstates = ppstates[:, 3][ppstates[:, 3] != 0]
        p4_pstates = ppstates[:, 4][ppstates[:, 4] != 0]
        p5_pstates = ppstates[:, 5][ppstates[:, 5] != 0]
        e0_pstates = ppstates[:, 6][ppstates[:, 6] != 0]
        e1_pstates = ppstates[:, 7][ppstates[:, 7] != 0]

        # concat p0~e1
        df = pd.DataFrame({
            'label': ['P0'] * len(p0_pstates) +['P1'] * len(p1_pstates) + ['P2'] * len(p2_pstates) + ['P3'] * len(p3_pstates) + ['P4'] * len(p4_pstates) + ['P5'] * len(p5_pstates) + ['E0'] * len(e0_pstates) + ['E1'] * len(e1_pstates),
            'value': np.concatenate((p0_pstates, p1_pstates, p2_pstates, p3_pstates, p4_pstates, p5_pstates, e0_pstates, e1_pstates)),
        })
        df_count = df.groupby(['label', 'value']).size().reset_index(name='count')
        dfs.append(df_count)
        
    labels = ['P0', 'P1', 'P2', 'P3', 'P4', 'P5', 'E0', "E1"]
    label_cat = pd.CategoricalDtype(categories=labels, ordered=True)
    relative_size = 0.005 *25000 / total_len
    offset = -0.26
    for df in dfs:
        df['label'] = df['label'].astype(label_cat)
        df['x'] = df['label'].cat.codes + offset
        offset += 0.13

    # Plot
    plt.figure(figsize=(12, 2.6))
    for i in range(0, len(dfs)):
        dfs[i] = dfs[i][dfs[i]['count'] > total_len * 0.002]
        dfs[i] = dfs[i][dfs[i]['value'] > 4]
        plt.scatter(dfs[i]['x'], dfs[i]['value'], 
                    s=dfs[i]['count'] * relative_size, 
                    label=inst_list[i], zorder=10)
        for label in labels:
            if len(dfs[i][dfs[i]['label'] == label]) == 0:
                continue
            mean = np.average(dfs[i][dfs[i]['label'] == label]['value'], weights=dfs[i][dfs[i]['label'] == label]['count'])
            plt.scatter(dfs[i]['x'][dfs[i]['label'] == label].values[0], mean, s=15, color='black', marker='x', linewidths=0.5, zorder=11)

    plt.xticks(ticks=range(len(labels)), labels=print_labels)
    plt.ylabel("P-state")
    plt.xlabel("CPU core")
    ax = plt.gca()
    ax.grid(True, axis='y', linewidth=0.3, zorder=0)
    if "default" in data_dirs[0]:
        name = "default"
        plt.yticks(np.arange(6, 30, 2))
    else:
        name = "lp"
        plt.yticks(np.arange(4, 16, 1))

    plt.xticks(fontsize=12)
    plt.legend(fontsize=8, loc='upper right')

    plot_output_filename = "./plot/" + name + "-pstates.pdf"
    plt.tight_layout(pad=0.1)
    plt.savefig(plot_output_filename, dpi=300)
    plt.clf()
    plt.close()

if __name__ == "__main__":
    main()
