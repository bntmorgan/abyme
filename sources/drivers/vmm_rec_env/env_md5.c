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

#include "env.h"
#include "env_md5.h"
#include "stdint.h"
#include "string.h"
#include "stdio.h"
#include "md5.h"
#include "paging.h"
#include "vmx.h"
#include "vmcs.h"
#include "debug_server/debug_server.h"

#define MAX_PAGES 128

static uintptr_t start;
static uintptr_t end;
static uint8_t initialized = 0;
static uint64_t cr3;
static unsigned int idx = 0;

typedef struct _page {
  uint64_t a;
  uint64_t s;
} page;

static page pages[MAX_PAGES];

md5_byte_t last_digest[16];

void env_md5_send(md5_byte_t *digest) {
  uint8_t b[sizeof(message_user_defined) + 16];
  message_user_defined *m = (message_user_defined *)&b[0];
  uint8_t *data = b + sizeof(message_user_defined);
  m->type = MESSAGE_USER_DEFINED;
  m->vmid = vm->index;
  m->user_type = USER_DEFINED_LOG_MD5;
  m->length = 16;
  // Copy MD5
  memcpy(&data[0], &digest[0], 16);
  // Send it !
  debug_server_send(b, sizeof(b));
}

int env_md5_init(void) {
  INFO("Env md5 init\n");
  return ENV_OK;
}

int env_md5_walk(struct registers *guest_regs) {
  uint64_t lcurrent, pcurrent, *entry;
  uint8_t size;

  int ret;
  idx = 0;
  cr3 = cpu_vmread(GUEST_CR3);
  start = guest_regs->rsi;
  end = guest_regs->rdi;

  INFO("start = 0x%016X, end = 0x%016X%, 0x%016X octets..\n", start, end, end -
      start);

  //
  // Walk all the pages
  //
  lcurrent = start;

  // Walk the first page
  ret = paging_walk(cr3, lcurrent, &entry, &pcurrent, &size);
  if (ret) {
    INFO("ERROR walking 0x%016X : %d\n", lcurrent, ret);
    return ENV_ERROR;
  }
  pages[idx].a = pcurrent;
  pages[idx].s = (1 << size);
  idx++;
  INFO("Linear 0x%016X, physical : 0x%016X, size 0x%016X\n", lcurrent, pcurrent,
      size);
  while (!walk_is_in_page(lcurrent, end, size)) {
    // Compute the next page
    lcurrent = (1 << size) + (lcurrent & (~((uint64_t)size - 1)));
    // Walk the next page
    ret = paging_walk(cr3, lcurrent, &entry, &pcurrent, &size);
    if (ret) {
      INFO("ERROR walking 0x%016X : %d\n", lcurrent, ret);
      return ENV_ERROR;
    }
    pages[idx].a = pcurrent;
    pages[idx].s = (1 << size);
    idx++;
    INFO("Linear 0x%016X, physical : 0x%016X, size 0x%016X\n", lcurrent,
        pcurrent, size);
  }
  INFO("Walked %d pages\n", idx);

  initialized = 1;
  return ENV_OK;
}

/**
 * Change protected space data to demonstrate the effect of DMA attack
 */
#define ENV_MD5_MAGIC 0xcafebabedeadc0de
int env_md5_flip(uint8_t f) {
  static uint64_t data;
  if (!f) {
    if (*(uint64_t *)pages[0].a == ENV_MD5_MAGIC) {
      *(uint64_t *)pages[0].a = data;
    }
  } else {
    if (*(uint64_t *)pages[0].a != ENV_MD5_MAGIC) {
      data = *(uint64_t *)pages[0].a;
      *(uint64_t *)pages[0].a = ENV_MD5_MAGIC;
    }
  }
  return ENV_OK;
}

int env_md5_call(struct registers *guest_regs) {
  switch (guest_regs->rdx) {
    case ENV_MD5_VMCALL_ADDR:
      return env_md5_walk(guest_regs);
    case ENV_MD5_VMCALL_FLIP:
      return env_md5_flip(1);
    case ENV_MD5_VMCALL_UNFLIP:
      return env_md5_flip(0);
  }
  return ENV_ERROR;
}

int env_md5_execute(void) {
  md5_state_t state;
  md5_byte_t digest[16];
  int i;

  if (!initialized) {
    return ENV_ERROR;
  }

  // Init md5
  md5_init(&state);

  for (i = 0; i < idx - 1; i++) {
    // Compute md5 on current page
    md5_append(&state, (const md5_byte_t *)pages[i].a, pages[i].s -
        (pages[i].a & ((uint64_t)pages[i].s - 1)));
  }
  // Compute md5 on the last page
  md5_append(&state, (const md5_byte_t *)pages[i].a, (end &
        ((uint64_t)pages[i].s - 1)) + 1);
  md5_finish(&state, digest);

  // End md5
  int di;
  printk("MD5 (space) = ");
  for (di = 0; di < 16; ++di)
    printk("%02x", digest[di]);
  printk("\n");

  // Send the md5 to the debug client
  env_md5_send(&digest[0]);

  // Copy the new md5 as the new current
  memcpy(&last_digest[0], &digest[0], sizeof(last_digest));
  return ENV_OK;
}
