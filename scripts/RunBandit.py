from scripts.BuildBandit import Build
import numpy as np
from scripts.AbstractExperiments import ExperimentRunner
from datetime import datetime
import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.ticker import MaxNLocator
from scipy.optimize import curve_fit
import numpy as np

configs = []
DeafaultRanks = [1]
DefaultActions = 50
DefaultBranches = 50
DefaultTheories = 50

minActionsIn=10
maxActionsEx=101
actionStep=10

minTheoriesIn=10
maxTheoriesEx=101
theoriesStep=10

minBranchesIn=10
maxBranchesEx=101
branchesStep=10

Env_reps = 5
config_reps = 5

if (False):
    minActionsIn=1
    maxActionsEx=5
    actionStep=1

    minTheoriesIn=1
    maxTheoriesEx=5
    theoriesStep=1

    minBranchesIn=1
    maxBranchesEx=5
    branchesStep=1

for i in range(minActionsIn,maxActionsEx,actionStep):
    for r in DeafaultRanks:
        configs += {"Name": f"Action_{i}_r{r}", "Theories": DefaultTheories, "ranks":r, "actions":i, "minBranches":DefaultBranches, "maxBranches":DefaultBranches+1},

for i in range(minTheoriesIn,maxTheoriesEx,theoriesStep):
    #ranks = np.floor_divide(i, 3)
    #ranks = ranks if ranks>0 else 1
    #ranks_2 = np.floor_divide(i*2, 3)
    #ranks_2 = ranks_2 if ranks_2>0 else 1
    ranks = 1
    configs += {"Name": f"Theories_{i}_r{ranks}", "Theories": i, "ranks":ranks, "actions":DefaultActions, "minBranches":DefaultBranches, "maxBranches":DefaultBranches+1},
    #configs += {"Name": f"Theories_{i}_r{ranks_2}", "Theories": i, "ranks":ranks_2, "actions":DefaultActions, "minBranches":DefaultBranches, "maxBranches":DefaultBranches+1},
    #configs += {"Name": f"Theories_{i}_r{i}", "Theories": i, "ranks":i, "actions":DefaultActions, "minBranches":DefaultBranches, "maxBranches":DefaultBranches+1},

for i in range(minBranchesIn,maxBranchesEx,branchesStep):
    for r in DeafaultRanks:
        configs += {"Name": f"Branches_{i}_r{r}", "Theories": DefaultTheories, "ranks":r, "actions":DefaultActions, "minBranches":i, "maxBranches":i+1},


for c in configs:
    c["Considerations"] = c["Theories"]
    c["N"] = c["minBranches"] * c["actions"] * c["Theories"]

er = ExperimentRunner("Bandit", configs)

for el in configs:
    for con_rep in range(config_reps):
        theories = el["Theories"]
        ranks = el["ranks"]
        actions = el["actions"]
        minBranches = el["minBranches"]
        maxBranches = el["maxBranches"]
        o = Build(filePath=er.makeMdpFileName(el["Name"], con_rep),
            theories=theories, numOfRanks=ranks, 
            numOfActions=actions, minBranches=minBranches, 
            maxBranches=maxBranches)


er.runPlanner(config_reps, Env_reps)
er.extractData(5, 5)
df = pd.DataFrame(er.data)
df.to_csv(er.getAllDataFilePath())
agg_rules = {
    'Mehr_time': 'mean',
}

