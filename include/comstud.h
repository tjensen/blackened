#ifndef COMSTUD_H
#define COMSTUD_H
/* comstud.h -- All my crap that other files need */

#include <stdio.h>

#define OPS_METHOD_DEFAULT	0
#define OPS_METHOD_HYBRID	1
#define OPS_METHOD_WALLCHOPS	2
#define OPS_METHOD_AUTO		-1

extern char auto_who[255];
extern int  com_action;
extern char com_arg[2048];
extern char com_nickflood[NICKNAME_LEN+1];
extern char msglog[255];
extern char reason[255];

void split_kill(char *, char **, char **, char **, char **, char **);
void utimediff(long int, long int, long int, long int, time_t*, time_t*);
void	log_kill(char *, char *, char *, char *, char *);
void	logaddnolog(char *nick);
void	logremovenolog(char *nick);
int	logmsg(int sr, char *type, char *from, char *string, char *extra, int flag);
void	msglogger(int flag);
void	com_unban(char *channel, char *arg);
char	*do_nslookup(char *host);
char	*random_str(int min, int max);
void	do_newuser(char *newnick, char *newusername, char *ircname);
char	*cluster(char *host);
char	*get_token(char **doh, char *chars);
void    do_reconnect();
void	reset();
void    newnick();
void    cycle();
void	dokick();
void    multkick();
void	my_ignore();
void	my_ignorehost();
void    tochanops();
void    newuser();
void	newhost();
void    bomb();
void    do_flood();
void    nslookup();
void    massop();
void    massdeop();
void    masskick();
void    massban();
void    masskickban();
void    ban();
void    kickban();
void    unban();
void    finger();
void    masskickbanold();
void    massfuck();
void    siteban();
void    sitekickban();
void	users();
void	whokill();
void	handle_masskill(char *);
void	handle_tracekill(char *);
void	traceserv();
void	tracekill();
void	doop();
void	dodeop();
int	silly_match();
char	*my_next_arg(char **);
extern char *my_stristr(char *s1, char *s2);
char	*get_reason();
void	set_ops_method(int);
void	not_on_a_channel(void);
void	netfinger(char *);
int	isme(char *);
void	lognolog(char *);
int	freadln(FILE *, char *);

#define getrandom(min, max) ((rand() % (int)(((max)+1) - (min))) + (min))

#endif
