import sys, os
import numpy as np
import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt

gpu_list=["fmul", "fma_l", "fma_h"]
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

                outer, gpu_inst, thread_num, _ = os.path.basename(file)[:-4].split("_")
                gpu_inst = gpu_list[int(gpu_inst)]

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
                    count = total_pcore_count = np.sum(~np.all(total_data == 0,axis=2), axis=1)
                    pstates = np.array(pstates[rm_pstate_start:rm_pstate_end])
                    ppstates = np.array(pstates)
                    shade = np.zeros(ppstates.shape[0])
                    #FIXME based on P-core and E-core
                    p_pstates = ppstates[:, :6]
                    p_pstates[p_pstates == 0] = 100

                    p_pstates = np.min(p_pstates, axis=1)
                    p_pstates[p_pstates == 100] = 0
                    e_pstates = ppstates[:, 6:]
                    e_pstates[e_pstates == 0] = 100
                    e_pstates = np.min(e_pstates, axis=1)
                    e_pstates[e_pstates == 100] = 0

                    ppstates = np.column_stack((p_pstates, e_pstates))
                    
                    shade[(ppstates[:, 0] < 17) | (ppstates[:, 1] < 14)] = True
                    time = np.arange(len(ppstates)) * 0.01
                    def get_fill_regions(x, mask):
                        regions = []
                        start = None
                        for i, val in enumerate(mask):
                            if val and start is None:
                                start = i
                            elif not val and start is not None:
                                regions.append((x[start], x[i-1]))
                                start = None
                        if start is not None:
                            regions.append((x[start], x[-1]))
                        return regions
                    fill_regions = get_fill_regions(time, shade)
                    i = 0
                    width = 0.35
                    while i < len(fill_regions)-1:
                        if fill_regions[i+1][0] - fill_regions[i][1] < width:
                            fill_regions[i] = (fill_regions[i][0], fill_regions[i+1][1])
                            fill_regions.pop(i+1)
                        else:
                            i += 1

                    fig, axs = plt.subplots(figsize = (3,2))
                    df = pd.DataFrame({'idle': 14-count, 'Time(s)': [x/20 for x in range(len(total_data))] })
                    sns.lineplot(x='Time(s)', y='idle', data=df)
                    for start, end in fill_regions:
                        if end - start < width:
                            plt.axvline(x=start, color='y', alpha=0.2)
                        else:
                            plt.axvspan(start, end, color='y', alpha=0.2, linewidth=0.01)
                    plt.xlabel('Time (s)')
                    plt.ylabel('Number of idling hints')
                    plt.ylim(-1, 15)
                    plt.xlim(-2,52.5)
                    plt.tight_layout(pad=0.1)
                    plot_output_filename = "./plot/" + gpu_inst + "-idling-hints.pdf"
                    plt.savefig(plot_output_filename, dpi=300)
                    plt.clf()
                    plt.close()

                    fig, axs = plt.subplots(figsize = (3,2))
                    df = pd.DataFrame(ppstates)
                    df['Time(s)'] = [x/100 for x in range(len(ppstates))]
                    df_melted = pd.melt(df, id_vars=['Time(s)'], var_name='core', value_name='value')
                    core_array = [r'$\mathrm{{P}_{min}}$',r'$\mathrm{{E}_{min}}$']
                    tab10_palette = sns.color_palette("tab10", n_colors=10)

                    df_melted['core'] = df_melted['core'].apply(lambda x: core_array[int(x)])
                    sns.lineplot(x='Time(s)', y='value', data=df_melted, hue='core', style='core')
                    
                    for start, end in fill_regions:
                        if end - start < width:
                            plt.axvline(x=start, color='y', alpha=0.2)
                        else:
                            plt.axvspan(start, end, color='y', alpha=0.2, linewidth=0.01)

                    plt.xlabel('Time (s)')
                    plt.ylabel('P-state')
                    plt.ylim(0, 50)
                    plt.xlim(-2,52.5)
                    plt.legend(loc='upper right', prop=({'size':7}), columnspacing=0.5, labelspacing=0.3, handlelength=2.0, handletextpad=0.4, ncol=2)
                    plt.tight_layout(pad=0.1)
                    plot_output_filename = "./plot/" + gpu_inst + "-pstate.pdf"
                    plt.savefig(plot_output_filename, dpi=300)
                    plt.clf()
                    plt.close()

                print("file " + file + " complete")


if __name__ == "__main__":
    main()
