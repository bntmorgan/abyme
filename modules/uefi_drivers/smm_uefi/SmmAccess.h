/*++

Copyright (c) 1999 - 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


Module Name:

  SmmAccess.h

Abstract:

  This file defines SMM SMRAM Access abstraction protocol defined 
  by the SMM CIS.

--*/

#ifndef _SMM_ACCESS_H_
#define _SMM_ACCESS_H_

///
/// Structure describing a SMRAM region and its accessibility attributes.
///
typedef struct {
  ///
  /// Designates the physical address of the SMRAM in memory. This view of memory is 
  /// the same as seen by I/O-based agents, for example, but it may not be the address seen 
  /// by the processors.
  ///
  EFI_PHYSICAL_ADDRESS  PhysicalStart;
  ///
  /// Designates the address of the SMRAM, as seen by software executing on the 
  /// processors. This address may or may not match PhysicalStart.
  ///
  EFI_PHYSICAL_ADDRESS  CpuStart;       
  ///
  /// Describes the number of bytes in the SMRAM region.
  ///
  UINT64                PhysicalSize;
  ///
  /// Describes the accessibility attributes of the SMRAM.  These attributes include the 
  /// hardware state (e.g., Open/Closed/Locked), capability (e.g., cacheable), logical 
  /// allocation (e.g., allocated), and pre-use initialization (e.g., needs testing/ECC 
  /// initialization).
  ///
  UINT64                RegionState;
} EFI_SMRAM_DESCRIPTOR;

typedef struct _EFI_SMM_ACCESS_PROTOCOL EFI_SMM_ACCESS_PROTOCOL;

#define EFI_SMM_ACCESS_PROTOCOL_GUID \
  { \
    0x3792095a, 0xe309, 0x4c1e, {0xaa, 0x01, 0x85, 0xf5, 0x65, 0x5a, 0x17, 0xf1} \
  }

//
// SMM Access specification Data Structures
//
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_OPEN) (
  IN EFI_SMM_ACCESS_PROTOCOL         * This,
  UINTN                              DescriptorIndex
  );

typedef
EFI_STATUS
(EFIAPI *EFI_SMM_CLOSE) (
  IN EFI_SMM_ACCESS_PROTOCOL          * This,
  UINTN                               DescriptorIndex
  );

typedef
EFI_STATUS
(EFIAPI *EFI_SMM_LOCK) (
  IN EFI_SMM_ACCESS_PROTOCOL         * This,
  UINTN                              DescriptorIndex
  );

typedef
EFI_STATUS
(EFIAPI *EFI_SMM_CAPABILITIES) (
  IN EFI_SMM_ACCESS_PROTOCOL             * This,
  IN OUT UINTN                           *SmramMapSize,
  IN OUT EFI_SMRAM_DESCRIPTOR            * SmramMap
  );

struct _EFI_SMM_ACCESS_PROTOCOL {
  EFI_SMM_OPEN          Open;
  EFI_SMM_CLOSE         Close;
  EFI_SMM_LOCK          Lock;
  EFI_SMM_CAPABILITIES  GetCapabilities;
  BOOLEAN               LockState;
  BOOLEAN               OpenState;
};

extern EFI_GUID gEfiSmmAccessProtocolGuid;

#endif
