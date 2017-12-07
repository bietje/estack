/*
 * E/STACK Ethernet address utilities
 *
 * Author: Michel Megens
 * Date: 07/12/2017
 * Email: dev@bietje.net
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include <estack/estack.h>
#include <estack/ethernet.h>

bool ethernet_addr_is_broadcast(const uint8_t *addr)
{
	int i;

	i = 0;
	while (i < ETHERNET_MAC_LENGTH && *(addr + i) == 0)
		i++;

	return i < 5 ? false : true;
}
