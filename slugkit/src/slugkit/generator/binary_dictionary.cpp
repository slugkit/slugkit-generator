#include <slugkit/generator/binary_dictionary.hpp>

namespace slugkit::generator::binary {

namespace {
auto CheckPointer(const std::byte* base, std::string_view name, std::int64_t alignment = 4) -> void {
    if (base == nullptr) {
        throw std::runtime_error(fmt::format("Invalid {}: base is nullptr", name));
    }
    if (reinterpret_cast<std::int64_t>(base) % alignment != 0) {
        throw std::runtime_error(fmt::format("Invalid {}: base is not aligned to {}", name, alignment));
    }
}
}  // namespace

//-----------------------------------------------------------------------------
// SparseIndex
//-----------------------------------------------------------------------------

auto SparseIndex::GetSparseIndex(RawData data) -> const SparseIndex* {
    if (data.size() < sizeof(SparseIndex)) {
        throw std::runtime_error(fmt::format(
            "Invalid data: size {} is less than sparse index expected size {}", data.size(), sizeof(SparseIndex)
        ));
    }
    const auto* sparse_index = GetSparseIndex(data.data());
    if (data.size() < sparse_index->Size().GetUnderlying()) {
        throw std::runtime_error(fmt::format(
            "Invalid data: size {} is less than sparse index expected size {}", data.size(), sparse_index->Size()
        ));
    }
    return sparse_index;
}

auto SparseIndex::GetSparseIndex(const std::byte* base) -> const SparseIndex* {
    CheckPointer(base, "sparse index");
    const auto* sparse_index = reinterpret_cast<const SparseIndex*>(base);
    return sparse_index;
}

//-----------------------------------------------------------------------------
// Header
//-----------------------------------------------------------------------------
auto Header::GetHeader(RawData data) -> const Header* {
    if (data.size() < sizeof(Header)) {
        throw std::runtime_error(fmt::format("Invalid data: size {} is less than {}", data.size(), sizeof(Header)));
    }
    const auto* header = GetHeader(data.data());
    if (header->Size().GetUnderlying() > data.size()) {
        throw std::runtime_error(
            fmt::format("Invalid data: size {} is less than header expected size {}", data.size(), header->Size())
        );
    }
    return header;
}

auto Header::GetHeader(const std::byte* base) -> const Header* {
    CheckPointer(base, "header");
    const auto* header = reinterpret_cast<const Header*>(base);
    if (!header->IsValid()) {
        throw std::runtime_error(fmt::format("Invalid header: magic num {} is not {}", header->MagicNum(), kMagicNum));
    }
    return header;
}

//-----------------------------------------------------------------------------
// IndexTable
//-----------------------------------------------------------------------------

auto IndexTable::GetIndexTable(RawData data) -> const IndexTable* {
    if (data.size() < sizeof(IndexTable)) {
        throw std::runtime_error(fmt::format(
            "Invalid data: size {} is less than index table expected size {}", data.size(), sizeof(IndexTable)
        ));
    }
    const auto* index_table = GetIndexTable(data.data());
    if (data.size() < index_table->Size().GetUnderlying() + sizeof(OffsetType)) {
        throw std::runtime_error(fmt::format(
            "Invalid data: size {} is less than index table expected size {}",
            data.size(),
            index_table->Size().GetUnderlying() + sizeof(OffsetType)
        ));
    }
    return index_table;
}

auto IndexTable::GetIndexTable(const std::byte* base) -> const IndexTable* {
    CheckPointer(base, "index table");
    const auto* index_table = reinterpret_cast<const IndexTable*>(base);
    if (!index_table->IsValid()) {
        throw std::runtime_error(
            fmt::format("Invalid index table: magic num {} is not {}", index_table->MagicNum(), kMagicNum)
        );
    }
    return index_table;
}

//-----------------------------------------------------------------------------
// LanguageInfo
//-----------------------------------------------------------------------------

auto LanguageInfo::GetLanguageInfo(RawData data) -> const LanguageInfo* {
    if (data.size() < sizeof(LanguageInfo)) {
        throw std::runtime_error(fmt::format(
            "Invalid data: size {} is less than language info expected size {}", data.size(), sizeof(LanguageInfo)
        ));
    }
    const auto* language_info = GetLanguageInfo(data.data());
    if (data.size() < language_info->Size().GetUnderlying() + sizeof(OffsetType)) {
        throw std::runtime_error(fmt::format(
            "Invalid data: size {} is less than language info expected size {}",
            data.size(),
            language_info->Size().GetUnderlying() + sizeof(OffsetType)
        ));
    }
    return language_info;
}

auto LanguageInfo::GetLanguageInfo(const std::byte* base) -> const LanguageInfo* {
    CheckPointer(base, "language info");
    const auto* language_info = reinterpret_cast<const LanguageInfo*>(base);
    if (!language_info->IsValid()) {
        throw std::runtime_error(fmt::format(
            "Invalid language info: magic num {} is not {}",
            language_info->length_index_table_.MagicNum(),
            LengthIndexTable::kMagicNum
        ));
    }
    return language_info;
}

//-----------------------------------------------------------------------------
// LanguageTable
//-----------------------------------------------------------------------------

auto LanguageTable::GetLanguageTable(RawData data) -> const LanguageTable* {
    if (data.size() < sizeof(LanguageTable)) {
        throw std::runtime_error(fmt::format(
            "Invalid data: size {} is less than language table expected size {}", data.size(), sizeof(LanguageTable)
        ));
    }
    const auto* language_table = GetLanguageTable(data.data());
    if (data.size() < language_table->Size().GetUnderlying() + sizeof(OffsetType)) {
        throw std::runtime_error(fmt::format(
            "Invalid data: size {} is less than language table expected size {}",
            data.size(),
            language_table->Size().GetUnderlying() + sizeof(OffsetType)
        ));
    }
    return language_table;
}

auto LanguageTable::GetLanguageTable(const std::byte* base) -> const LanguageTable* {
    CheckPointer(base, "language table");
    const auto* language_table = reinterpret_cast<const LanguageTable*>(base);
    if (!language_table->IsValid()) {
        throw std::runtime_error(
            fmt::format("Invalid language table: magic num {} is not {}", language_table->MagicNum(), kMagicNum)
        );
    }
    return language_table;
}

//-----------------------------------------------------------------------------
// TagEntry
//-----------------------------------------------------------------------------
auto TagEntry::GetTagEntry(RawData data) -> const TagEntry* {
    if (data.size() < sizeof(TagEntry)) {
        throw std::runtime_error(
            fmt::format("Invalid data: size {} is less than tag entry expected size {}", data.size(), sizeof(TagEntry))
        );
    }
    return GetTagEntry(data.data());
}

auto TagEntry::GetTagEntry(const std::byte* base) -> const TagEntry* {
    CheckPointer(base, "tag entry");
    const auto* tag_entry = reinterpret_cast<const TagEntry*>(base);
    if (!tag_entry->IsValid()) {
        throw std::runtime_error(
            fmt::format("Invalid tag entry: magic num {} is not {}", tag_entry->MagicNum(), kMagicNum)
        );
    }
    return tag_entry;
}

//-----------------------------------------------------------------------------
// TagsTable
//-----------------------------------------------------------------------------
auto TagsTable::GetTagsTable(RawData data) -> const TagsTable* {
    if (data.size() < sizeof(TagsTable)) {
        throw std::runtime_error(fmt::format(
            "Invalid data: size {} is less than tags table expected size {}", data.size(), sizeof(TagsTable)
        ));
    }
    const auto* tags_table = GetTagsTable(data.data());

    return tags_table;
}

auto TagsTable::GetTagsTable(const std::byte* base) -> const TagsTable* {
    CheckPointer(base, "tags table");
    const auto* tags_table = reinterpret_cast<const TagsTable*>(base);
    if (!tags_table->IsValid()) {
        throw std::runtime_error(
            fmt::format("Invalid tags table: magic num {} is not {}", tags_table->MagicNum(), kMagicNum)
        );
    }
    return tags_table;
}

//-----------------------------------------------------------------------------
// WordData
//-----------------------------------------------------------------------------

auto WordData::GetWordData(RawData data) -> const WordData* {
    if (data.size() < sizeof(WordData)) {
        throw std::runtime_error(
            fmt::format("Invalid data: size {} is less than word data expected size {}", data.size(), sizeof(WordData))
        );
    }
    return GetWordData(data.data());
}

auto WordData::GetWordData(const std::byte* base) -> const WordData* {
    CheckPointer(base, "word data");
    const auto* word_data = reinterpret_cast<const WordData*>(base);
    if (!word_data->IsValid()) {
        throw std::runtime_error(
            fmt::format("Invalid word data: magic num {} is not {}", word_data->MagicNum(), kMagicNum)
        );
    }
    return word_data;
}

}  // namespace slugkit::generator::binary
