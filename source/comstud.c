/*
 * comstud.c: A bunch of stuff Comstud wrote
 *
 */

#ifndef lint
static	char	rcsid[] = "@(#)$Id: comstud.c,v 1.29.2.1 2002/03/07 19:35:40 toast Exp $";
#endif

#include <stdio.h>
#include <ctype.h>

#ifdef linux
#  include <sys/mman.h>
#endif /* linux */
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>

#ifdef ESIX
#  include <lan/net_types.h>
#endif /* ESIX */
#include <sys/types.h>
#include "irc.h"
#include "term.h"
#include "server.h"
#include "edit.h"
#include "crypt.h"
#include "vars.h"
#include "ircaux.h"
#include "lastlog.h"
#include "window.h"
#include "screen.h"
#include "whois.h"
#include "hook.h"
#include "input.h"
#include "ignore.h"
#include "keys.h"
#include "names.h"
#include "alias.h"
#include "history.h"
#include "funny.h"
#include "ctcp.h"
#include "dcc.h"
#include "translat.h"
#include "output.h"
#include "exec.h"
#include "notify.h"
#include "numbers.h"
#include "status.h"
#include "hook.h"
#include "toast.h"
#include "format.h"
#include "comstud.h"

#define getrandom(min, max) ((rand() % (int)(((max)+1) - (min))) + (min))


#define CHECK_FOR_OPS	/* define this if you want to
			make sure you are opped before
			doing a mode */ 

int  ops_method = OPS_METHOD_DEFAULT;
int  com_action = 0;
char com_arg[2048];
char com_nickflood[NICKNAME_LEN+1];
char reason[255] = "";
#ifdef COMSTUD_ORIG
char buffer[255];
#endif
char msglog[255];
FILE *logptr = (FILE *) 0;

char *get_token(char **src, char *chars);
/*char *my_next_arg(char **the_arg);*/

extern char MyHostName[80];
extern struct   in_addr MyHostAddr;             /* The local machine address */

int
isme(nick)
	char	*nick;
{
	return !my_stricmp(nick, get_server_nickname(from_server));
}

/*
irc.HACKS.Arizona.EDU *** Notice -- Received KILL message for imanoste. From
 cs-pub.bu.edu Path:
 irc1.portal.com[156.151.6.50]!irc2.ece.uiuc.edu[128.174.64.10]!*.bu.edu[cs-pub
 .bu.edu]!cs-pub.bu.edu (imanoste <- irc2.ece.uiuc.edu)
*/

/* Hybrid-6 local kill:
:irc.blackened.com NOTICE ToastTEST :*** Notice -- Received KILL message for killtest. From Toast Path: plasma.powered!toast!Toast (testing)
*/

void
split_kill(path, who, who_uh, server, dead, reason)
char *path;
char **who;
char **who_uh;
char **server;
char **dead;
char **reason;
{
	static char uhbuf[128];
	char *uname, *hname;
	char *temp;

	*who = *who_uh = *server = *reason = NULL;

	*dead = strstr(path, "for");
	if (!*dead)
		return;
	*dead = strchr(*dead, ' ');
	if (!*dead)
		return;
	(*dead)++;
	temp = strchr(*dead, ' ');
	if (!temp)
		return;
	*(temp-1) = '\0';
	temp++;
	*who = strchr(temp, ' ');	
	if (!*who)
		return;
	(*who)++;
	temp = strchr(*who, ' ');
	if (!temp)
		return;
	*temp = '\0';
	temp+=7;  /* now pointing at beginning of the Path */
	*(temp-2) = '\0'; /* to fix the below while() loops */
	if (*reason = strchr(temp, ' '))
	{
		**reason = '\0';
		(*reason)++;
	}
	if (!*reason || !**reason)
		*reason = NULL;
	temp = strrchr(temp, '!');
	if (!temp)
		return;
	*temp = '\0';		/* chop off the nick */

	temp--;
	while (*temp && (*temp != ' ') && (*temp != '!'))
		temp--;
	if ((*temp == '!') && !strchr(temp, '.')) {
		*temp = '\0';
		uname = temp+1;
		temp--;
		while (*temp && (*temp != ' ') && (*temp != '!'))
			temp--;
		*temp = '\0';
		hname = temp+1;
	} else {
		*temp = '\0';
		uname = temp+1;
		hname = NULL;
	}

	if (!uname || !*uname)
		uname = NULL;
	if (hname != NULL) {
		sprintf(uhbuf, "%s@%s", uname, hname);
		*who_uh = (char *)&uhbuf;
	} else if (uname != NULL)
		*who_uh = uname;

	temp--;
	while (*temp && (*temp != ' ') && (*temp != '!'))
		temp--;
	*temp = '\0';
	*server = temp+1;
	if (!*server || !**server)
		*server = NULL; 
}

void utimediff(s1, us1, s2, us2, temp, utemp)
long int	s1, us1, s2, us2;
time_t		*temp, *utemp;
{
	*temp = s1-s2;
	if (us2 > us1)
	{
		(*temp)--;
		us1 += 1000000;
	}
	*utemp = us1-us2;
}

void log_kill(nick, from, host, server, reason)
char *nick;
char *from;
char *host;
char *server;
char *reason;
{
	FILE *whee;
	char filename[255];
	char *blah;
	long int t;

	t = time(0);
	if (!get_string_var(KILLLOGFILE_VAR))
		return;
	strcpy(filename, my_path);
	strcat(filename, "/");
	strcat(filename, get_string_var(KILLLOGFILE_VAR));
	if (!(whee=fopen(filename, "at")))
		return;
	if (nick)
	{
		blah = SmallTS((time_t *)&t);
		if (strchr(from, '!') || strchr(from, '@'))
			fprintf(whee, "%s%s%s by %s on %s %s\n",blah?blah:"",
				blah?"> ":"", nick, from, server, reason);
		else
			fprintf(whee, "%s%s%s by %s@%s on %s %s\n",blah?blah:"",
				blah?"> ":"", nick, from, host, server, reason);
	}
	else if (from)
		fprintf(whee, "KillLog started at %s\n", SmallTS((time_t *)&t));
	else
		fprintf(whee, "KillLog ended at %s\n", SmallTS((time_t *)&t));
	fclose(whee);
}

void
logaddnolog(nick)
char	*nick;
{
	char *timestr;
	time_t t;

	t = time(0);
	if (logptr) {
		fprintf(logptr, "*** Stopped logging messages to/from %s <%s>\n",
			nick, SmallTS(&t));
	}
}

void
logremovenolog(nick)
char	*nick;
{
	char *timestr;
	time_t t;

	t = time(0);
	if (logptr) {
		fprintf(logptr, "*** Started logging messages to/from %s <%s>\n",
			nick, SmallTS(&t));
	}
}

void
lognolog(nicks)
char	*nicks;
{
	char *timestr;
	time_t t;

	t = time(0);
	if (nicks && *nicks)
		fprintf(logptr, "*** Not logging messages to/from: %s <%s>\n", nicks, SmallTS(&t));
	else
		fprintf(logptr, "*** All nicks are being logged <%s>\n", SmallTS(&t));
}

int  logmsg(rc, type, from, string, extra, flag)
int  rc;
char *type;
char *from;
char *string;
char *extra;
int  flag;
{
	char *timestr;
	FILE *out;
	time_t t;
	char filename[255];
	mode_t savemask;
	char *nologlist;

	/*
	 * Check the NO_LOG variable for 'from'. If it contains that nick,
	 * don't log this msg. This is great when you're talking to a tcm
	 * bot or something else that quickly fills your .MsgLog.
	 * (Toast 9/12/1997)
	 */
	if (Do_Not_Log(from))
		return 0;

/*	say("MSGLOGFILEVAR = %d",MSGLOGFILE_VAR);*/
	if (!get_string_var(MSGLOGFILE_VAR))
		return 0;
	strcpy(filename, my_path);
	strcat(filename, "/");
	strcat(filename, get_string_var(MSGLOGFILE_VAR));
/*	say("MSGLOGFILE = %s",get_string_var(MSGLOGFILE_VAR));*/
	if (get_int_var(MSGLOG_VAR) && !logptr)
		flag = 1;
	if (flag == 0 && !logptr)
		return 0;
	t = time(0);
/*	timestr = update_clock(GET_TIME);*/
	timestr = SmallTS(&t);
	if (!flag)
	{
		fprintf(logptr, "%s%s%s%s %s <%s%s%s>\n",
			rc ? "-> " : "", type, from,
			(type && (*type == '<')) ? ">" : type,
			string, timestr ? timestr : "",
			extra ? " " : "", extra ? extra : "");
		fflush(logptr);
	}
	else if (flag == 1)
	{
		if (logptr)
		{
			say("Already logging messages");
			return 1;
		}
		savemask = umask(S_IRWXG | S_IRWXO);
		if (!(logptr = fopen(filename, "at")))
		{
			(void)umask(savemask);
			set_int_var(MSGLOG_VAR, 0);
			return 0;
		}
		(void)umask(savemask);
		fprintf(logptr, "MsgLog started at %s\n", SmallTS(&t));
		if (nologlist = get_string_var(NO_LOG_VAR))
			fprintf(logptr, "*** Not logging messages to/from: %s\n", nologlist);
		fflush(logptr);
        	if (string)
			return logmsg(rc, type, from, string, extra, 0);
		say("Now logging messages to: %s", filename);
	}
	else if (flag == 2)
	{
		if (!logptr)
		{
			say("Already not logging messages");
			return 1;
		}
		fprintf(logptr, "MsgLog ended at %s\n", SmallTS(&t));
		fclose(logptr);
		logptr = (FILE *) 0;
	}
	return 1;
}

