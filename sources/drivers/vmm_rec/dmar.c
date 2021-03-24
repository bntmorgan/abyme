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

#include "dmar.h"
#include "efiw.h"
#include "stdio.h"
#include "string.h"
#include "pci.h"

// See linux-x.y.z/Documentation/Intel-IOMMU.txt to get DMAR addresses
// See Ivy Bridge/018s/ReferenceCode/Chipset/SystemAgent/SaInit/Dxe/Vtd.c
//    VtdInit()

uint64_t rcba, mchbar, vtd1, vtd2;

void print_protected_memory(void) {
  uint32_t pmen, plmbase, plmlimit, phmbase, phmlimit;
  // Print the protected low memory base and high registers
  pmen = *(uint32_t*)(vtd1 + PCI_VC0PREMAP_PMEN);
  plmbase = *(uint32_t*)(vtd1 + PCI_VC0PREMAP_PLMBASE);
  plmlimit = *(uint32_t*)(vtd1 + PCI_VC0PREMAP_PLMLIMIT);
  phmbase = *(uint32_t*)(vtd1 + PCI_VC0PREMAP_PLMBASE);
  phmlimit = *(uint32_t*)(vtd1 + PCI_VC0PREMAP_PLMLIMIT);
  INFO("pmen(0x%08x)\n", pmen);
  INFO("plmbase(0x%08x)\n", plmbase);
  INFO("plmlimit(0x%08x)\n", plmlimit);
  INFO("phmbase(0x%08x)\n", phmbase);
  INFO("phmlimit(0x%08x)\n", phmlimit);
}

void dmar_init(void) {
//  uint64_t i, j;
  uint32_t host_bridge = PCI_MAKE_MMCONFIG(0, 0, 0);
  uint32_t lpc = PCI_MAKE_MMCONFIG(0, 31, 0);
//  union vtd_rtaddr *rtaddr;
//  union vtd_gcmd *gcmd;
//  union vtd_gsts *gsts;
//  union vtd_fsts *fsts;
//  union vtd_frcdl *frcdl;
//  union vtd_frcdh *frcdh;

  INFO("INIT DMAR for Intel 4th Gen core family\n");

//  struct dmar_pages *dp = efi_allocate_pages(
//      sizeof(struct dmar_pages) >> 12);

  INFO("Host bridge VID 0x%x\n", pci_mm_readw(host_bridge, 0));
  INFO("Host bridge DID 0x%x\n", pci_mm_readw(host_bridge, 2));

  // Get the address of IOMMUs
  rcba = pci_mm_readd(lpc, PCI_LPC_RCBA) & ~(uint64_t)0x1;
  INFO("RCBA(@0x%016X)\n", rcba);

  mchbar = pci_mm_readq(host_bridge, PCI_HOST_BRIDGE_MCHBAR) &
      ~(uint64_t)0x7fff;
  INFO("MCHBAR(@0x%016X)\n", mchbar);

  vtd1 = pci_bar_readd(mchbar, PCI_MCHBAR_VTD1) & ~0xfff;
  INFO("VTD1(@0x%016X)\n", vtd1);
  // dmar_dump_iommu(vtd1);
  vtd2 = pci_bar_readd(mchbar, PCI_MCHBAR_VTD2) & ~0xfff;
  INFO("VTD2(@0x%016X)\n", vtd2);
  // dmar_dump_iommu(vtd2);

  // Initialisation of translation tables
//  memset(dp, 0, sizeof(struct dmar_pages));

  // Maps the PCI ids to the global pml4
//  for (i = 0; i < 256; i++) {
//    dp->root_table[i].p = 1;
//    dp->root_table[i].ctp = ((uint64_t)&dp->context_table[i][0]) >> 12;
//    for(j = 0; j < 256; j++) {
//      dp->context_table[i][j].slptptr = ((uint64_t)&dp->pml4[0]) >> 12;
//      dp->context_table[i][j].p = 1;
//    }
//  }

  // Id mapping for everyone
//  for(i = 0; i < 512; i++) {
//    dp->pml4[i].a = ((uint64_t)&dp->pdpt[i][0]) >> 12;
//    dp->pml4[i].r = 1;
//    dp->pml4[i].w = 1;
//    dp->pml4[i].x = 1;
//    for(j = 0; j < 512; j++) {
//      dp->pdpt[i][j].a = ((i << 39) | (j << 30)) >> 12;
//      dp->pdpt[i][j].r = 1;
//      dp->pdpt[i][j].w = 1;
//      dp->pdpt[i][j].x = 1;
//      dp->pdpt[i][j].ps = 1;
//    }
//  }

  // Get the rtaddr register
//  rtaddr = (union vtd_rtaddr*)(vtd1 + PCI_VC0PREMAP_RTADDR);
//  rtaddr->rta = ((uint64_t)&dp->root_table[0]) >> 12;
//  dmar_print_vtd_rtaddr(*rtaddr);

  // Starts DMAR MAN !
//  gcmd = (union vtd_gcmd*)(vtd1 + PCI_VC0PREMAP_GCMD);
//  gcmd->te = 1;
//  dmar_print_vtd_gcmd(*gcmd);
//
//  gsts = (union vtd_gsts*)(vtd1 + PCI_VC0PREMAP_GSTS);
//  dmar_print_vtd_gsts(*gsts);
//
//  fsts = (union vtd_fsts*)(vtd1 + PCI_VC0PREMAP_FSTS);
//  dmar_print_vtd_fsts(*fsts);
//
//  // Print faults
//  uint64_t fl = pci_bar_readq(vtd1, PCI_VC0PREMAP_FRCDL);
//  INFO("FRCDL(0x%016X)\n", fl);
//  uint64_t fh = pci_bar_readq(vtd1, PCI_VC0PREMAP_FRCDH);
//  INFO("FRCDH(0x%016X)\n", fh);
//  frcdl = (union vtd_frcdl*)(vtd1 + PCI_VC0PREMAP_FRCDL);
//  dmar_print_vtd_frcdl(*frcdl);
//  frcdh = (union vtd_frcdh*)(vtd1 + PCI_VC0PREMAP_FRCDH);
//  dmar_print_vtd_frcdh(*frcdh);
}

