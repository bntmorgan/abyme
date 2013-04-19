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

//
// PCI_IO
//
#define EFI_PCI_IO_PROTOCOL_GUID \
    { 0x4cf5b200, 0x68b8, 0x4ca5, {0x9e, 0xec, 0xb2, 0x3e, 0x3f, 0x50, 0x2, 0x9a } }

#define EFI_PCI_IO_PASS_THROUGH_BAR               0xff

typedef struct _EFI_PCI_IO_PROTOCOL  EFI_PCI_IO_PROTOCOL;

typedef enum {
  EfiPciIoWidthUint8      = 0,
  EfiPciIoWidthUint16,
  EfiPciIoWidthUint32,
  EfiPciIoWidthUint64,
  EfiPciIoWidthFifoUint8,
  EfiPciIoWidthFifoUint16,
  EfiPciIoWidthFifoUint32,
  EfiPciIoWidthFifoUint64,
  EfiPciIoWidthFillUint8,
  EfiPciIoWidthFillUint16,
  EfiPciIoWidthFillUint32,
  EfiPciIoWidthFillUint64,
  EfiPciIoWidthMaximum
} EFI_PCI_IO_PROTOCOL_WIDTH;

typedef enum {
  EfiPciIoOperationBusMasterRead,
  EfiPciIoOperationBusMasterWrite,
  EfiPciIoOperationBusMasterCommonBuffer,
  EfiPciIoOperationMaximum
} EFI_PCI_IO_PROTOCOL_OPERATION;

typedef
EFI_STATUS
(EFIAPI *EFI_PCI_IO_PROTOCOL_POLL_IO_MEM)(
  IN EFI_PCI_IO_PROTOCOL           *This,
  IN  EFI_PCI_IO_PROTOCOL_WIDTH    Width,
  IN  UINT8                        BarIndex,
  IN  UINT64                       Offset,
  IN  UINT64                       Mask,
  IN  UINT64                       Value,
  IN  UINT64                       Delay,
  OUT UINT64                       *Result
  );  

typedef
EFI_STATUS
(EFIAPI *EFI_PCI_IO_PROTOCOL_IO_MEM)(
  IN EFI_PCI_IO_PROTOCOL              *This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH    Width,
  IN     UINT8                        BarIndex,
  IN     UINT64                       Offset,
  IN     UINTN                        Count,
  IN OUT VOID                         *Buffer
  );

typedef struct {
  EFI_PCI_IO_PROTOCOL_IO_MEM  Read;        
  EFI_PCI_IO_PROTOCOL_IO_MEM  Write;               
} EFI_PCI_IO_PROTOCOL_ACCESS;

typedef
EFI_STATUS
(EFIAPI *EFI_PCI_IO_PROTOCOL_CONFIG)(
  IN EFI_PCI_IO_PROTOCOL              *This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH    Width,
  IN     UINT32                       Offset,
  IN     UINTN                        Count,
  IN OUT VOID                         *Buffer
  );

typedef struct {
  EFI_PCI_IO_PROTOCOL_CONFIG  Read;
  EFI_PCI_IO_PROTOCOL_CONFIG  Write;
} EFI_PCI_IO_PROTOCOL_CONFIG_ACCESS;

typedef
EFI_STATUS
(EFIAPI *EFI_PCI_IO_PROTOCOL_COPY_MEM)(
  IN EFI_PCI_IO_PROTOCOL              *This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH    Width,
  IN     UINT8                        DestBarIndex,
  IN     UINT64                       DestOffset,
  IN     UINT8                        SrcBarIndex,
  IN     UINT64                       SrcOffset,
  IN     UINTN                        Count
  );

typedef
EFI_STATUS
(EFIAPI *EFI_PCI_IO_PROTOCOL_MAP)(
  IN EFI_PCI_IO_PROTOCOL                *This,
  IN     EFI_PCI_IO_PROTOCOL_OPERATION  Operation,
  IN     VOID                           *HostAddress,
  IN OUT UINTN                          *NumberOfBytes,
  OUT    EFI_PHYSICAL_ADDRESS           *DeviceAddress,
  OUT    VOID                           **Mapping
  );

