import tkinter as tk
from tkinter import filedialog

def select_file():
    # Create a root window (it's necessary to have a Tkinter root window)
    root = tk.Tk()
    # Hide the root window
    root.withdraw()

    # Open a file dialog and store the selected file path
    file_path = filedialog.askopenfilename()

    # Check if a file was selected
    if file_path:
        print(f"Selected file: {file_path}")
    else:
        print("No file selected")

if __name__ == "__main__":
    select_file()
