/*
 * E/STACK inet header
 *
 * Author: Michel Megens
 * Date: 03/12/2017
 * Email: dev@bietje.net
 */

#ifndef __INET_H__
#define __INET_H__

#include "config.h"

#ifdef HAVE_WINSOCK_H
#include <WinSock2.h>
#else
#include <arpa/inet.h>
#endif

#endif // !__INET_H__
