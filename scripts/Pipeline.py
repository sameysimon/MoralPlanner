from scripts.AbstractExperiments import ExperimentRunner
from copy import deepcopy
import websocket
import json

config = {"Name": "Red=Blue", "Horizon": 8}
config.update(
    {
        "Theories": [["Red_Utility", "Utility", 0], ["Blue_Utility", "Utility", 0]], 
        "Considerations": [["blue:wellbeing", "Blue_Utility"], ["red:wellbeing", "Red_Utility"]]
    }
)


er = ExperimentRunner("SearchRescue", [config])
er.run(1,1)
outFile = er.makePlanOutFileName(config["Name"], 0, 0)
with open(outFile, "r") as f:
    data = json.load(f)

ws = websocket.WebSocket()
ws.connect("ws://localhost:8080/mdp")
ws.send(json.dumps(data))
ws.close()
