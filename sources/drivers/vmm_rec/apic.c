#include "apic.h"
#include "msr.h"
#include "stdint.h"
#include "cpuid.h"
#include "stdio.h"

static struct local_apic *la;
static struct apic_base_msr apic_base;
static uint8_t max_lvt_entry;
static uint8_t max_phyaddr; // Physical address space
static uint8_t apic_mode;

enum apic_mode {
  MODE_APIC,
  MODE_X2APIC
};

void x2apic_print(void) {
  struct x2apic_id id = {.raw =
    msr_read(MSR_ADDRESS_IA32_X2APIC_APICID)};
  struct x2apic_version version = {.raw = msr_read(MSR_ADDRESS_IA32_X2APIC_VESION)};
  struct x2apic_lvt_cmci lvt_cmci = {.raw = msr_read(MSR_ADDRESS_IA32_X2APIC_LVT_CMCI)};
  struct x2apic_lvt_timer lvt_timer =
    {.raw = msr_read(MSR_ADDRESS_IA32_X2APIC_LVT_TIMER)};
  struct x2apic_lvt_thermal_sensor lvt_thermal_sensor =
    {.raw = msr_read(MSR_ADDRESS_IA32_X2APIC_LVT_THERMAL)};
  struct x2apic_lvt_performance_mon_counters lvt_performance_mon_counters =
    {.raw = msr_read(MSR_ADDRESS_IA32_X2APIC_LVT_PMI)};
  struct x2apic_lvt_lint0 lvt_lint0 =
    {.raw = msr_read(MSR_ADDRESS_IA32_X2APIC_LVT_LINT0)};
  struct x2apic_lvt_lint1 lvt_lint1 =
    {.raw = msr_read(MSR_ADDRESS_IA32_X2APIC_LVT_LINT1)};
  struct x2apic_lvt_error lvt_error =
    {.raw = msr_read(MSR_ADDRESS_IA32_X2APIC_LVT_ERROR)};
  INFO("x2APIC ID 0x%08x\n", id.id);
  INFO("x2APIC version 0x%08x\n", version.version);
  INFO("CMCI : vector(0x%02x), delivery_mode(0x%x), \
delivery_status(%x), mask(%x)\n",
      lvt_cmci.vector, lvt_cmci.delivery_mode,
      lvt_cmci.delivery_status, lvt_cmci.mask); 
  INFO("Timer : vector(0x%02x), mode(0x%x), \
delivery_status(%x), mask(%x)\n",
      lvt_timer.vector, lvt_timer.mode,
      lvt_timer.delivery_status, lvt_timer.mask); 
  INFO("Thermal sensor : vector(0x%02x), delivery_mode(0x%x), \
delivery_status(%x), mask(%x)\n",
      lvt_thermal_sensor.vector, lvt_thermal_sensor.delivery_mode,
      lvt_thermal_sensor.delivery_status, lvt_thermal_sensor.mask); 
  INFO("Performance mon counters: vector(0x%02x), delivery_mode(0x%x), \
delivery_status(%x), mask(%x)\n",
      lvt_performance_mon_counters.vector,
      lvt_performance_mon_counters.delivery_mode,
      lvt_performance_mon_counters.delivery_status,
      lvt_performance_mon_counters.mask); 
  INFO("lint0 : vector(0x%02x), delivery_mode(0x%x), \
interrupt_input_pin_polarity(%x), remote_irr(%x), \n    trigger_mode(%x), \
delivery_status(%x), mask(%x)\n",
      lvt_lint0.vector, lvt_lint0.delivery_mode,
      lvt_lint0.interrupt_input_pin_polarity, lvt_lint0.remote_irr,
      lvt_lint0.trigger_mode, lvt_lint0.delivery_status,
      lvt_lint0.mask); 
  INFO("lint1 : vector(0x%02x), delivery_mode(0x%x), \
interrupt_input_pin_polarity(%x), remote_irr(%x), \n    trigger_mode(%x), \
delivery_status(%x), mask(%x)\n",
      lvt_lint1.vector, lvt_lint1.delivery_mode,
      lvt_lint1.interrupt_input_pin_polarity, lvt_lint1.remote_irr,
      lvt_lint1.trigger_mode, lvt_lint1.delivery_status,
      lvt_lint1.mask); 
  INFO("Error : vector(0x%02x), delivery_status(%x), mask(%x)\n",
      lvt_error.vector, lvt_error.delivery_status, lvt_error.mask); 
}

