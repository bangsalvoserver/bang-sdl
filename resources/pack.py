#!/usr/bin/env python3

import argparse
from fileinput import filename
from pathlib import Path
from sys import byteorder

class FileTableItem:
    def __init__(self, filename, name, begin, size) -> None:
        self.filename = filename
        self.name = name
        self.begin = begin
        self.size = size

def main():
    parser = argparse.ArgumentParser('Pack resources')
    parser.add_argument('-q','--quiet', action='store_true', help='Quiet')
    parser.add_argument('-D','--root-path', type=Path, help='Root Path', default=Path(__file__).parent)
    parser.add_argument('output_file', type=Path, help='Output File')
    parser.add_argument('input_files', nargs='*', type=Path, help='Input Files')
    args = parser.parse_args()

    for path in args.input_files:
        if not path.exists():
            print(f'Invalid input file: {path}')
            exit(1)

    out_bytes = bytearray()
    items = []
    for file in args.input_files:
        file = file.resolve()
        name = str(file.relative_to(args.root_path).with_suffix('').as_posix())

        if not args.quiet:
            print(name)

        if any(item.name == name for item in items):
            print(f'duplicate file key: {name}')
            exit(1)

        with open(file, 'rb') as f:
            file_bytes = bytearray(f.read())

        items.append(FileTableItem(file, name, len(out_bytes), len(file_bytes)))

        out_bytes += file_bytes
    
    with open(args.output_file, 'wb') as f:
        f.write(len(items).to_bytes(8, byteorder='little', signed=False))
        for item in items:
            item_name_encoded = item.name.encode()
            f.write(len(item_name_encoded).to_bytes(8, byteorder='little', signed=False))
            f.write(item_name_encoded)
            f.write(item.begin.to_bytes(8, byteorder='little', signed=False))
            f.write(item.size.to_bytes(8, byteorder='little', signed=False))
        f.write(out_bytes)

if __name__ == '__main__':
    main()