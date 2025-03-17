import os
import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.widgets import CheckButtons
from datetime import datetime
import sys

''' 
	Tanto le statistiche sono sempre qui dentro:
	/Machine_Statistics
			/Temperatures
			/Humidity
			
	E in ogni subfolder memorizzo i vari dati.
'''

def plot_all_days(data_type):
    all_data = []

    machine_statistics_folder_path = "Machine_Statistics"
    # faccio selezione del folder da cui pescare i dati.
    if data_type == 'Temperatures':
        folder_path = os.path.join(machine_statistics_folder_path, 'Temperatures')
    elif data_type == 'Humidity':
        folder_path = os.path.join(machine_statistics_folder_path, 'Humidity')
    else:
        raise ValueError("Invalid plot configuration")	
	    
    print(folder_path)

    # Read all CSV files in the folder
    file_count = 0
    for file_name in os.listdir(folder_path):
        if file_name.endswith('.csv'):
            file_count += 1
            file_path = os.path.join(folder_path, file_name)
            data = pd.read_csv(file_path, parse_dates=['Timestamp'])
            all_data.append(data)

    if not all_data:
        print("No data files found in the folder.")
        return

    # Concatenate all data
    concatenated_data = pd.concat(all_data)
    concatenated_data.sort_values('Timestamp', inplace = True)

	# Filter valid data
    concatenated_data = filter_valid_data(concatenated_data)

    # Plot the data interactively
    fig, ax = plt.subplots(figsize=(10, 6))
    lines = {}  # Dictionary to store the Line2D objects

    # Plot each column except 'Timestamp'
    for column in concatenated_data.columns:
        if column != 'Timestamp':
            lines[column], = ax.plot(concatenated_data['Timestamp'], concatenated_data[column], label=column, linestyle='-', marker='o')

    ax.set_xlabel('Timestamp')
    ax.set_ylabel('Values')
    ax.set_title(f'Data from All Files ({file_count} days)')
    ax.grid(True)
    ax.legend()

    # Adjust layout
    fig.autofmt_xdate(rotation=45)
    fig.tight_layout()

    # Function to toggle visibility of traces
    def toggle_trace(label):
        lines[label].set_visible(not lines[label].get_visible())
        plt.draw()

    # Create CheckButtons for each trace
    ax_stack = plt.axes([0.85, 0.1, 0.1, 0.15], facecolor='lightgoldenrodyellow')  # Stack axes for CheckButtons
    visibility = [True] * len(lines)

    # Get labels for CheckButtons
    trace_labels = list(lines.keys())

    # Create CheckButtons and their event handlers
    check_buttons = CheckButtons(ax_stack, trace_labels, visibility)
    check_buttons.on_clicked(lambda label: toggle_trace(label))

    # Show the interactive plot
    plt.show()


# Function to plot current day's data interactively
def plot_current_day(data_type):
    current_date = datetime.now().strftime('%Y-%m-%d')

    machine_statistics_folder_path = "Machine_Statistics"
    # faccio selezione del folder da cui pescare i dati.
    if data_type == 'Temperatures':
        folder_path = os.path.join(machine_statistics_folder_path, 'Temperatures')
    elif data_type == 'Humidity':
        folder_path = os.path.join(machine_statistics_folder_path, 'Humidity')
    else:
        raise ValueError("Invalid plot configuration")	
	    
    # i files avranno lo stesso formato, tanto è già la cartella che li separa	
    file_path = os.path.join(folder_path, f"{current_date}.csv")

    if not os.path.exists(file_path):
        print(f"No data file found for the current day: {current_date}")
        return

    # Read the current day's data
    current_day_data = pd.read_csv(file_path, parse_dates=['Timestamp'])
	
	# Filter valid data
    current_day_data = filter_valid_data(current_day_data)

    # Plot the data interactively
    fig, ax = plt.subplots(figsize=(10, 6))
    lines = {}  # Dictionary to store the Line2D objects

    # Plot each column except 'Timestamp'
    for column in current_day_data.columns:
        if column != 'Timestamp':
            lines[column], = ax.plot(current_day_data['Timestamp'], current_day_data[column], label=column, linestyle='-', marker='o')

    ax.set_xlabel('Timestamp')
    ax.set_ylabel('Values')
    ax.set_title(f'Data for {current_date}')
    ax.grid(True)
    ax.legend()

    # Adjust layout
    fig.autofmt_xdate(rotation=45)
    fig.tight_layout()

    # Function to toggle visibility of traces
    def toggle_trace(label):
        lines[label].set_visible(not lines[label].get_visible())
        plt.draw()

    # Create CheckButtons for each trace
    ax_stack = plt.axes([0.85, 0.1, 0.1, 0.15], facecolor='lightgoldenrodyellow')  # Stack axes for CheckButtons
    visibility = [True] * len(lines)

    # Get labels for CheckButtons
    trace_labels = list(lines.keys())

    # Create CheckButtons and their event handlers
    check_buttons = CheckButtons(ax_stack, trace_labels, visibility)
    check_buttons.on_clicked(lambda label: toggle_trace(label))

    # Show the interactive plot
    plt.show()
	
def filter_valid_data(data):
    """Remove values outside the valid range if filtering is enabled."""
    if remove_erroneous_values_from_plot:
        for column in data.columns:
            if column != 'Timestamp':
                data = data[(data[column] >= VALID_RANGE[0]) & (data[column] <= VALID_RANGE[1])]
    return data


# Check the number of arguments passed
if len(sys.argv) < 2:
    print("Usage: python path_to_your_script.py arg1")
    sys.exit(1)

# Extract arguments
arg1 = sys.argv[1]
min_value = float(sys.argv[2])  # Convert argument to float
max_value = float(sys.argv[3])  # Convert argument to float
VALID_RANGE = (min_value, max_value)  # Define the range dynamically
remove_erroneous_values_from_plot = sys.argv[4]

print(f"Argument 1: {arg1}") # arg1 = quale tipo di statistica vogliamo eseguire.

# Example usage
try:
    if arg1 == 'PLOT_ALL_DAYS_DATA_TEMPERATURES':
        plot_all_days('Temperatures')
    elif arg1 == 'PLOT_CURRENT_DAY_DATA_TEMPERATURES':
        plot_current_day('Temperatures')
    elif arg1 == 'PLOT_ALL_DAYS_DATA_HUMIDITY':
        plot_all_days('Humidity')
    elif arg1 == 'PLOT_CURRENT_DAY_DATA_HUMIDITY':
        plot_current_day('Humidity')
    else:
        raise ValueError("Invalid arg1 value: {}".format(arg1))

except ValueError as ve:
    print("Error:", ve)
    # Handle the error appropriately, such as logging or providing feedback to the user
except Exception as e:
    print("An unexpected error occurred:", e)
    # Handle other unexpected errors