void dmar_dump_iommu(uint64_t addr) {
  INFO("Dumping IOMMU @0x(%016X)\n", addr);
  printk("VER(0x%x)\n", pci_bar_readd(addr, PCI_VC0PREMAP_VER));
  dmar_print_vtd_cap((union vtd_cap)pci_bar_readq(addr, PCI_VC0PREMAP_CAP));
  dmar_print_vtd_ecap((union vtd_ecap)pci_bar_readq(addr, PCI_VC0PREMAP_ECAP));
  dmar_print_vtd_gcmd((union vtd_gcmd)pci_bar_readd(addr, PCI_VC0PREMAP_GCMD));
  dmar_print_vtd_gsts((union vtd_gsts)pci_bar_readd(addr, PCI_VC0PREMAP_GSTS));
  dmar_print_vtd_rtaddr((union vtd_rtaddr)pci_bar_readq(addr, PCI_VC0PREMAP_RTADDR));
  dmar_print_vtd_ccmd((union vtd_ccmd)pci_bar_readq(addr, PCI_VC0PREMAP_CCMD));
  dmar_print_vtd_fsts((union vtd_fsts)pci_bar_readd(addr, PCI_VC0PREMAP_FSTS));
  dmar_print_vtd_fectl((union vtd_fectl)pci_bar_readd(addr, PCI_VC0PREMAP_FECTL));
  printk("FEDATA(0x%x)\n", pci_bar_readd(addr, PCI_VC0PREMAP_FEDATA));
  printk("FEADDR(0x%x)\n", pci_bar_readd(addr, PCI_VC0PREMAP_FEADDR));
  printk("FEUADDR(0x%x)\n", pci_bar_readd(addr, PCI_VC0PREMAP_FEUADDR));
  printk("AFLOG(0x%X)\n", pci_bar_readq(addr, PCI_VC0PREMAP_AFLOG));
  printk("PMEN(0x%x)\n", pci_bar_readd(addr, PCI_VC0PREMAP_PMEN));
  printk("PLMBASE(0x%x)\n", pci_bar_readd(addr, PCI_VC0PREMAP_PLMBASE));
  printk("PLMLIMIT(0x%x)\n", pci_bar_readd(addr, PCI_VC0PREMAP_PLMLIMIT));
  printk("PHMBASE(0x%X)\n", pci_bar_readq(addr, PCI_VC0PREMAP_PHMBASE));
  printk("PHMLIMIT(0x%X)\n", pci_bar_readq(addr, PCI_VC0PREMAP_PHMLIMIT));
  printk("IQH(0x%X)\n", pci_bar_readq(addr, PCI_VC0PREMAP_IQH));
  printk("IQT(0x%X)\n", pci_bar_readq(addr, PCI_VC0PREMAP_IQT));
  printk("IQA(0x%X)\n", pci_bar_readq(addr, PCI_VC0PREMAP_IQA));
  printk("ICS(0x%x)\n", pci_bar_readd(addr, PCI_VC0PREMAP_ICS));
  printk("IECTL(0x%x)\n", pci_bar_readd(addr, PCI_VC0PREMAP_IECTL));
  printk("IEDATA(0x%x)\n", pci_bar_readd(addr, PCI_VC0PREMAP_IEDATA));
  printk("IEADDR(0x%x)\n", pci_bar_readd(addr, PCI_VC0PREMAP_IEADDR));
  printk("IEUADDR(0x%x)\n", pci_bar_readd(addr, PCI_VC0PREMAP_IEUADDR));
  printk("IRTA(0x%X)\n", pci_bar_readq(addr, PCI_VC0PREMAP_IRTA));
  printk("IVA(0x%X)\n", pci_bar_readq(addr, PCI_VC0PREMAP_IVA));
  printk("IOTLB(0x%X)\n", pci_bar_readq(addr, PCI_VC0PREMAP_IOTLB));
  printk("FRCDL(0x%X)\n", pci_bar_readq(addr, PCI_VC0PREMAP_FRCDL));
  printk("FRCDH(0x%X)\n", pci_bar_readq(addr, PCI_VC0PREMAP_FRCDH));
}