void apic_lvt_print(void) {
  INFO("@CMCI 0x%016X == 0x2f0\n", &la->lvt_cmci);
  INFO("CMCI : vector(0x%02x), delivery_mode(0x%x), \
delivery_status(%x), mask(%x)\n",
      la->lvt_cmci.vector, la->lvt_cmci.delivery_mode,
      la->lvt_cmci.delivery_status, la->lvt_cmci.mask); 
  INFO("Timer : vector(0x%02x), mode(0x%x), \
delivery_status(%x), mask(%x)\n",
      la->lvt_timer.vector, la->lvt_timer.mode,
      la->lvt_timer.delivery_status, la->lvt_timer.mask); 
  INFO("Thermal sensor : vector(0x%02x), delivery_mode(0x%x), \
delivery_status(%x), mask(%x)\n",
      la->lvt_thermal_sensor.vector, la->lvt_thermal_sensor.delivery_mode,
      la->lvt_thermal_sensor.delivery_status, la->lvt_thermal_sensor.mask); 
  INFO("Performance mon counters: vector(0x%02x), delivery_mode(0x%x), \
delivery_status(%x), mask(%x)\n",
      la->lvt_performance_mon_counters.vector,
      la->lvt_performance_mon_counters.delivery_mode,
      la->lvt_performance_mon_counters.delivery_status,
      la->lvt_performance_mon_counters.mask); 
  INFO("lint0 : vector(0x%02x), delivery_mode(0x%x), \
interrupt_input_pin_polarity(%x), remote_irr(%x), \n    trigger_mode(%x), \
delivery_status(%x), mask(%x)\n",
      la->lvt_lint0.vector, la->lvt_lint0.delivery_mode,
      la->lvt_lint0.interrupt_input_pin_polarity, la->lvt_lint0.remote_irr,
      la->lvt_lint0.trigger_mode, la->lvt_lint0.delivery_status,
      la->lvt_lint0.mask); 
  INFO("lint1 : vector(0x%02x), delivery_mode(0x%x), \
interrupt_input_pin_polarity(%x), remote_irr(%x), \n    trigger_mode(%x), \
delivery_status(%x), mask(%x)\n",
      la->lvt_lint1.vector, la->lvt_lint1.delivery_mode,
      la->lvt_lint1.interrupt_input_pin_polarity, la->lvt_lint1.remote_irr,
      la->lvt_lint1.trigger_mode, la->lvt_lint1.delivery_status,
      la->lvt_lint1.mask); 
  INFO("Error : vector(0x%02x), delivery_status(%x), mask(%x)\n",
      la->lvt_error.vector, la->lvt_error.delivery_status, la->lvt_error.mask); 
}

void apic_print(void) {
  INFO("APIC base(0x%016X), is_bsp(%x), enabled(%x)\n", la, apic_base.bsp,
      apic_base.global_enable); 
  INFO("PROCESSOR ID 0x%02x\n", la->id.id);
  INFO("Version 0x%02x, max lvte 0x%02x, eoib_supppression_support %x\n",
      la->version.version, la->version.max_lvt_entry,
      la->version.eoi_broadcast_suppression);
  apic_lvt_print();
}

void apic_setup(void) {
  max_phyaddr = cpuid_get_maxphyaddr(); // Physical address space
  uint64_t max_address = ((uint64_t)1 << max_phyaddr);
  uint64_t max_address_mask = (max_address - 1);
  apic_base.raw = msr_read(MSR_ADDRESS_IA32_APIC_BASE);
  if (!apic_base.global_enable) {
    ERROR("APIC disabled\n");
  }
  la = (struct local_apic *)((apic_base.apic_base & max_address_mask) << 12);
  max_lvt_entry = la->version.max_lvt_entry + 1; // Local APIC Version reg doc
  apic_mode = MODE_APIC;
  if (cpuid_is_x2APIC_supported()) {
    INFO("x2APIC mode supported\n");
    if (apic_base.x2APIC_enable) {
      INFO("x2APIC mode enabled\n");
      apic_mode = MODE_X2APIC;
    }
  }
  switch (apic_mode) {
    case MODE_APIC:
      // apic_print();
      break;
    case MODE_X2APIC:
      // x2apic_print();
      break;
    default:
      ERROR("unknow APIC mode\n");
  }
};
