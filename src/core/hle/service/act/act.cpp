// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/hle/kernel/handle_table.h"
#include "core/hle/kernel/shared_memory.h"
#include "core/hle/service/act/act.h"
#include "core/hle/service/act/act_a.h"
#include "core/hle/service/act/act_u.h"

namespace Service {
namespace ACT {

static Kernel::SharedPtr<Kernel::SharedMemory> shared_memory;

enum class BlkID : u32 {
    NNID = 0x8,          // <<< ASCII NUL-terminated Nintendo Network ID
    Birthday = 0xA,      // <<< Birthday
    CountryName = 0xB,   // <<< ASCII NUL-terminated Country Name
    PrincipalID = 0xC,   // <<< Principal ID
    AccountInfo = 0x11,  // <<< size=0xA0
    Gender = 0x13,       // <<< Gender, size=1
    UtcOffset = 0x19,    // <<< UTC Offset
    Commited = 0x1A,     // <<< IsCommited
    MiiName = 0x1B,      // <<< MiiName
    TimeZone = 0x1E,     // <<< ASCII Time Zone Location, size=0x41
    AccountToken = 0x21, // <<< Account Token
    Age = 0x2C,          // <<< Age
    CountryInfo = 0x2F,  // <<< CountryInfo
};

void Initialize(Interface* self) {
    u32* cmd_buff = Kernel::GetCommandBuffer();

    u32 version = cmd_buff[1];
    self->SetVersion(version);
    u32 size = cmd_buff[2];
    Kernel::Handle sh_mem = cmd_buff[6];

    if (Kernel::g_handle_table.IsValid(sh_mem)) {
        shared_memory = Kernel::g_handle_table.Get<Kernel::SharedMemory>(sh_mem);
        shared_memory->name = "ACT:shared_memory";
    }

    cmd_buff[1] = RESULT_SUCCESS.raw; // No error
    LOG_WARNING(Service_ACT, "(STUBBED) called, version=0x%X, sh_mem=0x%X, size=0x%X", version,
                sh_mem, size);
}

void GetErrorCode(Interface* self) {
    u32* cmd_buff = Kernel::GetCommandBuffer();

    cmd_buff[1] = RESULT_SUCCESS.raw; // No error
    cmd_buff[2] = RESULT_SUCCESS.raw; // No error
    LOG_WARNING(Service_ACT, "(STUBBED) called");
}

void GetAccountDataBlock(Interface* self) {
    u32* cmd_buff = Kernel::GetCommandBuffer();

    u8 unknown = static_cast<u8>(cmd_buff[1] & 0xFF);
    u32 size = cmd_buff[2];
    BlkID blk_id = static_cast<BlkID>(cmd_buff[3]);
    VAddr buf = cmd_buff[5];

    Memory::ZeroBlock(buf, size);
    switch (blk_id) {
    case BlkID::NNID:
        Memory::WriteBlock(buf, "mailwl", sizeof("mailwl"));
        break;
    }

    cmd_buff[1] = RESULT_SUCCESS.raw; // No error
    LOG_WARNING(Service_ACT, "(STUBBED) called, unknown=0x%X, size=0x%X, blk_id=0x%X, buf=0x%08X",
                unknown, size, blk_id, buf);
}

void Init() {
    AddService(new ACT_A);
    AddService(new ACT_U);

    shared_memory = nullptr;
}

void Shutdown() {
    shared_memory = nullptr;
}

} // namespace ACT
} // namespace Service