void dmar_print_vtd_cap(union vtd_cap f) {
  printk("CAP(0x%016X)\n", f.raw);
  printk("  nd(0x%x)\n", f.nd);
  printk("  afl(0x%x)\n", f.afl);
  printk("  rwbf(0x%x)\n", f.rwbf);
  printk("  plmr(0x%x)\n", f.plmr);
  printk("  phmr(0x%x)\n", f.phmr);
  printk("  cm(0x%x)\n", f.cm);
  printk("  sagaw(0x%x)\n", f.sagaw);
  printk("  mgaw(0x%x)\n", f.mgaw);
  printk("  zlr(0x%x)\n", f.zlr);
  printk("  isoch(0x%x)\n", f.isoch);
  printk("  fro(0x%x)\n", f.fro);
  printk("  sps(0x%x)\n", f.sps);
  printk("  psi(0x%x)\n", f.psi);
  printk("  nfr(0x%x)\n", f.nfr);
  printk("  mamv(0x%x)\n", f.mamv);
  printk("  dwd(0x%x)\n", f.dwd);
  printk("  drd(0x%x)\n", f.drd);
}

void dmar_print_vtd_ecap(union vtd_ecap f) {
  printk("ECAP(0x%016X)\n", f.raw);
  printk("  c(0x%x)\n", f.c);
  printk("  qi(0x%x)\n", f.qi);
  printk("  di(0x%x)\n", f.di);
  printk("  ir(0x%x)\n", f.ir);
  printk("  eim(0x%x)\n", f.eim);
  printk("  ch(0x%x)\n", f.ch);
  printk("  pt(0x%x)\n", f.pt);
  printk("  sc(0x%x)\n", f.sc);
  printk("  iro(0x%x)\n", f.iro);
  printk("  mhmv(0x%x)\n", f.mhmv);
}

void dmar_print_vtd_gcmd(union vtd_gcmd f) {
  printk("GCMD(0x%016X)\n", f.raw);
  printk("  cfi(0x%x)\n", f.cfi);
  printk("  sirtp(0x%x)\n", f.sirtp);
  printk("  ire(0x%x)\n", f.ire);
  printk("  qie(0x%x)\n", f.qie);
  printk("  wbf(0x%x)\n", f.wbf);
  printk("  eafl(0x%x)\n", f.eafl);
  printk("  sfl(0x%x)\n", f.sfl);
  printk("  srtp(0x%x)\n", f.srtp);
  printk("  te(0x%x)\n", f.te);
}

void dmar_print_vtd_gsts(union vtd_gsts f) {
  printk("GSTS(0x%016X)\n", f.raw);
  printk("  cfis(0x%x)\n", f.cfis);
  printk("  irtps(0x%x)\n", f.irtps);
  printk("  ires(0x%x)\n", f.ires);
  printk("  qies(0x%x)\n", f.qies);
  printk("  wbfs(0x%x)\n", f.wbfs);
  printk("  afls(0x%x)\n", f.afls);
  printk("  fls(0x%x)\n", f.fls);
  printk("  rtps(0x%x)\n", f.rtps);
  printk("  tes(0x%x)\n", f.tes);
}

void dmar_print_vtd_ccmd(union vtd_ccmd f) {
  printk("CCMD(0x%016X)\n", f.raw);
  printk("  did(0x%x)\n", f.did);
  printk("  sid(0x%x)\n", f.sid);
  printk("  fm(0x%x)\n", f.fm);
  printk("  caig(0x%x)\n", f.caig);
  printk("  cirg(0x%x)\n", f.cirg);
  printk("  icc(0x%x)\n", f.icc);
}

