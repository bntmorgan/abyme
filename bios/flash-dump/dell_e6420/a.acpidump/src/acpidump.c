/* Copyright (c) 2011 Gianni Tedesco */
/* According to ACPI v4.0 spec */

#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <ctype.h>

#define PACKED __attribute__((packed))

#define DEV_MEM_NODE "/dev/mem"

/* 5.2.5.1, part two, BIOS read-only memory space */
/* FIXME: scan first 1KB of EBDA aswell */
#define ROM_BIOS_BASE 0xE0000
#define ROM_BIOS_SIZE 0x20000

/* more than enough */

struct acpi_hdr {
	uint8_t acpi_sig[4];
	uint32_t acpi_len;
	uint8_t acpi_rev;
	uint8_t acpi_csum;
	uint8_t acpi_oem_id[6];
	uint8_t acpi_oem_tbl_id[8];
	uint8_t acpi_oem_rev[4];
	uint8_t acpi_creator_id[4];
	uint8_t acpi_creator_rev[4];
} PACKED;

#define ACPI_RSDP_CS_LENGTH   0x14
#define ACPI_RSDP_XCS_LENGTH  0x24

struct acpi_rsdp {
#define RSDP_SIG "RSD PTR "
	uint8_t rsdp_sig[8];
	uint8_t rsdp_csum;
	uint8_t rsdp_oem_id[6];
	uint8_t rsdp_rev;
	uint32_t rsdp_rsdt_ptr;
	uint32_t rsdp_len;
	uint64_t rsdp_xsdt_ptr;
	uint8_t rsdp_ext_csum;
	uint8_t rsdp__reserved[3];
} PACKED;

struct acpi_info {
	uint64_t xsdt;
	uint32_t xsdt_len;
	uint32_t rsdt;
	uint32_t rsdt_len;
	uint32_t rsdp;
	uint32_t rsdp_len;
	uint8_t got:1, rev1:1;
};

static void hex_dumpf(FILE *f, const uint8_t *tmp, size_t len, size_t llen)
{
	size_t i, j;
	size_t line;

	if ( NULL == f || 0 == len )
		return;

	for(j = 0; j < len; j += line, tmp += line) {
		if ( j + llen > len ) {
			line = len - j;
		}else{
			line = llen;
		}

		fprintf(f, " %05zx : ", j);

		for(i = 0; i < line; i++) {
			if ( isprint(tmp[i]) ) {
				fprintf(f, "%c", tmp[i]);
			}else{
				fprintf(f, ".");
			}
		}

		for(; i < llen; i++)
			fprintf(f, " ");

		for(i = 0; i < line; i++)
			fprintf(f, " %02x", tmp[i]);

		fprintf(f, "\n");
	}
	fprintf(f, "\n");
}

static void hex_dump(const uint8_t *ptr, size_t len, size_t llen)
{
	hex_dumpf(stdout, ptr, len, llen);
}
static int acpi_csum(const uint8_t *ptr, size_t len)
{
	uint8_t sum;
	size_t i;

	for(sum = i = 0; i < len; i++, ptr++) {
		sum += *ptr;
	}

	return (0 == sum);
}

static int get_tbl_len(int fd, uint64_t base, uint32_t *sz)
{
	size_t pgsz = sysconf(_SC_PAGESIZE);
	uint64_t pgoff = base % pgsz;
	struct acpi_hdr *hdr;
	uint8_t *map;

	/* map 2 pages incase it straddles page boundary */
	map = mmap(NULL, pgsz * 2, PROT_READ,
			MAP_SHARED, fd, base - pgoff);
	if ( MAP_FAILED == map )
		return 0;

	hdr = (struct acpi_hdr *)(map + pgoff);
	*sz = hdr->acpi_len;

	munmap(map, pgsz * 2);
	return 1;
}

