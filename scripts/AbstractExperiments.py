import subprocess
import os
import pandas as pd
import numpy as np
from EnvironmentBuilder import MDPFactory
import json
from datetime import datetime
import matplotlib.pyplot as plt
from scipy.stats import linregress
import copy
import time
import requests
import atexit
from scripts.TexTables import SaveDataFrameToTexTemplate
import re
from matplotlib.ticker import MaxNLocator
import ast


def latex_config_name(name):
    return "$" + name.replace("^0", "^{0}") + "$"


def GenerateConfigs(inputConfigs, defaultConfig):
    configs = []
    for name, dat in inputConfigs.items():
        c = copy.deepcopy(defaultConfig)
        c["Name"] = name
        c["Theories"] = dat["Theories"]
        c["Considerations"] = dat["Considerations"]
        if "Budget" in dat.keys():
            c["Budget"] = dat["Budget"]
        c["Horizon"] = dat["Horizon"] if "Horizon" in dat.keys() else c["Horizon"]
        configs.append(c)
    return configs

class ExperimentRunner:
    time_columns = ['Total_time', "Heuristic_time", 'Plan_time', 'Mehr_time', 'Sol_time']
    def __init__(self, domain, configs:list=None, outFolder=None, MoralPlanner_Location="/", date_time=None) -> None:
        fs_start = MoralPlanner_Location
        self.domain = domain
        self.configs = configs
        self.planner = f"{os.getcwd()}{fs_start}MPlan/cmake-build-release-clang/MPlan"
        self.planner = f"{os.getcwd()}{fs_start}MPlan/cmake-build-release/MPlan"
        self.hasPlannerRun = False

        self.horizon = 3
        self.budget = 18

        self.branchF = 2
        self.actionF = 2

        self.goalP = 0.7
        self.seed = 123
        
        self.ServerProcess = None
        
        self.datetimeNow = date_time
        if date_time is None:
            self.datetimeNow = datetime.now().strftime("%Y-%m-%d_%H:%M:%S")
        
        self.loglevel = 1
        if not outFolder is None:
            self.outputFolder = f"{os.getcwd()}{fs_start}Data/Experiments/{domain}/{outFolder}"
        else:
            self.outputFolder = f"{os.getcwd()}{fs_start}Data/Experiments/{domain}/{self.datetimeNow}"
        
        self.texTablesFolder = f"{os.getcwd()}{fs_start}Data/TexTables"
        self.mdpFolder = f"{self.outputFolder}/mdps"
        self.figuresFolder = f"{self.outputFolder}/Figures"
        self.rawOutFolder = f"{self.outputFolder}/raw"
        self.texOutFolder = f"{self.outputFolder}/tex"

        os.makedirs(self.outputFolder, exist_ok=True)
        os.makedirs(self.figuresFolder, exist_ok=True)
        os.makedirs(self.mdpFolder, exist_ok=True)
        os.makedirs(self.rawOutFolder, exist_ok=True)
        os.makedirs(self.texOutFolder, exist_ok=True)

    def log(self, msg:str, level:int):
        if level > self.loglevel:
            print(msg)

    #
    # File path stuff
    #
    def makeMdpFileName(self, confName:str, confRep:int=None):
        return f"{self.mdpFolder}/{confName}_con{confRep}.json"

    def makePlanOutFileName(self, confName:str, confRep:int, envRep:int):
        return f"{self.rawOutFolder}/{confName}_con{confRep}_rep{envRep}.json"
    
    def getAllDataFilePath(self) -> str:
        return self.outputFolder + "/all_data.csv"

    def getDurationsFilePath(self) -> str:
        return self.outputFolder + "/durations.csv"

    def getDurationsByTheoryFilePath(self) -> str:
        return self.outputFolder + "/durations_by_theory.csv"
    
    def getTheoryExpectationsFilePath(self) -> str:
        return self.outputFolder + "/theory_expectations.csv"


    def run(self, configRepetitions=1, envRepetitions=1):
        # Generate Environments.
        self.log("*********Builing environments...*********",0)
        self.buildEnvironments(configRepetitions)
        self.log("*********Built all environments!*********",0)

        # Call planner on all envs
        self.log("*********Planning...*********",0)
        self.runPlanner(configRepetitions, envRepetitions)
        self.log("*********Finished planning!*********",0)

        # Extract all consideration tags from across output files
        # (inefficient reading each file twice.)
        self.extractData(configRepetitions, envRepetitions)

    def buildEnvironments(self, configRepetitions:int=1):
        mdps = {}
        for i in range(configRepetitions):
            for conf in self.configs:
                mdps.setdefault(conf["Name"], [])
                mdps[conf["Name"]].append(MDPFactory.buildEnvToFile(self.domain, fileOut=self.makeMdpFileName(conf['Name'], i), **conf))
        return mdps
    
    def runPlanner(self, configRepetitions=1, envRepetitions=1):
        for conf in self.configs:
            for conf_rep in range(configRepetitions):
                inFile = self.makeMdpFileName(conf['Name'], conf_rep)
                for env_rep in range(envRepetitions):
                    self.log(f"Config Name: {conf["Name"]}; Env Repetition: {env_rep}",0)
                    outFile = self.makePlanOutFileName(conf['Name'], conf_rep, env_rep)
                    p = [self.planner, "--debug", "0", inFile, outFile]
                    print('EXECUTE ' + ' '.join(p))
                    result = subprocess.run(p)
                    result.check_returncode()
            self.log(f"Finished env on {conf["Name"]}.",0)

    def extractData(self, configRepetitions=1, envRepetitions=1):
        self.con_tags = []
        for conf_rep in range(configRepetitions):
            for conf in self.configs:
                for env_rep in range(envRepetitions):
                    # open file
                    outFile = self.makePlanOutFileName(conf['Name'], conf_rep, env_rep)
                    with open(outFile, 'r') as file:
                        json_data = json.load(file)
                        for tag in json_data["Solutions"][0]["Expectation"].keys():
                            if (not tag in self.con_tags):
                                self.con_tags.append(tag)

        # Open output file from planner and interpret
        self.data = []
        for conf_rep in range(configRepetitions):
            for conf in self.configs:
                for env_rep in range(envRepetitions):
                    # open file
                    outFile = self.makePlanOutFileName(conf['Name'], conf_rep, env_rep)
                    with open(outFile, 'r') as file:
                        json_data = json.load(file)
                        bestPolicyIdx = json_data['Solutions_Order'][0]
                        entry = {
                            "Config_name": conf['Name'],
                            "Conf_rep": conf_rep,
                            "Env_rep": env_rep,
                            "Theories": str(conf['Theories']),
                            "Considerations": str(conf['Considerations']),
                            "Total_time": json_data['Duration_Total'],
                            "Heuristic_time": json_data['Duration_Heuristic'],
                            "Plan_time": json_data['Duration_Plan'],
                            "Sol_time": json_data['Duration_Sols'],
                            "Mehr_time": json_data['Duration_MEHR'],
                            "CQ1_time": json_data['Duration_CQ1'],
                            "CQ2_time": json_data['Duration_CQ2'],
                            "Out_time": json_data['Duration_Outs'],
                            "Sol_reduce_time": json_data['Duration_Sols_Reduce'],
                            "Expanded_states": json_data['Expanded'],
                            "BSG_states": json_data['Best_subgraph_size'],
                            "Average_histories": json_data['Average_histories'],
                            "Max_histories": json_data['Max_histories'],
                            "Min_histories": json_data['Min_histories'],
                            "Total_states": json_data['Total_states'],
                            "Backups": json_data['Backups'],
                            "Iterations": json_data['Iterations'],
                            "Horizon": json_data['Horizon'] - 1,
                            "Min_non_accept": json_data["Solutions"][bestPolicyIdx]["Acceptability"],
                            "Num_of_min_non_accept": json_data['Num_Min_Non_Acceptability'],
                            "Num_of_sols": len(json_data["Solutions"]),
                            "Total_Attacks": json_data["Total_Attacks"],
                            "Total_reachable_policies": json_data["Total_reachable_policies"]
                        }
                        for tag in self.con_tags:
                            if (tag in json_data["Solutions"][bestPolicyIdx]["Expectation"].keys()):
                                entry[tag] = json_data["Solutions"][bestPolicyIdx]["Expectation"][tag]
                            else:
                                entry[tag] = "N/A"
                        self.data.append(entry)

    def saveResults(self):
        # Save all data csv
        df = pd.DataFrame(self.data)
        df.to_csv(self.getAllDataFilePath())

        # Save durations csv
        cols = ExperimentRunner.time_columns + ['Num_of_sols']
        average_durations = df.groupby('Horizon')[cols].mean().reset_index()
        average_durations.to_csv(self.getDurationsFilePath())
        
        # Save durations by theory csv
        agg_rules = {
            'Heuristic_time': 'mean',
            'Plan_time': 'mean',
            'Sol_time': 'mean',
            'Mehr_time': 'mean',
            'Total_time': 'mean',
            'Backups': 'mean',
            'Num_of_sols': 'mean'
        }
        theoryTimes = df.groupby('Config_name', sort=False).agg(agg_rules).reset_index()
        theoryTimes.to_csv(self.getDurationsByTheoryFilePath())

        SaveDataFrameToTexTemplate(theoryTimes, f"{self.texTablesFolder}/Time_table.tex", f"{self.texOutFolder}/time_table.tex")

        # Check consideration worth is the same across same config.
        cols = ["Config_name", "Conf_rep", "Env_rep", "Min_non_accept", "Num_of_min_non_accept"]
        cols.extend(self.con_tags)
        u = df[cols].groupby(["Config_name", "Conf_rep", "Env_rep"], sort=False)
        self.log(u.head(),0)
        uniqueValues = u.nunique()
        if (not (uniqueValues==1).all().all()):
            raise Exception("Sim: Different iterations returned different expected worth!")

        theoryResults = df[cols].groupby('Config_name', sort=False).first()
        theoryResults.to_csv(self.getTheoryExpectationsFilePath())
        SaveDataFrameToTexTemplate(theoryResults, f"{self.texTablesFolder}/UtilitarianResults.tex", f"{self.texOutFolder}/Utility_table.tex")

    def GetWorthData(self, worth_tags, configRepetitions=1, envRepetitions=1):
        # Open output file from planner and interpret
        data = []
        for conf_rep in range(configRepetitions):
            for conf in self.configs:
                for env_rep in range(envRepetitions):
                    # open file
                    outFile = self.makePlanOutFileName(conf['Name'], conf_rep, env_rep)
                    with open(outFile, 'r') as file:
                        json_data = json.load(file)
                        soln_order = json_data['Solutions_Order']
                        num_nacc = json_data['Num_Min_Non_Acceptability']
                        for i in range(num_nacc):
                            e = {"Config_name": conf['Name'],
                                "Conf_rep": conf_rep,
                                "Env_rep": env_rep,
                                "Horizon": json_data["Horizon"],
                                "Moral_policy_idx": i,
                                "Non-acceptability": json_data["Solutions"][soln_order[i]]["Acceptability"],
                            }
                            for wt in worth_tags:
                                if wt in json_data["Solutions"][soln_order[i]]["Expectation"].keys():
                                    e[wt] = json_data["Solutions"][soln_order[i]]["Expectation"][wt]
                                else:
                                    e[wt] = np.nan
                            data.append(e)
        return data


    #
    # Server Stuff
    #
    def StartServerAndPost(self, fileName, port=18080):
        self.StartServer(port)
        time.sleep(1)
        self.PostMDPToServer(fileName, port)

    def StartServer(self, port=18080, autoTerminate=True):
        self.ServerProcess = subprocess.Popen([self.planner, "--server", "--debug", "3", "--port", str(port)])
        if autoTerminate:
            atexit.register(self.ServerProcess.terminate)

    def PostMDPToServer(self, fileName, port=18080, fileOut=None, from_data_folder=False):
        req = {'file_in': fileName, 'from_data_folder':from_data_folder}
        if not fileOut is None:
            req['file_out'] = fileOut
        resp = requests.post(f"http://localhost:{str(port)}/MDP", json=req)
        if (resp.status_code!=200):
            self.log(f"MPlan Server Error: {resp.reason}",0)
            if not self.ServerProcess is None:
                self.ServerProcess.terminate()
            return
        dat = resp.json
        resp.close()
        return dat

    def CacheSuccessorsOnServer(self, scrs, actions):
        scr_states = [0] + [s[1] for s in scrs]
        req = {'states_index': scr_states, 'actions':actions}
        resp = requests.post("http://localhost:18080/CacheSuccessors", json=req)
    
    def GetCachedSuccessorsFromServer(self) -> requests.Response:
        return requests.post("http://localhost:18080/AggregateCachedSuccessors")
        
    #
    # Lookahead Methods
    #
    def PlanWithLookahead(self, 
                          domain="Rescue", 
                          filename="", 
                          real_horizon=20, 
                          look_ahead=4, 
                          config_index=0,
                          act_ahead=1):
        scr_sequence = []
        scr_tag_sequence = []
        action_sequence = []
        moral_pols = []

        curr_state = 0
        scr_props = None
        for i in range(0, real_horizon, act_ahead):
            # 1. Generate MDP from timestep
            lookahead_ = look_ahead
            if "Horizon" in self.configs[config_index].keys():
                lookahead_ = self.configs[config_index]["Horizon"]
            mdp = self.makeMDP(domain,
                        Theories = self.configs[config_index]["Theories"],
                        Considerations = self.configs[config_index]["Considerations"],
                        Horizon = lookahead_, 
                        initialProps=scr_props)
            mdp.makeAllStatesExplicit()
            # 2. Save new MDP
            curr_file = f"{filename}_t={str(i).rjust(2, '0')}.json"
            mdp_file = f"{self.mdpFolder}/{curr_file}"
            self.SaveEnvToJSON(mdp, mdp_file, domain)
            # 3. Plan on MDP
            fo = f"{self.rawOutFolder}/{curr_file}"
            self.PostMDPToServer(fileName=mdp_file, fileOut=fo, from_data_folder=False)
            # 4. Sample random trajectory
            dat = self.SampleTrajectoryFromServer(act_ahead, add_worth_to_history=True)
            scrs = []
            for i in range(len(dat['visited_states'])):
                curr_scr = [dat['transition_probabilities'][i], dat['visited_states'][i]]
                for key, val in dat['transition_worth'].items():
                    curr_scr.extend(val)
                scrs.append(curr_scr)
            action_sequence = dat['actions']
            
            with open(fo) as f:
                d = json.load(f)
                for i in dat['visited_states']:
                    scr_props = d["State_tags"][[i]]
                    scr_props = ast.literal_eval(scr_props)
                    scr_props["time"] = 0
                    if (i==0):
                        scr_tag_sequence.append(ast.literal_eval(d["State_tags"][0]))
                scr_tag_sequence.append(scr_props)

        dat = self.GetCachedSuccessorsFromServer()
        dat = dat.json()
        cumulative_worth = dat["Cumulative_Worth"]
        transitions_worth = dat["Total_History"]



    def PostNextMDPToServer(self, fileName, policy_idx, history_idx):
        self.buildEnvironments(1)
        resp = requests.post("http://localhost:18080/PlanFromHistory", json={'policyIdx': fileName, 'from_data_folder':False})
        if (resp.status_code!=200):
            self.log(f"MPlan Server Error: {resp.reason}",0)
            self.ServerProcess.terminate()
            return
        dat = resp.json
        resp.close()
        return dat

    def SampleTrajectoryFromServer(self, time_steps:int, add_worth_to_history:bool = True, seed=None):
        req = {'time_steps': time_steps, 'add_worth_to_history': add_worth_to_history}
        if not seed is None:
            req['seed'] = seed

        resp = requests.post("http://localhost:18080/RandomTrajectory", json=req)
        dat = resp.json
        resp.close()
        return dat


    #
    # Data Visualisation
    #
    def loadResults(self, of):
        self.outputFolder = of
        df = pd.read_csv(self.getAllDataFilePath())
        self.data = df.to_dict()

    def plotConfigsAgainstHorizon(self, configs, 
                                  dep_var="Hal",
                                  title=None,
                                  x_title=None,
                                  y_title=None,
                                  legend_loc=None,
                                  df="None"):

        if (not isinstance(df, pd.DataFrame)):
            df = pd.DataFrame(self.data)
        
        # Ensure numeric values
        df[dep_var] = pd.to_numeric(df[dep_var])
        df["Horizon"] = pd.to_numeric(df["Horizon"])

        fig, ax = plt.subplots(figsize=(10, 7))

        linestyles = ["-", "--", "-.", ":"]
        markers = ["o", "s", "^", "D", "x", "*"]

        for i, config_name in enumerate(configs):
            group = (
                df[df["Config_name"] == config_name]
                .groupby("Horizon", as_index=False)[dep_var]
                .mean()
                .sort_values("Horizon")
            )
            ax.plot(
                group["Horizon"],
                group[dep_var],
                marker=markers[i  % len(markers)],
                label=latex_config_name(config_name),
                alpha=0.75,
                linewidth=1.5,
                linestyle=linestyles[i % len(linestyles)]
            )

        ax.xaxis.set_major_locator(MaxNLocator(integer=True))
        if title is None:
            title = f"Horizon vs. {dep_var}"
        ax.set_title(title)
        
        if x_title is None:
            x_title = "Horizon"
        if y_title is None:
            y_title = dep_var

        ax.set_xlabel(x_title)
        ax.set_ylabel(y_title)
        
        if legend_loc is None:
            legend_loc = "upper left"

        ax.legend(loc=legend_loc)
        fig.tight_layout()

    def plotTimeResults(self, config_names=None, df="", title=None, legend_loc="upper left"):
        if (not isinstance(df, pd.DataFrame)):
            df = pd.DataFrame(self.data)

        if (config_names is None):
            config_names = sorted(df['Config_name'].unique())
        elif isinstance(config_names, str):
            config_names = [config_names]

        average_durations = df.groupby(['Config_name', 'Horizon'])[ExperimentRunner.time_columns].mean().reset_index()
        average_durations['Mehr_time'] = average_durations['Mehr_time'].replace(0, 0.0001)

        plt.figure(figsize=(12, 7))

        color_cycle = plt.rcParams['axes.prop_cycle'].by_key()['color']
        component_colors = {
            col: color_cycle[i % len(color_cycle)]
            for i, col in enumerate(ExperimentRunner.time_columns)
        }
        line_styles = ['-', '--', '-.', ':']
        markers = ['o', 's', '^', 'D', 'x', '*', 'P', 'X']
        
        for config_idx, config_name in enumerate(config_names):
            config_group = average_durations[average_durations['Config_name'] == config_name]
            if config_group.empty:
                continue

            line_style = line_styles[config_idx % len(line_styles)]
            marker = markers[config_idx % len(markers)]
            for column in ExperimentRunner.time_columns:
                plt.plot(
                    config_group['Horizon'],
                    config_group[column],
                    label=f"{config_name} - {column}",
                    color=component_colors[column],
                    linestyle=line_style,
                    marker=marker,
                    alpha=0.85,
                    linewidth=1.7,
                    markersize=6,
                )

        plt.yscale('log')
        if title is None:
            title = "Time Metrics vs Horizon (Log Scale, same color = same component)"
        plt.title(title)
        plt.xlabel("Horizon")
        plt.ylabel("Time (microseconds, log scale)")

        horizons = average_durations['Horizon']
        if not horizons.empty:
            plt.xticks(ticks=np.arange(min(horizons), max(horizons) + 1, 1))

        plt.legend(loc=legend_loc, fontsize='small', ncol=2)
        plt.grid(True, which='both', linestyle='--', linewidth=0.5, alpha=0.4)
        plt.savefig(self.outputFolder + "timeVHorizon_LOG_SCALE.png")
        plt.show()

    def plotPercentTimeResults(self, config_name=None, df="", legend_loc=None, latex_name=None, labels=None):
        if (not isinstance(df, pd.DataFrame)):
            df = pd.DataFrame(self.data)
        
        if latex_name is None:
            latex_name = config_name
        
        average_durations = df.copy()
        if (not (config_name is None)):
            average_durations = average_durations[average_durations['Config_name']==config_name]

        average_durations = average_durations.groupby('Horizon')[ExperimentRunner.time_columns].mean().reset_index()
        cols = copy.deepcopy(ExperimentRunner.time_columns)
        cols.remove('Total_time')
        for column in cols:
            average_durations[f'{column}_percent'] = (average_durations[column] / average_durations['Total_time']) * 100

        plt.figure(figsize=(10, 6))
        cols_percent = [c + "_percent" for c in cols]
        for column in cols_percent:
            l = column.replace('_percent', '')
            if not (labels is None):
                l = labels[l]
            plt.plot(average_durations['Horizon'], average_durations[column], marker='o', label=l)

        plt.title(f"Time Components as Percentage of Total Time vs Horizon for {latex_config_name(config_name)}")
        plt.xlabel("Horizon")
        plt.ylabel("Percentage of Total Time (%)")
        plt.xticks(ticks=np.arange(min(average_durations['Horizon']), max(average_durations['Horizon']) + 1, 1))

        plt.legend(loc=legend_loc)

    def plotParetoGraph(self, configName:str, conf_rep:int, con_one_label:str, con_two_label:str, env_rep:int=0, fileName:str="ParetoGraph.png"):
        outFile = self.makePlanOutFileName(configName, conf_rep, env_rep)

        with open(outFile, 'r') as file:
            json_data = json.load(file)
        x = []
        for sol in json_data["Solutions"]:
            x.append([float(sol["Expectation"][con_one_label]), float(sol["Expectation"][con_two_label]), sol["Acceptability"]])
        
        df = pd.DataFrame(x, columns=[con_one_label, con_two_label, "Non-acceptability"])
        
        df = df.sort_values(by="Non-acceptability", ascending=False)

        plt.figure(figsize=(10,7))
        scatter = plt.scatter(
            df[con_one_label], 
            df[con_two_label], 
            c=df["Non-acceptability"], 
            cmap='RdYlGn_r', 
            edgecolors='black',
            linewidth=0.3, # Thinner line helps reduce clutter
            alpha=0.8,     # Slight transparency helps see density
            s=35           # Explicitly set size
        )
        plt.xlabel(con_one_label)
        plt.ylabel(con_two_label)
        plt.title(f"Expected moral worth of {len(json_data["Solutions"])} proper Pareto front policies by {con_one_label} and {con_two_label}")
        
        cbar = plt.colorbar(scatter)
        cbar.set_label('Non-acceptability (Lower = Green, Higher = Red)')

        plt.grid(True, linestyle='--', alpha=0.6)
        plt.tight_layout()

        plt.savefig(f"{self.figuresFolder}/{configName}_{fileName}", dpi=300)
        plt.show()
    
    def plotNonAcceptGraph(self, config_names:list, fileName:str="NaccGraph.png"):

        config_names = ["(Hal)^0, (Carla)^0",
                 "(Hal)^0, (Carla)^0, (Hal,Carla)^0",
                 "(HalLife)^0, (CaraLife)^0"
                 ]

        for c in config_names:
            # Assuming self.makePlanOutFileName exists
            outFile = self.makePlanOutFileName(c, 0, 0)
            with open(outFile, 'r') as file:
                json_data = json.load(file)
                nacc = [sol["Acceptability"] for sol in json_data["Solutions"]]  
                x_sorted = np.sort(nacc)
                y_cumulative = np.arange(1, len(x_sorted) + 1)
                
                formatted_label = re.sub(r"\^(\d)", r"$^{\1}$", str(c))
                
        plt.figure(figsize=(8, 5))
        plt.step(y_cumulative, x_sorted, where='post', linewidth=2, label=formatted_label)
        # 4. Create the step plot
        plt.title(f"Cumulative proper Pareto front policies vs. Non-Acceptability")
        plt.ylabel("Non-acceptability")
        plt.xlabel("Cumulative proper Pareto front policies")
        plt.grid(True, linestyle='--', alpha=0.6)

        plt.ylim(bottom=0, top=2.5) 
        plt.xlim(left=1) 

        plt.legend(title="Configurations")

        plt.tight_layout()

        plt.tight_layout()
        plt.savefig(f"{self.figuresFolder}/{fileName}", dpi=300)
        plt.show()

    def plotTimeChart(self, configs, envRepetitions:int, fileName:str="ConfigsPlot.png"):
        data = []
        for c in configs.keys():
            for env_rep in range(envRepetitions):
                # Assuming self.makePlanOutFileName exists
                outFile = self.makePlanOutFileName(c, 0, env_rep)
                with open(outFile, 'r') as file:
                    json_data = json.load(file)
                    data.append([c, env_rep, 
                                 json_data["Duration_Heuristic"],
                                 json_data["Duration_Plan"],
                                 json_data["Duration_Sols"],
                                 json_data["Duration_MEHR"],
                                 json_data["Duration_Total"]])
                                 
        df = pd.DataFrame(data, columns=["Config_name", "Env_rep", "Heuristic", "Plan", "Solution Extraction", "MEHR", "Total"])
        time_categories = ['Heuristic', 'Plan', 'Solution Extraction', 'MEHR', 'Total']
        
        averages = df.groupby('Config_name', sort=False)[time_categories].mean()

        fig, ax = plt.subplots(layout='constrained', figsize=(10, 6))
        
        averages.plot.bar(ax=ax, width=0.8, rot=0)

        for container in ax.containers:
            ax.bar_label(container, padding=3, fmt='%.1f')
            
        ax.set_ylabel('Duration (microseconds)')
        ax.set_xlabel('')
        ax.set_title('CPU time on Utilitarian Lost Insulin problems')
        
        ax.legend(loc='upper left', ncols=5)
        ax.set_ylim(0, df['Total'].max() * 1.25)
        
        plt.savefig(f"{self.figuresFolder}/{fileName}", dpi=300)
        #plt.show()


    def TexSummary(self, agg_rules:dict, tex_template, tex_output, merge:dict={}, round=3):
        df = pd.DataFrame(self.data)
        df = df.replace(["", "N/A", "NA", "nan", "None"], np.nan)

        for new_key, old_keys in merge.items():
            for k in range(len(old_keys) - 1):
                df[new_key] = df[old_keys[k]].combine_first(old_keys+1)
        df = df.replace(np.nan, "SKIP")

        df = df[['Config_name'] + list(agg_rules.keys())].groupby('Config_name', sort=False).agg(agg_rules)
        df.round(3)
        SaveDataFrameToTexTemplate(df,
                                   f"{self.texTablesFolder}/{tex_template}", f"{self.texOutFolder}/{tex_output}", 
                                   row_template_mode=False,
                                   timestamp=self.datetimeNow)
        return df
