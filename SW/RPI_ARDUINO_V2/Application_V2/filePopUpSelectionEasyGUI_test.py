import easygui
import os

def select_file():
    file_path = easygui.fileopenbox()
    
    if file_path:
        print(f"Selected file: {file_path}")
    else:
        print("No file selected")
    return file_path

if __name__ == "__main__":
    # Absolute path to app.py dynamically obtained
    path_to_app_py = os.path.abspath(__file__)
    print(path_to_app_py)
    
    # Get the common base directory
    base_dir = os.path.dirname(path_to_app_py)
    
    path_to_selected_file = select_file()
    
    # Get relative path from base directory to version.json
    relative_path = os.path.relpath(path_to_selected_file, start=base_dir)
    print("Relative path:", relative_path)

