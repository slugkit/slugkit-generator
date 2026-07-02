# Binary Format for SlugKit Dictionary Files

> Format version: **2**. v2 stores the logical word index in lexicographic order (matching
> the in-memory dictionary's generation order) and replaces v1's contiguous length-sorted
> ranges with a per-length sparse index over lexicographic positions. See the `LengthIndex`,
> `LengthIndexTable` and `Word Data` sections below.

> Note: *START and SIZE are seem to be redundant when in a row, but we'll use the string_markup utility that is perfectly binary mapped to those two numbers*

| Section | Size |
|---|---|
| Header | 24 bytes + string data |
| Language Info Table | 8 bytes size + size * language info data |
| Tag Info Table | 8 bytes size + size * tag info data |
| Word Index | 8 bytes size + size * uint32_t |
| Word Data | variable, index size * word data |

All integers are written as little-endian

## Main structures

### Header

| Field | Size | Data type | Meaning |
|---|---|---|---|
| `MAGIC_NUM` | 8 | `array<char, 8>` | binary "SLUGDICT" |
| `VERSION` | 4 | `uint32_t` | binary format version |
| `KIND` | 4 | `StringMarkup` | kind of dictionary (noun, adjective, etc), offset and size for the field in string section |
| `VERSION` | 4 | `StringMarkup` | dictionary version, offset and size for the field in string section |
| `DESCRIPTION` | 4 | `StringMarkup` | dictionary description, offset and size for the field in string section |
| `STRING_DATA` | *variable* | char[] | strings for the kind, verstion and description fields, not zero-terminated |

### Language Info Table

> As language info can be of variable lenght, we cannot easily calculate the section size, so we write the size to the binary format.

| Field | Size | Data type | Meaning |
|---|---|---|---|
| `MAGIC_NUMBER` | 16 | `array<char, 16>` | binary "LANG-TABLE======" |
| `SIZE` | 4 | `SizeType` | size of the section in bytes |
| `COUNT` | 4 | `IndexType` | count of language info items |
| `OFFSET_TABLE` | 4 * `COUNT` | `OffsetType[]` | offsets to the language info items, base address from the beggining of language infos, item 0 offset = 0 |
| `LANGUAGES` | *variable* | `LanguageInfo[]` | table containing language info items  |



### Language Info

| Field | Size | Data type | Meaning |
|---|---|---|---|
| `CODE` | 4 | `array<char, 4>` | a zero-terminated language code string, allows for 2 and 3 char language codes. agnostic is encoded as "agn" |
| `LANGUAGE_RANGE` | 8 | `IndexRange` | the range of words for this language |
| `LENGHT_INDEX_TABLE` | *variable* | `LengthIndexTable` | word language indexes for the language |

### Tags Table

> As tag info can have variable length, the table has total size in bytes and offsetts into individual tag infos. Tags are sorted lexicograpthically

| Field | Size | Data type | Meaning |
|---|---|---|---|
| `MAGIC_NUM` | 16 | `array<char, 16>` | binary "TAGS-TABLE======" |
| `SIZE` | 4 | `SizeType` | size of tags table in bytes |
| `COUNT` | 4 | `IndexType` | count of tag info entries |
| `OFFSET_TABLE` | 4 * `COUNT` | `OffsetType[]` | offsets to the tag info items, base address from the beggining of language infos, item 0 offset = 0 |
| `TAGS` | *variable* | `TagInfo[]` | tags table |

### Tag Info

| Field | Size | Data type | Meaning |
|---|---|---|---|
| `MAGIC_NUM` | 8 | `array<char, 8>` | binary "TAG=====" |
| `TAG` | 4 | `StringMarkup` | tag, offset and size for the field in string section |
| `DESCRIPTION` | 4 | `StringMarkup` | description, offset and size for the field in string section |
| `OPT_IN` | 1 | `bool` | opt-in flag |
| `STRING_DATA` | *variable* | `char[]` | character data for tag and description |
| `PADDING` | *variable* | | pad to 4 byte boundary |
| `INDEX` | *variable* | `SparseIndex` | indexes of words having the tag |

### Word

A word contains precomputed lowercase, uppercase and title variants of the word. If a transfomation is equal to some other, e.g. lc == uc, uppercase offset/size may point to the same data.

A **verbatim** word (from a dictionary compiled with `case_mutation: false`) stores the original text in the lowercase slot and leaves `UC_SIZE` and `TITLE_SIZE` at `0`. Readers fall back to the lowercase slot for any requested case, so the word is emitted exactly as written (and counts as a single form in mixed case).

* `LC_START` (2 bytes, uint16_t) -> start of lower case word string, from word string data begin
* `LC_SIZE` (2 bytes, uint16_t) -> size of lower case word string
* `UC_START` (2 bytes, uint16_t) -> start of upper case word string, from word string data begin
* `UC_SIZE` (2 bytes, uint16_t) -> size of upper case word string
* `TITLE_START` (2 bytes, uint16_t) -> start of title case word string, from word string data begin
* `TITLE_SIZE` (2 bytes, uint16_t) -> size of title case word string
* `STRING_DATA` (variable) -> string data for the word

### Word Data

Words are stored in a contiguous memory region, grouped by language and sorted lexicographically. The logical word index (the Word Index offsets and every `SparseIndex` of word positions) is therefore in lexicographic order, so all selectors enumerate words in the same order as the in-memory dictionary. Access is via offset to the beginning of data. Language filtering uses the contiguous per-language `LANGUAGE_RANGE`; length filtering uses the per-length `LengthIndexTable`.

### Word Index

* `COUNT` (4 bytes, uint32_t) -> size of index, effectively number of words in the dictionary
* `OFFSETS` (variable) -> list of offsets of words, starting from the beginning of word list


## Auxilary Types

### `SizeType`

Numbers representing size in bytes

4 bytes, strong typeded to `std::uint32_t`

### `OffsetType`

Numbers representing offsets in memory

4 bytes, strong typeded to `std::uint32_t`

### `IndexType`

Numbers representing indexes into arrays and item counts

4 bytes, strong typeded to `std::uint32_t`


### `StringMarkup`

A structure containing a string's offset and size.

4 bytes, each field `std::uint16_t`

### `IndexRange`

A pair of indexes, first index and index after last

2 fields of `IndexType`, 8 bytes

### `SpatialIndex`

Array of non-contiguous indexes

| Field | Size | Data type | Meaning |
|---|---|---|---|
| `COUNT` | 4 | `IndexType` | number of items in the index |
| `INDEXES` | 4 * `sizeof(IndexType)` | `IndexType` | array of indexes. sorted by the index |

### `LengthIndex`

Because the logical word order is lexicographic, words of a given length are not contiguous,
so each length maps to a sparse index of the lexicographic positions of its words rather than
a range. The trailing `SparseIndex` makes the entry variable-size.

| Field | Size | Data type | Meaning |
|---|---|---|---|
| `LENGTH` | 4 | `SizeType` | the length of words in the index |
| `INDEX` | *variable* | `SparseIndex` | lexicographic positions of words of this length, ascending |

`size = sizeof(SizeType) + INDEX.size()`

### `LengthIndexTable`

The entries are sorted by word length ascending. Entries are variable-size (each carries a
`SparseIndex`), so they are stored back to back and walked by their own size; the total table
size is the sum of the entry sizes. A length query unions the `SparseIndex` of every entry
whose length satisfies the constraint, yielding a lexicographically ordered set.

| Field | Size | Data type | Meaning |
|---|---|---|---|
| `MAGIC_NUM` | 16 | `array<char, 16>` | binary "LENGTH-INDEX====" |
| `COUNT` | 4 | `IndexType` | number of indexes in the table |
| `INDEXES` | *variable* | `LengthIndex[]` | per-length sparse indexes, back to back |

