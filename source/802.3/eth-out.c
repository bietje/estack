/*
* E/Stack ethernet input
*
* Author: Michel Megens
* Date: 03/12/2017
* Email: dev@bietje.net
*/

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include <estack/estack.h>
#include <estack/netdev.h>
#include <estack/netbuf.h>
#include <estack/ethernet.h>
#include <estack/inet.h>

void ethernet_output(struct netbuf *nb)
{
	netbuf_set_flag(nb, NBUF_DROPPED);
}
