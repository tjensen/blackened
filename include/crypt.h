/*
 * crypt.h: header for crypt.c 
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 *
 * @(#)$Id: crypt.h,v 1.1.1.1 1999/07/16 21:21:13 toast Exp $
 */

#ifndef _CRYPT_H_
#define _CRYPT_H_

extern	char	*crypt_msg();
extern	void	encrypt_cmd _((char *, char *, char *));
extern	char	*is_crypted();

#define CRYPT_HEADER ""
#define CRYPT_HEADER_LEN 5

#endif /* _CRYPT_H_ */
