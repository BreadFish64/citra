// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "common/common_funcs.h"
#include "common/common_types.h"

namespace Service {

class Interface;

namespace NFC {

enum class OperationType : u8 {
    Unknown = 1,
    NFCTag = 2,
    RawNFC = 3,
};

enum class TagState : u8 {
    NotInitialized = 0,
    NotScanning = 1,
    Scanning = 2,
    TagInRange = 3,
    TagOutOfRange = 4,
    TagDataLoaded = 5,
};

enum class CommunicationStatus : u8 {
    AttemptInitialize = 1,
    NfcInitialized = 2,
};

struct Date {
    u16 year;
    u8 month;
    u8 day;
};

struct AmiiboSettings {
    u8 mii[0x60];
    u16 nick_name[11];
    u8 flags;
    u8 country;
    Date setup_date;
    INSERT_PADDING_BYTES(0x2C);
};
static_assert(sizeof(AmiiboSettings) == 0xA8, "AmiiboSettings size is wrong");

struct AppDataWriteStruct {
    u8 id[10];
    u8 id_size;
    INSERT_PADDING_BYTES(0x15);
};

struct TagInfo {
    AppDataWriteStruct tag_id;
    u8 protocol;
    u8 type;
    INSERT_PADDING_BYTES(0xA);
};
static_assert(sizeof(TagInfo) == 0x2C, "TagInfo size is wrong");

struct AmiiboConfig {
    Date last_write_date;
    u16 write_counter;
    u8 character_id[3];
    u8 series_id;
    u16 amiibo_id;
    u8 type;
    u8 version;
    u16 application_data_size = 0xD8;
    INSERT_PADDING_BYTES(0x30);
};
static_assert(sizeof(AmiiboConfig) == 0x40, "AmiiboConfig size is wrong");


/**
 * NFC::Initialize service function
 *  Inputs:
 *      0 : Header code [0x00010040]
 *      1 : (u8) OperationType
 *  Outputs:
 *      1 : Result of function, 0 on success, otherwise error code
 */
void Initialize(Interface* self);

/**
 * NFC::Shutdown service function
 *  Inputs:
 *      0 : Header code [0x00020040]
 *      1 : (u8) OperationType
 *  Outputs:
 *      1 : Result of function, 0 on success, otherwise error code
 */
void Shutdown(Interface* self);

/**
 * NFC::StartCommunication service function
 *  Inputs:
 *      0 : Header code [0x00030000]
 *  Outputs:
 *      1 : Result of function, 0 on success, otherwise error code
 */
void StartCommunication(Interface* self);

/**
 * NFC::StopCommunication service function
 *  Inputs:
 *      0 : Header code [0x00040000]
 *  Outputs:
 *      1 : Result of function, 0 on success, otherwise error code
 */
void StopCommunication(Interface* self);

/**
 * NFC::StartTagScanning service function
 *  Inputs:
 *      0 : Header code [0x00050040]
 *      1 : (u16) unknown. This is normally 0x0
 *  Outputs:
 *      1 : Result of function, 0 on success, otherwise error code
 */
void StartTagScanning(Interface* self);

/**
 * NFC::StopTagScanning service function
 *  Inputs:
 *      0 : Header code [0x00060000]
 *  Outputs:
 *      1 : Result of function, 0 on success, otherwise error code
 */
void StopTagScanning(Interface* self);

/**
 * NFC::LoadAmiiboData service function
 *  Inputs:
 *      0 : Header code [0x00070000]
 *  Outputs:
 *      1 : Result of function, 0 on success, otherwise error code
 */
void LoadAmiiboData(Interface* self);

/**
 * NFC::ResetTagScanState service function
 *  Inputs:
 *      0 : Header code [0x00080000]
 *  Outputs:
 *      1 : Result of function, 0 on success, otherwise error code
 */
void ResetTagScanState(Interface* self);

void UpdateStoredAmiiboData(Interface* self);

/**
 * NFC::GetTagInRangeEvent service function
 *  Inputs:
 *      0 : Header code [0x000B0000]
 *  Outputs:
 *      1 : Result of function, 0 on success, otherwise error code
 *      2 : Copy handle descriptor
 *      3 : Event Handle
 */
void GetTagInRangeEvent(Interface* self);

/**
 * NFC::GetTagOutOfRangeEvent service function
 *  Inputs:
 *      0 : Header code [0x000C0000]
 *  Outputs:
 *      1 : Result of function, 0 on success, otherwise error code
 *      2 : Copy handle descriptor
 *      3 : Event Handle
 */
void GetTagOutOfRangeEvent(Interface* self);

/**
 * NFC::GetTagState service function
 *  Inputs:
 *      0 : Header code [0x000D0000]
 *  Outputs:
 *      1 : Result of function, 0 on success, otherwise error code
 *      2 : (u8) Tag state
 */
void GetTagState(Interface* self);

/**
 * NFC::CommunicationGetStatus service function
 *  Inputs:
 *      0 : Header code [0x000F0000]
 *  Outputs:
 *      1 : Result of function, 0 on success, otherwise error code
 *      2 : (u8) Communication state
 */
void CommunicationGetStatus(Interface* self);


void GetTagInfo(Interface* self);

void OpenAppData(Interface* self);

void ReadAppData(Interface* self);

void WriteAppData(Interface* self);

void GetAmiiboSettings(Interface* self);

void GetAmiiboConfig(Interface* self);

void IsAvailableFontRegion(Interface* self);

/// Initialize all NFC services.
void Init();

/// Shutdown all NFC services.
void Shutdown();

} // namespace NFC
} // namespace Service
