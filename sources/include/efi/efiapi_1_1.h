#ifndef __efiapi_1_1_h__
#define __efiapi_1_1_h__

//
// From EDK 1.1 Spec
//

#define EFI_BOOTSERVICE11
#define IN
#define OUT

typedef struct {
  UINT8 Type;
  UINT8 SubType;
  UINT8 Length[2];
} EFI_DEVICE_PATH_PROTOCOL;

typedef
EFI_BOOTSERVICE11
EFI_STATUS
(EFIAPI *EFI_CONNECT_CONTROLLER) (
  IN  EFI_HANDLE                    ControllerHandle,
  IN  EFI_HANDLE                    * DriverImageHandle OPTIONAL,
  IN  EFI_DEVICE_PATH_PROTOCOL      * RemainingDevicePath OPTIONAL,
  IN  BOOLEAN                       Recursive
  );

typedef
EFI_BOOTSERVICE11
EFI_STATUS
(EFIAPI *EFI_DISCONNECT_CONTROLLER) (
  IN EFI_HANDLE                              ControllerHandle,
  IN EFI_HANDLE                              DriverImageHandle, OPTIONAL
  IN EFI_HANDLE                              ChildHandle        OPTIONAL
  );

typedef
EFI_BOOTSERVICE11
EFI_STATUS
(EFIAPI *EFI_OPEN_PROTOCOL) (
  IN EFI_HANDLE                 Handle,
  IN EFI_GUID                   * Protocol,
  OUT VOID                      **Interface,
  IN  EFI_HANDLE                ImageHandle,
  IN  EFI_HANDLE                ControllerHandle, OPTIONAL
  IN  UINT32                    Attributes
  );

typedef
EFI_BOOTSERVICE11
EFI_STATUS
(EFIAPI *EFI_CLOSE_PROTOCOL) (
  IN EFI_HANDLE               Handle,
  IN EFI_GUID                 * Protocol,
  IN EFI_HANDLE               ImageHandle,
  IN EFI_HANDLE               DeviceHandle
  );

typedef struct {
  EFI_HANDLE  AgentHandle;
  EFI_HANDLE  ControllerHandle;
  UINT32      Attributes;
  UINT32      OpenCount;
} EFI_OPEN_PROTOCOL_INFORMATION_ENTRY;

typedef
EFI_BOOTSERVICE11
EFI_STATUS
(EFIAPI *EFI_OPEN_PROTOCOL_INFORMATION) (
  IN  EFI_HANDLE                          UserHandle,
  IN  EFI_GUID                            * Protocol,
  IN  EFI_OPEN_PROTOCOL_INFORMATION_ENTRY **EntryBuffer,
  OUT UINTN                               *EntryCount
  );

typedef
EFI_BOOTSERVICE11
EFI_STATUS
(EFIAPI *EFI_PROTOCOLS_PER_HANDLE) (
  IN EFI_HANDLE       UserHandle,
  OUT EFI_GUID        ***ProtocolBuffer,
  OUT UINTN           *ProtocolBufferCount
  );

typedef
EFI_BOOTSERVICE11
EFI_STATUS
(EFIAPI *EFI_LOCATE_HANDLE_BUFFER) (
  IN EFI_LOCATE_SEARCH_TYPE       SearchType,
  IN EFI_GUID                     * Protocol OPTIONAL,
  IN VOID                         *SearchKey OPTIONAL,
  IN OUT UINTN                    *NumberHandles,
  OUT EFI_HANDLE                  **Buffer
  );

typedef
EFI_BOOTSERVICE11
EFI_STATUS
(EFIAPI *EFI_LOCATE_PROTOCOL) (
  EFI_GUID  * Protocol,
  VOID      *Registration, OPTIONAL
  VOID      **Interface
  );

typedef struct _EFI_BOOT_SERVICES_1_1 {

  //
  // From EDK
  //

  //
  //////////////////////////////////////////////////////
  // EFI 1.1 Services
  //////////////////////////////////////////////////////

  //
  // DriverSupport Services
  //
  EFI_CONNECT_CONTROLLER                      ConnectController;
  EFI_DISCONNECT_CONTROLLER                   DisconnectController;

  //
  // Added Open and Close protocol for the new driver model
  //
  EFI_OPEN_PROTOCOL                           OpenProtocol;
  EFI_CLOSE_PROTOCOL                          CloseProtocol;
  EFI_OPEN_PROTOCOL_INFORMATION               OpenProtocolInformation;

  //
  // Added new services to EFI 1.1 as Lib to reduce code size.
  //
  EFI_PROTOCOLS_PER_HANDLE                    ProtocolsPerHandle;
  EFI_LOCATE_HANDLE_BUFFER                    LocateHandleBuffer;
  EFI_LOCATE_PROTOCOL                         LocateProtocol;

} EFI_BOOT_SERVICES_1_1;

#endif
