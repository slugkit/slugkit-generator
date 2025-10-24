import io

import logging

from typing import BinaryIO
from pydantic import BaseModel, Field

from slugkit.tools import DictionaryData, TagData

SLGUKIT_MAGIC_NUMBER = b"SLUGDICT"
SLUGKIT_BINARY_FORMAT_VERSION = 1

CHAR_OFFSET_SIZE = 2
INDEX_TYPE_SIZE = 4

LANG_CODE_SIZE = 4

PAD_SYMBOL = b"*"

STRING_MARKUP_SIZE = 4  # 2 bytes for length, 2 bytes for offset
ZERO_BINARY = b"\x00\x00"

# Magic numbers should align to 8 bytes
INDEX_MAGIC_NUMBER = b"INDEX==="
WORDS_MAGIC_NUMBER = b"WORDS==="
LANGUAGE_TABLE_MAGIC_NUMBER = b"LANG-TABLE======"
LENGTH_INDEX_TABLE_MAGIC_NUMBER = b"LENGTH-INDEX===="
TAG_RECORD_MAGIC_NUMBER = b"TAG====="
TAGS_TABLE_MAGIC_NUMBER = b"TAGS-TABLE======"
TAG_INDEX_MAGIC_NUMBER = b"TAG-INDEX======="

LOGGER = logging.getLogger(__name__)


def _pad_count(size: int) -> int:
    return INDEX_TYPE_SIZE - (size % INDEX_TYPE_SIZE)
    if pad_count != INDEX_TYPE_SIZE:
        return pad_count
    return 0


class SparseIndex:
    index: list[int]

    def __init__(self):
        self.index = []

    def size(self) -> int:
        return len(self.index) * INDEX_TYPE_SIZE + INDEX_TYPE_SIZE

    def write(self, buffer: BinaryIO) -> bytes:
        # write the index count
        buffer.write(len(self.index).to_bytes(INDEX_TYPE_SIZE, "little"))
        # write the index values
        for index in self.index:
            buffer.write(index.to_bytes(INDEX_TYPE_SIZE, "little"))


class RangeIndex:
    def __init__(self, start_index: int, end_index: int):
        self.start_index = start_index
        self.end_index = end_index

    def size(self) -> int:
        return INDEX_TYPE_SIZE * 2

    def write(self, buffer: BinaryIO) -> bytes:
        buffer.write(self.start_index.to_bytes(INDEX_TYPE_SIZE, "little"))
        buffer.write(self.end_index.to_bytes(INDEX_TYPE_SIZE, "little"))


class LengthIndex:
    def __init__(self, length: int, range_index: RangeIndex):
        self.length = length
        self.range_index = range_index

    def size(self) -> int:
        return INDEX_TYPE_SIZE + self.range_index.size()

    def write(self, buffer: BinaryIO) -> bytes:
        buffer.write(self.length.to_bytes(INDEX_TYPE_SIZE, "little"))
        self.range_index.write(buffer)


class LengthIndexTable:
    length_indexes: list[LengthIndex]

    def __init__(self):
        self.length_indexes = []

    def size(self) -> int:
        return (
            len(LENGTH_INDEX_TABLE_MAGIC_NUMBER)
            + INDEX_TYPE_SIZE
            + len(self.length_indexes) * self.length_indexes[0].size()
        )

    def write(self, buffer: BinaryIO) -> bytes:
        # write the magic number
        buffer.write(LENGTH_INDEX_TABLE_MAGIC_NUMBER)
        # write the length index count
        buffer.write(len(self.length_indexes).to_bytes(INDEX_TYPE_SIZE, "little"))
        # write the length index values
        for length_index in self.length_indexes:
            length_index.write(buffer)


class Header:
    def __init__(self, kind: str, dictionary_data: DictionaryData):
        self.kind = kind.lower().encode("utf-8")
        self.version = dictionary_data.version.encode("utf-8")
        self.description = dictionary_data.description.encode("utf-8") if dictionary_data.description else b""
        self.dictionary_data = dictionary_data

    def size(self) -> int:
        return (
            len(SLGUKIT_MAGIC_NUMBER)
            + len(SLUGKIT_BINARY_FORMAT_VERSION.to_bytes(4, "little"))
            + len(self.kind)
            + len(self.version)
            + len(self.description)
            + STRING_MARKUP_SIZE * 3
        )

    def write(self, file: BinaryIO) -> None:
        # write the header to the file
        file.write(SLGUKIT_MAGIC_NUMBER)
        file.write(SLUGKIT_BINARY_FORMAT_VERSION.to_bytes(4, "little"))
        # write the kind offset and length
        file.write(ZERO_BINARY)
        file.write(len(self.kind).to_bytes(2, "little"))
        # write the version offset and length
        file.write(len(self.kind).to_bytes(2, "little"))
        file.write(len(self.version).to_bytes(2, "little"))
        # write the description offset and length
        file.write((len(self.kind) + len(self.version)).to_bytes(2, "little"))
        file.write(len(self.description).to_bytes(2, "little"))
        # write string data
        file.write(self.kind)
        file.write(self.version)
        file.write(self.description)
        # write the padding
        pad_count = _pad_count(self.size())
        file.write(PAD_SYMBOL * pad_count)

    def deserialize(self, data: bytes) -> DictionaryData:
        return DictionaryData.from_binary(data)


