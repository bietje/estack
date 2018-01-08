/**
 * E/STACK - UDP socket test
 *
 * Author: Michel Megens
 * Email:  dev@bietje.net
 * Date:   08/01/2018
 */

#include <stdlib.h>
#include <stdio.h>
#include <estack.h>

#include <estack/socket.h>

int main(int argc, char **argv)
{
	estack_init(stdout);
	estack_destroy();

	return -EXIT_SUCCESS;
}
