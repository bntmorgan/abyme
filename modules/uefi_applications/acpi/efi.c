#include <efi.h>
#include <efilib.h>

//
// here is the slighlty complicated ACPI poweroff code
//

uint32_t *SMI_CMD;
uint8_t ACPI_ENABLE;
uint8_t ACPI_DISABLE;
uint32_t *PM1a_CNT;
uint32_t *PM1b_CNT;
uint16_t SLP_TYPa;
uint16_t SLP_TYPb;
uint16_t SLP_EN;
uint16_t SCI_EN;
uint8_t PM1_CNT_LEN;

struct RSDPtr
{
   uint8_t Signature[8];
   uint8_t CheckSum;
   uint8_t OemID[6];
   uint8_t Revision;
   uint32_t *RsdtAddress;
};

struct FACP
{
   uint8_t Signature[4];
   uint32_t Length;
   uint8_t unneded1[40 - 8];
   uint32_t *DSDT;
   uint8_t unneded2[48 - 44];
   uint32_t *SMI_CMD;
   uint8_t ACPI_ENABLE;
   uint8_t ACPI_DISABLE;
   uint8_t unneded3[64 - 54];
   uint32_t *PM1a_CNT_BLK;
   uint32_t *PM1b_CNT_BLK;
   uint8_t unneded4[89 - 72];
   uint8_t PM1_CNT_LEN;
};

int memcmp(uint8_t *s1, uint8_t *s2, int n)
{
   int i;
   for(i=0;i<n;i++)
   {
      if(s1[n] != s2[n])
         return 1;
   }
   return 0;
}

// check if the given address has a valid header
unsigned int *acpiCheckRSDPtr(unsigned int *ptr)
{
   char *sig = "RSD PTR ";
   struct RSDPtr *rsdp = (struct RSDPtr *) ptr;
   uint8_t *bptr;
   uint8_t check = 0;
   int i;

   if (memcmp((uint8_t*)sig, (uint8_t*)rsdp, 8) == 0)
   {
      // check checksum rsdpd
      bptr = (uint8_t *) ptr;
      for (i=0; i<sizeof(struct RSDPtr); i++)
      {
         check += *bptr;
         bptr++;
      }

      // found valid rsdpd   
      if (check == 0) {
         /*
          if (desc->Revision == 0)
            Print(L"acpi 1");
         else
            Print(L"acpi 2");
         */
         return (unsigned int *) rsdp->RsdtAddress;
      }
   }

   return NULL;
}



// finds the acpi header and returns the address of the rsdt
unsigned int *acpiGetRSDPtr(void)
{
   uint64_t addr;
   unsigned int *rsdp;

   // search below the 1mb mark for RSDP signature
   for (addr = 0x000E0000; addr<0x00100000; addr += 0x10/sizeof(addr))
   {
      rsdp = acpiCheckRSDPtr((unsigned int*)addr);
      if (rsdp != NULL)
         return rsdp;
   }

   // at address 0x40:0x0E is the RM segment of the ebda
   uint64_t ebda = *((short *) 0x40E);   // get pointer
   ebda = ebda*0x10 &0x000FFFFF;   // transform segment into linear address

   // search Extended BIOS Data Area for the Root System Description Pointer signature
   for (addr = ebda; addr<ebda+1024; addr+= 0x10/sizeof(addr))
   {
      rsdp = acpiCheckRSDPtr((unsigned int*)addr);
      if (rsdp != NULL)
         return rsdp;
   }

   return NULL;
}



// checks for a given header and validates checksum
int acpiCheckHeader(unsigned int *ptr, char *sig)
{
   if (memcmp((uint8_t*)ptr, (uint8_t*)sig, 4) == 0)
   {
      char *checkPtr = (char *) ptr;
      int len = *(ptr + 1);
      char check = 0;
      while (0<len--)
      {
         check += *checkPtr;
         checkPtr++;
      }
      if (check == 0)
         return 0;
   }
   return -1;
}

uint16_t inw(uint32_t port) {
  uint16_t v;
  __asm__ __volatile__(
      "in %%dx, %%ax;"
  : "=a"(v) : "d"(port));
  return v;
}

uint16_t inb(uint32_t port) {
  uint16_t v;
  __asm__ __volatile__(
      "in %%dx, %%al;"
  : "=a"(v) : "d"(port));
  return v;
}

void outb(uint32_t port, uint8_t value) {
  __asm__ __volatile__(
      "out %%al, %%dx;"
  : "=a"(value) : "d"(port));
}

void outw(uint32_t port, uint16_t value) {
  __asm__ __volatile__(
      "out %%ax, %%dx;"
  : "=a"(value) : "d"(port));
}

void sleeep(uint32_t time) {
  uint64_t i;
  for (i = 0; i < time * 10e6; i++);
}

