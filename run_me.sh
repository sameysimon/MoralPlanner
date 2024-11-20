#!/bin/bash

checkEnvSuccess() {
	if [ $? -eq 0 ]; then
		echo "Environment Built Successfully! -- $1"
	else
		echo "Environment Build Failed: $1"
		exit 1
	fi
}
checkPlannerSuccess() {
	if [ $? -eq 0 ]; then
		echo "Planner ran Successfully! -- $1"
		echo ""
		echo ""
		echo ""
	else
		echo "Planner Failed: $1"
		exit 1
	fi
}

echo "Building Environment..."
cd EnvironmentBuilder
python3 -m EnvironmentBuilder --domain LostInsulin --out ../Data/MDPs/HalCarlaEqual.json --theoryTags 0 CarlaLife 0 HalLife
checkEnvSuccess "HalCarlaEqual"
python3 -m EnvironmentBuilder --domain LostInsulin --out ../Data/MDPs/HalHigherCarla.json --theoryTags 0 CarlaLife 1 HalLife
checkEnvSuccess "HalHigherCarla"
python3 -m EnvironmentBuilder --domain LostInsulin --out ../Data/MDPs/HalCarlaHigher.json --theoryTags 1 CarlaLife 0 HalLife
checkEnvSuccess "HalCarlaHigher"

python3 -m EnvironmentBuilder --domain LostInsulin --out ../Data/MDPs/HalCarlaNoSteal.json --theoryTags 0 CarlaLife 0 HalLife 0 ToSteal
checkEnvSuccess "HalCarlaNoSteal"
python3 -m EnvironmentBuilder --domain LostInsulin --out ../Data/MDPs/HalCarlaNoStealHigher.json --theoryTags 0 CarlaLife 0 HalLife 1 ToSteal
checkEnvSuccess "HalCarlaNoStealHigher"
python3 -m EnvironmentBuilder --domain LostInsulin --out ../Data/MDPs/HalCarlaNoStealComp.json --theoryTags 0 CarlaLife 0 HalLife 0 ToStealWithComp
checkEnvSuccess "HalCarlaNoStealComp"

python3 -m EnvironmentBuilder --domain LostInsulin --out ../Data/MDPs/CostCarlaSteal.json --theoryTags 0 CarlaLife 0 Cost 0 ToSteal
checkEnvSuccess "CostCarlaSteal"
python3 -m EnvironmentBuilder --domain LostInsulin --out ../Data/MDPs/CostCarla.json --theoryTags 0 CarlaLife 0 Cost
checkEnvSuccess "CostCarla"

cd ..
echo "Built All Environments."

echo "Running MoralPlanner"
./MPlan/cmake-build-release/MPlan ./Data/MDPs/HalCarlaEqual.json ./Data/MPlan-out/HalCarlaEqual.json
checkPlannerSuccess "HalCarlaEqual"
./MPlan/cmake-build-release/MPlan ./Data/MDPs/HalHigherCarla.json ./Data/MPlan-out/HalHigherCarla.json
checkPlannerSuccess "HalHigherCarla"
./MPlan/cmake-build-release/MPlan ./Data/MDPs/HalCarlaHigher.json ./Data/MPlan-out/HalCarlaHigher.json
checkPlannerSuccess "HalCarlaHigher"

./MPlan/cmake-build-release/MPlan ./Data/MDPs/HalCarlaNoSteal.json ./Data/MPlan-out/HalCarlaNoSteal.json
checkPlannerSuccess "HalCarlaNoSteal"
./MPlan/cmake-build-release/MPlan ./Data/MDPs/HalCarlaNoStealHigher.json ./Data/MPlan-out/HalCarlaNoStealHigher.json
checkPlannerSuccess "HalCarlaNoStealHigher"
./MPlan/cmake-build-release/MPlan ./Data/MDPs/HalCarlaNoStealComp.json ./Data/MPlan-out/HalCarlaNoStealComp.json
checkPlannerSuccess "HalCarlaNoStealComp"



./MPlan/cmake-build-release/MPlan ./Data/MDPs/CostCarlaSteal.json ./Data/MPlan-out/CostCarlaSteal.json
checkPlannerSuccess "CostCarlaSteal"
./MPlan/cmake-build-release/MPlan ./Data/MDPs/CostCarla.json ./Data/MPlan-out/CostCarla.json
checkPlannerSuccess "CostCarla"

echo "Ran all environments!"





