import subprocess
import sys
import pkg_resources
import os

def check_and_install_dependencies():
    """
    Checks requirements.txt and installs any missing packages.
    """
    base_dir = os.path.dirname(os.path.abspath(__file__))
    req_file = os.path.join(base_dir, "requirements.txt")
    
    if not os.path.exists(req_file):
        print(f"Warning: {req_file} not found. Skipping auto-install.")
        return

    with open(req_file, "r") as f:
        requirements = [line.strip() for line in f if line.strip() and not line.startswith("#")]

    missing = []
    for req in requirements:
        # Simple parsing for "package==version" or just "package"
        pkg_name = req.split("==")[0].split(">=")[0].split("<=")[0].strip()
        try:
            pkg_resources.require(req)
        except (pkg_resources.DistributionNotFound, pkg_resources.VersionConflict):
            missing.append(req)

    if missing:
        print(f"Missing dependencies found: {missing}")
        print("Installing now... (This may take a moment)")
        try:
            subprocess.check_call([sys.executable, "-m", "pip", "install", *missing])
            print("Successfully installed missing dependencies.")
        except subprocess.CalledProcessError as e:
            print(f"Error installing dependencies: {e}")
            sys.exit(1)
    else:
        print("All dependencies are satisfied locally.")

if __name__ == "__main__":
    check_and_install_dependencies()
