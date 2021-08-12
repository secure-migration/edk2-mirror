/** @file
  In-guest support for confidential migration

  Copyright (C) 2021 IBM Coporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiLib.h>

#include "VirtualMemory.h"

//
// Functions implemented by the migration handler
//
#define MH_FUNC_INIT          0
#define MH_FUNC_SAVE_PAGE     1
#define MH_FUNC_RESTORE_PAGE  2
#define MH_FUNC_RESET         3

//
// Return codes for MH functions
//
#define MH_SUCCESS            0
#define MH_INVALID_FUNC     (-1)
#define MH_AUTH_ERR         (-2)

//
// Mailbox for communication with hypervisor
//
typedef volatile struct {
  UINT64       Nr;
  UINT64       Gpa;
  UINT32       DoPrefetch;
  UINT32       Ret;
  UINT32       Go;
  UINT32       Done;
} MH_COMMAND_PARAMETERS;

//
// Offset for non-cbit mapping.
//
#define UNENC_VIRT_ADDR_BASE    0xffffff8000000000ULL

STATIC PAGE_TABLE_POOL   *mPageTablePool = NULL;
PHYSICAL_ADDRESS  mMigrationHandlerPageTables = 0;

/**
  Allocates and fills in custom page tables for Migration Handler.
  The MH must be able to write to any encrypted page. Thus, it
  uses an identity map where the C-bit is set for every page. The
  HV should never ask the MH to import/export a shared page. The
  MH must also be able to read some shared pages. The first 1GB
  of memory is mapped at offset UNENC_VIRT_ADDR_BASE.
**/
VOID
PrepareMigrationHandlerPageTables (
  VOID
  )
{
  UINTN                            PoolPages;
  VOID                             *Buffer;
  VOID                             *Start;
  PAGE_MAP_AND_DIRECTORY_POINTER   *PageMapLevel4Entry;
  PAGE_TABLE_1G_ENTRY              *PageDirectory1GEntry;
  PAGE_TABLE_1G_ENTRY              *Unenc1GEntry;
  UINT64                           AddressEncMask;

  PoolPages = 1 + 10;
  Buffer = AllocateAlignedRuntimePages (PoolPages, PAGE_TABLE_POOL_ALIGNMENT);
  mPageTablePool = Buffer;
  mPageTablePool->NextPool = mPageTablePool;
  mPageTablePool->FreePages  = PoolPages - 1;
  mPageTablePool->Offset = EFI_PAGES_TO_SIZE (1);

  Start = (UINT8 *)mPageTablePool + mPageTablePool->Offset;
  ZeroMem(Start, mPageTablePool->FreePages * EFI_PAGE_SIZE);

  AddressEncMask = 1ULL << 47;

  PageMapLevel4Entry = Start;
  PageDirectory1GEntry = (PAGE_TABLE_1G_ENTRY*)((UINT8*)Start + EFI_PAGE_SIZE);
  Unenc1GEntry = (PAGE_TABLE_1G_ENTRY*)((UINT8*)Start + 2 * EFI_PAGE_SIZE);

  PageMapLevel4Entry = Start;
  PageMapLevel4Entry += PML4_OFFSET(0x0ULL);
  PageMapLevel4Entry->Uint64 = (UINT64)PageDirectory1GEntry | AddressEncMask | 0x23;

  PageMapLevel4Entry = Start;
  PageMapLevel4Entry += PML4_OFFSET(UNENC_VIRT_ADDR_BASE); // should be 511
  PageMapLevel4Entry->Uint64 = (UINT64)Unenc1GEntry | AddressEncMask | 0x23;

  UINT64 PageAddr = 0;
  for (int i = 0; i < 512; i++, PageAddr += SIZE_1GB) {
    PAGE_TABLE_1G_ENTRY *e = PageDirectory1GEntry + i;
    e->Uint64 = PageAddr | AddressEncMask | 0xe3; // 1GB page
  }

  UINT64 UnencPageAddr = 0;
  Unenc1GEntry->Uint64 = UnencPageAddr | 0xe3; // 1GB page unencrypted

  mMigrationHandlerPageTables = (UINT64)Start | AddressEncMask;
}


VOID
EFIAPI
MigrationHandlerMain ()
{
  UINT64                       MailboxStart;
  MH_COMMAND_PARAMETERS        *Params;
  VOID                         *PageVa;

  DebugPrint (DEBUG_INFO,"Migration Handler Started\n");

  MailboxStart = PcdGet32 (PcdConfidentialMigrationMailboxBase);
  Params = (VOID *)(MailboxStart + UNENC_VIRT_ADDR_BASE);
  PageVa = (VOID *)(MailboxStart + UNENC_VIRT_ADDR_BASE + 0x1000);

  DisableInterrupts ();
  Params->Go = 0;

  while (1) {
    while (!Params->Go) {
      CpuPause ();
    }
    Params->Done = 0;

    switch (Params->Nr) {
    case MH_FUNC_INIT:
      Params->Ret = MH_SUCCESS;
      break;

    case MH_FUNC_SAVE_PAGE:
      CopyMem (PageVa, (VOID *)Params->Gpa, 4096);
      Params->Ret = MH_SUCCESS;
      break;

    case MH_FUNC_RESTORE_PAGE:
      CopyMem ((VOID *)Params->Gpa, PageVa, 4096);
      Params->Ret = MH_SUCCESS;
      break;

    case MH_FUNC_RESET:
      Params->Ret = MH_SUCCESS;
      break;

    default:
      Params->Ret = MH_INVALID_FUNC;
      break;
    }

    Params->Go = 0;
    Params->Done = 1;

  }
}

/**
SetupMigrationHandler runs in the firmware of the main VM to setup
regions of memory that the Migration Handler can use when executing
in the mirror VM.

**/
EFI_STATUS
EFIAPI
SetupMigrationHandler (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{

  if (!PcdGetBool(PcdStartConfidentialMigrationHandler)) {
    return 0;
  }

	PrepareMigrationHandlerPageTables ();

  //
  // If VM is migration target, wait until hypervisor modifies CPU state
  // and restarts execution.
  //
  if (PcdGetBool(PcdIsConfidentialMigrationTarget)) {
    DebugPrint (DEBUG_INFO,"Waiting for incoming confidential migration.\n");

    while (1) {
      CpuPause ();
    }
  }

  //
  // If VM is migration source, continue with boot.
  //
  return 0;
}
