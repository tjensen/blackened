/*
 * whois.h: header for whois.c 
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 *
 * @(#)$Id: whois.h,v 1.3 2001/12/15 01:59:57 toast Exp $
 */

#ifndef __whois_h_
# define __whois_h_

# ifdef USE_STDARG_H
extern	void	add_to_whois_queue(char *, void (*)(), char *, ...);
extern	void	typed_add_to_whois_queue _((int, char *, void (*)(), char *, va_list));
# else
extern	void	add_to_whois_queue();
extern  void	typed_add_to_whois_queue();
# endif
extern	void	add_ison_to_whois();
extern void add_userhost_to_whois(char *nick, void (*func)());
extern	void	whois_name();
extern	void	whowas_name();
extern	void	whois_channels();
extern	void	whois_server();
extern	void	whois_oper();
extern	void	whois_admin();
extern	void	whois_lastcom();
extern	void	whois_nickname();
extern	void	whois_ignore_msgs();
extern	void	whois_ignore_notices();
extern	void	whois_ignore_walls();
extern	void	whois_ignore_invites();
extern	void	whois_join();
extern	void	whois_privmsg();
extern	void	whois_notify();
extern	void	whois_new_wallops();
extern	void	clean_whois_queue();
extern	void	set_beep_on_msg();
extern	int	beep_on_level;
extern  void    userhost_cmd_returned();
extern	void	user_is_away();
extern	void	userhost_returned();
extern	void	ison_returned();
extern	void	whois_chop();
extern	void	end_of_whois();
extern	void	whoreply();
extern	void	convert_to_whois();
extern	void	no_such_nickname();
extern	void	show_whois_queue();

extern	char	*redirect_format;

#define	WHOIS_WHOIS	0
#define	WHOIS_ISON	1
#define	WHOIS_USERHOST	2

#define	USERHOST_USERHOST ((void (*)()) 1)

#endif /* __whois_h_ */