int acpiEnable(void)
{
   // check if acpi is enabled
   if ( (inw((uint64_t) PM1a_CNT) &SCI_EN) == 0 )
   {
      // check if acpi can be enabled
      if (SMI_CMD != 0 && ACPI_ENABLE != 0)
      {
         outb((uint64_t) SMI_CMD, ACPI_ENABLE); // send acpi enable command
         // give 3 seconds time to enable acpi
         int i;
         for (i=0; i<300; i++ )
         {
            if ( (inw((uint64_t) PM1a_CNT) &SCI_EN) == 1 )
               break;
            sleeep(100);
         }
         if (PM1b_CNT != 0)
            for (; i<300; i++ )
            {
               if ( (inw((uint64_t) PM1b_CNT) &SCI_EN) == 1 )
                  break;
               sleeep(100);
            }
         if (i<300) {
            Print(L"enabled acpi.\n");
            return 0;
         } else {
            Print(L"couldn't enable acpi.\n");
            return -1;
         }
      } else {
         Print(L"no known way to enable acpi.\n");
         return -1;
      }
   } else {
      Print(L"acpi was already enabled.\n");
      return 0;
   }
}



//
// bytecode of the \_S5 object
// -----------------------------------------
//        | (optional) |    |    |    |   
// NameOP | \          | _  | S  | 5  | _
// 08     | 5A         | 5F | 53 | 35 | 5F
// 
// -----------------------------------------------------------------------------------------------------------
//           |           |              | ( SLP_TYPa   ) | ( SLP_TYPb   ) | ( Reserved   ) | (Reserved    )
// PackageOP | PkgLength | NumElements  | byteprefix Num | byteprefix Num | byteprefix Num | byteprefix Num
// 12        | 0A        | 04           | 0A         05  | 0A          05 | 0A         05  | 0A         05
//
//----this-structure-was-also-seen----------------------
// PackageOP | PkgLength | NumElements | 
// 12        | 06        | 04          | 00 00 00 00
//
// (Pkglength bit 6-7 encode additional PkgLength bytes [shouldn't be the case here])
//
int initAcpi(void)
{
   unsigned int *ptr = acpiGetRSDPtr();

   // check if address is correct  ( if acpi is available on this pc )
   if (ptr != NULL && acpiCheckHeader(ptr, "RSDT") == 0)
   {
      // the RSDT contains an unknown number of pointers to acpi tables
      int entrys = *(ptr + 1);
      entrys = (entrys-36) /4;
      ptr += 36/4;   // skip header information

      while (0<entrys--)
      {
         // check if the desired table is reached
         if (acpiCheckHeader((unsigned int *) ((uint64_t)*ptr), "FACP") == 0)
         {
            entrys = -2;
            struct FACP *facp = (struct FACP *) ((uint64_t)*ptr);
            if (acpiCheckHeader((unsigned int *) ((uint64_t)facp->DSDT), "DSDT") == 0)
            {
               // search the \_S5 package in the DSDT
               char *S5Addr = (char *) facp->DSDT +36; // skip header
               int dsdtLength = *(facp->DSDT+1) -36;
               while (0 < dsdtLength--)
               {
                  if ( memcmp((uint8_t*)S5Addr, (uint8_t*)"_S5_", 4) == 0)
                     break;
                  S5Addr++;
               }
               // check if \_S5 was found
               if (dsdtLength > 0)
               {
                  // check for valid AML structure
                  if ( ( *(S5Addr-1) == 0x08 || ( *(S5Addr-2) == 0x08 && *(S5Addr-1) == '\\') ) && *(S5Addr+4) == 0x12 )
                  {
                     S5Addr += 5;
                     S5Addr += ((*S5Addr &0xC0)>>6) +2;   // calculate PkgLength size

                     if (*S5Addr == 0x0A)
                        S5Addr++;   // skip byteprefix
                     SLP_TYPa = *(S5Addr)<<10;
                     S5Addr++;

                     if (*S5Addr == 0x0A)
                        S5Addr++;   // skip byteprefix
                     SLP_TYPb = *(S5Addr)<<10;

                     SMI_CMD = facp->SMI_CMD;

                     ACPI_ENABLE = facp->ACPI_ENABLE;
                     ACPI_DISABLE = facp->ACPI_DISABLE;

                     PM1a_CNT = facp->PM1a_CNT_BLK;
                     PM1b_CNT = facp->PM1b_CNT_BLK;
                     
                     PM1_CNT_LEN = facp->PM1_CNT_LEN;

                     SLP_EN = 1<<13;
                     SCI_EN = 1;

                     return 0;
                  } else {
                     Print(L"\\_S5 parse error.\n");
                  }
               } else {
                  Print(L"\\_S5 not present.\n");
               }
            } else {
               Print(L"DSDT invalid.\n");
            }
         }
         ptr++;
      }
      Print(L"no valid FACP present.\n");
   } else {
      Print(L"no acpi.\n");
   }

   return -1;
}



void acpiPowerOff(void)
{
   // SCI_EN is set to 1 if acpi shutdown is possible
   //if (SCI_EN == 0)
      //return;

   initAcpi();
   acpiEnable();

   // send the shutdown command
   outw((unsigned int) (uint64_t)PM1a_CNT, SLP_TYPa | SLP_EN );
   if ( PM1b_CNT != 0 )
      outw((unsigned int) (uint64_t)PM1b_CNT, SLP_TYPb | SLP_EN );

   Print(L"acpi poweroff failed.\n");
}

EFI_STATUS
efi_main (EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *st)
{
  InitializeLib(image_handle, st);
  EFI_STATUS status = 0;
  Print(L"Shell %d\n", status);

  acpiPowerOff();
        
  return EFI_SUCCESS;
}