static int acpi_rom_scan(int fd, struct acpi_info *info)
{
	uint8_t *map, *ptr;
	int ret = 0;

	if ( info->got )
		return 1;

	printf("Scanning ROM BIOS area for RSDP\n");

	map = mmap(NULL, ROM_BIOS_SIZE, PROT_READ,
			MAP_SHARED, fd, ROM_BIOS_BASE);
	if ( MAP_FAILED == map )
		return 0;

	for(ptr = map; ptr < (map + ROM_BIOS_SIZE); ptr += 16) {
		struct acpi_rsdp *rsdp;
		size_t len;
		int rev1;

		if ( memcmp(ptr, RSDP_SIG, sizeof(rsdp->rsdp_sig)) )
			continue;

		rsdp = (struct acpi_rsdp *)ptr;

		if ( rsdp->rsdp_rev < 2) {
			len = ACPI_RSDP_CS_LENGTH;
			printf("rev1 candidate\n");
			rev1 = 1;
		}else{
			len = ACPI_RSDP_XCS_LENGTH;
			printf("rev2+ candidate\n");
			rev1 = 0;
		}

		if ( !acpi_csum(ptr, len) ) {
			printf("csum fail\n");
			continue;
		}

		printf("csum OK\n");
		info->rsdp = (ptr - map);
		info->rsdp_len = rsdp->rsdp_len;
		info->xsdt = rsdp->rsdp_xsdt_ptr;
		if ( !get_tbl_len(fd, info->xsdt, &info->xsdt_len) )
			break;
		info->rsdt = rsdp->rsdp_rsdt_ptr;
		if ( !get_tbl_len(fd, info->rsdt, &info->rsdt_len) )
			break;
		info->rev1 = rev1;
		info->got = 1;
		ret = 1;

		printf("Got RSDP @ %.8"PRIx32": RSDT=%.8"PRIx32" XSDT=%.16"PRIx64"\n",
			info->rsdp, info->rsdt, info->xsdt);

		break;
	}

	munmap(map, ROM_BIOS_SIZE);
	return ret;
}

static int tbl_to_file(const uint8_t *buf, size_t len, unsigned int idx)
{
	struct acpi_hdr *hdr;
	ssize_t ret;
	char fn[32];
	int fd;

	hdr = (struct acpi_hdr *)buf;

	snprintf(fn, sizeof(fn), "%.4s.acpi-tbl-%u", hdr->acpi_sig, idx);
	fd = open(fn, O_WRONLY|O_CREAT|O_TRUNC, 0600);
	if ( fd < 0 )
		return 0;

	ret = write(fd, buf, len);
	if ( ret < 0 || (size_t)ret < len ) {
		close(fd);
		return 0;
	}

	close(fd);
	return 1;
}

static int acpi_rip_table(int fd, uint64_t base)
{
	size_t pgsz = sysconf(_SC_PAGESIZE);
	uint64_t pgoff = base % pgsz;
	static unsigned int i;
	struct acpi_hdr *hdr;
	uint8_t *map;
	uint32_t len;

	if ( !get_tbl_len(fd, base, &len) )
		return 0;

	map = mmap(NULL, len + pgoff, PROT_READ,
			MAP_SHARED, fd, base - pgoff);
	if ( MAP_FAILED == map )
		return 0;

	hdr = (struct acpi_hdr *)(map + pgoff);
	printf(" - %.4s: %5"PRIu32" byte table @ %.16"PRIx64"\n",
		hdr->acpi_sig, len, base);
	//hex_dump(map + pgoff, len, 16);

	tbl_to_file(map + pgoff, len, i++);
	munmap(map, len + pgoff);
	return 1;
}

static int acpi_rip_sdt(int fd, uint64_t base, uint32_t len, int extended)
{
	size_t pgsz = sysconf(_SC_PAGESIZE);
	uint64_t pgoff = base % pgsz;
	struct acpi_hdr *hdr;
	uint32_t num_ent, i;
	size_t ent_sz;
	uint8_t *map, *ptr;

	map = mmap(NULL, len + pgoff, PROT_READ,
			MAP_SHARED, fd, base - pgoff);
	if ( MAP_FAILED == map )
		return 0;

	hdr = (struct acpi_hdr *)(map + pgoff);
	ptr = map + pgoff + sizeof(*hdr);
	ent_sz = (extended) ? sizeof(uint64_t) : sizeof(uint32_t);
	num_ent = (len - sizeof(*hdr)) / ent_sz;

	printf("Scanning %"PRIu32" secondary tables in %.4s\n",
		num_ent, hdr->acpi_sig);
	for (i = 0; i < num_ent; i++, ptr += ent_sz) {
		uint64_t loc;
		loc = (extended) ? 
			*(uint64_t *)ptr : *(uint32_t *)ptr;
		acpi_rip_table(fd, loc);
	}

	munmap(map, len + pgoff);
	return 1;
}

static int acpi_dump(void)
{
	static struct acpi_info info;
	int fd, rc, ret;

	fd = open(DEV_MEM_NODE, O_RDONLY);
	if ( fd < 0 )
		return 0;

	if ( !acpi_rom_scan(fd, &info) )
		goto out;

	/* TODO: EFI scan if rombios scan failes */

	if ( info.rev1 )
		rc = acpi_rip_sdt(fd, info.rsdt, info.rsdt_len, 0);
	else
		rc = acpi_rip_sdt(fd, info.xsdt, info.xsdt_len, 1);

	if ( !rc )
		goto out;

	ret = 1;
out:
	close(fd);
	return ret;
}

int main(int argc, char **argv)
{
	if ( !acpi_dump() )
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
