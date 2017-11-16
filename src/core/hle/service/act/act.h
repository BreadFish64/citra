// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

namespace Service {

class Interface;

namespace ACT {

/**
 * ACT::Initialize service function
 *  Inputs:
 *      0 : Command header (0x00010084)
 *      1 : SDK version
 *      2 : Shared memory size
 *      3 : Kernel PID header
 *      4 : The code to request the current process handle
 *      5 : Handle-transfer header for kernel
 *      6 : Shared memory address value
 *  Outputs:
 *      1 : Result, 0 on success, otherwise error code
 */
void Initialize(Interface* self);

/**
 * ACT::GetErrorCode service function
 *  Inputs:
 *      0 : Command header (0x00020040)
 *      1 : SDK version
 *  Outputs:
 *      1 : Result, 0 on success, otherwise error code
 *      2 : Result of last operation
 */
void GetErrorCode(Interface* self);

/**
 * ACT::GetAccountDataBlock service function
 *  Inputs:
 *      0 : Command header (0x000600C2)
 *      1 : Unknown, usually 0xFE?
 *      2 : Size
 *      3 : BlkID
 *      4 : (Size<<4) | 12
 *      5 : Output buffer pointer
 *  Outputs:
 *      1 : Result, 0 on success, otherwise error code
 */
void GetAccountDataBlock(Interface* self);

/// Initializes all ACT services
void Init();

/// Shutdown all ACT services
void Shutdown();

} // namespace ACT
} // namespace Service
