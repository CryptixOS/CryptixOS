import os
import subprocess

def process_directory(base_path, root):
    for dirpath, dirnames, filenames in os.walk(root):
        # Create directories first
        relative_path = os.path.relpath(dirpath, base_path)
        if relative_path != ".":
            subprocess.run(["echfs-utils", "-m", "-p2", "image.hdd", "mkdir", relative_path], check=True)
            print(f"Created directory: {relative_path}")
        
        # Import files
        for filename in filenames:
            file_relative_path = os.path.join(relative_path, filename)
            subprocess.run(["echfs-utils", "-m", "-p2", "image.hdd", "import", os.path.join(dirpath, filename), file_relative_path], check=True)
            print(f"Imported file: {file_relative_path}")

if __name__ == "__main__":
    base_dir = "../Kernel"
    if not os.path.exists(base_dir):
        print(f"Error: {base_dir} does not exist!")
    else:
        process_directory(base_dir, base_dir)
