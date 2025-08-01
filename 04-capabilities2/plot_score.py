import sys, os
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns

pstate_rm_len = 500 # rm first 5s
table_rm_len = 100 # rm first 5s
inst_list=['imul','and',r'${\rm xor_l}$', r'${\rm xor_h}$', 'str']

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
                outer, cpu_inst, thread_num, _ = os.path.basename(file)[:-4].split("_")
                if int(outer) == 0:
                    continue
                cpu_inst = inst_list[int(cpu_inst)]
                thread_num = int(thread_num)
                if (cpu_inst,thread_num) not in total_data:
                    total_data[(cpu_inst,thread_num)] = {}

                with open(file) as f:
                    value = {}
                    total_score_data = []
                    pstates = []
                    total_time = 0
                    for line in f:
                        if ":" not in line and "," not in line:
                            total_time = float(line)
                            break
                        if ":" in line:
                            timestamp , core_info = [x.strip() for x in line.split(":")]
                            core_info = core_info[:-1].split(",")
                            core_info = [int(x) for x in core_info]
                            core_data = np.array(core_info).reshape(-1, 8)
                            total_score_data.append(core_data)
                        else:
                            pstate = line.strip()[:-1].split(",")
                            pstate = [int(x) for x in pstate]
                            pstates.append(pstate)
                    total_score_data = total_score_data[table_rm_len:]
                    pstates = pstates[pstate_rm_len:]
                    if total_data[(cpu_inst,thread_num)] == {}:
                        total_data[(cpu_inst,thread_num)]['score'] = np.array(total_score_data)
                        total_data[(cpu_inst,thread_num)]['pstate'] = np.array(pstates)
                        total_data[(cpu_inst,thread_num)]['time'] = total_time
                    else:
                        total_data[(cpu_inst,thread_num)]['score'] = np.append(total_data[(cpu_inst,thread_num)]['score'], np.array(total_score_data), axis=0)
                        total_data[(cpu_inst,thread_num)]['pstate'] = np.append(total_data[(cpu_inst,thread_num)]['pstate'], np.array(pstates), axis=0)
                        total_data[(cpu_inst,thread_num)]['time'] += total_time
                print("file " + file + " complete")
    summary = {}
    for (cpu_inst,thread_num), value in total_data.items():
        score = value['score']
        pstate = value['pstate']
        time = value['time']
        subfig = 0
        if thread_num not in summary:
            summary[thread_num] = {}
        summary[thread_num][cpu_inst] = {}
        active_pcore_num = thread_num if thread_num <= 6 else 6
        # remove periodic zero
        score = np.delete(score, np.where(np.all(score[:,0,0:8] == 0, axis=1) & np.any(score[:,2,0:8] != 0, axis=1)), axis=0)
        summary[thread_num][cpu_inst]['p_perf0'] = np.mean(score[:,0:active_pcore_num,0])
        summary[thread_num][cpu_inst]['p_eff0'] = np.mean(score[:,0:active_pcore_num,4])
        summary[thread_num][cpu_inst]['e_perf0'] = np.mean(score[:,6:14,0])
        summary[thread_num][cpu_inst]['e_eff0'] = np.mean(score[:,6:14,4])
        summary[thread_num][cpu_inst]['ppstate'] = np.mean(pstate[:,:active_pcore_num])
        summary[thread_num][cpu_inst]['ppstate_std'] = np.std(pstate[:,:active_pcore_num])

    df = pd.DataFrame.from_dict(summary, orient='index')
    df = df.reindex(sorted(df.index))
    df = pd.melt(df.reset_index(), id_vars=['index'], value_vars=inst_list,var_name='instruction', value_name='performance_data')
    df = pd.concat([df.drop(columns=['performance_data']), pd.json_normalize(df['performance_data'])], axis=1)

    plt.figure(figsize=(4.5, 2.6))
    df_pperf0 = df[['index','instruction','p_perf0']]
    sns.scatterplot(x=df_pperf0['index'], y=df_pperf0['p_perf0'], hue=df_pperf0['instruction'])
    plt.legend(prop={'size': 7})
    plt.xticks(np.arange(0, 17, 2))
    plt.ylabel('Capabilities (Perf)')
    plt.xlabel('Number of pinned cores')
    plot_output_filename = "./plot/perf-score.pdf"
    plt.ylim(-8, 80)
    plt.tight_layout(pad=0.1)
    plt.savefig(plot_output_filename, dpi=300)
    plt.clf()
    plt.close()
    
    plt.figure(figsize=(4.5, 2.6))
    df_peff0 = df[['index','instruction','p_eff0']]
    sns.scatterplot(x=df_peff0['index'], y=df_peff0['p_eff0'], hue=df_peff0['instruction'])
    plt.legend(prop={'size': 7}, loc="lower left")
    plt.xticks(np.arange(0, 17, 2))
    plt.ylabel('Capabilities (EE)')
    plt.xlabel('Number of pinned cores')
    plot_output_filename = "./plot/eff-score.pdf"
    plt.ylim(-4, 40)
    plt.tight_layout(pad=0.1)
    plt.savefig(plot_output_filename, dpi=300)
    plt.clf()
    plt.close()

    plt.figure(figsize=(4.5, 2.6))
    df_pperf0 = df[['index','instruction','ppstate']]
    sns.scatterplot(x=df_pperf0['index'], y=df_pperf0['ppstate'], hue=df_pperf0['instruction'])
    plt.xticks(np.arange(0, 17, 2))
    plt.ylabel('P-state')
    plt.xlabel('Number of pinned cores')
    plt.ylim(10,50)
    plt.tight_layout(pad=0.1)
    plt.legend(prop={'size': 7})
    plot_output_filename = "./plot/ppstate.pdf"
    plt.savefig(plot_output_filename, dpi=300)
    plt.clf()
    plt.close()

if __name__ == "__main__":
    main()
