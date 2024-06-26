import os
import json
from datetime import datetime

def load_json_file(file_path):
    with open(file_path, 'r') as f:
        return json.load(f)

def get_most_recent_file(folder_path):
    files = [f for f in os.listdir(folder_path) if f.endswith('.json')]
    if not files:
        return None
    files = [os.path.join(folder_path, f) for f in files]
    most_recent_file = max(files, key=os.path.getmtime)
    return most_recent_file

def load_configuration():
    default_config_folder = 'defaultConfiguration'
    current_config_folder = 'currentConfiguration'
    
    # Check if currentConfiguration folder is empty
    if not os.listdir(current_config_folder):
        print("currentConfiguration folder is empty. Loading from defaultConfiguration.")
        # Load a file from defaultConfiguration
        default_files = [f for f in os.listdir(default_config_folder) if f.endswith('.json')]
        if not default_files:
            raise FileNotFoundError("No JSON files found in defaultConfiguration folder.")
        # Load the first file (or you can implement logic to select a specific file)
        default_file_path = os.path.join(default_config_folder, default_files[0])
        return load_json_file(default_file_path)
    else:
        # Load the most recent JSON file from currentConfiguration
        print("Loading the most recent JSON file from currentConfiguration.")
        most_recent_file = get_most_recent_file(current_config_folder)
        if not most_recent_file:
            raise FileNotFoundError("No JSON files found in currentConfiguration folder.")
        return load_json_file(most_recent_file), most_recent_file

# Usage example
configuration, filename = load_configuration()
print(f"Loaded configuration from: {filename}")
print(configuration)