void msglogger(flag) /* called when msglog is set */
int flag;
{
        char    *logfile;

        if ((logfile = get_string_var(MSGLOGFILE_VAR)) == (char *) 0)
        {
                say("You must set the MSGLOGFILE variable first!");
                set_int_var(MSGLOG_VAR, 0);
                return;
        }
	logmsg(0, NULL, NULL, NULL, NULL, flag ? 1 : 2);
}

void
not_on_a_channel(void)
{
        say("D'oh! You're not on a channel!");
}

int are_you_opped(channel)
char *channel;
{
        return is_chanop(channel, get_server_nickname(from_server));
}

void you_not_opped(channel)
char *channel;
{
        say("D'oh! You're not opped on %s", channel);
}

char *terminate(string, chars)
char *string;
char *chars;
{
  char *ptr;

  if (!string || !chars)
    return "";
  while (*chars)
    {
      if (ptr = strrchr(string, *chars))
        *ptr = '\0';
      chars++;
    }
  return string;
}

int
freadln(stream, lin)
	FILE	*stream;
	char	*lin;
{
        char *p;

        do
          p = fgets(lin, 1000, stream);
        while (p && (*lin == '#'));

        if (!p)
          return 0;
        terminate(lin, "\n\r");
        return 1;
}

char *randreason()
{
  int count, min, max, i;
  FILE *bleah;
  char filename[1024];
  char static buffer[1024];
  min = 1;
  count = 0;

  strcpy(buffer, "");
  strcpy(filename, irc_lib);
  strcat(filename, "reasons");
  if (!(bleah = fopen(filename, "r")))
    return NULL; 
  while (!feof(bleah))
      if (freadln(bleah, buffer))
        count++;
  fseek(bleah, 0, 0);
  i = getrandom(1, count);
  count = 0;
  while (!feof(bleah) && (count < i))
      if (freadln(bleah, buffer))
        count++;
  fclose(bleah);
  if (*buffer)
    return buffer;
  return NULL; 
}

char *get_reason()
{
   char *temp;
   if (!get_int_var(REASON_TYPE_VAR))  /* use default reason */
      return get_string_var(DEFAULT_REASON_VAR);
   temp = randreason();
   return temp ? temp : get_string_var(DEFAULT_REASON_VAR);
}

char *
do_nslookup(host)
char *host;
{
  struct hostent *temp;
  struct in_addr temp1;
  int ip;
  ip=0;

  if (!host)
    return (char *) NULL;
  if (isdigit(*(host+strlen(host)-1)))
  {
    ip=1;
    temp1.s_addr = inet_addr(host);
    temp=gethostbyaddr((char*)&temp1, sizeof (struct in_addr), AF_INET);
  }
  else
  {
    temp=gethostbyname(host);
    if (temp)
	memcpy((caddr_t)&temp1, temp->h_addr, temp->h_length);
    else
        return (char *) NULL;
  }
  if (!temp)
    return (char *) NULL;
  if (ip)
    return (char *)temp->h_name;
  return ((char *)inet_ntoa(temp1));
}

void do_reconnect(newnick)
char *newnick;
{
#ifdef EHUD
printf("comstud.c do_reconnect(): from_server %d \n",from_server);
printf("comstud.c do_reconnect(): name hex %x\n",server_list[from_server].name);
printf("comstud.c do_reconnect(): name str %s\n",server_list[from_server].name);
#endif /* EHUD */
  say("Reconnecting to server %s", server_list[from_server].name);
  close_server(from_server, "brb");
  if (connect_to_server(server_list[from_server].name,
                        server_list[from_server].port, primary_server) != -1)
    {
      if (newnick)
         send_to_server("NICK %s", newnick);
      change_server_channels(primary_server, from_server);
      set_window_server(-1, from_server, 1);
    }
  else
    say ("D'oh! Unable to reconnect.  Use /SERVER to connect.");
}

void do_newuser(newnick, newusername, ircname)
char *newnick;
char *newusername;
char *ircname;
{
        if (newusername)
        {
                if (!newusername) newusername = "";
                        strlcpy(username, newusername, sizeof(username));
                if (ircname && *ircname)
                        strlcpy(realname, ircname, sizeof(realname));
                do_reconnect(newnick);
        }
}

char *left(string, i)
char *string;
int i;
{
  static char temp[255];
  strcpy(temp, string);
  temp[i] = '\0';
  return temp;
}

char *right(string, num)
char *string;
int num;
{
        if (strlen(string) < num)
                return string;
        return (string+strlen(string)-num);
}

char *random_str(min, max)
int min;
int max;
{
    int i, ii;
    static char str[256];

    i = getrandom(min, max);
    for (ii=0;ii<i;ii++)
       str[ii] = (char) getrandom(97, 122);
    str[ii]='\0';
    return str;
}

char *get_token(src, token_sep)
char **src;
char *token_sep;
{
  char    *tok;

  if (!(src && *src && **src))
    return NULL;

  while(**src && strchr(token_sep, **src))
    (*src)++;

  if(**src)
    tok = *src;
  else
    return NULL;

  *src = strpbrk(*src, token_sep);
  if (*src)
    {
      **src = '\0';
      (*src)++;
      while(**src && strchr(token_sep, **src))
        (*src)++;
    }
  else
    *src = "";
  return tok;
}

char *my_next_arg(the_arg)
char **the_arg;
{
	return get_token(the_arg, " ");
}

int numchar(string, c)
char *string;
char c;
{
        int num = 0;
        while (*string)
        {
                if (tolower(*string) == tolower(c))
                        num++;
                string++;
        }
        return num;
}

char *my_stristr(s1, s2)
char *s1;
char *s2;
{
  char n1[1024], n2[1024];
  char *temp, *ptr1, *ptr2;

  ptr1 = n1;
  ptr2 = n2;
  if (!s1 || !s2)
    return NULL;
  while (*s1)
    *(ptr1++) = toupper(*(s1++));
  while (*s2)
    *(ptr2++) = toupper(*(s2++));
  *ptr1 = '\0';
  *ptr2 = '\0';
  temp = strstr(n1, n2);
  if (temp)
    return (s1 + (temp-n1));
  return NULL;
}

char *cluster(hostname)
char *hostname;
{
	static char result[1024];
	char	    temphost[255];
	char	    *host;

	host = temphost;
	strcpy(result, "");
	if (!hostname)
   /* Some systems are picky when it comes to doing string operations
    * on null pointers. Rather than waste my time adding yet another
    * if block to parse_notice() for detecting notices from servers
    * (like services.us), I'll just put a stupid kludge here.
    *					(Toast 9/8/1997)
    *   	return (char *) 0;
    */
	 	return result;
	if (strchr(hostname, '@'))
        {
	    if (*hostname == '~')
		hostname++;
            strcpy(result, hostname);
            *strchr(result, '@') = '\0';
            strcat(result, "@");
	    if (!(hostname = strchr(hostname, '@')))
            	return (char *) 0;
            hostname++;
        }
	strcpy(host, hostname);

	if (*host && isdigit(*(host+strlen(host)-1)))
	{
		int i;
		char *tmp;
                tmp = host;
		/*for (i=0;i<2;i++)*/
		for (i=0;i<3;i++)
			tmp = strchr(tmp, '.')+1;
		*tmp = '\0';
		strcat(result, host);
		/*strcat(result, "*.*");*/
		strcat(result, "*");
	}
	else
	{
                char	*tmp;
                int	num;

		num = 1;
		tmp = right(host, 3);
		if (my_stricmp(tmp, "com") && my_stricmp(tmp, "edu") &&
			  my_stricmp(tmp, "net") && my_stricmp(tmp, "org") &&
			  my_stricmp(tmp, "gov") && my_stricmp(tmp, "mil") &&
			  (my_stristr(host, ".com.") ||
			  my_stristr(host, ".net.") ||
			  my_stristr(host, ".org.") ||
			  my_stristr(host, ".gov.") ||
			  my_stristr(host, ".mil.") ||
			  my_stristr(host, ".edu.")))
          		num = 2;
		while (host && *host && (numchar(host, '.') > num))
                {
			if ((host = strchr(host, '.')) != NULL)
				host++;
                        else
                                return (char *) NULL;
		}
		strcat(result, "*");
		if (my_stricmp(host, temphost))
			strcat(result, ".");
                strcat(result, host);
	}
	return result;
}

