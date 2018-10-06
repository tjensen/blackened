/*
 * format.h: header for format.c
 *
 */

typedef struct
{
	char	*name;
	char	*fmt;
	char	*args;
	char	int_flags;
} IrcFormat;

typedef struct numfmt
{
	int	num;
	char	*fmt;
	char	int_flags;
	struct numfmt *next;
} NumFormat;

extern	void	formatcmd();
extern	char	*parseformat();
extern	char	*numericformat();
extern	void	save_formats(FILE *);
extern	void	do_highlight(int);
extern	int	num_format_exists(int);
extern	int	format_get(char *, IrcFormat *);
extern	int	format_set(char *, IrcFormat *);
extern	int	format_nget(int, NumFormat *);
extern	int	format_nset(int, NumFormat *);

#ifndef _FORMAT_H_
#define _FORMAT_H_

enum FMT_TYPES {
	ACTION_FMT,
	ACTION_OTHER_FMT,
	CTCP_FMT,
	CTCP_PRIVATE_FMT,
	CTCP_REPLY_FMT,
	CTCP_UNKNOWN_FMT,
	DCC_CHAT_FMT,
	DCC_CHAT_ACTION_FMT,
	DCC_TALK_FMT,
	GONE_FMT,
	INVITE_FMT,
	JOIN_FMT,
	JOIN_SELF_FMT,
	KICK_FMT,
	KICK_SELF_FMT,
	KILL_FMT,
	MODE_FMT,
	MSG_FMT,
	MSG_GROUP_FMT,
	NICK_FMT,
	NOTICE_FMT,
	NOTIFY_SIGNOFF_FMT,
	NOTIFY_SIGNON_FMT,
	NOTIFY_SIGNON_USERHOST_FMT,
	OPS_FMT,
	OPS_OTHER_FMT,
	PART_FMT,
	PART_SELF_FMT,
	PUBLIC_FMT,
	PUBLIC_ACTION_FMT,
	PUBLIC_MSG_FMT,
	PUBLIC_NOTICE_FMT,
	PUBLIC_OTHER_FMT,
	RECORDER_BODY_FMT,
	RECORDER_HEAD_FMT,
	SEND_ACTION_FMT,
	SEND_DCC_CHAT_FMT,
	SEND_DCC_TALK_FMT,
	SEND_LOCOPS_FMT,
	SEND_MSG_FMT,
	SEND_NOTICE_FMT,
	SEND_OPERWALL_FMT,
	SEND_OPS_FMT,
	SEND_OPS_OTHER_FMT,
	SEND_PUBLIC_FMT,
	SEND_PUBLIC_ACTION_FMT,
	SEND_PUBLIC_MSG_FMT,
	SEND_PUBLIC_OTHER_FMT,
	SEND_WALLOPS_FMT,
	SERVER_KILL_FMT,
	SERVER_NOTICE_FMT,
	SERVER_WALLOPS_FMT,
	TOPIC_FMT,
	QUIT_FMT,
	UMODE_FMT,
	WALL_FMT,
	WALLOPS_FMT,
	WHOIS_ADMIN_FMT,
	WHOIS_AWAY_FMT,
	WHOIS_CHANNELS_FMT,
	WHOIS_IDLE_FMT,
	WHOIS_IRCNAME_FMT,
	WHOIS_NSLOOKUP_FMT,
	WHOIS_OPER_FMT,
	WHOIS_SERVER_FMT,
	WHOIS_SIGNON_FMT,
	WHOIS_USERHOST_FMT,
	WHOWAS_USERHOST_FMT,
	NUMBER_OF_FORMATS
};

#endif /* _FORMAT_H_ */
