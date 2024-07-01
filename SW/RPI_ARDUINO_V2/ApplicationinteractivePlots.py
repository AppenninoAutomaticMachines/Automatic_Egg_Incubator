import os
import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.widgets import CheckButtons
from datetime import datetime


def plot_all_data(folder_path):
    all_data = []

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
    concatenated_data.sort_values('Timestamp', inplace=True)

    # Plot the data interactively
    fig, ax = plt.subplots(figsize=(10, 6))
    lines = {}  # Dictionary to store the Line2D objects

    # Plot each column except 'Timestamp'
    for column in concatenated_data.columns:
        if column != 'Timestamp':
            lines[column], = ax.plot(concatenated_data['Timestamp'], concatenated_data[column], label=column)

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
def plot_current_day_data_interactive(folder_path):
    current_date = datetime.now().strftime('%Y-%m-%d')
    file_path = os.path.join(folder_path, f"{current_date}_temperatures.csv")

    if not os.path.exists(file_path):
        print(f"No data file found for the current day: {current_date}")
        return

    # Read the current day's data
    current_day_data = pd.read_csv(file_path, parse_dates=['Timestamp'])

    # Plot the data interactively
    fig, ax = plt.subplots(figsize=(10, 6))
    lines = {}  # Dictionary to store the Line2D objects

    # Plot each column except 'Timestamp'
    for column in current_day_data.columns:
        if column != 'Timestamp':
            lines[column], = ax.plot(current_day_data['Timestamp'], current_day_data[column], label=column)

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


# Example usage
plot_all_data('Machine_Statistics')
plot_current_day_data_interactive('Machine_Statistics')
