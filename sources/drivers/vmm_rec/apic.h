#ifndef __APIC_H__
#define __APIC_H_

#include "stdint.h"

// See is controlled using APIC Range Select (ASEL) field
// and APIC Enable (AEN) bit to determine 0xfecxx000 xx bits
#define IO_APIC_DEFAULT_ADDR            0xfec00000
#define APIC_DEFAULT_ADDR               0xfee00000

struct apic_base_msr {
  union {
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
  struct {uint32_t r[4];} _jmp[43]; // XXX on saute jusqu'à la LVT
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
};

/**
 * Local APIC x2APIC fields
 */
struct x2apic_id {
  union {
    uint32_t id;
    uint32_t raw;
  };
};

struct x2apic_version {
  union{
    struct {
      uint32_t version:8;
      uint32_t _r0:8;
      uint32_t max_lvt_entry:8;
      uint32_t eoi_broadcast_suppression:1;
      uint32_t _r1:7;
    };
    uint32_t raw;
  };
};

struct x2apic_lvt_cmci {
  union{
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
};

struct x2apic_lvt_timer {
  union{
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
};

struct x2apic_lvt_thermal_sensor {
  union{
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
};

struct x2apic_lvt_performance_mon_counters {
  union{
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
};

struct x2apic_lvt_lint0 {
  union{
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
};

struct x2apic_lvt_lint1 {
  union{
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
};

struct x2apic_lvt_error {
  union{
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
};

void apic_setup(void);

#endif//__APIC_H__
