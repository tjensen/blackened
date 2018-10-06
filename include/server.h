/*
 * server.h: header for server.c 
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 *
 * @(#)$Id: server.h,v 1.13 2001/11/29 23:58:43 toast Exp $
 */

#ifndef _SERVER_H_
#define _SERVER_H_
  
#include "autoop.h"
#include "toast.h"
#include "parse.h"
#include "rejoin.h"

/*
 * type definition to distinguish different
 * server versions
 */
#define Server2_5	0
#define Server2_6	1
#define Server2_7	2
#define Server2_8	3
#define Server2_9	4
#define Server2_10	5

/*
 * Server Options
 */
#define SOptions_None		0x0	/* No server options */
#define SOptions_AtChannel	0x1	/* Hybrid6-style /ops */
#define SOptions_WallChops	0x2	/* Undernet WALLCHOPS-style /ops */

/* Server: a structure for the server_list */
typedef	struct
{
	char	*name;			/* the name of the server */
	char	*itsname;		/* the server's idea of its name */
	char	*password;		/* password for that server */
	int	port;			/* port number on that server */
	char	*nickname;		/* nickname for this server */
	char	*away;			/* away message for this server */
	int	operator;		/* true if operator */
	char    *op_nick;		/* oper nick if operator */
        char    *op_pwd;                /* oper password if operator */
	int	version;		/* the version of the server -
					 * defined above */
	int	options;		/* ircd supported options */
	char	*version_string;	/* what is says */
	int	whois;			/* true if server sends numeric 318 */
	int	flags;			/* Various flags */
	int	connected;		/* true if connection is assured */
	int	write;			/* write descriptor */
	int	read;			/* read descriptior */
	pid_t	pid;			/* process id of server */
	int	eof;			/* eof flag for server */
	int	motd;			/* motd flag (used in notice.c) */
	int	sent;			/* set if something has been sent,
					 * used for redirect */
	char	*buffer;		/* buffer of what dgets() doesn't get */
	WhoisQueue	*WQ_head;	/* WHOIS Queue head */
	WhoisQueue	*WQ_tail;	/* WHOIS Queue tail */
	WhoisStuff	whois_stuff;	/* Whois Queue current collection buffer */
	struct in_addr local_addr;	/* ip address of this connection */

	AutoOp_Timer	*OpTimers;	/* Delayed AutoOp queue */
	SpingStuff	*spingstuff;	/* Server Pings */
	AutoWho		*autowho;	/* Auto-who on join */
	RejoinChan	*rejoins;	/* Auto-rejoin list */


}	Server;

typedef	unsigned	short	ServerType;

extern	void	add_to_server_list();
extern	void	build_server_list();
extern	int	connect_to_server();
extern	void	get_connected();
extern	int	read_server_file();
extern	void	display_server_list();
extern	char	*current_server_password();
extern	int	server_list_size();
# ifdef USE_STDARG_H
extern	void	send_to_server(char *, ...);
# else
extern	void	send_to_server();
# endif
extern	int	get_server_whois _((int));

extern	WhoisStuff	*get_server_whois_stuff();
extern	WhoisQueue	*get_server_qhead();
extern	WhoisQueue	*get_server_qtail();

extern	int	attempting_to_connect;
extern	int	number_of_servers;
extern	int	connected_to_server;
extern	int	never_connected;
extern	int	using_server_process;
extern	int	primary_server;
extern	int	from_server;
extern	char	*connect_next_nick;
extern	int	parsing_server_index;

extern	void	server _((char *, char *, char *));
extern	char	*get_server_nickname();
extern	char	*get_server_name();
extern	char	*get_server_itsname();
extern	void	set_server_flag();
extern	int	find_in_server_list();
extern	char	*create_server_list();
extern	void	set_server_motd();
extern	int	get_server_motd();
extern	int	get_server_operator();
extern	char	*get_server_operator_nick(int);
extern	char	*get_server_operator_pwd(int);
extern	void	clear_all_server_operator_pwd(void);
extern	int	get_server_2_6_2();
extern	int	get_server_version();
extern	int	get_server_options();
extern	void	close_server();
extern	void	MarkAllAway();
extern	int	is_server_connected();
extern	void	flush_server();
extern	int	get_server_flag();
extern	void	set_server_operator();
extern	void	server_is_connected();
extern	int	parse_server_index();
extern	void	parse_server_info();
extern	void	set_server_bits();
extern	void	set_server_itsname();
extern	void	set_server_version();
extern	void	set_server_options();
extern	int	is_server_open();
extern	int	get_server_port();
extern	char	*set_server_password();
extern	void	set_server_nickname();
extern	void	set_server_2_6_2();
extern	void	set_server_qhead();
extern	void	set_server_qtail();
extern	void	set_server_whois();
extern	void	close_all_server _((void));
extern	void	disconnectcmd _((char *, char *, char *));
extern	void	got_isupport _((char **));

/* server_list: the list of servers that the user can connect to,etc */
extern	Server	*server_list;

#define	SERVER_2_6_2	0x0001
#define	USER_MODE_I	0x0002
#define	USER_MODE_W	0x0004
#define	USER_MODE_S	0x0008
#define CLOSING_SERVER	0x0010
#define USER_MODE_F	0x0020
#define USER_MODE_U	0x0040
#define USER_MODE_C	0x0080
#define USER_MODE_K	0x0100
#define USER_MODE_Z	0x0200
#define USER_MODE_R	0x0400
#define USER_MODE_Y	0x0800
#define USER_MODE_D	0x1000
#define USER_MODE_N	0x2000
#define USER_MODE_X	0x4000

#endif /* _SERVER_H_ */
