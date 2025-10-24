#! /usr/bin/env uv run

import datetime
import argparse
import os
import sys
import logging

from slugkit.tools import DictionaryData, DictionaryFile
from slugkit.tools.binary_format import BinaryDictionary

logging.basicConfig(level=logging.DEBUG)
LOGGER = logging.getLogger(__name__)


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("input_file", type=str, help="Input file")
    parser.add_argument("output_file", type=str, help="Output file")
    return parser.parse_args()


def main():
    args = parse_args()
    dict_file = DictionaryFile.from_yaml(args.input_file)
    for kind, dictionary_data in dict_file.dictionaries.items():
        start_time = datetime.datetime.now()
        LOGGER.info(f"Compiling {kind} dictionary")
        binary_dictionary = BinaryDictionary(kind, dictionary_data)
        with open(args.output_file.replace(".yaml", f".{kind}.bin"), "wb") as f:
            binary_dictionary.write(f)
        end_time = datetime.datetime.now()
        LOGGER.info(f"Compiled {kind} dictionary in {end_time - start_time}")


if __name__ == "__main__":
    main()