def ProcessValue(val_name='Action', plural=None):
    plural = val_name + 's' if plural == None else plural
    # Filter dataframe for Config_name starting with 'Action_'
    filtered_df = df[df['Config_name'].str.startswith(f"{val_name}_", na=False)].copy()

    # Add actions column extracted from Config_name (first number between underscores)
    regex = rf"^{val_name}_(\d+)_"
    filtered_df[val_name] = filtered_df['Config_name'].str.extract(regex)[0].astype(float)
    if val_name=='Action':
        filtered_df['N'] = filtered_df[val_name] * DefaultBranches * DefaultTheories
    if val_name=='Theories':
        filtered_df['N'] = filtered_df[val_name] * DefaultBranches * DefaultActions
    if val_name=='Branches':
        filtered_df['N'] = filtered_df[val_name] * DefaultTheories * DefaultActions
    df.loc[filtered_df.index, 'N'] = filtered_df['N']

    # Group by actions only and compute summary statistics for Mehr_time
    summary_df = filtered_df.groupby([val_name], as_index=False).agg({
        'Mehr_time': ['mean', 'min', 'max']
    })
    summary_df.columns = [val_name, 'Mehr_time_mean', 'Mehr_time_min', 'Mehr_time_max']

    # Plot number of actions against average Mehr_time with min/max bars
    plt.figure(figsize=(6, 6))
    plt.errorbar(
        summary_df[val_name],
        summary_df['Mehr_time_mean'],
        yerr=[
            summary_df['Mehr_time_mean'] - summary_df['Mehr_time_min'],
            summary_df['Mehr_time_max'] - summary_df['Mehr_time_mean']
        ],
        fmt='o',
        capsize=5,
        linestyle='none',
        ecolor='gray',
        marker='s',
        markersize=6,
        color='blue'
    )
    
    # Fit power law: y = c * x^k
    def power_law(x, c, k):
        return c * np.power(x, k)
    
    try:
        popt, _ = curve_fit(power_law, summary_df[val_name], summary_df['Mehr_time_mean'])
        c, k = popt
        
        # Plot fitted line
        x_fit = np.linspace(summary_df[val_name].min(), summary_df[val_name].max(), 100)
        y_fit = power_law(x_fit, c, k)
        plt.plot(x_fit, y_fit, 'r-', label=f'Power: y = {c:.4f} × x^{k:.4f}')
    except:
        pass
    
    # Fit linear: y = m * x + b
    def linear(x, m, b):
        return m * x + b
    
    try:
        popt_lin, _ = curve_fit(linear, summary_df[val_name], summary_df['Mehr_time_mean'])
        m, b = popt_lin
        
        # Plot fitted line
        x_fit_lin = np.linspace(summary_df[val_name].min(), summary_df[val_name].max(), 100)
        y_fit_lin = linear(x_fit_lin, m, b)
        plt.plot(x_fit_lin, y_fit_lin, 'g--', label=f'Linear: y = {m:.4f} × x + {b:.4f}')
    except:
        pass
    
    plt.legend()
    
    ax = plt.gca()
    ax.xaxis.set_major_locator(MaxNLocator(integer=True))
    plt.xlabel(f"Total {plural}")
    plt.ylabel('Average MEHR time (microseconds)')
    plt.title(f"Relationship between Number of {plural} and Average Mehr_time")
    plt.grid(True)
    plt.savefig(f"{er.figuresFolder}/{val_name}_VS_Time.png")  # Save the plot
    plt.show()


ProcessValue('Action')
ProcessValue('Theories', 'Theories')
ProcessValue('Branches', 'Branches')

summary_N = df.groupby(['N'], as_index=False).agg({
    'Mehr_time': ['mean', 'min', 'max']
})
summary_N.columns = ['N', 'Mehr_time_mean', 'Mehr_time_min', 'Mehr_time_max']

plt.figure(figsize=(6, 6))
plt.errorbar(
    summary_N['N'],
    summary_N['Mehr_time_mean'],
    yerr=[
        summary_N['Mehr_time_mean'] - summary_N['Mehr_time_min'],
        summary_N['Mehr_time_max'] - summary_N['Mehr_time_mean']
    ],
    fmt='o',
    capsize=5,
    linestyle='none',
    ecolor='gray',
    marker='s',
    markersize=6,
    color='blue'
)
# Fit power law: y = c * x^k
def power_law(x, c, k):
    return c * np.power(x, k)

try:
    popt, _ = curve_fit(power_law, summary_N['N'], summary_N['Mehr_time_mean'])
    c, k = popt
    
    # Plot fitted line
    x_fit = np.linspace(summary_N['N'].min(), summary_N['N'].max(), 100)
    y_fit = power_law(x_fit, c, k)
    plt.plot(x_fit, y_fit, 'r-', label=f'Power: y = {c:.4f} × x^{k:.4f}')
except Exception:
    pass

# Fit linear: y = m * x + b
def linear(x, m, b):
    return m * x + b

try:
    popt_lin, _ = curve_fit(linear, summary_N['N'], summary_N['Mehr_time_mean'])
    m, b = popt_lin
    
    # Plot fitted line
    x_fit_lin = np.linspace(summary_N['N'].min(), summary_N['N'].max(), 100)
    y_fit_lin = linear(x_fit_lin, m, b)
    plt.plot(x_fit_lin, y_fit_lin, 'g--', label=f'Linear: y = {m:.4f} × x + {b:.4f}')
except Exception:
    pass

plt.legend()

ax = plt.gca()
ax.xaxis.set_major_locator(MaxNLocator(integer=True))
plt.xlabel(f"Product of #Theories, #Branches, #Actions ")
plt.ylabel('Average MEHR time (microseconds)')
plt.title(f"Relationship between product of #Moral Theories, #Actions and #Branches VS Average MEHR time")
plt.grid(True)
plt.savefig(f"{er.figuresFolder}/Product_VS_Time.png")
plt.show()