/** @file

  Define Secure Encrypted Virtualization (SEV) base library helper function

  Copyright (c) 2017 - 2020, AMD Incorporated. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _MEM_ENCRYPT_SEV_LIB_H_
#define _MEM_ENCRYPT_SEV_LIB_H_

#include <Base.h>

//
// Define the maximum number of #VCs allowed (e.g. the level of nesting
// that is allowed => 2 allows for 1 nested #VCs). I this value is changed,
// be sure to increase the size of
//   gUefiOvmfPkgTokenSpaceGuid.PcdOvmfSecGhcbBackupSize
// in any FDF file using this PCD.
//
#define VMGEXIT_MAXIMUM_VC_COUNT   2

//
// Per-CPU data mapping structure
//   Use UINT32 for cached indicators and compare to a specific value
//   so that the hypervisor can't indicate a value is cached by just
//   writing random data to that area.
//
typedef struct {
  UINT32  Dr7Cached;
  UINT64  Dr7;

  UINTN   VcCount;
  VOID    *GhcbBackupPages;
} SEV_ES_PER_CPU_DATA;

//
// Internal structure for holding SEV-ES information needed during SEC phase
// and valid only during SEC phase and early PEI during platform
// initialization.
//
// This structure is also used by assembler files:
//   OvmfPkg/ResetVector/ResetVector.nasmb
//   OvmfPkg/ResetVector/Ia32/PageTables64.asm
//   OvmfPkg/ResetVector/Ia32/Flat32ToFlat64.asm
// any changes must stay in sync with its usage.
//
typedef struct _SEC_SEV_ES_WORK_AREA {
  UINT8    SevEsEnabled;
  UINT8    Reserved1[7];

  UINT64   RandomData;

  UINT64   EncryptionMask;
} SEC_SEV_ES_WORK_AREA;

//
// Memory encryption address range states.
//
typedef enum {
  MemEncryptSevAddressRangeUnencrypted,
  MemEncryptSevAddressRangeEncrypted,
  MemEncryptSevAddressRangeMixed,
  MemEncryptSevAddressRangeError,
} MEM_ENCRYPT_SEV_ADDRESS_RANGE_STATE;

/**
  Returns a boolean to indicate whether SEV-ES is enabled.

  @retval TRUE           SEV-ES is enabled
  @retval FALSE          SEV-ES is not enabled
**/
BOOLEAN
EFIAPI
MemEncryptSevEsIsEnabled (
  VOID
  );

/**
  Returns a boolean to indicate whether SEV is enabled

  @retval TRUE           SEV is enabled
  @retval FALSE          SEV is not enabled
**/
BOOLEAN
EFIAPI
MemEncryptSevIsEnabled (
  VOID
  );

/**
  Returns a boolean to indicate whether SEV live migration is enabled.

  @retval TRUE           SEV live migration is enabled
  @retval FALSE          SEV live migration is not enabled
**/
BOOLEAN
EFIAPI
MemEncryptSevLiveMigrationIsEnabled (
  VOID
  );

/**
  This function clears memory encryption bit for the memory region specified by
  BaseAddress and NumPages from the current page table context.

  @param[in]  Cr3BaseAddress          Cr3 Base Address (if zero then use
                                      current CR3)
  @param[in]  BaseAddress             The physical address that is the start
                                      address of a memory region.
  @param[in]  NumPages                The number of pages from start memory
                                      region.

  @retval RETURN_SUCCESS              The attributes were cleared for the
                                      memory region.
  @retval RETURN_INVALID_PARAMETER    Number of pages is zero.
  @retval RETURN_UNSUPPORTED          Clearing the memory encryption attribute
                                      is not supported
**/
RETURN_STATUS
EFIAPI
MemEncryptSevClearPageEncMask (
  IN PHYSICAL_ADDRESS         Cr3BaseAddress,
  IN PHYSICAL_ADDRESS         BaseAddress,
  IN UINTN                    NumPages
  );

/**
  This function sets memory encryption bit for the memory region specified by
  BaseAddress and NumPages from the current page table context.

  @param[in]  Cr3BaseAddress          Cr3 Base Address (if zero then use
                                      current CR3)
  @param[in]  BaseAddress             The physical address that is the start
                                      address of a memory region.
  @param[in]  NumPages                The number of pages from start memory
                                      region.

  @retval RETURN_SUCCESS              The attributes were set for the memory
                                      region.
  @retval RETURN_INVALID_PARAMETER    Number of pages is zero.
  @retval RETURN_UNSUPPORTED          Setting the memory encryption attribute
                                      is not supported
**/
RETURN_STATUS
EFIAPI
MemEncryptSevSetPageEncMask (
  IN PHYSICAL_ADDRESS         Cr3BaseAddress,
  IN PHYSICAL_ADDRESS         BaseAddress,
  IN UINTN                    NumPages
  );


