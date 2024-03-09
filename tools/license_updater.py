import os
import glob
import re

# Define the comment formats
cpp_comment = """/* SPDX-FileCopyrightText: 2018-2024 The ClusterDuck Protocol Authors */
/* SPDX-License-Identifier: Apache-2.0 */


"""
ino_comment = """// SPDX-FileCopyrightText: 2018-2024 The ClusterDuck Protocol Authors
// SPDX-License-Identifier: Apache-2.0


"""
ini_comment = """; SPDX-FileCopyrightText: 2018-2024 The ClusterDuck Protocol Authors
; SPDX-License-Identifier: Apache-2.0


"""
py_comment = """# SPDX-FileCopyrightText: 2018-2024 The ClusterDuck Protocol Authors
# SPDX-License-Identifier: Apache-2.0


"""

# Define the directories to traverse
directories = ["src", "examples", "test"]

# Define the file extensions and their corresponding comment formats
file_extensions = {
    ".h": cpp_comment,
    ".cpp": cpp_comment,
    ".c": cpp_comment,
    ".ino": ino_comment,
    ".ini": ini_comment,
    ".py": py_comment,
}

# SPDX line pattern
spdx_pattern = re.compile(r"SPDX-FileCopyrightText:.*|SPDX-License-Identifier:.*")

# Traverse the directories
update_count = 0
for directory in directories:
    for extension, comment in file_extensions.items():
        for filename in glob.iglob(f"{directory}/**/*{extension}", recursive=True):
            with open(filename, 'r+') as file:
                content = file.read()
                # Remove all SPDX lines
                content = re.sub(spdx_pattern, '', content)
                file.seek(0, 0)
                file.write(comment + content)
                file.truncate()
            update_count += 1

print(f"Updated {update_count} files")