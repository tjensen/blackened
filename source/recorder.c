/*
 * recorder.c: Stuff dealing with the Away Message Recorder
 *
 * Written By Timothy Jensen
 *
 * Copyright (c) 1999 
 *
 * See the COPYRIGHT file, or do a /HELP COPYRIGHT 
 */

#ifndef lint
static	char	rcsid[] = "@(#)$Id: recorder.c,v 1.7 2001/04/01 15:58:27 toast Exp $";
#endif

#include "irc.h"

#include "screen.h"
#include "term.h"
#include "recorder.h"
#include "format.h"
#include "parse.h"
#include "status.h"
#include "ircaux.h"
#include "output.h"

static Rec_Msg	*Msgs = (Rec_Msg *) 0;
static int	ReadingMessages = 0;

static int
count_msgs()
{
	Rec_Msg	*tmp = Msgs;
	int	count = 0;

	while (tmp) {
		count++;
		tmp = tmp->next;
	}
	return count;
}

char	*
count_messages()
{
	static char	ret_msg[8];
	int		num;

	num = count_msgs();
	if (num) {
		sprintf(ret_msg, "%d", num);
		return (char *) &ret_msg;
	}
	return (char *) 0;
}

void
record_msg(time, from, userhost, message)
	time_t	time;
	char	*from,
		*userhost,
		*message;
{
	Rec_Msg	*New, *pass;

	New = (Rec_Msg *)new_malloc(sizeof(Rec_Msg));

	New->time = time;
	New->from = New->userhost = New->message = (char *) 0;
	malloc_strcpy(&New->from, from);
	malloc_strcpy(&New->userhost, userhost);
	malloc_strcpy(&New->message, message);

	if (Msgs && (Msgs->time < time)) {
		pass = Msgs;
		while (pass->next && (pass->next->time < time))
			pass = pass->next;
		New->next = pass->next;
		pass->next = New;
	} else {
		New->next = Msgs;
		Msgs = New;
	}
}

static void
del_msg(msgnum)
	int	msgnum;
{
	Rec_Msg	*foo, *tmp = Msgs;
	int	count = 1;

	if (tmp && (msgnum > 0)) {
		if (msgnum == 1) {
			Msgs = Msgs->next;
			new_free(&tmp->from);
			new_free(&tmp->userhost);
			new_free(&tmp->message);
			new_free(&tmp);
		} else {
			while (tmp->next && (++count != msgnum))
				tmp = tmp->next;
			if (tmp->next) {
				foo = tmp->next;
				tmp->next = foo->next;
				new_free(&foo->from);
				new_free(&foo->userhost);
				new_free(&foo->message);
				new_free(&foo);
			}
		}
	}
	update_all_status();
}

void
clear_msgs(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	Rec_Msg	*tmp;
	int	count = 0;

	while (Msgs) {
		tmp = Msgs;
		Msgs = Msgs->next;
		new_free(&tmp);
		count++;
	}
	say("%d message%s erased.", count, (count == 1) ? "" : "s");
	update_all_status();
}

static	void
show_msg(msgnum)
	int	msgnum;
{
	Rec_Msg	*tmp = Msgs;
	char	datetime[128], num[20];
	char	*olduserhost;
	int	i = 1;

	while (tmp && (i != msgnum)) {
		tmp = tmp->next;
		i++;
	}

	if (tmp) {
		strftime(datetime, 128, "%T on %D", localtime(&tmp->time));
		sprintf(num, "%d", msgnum);
		olduserhost = FromUserHost;
		FromUserHost = tmp->userhost;
		put_it("%s", parseformat(RECORDER_HEAD_FMT, datetime,
			tmp->from, tmp->message, num));
		put_it("%s", parseformat(RECORDER_BODY_FMT, datetime,
			tmp->from, tmp->message, num));
		FromUserHost = olduserhost;
	}
}

static	void
msg_prompt(msgnum, input)
	char	*msgnum,
		*input;
{
	char	tmp[20];
	int	num;

	num = atoi(msgnum);
	switch(*input) {
	case 'd':
	case 'D':
		del_msg(num);
		if (!count_msgs()) {
			say("No more messages.");
			ReadingMessages = 0;
			break;
		} else
			say("Deleted.");
		while ((num > 1) && (num > count_msgs()))
			num--;
		show_msg(num);
		sprintf(tmp, "%d", num);
		add_wait_prompt("'d' delete, 'n' next, 'p' previous, 'q' quit: ",
			msg_prompt, tmp, WAIT_PROMPT_KEY);
		break;
	case 'p':
	case 'P':
		if (num > 1) {
			num--;
			show_msg(num);
		} else
			term_beep();
		sprintf(tmp, "%d", num);
		add_wait_prompt("'d' delete, 'n' next, 'p' previous, 'q' quit: ",
			msg_prompt, tmp, WAIT_PROMPT_KEY);
		break;
	case 'n':
	case 'N':
	case ' ':
		if (num < count_msgs()) {
			num++;
			show_msg(num);
		} else
			term_beep();
		sprintf(tmp, "%d", num);
		add_wait_prompt("'d' delete, 'n' next, 'p' previous, 'q' quit: ",
			msg_prompt, tmp, WAIT_PROMPT_KEY);
		break;
	case 'q':
	case 'Q':
		num = count_msgs();
		say("%d message%s still waiting.", num, (num==1) ? "" : "s");
		ReadingMessages = 0;
		break;
	default:
		term_beep();
		add_wait_prompt("'d' delete, 'n' next, 'p' previous, 'q' quit: ",
			msg_prompt, msgnum, WAIT_PROMPT_KEY);
	}
}

void
play_msgs(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	char	msgnum[20];

	if (!Msgs) {
		say("No messages saved!");
		return;
	}

	if (ReadingMessages) {
		say("You're already reading your messages!");
		return;
	}

	ReadingMessages = 1;
	say("Your messages:");
	show_msg(1);
	strcpy(msgnum, "1");
	add_wait_prompt("'d' delete, 'n' next, 'p' previous, 'q' quit: ",
			msg_prompt, msgnum, WAIT_PROMPT_KEY);
}

void
tell_msg_count()
{
	Rec_Msg *tmp = Msgs;
	int	count = 0;

	while (tmp) {
		count++;
		tmp = tmp->next;
	}

	say("You have %d message%s waiting.", count, (count==1) ? "" : "s");
}

int
have_msgs()
{
	return (Msgs != NULL);
}
