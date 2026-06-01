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


class ExperimentRunner:
    time_columns = ['Total_time', "Heuristic_time", 'Plan_time', 'Mehr_time', 'CQ1_time', 'CQ2_time', 'Sol_time', 'Out_time', 'Sol_reduce_time']
    def __init__(self, domain, configs:list=None, outFolder=None) -> None:
        self.domain = domain
        self.configs = configs
        self.planner = os.getcwd() + "/MPlan/cmake-build-release-clang/MPlan"
        
        self.horizon = 3
        self.budget = 18

        self.branchF = 2
        self.actionF = 2

        self.goalP = 0.7
        self.seed = 123
        
        self.ServerProcess = None

        self.loglevel = 1
        if not outFolder is None:
            self.outputFolder = outFolder
        else:
            self.outputFolder = os.getcwd() + f"/Data/Experiments/{domain}/{datetime.now().strftime("%Y-%m-%d %H:%M:%S")}"
        
        self.texTablesFolder = os.getcwd() + "/Data/TexTables"
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

    def makeMdpFileName(self, confName:str, confRep:int=None):
        return f"{self.mdpFolder}/{confName}_con{confRep}.json"

    def makePlanOutFileName(self, confName:str, confRep:int, envRep:int):
        return f"{self.rawOutFolder}/{confName}_con{confRep}_rep{envRep}.json"


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
        for i in range(configRepetitions):
            for conf in self.configs:
                MDPFactory.buildEnvToFile(self.domain, fileOut=self.makeMdpFileName(conf['Name'], i), **conf)
    
    def runPlanner(self, configRepetitions=1, envRepetitions=1):
        for conf in self.configs:
            for conf_rep in range(configRepetitions):
                inFile = self.makeMdpFileName(conf['Name'], conf_rep)
                for env_rep in range(envRepetitions):
                    self.log(f"Config Name: {conf["Name"]}; Env Repetition: {env_rep}",0)
                    outFile = self.makePlanOutFileName(conf['Name'], conf_rep, env_rep)
                    result = subprocess.run([self.planner, "--debug", "0", inFile, outFile])
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
                            "Mehr_time": json_data['Duration_MEHR'],
                            "CQ1_time": json_data['Duration_CQ1'],
                            "CQ2_time": json_data['Duration_CQ2'],
                            "Sol_time": json_data['Duration_Sols'],
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
                            "Horizon": json_data['Horizon'],
                            "Min_non_accept": json_data["Solutions"][bestPolicyIdx]["Acceptability"],
                            "Num_of_min_non_accept": json_data['Num_Min_Non_Acceptability'],
                            "Num_of_sols": len(json_data["Solutions"]),
                            "Total_Attacks": json_data["Total_Attacks"]
                        }
                        for tag in self.con_tags:
                            if (tag in json_data["Solutions"][bestPolicyIdx]["Expectation"].keys()):
                                entry[tag] = json_data["Solutions"][bestPolicyIdx]["Expectation"][tag]
                            else:
                                entry[tag] = "N/A"
                        self.data.append(entry)



    def StartServerAndPost(self, fileName):
        self.StartServer()
        time.sleep(1)
        self.PostMDPToServer(fileName)

    def StartServer(self):
        self.ServerProcess = subprocess.Popen([self.planner, "--server", "--debug", "0"])
        atexit.register(self.ServerProcess.terminate)

    def PostMDPToServer(self, fileName, fileOut=None):
        req = {'file_in': fileName, 'from_data_folder':False}
        if fileOut:
            req['file_out'] = fileOut
        self.buildEnvironments(1)
        resp = requests.post("http://localhost:18080/MDP", json=req)
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
    
    def GetCachedSuccessorsFromServer(self):
        return requests.post("http://localhost:18080/AggregateCachedSuccessors")
        

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

    

    def getAllDataFilePath(self) -> str:
        return self.outputFolder + "/all_data.csv"

    def getDurationsFilePath(self) -> str:
        return self.outputFolder + "/durations.csv"

    def getDurationsByTheoryFilePath(self) -> str:
        return self.outputFolder + "/durations_by_theory.csv"
    
    def getTheoryExpectationsFilePath(self) -> str:
        return self.outputFolder + "/theory_expectations.csv"

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

        print(theoryTimes)
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


    def loadResults(self, of):
        self.outputFolder = of
        df = pd.read_csv(self.getAllDataFilePath())
        self.data = df.to_dict()


    #TODO update strings below. 
    def plotTimeResults(self, df=None):
        if (df==None):
            df = pd.DataFrame(self.data)

        average_durations = df.groupby('Horizon')[ExperimentRunner.time_columns].mean().reset_index()
        average_durations['Mehr_time'] = average_durations['Mehr_time'].replace(0, 0.0001)

        plt.figure(figsize=(10, 6))
        for column in ExperimentRunner.time_columns:
            plt.scatter(average_durations['Horizon'], average_durations[column], label=column)

            plt.plot(average_durations['Horizon'], average_durations[column], linestyle='-', alpha=0.6)
            # Add a line of best fit
            x = average_durations['Horizon']
            y = average_durations[column]
            slope, intercept, r_value, _, _ = linregress(x, np.log10(y))  # Fit line in log scale
            best_fit_line = 10 ** (slope * x + intercept)  # Transform back to original scale
            plt.plot(x, best_fit_line, linestyle='--', alpha=0.8, label=f'{column} Best Fit (R={r_value:.2f})')

        plt.yscale('log')  # Set the y-axis to logarithmic scale
        plt.title("Time Metrics vs Horizon (Logarithmic Scale)")
        plt.xlabel("Horizon")
        plt.ylabel("Time (microseconds, log scale)")
        plt.xticks(ticks=np.arange(min(average_durations['Horizon']), max(average_durations['Horizon']) + 1, 1))
        plt.legend()
        #plt.grid(True, which='both', linestyle='--', linewidth=0.5)   Adjust grid for log scale
        plt.savefig(self.outputFolder+ "timeVHorizon_LOG_SCALE.png")  # Save the plot

        plt.show()  # Show the plot

    def plotPercentTimeResults(self, df=None):
        if (df==None):
            df = pd.DataFrame(self.data)
        average_durations = df.groupby('Horizon')[ExperimentRunner.time_columns].mean().reset_index()
        cols = copy.deepcopy(ExperimentRunner.time_columns)
        cols.remove('Total_time')
        for column in cols:
            average_durations[f'{column}_percent'] = (average_durations[column] / average_durations['Total_time']) * 100

        plt.figure(figsize=(10, 6))
        cols_percent = [c + "_percent" for c in cols]
        for column in cols_percent:
            plt.plot(average_durations['Horizon'], average_durations[column], marker='o', label=column.replace('_percent', ''))

        plt.title("Time Components as Percentage of Total Time vs Horizon")
        plt.xlabel("Horizon")
        plt.ylabel("Percentage of Total Time (%)")
        plt.xticks(ticks=np.arange(min(average_durations['Horizon']), max(average_durations['Horizon']) + 1, 1))

        plt.legend()
        plt.savefig(self.outputFolder + "time_components_percentage_vs_horizon.png")  # Save the plot
        plt.show()

    def plotParetoGraph(self, configName:str, conf_rep:int, con_one_label:str, con_two_label:str, env_rep:int=0, fileName:str="ParetoGraph.png"):
        outFile = self.makePlanOutFileName(configName, conf_rep, env_rep)

        with open(outFile, 'r') as file:
            json_data = json.load(file)
        x = []
        for sol in json_data["Solutions"]:
            x.append([float(sol["Expectation"][con_one_label]), float(sol["Expectation"][con_two_label]), sol["Acceptability"]])
        
        df = pd.DataFrame(x, columns=[con_one_label, con_two_label, "Non-acceptability"])
        
        # FIX: Sort so low non-acceptability (green) is drawn LAST (on top)
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
        plt.title(f"Expected moral worth of policies under {con_one_label} and {con_two_label}")
        
        cbar = plt.colorbar(scatter)
        cbar.set_label('Non-acceptability (Lower = Green, Higher = Red)')

        plt.grid(True, linestyle='--', alpha=0.6)
        plt.tight_layout()

        plt.savefig(f"{self.figuresFolder}/{fileName}")
        plt.show()