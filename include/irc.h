/*
 * irc.h: header file for all of ircII! 
 *
 * Written By Michael Sandrof
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 *
 * @(#)$Id: irc.h,v 1.5.2.1 2002/03/07 19:35:15 toast Exp $
 */

#ifndef __irc_h
#define __irc_h

#define IRCII_COMMENT   "http://www.blackened.com/blackened/"

#define IRCRC_NAME "/.ircrc"

/*
 * Here you can set the in-line quote character, normally backslash, to
 * whatever you want.  Note that we use two backslashes since a backslash is
 * also C's quote character.  You do not need two of any other character.
 */
#define QUOTE_CHAR '\\'

#if defined(ISC30)		/* for some reason it doesn't get defined */
# define _POSIX_SOURCE
#endif /* ISC30 */

#include "defs.h"
#include "config.h"
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#ifndef WINS
#include <netinet/in.h>
#else
#include <sys/twg_config.h>
#include <sys/in.h>
#undef server
#endif
#include <arpa/inet.h>
#include <signal.h>
#include <sys/param.h>

#ifdef TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif /* HAVE_SYS_TIME_H */
#endif /* TIME_WITH_SYS_TIME */

#ifdef HAVE_SYS_FCNTL_H
# include <sys/fcntl.h>
#else
# ifdef HAVE_FCNTL_H
#  include <fcntl.h>
# endif /* HAVE_FCNTL_H */
#endif /* HAVE_SYS_FCNTL_H */

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#ifdef HAVE_SYS_FILE_H
# include <sys/file.h>
#endif

#ifdef HAVE_NETDB_H
# include <netdb.h>
#endif

#if defined(__QNX__)
# include <sys/select.h>
# include <unix.h>
# define BROKEN_GETPGRP 1
# define USE_STDARG_H
#endif

#ifdef HAVE_STDARG_H
# include <stdarg.h>
#endif

#include "irc_std.h"
#include "debug.h"

/* these define what characters do, inverse, underline, bold and all off */
#define REV_TOG		'\026'		/* ^V */
#define UND_TOG		'\037'		/* ^_ */
#define BOLD_TOG	'\002'		/* ^B */
#define BLINK_TOG	'\006'		/* ^F */
#define ALL_OFF		'\017'		/* ^O */
#define ESCAPE		'\033'		/* ^[ */

#define IRCD_BUFFER_SIZE	1024
#define BIG_BUFFER_SIZE		(IRCD_BUFFER_SIZE * 2)

#ifndef INPUT_BUFFER_SIZE
/*
#define INPUT_BUFFER_SIZE	(IRCD_BUFFER_SIZE / 4)
*/
#define INPUT_BUFFER_SIZE	512
#endif

#ifdef notdef
# define DAEMON_UID 1
#endif

#define NICKNAME_LEN 32
#define NAME_LEN 80
#define REALNAME_LEN 50
#define PATH_LEN 1024

#if defined(__hpux) || defined(hpux) || defined(_HPUX_SOURCE)
# undef HPUX
# define HPUX
# ifndef HPUX7
#  define killpg(pgrp,sig) kill(-pgrp,sig)
# endif
#endif

#if defined(__sgi)
# define USE_TERMIO
#endif /* __sgi */

#ifdef DGUX
# define USE_TERMIO
# define inet_addr(x) inet_network(x)	/* dgux lossage */
#endif /* DGUX */

/*
 * Lame Linux doesn't define X_OK in a non-broken header file, so
 * we define it here.. 
 */
#if defined(linux) && !defined(X_OK)
# define X_OK  1
#endif /* linux */

#if __osf__
# define _BSD
#endif

#if defined(UNICOS) && !defined(USE_TERMIO)
# define USE_TERMIO
#endif /* UNICOS */

/* systems without getwd() can lose, if this dies */
#if defined(NEED_GETCWD)
# define getcwd(b, c)	getwd(b);
#endif

#if defined(ISC22) || defined(ISC30)
# define USE_TERMIO
# define ISC
#endif /* ISC22 || ISC30 */

#if defined(_AUX_SOURCE) && !defined(USE_TERMIO)
# define USE_TERMIO
#endif

#ifdef MAIL_DIR
# undef UNIX_MAIL
# define UNIX_MAIL MAIL_DIR
#endif

