/*
 * cignore.h: header for cignore.c 
 *
 * Written By Timothy Jensen
 *
 * Copyright(c) 1998
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 */

#ifndef _CIGNORE_H_
#define _CIGNORE_H_

/* declared in cignore.c */
extern	int	is_cignored();
extern	void	cignore _((char *, char *, char *));

/* Type of ignored nicks */
#define CIGNORE_MSGS	0x0001
#define CIGNORE_NOTICES	0x0002
#define CIGNORE_CTCPS	0x0004
#define CIGNORE_JOINS	0x0008
#define CIGNORE_PARTS	0x0010
#define CIGNORE_KICKS	0x0020
#define CIGNORE_MODES	0x0040
#define CIGNORE_TOPICS	0x0080
#define CIGNORE_CRAP	0x0100
#define CIGNORE_ALL (CIGNORE_MSGS | CIGNORE_MSGS | CIGNORE_NOTICES \
		| CIGNORE_CTCPS | CIGNORE_JOINS | CIGNORE_PARTS | \
		CIGNORE_KICKS | CIGNORE_MODES | CIGNORE_TOPICS | \
		CIGNORE_CRAP)

#define CIGNORED 1

#endif /* _CIGNORE_H_ */