class LanguageInfo:
    def __init__(self, offset: int, language: str, start_index: int, end_index: int, length_table: LengthIndexTable):
        self.offset = offset
        # limit to 3 characters for the code
        self.code = language.lower()[: (LANG_CODE_SIZE - 1)].encode("utf-8")
        self.range_index = RangeIndex(start_index, end_index)
        self.length_table = length_table

    def size(self) -> int:
        return LANG_CODE_SIZE + self.range_index.size() + self.length_table.size()

    def write(self, buffer: BinaryIO) -> bytes:
        # write the code
        buffer.write(self.code)
        # zero terminate and fill the code to 4 characters
        buffer.write(b"\x00" * (LANG_CODE_SIZE - len(self.code)))
        # write the range
        self.range_index.write(buffer)
        # write the length table
        self.length_table.write(buffer)


class LanguageTable:
    def __init__(self):
        # TODO: calculate indexes and stuff
        self.language_infos = []

    def size(self) -> int:
        return (
            len(LANGUAGE_TABLE_MAGIC_NUMBER)
            + INDEX_TYPE_SIZE * 2
            + len(self.language_infos) * INDEX_TYPE_SIZE
            + sum(language_info.size() for language_info in self.language_infos)
        )

    def write(self, buffer: BinaryIO) -> bytes:
        # write the magic number
        buffer.write(LANGUAGE_TABLE_MAGIC_NUMBER)
        # write the language table size
        buffer.write(self.size().to_bytes(INDEX_TYPE_SIZE, "little"))
        # write the language info count
        buffer.write(len(self.language_infos).to_bytes(INDEX_TYPE_SIZE, "little"))
        # write the language info offsets
        for language_info in self.language_infos:
            buffer.write(language_info.offset.to_bytes(INDEX_TYPE_SIZE, "little"))
        # write the language info values
        for language_info in self.language_infos:
            language_info.write(buffer)


class TagEntry:
    def __init__(self, tag: str, description: str = "", opt_in: bool = False):
        self.tag = tag.lower().encode("utf-8")
        self.index = SparseIndex()
        self.description = description.encode("utf-8") if description else b""
        self.opt_in = opt_in

    def size(self) -> int:
        return (
            len(TAG_RECORD_MAGIC_NUMBER)
            + STRING_MARKUP_SIZE * 2
            + 1  # opt in flag
            + len(self.tag)
            + len(self.description)
            + self._padding()
            + self.index.size()
        )

    def _padding(self) -> int:
        return _pad_count(
            len(TAG_RECORD_MAGIC_NUMBER)
            + STRING_MARKUP_SIZE * 2
            + 1  # opt in flag
            + len(self.tag)
            + len(self.description)
        )

    def write(self, buffer: BinaryIO) -> None:
        buffer.write(TAG_RECORD_MAGIC_NUMBER)
        # write the tag offset and length
        buffer.write(ZERO_BINARY)
        buffer.write(len(self.tag).to_bytes(2, "little"))
        # write the tag description offset and length
        buffer.write(len(self.tag).to_bytes(2, "little"))
        buffer.write(len(self.description).to_bytes(2, "little"))
        # write the opt in
        buffer.write(self.opt_in.to_bytes(1, "little"))
        # write string data
        buffer.write(self.tag)
        buffer.write(self.description)
        # write the padding
        pad_count = self._padding()
        buffer.write(PAD_SYMBOL * pad_count)
        # write the index
        self.index.write(buffer)


class TagsTable:
    tags: dict[str, TagEntry]

    def __init__(self):
        self.tags = {}

    def add_tag_entry(self, tag: str, description: str = "", opt_in: bool = False):
        if tag not in self.tags:
            self.tags[tag] = TagEntry(tag, description, opt_in)
        return self.tags[tag]

    def add_index(self, tag: str, index: int):
        if tag not in self.tags:
            self.tags[tag] = TagEntry(tag)
        self.tags[tag].index.index.append(index)

    def size(self) -> int:
        return (
            len(TAGS_TABLE_MAGIC_NUMBER)
            + INDEX_TYPE_SIZE * 2
            + len(self.tags) * INDEX_TYPE_SIZE
            + sum(tag_entry.size() for tag_entry in self.tags.values())
        )

    def write(self, buffer: BinaryIO) -> None:
        # write the magic number
        buffer.write(TAGS_TABLE_MAGIC_NUMBER)
        # write the tags table size
        buffer.write(self.size().to_bytes(INDEX_TYPE_SIZE, "little"))
        # write the tag count
        buffer.write(len(self.tags).to_bytes(INDEX_TYPE_SIZE, "little"))
        # write the tag offsets
        offset = 0
        for tag, info in sorted(self.tags.items(), key=lambda x: x[0].lower()):
            buffer.write(offset.to_bytes(INDEX_TYPE_SIZE, "little"))
            offset += info.size()
        # write the tag entries
        for tag, info in sorted(self.tags.items(), key=lambda x: x[0].lower()):
            info.write(buffer)


