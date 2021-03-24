/*
Copyright (C) 2021  Benoît Morgan

This file is part of abyme

abyme is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

abyme is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with abyme.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __API_H__
#define __API_H__

#include <efi.h>
#include "efi/efi_82579LM.h"

#define API_ETH_TYPE 0xb00b

void test_send();
uint32_t send(const void *buf, uint32_t len, uint8_t flags);
uint32_t recv(void *buf, uint32_t len, uint8_t flags);
int uninstall(void);
int get_level(void);

extern EFI_HANDLE ih;
extern EFI_GUID guid_82579LM;
extern protocol_82579LM proto;

#endif

