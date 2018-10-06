#ifndef _AUTOOP_H_
#define _AUTOOP_H_

typedef struct autoop_timer_st
{
	char	*nick;
	char	*channel;
	time_t	when;
	struct autoop_timer_st *next;
} AutoOp_Timer;

extern void	handle_autoop();
extern void	autoop();
extern void	save_autoops(FILE*);
extern void	OpTimer();

extern void	remove_autoop_chan(char *, char *);
extern void	remove_autoop_all(char *);
extern void	rename_autoop(char *, char *);

#endif