class WordEntry:
    def __init__(self, word: str, offset: int):
        self.offset = offset
        self.lower = word.lower().encode("utf-8")
        self.upper = word.upper().encode("utf-8")
        self.title = word.title().encode("utf-8")

    def write(self, buffer: BinaryIO) -> bytes:
        # TODO optimize by writing only the needed data
        # write the lower offset and length
        buffer.write(ZERO_BINARY)
        buffer.write(len(self.lower).to_bytes(2, "little"))
        # write the upper offset and length
        buffer.write(len(self.lower).to_bytes(2, "little"))
        buffer.write(len(self.upper).to_bytes(2, "little"))
        # write the title offset and length
        buffer.write((len(self.lower) + len(self.upper)).to_bytes(2, "little"))
        buffer.write(len(self.title).to_bytes(2, "little"))
        # write string data
        buffer.write(self.lower)
        buffer.write(self.upper)
        buffer.write(self.title)

    def size(self) -> int:
        return len(self.lower) + len(self.upper) + len(self.title) + STRING_MARKUP_SIZE * 3


class WordsData:
    def __init__(self, dictionary_data: DictionaryData):
        self.dictionary_data = dictionary_data
        self.total_count = 0
        for language, words in dictionary_data.words.items():
            self.total_count += len(words)
        self.word_entries = []
        self.lang_table = LanguageTable()
        self.tags_table = TagsTable()
        self._build_word_entries()

    def _build_word_entries(self):
        offset = 0
        index = 0
        language_offset = 0
        for tag, tag_entry in self.dictionary_data.tags.items():
            self.tags_table.add_tag_entry(tag, tag_entry.description, tag_entry.opt_in)
        for language, words in self.dictionary_data.words.items():
            lang_start_index = index
            current_size = 0
            size_start_index = index
            length_index_table = LengthIndexTable()
            for word, tags in sorted(words.items(), key=lambda x: (len(x[0]), x[0].lower())):
                size = len(word)
                if current_size != size:
                    if current_size != 0:
                        # LOGGER.debug(f"Adding {language} size index for size {current_size} from {size_start_index} to {index}")
                        length_index_table.length_indexes.append(
                            LengthIndex(current_size, RangeIndex(size_start_index, index))
                        )
                    current_size = size
                    size_start_index = index
                for tag in tags:
                    self.tags_table.add_index(tag, index)
                entry = WordEntry(word, offset)
                self.word_entries.append(entry)
                index += 1
                offset += entry.size()

            # trailing length index
            length_index_table.length_indexes.append(LengthIndex(current_size, RangeIndex(size_start_index, index)))

            lang_info = LanguageInfo(language_offset, language, lang_start_index, index, length_index_table)
            self.lang_table.language_infos.append(lang_info)
            language_offset += lang_info.size()

    def write_index(self, buffer: BinaryIO) -> None:
        buffer.write(INDEX_MAGIC_NUMBER)
        buffer.write(self.total_count.to_bytes(INDEX_TYPE_SIZE, "little"))
        offset = 0
        for word_entry in self.word_entries:
            buffer.write(word_entry.offset.to_bytes(INDEX_TYPE_SIZE, "little"))

    def write_words(self, buffer: BinaryIO) -> None:
        buffer.write(WORDS_MAGIC_NUMBER)
        for word_entry in self.word_entries:
            word_entry.write(buffer)

    def write(self, buffer: BinaryIO) -> None:
        self.write_index(buffer)
        self.write_words(buffer)


class OutputOptions(BaseModel):
    language_table: bool = Field(default=True, description="Whether to include the language table")
    tags_table: bool = Field(default=True, description="Whether to include the tags table")


class BinaryDictionary:
    def __init__(self, kind: str, dictionary_data: DictionaryData):
        self.kind = kind.lower()
        self.dictionary_data = dictionary_data

    def write(self, buffer: BinaryIO, options: OutputOptions = OutputOptions()) -> None:
        Header(self.kind, self.dictionary_data).write(buffer)
        words_data = WordsData(self.dictionary_data)
        words_data.write_index(buffer)
        if options.language_table:
            LOGGER.debug("Writing language table")
            words_data.lang_table.write(buffer)
        if options.tags_table:
            LOGGER.debug("Writing tags table")
            words_data.tags_table.write(buffer)
        words_data.write_words(buffer)
