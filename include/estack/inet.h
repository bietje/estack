/*
 * E/STACK inet header
 *
 * Author: Michel Megens
 * Date: 03/12/2017
 * Email: dev@bietje.net
 */

#ifndef __INET_H__
#define __INET_H__

#include <stdint.h>
#include "config.h"

#include <estack/compiler.h>

#ifndef CONFIG_NO_SYS
#ifdef HAVE_WINSOCK_H
#include <WinSock2.h>
#include <inaddr.h>
#include <in6addr.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#endif

#else /* NO_SYS */
extern DLL_EXPORT uint16_t htons(uint16_t s);
extern DLL_EXPORT uint32_t htonl(uint32_t l);

extern DLL_EXPORT uint16_t ntohs(uint16_t s);
extern DLL_EXPORT uint32_t ntohl(uint32_t l);
#endif

#endif // !__INET_H__
