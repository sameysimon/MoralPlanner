import subprocess
import time
import requests
import atexit
from scripts.AbstractExperiments import ExperimentRunner

domain = "Elder"
config = {"Name": "Test",
          "Budget": 6, 
          "Horizon": 8, 
          "Theories":[["M_Privacy", "Absolutism", 0], ["M_Autonomy", "Utility", 0], ["M_Health", "Utility", 0]],
            "Considerations":[["privacy", "M_Privacy"], ["autonomy", "M_Autonomy"], ["health", "M_Health"], ["cost"]]
        }


er = ExperimentRunner(domain, [config])
er.buildEnvironments(1)
mdpFile = er.makeMdpFileName("Test",0)
proc = subprocess.Popen([er.planner, "--server", "3"])

time.sleep(3) # give planner a chance to setup
# send the resuest
print(mdpFile)

resp = requests.post("http://localhost:18080/MDP", json={'file_in': mdpFile, 'from_data_folder':False})
if (resp.status_code!=200):
    print(f"Error: {resp.reason}")
resp.close()
atexit.register(proc.terminate)
input("Running MPlan as Server. Hit ENTER to terminate.")