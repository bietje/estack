/*
 * E/STACK error codes
 *
 * Author: Michel Megens
 * Date: 04/12/2017
 * Email: dev@bietje.net
 */

#ifndef __ESTACK_ERROR_H__
#define __ESTACK_ERROR_H__

typedef enum error_code {
	EOK = 0,
	EDRPPPED,
	EARRIVED,
	EINVALID,
} error_t;

#endif // !__ESTACK_ERROR_H__
