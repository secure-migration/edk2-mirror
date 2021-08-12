/** @file
  In-guest support for confidential migration

  Copyright (C) 2021 IBM Coporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/DebugLib.h>
#include <Library/UefiLib.h>

VOID
EFIAPI
MigrationHandlerMain ()
{
  DebugPrint (DEBUG_INFO,"Migration Handler Started\n");

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
