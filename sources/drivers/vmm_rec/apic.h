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

#ifndef __APIC_H__
#define __APIC_H_

#include "stdint.h"

// See is controlled using APIC Range Select (ASEL) field
// and APIC Enable (AEN) bit to determine 0xfecxx000 xx bits
#define IO_APIC_DEFAULT_ADDR            0xfec00000
#define APIC_DEFAULT_ADDR               0xfee00000

enum apic_mode {
  APIC_MODE_APIC,
  APIC_MODE_X2APIC
};

union apic_base_msr {
  uint64_t raw;
  struct {
    uint64_t _r0:8;
    uint64_t bsp:1; // is current core the bsp
    uint64_t _r1:1;
    uint64_t x2APIC_enable:1; // is current core the bsp
    uint64_t global_enable:1; // is apic enabled
    uint64_t apic_base:52; // mask with max phyaddr
  };
};

enum apic_timer_mode {
  APIC_TIMER_MODE_ONE_SHOT,
  APIC_TIMER_MODE_ONE_PERIODIC,
  APIC_TIMER_MODE_ONE_TSC_DEADLINE,
  APIC_TIMER_MODE_ONE_RESERVED,
};

union apic_timer_register {
  uint32_t raw;
  struct {
    uint32_t r0:17;
    uint32_t mode:2;
    uint32_t r1:13;
  };
};

struct local_apic {
  struct {uint32_t r[4];} _r0; // 00
  struct {uint32_t r[4];} _r1; // 10
  struct {
    uint32_t _r0:24;
    uint32_t id:8;
    uint32_t _r1[3];
  } id; // 20
  struct {
    uint32_t version:8;
    uint32_t _r0:8;
    uint32_t max_lvt_entry:8;
    uint32_t eoi_broadcast_suppression:1;
    uint32_t _r1:7;
    uint32_t _r2[3];
  } version; // 30
  struct {uint32_t r[4];} _jmp0[43]; // XXX on saute jusqu'à la LVT
  struct {
    uint32_t vector:8;
    uint32_t delivery_mode:3;
    uint32_t _r0:1;
    uint32_t delivery_status:1;
    uint32_t _r1:3;
    uint32_t mask:1;
    uint32_t _r2:15;
    uint32_t _r3[3];
  } lvt_cmci; // 2f0
  struct {uint32_t r[4];} icr_low; // 300
  struct {uint32_t r[4];} icr_high; // 310
  struct {
    uint32_t vector:8;
    uint32_t _r0:4;
    uint32_t delivery_status:1;
    uint32_t _r1:3;
    uint32_t mask:1;
    uint32_t mode:2;
    uint32_t _r2:13;
    uint32_t _r3[3];
  } lvt_timer; // 320
  struct {
    uint32_t vector:8;
    uint32_t delivery_mode:3;
    uint32_t _r0:1;
    uint32_t delivery_status:1;
    uint32_t _r1:3;
    uint32_t mask:1;
    uint32_t _r2:15;
    uint32_t _r3[3];
  } lvt_thermal_sensor; // 330
  struct {
    uint32_t vector:8;
    uint32_t delivery_mode:3;
    uint32_t _r0:1;
    uint32_t delivery_status:1;
    uint32_t _r1:3;
    uint32_t mask:1;
    uint32_t _r2:15;
    uint32_t _r3[3];
  } lvt_performance_mon_counters; // 340
  struct {
    uint32_t vector:8;
    uint32_t delivery_mode:3;
    uint32_t _r0:1;
    uint32_t delivery_status:1;
    uint32_t interrupt_input_pin_polarity:1;
    uint32_t remote_irr:1;
    uint32_t trigger_mode:1;
    uint32_t mask:1;
    uint32_t _r2:15;
    uint32_t _r3[3];
  } lvt_lint0; // 350
  struct {
    uint32_t vector:8;
    uint32_t delivery_mode:3;
    uint32_t _r0:1;
    uint32_t delivery_status:1;
    uint32_t interrupt_input_pin_polarity:1;
    uint32_t remote_irr:1;
    uint32_t trigger_mode:1;
    uint32_t mask:1;
    uint32_t _r2:15;
    uint32_t _r3[3];
  } lvt_lint1; // 360
  struct {
    uint32_t vector:8;
    uint32_t _r0:4;
    uint32_t delivery_status:1;
    uint32_t _r1:3;
    uint32_t mask:1;
    uint32_t _r2:15;
    uint32_t _r3[3];
  } lvt_error; // 370
  uint32_t initial_count; // 380
  uint32_t jmp1;
  uint32_t current_count; // 390
  uint32_t jmp2;
  struct {uint32_t r[4];} _jmp3[5]; // XXX on saute jusqu'au div. configuration
  struct {
    uint32_t div0:2;
    uint32_t _r0:1;
    uint32_t div1:1;
    uint32_t _r1:28;
  } divide_configuration;
};