typedef
EFI_STATUS
(EFIAPI *EFI_PCI_IO_PROTOCOL_UNMAP)(
  IN EFI_PCI_IO_PROTOCOL           *This,
  IN  VOID                         *Mapping
  );

typedef
EFI_STATUS
(EFIAPI *EFI_PCI_IO_PROTOCOL_ALLOCATE_BUFFER)(
  IN EFI_PCI_IO_PROTOCOL           *This,
  IN  EFI_ALLOCATE_TYPE            Type,
  IN  EFI_MEMORY_TYPE              MemoryType,
  IN  UINTN                        Pages,
  OUT VOID                         **HostAddress,
  IN  UINT64                       Attributes
  );

typedef enum {
  EfiPciIoAttributeOperationGet,
  EfiPciIoAttributeOperationSet,
  EfiPciIoAttributeOperationEnable,
  EfiPciIoAttributeOperationDisable,
  EfiPciIoAttributeOperationSupported,
  EfiPciIoAttributeOperationMaximum
} EFI_PCI_IO_PROTOCOL_ATTRIBUTE_OPERATION;


typedef
EFI_STATUS
(EFIAPI *EFI_PCI_IO_PROTOCOL_FLUSH)(
  IN EFI_PCI_IO_PROTOCOL  *This
  );

typedef
EFI_STATUS
(EFIAPI *EFI_PCI_IO_PROTOCOL_GET_LOCATION)(
  IN EFI_PCI_IO_PROTOCOL          *This,
  OUT UINTN                       *SegmentNumber,
  OUT UINTN                       *BusNumber,
  OUT UINTN                       *DeviceNumber,
  OUT UINTN                       *FunctionNumber
  );

typedef
EFI_STATUS
(EFIAPI *EFI_PCI_IO_PROTOCOL_ATTRIBUTES)(
  IN EFI_PCI_IO_PROTOCOL                       *This,
  IN  EFI_PCI_IO_PROTOCOL_ATTRIBUTE_OPERATION  Operation,
  IN  UINT64                                   Attributes,
  OUT UINT64                                   *Result OPTIONAL
  );

typedef
EFI_STATUS
(EFIAPI *EFI_PCI_IO_PROTOCOL_GET_BAR_ATTRIBUTES)(
  IN EFI_PCI_IO_PROTOCOL             *This,
  IN  UINT8                          BarIndex,
  OUT UINT64                         *Supports, OPTIONAL
  OUT VOID                           **Resources OPTIONAL
  );

typedef
EFI_STATUS
(EFIAPI *EFI_PCI_IO_PROTOCOL_SET_BAR_ATTRIBUTES)(
  IN EFI_PCI_IO_PROTOCOL              *This,
  IN     UINT64                       Attributes,
  IN     UINT8                        BarIndex,
  IN OUT UINT64                       *Offset,
  IN OUT UINT64                       *Length
  );

typedef
EFI_STATUS
(EFIAPI *EFI_PCI_IO_PROTOCOL_FREE_BUFFER)(
  IN EFI_PCI_IO_PROTOCOL           *This,
  IN  UINTN                        Pages,
  IN  VOID                         *HostAddress
  );

struct _EFI_PCI_IO_PROTOCOL {
  EFI_PCI_IO_PROTOCOL_POLL_IO_MEM         PollMem;
  EFI_PCI_IO_PROTOCOL_POLL_IO_MEM         PollIo;
  EFI_PCI_IO_PROTOCOL_ACCESS              Mem;
  EFI_PCI_IO_PROTOCOL_ACCESS              Io;
  EFI_PCI_IO_PROTOCOL_CONFIG_ACCESS       Pci;
  EFI_PCI_IO_PROTOCOL_COPY_MEM            CopyMem;
  EFI_PCI_IO_PROTOCOL_MAP                 Map;
  EFI_PCI_IO_PROTOCOL_UNMAP               Unmap;
  EFI_PCI_IO_PROTOCOL_ALLOCATE_BUFFER     AllocateBuffer;
  EFI_PCI_IO_PROTOCOL_FREE_BUFFER         FreeBuffer;
  EFI_PCI_IO_PROTOCOL_FLUSH               Flush;
  EFI_PCI_IO_PROTOCOL_GET_LOCATION        GetLocation;
  EFI_PCI_IO_PROTOCOL_ATTRIBUTES          Attributes;
  EFI_PCI_IO_PROTOCOL_GET_BAR_ATTRIBUTES  GetBarAttributes;
  EFI_PCI_IO_PROTOCOL_SET_BAR_ATTRIBUTES  SetBarAttributes;
  UINT64                                  RomSize;
  VOID                                    *RomImage;
};

