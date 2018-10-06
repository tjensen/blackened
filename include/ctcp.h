/*
 * ctcp.h: header file for ctcp.c
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 *
 * @(#)$Id: ctcp.h,v 1.1.1.1 1999/07/16 21:21:13 toast Exp $
 */

#ifndef _CTCP_H_
#define _CTCP_H_

/*
 * ctcp_entry: the format for each ctcp function.   note that the function
 * described takes 4 parameters, a pointer to the ctcp entry, who the message
 * was from, who the message was to (nickname, channel, etc), and the rest of
 * the ctcp message.  it can return null, or it can return a malloced string
 * that will be inserted into the oringal message at the point of the ctcp.
 * if null is returned, nothing is added to the original message
 */
typedef	struct
{
	char	*name;                 /* name of ctcp datatag */
	char	*desc;                 /* description returned by ctcp clientinfo */
	int	flag;
	char	*(*func)();           /* function that does the dirty deed */
}	CtcpEntry;

#define CTCP_DELIM_CHAR '\001'
#define CTCP_DELIM_STR "\001"
#define CTCP_QUOTE_CHAR '\\'
#define CTCP_QUOTE_STR "\\"

#define CTCP_QUOTE_EM "\n\r\001\\"

#define CTCP_PRIVMSG 0
#define CTCP_NOTICE 1

#define	CTCP_SED 0
#define CTCP_VERSION 1
#define CTCP_CLIENTINFO 2
#define	CTCP_USERINFO 3
#define	CTCP_ERRMSG 4
#define	CTCP_FINGER 5
#define	CTCP_TIME 6
#define CTCP_ACTION 7
#define	CTCP_DCC_CHAT 8
#define	CTCP_UCT 9
#define CTCP_PING 10
#define CTCP_ECHO 11
#define	NUMBER_OF_CTCPS 12

extern	char	*ctcp_type[];
extern	int	sed;

extern	char	*do_ctcp();
extern	char	*ctcp_quote_it();
extern	char	*ctcp_unquote_it();
extern	char	*do_notice_ctcp();
extern	int	in_ctcp();
#ifdef USE_STDARG_H
extern        void    send_ctcp_reply(char *, char *, char *, ...);
extern        void    send_ctcp(char *, char *, char *, char *, ...);
#else
extern	void	send_ctcp_reply();
extern	void	send_ctcp();
#endif

#endif /* _CTCP_H_ */
