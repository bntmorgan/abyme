#ifndef __EFI_API_H__
#define __EFI_API_H__

#include "stdint.h"

typedef uint64_t UINTN;
typedef uint32_t UINT32;
typedef int32_t INT32;
typedef uint16_t UINT16;
typedef int16_t INT16;
typedef uint8_t UINT8;
typedef int8_t INT8;

// #define EFIAPI
#define EFIAPI __attribute__((ms_abi))

#ifndef IN
  #define IN
  #define OUT
  #define OPTIONAL
#endif

#undef VOID
#define VOID void

typedef UINTN EFI_STATUS;
typedef VOID *EFI_HANDLE;
typedef UINT16 CHAR16;

typedef struct {          
  UINT32 Data1;
  UINT16 Data2;
  UINT16 Data3;
  UINT8 Data4[8]; 
} EFI_GUID;

typedef enum {
  EFI_NATIVE_INTERFACE,
  EFI_PCODE_INTERFACE
} EFI_INTERFACE_TYPE;

typedef
EFI_STATUS
(EFIAPI *EFI_LOCATE_PROTOCOL) (
  IN EFI_GUID *Protocol,
  IN VOID *Registration OPTIONAL,
  OUT VOID **Interface);

typedef
EFI_STATUS
(EFIAPI *EFI_INSTALL_PROTOCOL_INTERFACE) (
  IN OUT EFI_HANDLE *Handle,
  IN EFI_GUID *Protocol,
  IN EFI_INTERFACE_TYPE InterfaceType,
  IN VOID *Interface);

typedef VOID SIMPLE_TEXT_OUTPUT_INTERFACE;

typedef
EFI_STATUS
(EFIAPI *EFI_TEXT_OUTPUT_STRING) (
  IN SIMPLE_TEXT_OUTPUT_INTERFACE *This,
  IN CHAR16 *String);

typedef struct _EFI_S_T {
  EFI_LOCATE_PROTOCOL LocateProtocol;
  EFI_INSTALL_PROTOCOL_INTERFACE InstallProtocolInterface;
  EFI_TEXT_OUTPUT_STRING OutputString;
  SIMPLE_TEXT_OUTPUT_INTERFACE *ConOut;
  EFI_HANDLE image;
} EFI_S_T;

void efi_output_string(CHAR16 *string);

#endif
