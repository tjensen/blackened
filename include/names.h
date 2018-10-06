/*
 * names.h: Header for names.c
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 *
 * @(#)$Id: names.h,v 1.1.1.1 1999/07/16 21:21:14 toast Exp $
 */

#ifndef _NAMES_H_
#define _NAMES_H_

#include "window.h"
#include "irc.h"

/* from names.c - "iklmnpst" */
#define MODE_INVITE	((u_long) 0x0001)
#define MODE_KEY	((u_long) 0x0002)
#define MODE_LIMIT	((u_long) 0x0004)
#define MODE_MODERATED	((u_long) 0x0008)
#define MODE_MSGS	((u_long) 0x0010)
#define MODE_PRIVATE	((u_long) 0x0020)
#define MODE_SECRET	((u_long) 0x0040)
#define MODE_TOPIC	((u_long) 0x0080)

/* ChannelList: structure for the list of channels you are current on */
typedef	struct	channel_stru
{
	struct	channel_stru	*next;	/* pointer to next channel entry */
	char	*channel;		/* channel name */
	int	server;			/* server index for this channel */
	u_long	mode;			/* Current mode settings for channel */
	u_long	i_mode;			/* channel mode for cached string */
	char	*s_mode;		/* cached string version of modes */
	int	limit;			/* max users for the channel */
	int	i_limit;		/* max users in cached string */
	char	*key;			/* key for this channel */
	char	chop;			/* true if you are chop */
	char	voice;			/* true if you are voiced */
	Window	*window;		/* the window that the channel is "on" */
	NickList	*nicks;		/* pointer to list of nicks on channel */
}	ChannelList;

extern	int	is_channel_mode();
extern	int	is_chanop();
extern	ChannelList	*lookup_channel();
extern	char	*get_channel_mode();
extern	void	add_channel();
extern	void	add_to_channel _((char *, char *, int, int, int, char *));
extern	void	remove_channel();
extern	void	remove_from_channel();
extern	int	is_on_channel();
extern	void	list_channels();
extern	void	reconnect_all_channels();
extern	void	switch_channels();
extern	char	*what_channel();
extern	char	*walk_channels();
extern	char	*real_channel();
extern	char	*old_current_channel();
extern	void	rename_nick();
extern	void	update_channel_mode();
extern	void	set_channel_window();
extern	char	*create_channel_list();
extern	int	get_channel_oper();
extern	int	get_channel_voice();
extern	void	channel_server_delete();
extern	void	change_server_channels();
extern	void	clear_channel_list();
extern	char	*channel_key();
extern	void	set_waiting_channel _((int));
extern	void	remove_from_mode_list _((char *));

#endif /* _NAMES_H_ */
