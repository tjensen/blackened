/*
 * parse.h
 *
 * written by matthew green
 * copyright (c) 1993
 *
 * @(#)$Id: parse.h,v 1.2 1999/07/26 03:15:38 toast Exp $
 */

#ifndef __parse_h_
# define __parse_h_

typedef struct auto_who
{
	char	*channel;
	struct auto_who *next;
} AutoWho;

void	remove_autowho(char *);

extern	char	*PasteArgs();
extern	void	parse_server _((char *));

extern	char	*FromUserHost;

extern	int	doing_privmsg;

#endif /* __parse_h_ */
