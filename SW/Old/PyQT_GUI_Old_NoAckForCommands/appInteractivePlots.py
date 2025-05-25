import os
import csv
import matplotlib.pyplot as plt
from matplotlib.widgets import CheckButtons
from datetime import datetime
import sys

def read_csv_file(file_path):
    timestamps = []
    data_columns = {}

    with open(file_path, 'r') as csvfile:
        reader = csv.DictReader(csvfile)
        for row in reader:
            try:
                timestamp = datetime.strptime(row['Timestamp'], '%Y-%m-%d %H:%M:%S')
                timestamps.append(timestamp)
                for key in row:
                    if key == 'Timestamp':
                        continue
                    if key not in data_columns:
                        data_columns[key] = []

                    value = row[key].strip()

                    # Convert 'True'/'False' to 1/0
                    if value.lower() == 'true':
                        data_columns[key].append(1.0)
                    elif value.lower() == 'false':
                        data_columns[key].append(0.0)
                    else:
                        data_columns[key].append(float(value))
            except Exception as e:
                print(f"Error parsing row {row}: {e}")

    return timestamps, data_columns

def filter_valid_data(timestamps, data_columns, remove_erroneous):
    if not remove_erroneous:
        return timestamps, data_columns

    filtered = {key: [] for key in data_columns}
    filtered_timestamps = []

    for i in range(len(timestamps)):
        keep = True
        for key in data_columns:
            value = data_columns[key][i]
            if not (VALID_RANGE[0] <= value <= VALID_RANGE[1]):
                keep = False
                break
        if keep:
            filtered_timestamps.append(timestamps[i])
            for key in data_columns:
                filtered[key].append(data_columns[key][i])

    return filtered_timestamps, filtered

def plot_data(timestamps, data_columns, title):
    fig, ax = plt.subplots(figsize=(10, 6))
    lines = {}

    for key in data_columns:
        lines[key], = ax.plot(timestamps, data_columns[key], label=key, linestyle='-', marker='o')

    ax.set_xlabel('Timestamp')
    ax.set_ylabel('Values')
    ax.set_title(title)
    ax.grid(True)
    ax.legend()

    fig.autofmt_xdate(rotation=45)
    fig.tight_layout()

    def toggle_trace(label):
        lines[label].set_visible(not lines[label].get_visible())
        plt.draw()

    ax_stack = plt.axes([0.85, 0.1, 0.1, 0.15], facecolor='lightgoldenrodyellow')
    visibility = [True] * len(lines)
    trace_labels = list(lines.keys())

    check_buttons = CheckButtons(ax_stack, trace_labels, visibility)
    check_buttons.on_clicked(lambda label: toggle_trace(label))

    plt.show()

def load_folder_data(folder_path):
    timestamps_all = []
    data_all = {}

    for file_name in os.listdir(folder_path):
        if file_name.endswith('.csv'):
            file_path = os.path.join(folder_path, file_name)
            timestamps, data_columns = read_csv_file(file_path)

            for key in data_columns:
                if key not in data_all:
                    data_all[key] = []
            timestamps_all.extend(timestamps)
            for key in data_columns:
                data_all[key].extend(data_columns[key])

    return timestamps_all, data_all

def plot_all_days(data_type, remove_erroneous_values):
    script_dir = os.path.dirname(os.path.abspath(__file__))
    machine_statistics_folder_path = os.path.join(script_dir, "Machine_Statistics") 
    folder = os.path.join(machine_statistics_folder_path, data_type)
    timestamps, data_columns = load_folder_data(folder)
    timestamps, data_columns = sort_by_timestamp(timestamps, data_columns)
    timestamps, data_columns = filter_valid_data(timestamps, data_columns, remove_erroneous_values)
    plot_data(timestamps, data_columns, f"All Days - {data_type}")

def plot_current_day(data_type, remove_erroneous_values):
    script_dir = os.path.dirname(os.path.abspath(__file__))
    machine_statistics_folder_path = os.path.join(script_dir, "Machine_Statistics") 
    folder = os.path.join(machine_statistics_folder_path, data_type)
    current_date = datetime.now().strftime('%Y-%m-%d')
    file_path = os.path.join(folder, f"{current_date}.csv")

    if not os.path.exists(file_path):
        print(f"No data file found for {current_date}")
        return

    timestamps, data_columns = read_csv_file(file_path)
    timestamps, data_columns = sort_by_timestamp(timestamps, data_columns)
    timestamps, data_columns = filter_valid_data(timestamps, data_columns, remove_erroneous_values)
    plot_data(timestamps, data_columns, f"Today - {data_type}")

def sort_by_timestamp(timestamps, data_columns):
    combined = list(zip(timestamps, *data_columns.values()))
    combined.sort()
    sorted_timestamps = [row[0] for row in combined]
    sorted_data = {
        key: [row[i + 1] for row in combined]
        for i, key in enumerate(data_columns.keys())
    }
    return sorted_timestamps, sorted_data

# Command-line argument handling
if len(sys.argv) < 5:
    print("Usage: python script.py <operation> <min> <max> <remove_erroneous>")
    sys.exit(1)

arg1 = sys.argv[1]
min_value = float(sys.argv[2])
max_value = float(sys.argv[3])
remove_errors = sys.argv[4].lower() == 'true'
VALID_RANGE = (min_value, max_value)

try:
    if arg1 == 'PLOT_ALL_DAYS_DATA_TEMPERATURES':
        plot_all_days('Temperatures', remove_errors)
    elif arg1 == 'PLOT_CURRENT_DAY_DATA_TEMPERATURES':
        plot_current_day('Temperatures', remove_errors)
    elif arg1 == 'PLOT_ALL_DAYS_DATA_HUMIDITY':
        plot_all_days('Humidity', remove_errors)
    elif arg1 == 'PLOT_CURRENT_DAY_DATA_HUMIDITY':
        plot_current_day('Humidity', remove_errors)
    elif arg1 == 'PLOT_ALL_DAYS_DATA_HEATER':
        plot_all_days('Heater', remove_errors)
    elif arg1 == 'PLOT_CURRENT_DAY_DATA_HEATER':
        plot_current_day('Heater', remove_errors)
    elif arg1 == 'PLOT_ALL_DAYS_DATA_HUMIDIFIER':
        plot_all_days('Humidifier', remove_errors)
    elif arg1 == 'PLOT_CURRENT_DAY_DATA_HUMIDIFIER':
        plot_current_day('Humidifier', remove_errors)
    else:
        raise ValueError("Invalid operation specified")
except Exception as e:
    print(f"Error: {e}")
