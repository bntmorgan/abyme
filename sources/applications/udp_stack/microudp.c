#include "microudp.h"
#include "stdio.h"

uint8_t my_mac[6];
uint32_t my_ip;

/* ARP cache - one entry only */
uint8_t cached_mac[6];
uint32_t cached_ip;

void microudp_start(uint8_t *macaddr, uint32_t ip)
{
	int i;
	for(i=0;i<6;i++)
		my_mac[i] = macaddr[i];
	my_ip = ip;

	cached_ip = 0;
	for(i=0;i<6;i++)
		cached_mac[i] = 0;
	INFO("LOOOOOOOOOL\n");
}
