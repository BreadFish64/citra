// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/hle/ipc.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/kernel/event.h"
#include "core/hle/kernel/handle_table.h"
#include "core/hle/service/nfc/nfc.h"
#include "core/hle/service/nfc/nfc_m.h"
#include "core/hle/service/nfc/nfc_u.h"
#include "core/memory.h"

namespace Service {
namespace NFC {

static Kernel::SharedPtr<Kernel::Event> tag_in_range_event;
static Kernel::SharedPtr<Kernel::Event> tag_out_of_range_event;
static CommunicationStatus nfc_status = CommunicationStatus::AttemptInitialize;
static TagState nfc_tag_state = TagState::NotInitialized;
static AmiiboSettings amiibo_settings{};
static TagInfo tag_info{};
static AmiiboConfig amiibo_config{};

void Initialize(Interface* self) {
    IPC::RequestParser rp(Kernel::GetCommandBuffer(), 0x01, 1, 0);
    OperationType operation_type = static_cast<OperationType>(rp.Pop<u8>());
    LOG_WARNING(Service_NFC, "(STUBBED) called, operation_type=%u", operation_type);

    nfc_tag_state = TagState::NotScanning;
    nfc_status = CommunicationStatus::NfcInitialized;

    IPC::RequestBuilder rb = rp.MakeBuilder(1, 0);
    rb.Push(RESULT_SUCCESS);
}

void Shutdown(Interface* self) {
    IPC::RequestParser rp(Kernel::GetCommandBuffer(), 0x02, 1, 0);
    OperationType operation_type = static_cast<OperationType>(rp.Pop<u8>());
    LOG_WARNING(Service_NFC, "(STUBBED) called, operation_type=%u", operation_type);

    nfc_status = CommunicationStatus::AttemptInitialize;
    nfc_tag_state = TagState::NotInitialized;

    IPC::RequestBuilder rb = rp.MakeBuilder(1, 0);
    rb.Push(RESULT_SUCCESS);
}

void StartCommunication(Interface* self) {
    IPC::RequestParser rp(Kernel::GetCommandBuffer(), 0x03, 0, 0);
    LOG_WARNING(Service_NFC, "(STUBBED) called");
    IPC::RequestBuilder rb = rp.MakeBuilder(1, 0);
    rb.Push(RESULT_SUCCESS);
}

void StopCommunication(Interface* self) {
    IPC::RequestParser rp(Kernel::GetCommandBuffer(), 0x04, 0, 0);
    LOG_WARNING(Service_NFC, "(STUBBED) called");
    IPC::RequestBuilder rb = rp.MakeBuilder(1, 0);
    rb.Push(RESULT_SUCCESS);
}

void StartTagScanning(Interface* self) {
    IPC::RequestParser rp(Kernel::GetCommandBuffer(), 0x05, 1, 0);
    u16 param = rp.Pop<u16>();

    nfc_tag_state = TagState::TagInRange;
    tag_in_range_event->Signal();

    LOG_WARNING(Service_NFC, "(STUBBED) called, param=%u", param);
    IPC::RequestBuilder rb = rp.MakeBuilder(1, 0);
    rb.Push(RESULT_SUCCESS);
}

void StopTagScanning(Interface* self) {
    IPC::RequestParser rp(Kernel::GetCommandBuffer(), 0x06, 0, 0);

    nfc_tag_state = TagState::NotScanning;

    LOG_WARNING(Service_NFC, "(STUBBED) called");
    IPC::RequestBuilder rb = rp.MakeBuilder(1, 0);
    rb.Push(RESULT_SUCCESS);
}

void LoadAmiiboData(Interface* self) {
    IPC::RequestParser rp(Kernel::GetCommandBuffer(), 0x07, 0, 0);

    nfc_tag_state = TagState::TagDataLoaded;

    LOG_WARNING(Service_NFC, "(STUBBED) called");
    IPC::RequestBuilder rb = rp.MakeBuilder(1, 0);
    rb.Push(RESULT_SUCCESS);
}

void ResetTagScanState(Interface* self) {
    IPC::RequestParser rp(Kernel::GetCommandBuffer(), 0x08, 0, 0);

    nfc_tag_state = TagState::NotScanning;

    LOG_WARNING(Service_NFC, "(STUBBED) called");
    IPC::RequestBuilder rb = rp.MakeBuilder(1, 0);
    rb.Push(RESULT_SUCCESS);
}

void UpdateStoredAmiiboData(Interface* self) {
    IPC::RequestParser rp(Kernel::GetCommandBuffer(), 0x09, 0, 2);

    const Kernel::Handle pid = rp.PopHandle();

    LOG_WARNING(Service_NFC, "(STUBBED) called, pid=0x%x", pid);
    IPC::RequestBuilder rb = rp.MakeBuilder(1, 0);
    rb.Push(RESULT_SUCCESS);
}

void GetTagInRangeEvent(Interface* self) {
    IPC::RequestParser rp(Kernel::GetCommandBuffer(), 0x0B, 0, 0);
    LOG_WARNING(Service_NFC, "(STUBBED) called");
    IPC::RequestBuilder rb = rp.MakeBuilder(1, 2);
    rb.Push(RESULT_SUCCESS);
    rb.PushCopyHandles(Kernel::g_handle_table.Create(tag_in_range_event).Unwrap());
}

void GetTagOutOfRangeEvent(Interface* self) {
    IPC::RequestParser rp(Kernel::GetCommandBuffer(), 0x0C, 0, 0);
    LOG_WARNING(Service_NFC, "(STUBBED) called");
    IPC::RequestBuilder rb = rp.MakeBuilder(1, 2);
    rb.Push(RESULT_SUCCESS);
    rb.PushCopyHandles(Kernel::g_handle_table.Create(tag_out_of_range_event).Unwrap());
}

void GetTagState(Interface* self) {
    IPC::RequestParser rp(Kernel::GetCommandBuffer(), 0x0D, 0, 0);
    LOG_DEBUG(Service_NFC, "(STUBBED) called");
    IPC::RequestBuilder rb = rp.MakeBuilder(2, 0);
    rb.Push(RESULT_SUCCESS);
    rb.Push(static_cast<u8>(nfc_tag_state));
}

void CommunicationGetStatus(Interface* self) {
    IPC::RequestParser rp(Kernel::GetCommandBuffer(), 0x0F, 0, 0);
    LOG_WARNING(Service_NFC, "(STUBBED) called");
    IPC::RequestBuilder rb = rp.MakeBuilder(2, 0);
    rb.Push(RESULT_SUCCESS);
    rb.Push(static_cast<u8>(nfc_status));
}

void GetTagInfo(Interface* self) {
    IPC::RequestParser rp(Kernel::GetCommandBuffer(), 0x11, 0, 0);
    LOG_WARNING(Service_NFC, "(STUBBED) called");
    IPC::RequestBuilder rb = rp.MakeBuilder(1 + sizeof(TagInfo) / sizeof(u32), 0);
    rb.Push(RESULT_SUCCESS);
    rb.PushRaw(tag_info);
}

void OpenAppData(Interface* self) {
    IPC::RequestParser rp(Kernel::GetCommandBuffer(), 0x13, 1, 0);
    u32 amiibo_appid = rp.Pop<u32>();
    LOG_WARNING(Service_NFC, "(STUBBED) called, amiibo_appid=0x%x", amiibo_appid);
    IPC::RequestBuilder rb = rp.MakeBuilder(1, 0);
    rb.Push(RESULT_SUCCESS);
}

void ReadAppData(Interface* self) {
    IPC::RequestParser rp(Kernel::GetCommandBuffer(), 0x15, 1, 0);
    u32 size = rp.Pop<u32>();
    size_t buff_size;
    const VAddr out_buff = rp.PeekStaticBuffer(0, &buff_size);
    ASSERT_MSG(size == buff_size, "Sizes is not equal");

    Memory::ZeroBlock(out_buff, size); // TODO: fill data

    LOG_WARNING(Service_NFC, "(STUBBED) called, size=0x%x", size);
    IPC::RequestBuilder rb = rp.MakeBuilder(1, 0);
    rb.Push(RESULT_SUCCESS);
}

void WriteAppData(Interface* self) {
    IPC::RequestParser rp(Kernel::GetCommandBuffer(), 0x16, 9, 2);
    u32 size = rp.Pop<u32>();
    AppDataWriteStruct write_struct = rp.PopRaw<AppDataWriteStruct>();
    size_t buff_size;
    VAddr buff = rp.PopStaticBuffer(&buff_size);
    ASSERT_MSG(size == buff_size, "Sizes is not equal");

    LOG_WARNING(Service_NFC, "(STUBBED) called, size=0x%x", size);
    IPC::RequestBuilder rb = rp.MakeBuilder(1, 0);
    rb.Push(RESULT_SUCCESS);
}

void GetAmiiboSettings(Interface* self) {
    IPC::RequestParser rp(Kernel::GetCommandBuffer(), 0x17, 0, 0);
    LOG_WARNING(Service_NFC, "(STUBBED) called");
    IPC::RequestBuilder rb = rp.MakeBuilder(1 + sizeof(AmiiboSettings) / sizeof(u32), 0);
    rb.Push(RESULT_SUCCESS);
    rb.PushRaw(amiibo_settings);
}

void GetAmiiboConfig(Interface* self) {
    IPC::RequestParser rp(Kernel::GetCommandBuffer(), 0x18, 0, 0);
    LOG_WARNING(Service_NFC, "(STUBBED) called");
    IPC::RequestBuilder rb = rp.MakeBuilder(1 + sizeof(AmiiboConfig) / sizeof(u32), 0);
    rb.Push(RESULT_SUCCESS);
    rb.PushRaw(amiibo_config);
}

void IsAvailableFontRegion(Interface* self) {
    IPC::RequestParser rp(Kernel::GetCommandBuffer(), 0x1E, 1, 0);
    u8 region = rp.Pop<u8>();
    LOG_WARNING(Service_NFC, "(STUBBED) called, region=%u", region);
    IPC::RequestBuilder rb = rp.MakeBuilder(2, 0);
    rb.Push(RESULT_SUCCESS);
    rb.Push(true);
}

void Init() {
    AddService(new NFC_M());
    AddService(new NFC_U());

    using Kernel::Event;
    tag_in_range_event = Event::Create(Kernel::ResetType::OneShot, "NFC::tag_in_range_event");
    tag_out_of_range_event = Event::Create(Kernel::ResetType::OneShot, "NFC::tag_out_range_event");

    nfc_status = CommunicationStatus::AttemptInitialize;
    nfc_tag_state = TagState::NotInitialized;
}

void Shutdown() {
    tag_in_range_event = nullptr;
    tag_out_of_range_event = nullptr;
}

} // namespace NFC
} // namespace Service
