/** @file
  In-guest support for confidential migration

  Copyright (C) 2021 IBM Coporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiLib.h>

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


VOID
EFIAPI
MigrationHandlerMain ()
{
  UINT64                       MailboxStart;
  MH_COMMAND_PARAMETERS        *Params;
  VOID                         *PageVa;

  DebugPrint (DEBUG_INFO,"Migration Handler Started\n");

  MailboxStart = PcdGet32 (PcdConfidentialMigrationMailboxBase);
  Params = (VOID *)MailboxStart;
  PageVa = (VOID *)(MailboxStart + 0x1000);

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
