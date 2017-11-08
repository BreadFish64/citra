// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <cinttypes>
#include <cryptopp/sha.h>
#include "common/alignment.h"
#include "common/file_util.h"
#include "common/logging/log.h"
#include "core/file_sys/cia_container.h"
#include "core/file_sys/file_backend.h"
#include "core/loader/loader.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
// FileSys namespace

namespace FileSys {

constexpr u32 CIA_SECTION_ALIGNMENT = 0x40;

Loader::ResultStatus CIAContainer::Load(const FileBackend& backend) {
    std::vector<u8> header_data(sizeof(Header));

    // Load the CIA Header
    ResultVal<size_t> read_result = backend.Read(0, sizeof(Header), header_data.data());
    if (read_result.Failed() || *read_result != sizeof(Header))
        return Loader::ResultStatus::Error;

    Loader::ResultStatus result = LoadHeader(header_data);
    if (result != Loader::ResultStatus::Success)
        return result;

    // Load Title Metadata
    std::vector<u8> tmd_data(cia_header.tmd_size);
    read_result = backend.Read(GetTitleMetadataOffset(), cia_header.tmd_size, tmd_data.data());
    LOG_DEBUG(Service_FS, "TMD Offset:         0x%08" PRIx64 " bytes", GetTitleMetadataOffset());
    if (read_result.Failed() || *read_result != cia_header.tmd_size)
        return Loader::ResultStatus::Error;

    result = LoadTitleMetadata(tmd_data);
    if (result != Loader::ResultStatus::Success)
        return result;

    // Load CIA Metadata
    if (cia_header.meta_size) {
        std::vector<u8> meta_data(sizeof(Metadata));
        read_result = backend.Read(GetMetadataOffset(), sizeof(Metadata), meta_data.data());
        if (read_result.Failed() || *read_result != sizeof(Metadata))
            return Loader::ResultStatus::Error;

        result = LoadMetadata(meta_data);
        if (result != Loader::ResultStatus::Success)
            return result;
    }

    return Loader::ResultStatus::Success;
}

Loader::ResultStatus CIAContainer::Load(const std::string& filepath) {
    FileUtil::IOFile file(filepath, "rb");
    if (!file.IsOpen())
        return Loader::ResultStatus::Error;

    // Load CIA Header
    std::vector<u8> header_data(sizeof(Header));
    if (file.ReadBytes(header_data.data(), sizeof(Header)) != sizeof(Header))
        return Loader::ResultStatus::Error;

    Loader::ResultStatus result = LoadHeader(header_data);
    if (result != Loader::ResultStatus::Success)
        return result;

    // Load Title Metadata
    std::vector<u8> tmd_data(cia_header.tmd_size);
    file.Seek(GetTitleMetadataOffset(), SEEK_SET);
    LOG_DEBUG(Service_FS, "CIA HEADER TMD SIZE:               %u",
              static_cast<u32>(cia_header.tmd_size));
    LOG_DEBUG(Service_FS, "TMD Offset:         0x%08" PRIx64 " bytes", GetTitleMetadataOffset());
    if (file.ReadBytes(tmd_data.data(), cia_header.tmd_size) != cia_header.tmd_size)
        return Loader::ResultStatus::Error;

    result = LoadTitleMetadata(tmd_data);
    if (result != Loader::ResultStatus::Success)
        return result;

    // Load CIA Metadata
    if (cia_header.meta_size) {
        std::vector<u8> meta_data(sizeof(Metadata));
        file.Seek(GetMetadataOffset(), SEEK_SET);
        if (file.ReadBytes(meta_data.data(), sizeof(Metadata)) != sizeof(Metadata))
            return Loader::ResultStatus::Error;

        result = LoadMetadata(meta_data);
        if (result != Loader::ResultStatus::Success)
            return result;
    }

    return Loader::ResultStatus::Success;
}

Loader::ResultStatus CIAContainer::Load(const std::vector<u8>& file_data) {
    Loader::ResultStatus result = LoadHeader(file_data);
    if (result != Loader::ResultStatus::Success)
        return result;

    // Load Title Metadata
    result = LoadTitleMetadata(file_data, GetTitleMetadataOffset());
    if (result != Loader::ResultStatus::Success)
        return result;

    // Load CIA Metadata
    if (cia_header.meta_size) {
        result = LoadMetadata(file_data, GetMetadataOffset());
        if (result != Loader::ResultStatus::Success)
            return result;
    }

    return Loader::ResultStatus::Success;
}

Loader::ResultStatus CIAContainer::LoadHeader(const std::vector<u8>& header_data, size_t offset) {
    if (header_data.size() - offset < sizeof(Header))
        return Loader::ResultStatus::Error;

    memcpy(&cia_header, header_data.data(), sizeof(Header));

    return Loader::ResultStatus::Success;
}

Loader::ResultStatus CIAContainer::LoadTitleMetadata(const std::vector<u8>& tmd_data,
                                                     size_t offset) {
    return cia_tmd.Load(tmd_data, offset);
}

Loader::ResultStatus CIAContainer::LoadMetadata(const std::vector<u8>& meta_data, size_t offset) {
    if (meta_data.size() - offset < sizeof(Metadata))
        return Loader::ResultStatus::Error;

    memcpy(&cia_metadata, meta_data.data(), sizeof(Metadata));

    return Loader::ResultStatus::Success;
}

const TitleMetadata& CIAContainer::GetTitleMetadata() const {
    return cia_tmd;
}

std::array<u64, 0x30>& CIAContainer::GetDependencies() {
    return cia_metadata.dependencies;
}

u32 CIAContainer::GetCoreVersion() const {
    return cia_metadata.core_version;
}

u64 CIAContainer::GetCertificateOffset() const {
    return Common::AlignUp(cia_header.header_size, CIA_SECTION_ALIGNMENT);
}

u64 CIAContainer::GetTicketOffset() const {
    return Common::AlignUp(GetCertificateOffset() + cia_header.cert_size, CIA_SECTION_ALIGNMENT);
}

u64 CIAContainer::GetTitleMetadataOffset() const {
    return Common::AlignUp(GetTicketOffset() + cia_header.tik_size, CIA_SECTION_ALIGNMENT);
}

u64 CIAContainer::GetMetadataOffset() const {
    u64 offset =
        Common::AlignUp(GetTitleMetadataOffset() + cia_header.tmd_size, CIA_SECTION_ALIGNMENT);

    // Meta exists after all content in the CIA
    for (u16 i = 0; i < cia_tmd.GetContentCount(); i++) {
        offset += cia_tmd.GetContentSizeByIndex(i);
    }

    return offset;
}

u64 CIAContainer::GetContentOffset(u16 index) const {
    u64 offset =
        Common::AlignUp(GetTitleMetadataOffset() + cia_header.tmd_size, CIA_SECTION_ALIGNMENT);
    for (u16 i = 0; i < index; i++) {
        // The content_present is a bit array which defines which content in the TMD
        // is included in the CIA, so check the bit for this index and add if set.
        // The bits in the content index are arranged w/ index 0 as the MSB, 7 as the LSB, etc.
        if (cia_header.content_present[i >> 3] & 0x80 >> (i & 7))
            offset += cia_tmd.GetContentSizeByIndex(i);
    }
    return offset;
}

u32 CIAContainer::GetCertificateSize() const {
    return cia_header.cert_size;
}

u32 CIAContainer::GetTicketSize() const {
    return cia_header.tik_size;
}

u32 CIAContainer::GetTitleMetadataSize() const {
    return cia_header.tmd_size;
}

u32 CIAContainer::GetMetadataSize() const {
    return cia_header.meta_size;
}

u64 CIAContainer::GetContentSize(u16 index) const {
    // If the content doesn't exist in the CIA, it doesn't have a size.
    if (!(cia_header.content_present[index >> 3] & 0x80 >> (index & 7)))
        return 0;

    return cia_tmd.GetContentSizeByIndex(index);
}

void CIAContainer::Print() const {
    LOG_DEBUG(Service_FS, "Type:               %u", static_cast<u32>(cia_header.type));
    LOG_DEBUG(Service_FS, "Version:            %u\n", static_cast<u32>(cia_header.version));

    LOG_DEBUG(Service_FS, "Certificate Size: 0x%08x bytes", GetCertificateSize());
    LOG_DEBUG(Service_FS, "Ticket Size:      0x%08x bytes", GetTicketSize());
    LOG_DEBUG(Service_FS, "TMD Size:         0x%08x bytes", GetTitleMetadataSize());
    LOG_DEBUG(Service_FS, "Meta Size:        0x%08x bytes\n", GetMetadataSize());

    LOG_DEBUG(Service_FS, "Certificate Offset: 0x%08" PRIx64 " bytes", GetCertificateOffset());
    LOG_DEBUG(Service_FS, "Ticket Offset:      0x%08" PRIx64 " bytes", GetTicketOffset());
    LOG_DEBUG(Service_FS, "TMD Offset:         0x%08" PRIx64 " bytes", GetTitleMetadataOffset());
    LOG_DEBUG(Service_FS, "Meta Offset:        0x%08" PRIx64 " bytes", GetMetadataOffset());
    for (u16 i = 0; i < cia_tmd.GetContentCount(); i++) {
        LOG_DEBUG(Service_FS, "Content %x Offset:   0x%08" PRIx64 " bytes", i, GetContentOffset(i));
    }
}
} // namespace FileSys
