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
    "var1": "Hello",
    "var2": 42,
    "var3": [1, 2, 3]
}

# Save variables
save_variables('variables.json', data)

# Load variables
loaded_data = load_variables('variables.json')
if loaded_data:
    var1 = loaded_data["var1"]
    var2 = loaded_data["var2"]
    var3 = loaded_data["var3"]
    print(var1, var2, var3)
else:
    print("No data found")
