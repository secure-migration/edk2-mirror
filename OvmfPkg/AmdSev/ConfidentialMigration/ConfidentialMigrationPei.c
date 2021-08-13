/** @file
  Reserve memory for confidential migration handler.

  Copyright (C) 2020 IBM Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <PiPei.h>
#include <Library/HobLib.h>
#include <Library/PcdLib.h>

EFI_STATUS
EFIAPI
InitializeConfidentialMigrationPei (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  BuildMemoryAllocationHob (
    PcdGet32 (PcdConfidentialMigrationMailboxBase),
    PcdGet32 (PcdConfidentialMigrationMailboxSize),
    EfiRuntimeServicesData
    );

  BuildMemoryAllocationHob (
    PcdGet32 (PcdConfidentialMigrationEntryBase),
    PcdGet32 (PcdConfidentialMigrationEntrySize),
    EfiRuntimeServicesData
    );

  return EFI_SUCCESS;
}