void dmar_print_vtd_fsts(union vtd_fsts f) {
  printk("FSTS(0x%08x)\n", f.raw);
  printk("  pfo(0x%x)\n", f.pfo);
  printk("  ppf(0x%x)\n", f.ppf);
  printk("  afo(0x%x)\n", f.afo);
  printk("  apf(0x%x)\n", f.apf);
  printk("  iqe(0x%x)\n", f.iqe);
  printk("  ice(0x%x)\n", f.ice);
  printk("  ite(0x%x)\n", f.ite);
  printk("  r0(0x%x)\n", f.r0);
  printk("  fri(0x%x)\n", f.fri);
  printk("  r1(0x%x)\n", f.r1);
}

void dmar_print_vtd_fectl(union vtd_fectl f) {
  printk("FECTL(0x%08x)\n", f.raw);
  printk("  ip(0x%x)\n", f.ip);
  printk("  im(0x%x)\n", f.im);
}

void dmar_print_vtd_frcdl(union vtd_frcdl f) {
  printk("FRCDL(0x%016X)\n", f.raw);
  printk("  fi(0x%x)\n", f.fi);
}

void dmar_print_vtd_frcdh(union vtd_frcdh f) {
  printk("FRCDH(0x%016X)\n", f.raw);
  printk("  sid(0x%x)\n", f.sid);
  printk("  fr(0x%x)\n", f.fr);
  printk("  at(0x%x)\n", f.at);
  printk("  t(0x%x)\n", f.t);
  printk("  f(0x%x)\n", f.f);
}

void dmar_print_vtd_rtaddr(union vtd_rtaddr f) {
  printk("RTADDR(0x%016X)\n", f.raw);
  printk("  rtt(0x%x)\n", f.rtt);
  printk("  rta(0x%x)\n", f.rta);
}

void dmar_print_dmar_sl_pml4_entry(union dmar_sl_pml4_entry e) {
  printk("pml4e(0x%016X)\n", e.raw);
  printk("  r(0x%x)\n", e.r);
  printk("  w(0x%x)\n", e.w);
  printk("  x(0x%x)\n", e.x);
  printk("  a(0x%x)\n", e.a);
}

void dmar_print_dmar_sl_pdpt_entry(union dmar_sl_pdpt_entry e) {
  printk("pdpte(0x%016X)\n", e.raw);
  printk("  r(0x%x)\n", e.r);
  printk("  w(0x%x)\n", e.w);
  printk("  x(0x%x)\n", e.x);
  printk("  emt(0x%x)\n", e.emt);
  printk("  ipat(0x%x)\n", e.ipat);
  printk("  ps(0x%x)\n", e.ps); // Page size
  printk("  snp(0x%x)\n", e.snp);
  printk("  a(0x%x)\n", e.a);
  printk("  mt(0x%x)\n", e.mt);
}

void dmar_print_dmar_root_entry(union dmar_root_entry e) {
  printk("root_entry[0](0x%016X)\n", e.raw[0]);
  printk("root_entry[1](0x%016X)\n", e.raw[1]);
  printk("  p(0x%x)\n", e.p);
  printk("  ctp(0x%x)\n", e.ctp);
}

void dmar_print_dmar_context_entry(union dmar_context_entry e) {
  printk("context_entry[0](0x%016X)\n", e.raw[0]);
  printk("context_entry[1](0x%016X)\n", e.raw[1]);
  printk("  p(0x%x)\n", e.p);
  printk("  fpd(0x%x)\n", e.fpd);
  printk("  t(0x%x)\n", e.t);
  printk("  slptptr(0x%x)\n", e.slptptr);
  printk("  aw(0x%x)\n", e.aw);
  printk("  avail(0x%x)\n", e.avail);
  printk("  did(0x%x)\n", e.did);
}

void dmar_dump(union vtd_rtaddr rta) {
  uint32_t i, j;
  union dmar_root_entry *re;
  union dmar_context_entry *ce;
  for (i = 0; i < 256; i++) {
    re = &((union dmar_root_entry *)rta.raw)[i];
    // print re
    if (re->p) {
      INFO("bus(0x%x), @0x%016X\n", i, re);
      dmar_print_dmar_root_entry(*re);
      for (j = 0; j < 256; j++) {
        ce = &((union dmar_context_entry *)((uint64_t)re->ctp << 12))[j];
        if (ce->p) {
          INFO("dev(0x%x).fun(0x%x), @0x%016X\n", j >> 3, j & 0x7, ce);
          dmar_print_dmar_context_entry(*ce);
        }
      }
    }
  }
}
