import subprocess
import os
import pandas as pd
import numpy as np
from EnvironmentBuilder import MDPFactory
import json
from datetime import datetime
import matplotlib.pyplot as plt
from scipy.stats import linregress


class ExperimentRunner:
    def __init__(self, domain, configs:list, outFolder=None) -> None:
        self.domain = domain
        self.configs = configs
        self.planner = os.getcwd() + "/MPlan/cmake-build-release/MPlan"
        self.horizon = 3
        self.budget = 18

        self.branchF = 2
        self.actionF = 2

        self.goalP = 0.7
        self.seed = 123

        if not outFolder is None:
            self.outputFolder = outFolder
        else:
            self.outputFolder = os.getcwd() + f"/Data/Experiments/{domain}/{datetime.now().strftime("%Y-%m-%d %H:%M:%S")}"
        self.mdpFolder = f"{self.outputFolder}/mdps"
        self.rawOutFolder = f"{self.outputFolder}/raw"

        os.makedirs(self.outputFolder, exist_ok=True)
        os.makedirs(self.mdpFolder, exist_ok=True)
        os.makedirs(self.rawOutFolder, exist_ok=True)


    def makeMdpFileName(self, confName:str, confRep:int):
        return f"{self.mdpFolder}/{confName}_con{confRep}.json"

    def makePlanOutFileName(self, confName:str, confRep:int, envRep:int):
        return f"{self.rawOutFolder}/{confName}_con{confRep}_rep{envRep}.json"

    def getTheoryTags(self):
        # get all theory tags
        theoryTags = set()
        for con in self.configs:
            for i in range(1, len(con['theories']), 2):
                theoryTags.add(con['theories'][i])
        print("Theory tags")
        print(theoryTags)
        return list(theoryTags)

    def buildEnvironments(self, configRepetitions:int=1):
        for i in range(configRepetitions):
            for conf in self.configs:
                MDPFactory.buildEnvToFile(self.domain, fileOut=self.makeMdpFileName(conf['name'], i), **conf)
                

    def run(self, configRepetitions=1, envRepetitions=1):
        # Generate Environments.
        print("****\nBuiling environments...\n****")
        self.buildEnvironments(configRepetitions)
        print("****\nBuilt all environments.\n****")


        # Call planner on all envs
        for conf in self.configs:
            for conf_rep in range(configRepetitions):
                inFile = self.makeMdpFileName(conf['name'], conf_rep)
                for env_rep in range(envRepetitions):
                    print(f"Config Name: {conf["name"]}; Config Rep: {conf_rep}; Env Rep: {env_rep}")
                    outFile = self.makePlanOutFileName(conf['name'], conf_rep, env_rep)
                    result = subprocess.run([self.planner, inFile, outFile])
                    result.check_returncode()
            print(f"Finished env on {conf["name"]}, random config {conf_rep}.")
        print("Finished MPlan!")

        # Collect the Data.
        theoryTags = self.getTheoryTags()
        self.data = []
        for conf_rep in range(configRepetitions):
            for conf in self.configs:
                for env_rep in range(envRepetitions):
                    # open file
                    outFile = self.makePlanOutFileName(conf['name'], conf_rep, env_rep)
                    with open(outFile, 'r') as file:
                        json_data = json.load(file)
                        entry = {
                            "conf_rep": conf_rep,
                            "env_rep": env_rep,
                            "theories": str(conf['theories']),
                            "total_time": json_data['duration_Total'],
                            "plan_time": json_data['duration_Plan'],
                            "mehr_time": json_data['duration_MEHR'],
                            "sol_time": json_data['duration_Sols'],
                            "out_time": json_data['duration_Outs'],
                            "expanded_states": json_data['Expanded'],
                            "average_histories": json_data['average_histories'],
                            "max_histories": json_data['max_histories'],
                            "min_histories": json_data['min_histories'],
                            "total_states": json_data['total_states'],
                            "backups": json_data['Backups'],
                            "iterations": json_data['Iterations'],
                            "horizon": json_data['horizon'],
                            "min_non_accept": json_data["solutions"][0]["Non-Acceptability"],
                            "num_of_sols": len(json_data["solutions"])
                        }
                        
                        for tag in theoryTags:
                            if tag in json_data["solutions"][0]["Expectation"].keys():
                                entry[tag] = json_data["solutions"][0]["Expectation"][tag]
                            else: 
                                entry[tag] = "N/A"
                        self.data.append(entry)

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
        average_durations = df.groupby('horizon')[['total_time', 'plan_time', 'mehr_time', 'sol_time', 'out_time', 'num_of_sols']].mean().reset_index()
        average_durations.to_csv(self.getDurationsFilePath())
        # Save durations by theory csv
        theoryTimes = df.groupby("theories", sort=False)[["total_time", "plan_time", "sol_time", "out_time", "mehr_time", "expanded_states", "iterations", "backups", "min_non_accept", "num_of_sols"]].mean()
        theoryTimes.round(4)
        theoryTimes.to_csv(self.getDurationsByTheoryFilePath())

        # Check theory worth is the same across configs.
        cols = ["theories", "min_non_accept"]
        cols.extend(self.getTheoryTags())
        uniqueValues = df[cols].groupby('theories', sort=False).nunique()
        if (not (uniqueValues==1).all().all()):
            raise Exception("Sim: Different iterations returned different expected worth!")

        theoryResults = df[cols].groupby('theories', sort=False).first()
        theoryResults.to_csv(self.getTheoryExpectationsFilePath())


    def loadResults(self, of):
        self.outputFolder = of
        df = pd.read_csv(self.getAllDataFilePath())
        self.data = df.to_dict()


        
    def plotTimeResults(self, df=None):
        if (df==None):
            df = pd.DataFrame(self.data)
        average_durations = df.groupby('horizon')[['total_time', 'plan_time', 'mehr_time', 'sol_time', 'out_time']].mean().reset_index()
        average_durations['mehr_time'] = average_durations['mehr_time'].replace(0, 0.0001)

        plt.figure(figsize=(10, 6))
        for column in ['total_time', 'plan_time', 'mehr_time', 'sol_time', 'out_time']:
            plt.scatter(average_durations['horizon'], average_durations[column], label=column)

            plt.plot(average_durations['horizon'], average_durations[column], linestyle='-', alpha=0.6)
            # Add a line of best fit
            x = average_durations['horizon']
            y = average_durations[column]
            slope, intercept, r_value, _, _ = linregress(x, np.log10(y))  # Fit line in log scale
            best_fit_line = 10 ** (slope * x + intercept)  # Transform back to original scale
            plt.plot(x, best_fit_line, linestyle='--', alpha=0.8, label=f'{column} Best Fit (R={r_value:.2f})')

        plt.yscale('log')  # Set the y-axis to logarithmic scale
        plt.title("Time Metrics vs Horizon (Logarithmic Scale)")
        plt.xlabel("Horizon")
        plt.ylabel("Time (seconds, log scale)")
        plt.xticks(ticks=np.arange(min(average_durations['horizon']), max(average_durations['horizon']) + 1, 1))
        plt.legend()
        #plt.grid(True, which='both', linestyle='--', linewidth=0.5)   Adjust grid for log scale
        plt.savefig(self.outputFolder+ "timeVHorizon_LOG_SCALE.png")  # Save the plot

        plt.show()  # Show the plot

    def plotPercentTimeResults(self, df=None):
        if (df==None):
            df = pd.DataFrame(self.data)
        average_durations = df.groupby('horizon')[['total_time', 'plan_time', 'mehr_time', 'sol_time', 'out_time']].mean().reset_index()
        for column in ['plan_time', 'mehr_time', 'sol_time', 'out_time']:
            average_durations[f'{column}_percent'] = (average_durations[column] / average_durations['total_time']) * 100

        plt.figure(figsize=(10, 6))
        for column in ['plan_time_percent', 'mehr_time_percent', 'sol_time_percent', 'out_time_percent']:
            plt.plot(average_durations['horizon'], average_durations[column], marker='o', label=column.replace('_percent', ''))

        plt.title("Time Components as Percentage of Total Time vs Horizon")
        plt.xlabel("Horizon")
        plt.ylabel("Percentage of Total Time (%)")
        plt.xticks(ticks=np.arange(min(average_durations['horizon']), max(average_durations['horizon']) + 1, 1))

        plt.legend()
        plt.savefig(self.outputFolder + "time_components_percentage_vs_horizon.png")  # Save the plot
        plt.show()

            