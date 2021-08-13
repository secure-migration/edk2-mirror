; Entrypoint for Migration Handler

  DEFAULT REL
  SECTION .text

%define ENABLE_DEBUG   1
%define X86_CR0_PG     BIT31
%define X86_EFER_LME   BIT8
%define X86_CR4_PAE    BIT5

%define ENTRY_BASE FixedPcdGet32 (PcdConfidentialMigrationEntryBase)

%define LONG_MODE_OFFSET 0x200;
%define ENTRY_ADDRS_OFFSET 0x400
%define GDT_OFFSET 0x600

%define LONG_MODE_ADDR ENTRY_BASE + LONG_MODE_OFFSET
%define LINEAR_CODE64_SEL 0x38

BITS 32

global ASM_PFX(MigrationHandlerEntryPoint)
ASM_PFX(MigrationHandlerEntryPoint):

  ; CR3
  mov edi, [ENTRY_BASE + ENTRY_ADDRS_OFFSET]
  mov cr3, edi

  ; EFER.LME
  mov     ecx, 0xc0000080
  rdmsr
  bts      eax, 8
  wrmsr

  ; CR0.PG
  mov   eax, cr0
  bts   eax, 31
  mov   cr0, eax

  ; Far jump to enter long mode
  jmp LINEAR_CODE64_SEL:LONG_MODE_ADDR

BITS 64
global ASM_PFX(MigrationHandlerEntryPoint64)
ASM_PFX(MigrationHandlerEntryPoint64):

  ; RSP
  mov    rsp, [ENTRY_BASE + ENTRY_ADDRS_OFFSET + 0x8]

  ; Jump to MH
  jmp [ENTRY_BASE + ENTRY_ADDRS_OFFSET + 0x10]
