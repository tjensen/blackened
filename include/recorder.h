#ifndef _RECORDER_H_
#define _RECORDER_H_

typedef struct rec_msg
{
	time_t		time;
	char		*from,
			*userhost,
			*message;
	struct rec_msg	*next;
} Rec_Msg;

char *count_messages();
void record_msg(time_t, char *, char *, char *);
void clear_msgs(char *, char *, char *);
void play_msgs(char *, char *, char *);
void tell_msg_count();
int  have_msgs();

#endif
