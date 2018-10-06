/*
 * dcc.h: Things dealing client to client connections. 
 *
 * Written By Troy Rollo <troy@plod.cbme.unsw.oz.au> 
 *
 * Copyright(c) 1991 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 *
 * @(#)$Id: dcc.h,v 1.1.1.1 1999/07/16 21:21:13 toast Exp $
 */

/*
 * this file must be included after irc.h as i needed <sys/types.h>
 * <netinet/in.h> and <apra/inet.h>
 */

#ifndef _DCC_H_
#define _DCC_H_

#define DCC_CHAT	((unsigned) 0x0001)
#define DCC_FILEOFFER	((unsigned) 0x0002)
#define DCC_FILEREAD	((unsigned) 0x0003)
#define DCC_TALK	((unsigned) 0x0004)
#define DCC_SUMMON	((unsigned) 0x0005)
#define	DCC_RAW_LISTEN	((unsigned) 0x0006)
#define	DCC_RAW		((unsigned) 0x0007)
#define DCC_TYPES	((unsigned) 0x000f)

#define DCC_WAIT	((unsigned) 0x0010)
#define DCC_ACTIVE	((unsigned) 0x0020)
#define DCC_OFFER	((unsigned) 0x0040)
#define DCC_DELETE	((unsigned) 0x0080)
#define DCC_TWOCLIENTS	((unsigned) 0x0100)
#define DCC_STATES	((unsigned) 0xfff0)

typedef	struct	DCC_struct
{
	unsigned	flags;
	int	read;
	int	write;
	int	file;
	int	filesize;
	char	*description;
	char	*user;
	char	*othername;
	struct	DCC_struct	*next;
	struct	in_addr	remote;
	u_short	remport;
	long	bytes_read;
	time_t	bytes_sent;
	time_t	lasttime;
	time_t	starttime;
	char	*buffer;
	char	talkchars[3];
}	DCC_list;

#define DCC_TALK_CHECK 0
#define DCC_TALK_INVITE 1
#define DCC_TALK_ANNOUNCE 2
#define DCC_TALK_DELETE_LOCAL 3
#define DCC_TALK_DELETE_REMOTE 4
#define DCC_TALK_SUMMON 5
#define DCC_TALK_DELETE_SUMMON 6

extern	DCC_list	*dcc_searchlist();
extern	void	dcc_erase();
extern	void	register_dcc_offer();
extern	void	process_dcc();
extern	void	dcc_list();
extern	void	dcc_chat_transmit();
extern	void	dcc_message_transmit();
extern	int	send_talk_control();
extern	void	close_all_dcc();
extern	void	set_dcc_bits();
extern	void	dcc_check();

#endif /* _DCC_H_ */
