/*
 * flood.c: handle channel flooding. 
 *
 * This attempts to give you some protection from flooding.  Basically, it keeps
 * track of how far apart (timewise) messages come in from different people.
 * If a single nickname sends more than 3 messages in a row in under a
 * second, this is considered flooding.  It then activates the ON FLOOD with
 * the nickname and type (appropriate for use with IGNORE). 
 *
 * Thanks to Tomi Ollila <f36664r@puukko.hut.fi> for this one. 
 */

#ifndef lint
static	char	rcsid[] = "@(#)$Id: flood.c,v 1.2 1999/07/25 22:31:26 toast Exp $";
#endif

#include "irc.h"

#include "hook.h"
#include "ircaux.h"
#include "ignore.h"
#include "flood.h"
#include "vars.h"
#include "output.h"
#include "comstud.h"

static	char	*ignore_types[NUMBER_OF_FLOODS] =
{
	"MSG",
	"PUBLIC",
	"NOTICE",
	"WALL",
	"WALLOP",
	"CTCP",
	"INVITE",
	"NICK"
};

typedef struct flood_stru
{
	char	nick[NICKNAME_LEN + 1];
	int	type;
	char	flood;
	long	cnt;
	time_t	start;
}	Flooding;

static	Flooding *flood = (Flooding *) 0;

/*
 * check_flooding: This checks for message flooding of the type specified for
 * the given nickname.  This is described above.  This will return 0 if no
 * flooding took place, or flooding is not being monitored from a certain
 * person.  It will return 1 if flooding is being check for someone and an ON
 * FLOOD is activated. 
 */
int
check_flooding(nick, type, target, line)
	char	*nick;
	int	type;
	char	*target;
	char	*line;
{
	static	int	users = 0,
			pos = 0;
	int	i;
	time_t	flood_time,
	diff;
	Flooding *tmp;

	if (users != get_int_var(FLOOD_USERS_VAR))
	{
		if ((users = get_int_var(FLOOD_USERS_VAR)) == 0)
			return(1);
		if (flood)
			flood = (Flooding *) new_realloc(flood,
				sizeof(Flooding) * users);
		else
			flood = (Flooding *) new_malloc(sizeof(Flooding) *
				users);
		for (i = 0; i < users; i++)
		{
			*(flood[i].nick) = (char) 0;
			flood[i].cnt = 0;
			flood[i].flood = 0;
		}
	}
	if (users == 0)
		return (1);
	for (i = 0; i < users; i++)
	{
		if (*(flood[i].nick))
		{
			if ((my_stricmp(nick, flood[i].nick) == 0) &&
					(type == flood[i].type))
				break;
		}
	}
	flood_time = time(0);
	if (i == users)
	{
		tmp = &(flood[pos]);
		pos = (pos + 1) % users;
		if (type != NICK_FLOOD)
			strmcpy(tmp->nick, nick, NICKNAME_LEN);
		else
			strmcpy(tmp->nick, com_nickflood, NICKNAME_LEN);
		tmp->type = type;
		tmp->cnt = 1;
		tmp->start = flood_time;
		tmp->flood = 0;
		return (1);
	}
	else
		tmp = &(flood[i]);
	if (type == NICK_FLOOD)
	{
		strmcpy(tmp->nick, com_nickflood, NICKNAME_LEN);
		nick = com_nickflood;
	}
	tmp->cnt++;
	diff = flood_time - tmp->start;
	if (tmp->cnt > get_int_var(FLOOD_AFTER_VAR))
	{
		if ((diff == 0) || (tmp->cnt / diff) >
				get_int_var(FLOOD_RATE_VAR))
		{
			if (tmp->flood == 0)
			{
				if (get_int_var(FLOOD_WARNING_VAR))
					say("%s flooding detected from %s",
						ignore_types[type], nick);
				tmp->flood = 1;
			}
			return (do_hook(FLOOD_LIST, "%s %s %s %s", nick,
				ignore_types[type], target, line));
		}
		else
		{
			tmp->flood = 0;
			tmp->cnt = 1;
			tmp->start = flood_time;
		}
	}
	return (1);
}
