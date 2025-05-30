#  JSON Environment Information
```
{
    "total_states": 5, // Total integer number of states.
    "actions": ["A", "B", "C"], // All possible actions. Possibly obsolete.
    "state_transitions": [// Ordered list of dicts for each state transition
        {
            "A": [[0.5, 1, 1, 0, false], [0.5, 1, 2, 0, false]], // [Probability, Successor Idx, Theory_1 value, Theory_2 value ...]
            "B": [[1, 1, 1, 1, false]]
        },
        // Dict maps action to list of lists. 
        // Each list is an outcome:
        //      [probability, cost, destination Index, theory 1 judgement, theory 2 judgement]
        {
            "A": [[1, 1, 1, 0, false, [[dead_people], True, 1, 10] ]],//
            "B": [[1, 1, 1, 1, false]]
        }
    ],
    // Indicies of goal states 
    "goals":[2],
    // List of moral theories
    // Each has a type as follows: [Utility | Absolute | Threshold | Virtue]
    "theories": [
        {
            "Name": "Utilitarianism", // Arbitrary moral theory name.
            "Rank": 1, // Lexicographic Theory Ranking
            "Type": "Utility", // Type of moral theory
            "Heuristic":[0,0],// heuristic value for each state by index  
            "default": 0, // Default value of judgement, if not specified in transition.
            "label": "My Utilitarian theory"
        },
        {
            "Name": "Deontology",
            "Rank": 1,
            "Type": "Absolute",
            "Heuristic":[false,false]
            "default": false
        },
        {
            "Name": "Threshold",
            "Rank": 2,
            "Type": "Threshold",
            "Heuristic":[0,0]
            "default": 0,
            "threshold": 0.4 // Minimum acceptable probability
        },
        {
            "Name": "Principle of Double Effect",
            "Rank": 2,
            "Type": "PoDE",
            "Facts": ["group_died", "fatman_died", "person_died"],
            "Moral_Goals": [4], // List of states (by index) that are moral goals
            "Heuristic": [[[dead_people], False, 1, 10] ], [[dead_people], False, 1, 10] ], ... FOR EVERY STATE ... ]
        },
        {
            "Name": "Person_A", 
            "Type": "Maximin",
            "Component_Of: "Rawls", // Implicitly makes a Maximin theory called Rawls
            "Rank": 2,
            "Heuristic": [0,0,0],            
        },
        {
            "Name": "Person_B",
            "Type": "Maximin",
            "Component_Of: "Rawls"
            "Heuristic": [0,0,0],            
        },
        
    }
    ]





}


```
