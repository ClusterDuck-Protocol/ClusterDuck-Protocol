#!/usr/bin/python3
import os
import requests
import json
import zipfile
import sys

def run():
    destination = os.path.dirname(__file__)
    while os.path.basename(destination) != 'libraries':
        destination = os.path.dirname(destination)

    if not os.path.exists(destination):
        os.makedirs(destination)

    print(f"installing basic CDP dependencies in {destination}")
    install('dependencies.json', destination)

    prompting = True
    while prompting:
        print(f"install optional dependencies for sensors, gps, etc? (Y)es/(N)o")
        choice = input().lower()
        if choice == '' or choice in ["yes", "ye", "y"]:
            install('optional-dependencies.json', destination)
            print("Installation complete.")
            prompting = False
        elif choice in ["no", "n"]:
            prompting = False
            print("Installation complete.")
        else:
            print("Please respond with 'yes' or 'no'")


def install(dep_file_name, destination):
    retry_count = 0
    json_file = open(dep_file_name)
    dependencies = json.load(json_file)

    for dependency, info in dependencies.items():
        MAX_RETRIES = 5
        while retry_count < 5:
            try:
                response = requests.get(info["url"])

                if response.status_code == 200:
                    content_type = response.headers.get('Content-Type')
                    if content_type == 'application/zip':
                        zip_dir = os.path.join(destination, dependency)
                        if not os.path.exists(zip_dir):
                            os.makedirs(zip_dir)

                        file_path = os.path.join(destination, f"{dependency}.zip")

                        with open(file_path, 'wb') as file:
                            file.write(response.content)

                        print(f"{dependency} v{info['version']} successfully downloaded.")

                        print(f"Unzipping...")
                        with zipfile.ZipFile(file_path, 'r') as zip_ref:
                            zip_ref.extractall(destination)

                        os.rmdir(zip_dir)
                        os.remove(file_path)

                    elif content_type == 'application/octet-stream':
                        file_path = os.path.join(destination, dependency)
                        if not os.path.exists(file_path):
                            os.makedirs(file_path)

                        with open(os.path.join(file_path, dependency) + ".h", 'wb') as file:
                            file.write(response.content)

                        print(f"{dependency} successfully downloaded.")

                    else:
                        print(f"{dependency} -- unsupported content type: {content_type}")

                break

            except requests.exceptions.RequestException as e:
                print(f"Error downloading {dependency}: {e}")
                retry_count += 1
                if retry_count == MAX_RETRIES:
                    print(f"Failed to download {dependency}.")
                    retry_count = 0
                    break

#enforce use of python 3
if sys.version_info[0] < 3:
   print(f"Script requires python 3 -- your version: {sys.version_info[0]}")
else:
    run()