void
netfinger(name)
	char	*name;
{
        char *host;
        char fname[100], temp[2], tmpstr[255];
        struct hostent *hp;
        struct servent *sp;
        struct sockaddr_in sin;
        int s, num;
        char *rindex();
        register FILE *f;
        register int c;
        register int lastc;

        if (name)
                host = rindex(name, '@');
        if (!name || !host)
        {
            say("Invaild user@host.");
            return;
        }
        *host++ = 0;
        hp = gethostbyname(host);
        if (hp == NULL) {
                static struct hostent def;
                static struct in_addr defaddr;
                static char *alist[1];
                static char namebuf[128];

                defaddr.s_addr = inet_addr(host);
                if (defaddr.s_addr == -1) {
                        say("%s is unknown.", host);
                        return;
                }
                strcpy(namebuf, host);
                def.h_name = namebuf;
                def.h_addr_list = alist, def.h_addr = (char *)&defaddr;
                def.h_length = sizeof (struct in_addr);
                def.h_addrtype = AF_INET;
                def.h_aliases = 0;
                hp = &def;
        }
        sp = getservbyname("finger", "tcp");
        if (sp == 0) {
                say("tcp/finger: unknown service\n");
                return;
        }
        sin.sin_family = hp->h_addrtype;
        bcopy(hp->h_addr, (char *)&sin.sin_addr, hp->h_length);
        sin.sin_port = sp->s_port;
        s = socket(hp->h_addrtype, SOCK_STREAM, 0);
        if (s < 0) {
                say("socket error");
                return;
        }
        put_it("[%s]", hp->h_name);

#ifdef COMSTUD_ORIG
        if (connect(s, (char *)&sin, sizeof (sin)) < 0) {
#else
        if (connect(s, (struct sockaddr *)&sin, sizeof (sin)) < 0) {
#endif
                perror("connect");
                close(s);
                return;
        }
        write(s, "/W ", 3);
        write(s, name, strlen(name));
        write(s, "\r\n", 2);
        f = fdopen(s, "r");
        num = 0;
        strcpy(tmpstr, "");
        temp[1] = '\0';
        while ((c = getc(f)) != EOF) {
                switch(c) {
                case 0210:
                case 0211:
                case 0212:
                case 0214:
                        c -= 0200;
                        break;
                case 0215:
                        c = '\n';
                        break;
                }
                num++;
                lastc = c;
                if (isprint(c) || isspace(c))
                {
                    if ((c == '\n') || (c == '\r') || (num >= 250))
                    {
                        num = 0;
                        if (*tmpstr)
                           put_it("%s", tmpstr);
                        strcpy(tmpstr, "");
                    }
                    else
                    {
                        temp[0] = c;
                        strcat(tmpstr, temp);
                    }

                }
                else
                  break;
/*
                        putchar(c ^ 100);
*/      }
        if (lastc != '\n')
               put_it("%s", tmpstr);
        (void)fclose(f);
        return;
}

int silly_match(string1, string2)
char *string1;
char *string2;
{
	char buffer1[2048];
	char *ptr;
	char *this;

	strcpy(buffer1, string1);

	ptr = buffer1;
	this = my_next_arg(&ptr);
	while(this && *this)
	{
		if (match(this, string2) || match(string2, this))
			return 1;
		this = my_next_arg(&ptr);
	}
	return 0;
}

void com_unban(channel, arg)
char *channel;
char *arg;
{
	static char thebans[1024] = "";
	static int numbans = 0, totbans = 0;
	char modebuf[10];
	int count;
      
	if (!arg)
	{
		if (numbans)
		{
                        strcpy(modebuf, "-");
			for(count=0; count < numbans; count++)
				strcat(modebuf, "b");
			send_to_server("MODE %s %s %s",
				channel, modebuf, thebans);
		}
		if (!totbans)
                	say("No matches for the unban of %s on %s", com_arg, channel);
		numbans = 0;
                totbans = 0;
		strcpy(thebans, "");
		return;
	}
	if (silly_match(com_arg, arg))
	{
		strcat(thebans, arg);
		strcat(thebans, " ");
		numbans++;
                totbans++;
		if (numbans == get_int_var(NUM_BANMODES_VAR))
		{
			strcpy(modebuf, "-");
			for(count=0; count < numbans; count++)
				strcat(modebuf, "b");
			send_to_server("MODE %s %s %s",
				channel, modebuf, thebans);
			strcpy(thebans, "");
			numbans = 0;
		}
	}
}

void
userhost_nsl(stuff, nick, args)
WhoisStuff *stuff;
char *nick;
char *args;
{
	char *nsl;
	char *lookup;

	if (!stuff || stuff->not_on || !stuff->nick || !nick || my_stricmp(stuff->nick, nick))
		lookup = nick;
	else
		lookup = stuff->host;

	nsl = do_nslookup(lookup);
	say("%s is %s", lookup, nsl ? nsl : "unknown.");
}

void userhost_unban(stuff, nick, args)
WhoisStuff *stuff;
char *nick;
char *args;
{
        char *temp;

        if (!stuff || !stuff->nick || !nick || my_stricmp(stuff->nick, nick))
        {
                say("No match for the unban of %s on %s",
                        nick, args);
                return;
        }
	sprintf(com_arg, "%s!%s@%s", stuff->nick, stuff->user, stuff->host);
	com_action = 1;
	send_to_server("MODE %s +b", args);
}

void userhost_ignore(stuff, nick, args)
WhoisStuff *stuff;
char *nick;
char *args;
{
	char ignore_buf[1024];
	char *temp;

        if (!stuff || !stuff->nick || !nick || my_stricmp(stuff->nick, nick))
                return;
	if (stuff->not_on) {
		say("%s is not on IRC!", nick);
		return;
	}
	temp = stuff->user;
        if (temp && (*temp == '~'))
                temp++;
        if (temp && (*temp == '#'))
                temp++;
	sprintf(ignore_buf, "*%s@%s", temp, cluster(stuff->host));
	if (args)
	{
		strcat(ignore_buf, " ");
		strcat(ignore_buf, args); 
	}
	ignore(NULL, ignore_buf, NULL);
}

void userhost_ignorehost(stuff, nick, args)
WhoisStuff *stuff;
char *nick;
char *args;
{
        char ignore_buf[1024];

        if (!stuff || !stuff->nick || !nick || my_stricmp(stuff->nick, nick))
                return;
	if (stuff->not_on) {
		say("%s is not on IRC!", nick);
		return;
	}
        sprintf(ignore_buf, "*@%s", cluster(stuff->host));
        if (args)
        {
                strcat(ignore_buf, " ");
                strcat(ignore_buf, args);
        }
        ignore(NULL, ignore_buf, NULL);
}

void my_ignore(command, args)
char *command;
char *args;
{
	char ignore_buf[1024];
	char argbuf[1024];
	char *spec;

	if (spec=my_next_arg(&args))
	{
		strcpy(argbuf, get_string_var(IGN_PARAM_VAR));
		if (args && *args) {
			strcat(argbuf, " ");
			strcat(argbuf, args);
		}
		if (NULL == index(spec, '@'))
			add_to_whois_queue(spec, userhost_ignore, "%s",
				argbuf);
		else {
			char ignore_buf[1024];
			if (index(spec, '*'))
				sprintf(ignore_buf, "%s %s", spec, argbuf);
			else
				sprintf(ignore_buf, "*%s %s", cluster(spec),
					argbuf);
			ignore(NULL, ignore_buf, NULL);
		}
	}
	else
		say("No nick specified to ignore");
}

void my_ignorehost(command, args)
char *command;
char *args;
{
        char ignore_buf[1024];
	char argbuf[1024];
        char *spec;

        if (spec=my_next_arg(&args))
        {
		strcpy(argbuf, get_string_var(IGN_PARAM_VAR));
		if (args && *args) {
			strcat(argbuf, " ");
			strcat(argbuf, args);
		}
                add_to_whois_queue(spec, userhost_ignorehost, "%s",
                        argbuf);
        }
        else
                say("No nick specified to ignore");
}

void userhost_ban(stuff, nick, args)
WhoisStuff *stuff;
char *nick;
char *args;
{
	char	uh[1024];
	char	*temp;
	int	temptime;

	if (!stuff || !stuff->nick || !nick || my_stricmp(stuff->nick, nick))
	{
		say("No match for the ban of %s on %s",
			nick, args);
        	return;
        }
	temptime = atoi(next_arg(args, &args));

	temp = stuff->user;
	if (temp && (*temp == '~'))
		temp++;
	if (temp && (*temp == '#'))
		temp++;
	while (temp && (strlen(temp) > 8))
		temp++;
	sprintf(uh, "*!*%s@%s", temp, cluster(stuff->host));
	if (temptime > 0) {
		say("Ban on %s will be removed from %s in %d minute%s.",
			uh, args, temptime, (temptime == 1) ? "" : "s");
		insert_ban(args, uh, temptime);
	} else
		remove_ban(args, uh);
	send_to_server("MODE %s +b %s", args, uh);
}

void userhost_siteban(stuff, nick, args)
WhoisStuff *stuff;
char *nick;
char *args;
{
	char	*temp;
	char	uh[1024];
	int	temptime;

	if (!stuff || !stuff->nick || !nick || my_stricmp(stuff->nick, nick))
	{
		say("No match for the siteban of %s on %s",
			nick, args);
        	return;
        }

	temptime = atoi(next_arg(args, &args));
	sprintf(uh, "*!*@%s", cluster(stuff->host));
	if (temptime > 0) {
		say("Ban on %s will be removed from %s in %d minute%s.",
			uh, args, temptime, (temptime == 1) ? "" : "s");
		insert_ban(args, uh, temptime);
	} else
		remove_ban(args, uh);
	send_to_server("MODE %s +b %s", args, uh);
}

void reset(command, args)
char *command;
char *args;
{
	char e;
	e = (char) 27;
	printf("%cc%c(B%c%c[2J", e, e, e, e);
	refresh_screen();

	com_action = 0;
	toast_action = 0;
}

void do_flood(command, args)
char *command;
char *args;
{
  char buffer[1024];
  char *to, *what, *numb;
  int num, repeat;

  what = NULL;
  numb = NULL;
  if (!(to = my_next_arg(&args)))
  {
      say("D'oh!  No object specified for the flood.");
      return;
  }
  else if (!(what = my_next_arg(&args)));
  else
    numb = my_next_arg(&args);
  if (!strcmp(to, "*"))
    if ((to = get_channel_by_refnum(0))==NULL)
    {
       not_on_a_channel();
       return;
    }
  what = what ? what : "\007";
  repeat = atoi(numb ? numb : "2");
  if (repeat < 0)
    repeat = 2;

  num = 256 / (strlen(what)+2);
  strcpy(buffer, "");
  while (num--)
  {
      strcat(buffer, "\001");
      strcat(buffer, what);
      strcat(buffer, "\001");
  }
  while (repeat--)
    {
      say("Flooded the hell out of %s with %s", to, what);
      send_to_server("PRIVMSG %s :%s", to, buffer);
    }
}

void cycle(command, args)
char *command;
char *args;
{
   char *to;
   if (!(to=my_next_arg(&args)))
      to = "*";
   if (!is_channel(to) && strcmp(to, "*"))
   {
      say("Umm...I need a channel name");
      return;
   }
   if (!strcmp(to, "*"))
      if ((to = get_channel_by_refnum(0))==NULL)
      {
         not_on_a_channel();
         return;
      }
   send_to_server("PART %s", to);
   send_to_server("JOIN %s %s", to, channel_key(to));
}

void
tochanops(command, args)
	char	*command;
	char	*args;
{
   char buffer[1024];
   char *to, *themsg;
   ChannelList *Chan;
   NickList *Nick;
   int nickcount;
   int level;
   int saidit = 0;
   int method = ops_method;

   themsg = NULL;
   if (!(to=my_next_arg(&args)))
      to = "*";
   if (!is_channel(to) && strcmp(to, "*"))
   {
      themsg = to;
      if (args && *args)
         *(themsg+strlen(themsg)) = ' ';
      to="*";
   }
   if (!strcmp(to, "*"))
      if ((to = get_channel_by_refnum(0))==NULL)
      {
         not_on_a_channel();
         return;
      }
   if ((!themsg && !(themsg=args)) || !*themsg)
   {
      say("No message specified");
      return;
   }
   if (ops_method == OPS_METHOD_AUTO)
   {
	if (get_server_options(from_server) & SOptions_WallChops)
	{
		method = OPS_METHOD_WALLCHOPS;
	}
	else if (get_server_options(from_server) & SOptions_AtChannel)
	{
		method = OPS_METHOD_HYBRID;
	}
	else
	{
		method = OPS_METHOD_DEFAULT;
	}
   }
   if (method == OPS_METHOD_DEFAULT) {
      Chan = lookup_channel(to, from_server, 0);
      if (!Chan)
      {
         say("Error looking up nicks for %s", to);
         return;
      }
      strcpy(buffer, "");
      nickcount = 0;
      for (Nick = Chan->nicks; Nick; Nick = Nick->next)
      {
         if (Nick->chanop &&
             my_stricmp(Nick->nick, get_server_nickname(from_server)))
         {
            strcat(buffer, ",");
            strcat(buffer, Nick->nick);
   	    nickcount++;
   	    if (nickcount >= get_int_var(MAX_WALL_NICKS_VAR)) {
	       if ((*themsg == '-') && (*(themsg+1) == ' '))
	       {
	          (void) get_token(&themsg, " ");
	          send_to_server("PRIVMSG %s :%s", buffer+1, themsg);
	          if (!saidit) {
		     level = set_lastlog_msg_level(LOG_PUBLIC);
	    	     message_from(to, LOG_MSG);
	             say("[%s] Sent to ops on %s: %s", TimeStamp(), to, themsg);
	    	     message_from(NULL, LOG_CURRENT);
		     set_lastlog_msg_level(level);
		     saidit = 1;
	          }
	       }
	       else
	       {
	          send_to_server("NOTICE %s :\002[Wallop:%s]\002 %s", buffer+1, to, themsg);
	          if (!saidit) {
		     level = set_lastlog_msg_level(LOG_PUBLIC);
	    	     message_from(to, LOG_MSG);
	             say("[%s] Sent to ops on %s: \002[Wallop:%s]\002 %s", TimeStamp(), to, to, themsg);
	    	     message_from(NULL, LOG_MSG);
		     set_lastlog_msg_level(level);
		     saidit = 1;
	          }
	       }
	       nickcount = 0;
   	       strcpy(buffer, "");
	    }
         }
      }
      if (strlen(buffer) > 0) {
         if ((*themsg == '-') && (*(themsg+1) == ' '))
         {
            (void) get_token(&themsg, " ");
            send_to_server("PRIVMSG %s :%s", buffer+1, themsg);
            level = set_lastlog_msg_level(LOG_PUBLIC);
	    message_from(to, LOG_MSG);
            say("[%s] Sent to ops on %s: %s", TimeStamp(), to, themsg);
	    message_from(NULL, LOG_MSG);
 	    set_lastlog_msg_level(level);
         }
         else
         {
            send_to_server("NOTICE %s :\002[Wallop:%s]\002 %s", buffer+1, to,
		themsg);
            level = set_lastlog_msg_level(LOG_PUBLIC);
	    message_from(to, LOG_MSG);
            say("[%s] Sent to ops on %s: \002[Wallop:%s]\002 %s", TimeStamp(),
		to, to, themsg);
	    message_from(NULL, LOG_MSG);
 	    set_lastlog_msg_level(level);
         }
      }
   }
   else if (method == OPS_METHOD_WALLCHOPS)
   {
      int list_type;
      int format_type;

      if (is_current_channel(to, 0)) {
	 list_type = SEND_OPS_LIST;
	 format_type = SEND_OPS_FMT;
      } else {
	 list_type = SEND_OPS_OTHER_LIST;
	 format_type = SEND_OPS_OTHER_FMT;
      }
      level = set_lastlog_msg_level(LOG_PUBLIC);
      message_from(to, LOG_MSG);
      if (do_hook(list_type, "%s %s", to, themsg))
         put_it("%s", parseformat(format_type,
            get_server_nickname(get_window_server(0)), to,
            themsg));
      message_from(NULL, LOG_MSG);
      set_lastlog_msg_level(level);
      send_to_server("WALLCHOPS %s :%s", to, themsg);
   }
   else if (method == OPS_METHOD_HYBRID)
   {
      int list_type;
      int format_type;

      if (is_current_channel(to, 0)) {
	 list_type = SEND_OPS_LIST;
	 format_type = SEND_OPS_FMT;
      } else {
	 list_type = SEND_OPS_OTHER_LIST;
	 format_type = SEND_OPS_OTHER_FMT;
      } 
      level = set_lastlog_msg_level(LOG_PUBLIC);
      message_from(to, LOG_MSG);
      if (do_hook(list_type, "%s %s", to, themsg))
         put_it("%s", parseformat(format_type,
            get_server_nickname(get_window_server(0)), to,
            themsg));
      message_from(NULL, LOG_MSG);
      set_lastlog_msg_level(level);
      send_to_server("PRIVMSG @%s :%s", to, themsg);
   }
   else
   {
      say("Help!  I'm confused!  :-(");
   }
}

void bomb(command, args)
char *command;
char *args;
{
   char *to, *tag;

   if (to=my_next_arg(&args))
   {
      if (!strcmp(to, "*"))
         if ((to = get_channel_by_refnum(0))==NULL)
         {
            not_on_a_channel();
            return;
         }
      if (!(tag=my_next_arg(&args)))
         tag = "UTC";
      say("Bombed %s with %s %s", to, tag, (args && *args) ? args : "");
      if (args && *args)
         send_to_server("NOTICE %s :\001%s %s\001", to, tag, args);
      else
         send_to_server("NOTICE %s :\001%s\001", to, tag);
   }
   else
      say("No object specified to bomb");
}

void
nslookup(command, args)
char *command;
char *args;
{
  char *host, *hostname;

  if (host=my_next_arg(&args))
  {
        say("Checking tables...");
        if (!strchr(host, '.'))
	   typed_add_to_whois_queue(WHOIS_USERHOST, host, userhost_nsl, 0);
	else
        {
           hostname=do_nslookup(host);
           say("%s is %s", host, hostname ? hostname : "unknown.");
        }
  }
  else
     say("No host specified.");
}

void newnick(command, args)
char *command;
char *args;
{
    char *newnick, *newusername;

    if ((newnick = my_next_arg(&args)) &&
        (newusername = my_next_arg(&args)))
      do_newuser(newnick, newusername, args);
    else
      say("You must specify a nick and username");
}

/*
  newhost() taken from Hendrix's +multi version...
*/
void newhost(command, args)
char *command,
        *args;
{
        char    *newhname;
        struct hostent *hp;

        if (newhname = next_arg(args, &args))
        {
                strlcpy(MyHostName, newhname, sizeof(MyHostName));
                if ((hp = gethostbyname(MyHostName)) != NULL)
                {
                        bcopy(hp->h_addr, (char *) &MyHostAddr, sizeof(MyHostAddr));
                        local_ip_address.s_addr = ntohl(MyHostAddr.s_addr);
			set_string_var(VHOST_VAR, MyHostName);
                }
		do_reconnect(NULL);
        }
        else
                say("You must specify a new hostname");
}



void newuser(command, args)
char *command;
char *args;
{
    char *newusername;

    if (newusername = my_next_arg(&args))
       do_newuser(NULL, newusername, args);
    else
       say("You must specify a username.");
}

void finger(command, args)
char *command;
char *args;
{
    char *userhost;

    if (userhost=my_next_arg(&args))
    {
      if (!strchr(userhost, '@'))
      {
		  add_userhost_to_whois(userhost, userhost_finger);
                  /*say("Please specify the full user@host to finger");*/
                  return;
      } else

      netfinger(userhost);
    }
    else
      say("Please specify a user@host to finger.");
}

void multkick(command, args)
char *command;
char *args;
{
   char *to, *temp, *reason;
   reason = (char *) NULL;
   if (!(to=my_next_arg(&args)))
      to = "*";
   if (!is_channel(to) && strcmp(to, "*"))
   {
      temp = to;
      if (args && *args)
         *(temp+strlen(temp)) = ' ';
      to="*";
   }
   if (!strcmp(to, "*"))
      if ((to = get_channel_by_refnum(0))==NULL)
      {
         not_on_a_channel();
         return;
      }
   if (!temp || !*temp)
   {
      say("Who am I supposed to multikick?");
      return;
   }
   reason = strchr(temp, ':');
   if (reason)
      reason++;
   if (!reason || !*reason)
      reason = get_reason();
   say("Attempting multikick on %s", to);
   while (temp && *temp && (*temp != ':'))
      send_to_server("KICK %s %s :(MultKick) \002%s\002", to,
           my_next_arg(&temp), reason);
}

void massdeop(command, args)
char *command;
char *args;
{
	ChannelList	*chan;
	NickList	*nicks;
	char		*to, *spec, *rest;
	char		modebuf[1024];
	int		maxmodes, count, i, all;
        

	all = 0; /* deop those who are already deopped too? */
        maxmodes = get_int_var(NUM_OPMODES_VAR);
	rest = (char *) 0;
	spec = (char *) 0;
	if (!(to=my_next_arg(&args)))
		to = "*";
	if (!is_channel(to) && strcmp(to, "*"))
	{
		spec = to;
		to="*";
	}
	if (!strcmp(to, "*"))
		if ((to = get_channel_by_refnum(0)) == NULL)
		{
			not_on_a_channel();
			return;
		}
#ifdef CHECK_FOR_OPS
	if (!are_you_opped(to))
	{
		you_not_opped(to);
		return;
	}
#endif /* CHECK_FOR_OPS */
	if (!spec && !(spec = my_next_arg(&args)))
		spec = "*!*@*";
	if (*spec == '-')
	{
		rest = spec;
		spec = "*!*@*";
	}
	else
		rest = args;
	if (rest && !my_stricmp(rest, "-all"))
		all = 1;
	chan = lookup_channel(to, from_server, 0);
        if ((ChannelList *) 0 == chan)
	{
        	say("Error looking up channel %s", to);
		return;
	}
	nicks = chan->nicks;
	count = 0;
	while (nicks)
	{
        	i = 0;
		strcpy(modebuf, "");
		while (nicks && (i < maxmodes))
		{
			strcpy(buffer, "");
			strcat(buffer, nicks->nick);
			if (!nicks->user || !nicks->host)
			{
				say("ERROR: No userhost for %s", nicks->nick);
				continue;
                        }
                	strcat(buffer, "!");
                	strcat(buffer, nicks->user);
	                strcat(buffer, "@");
	                strcat(buffer, nicks->host);
			if ((all || nicks->chanop) &&
                        	my_stricmp(nicks->nick, get_server_nickname(from_server)) &&
				wild_match(spec, buffer))
			{
				count++;
				i++;
				strcat(modebuf, " ");
				strcat(modebuf, nicks->nick);
			}
                        nicks = nicks->next;
		}
		if (i)
			send_to_server("MODE %s -oooo %s", to, modebuf);
	}
	if (!count)
		say("No matches for massdeop of %s on %s", spec, to);
}

void doop(command, args)
char *command;
char *args;
{
	char *to, *temp;
	int count, max;

	count = 0;
	temp = (char *)0;
	max = get_int_var(NUM_OPMODES_VAR);
	strcpy(buffer, "");
        if (!(to=my_next_arg(&args)))
                to = "*";
        if (!is_channel(to) && strcmp(to, "*"))
        {
                temp = to;
                to="*";
        }
        if (!strcmp(to, "*"))
                if ((to = get_channel_by_refnum(0)) == NULL)
                {
                        not_on_a_channel();
                        return;
                }
#ifdef CHECK_FOR_OPS
        if (!are_you_opped(to))
        {
                you_not_opped(to);
                return;
        }
#endif /* CHECK_FOR_OPS */
	if (!temp)
		temp = my_next_arg(&args);
	while (temp && *temp)
	{
		count++;
		strcat(buffer, temp);
		if (count == max)
		{
			send_to_server("MODE %s +oooo %s", to, buffer);
			count = 0;
			strcpy(buffer, "");
		}
		else
			strcat(buffer, " ");
		temp = my_next_arg(&args);
	}
	if (count)
		send_to_server("MODE %s +oooo %s", to, buffer);
}

void dodeop(command, args)
char *command;
char *args;
{
        char *to, *temp;
        int count, max;

        count = 0;
	temp = (char *)0;
        max = get_int_var(NUM_OPMODES_VAR);
        strcpy(buffer, "");
        if (!(to=my_next_arg(&args)))
                to = "*";
        if (!is_channel(to) && strcmp(to, "*"))
        {
                temp = to;
                to="*";
        }
        if (!strcmp(to, "*"))
                if ((to = get_channel_by_refnum(0)) == NULL)
                {
                        not_on_a_channel();
                        return;
                }
#ifdef CHECK_FOR_OPS
        if (!are_you_opped(to))
        {
                you_not_opped(to);
                return;
        }
#endif /* CHECK_FOR_OPS */
	if (!temp)
		temp = my_next_arg(&args);
	while (temp && *temp)
        {
                count++;
                strcat(buffer, temp);
                if (count == max)
                {
                        send_to_server("MODE %s -oooo %s", to, buffer);
                        count = 0;
                        strcpy(buffer, "");
                }
		else
			strcat(buffer, " ");
		temp = my_next_arg(&args);
        }
        if (count)
                send_to_server("MODE %s -oooo %s", to, buffer);
}

void massop(command, args)
char *command;
char *args;
{
	ChannelList	*chan;
	NickList	*nicks;
	char		*to, *spec, *rest;
	char		modebuf[1024];
	int		maxmodes, count, i, all;
        
	all = 0;  /* op those who are already opped too? */
	maxmodes = get_int_var(NUM_OPMODES_VAR);
	rest = (char *) 0;
	spec = (char *) 0;
	if (!(to=my_next_arg(&args)))
		to = "*";
	if (!is_channel(to) && strcmp(to, "*"))
	{
		spec = to;
		to="*";
	}
	if (!strcmp(to, "*"))
		if ((to = get_channel_by_refnum(0)) == NULL)
		{
			not_on_a_channel();
			return;
		}
#ifdef CHECK_FOR_OPS
	if (!are_you_opped(to))
	{
		you_not_opped(to);
		return;
	}
#endif /* CHECK_FOR_OPS */
	if (!spec && !(spec = my_next_arg(&args)))
		spec = "*!*@*";
	if (*spec == '-')
	{
		rest = spec;
		spec = "*!*@*";
	}
	else
		rest = args;
	if (rest && !my_stricmp(rest, "-all"))
		all = 1;
	chan = lookup_channel(to, from_server, 0);
        if ((ChannelList *) 0 == chan)
	{
        	say("Error looking up channel %s", to);
		return;
	}
	nicks = chan->nicks;
	count = 0;
	while (nicks)
	{
        	i = 0;
		strcpy(modebuf, "");
		while (nicks && (i < maxmodes))
		{
			strcpy(buffer, "");
			strcat(buffer, nicks->nick);
                	strcat(buffer, "!");
                	strcat(buffer, nicks->user);
	                strcat(buffer, "@");
	                strcat(buffer, nicks->host);
			if ((all || !nicks->chanop) &&
				my_stricmp(nicks->nick, get_server_nickname(from_server)) &&
				wild_match(spec, buffer))
			{
				count++;
				i++;
				strcat(modebuf, " ");
				strcat(modebuf, nicks->nick);
			}
                        nicks = nicks->next;
		}
		if (i)
			send_to_server("MODE %s +oooo %s", to, modebuf);
	}
	if (!count)
		say("No matches for massop of %s on %s", spec, to);
}
	
void masskick(command, args)
char *command;
char *args;
{
	ChannelList	*chan;
	NickList	*nicks;
	char		*to, *spec, *rest;
	int		count, all;
        
	all = 0;  /* kick those who are opped too? */
	rest = (char *) 0;
	spec = (char *) 0;
	if (!(to=my_next_arg(&args)))
		to = "*";
	if (!is_channel(to) && strcmp(to, "*"))
	{
		spec = to;
		to="*";
	}
	if (!strcmp(to, "*"))
		if ((to = get_channel_by_refnum(0)) == NULL)
		{
			not_on_a_channel();
			return;
		}
#ifdef CHECK_FOR_OPS
	if (!are_you_opped(to))
	{
		you_not_opped(to);
		return;
	}
#endif /* CHECK_FOR_OPS */
	if (!spec && !(spec = my_next_arg(&args)))
	{
		say("No pattern given for the masskick");
		return;
	}
	if (args && !strncmp(args, "-all", 4))
        {
		all = 1;
		(void) my_next_arg(&args);
        }
	rest = args;
	if (rest && !*rest)
		rest = (char *) 0;
	chan = lookup_channel(to, from_server, 0);
        if ((ChannelList *) 0 == chan)
	{
        	say("Error looking up channel %s", to);
		return;
	}
	nicks = chan->nicks;
	count = 0;
	while (nicks)
	{
		strcpy(buffer, "");
		strcat(buffer, nicks->nick);
               	strcat(buffer, "!");
               	strcat(buffer, nicks->user);
                strcat(buffer, "@");
                strcat(buffer, nicks->host);
		if ((all || !nicks->chanop) &&
			my_stricmp(nicks->nick, get_server_nickname(from_server)) &&
			wild_match(spec, buffer))
		{
			count++;
			send_to_server("KICK %s %s :(%s) \002%s\002", to, nicks->nick, spec, rest ? rest : get_reason());
		}
		nicks = nicks->next;
	}
	if (!count)
		say("No matches for masskick of %s on %s", spec, to);
}
void masskickban(command, args)
char *command;
char *args;
{
	ChannelList	*chan;
	NickList	*nicks;
	char		*to, *spec, *rest;
	int		count, all;
	int		temptime = get_int_var(BAN_TIMEOUT_VAR);
        
	all = 0;  /* kick those who are opped too? */
	rest = (char *) 0;
	spec = (char *) 0;
	if (!(to=my_next_arg(&args)))
		to = "*";
	else
		if (is_number(to)) {
			temptime = atoi(to);
			if (temptime < 0)
				temptime = 0;
			if (!(to=my_next_arg(&args)))
				to = "*";
		}
	if (!is_channel(to) && strcmp(to, "*"))
	{
		spec = to;
		to="*";
	}
	if (!strcmp(to, "*"))
		if ((to = get_channel_by_refnum(0)) == NULL)
		{
			not_on_a_channel();
			return;
		}
#ifdef CHECK_FOR_OPS
	if (!are_you_opped(to))
	{
		you_not_opped(to);
		return;
	}
#endif /* CHECK_FOR_OPS */
	if (!spec && !(spec = my_next_arg(&args)))
	{
		say("No pattern given for the masskick");
		return;
	}
	if (args && !strncmp(args, "-all", 4))
        {
		all = 1;
		(void) my_next_arg(&args);
        }
	rest = args;
	if (rest && !*rest)
		rest = (char *) 0;
	chan = lookup_channel(to, from_server, 0);
        if ((ChannelList *) 0 == chan)
	{
        	say("Error looking up channel %s", to);
		return;
	}
	nicks = chan->nicks;
	count = 0;
	if (!strchr(spec, '!'))
	{
		char tempbuf[255];
		strcpy(tempbuf, "*!");
		if (!strchr(spec, '@'))
			strcat(tempbuf, "*@");
		strcat(tempbuf, spec);
		if (temptime > 0) {
			say("Ban on %s will be removed from %s in %d minute%s.",
				tempbuf, to, temptime, (temptime == 1) ? "" : "s");
			insert_ban(to, tempbuf, temptime);
		} else
			remove_ban(to, tempbuf);
		send_to_server("MODE %s +b %s", to, tempbuf);
	} else {
		if (temptime > 0) {
			say("Ban on %s will be removed from %s in %d minute%s.",
				spec, to, temptime, (temptime == 1) ? "" : "s");
			insert_ban(to, spec, temptime);
		} else
			remove_ban(to, spec);
        	send_to_server("MODE %s +b %s", to, spec);
	}
	while (nicks)
	{
		strcpy(buffer, "");
		strcat(buffer, nicks->nick);
               	strcat(buffer, "!");
               	strcat(buffer, nicks->user);
                strcat(buffer, "@");
                strcat(buffer, nicks->host);
		if ((all || !nicks->chanop) &&
			my_stricmp(nicks->nick, get_server_nickname(from_server)) &&
			wild_match(spec, buffer))
		{
			count++;
			send_to_server("KICK %s %s :(%s) \002%s\002", to, nicks->nick, spec, rest ? rest : get_reason());
		}
		nicks = nicks->next;
	}
	if (!count)
		say("No matches for masskickban of %s on %s", spec, to);
}

void masskickbanold(command, args)
char *command;
char *args;
{
	ChannelList	*chan;
	NickList	*nicks;
	char		*to, *spec, *rest;
	char		modebuf[1024];
	int		maxmodes, count, i, all;
        
	all = 0;  /* kick those who are opped too? */
	maxmodes = get_int_var(NUM_BANMODES_VAR);
	rest = (char *) 0;
	spec = (char *) 0;
	if (!(to=my_next_arg(&args)))
		to = "*";
	if (!is_channel(to) && strcmp(to, "*"))
	{
		spec = to;
		to="*";
	}
	if (!strcmp(to, "*"))
		if ((to = get_channel_by_refnum(0)) == NULL)
		{
			not_on_a_channel();
			return;
		}
#ifdef CHECK_FOR_OPS
	if (!are_you_opped(to))
	{
		you_not_opped(to);
		return;
	}
#endif /* CHECK_FOR_OPS */
	if (!spec && !(spec = my_next_arg(&args)))
	{
		say("No pattern given for the masskick");
		return;
	}
	if (args && !strncmp(args, "-all", 4))
        {
		all = 1;
		(void) my_next_arg(&args);
        }
	rest = args;
	if (rest && !*rest)
		rest = (char *) 0;
	chan = lookup_channel(to, from_server, 0);
        if ((ChannelList *) 0 == chan)
	{
        	say("Error looking up channel %s", to);
		return;
	}
	nicks = chan->nicks;
	count = 0;
	while (nicks)
	{
        	i = 0;
		strcpy(modebuf, "");
		while (nicks && (i < maxmodes))
		{
			strcpy(buffer, "");
			strcat(buffer, nicks->nick);
                	strcat(buffer, "!");
                	strcat(buffer, nicks->user);
	                strcat(buffer, "@");
	                strcat(buffer, nicks->host);
			if ((all || !nicks->chanop) &&
				my_stricmp(nicks->nick, get_server_nickname(from_server)) &&
				wild_match(spec, buffer))
			{
                                char *temp;
				count++;
				i++;
				temp = nicks->user;
				if (temp && (*temp == '~'))
					temp++;                                
				if (temp && (*temp == '#'))								if (temp && (*temp == '~'))
					temp++;                                
				strcat(modebuf, " ");
				strcat(modebuf, "*!*");
				strcat(modebuf, temp);
				strcat(modebuf, "@");
				strcat(modebuf, cluster(nicks->host));
				send_to_server("KICK %s %s :(%s) \002%s\002", to, nicks->nick, spec, rest ? rest : get_reason());
			}
                        nicks = nicks->next;
		}
		if (i)
		{
			char modestr[10];
                        strcpy(modestr, "");
			while (i)
			{
				strcat(modestr, "b");
				i--;
                        }
			send_to_server("MODE %s +%s %s", to, modestr, modebuf);
		}
	}
	if (!count)
		say("No matches for masskickban of %s on %s", spec, to);
}
	
void massban(command, args)
char *command;
char *args;
{
	ChannelList	*chan;
	NickList	*nicks;
	char		*to, *spec, *rest;
	char		modebuf[1024];
	int		maxmodes, count, i, all;
        
	all = 0;  /* op those who are already opped too? */
	maxmodes = get_int_var(NUM_BANMODES_VAR);
	rest = (char *) 0;
	spec = (char *) 0;
	if (!(to=my_next_arg(&args)))
		to = "*";
	if (!is_channel(to) && strcmp(to, "*"))
	{
		spec = to;
		to="*";
	}
	if (!strcmp(to, "*"))
		if ((to = get_channel_by_refnum(0)) == NULL)
		{
			not_on_a_channel();
			return;
		}
#ifdef CHECK_FOR_OPS
	if (!are_you_opped(to))
	{
		you_not_opped(to);
		return;
	}
#endif /* CHECK_FOR_OPS */
	if (!spec && !(spec = my_next_arg(&args)))
		spec = "*!*@*";
	if (*spec == '-')
	{
		rest = spec;
		spec = "*!*@*";
	}
	else
		rest = args;
	if (rest && !my_stricmp(rest, "-all"))
		all = 1;
	chan = lookup_channel(to, from_server, 0);
        if ((ChannelList *) 0 == chan)
	{
        	say("Error looking up channel %s", to);
		return;
	}
	nicks = chan->nicks;
	count = 0;
	while (nicks)
	{
        	i = 0;
		strcpy(modebuf, "");
		while (nicks && (i < maxmodes))
		{
			strcpy(buffer, "");
			strcat(buffer, nicks->nick);
                	strcat(buffer, "!");
                	strcat(buffer, nicks->user);
	                strcat(buffer, "@");
	                strcat(buffer, nicks->host);
			if ((all || !nicks->chanop) &&
				my_stricmp(nicks->nick, get_server_nickname(from_server)) &&
				wild_match(spec, buffer))
			{
				char *temp;
				count++;
				i++;
				temp = nicks->user;
				if (temp && (*temp == '~'))
					temp++;                                
				if (temp && (*temp == '#'))								if (temp && (*temp == '~'))
					temp++;                                
				strcat(modebuf, " ");
				strcat(modebuf, "*!*");
				strcat(modebuf, temp);
				strcat(modebuf, "@");
				strcat(modebuf, cluster(nicks->host));
			}
                        nicks = nicks->next;
		}
		if (i)
		{
			char modestr[10];
                        strcpy(modestr, "");
			while (i)
			{
				strcat(modestr, "b");
				i--;
                        }
			send_to_server("MODE %s +%s %s", to, modestr, modebuf);
		}
	}
	if (!count)
		say("No matches for massban of %s on %s", spec, to);
}

void unban(command, args)
char *command;
char *args;
{
	char		*to, *spec;

	spec = (char *) 0;
	if (!(to=my_next_arg(&args)))
		to = "*";
	if (!is_channel(to) && strcmp(to, "*"))
	{
		spec = to;
		to="*";
	}
	if (!strcmp(to, "*"))
		if ((to = get_channel_by_refnum(0)) == NULL)
		{
			not_on_a_channel();
			return;
		}
#ifdef CHECK_FOR_OPS
	if (!are_you_opped(to))
	{
		you_not_opped(to);
		return;
	}
#endif /* CHECK_FOR_OPS */
	if (!spec && !(spec = my_next_arg(&args)))
		spec = "*!*@*";
	if (!strchr(spec, '*'))
	{
       		add_to_whois_queue(spec, userhost_unban, "%s", to);
		return;
	}
	com_action = 1;
	if (args)
		sprintf(com_arg, "%s %s", spec, args);
	else
		strcpy(com_arg, spec);
	send_to_server("MODE %s +b", to);
}

void dokick(command, args)
char *command;
char *args;
{
   char *to, *spec, *reason;

   spec = (char *) 0;
   if (!(to=my_next_arg(&args)))
      to = "*";
   if (!is_channel(to) && strcmp(to, "*"))
   {
      spec = to;
      to = "*";
   }
   if (!strcmp(to, "*"))
      if ((to = get_channel_by_refnum(0))==NULL)
      {
         not_on_a_channel();
         return;
      }
#ifdef CHECK_FOR_OPS
   if (!are_you_opped(to))
      {
         you_not_opped(to);
         return;
      }
#endif /* CHECK_FOR_OPS */
   if (!spec && !(spec=my_next_arg(&args)))
   {
      say("No nickname specified.");
      return;
   }
   if (args && *args)
      reason = args;
   else
      reason = get_reason();
   say("Attempting kick of %s on %s", spec, to);
   send_to_server("KICK %s %s :%s", to, spec, reason); 
}

void kickban(command, args)
char *command;
char *args;
{
	char *to, *spec, *rest;
	ChannelList	*chan;
	NickList	*nicks;
	int		temptime = get_int_var(BAN_TIMEOUT_VAR);
        
	spec = (char *) 0;
	if (!(to=my_next_arg(&args)))
		to = "*";
	if (is_number(to)) {
		temptime = atoi(to);
		if (!(to=my_next_arg(&args)))
			to = "*";
	}
	if (!is_channel(to) && strcmp(to, "*"))
	{
		spec = to;
		to = "*";
	}
	if (!strcmp(to, "*"))
		if ((to = get_channel_by_refnum(0))==NULL)
		{
			not_on_a_channel();
			return;
		}
#ifdef CHECK_FOR_OPS
	if (!are_you_opped(to))
	{
		you_not_opped(to);
		return;
	}
#endif /* CHECK_FOR_OPS */
	if (!spec && !(spec=my_next_arg(&args)))
	{
		say("No nickname specified");
		return;
	}
	rest = args;
	if (rest && !*rest)
		rest = (char *) 0;
        chan = lookup_channel(to, from_server, 0);
        if ((ChannelList *) 0 == chan)
        {
                say("Error looking up channel %s", to);
                return;
        }
        nicks = chan->nicks;
        while (nicks)
        {
		if (!my_stricmp(spec, nicks->nick))
		{
			char *temp, uh[512];
			temp = nicks->user;
			if (temp && (*temp == '~'))
				temp++;
			if (temp && (*temp == '#'))
				temp++;
			while (temp && (strlen(temp) > 8))
				temp++;
			sprintf(uh, "*!*%s@%s", temp, cluster(nicks->host));
			send_to_server("MODE %s -o+b %s %s", to,
				nicks->nick, uh);
			if (temptime > 0) {
				say("Ban on %s will be removed from %s in %d minute%s.",
					uh, to, temptime, (temptime == 1) ? "" : "s");
				insert_ban(to, uh, temptime);
			} else
				remove_ban(to, uh);
			send_to_server("KICK %s %s :%s", to, nicks->nick,
				rest ? rest : get_reason());
			return;
		}
		nicks = nicks->next;
	}
	say("No match for kickban of %s on %s", spec, to);
}

void sitekickban(command, args)
char *command;
char *args;
{
	char *to, *spec, *rest;
	ChannelList	*chan;
	NickList	*nicks;
	int		temptime = get_int_var(BAN_TIMEOUT_VAR);
        
	spec = (char *) 0;
	if (!(to=my_next_arg(&args)))
		to = "*";
	if (is_number(to)) {
		temptime = atoi(to);
		if (!(to=my_next_arg(&args)))
			to = "*";
	}
	if (!is_channel(to) && strcmp(to, "*"))
	{
		spec = to;
		to = "*";
	}
	if (!strcmp(to, "*"))
		if ((to = get_channel_by_refnum(0))==NULL)
		{
			not_on_a_channel();
			return;
		}
#ifdef CHECK_FOR_OPS
	if (!are_you_opped(to))
	{
		you_not_opped(to);
		return;
	}
#endif /* CHECK_FOR_OPS */
	if (!spec && !(spec=my_next_arg(&args)))
	{
		say("No nickname specified");
		return;
	}
	rest = args;
	if (rest && !*rest)
		rest = (char *) 0;
        chan = lookup_channel(to, from_server, 0);
        if ((ChannelList *) 0 == chan)
        {
                say("Error looking up channel %s", to);
                return;
        }
        nicks = chan->nicks;
        while (nicks)
        {
		if (!my_stricmp(spec, nicks->nick))
		{
			char	uh[512];
			sprintf(uh, "*!*@%s", cluster(nicks->host));
			send_to_server("MODE %s -o+b %s %s", to,
				nicks->nick, uh);
			if (temptime > 0) {
				say("Ban on %s will be removed from %s, in %d minute%s.",
					uh, to, temptime, (temptime == 1) ? "" : "s");
				insert_ban(to, uh, temptime);
			} else
				remove_ban(to, uh);
			send_to_server("KICK %s %s :%s", to, nicks->nick,
				rest ? rest : get_reason());
			return;
		}
		nicks = nicks->next;
	}
	say("No match for sitekickban of %s on %s", spec, to);
}

void ban(command, args)
char *command;
char *args;
{
	char *to, *spec, *rest;
	ChannelList	*chan;
	NickList	*nicks;
	char		tmp[1024];
	int		temptime = get_int_var(BAN_TIMEOUT_VAR);
        
	spec = (char *) 0;
	if (!(to=my_next_arg(&args)))
		to = "*";
	else if (is_number(to)) {
		temptime = atoi(to);
		if (temptime < 0)
			temptime = 0;
		if (!(to=my_next_arg(&args)))
			to = "*";
	}
	if (!is_channel(to) && strcmp(to, "*"))
	{
		spec = to;
		to = "*";
	}
	if (!strcmp(to, "*"))
		if ((to = get_channel_by_refnum(0))==NULL)
		{
			not_on_a_channel();
			return;
		}
#ifdef CHECK_FOR_OPS
	if (!are_you_opped(to))
	{
		you_not_opped(to);
		return;
	}
#endif /* CHECK_FOR_OPS */
	if (!spec && !(spec=my_next_arg(&args)))
	{
		/*say("No nickname specified");*/
		show_tempbans();
		return;
	}
	rest = args;
	if (rest && !*rest)
		rest = (char *) 0;
	if (index(spec, '@')) {
		if (!index(spec, '!'))
			sprintf(tmp, "*!%s", spec);
		else
			strcpy((char *)&tmp, spec);
		if (temptime > 0) {
			say("Ban on %s will be removed from %s in %d minute%s.",
				tmp, to, temptime, (temptime == 1) ? "" : "s");
			insert_ban(to, tmp, temptime);
		} else
			remove_ban(to, tmp);
		send_to_server("MODE %s +b %s", to, tmp);
		return;
	}
        chan = lookup_channel(to, from_server, 0);
        if ((ChannelList *) 0 == chan)
        {
                say("Error looking up channel %s", to);
                return;
        }
        nicks = chan->nicks;
        while (nicks)
        {
		if (!my_stricmp(spec, nicks->nick))
		{
			char *temp;
			temp = nicks->user;
			if (temp && (*temp == '~'))
				temp++;
			if (temp && (*temp == '#'))
				temp++;
			while (temp && (strlen(temp) > 8))
				temp++;
			sprintf(tmp, "*!*%s@%s", temp, cluster(nicks->host));
			if (temptime > 0) {
				say("Ban on %s will be removed from %s in %d minute%s.",
					tmp, to, temptime, (temptime == 1) ? "" : "s");
				insert_ban(to, tmp, temptime);
			} else
				remove_ban(to, tmp);
			send_to_server("MODE %s -o+b %s %s", to,
				nicks->nick, tmp);
			return;
		}
		nicks = nicks->next;
	}
	sprintf(tmp, "%u %s", temptime, to);
	add_to_whois_queue(spec, userhost_ban, "%s", tmp);
}

void siteban(command, args)
char *command;
char *args;
{
	char *to, *spec, *rest;
	ChannelList	*chan;
	NickList	*nicks;
	char		tmp[1024];
	int		temptime = get_int_var(BAN_TIMEOUT_VAR);
        
	spec = (char *) 0;
	if (!(to=my_next_arg(&args)))
		to = "*";
	else if (is_number(to)) {
		temptime = atoi(to);
		if (temptime < 0)
			temptime = 0;
		if (!(to=my_next_arg(&args)))
			to = "*";
	}
	if (!is_channel(to) && strcmp(to, "*"))
	{
		spec = to;
		to = "*";
	}
	if (!strcmp(to, "*"))
		if ((to = get_channel_by_refnum(0))==NULL)
		{
			not_on_a_channel();
			return;
		}
#ifdef CHECK_FOR_OPS
	if (!are_you_opped(to))
	{
		you_not_opped(to);
		return;
	}
#endif /* CHECK_FOR_OPS */
	if (!spec && !(spec=my_next_arg(&args)))
	{
		say("No nickname specified");
		return;
	}
	rest = args;
	if (rest && !*rest)
		rest = (char *) 0;
        chan = lookup_channel(to, from_server, 0);
        if ((ChannelList *) 0 == chan)
        {
                say("Error looking up channel %s", to);
                return;
        }
        nicks = chan->nicks;
        while (nicks)
        {
		if (!my_stricmp(spec, nicks->nick))
		{
			sprintf(tmp, "*!*@%s", cluster(nicks->host));
			if (temptime > 0) {
				say("Ban on %s will be removed from %s in %d minute%s.",
					tmp, to, temptime, (temptime == 1) ? "" : "s");
				insert_ban(to, tmp, temptime);
			} else
				remove_ban(to, tmp);
			send_to_server("MODE %s -o+b %s %s", to,
				nicks->nick, tmp);
			return;
		}
		nicks = nicks->next;
	}
	sprintf(tmp, "%u %s", temptime, to);
	add_to_whois_queue(spec, userhost_siteban, "%s", tmp);
}

void users(command, args)
char *command;
char *args;
{
        ChannelList     *chan;
        NickList        *nicks;
        char            *to, *spec, *rest, *temp1;
        char            modebuf[1024];
	char		msgbuf[1024];
        int             count, ops, msg;


        rest = (char *) 0;
        spec = (char *) 0;
	temp1 = (char *) 0;
	strcpy(msgbuf, "");
        if (!(to=my_next_arg(&args)))
                to = "*";
        if (!is_channel(to) && strcmp(to, "*"))
        {
                spec = to;
                to="*";
        }
        if (!strcmp(to, "*"))
                if ((to = get_channel_by_refnum(0)) == NULL)
                {
                        not_on_a_channel();
                        return;
                }
        if (!spec && !(spec = my_next_arg(&args)))
                spec = "*!*@*";
        if (*spec == '-')
        {
                temp1 = spec;
                spec = "*!*@*";
        }
        else
                temp1 = my_next_arg(&args);
	ops = 0;
	msg = 0;
blah:
	if (temp1 && !my_stricmp(temp1, "-ops"))
		ops = 1;
	if (temp1 && !my_stricmp(temp1, "-nonops"))
		ops = 2;
	if (ops)
		temp1 = my_next_arg(&args);
       	if (temp1 && !my_stricmp(temp1, "-msg"))
		msg = 1;
	if (temp1 && !my_stricmp(temp1, "-notice"))
		msg = 2;
	if (temp1 && !my_stricmp(temp1, "-kill"))
		msg = 4;
	if (msg && !ops && args && *args == '-')
	{
		temp1 = my_next_arg(&args);
		goto blah;
	}
	if (msg && (msg != 3) && (msg != 4) && (!args || !*args))
	{
		say("No message given");
		return;
	}
	chan = lookup_channel(to, from_server, 0);
        if ((ChannelList *) 0 == chan)
        {
                say("Error looking up channel %s", to);
                return;
        }
        nicks = chan->nicks;
        count = 0;
	while (nicks)
	{
		sprintf(modebuf, "%s!%s@%s", nicks->nick,
			nicks->user ? nicks->user : "<UNKNOWN>",
			nicks->host ? nicks->host : "<UNKNOWN>");
		if (wild_match(spec, modebuf) && (!ops ||
						((ops==1)&&nicks->chanop) ||
						((ops==2)&&!nicks->chanop)))
		{
			if (msg == 4)
				if (!isme(nicks->nick))
				send_to_server("KILL %s :%s (%i", nicks->nick,
					args && *args ? args : get_reason(),
					count+1);
				else
					count--;
			else if (msg)
			{
				if (count)
					strcat(msgbuf, ",");
				strcat(msgbuf, nicks->nick);
			}
			else
				put_it("%c %s", nicks->chanop ? '@' : ' ',
					 modebuf);
			count++;
		}
		nicks = nicks->next;
	}
	if (!count)
		say("No match of %s on %s", spec, to);
	if (msg && (msg != 3) && (msg != 4) && count)
	{
		put_it("\002-> (%s)\002 %s", msgbuf, args);
		send_to_server("%s %s :%s", (msg==1) ? "PRIVMSG" : "NOTICE",
			msgbuf, args);
	}
}

void whokill(command, args)
char *command;
char *args;
{
	char *pattern;

        if (!(pattern=my_next_arg(&args)))
	{
		say("No pattern given for WHOKILL");
		return;
	}
	strcpy(reason, (args && *args) ? args : ""); 
	strcpy(com_arg, "");
	if (*pattern != '*')
		strcpy(com_arg, "*");
	strcat(com_arg, pattern);
	if ((*(com_arg+strlen(com_arg)-1) != '*') || (strlen(com_arg) == 1))
		strcat(com_arg, "*");
	say("Starting WHOKILL request for %s", com_arg);
	com_action = 2;
	send_to_server("WHO %s", com_arg);
}

void handle_masskill(nick)
char *nick;
{
	static count = 0;

	if (!nick)
	{
		count = 0;
		com_action = 0;
		strcpy(reason, "");
		return;
	}
	if (isme(nick))
		return;
	count++;
	send_to_server("KILL %s :%s (%i", nick, 
		*reason ? reason : get_reason(), count);	
}

void handle_tracekill(nick)
char *nick;
{
	char temp[20];
	if (wild_match(com_arg, nick))
	{
		if (com_action == 4)
			say("User: %s", nick);
		else
		{
			strncpy(temp, nick, 15);
			temp[15] = '\0';
			if (!strrchr(temp, '['))
				return;
			*strrchr(temp, '[') = '\0';
			handle_masskill(temp);
		}
	}
}

void tracekill(command, args)
char *command;
char *args;
{
	char *pattern;

	if (!(pattern = my_next_arg(&args)))
	{
		say("No pattern given for TRACEKILL");
		return;
	}
        strcpy(reason, (args && *args) ? args : "");
        strcpy(com_arg, "");
        if (*pattern != '*')
                strcpy(com_arg, "*");
        strcat(com_arg, pattern);
        if ((*(com_arg+strlen(com_arg)-1) != '*') || (strlen(com_arg) == 1))
                strcat(com_arg, "*");
	say("Starting TRACEKILL for %s", com_arg);
	if (*reason == '-')
		com_action = 4;
	else
		com_action = 3;
	send_to_server("TRACE");
}

void traceserv(command, args)
char *command;
char *args;
{
	char *server, *pattern;

	if (!(server = my_next_arg(&args)) ||
		!(pattern = my_next_arg(&args)))
	{
		say("Usage: /TRACESERV server pattern [<reason>]");
		return;
	}
        strcpy(reason, (args && *args) ? args : "");
        strcpy(com_arg, "");
        if (*pattern != '*')
                strcpy(com_arg, "*");
        strcat(com_arg, pattern);
        if ((*(com_arg+strlen(com_arg)-1) != '*') || (strlen(com_arg) == 1))
                strcat(com_arg, "*");
        say("Starting TRACEKILL for %s on %s", com_arg, server);
	if (*reason == '-')
		com_action = 4;
	else
	        com_action = 3;
        send_to_server("TRACE %s", server);
}

void
set_ops_method(value)
	int	value;
{
	ops_method = value;
}