/**
  Locate the page range that covers the initial (pre-SMBASE-relocation) SMRAM
  Save State Map.

  @param[out] BaseAddress     The base address of the lowest-address page that
                              covers the initial SMRAM Save State Map.

  @param[out] NumberOfPages   The number of pages in the page range that covers
                              the initial SMRAM Save State Map.

  @retval RETURN_SUCCESS      BaseAddress and NumberOfPages have been set on
                              output.

  @retval RETURN_UNSUPPORTED  SMM is unavailable.
**/
RETURN_STATUS
EFIAPI
MemEncryptSevLocateInitialSmramSaveStateMapPages (
  OUT UINTN *BaseAddress,
  OUT UINTN *NumberOfPages
  );

/**
  Returns the SEV encryption mask.

  @return  The SEV pagetable encryption mask
**/
UINT64
EFIAPI
MemEncryptSevGetEncryptionMask (
  VOID
  );

/**
  Returns the encryption state of the specified virtual address range.

  @param[in]  Cr3BaseAddress          Cr3 Base Address (if zero then use
                                      current CR3)
  @param[in]  BaseAddress             Base address to check
  @param[in]  Length                  Length of virtual address range

  @retval MemEncryptSevAddressRangeUnencrypted  Address range is mapped
                                                unencrypted
  @retval MemEncryptSevAddressRangeEncrypted    Address range is mapped
                                                encrypted
  @retval MemEncryptSevAddressRangeMixed        Address range is mapped mixed
  @retval MemEncryptSevAddressRangeError        Address range is not mapped
**/
MEM_ENCRYPT_SEV_ADDRESS_RANGE_STATE
EFIAPI
MemEncryptSevGetAddressRangeState (
  IN PHYSICAL_ADDRESS         Cr3BaseAddress,
  IN PHYSICAL_ADDRESS         BaseAddress,
  IN UINTN                    Length
  );

/**
  This function clears memory encryption bit for the MMIO region specified by
  BaseAddress and NumPages.

  @param[in]  Cr3BaseAddress          Cr3 Base Address (if zero then use
                                      current CR3)
  @param[in]  BaseAddress             The physical address that is the start
                                      address of a MMIO region.
  @param[in]  NumPages                The number of pages from start memory
                                      region.

  @retval RETURN_SUCCESS              The attributes were cleared for the
                                      memory region.
  @retval RETURN_INVALID_PARAMETER    Number of pages is zero.
  @retval RETURN_UNSUPPORTED          Clearing the memory encryption attribute
                                      is not supported
**/
RETURN_STATUS
EFIAPI
MemEncryptSevClearMmioPageEncMask (
  IN PHYSICAL_ADDRESS         Cr3BaseAddress,
  IN PHYSICAL_ADDRESS         BaseAddress,
  IN UINTN                    NumPages
  );

#define KVM_FEATURE_MIGRATION_CONTROL   BIT17

/**
  Figures out if we are running inside KVM HVM and
  KVM HVM supports SEV Live Migration feature.

  @retval TRUE           SEV live migration is supported.
  @retval FALSE          SEV live migration is not supported.
**/
BOOLEAN
EFIAPI
KvmDetectSevLiveMigrationFeature(
  VOID
  );

/**
 This hypercall is used to notify hypervisor when the page's encryption
 state changes.

 @param[in]   PhysicalAddress       The physical address that is the start address
                                    of a memory region.
 @param[in]   Pages                 Number of pages in memory region.
 @param[in]   IsEncrypted           Encrypted or Decrypted.

 @retval RETURN_SUCCESS             Hypercall returned success.
 @retval RETURN_UNSUPPORTED         Hypercall not supported.
 @retval RETURN_NO_MAPPING          Hypercall returned error.
**/
RETURN_STATUS
EFIAPI
SetMemoryEncDecHypercall3 (
  IN  UINTN     PhysicalAddress,
  IN  UINTN     Pages,
  IN  BOOLEAN   IsEncrypted
  );

#define KVM_HC_MAP_GPA_RANGE   12
#define KVM_MAP_GPA_RANGE_PAGE_SZ_4K    0
#define KVM_MAP_GPA_RANGE_PAGE_SZ_2M    BIT0
#define KVM_MAP_GPA_RANGE_PAGE_SZ_1G    BIT1
#define KVM_MAP_GPA_RANGE_ENC_STAT(n)   ((n) << 4)
#define KVM_MAP_GPA_RANGE_ENCRYPTED     KVM_MAP_GPA_RANGE_ENC_STAT(1)
#define KVM_MAP_GPA_RANGE_DECRYPTED     KVM_MAP_GPA_RANGE_ENC_STAT(0)

/**
  Interface exposed by the ASM implementation of the core hypercall

  @retval Hypercall returned status.
**/
UINTN
EFIAPI
SetMemoryEncDecHypercall3AsmStub (
  IN  UINTN  HypercallNum,
  IN  UINTN  PhysicalAddress,
  IN  UINTN  Pages,
  IN  UINTN  Attributes
  );

#endif // _MEM_ENCRYPT_SEV_LIB_H_
