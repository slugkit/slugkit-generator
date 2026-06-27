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
    parser = argparse.ArgumentParser(
        description="Compile one or more dictionary YAML files into binary .bin dictionaries. "
        "When several inputs are given they are merged per kind, so a large dictionary's source "
        "can be split across multiple files (e.g. geo.part1.yaml + geo.part2.yaml)."
    )
    parser.add_argument("input_files", nargs="+", help="Input YAML file(s), merged per kind")
    parser.add_argument(
        "-o", "--output", dest="output_file", required=True,
        help="Output base path; the binary is written as <base>.<kind>.bin",
    )
    return parser.parse_args()


def main():
    args = parse_args()
    dict_file = DictionaryFile.from_yaml(args.input_files[0])
    for extra in args.input_files[1:]:
        dict_file.merge(DictionaryFile.from_yaml(extra))
    for kind, dictionary_data in dict_file.dictionaries.items():
        start_time = datetime.datetime.now()
        n_words = sum(len(words) for words in dictionary_data.words.values())
        LOGGER.info(f"Compiling {kind} dictionary ({n_words} words from {len(args.input_files)} file(s))")
        binary_dictionary = BinaryDictionary(kind, dictionary_data)
        with open(args.output_file.replace(".yaml", f".{kind}.bin"), "wb") as f:
            binary_dictionary.write(f)
        end_time = datetime.datetime.now()
        LOGGER.info(f"Compiled {kind} dictionary in {end_time - start_time}")


if __name__ == "__main__":
    main()