/* flags used by who() and whoreply() for who_mask */
#define WHO_OPS		0x0001
#define WHO_NAME	0x0002
#define WHO_ZERO	0x0004
#define WHO_CHOPS	0x0008
#define WHO_FILE	0x0010
#define WHO_HOST	0x0020
#define WHO_SERVER	0x0040
#define	WHO_HERE	0x0080
#define	WHO_AWAY	0x0100
#define	WHO_NICK	0x0200
#define	WHO_LUSERS	0x0400
#define	WHO_REAL	0x0800

/*
 (* Some defines in case we're using IRCII setuid->root, so that we can bind
 * to privileged ports. This will allow future IRC nets to verify the true
 * identity of users much more reliably.
 */
#ifdef PRIV_PORT
# define	open		ruid_open
# define	fopen		ruid_fopen
# define	system		ruid_system
# define	unlink		ruid_unlink
# define	stat_file	ruid_stat
#ifdef EOF	/* defined if stdio.h has been included */
extern FILE *ruid_fopen();
# endif /*EOF*/
extern	int	ruid_open();
extern	int	ruid_system();
extern	int	ruid_unlink();
#else
# define stat_file stat
#endif /*PRIV_PORT*/

/*
 * declared in irc.c 
 */
extern	char	*cut_buffer;
extern	char	oper_command;
extern	int	irc_port;
extern	int	send_text_flag;
extern	int	irc_io_loop;
extern	int	break_io_processing;
extern	int	use_flow_control;
extern	char	*joined_nick;
extern	char	*public_nick;

extern	char	empty_string[];

extern	const	char	irc_version[];
extern	const	char	internal_version[];

extern	char	buffer[];
extern	char	nickname[NICKNAME_LEN + 1];
extern	char	*ircrc_file;
extern	char	hostname[NAME_LEN + 1];
extern	char	realname[REALNAME_LEN + 1];
extern	char	username[NAME_LEN + 1];
extern	char	*send_umode;
extern	char	*last_notify_nick;
extern	int	away_set;
extern	int	background;
extern	char	*my_path;
extern	char	*irc_path;
extern	char	*irc_lib;
extern	char	*args_str;
extern	char	*invite_channel;
extern	int	who_mask;
extern	char	*who_name;
extern	char	*who_host;
extern	char	*who_server;
extern	char	*who_file;
extern	char	*who_nick;
extern	char	*who_real;
extern	char	*cannot_open;
extern	char	global_all_off[];
extern	int	dumb;
extern	int	use_input;
extern	time_t	idle_time;
extern	time_t	start_time;
extern	int	waiting;
extern	char	wait_nick[];
extern	char	whois_nick[];
extern	char	lame_wait_nick[];
extern	char	**environ;
extern	int	current_numeric;
extern	int	new_select();
extern	int	quick_startup;

extern	struct	in_addr	local_ip_address;
extern	int	irc_io();
extern	int	dgets();
extern	time_t	dgets_timeout();
extern	char	*current_channel();
extern	void	new_stty();
extern	int	wild_match();
extern	char	*new_malloc();
extern	void	do_server();
extern	char	*get_server_nickname();
extern  int     connect_by_number();
extern	int	is_channel();
extern	void	irc_exit();
extern	void	new_close();
extern	void	beep_em();
extern	void	set_socket_options _((int));

typedef	struct	WhoisStuffStru
{
	char	*nick;
	char	*user;
	char	*host;
	char	*channel;
	char	*channels;
	char	*name;
	char	*server;
	char	*server_stuff;
	char	*away;
	int	oper;
	int	chop;
	int	not_on;
}	WhoisStuff;

/* Moved into here, because some weird CC's can't do (void *) */
typedef	struct	WhoisQueueStru
{
	char	*nick;			/* nickname of whois'ed person(s) */
	char	*text;			/* additional text */
	int	type;			/* Type of WHOIS queue entry */
	/*
	 * called with func((WhoisStuff *)stuff,(char *) nick, (char *) text) 
	 */
	void	(*func)();
	struct	WhoisQueueStru	*next;/* next element in queue */
}	WhoisQueue;

extern	char	*getenv();

#endif /* __irc_h */
