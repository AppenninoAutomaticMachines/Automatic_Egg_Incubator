import json
import os

def save_variables(filename, data):
    with open(filename, 'w') as f:
        json.dump(data, f, indent=4)

def load_variables(filename):
    if os.path.exists(filename):
        with open(filename, 'r') as f:
            return json.load(f)
    else:
        return None

# Usage example
data = {
    "tmp_higherHysteresisLimit": 37.8,
    "tmp_lowerHysteresisLimit": 37.4,
    "tmp_higherPercentageToActivateAuxiliaryHeater": 98,
    "tmp_lowerPercentageToActivateAuxiliaryHeater": 96,
    "stpr_secondsBtwEggsTurn": 3600,
    "stpr_defaultSpeed": 98
}

# Save variables
save_variables('variables.json', data)

# Load variables
loaded_data = load_variables('variables.json')
if loaded_data:
    tmp_higherHysteresisLimit = loaded_data["tmp_higherHysteresisLimit"]
    tmp_lowerHysteresisLimit = loaded_data["tmp_lowerHysteresisLimit"]
    tmp_higherPercentageToActivateAuxiliaryHeater = loaded_data["tmp_higherPercentageToActivateAuxiliaryHeater"]
    tmp_lowerPercentageToActivateAuxiliaryHeater = loaded_data["tmp_lowerPercentageToActivateAuxiliaryHeater"]
    stpr_secondsBtwEggsTurn = loaded_data["stpr_secondsBtwEggsTurn"]
    stpr_defaultSpeed = loaded_data["stpr_defaultSpeed"]
    print(tmp_higherHysteresisLimit, tmp_lowerHysteresisLimit) 
    print(tmp_higherPercentageToActivateAuxiliaryHeater, tmp_lowerPercentageToActivateAuxiliaryHeater)
    print(stpr_secondsBtwEggsTurn, stpr_defaultSpeed)
else:
    print("No data found")
