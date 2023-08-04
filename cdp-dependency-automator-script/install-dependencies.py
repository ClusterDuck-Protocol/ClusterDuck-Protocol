import os
import requests
import json
import zipfile

retry_count = 0
json_file = open('dependencies.json')
dependencies = json.load(json_file)

absolute_path = os.path.dirname(__file__)
destination = absolute_path
while os.path.basename(destination) != 'libraries':
    destination = os.path.dirname(destination)

if not os.path.exists(destination):
    os.makedirs(destination)

for dependency, info in dependencies.items():
    MAX_RETRIES = 5
    while retry_count < 5:
        try:
            response = requests.get(info["url"])

            if response.status_code == 200:
                content_type = response.headers.get('Content-Type')
                if content_type == 'application/zip':
                    file_path = os.path.join(destination, dependency)

                    with open(file_path, 'wb') as file:
                        file.write(response.content)

                    print(f"{dependency} successfully downloaded.")

                    print(f"Unzipping...")
                    with zipfile.ZipFile(file_path, 'r') as zip_ref:
                        zip_ref.extractall(destination)

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
