#ifndef _TOAST_H_
#define _TOAST_H_

/* Modifications by Timothy L. Jensen aka Toast. */

#define	TACTION_NONE		0
#define	TACTION_FINDU		1
#define TACTION_FINDK		2
#define	TACTION_BANCOUNT	3
#define TACTION_CSTAT		4
#define TACTION_FINDD		5
#define TACTION_WHOKILL		6

#define FIND_NONE	0
#define FIND_COUNT	1
#define FIND_REMOVE	2

typedef struct SpingStuffStru {
	char *server;
	long start, ustart;
	struct SpingStuffStru *next;
} SpingStuff;

extern	int	page_updown_key;

extern	int	toast_action;

extern	int	in_whois;
extern	int	bc_count;
extern	char	*bc_chname;

extern	int	setting_nolog;

char	*TimeStamp();
char	*MakeTimeStamp(long);
void	channel_created _((char *, char **));
void	channel_topictime _((char *, char **));
int	Do_Not_Log _((char *));
void	nolog();
void	set_nolog();
void	doleave();
void	dowho();
void	dowii();
void	dotopic();
void	domode();
void	dokline();
void	unkline();
void	vercmd();
void	auto_msg();
void	remnick _((char *));
extern void addnick(char *anick);
extern void adddccnick(char *anick);
void	do_new_window();
void	do_kill_window();
void	do_list_windows();
void	do_grow_window();
void	do_shrink_window();
void	do_hide_window();
void	do_opervision();
void	do_pageup();
void	do_pagedown();
void	handle_findu();
void	findu();
void	handle_findk();
void	findk();
void	bancount();
void	handle_cstat();
void	cstat();
void	userhost_notify();
int	handle_sping();
void	handle_sping_nosuchserver();
void	sping();
void	show_banner();
void	umode();
void	handle_whokill _((char *, char *, char *, char *));
void	add_to_away _((char *, char *));
int	seen_away_msg _((char *, char *));
char	*function_randread _((char *));
void	invitecmd();
char	*SmallTS _((const time_t *));
void	awaylistcmd();
void	set_oops _((char *, char *, char*));
void	oopscmd();
void	userhost_finger();
void	change_server_tempbans _((int, int));
void	insert_ban _((char *, char *, int));
extern int remove_ban(char *channel, char *mask);
void	BanTimer();
void	handle_findd();
void	findd();
char	*filterlog(char *);
void	chat_cmd();
void	rchat_cmd();
void	show_tempbans();
void	dovoice();
void	dodevoice();
void	massvoice();
void	massdevoice();
void	dmsg();
void	set_urllog_level _((char *));
void	set_urllog_file _((char *));
void	add_to_urllog _((char *));

#endif	/* _TOAST_H_ */
