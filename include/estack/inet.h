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

#ifndef HAVE_GENERIC_SYS
#ifdef HAVE_WINSOCK_H
#include <WinSock2.h>
#else
#include <arpa/inet.h>
#endif
#else
extern DLL_EXPORT uint16_t htons(uint16_t s);
extern DLL_EXPORT uint32_t htonl(uint32_t l);

extern DLL_EXPORT uint16_t ntohs(uint16_t s);
extern DLL_EXPORT uint32_t ntohl(uint32_t l);
#endif

#endif // !__INET_H__