/**
 * Local APIC x2APIC fields
 */
union x2apic_id {
  uint32_t id;
  uint32_t raw;
};

union x2apic_version {
  struct {
    uint32_t version:8;
    uint32_t _r0:8;
    uint32_t max_lvt_entry:8;
    uint32_t eoi_broadcast_suppression:1;
    uint32_t _r1:7;
  };
  uint32_t raw;
};

union x2apic_lvt_cmci {
  struct {
    uint32_t vector:8;
    uint32_t delivery_mode:3;
    uint32_t _r0:1;
    uint32_t delivery_status:1;
    uint32_t _r1:3;
    uint32_t mask:1;
    uint32_t _r2:15;
  };
  uint32_t raw;
};

union x2apic_lvt_timer {
  struct {
    uint32_t vector:8;
    uint32_t _r0:4;
    uint32_t delivery_status:1;
    uint32_t _r1:3;
    uint32_t mask:1;
    uint32_t mode:2;
    uint32_t _r2:13;
  };
  uint32_t raw;
};

union x2apic_lvt_thermal_sensor {
  struct {
    uint32_t vector:8;
    uint32_t delivery_mode:3;
    uint32_t _r0:1;
    uint32_t delivery_status:1;
    uint32_t _r1:3;
    uint32_t mask:1;
    uint32_t _r2:15;
  };
  uint32_t raw;
};

union x2apic_lvt_performance_mon_counters {
  struct {
    uint32_t vector:8;
    uint32_t delivery_mode:3;
    uint32_t _r0:1;
    uint32_t delivery_status:1;
    uint32_t _r1:3;
    uint32_t mask:1;
    uint32_t _r2:15;
  };
  uint32_t raw;
};

union x2apic_lvt_lint0 {
  struct {
    uint32_t vector:8;
    uint32_t delivery_mode:3;
    uint32_t _r0:1;
    uint32_t delivery_status:1;
    uint32_t interrupt_input_pin_polarity:1;
    uint32_t remote_irr:1;
    uint32_t trigger_mode:1;
    uint32_t mask:1;
    uint32_t _r2:15;
  };
  uint32_t raw;
};

union x2apic_lvt_lint1 {
  struct {
    uint32_t vector:8;
    uint32_t delivery_mode:3;
    uint32_t _r0:1;
    uint32_t delivery_status:1;
    uint32_t interrupt_input_pin_polarity:1;
    uint32_t remote_irr:1;
    uint32_t trigger_mode:1;
    uint32_t mask:1;
    uint32_t _r2:15;
  };
  uint32_t raw;
};

union x2apic_lvt_error {
  struct {
    uint32_t vector:8;
    uint32_t _r0:4;
    uint32_t delivery_status:1;
    uint32_t _r1:3;
    uint32_t mask:1;
    uint32_t _r2:15;
  };
  uint32_t raw;
};

void apic_setup(void);
int apic_get_mode(void);
void x2apic_print(void);
void apic_print(void);
int apic_is_vector_apic_timer(uint8_t vector);
void apic_emulate_apic_timer_expiration(void);

#endif//__APIC_H__
