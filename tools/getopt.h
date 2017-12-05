/*
 * E/STACK byte order tool
 *
 * Author: Michel Megens
 * Date: 28/11/2017
 * Email: dev@bietje.net
 */

#ifndef __GETOPT_H__
#define __GETOPT_H__

extern int getopt(int nargc, char * const nargv[], const char *ostr);
extern char *optarg; 
extern int optind, opterr, optopt;

#endif // !__GETOPT_H__