//
// SMM
//

#define EFI_SMM_BASE_PROTOCOL_GUID \
    { 0x1390954d, 0xda95, 0x4227, {0x93, 0x28, 0x72, 0x82, 0xc2, 0x17, 0xda, 0xa8} }

#define EFI_SMM_ACCESS_PROTOCOL_GUID \
    { 0x3792095a, 0xe309, 0x4c1e, {0xaa, 0x01, 0x85, 0xf5, 0x65, 0x5a, 0x17, 0xf1 } }
/*
typedef struct _EFI_SMM_ACCESS_PROTOCOL  EFI_SMM_ACCESS_PROTOCOL;

typedef struct {
  EFI_PHYSICAL_ADDRESS  PhysicalStart;
  EFI_PHYSICAL_ADDRESS  CpuStart;
  UINT64                PhysicalSize;
  UINT64                RegionState;
} EFI_SMRAM_DESCRIPTOR;

typedef
EFI_STATUS
(EFIAPI *EFI_SMM_OPEN)(
  IN EFI_SMM_ACCESS_PROTOCOL         *This,
  UINTN                              DescriptorIndex
  );

typedef
EFI_STATUS
(EFIAPI *EFI_SMM_CLOSE)(
  IN EFI_SMM_ACCESS_PROTOCOL          *This,
  UINTN                               DescriptorIndex
  );

typedef
EFI_STATUS
(EFIAPI *EFI_SMM_LOCK)(
  IN EFI_SMM_ACCESS_PROTOCOL         *This,
  UINTN                              DescriptorIndex
  );

typedef
EFI_STATUS
(EFIAPI *EFI_SMM_CAPABILITIES)(
  IN EFI_SMM_ACCESS_PROTOCOL             *This,
  IN OUT UINTN                           *SmramMapSize,
  IN OUT EFI_SMRAM_DESCRIPTOR            *SmramMap
  );

struct _EFI_SMM_ACCESS_PROTOCOL {
  EFI_SMM_OPEN          Open;
  EFI_SMM_CLOSE         Close;
  EFI_SMM_LOCK          Lock;
  EFI_SMM_CAPABILITIES  GetCapabilities;
  BOOLEAN               LockState;
  BOOLEAN               OpenState;
};

typedef struct _EFI_SMM_BASE_PROTOCOL  EFI_SMM_BASE_PROTOCOL;

typedef
EFI_STATUS
(EFIAPI *EFI_SMM_REGISTER_HANDLER)(
  IN  EFI_SMM_BASE_PROTOCOL                          *This,
  IN  EFI_DEVICE_PATH_PROTOCOL                       *FilePath,
  IN  VOID                                           *SourceBuffer OPTIONAL,
  IN  UINTN                                          SourceSize,
  OUT EFI_HANDLE                                     *ImageHandle,
  IN  BOOLEAN                                        LegacyIA32Binary OPTIONAL
  );

typedef
EFI_STATUS
(EFIAPI *EFI_SMM_UNREGISTER_HANDLER)(
  IN EFI_SMM_BASE_PROTOCOL          *This,
  IN EFI_HANDLE                     ImageHandle
  );

typedef
EFI_STATUS
(EFIAPI *EFI_SMM_COMMUNICATE)(
  IN EFI_SMM_BASE_PROTOCOL          *This,
  IN EFI_HANDLE                     ImageHandle,
  IN OUT VOID                       *CommunicationBuffer,
  IN OUT UINTN                      *SourceSize
  );

typedef
EFI_STATUS
(EFIAPI *EFI_SMM_CALLBACK_ENTRY_POINT)(
  IN EFI_HANDLE             SmmImageHandle,
  IN OUT VOID               *CommunicationBuffer OPTIONAL,
  IN OUT UINTN              *SourceSize OPTIONAL
  );

typedef
EFI_STATUS
(EFIAPI *EFI_SMM_CALLBACK_SERVICE)(
  IN EFI_SMM_BASE_PROTOCOL                            *This,
  IN EFI_HANDLE                                       SmmImageHandle,
  IN EFI_SMM_CALLBACK_ENTRY_POINT                     CallbackAddress,
  IN BOOLEAN                                          MakeLast OPTIONAL,
  IN BOOLEAN                                          FloatingPointSave OPTIONAL
  );

typedef
EFI_STATUS
(EFIAPI *EFI_SMM_ALLOCATE_POOL)(
  IN EFI_SMM_BASE_PROTOCOL          *This,
  IN EFI_MEMORY_TYPE                PoolType,
  IN UINTN                          Size,
  OUT VOID                          **Buffer
  );

typedef
EFI_STATUS
(EFIAPI *EFI_SMM_FREE_POOL)(
  IN EFI_SMM_BASE_PROTOCOL          *This,
  IN VOID                           *Buffer
  );

typedef
EFI_STATUS
(EFIAPI *EFI_SMM_INSIDE_OUT)(
  IN EFI_SMM_BASE_PROTOCOL          *This,
  OUT BOOLEAN                       *InSmm
  );

typedef struct _EFI_SMM_SYSTEM_TABLE      EFI_SMM_SYSTEM_TABLE;

typedef
EFI_STATUS
(EFIAPI *EFI_SMM_GET_SMST_LOCATION)(
  IN EFI_SMM_BASE_PROTOCOL          *This,
  IN OUT EFI_SMM_SYSTEM_TABLE       **Smst
  );

struct _EFI_SMM_BASE_PROTOCOL {
  EFI_SMM_REGISTER_HANDLER    Register;
  EFI_SMM_UNREGISTER_HANDLER  UnRegister;
  EFI_SMM_COMMUNICATE         Communicate;
  EFI_SMM_CALLBACK_SERVICE    RegisterCallback;
  EFI_SMM_INSIDE_OUT          InSmm;
  EFI_SMM_ALLOCATE_POOL       SmmAllocatePool;
  EFI_SMM_FREE_POOL           SmmFreePool;
  EFI_SMM_GET_SMST_LOCATION   GetSmstLocation;
};

typedef
EFI_STATUS
(EFIAPI *EFI_SMM_INSTALL_CONFIGURATION_TABLE)(
  IN EFI_SMM_SYSTEM_TABLE         *SystemTable,
  IN EFI_GUID                     *Guid,
  IN VOID                         *Table,
  IN UINTN                        TableSize
  );

struct _EFI_SMM_SYSTEM_TABLE {
  EFI_TABLE_HEADER                    Hdr;
  CHAR16                              *SmmFirmwareVendor;
  UINT32                              SmmFirmwareRevision;
  EFI_SMM_INSTALL_CONFIGURATION_TABLE SmmInstallConfigurationTable;
  EFI_GUID                            EfiSmmCpuIoGuid;
  EFI_SMM_CPU_IO_INTERFACE            SmmIo;
  EFI_SMMCORE_ALLOCATE_POOL           SmmAllocatePool;
  EFI_SMMCORE_FREE_POOL               SmmFreePool;
  EFI_SMMCORE_ALLOCATE_PAGES          SmmAllocatePages;
  EFI_SMMCORE_FREE_PAGES              SmmFreePages;
  EFI_SMM_STARTUP_THIS_AP             SmmStartupThisAp;
  UINTN                               CurrentlyExecutingCpu;
  UINTN                               NumberOfCpus;
  EFI_SMM_CPU_SAVE_STATE              *CpuSaveState;
  EFI_SMM_FLOATING_POINT_SAVE_STATE   *CpuOptionalFloatingPointState;
  UINTN                               NumberOfTableEntries;
  EFI_CONFIGURATION_TABLE             *SmmConfigurationTable;
};*/

//
// Bootservice
//

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
