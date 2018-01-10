/*
 * E/STACK initialisation
 *
 * Author: Michel Megens
 * Date: 10/01/2018
 * Email: dev@bietje.net
 */

#include <stdlib.h>
#include <stdio.h>
#include <estack.h>

void estack_init(const FILE *logfile)
{
	log_init(logfile);
	route4_init();
	devcore_init();
	socket_api_init();
}

void estack_destroy(void)
{
	socket_api_destroy();
	devcore_destroy();
	route4_destroy();
}
