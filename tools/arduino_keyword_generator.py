import re
import argparse

def extract_functions(filename, name_only):
    with open(filename, 'r') as file:
        data = file.read()

    # Matches C++ function declarations in the public section of a class
    pattern = re.compile(r'(public:)((.|\n)*?)(private:|protected:|public:|$)')
    public_section = re.search(pattern, data)

    if public_section:
        function_pattern = re.compile(r'(\w+\s+\w+\([^)]*\))')
        functions = re.findall(function_pattern, public_section.group(2))
        
        if name_only:
            # Extract only the function names
            name_pattern = re.compile(r'\w+(?=\()')
            functions = [re.search(name_pattern, function).group(0) for function in functions]
        
        return sorted(list(set(functions)))  # Remove duplicates and sort
    else:
        return []

parser = argparse.ArgumentParser(description='Extract public function names from a C++ header file.')
parser.add_argument('filename', type=str, help='The filename of the C++ header file.')
parser.add_argument('--name-only', action='store_true', help='Only print the function names, without parameters.')

args = parser.parse_args()
functions = extract_functions(args.filename, args.name_only)
for function in functions:
    if args.name_only:
        print(f"{function:<30}\tKEYWORD2")
    else:
        print(function)