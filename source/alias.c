/*
 * alias.c Handles command aliases for irc.c 
 *
 * Written By Timothy Jensen
 *
 * Copyright (c) 1999-2001 
 *
 * See the COPYRIGHT file, or do a /HELP COPYRIGHT 
 */

#ifndef lint
static	char	rcsid[] = "@(#)$Id: alias.c,v 1.29.2.1 2002/03/07 22:29:51 toast Exp $";
#endif

#include "irc.h"

#include "alias.h"
#include "debug.h"
#include "status.h"
#include "edit.h"
#include "history.h"
#include "vars.h"
#include "ircaux.h"
#include "server.h"
#include "screen.h"
#include "window.h"
#include "input.h"
#include "names.h"
#include "server.h"
#include "output.h"
#include "names.h"
#include "comstud.h"
#include "toast.h"
#include "fileio.h"

/* from comstud.c: */
extern        char    *cluster();

extern	char	*dcc_raw_listen();
extern	char	*dcc_raw_connect();

extern	void	strmcat();
extern	void	strmcat_ue();

extern	char	*FromUserHost;

extern	int	parse_number _((char **));
static	char	*next_unit _((char *, char *, int *, int));
static	long	randm();

static	char	*alias_detected();
static	char	*alias_sent_nick();
static	char	*alias_recv_nick();
static	char	*alias_msg_body();
static	char	*alias_joined_nick();
static	char	*alias_public_nick();
static	char	*alias_dollar();
static	char	*alias_channel();
static	char	*alias_server();
static	char	*alias_query_nick();
static	char	*alias_target();
static	char	*alias_nick();
static	char	*alias_invite();
/*static	char	*alias_cmdchar();*/
static	char	*alias_line();
static	char	*alias_away();
static	char	*alias_oper();
static	char	*alias_chanop();
static	char	*alias_modes();
static	char	*alias_buffer();
static	char	*alias_time();
static	char	*alias_version();
static	char	*alias_currdir();
static	char	*alias_current_numeric();
static	char	*alias_server_version();

typedef struct
{
	char	name;
	char	*(*func)();
}	BuiltIns;

static	BuiltIns built_in[] =
{
	{ '.',		alias_sent_nick },
	{ ',',		alias_recv_nick },
	{ ':',		alias_joined_nick },
	{ ';',		alias_public_nick },
	{ '$',		alias_dollar },
	{ 'A',		alias_away },
	{ 'B',		alias_msg_body },
	{ 'C',		alias_channel },
	{ 'D',		alias_detected },
/*
	{ 'E' },
	{ 'F' },
	{ 'G' },
*/
	{ 'H', 		alias_current_numeric },
	{ 'I',		alias_invite },
/*
	{ 'J' },
*/
	{ 'K',		alias_cmdchar },
	{ 'L',		alias_line },
	{ 'M',		alias_modes },
	{ 'N',		alias_nick },
	{ 'O',		alias_oper },
	{ 'P',		alias_chanop },
	{ 'Q',		alias_query_nick },
	{ 'R',		alias_server_version },
	{ 'S',		alias_server },
	{ 'T',		alias_target },
	{ 'U',		alias_buffer },
	{ 'V',		alias_version },
	{ 'W',		alias_currdir },
/*
	{ 'X' },
	{ 'Y' },
*/
	{ 'Z',		alias_time },
	{ (char) 0,	 NULL }
};

	char	command_line[BIG_BUFFER_SIZE+1];

	char	*function_left _((char *));
	char	*function_right _((char *));
	char	*function_mid _((char *));
	char	*function_before _((char *));
	char	*function_after _((char *));
	char	*function_rand _((char *));
	char	*function_srand _((char *));
	char	*function_time _((char *));
	char	*function_stime _((char *));
	char	*function_index _((char *));
	char	*function_rindex _((char *));
	char	*function_match _((char *));
	char	*function_rmatch _((char *));
	char	*function_userhost _((char *));
	char	*function_strip _((char *));
	char	*function_encode _((unsigned char *));
	char	*function_decode _((unsigned char *));
	char	*function_ischannel _((char *));
	char	*function_ischanop _((char *));
	char	*function_word _((char *));
	char	*function_winnum _((char *));
	char	*function_winnam _((char *));
	char	*function_connect _((char *));
	char	*function_listen _((char *));
	char	*function_tdiff _((char *));
	char	*function_toupper _((char *));
	char	*function_tolower _((char *));
	char	*function_channels _((char *));
	char	*function_servers _((char *));
	char	*function_curpos _((char *));
	char	*function_onchannel _((char *));
	char	*function_pid _((char *));
	char	*function_ppid _((char *));
	char	*function_chanusers _((char *));
	char	*function_strftime _((char *));
	char    *function_cluster _((char *));
	char    *function_getkey _((char *));
	char    *function_utime _((char *));
	char    *function_tdiff2 _((char *));
	char	*function_reason _((char *));
	char	*function_clusterhost _((char *));
	char	*function_timestamp _((char *));
	char	*function_length _((char *));
	char	*function_reverse _((char *));
	char	*function_format _((char *));
	char	*function_lformat _((char *));
	char	*function_center _((char *));
	char	*function_sandr _((char *));
	char	*function_tr _((char *));
	char	*function_notword _((char *));
	char	*function_strcnt _((char *));
	char	*function_shift _((char *));
	char	*function_unshift _((char *));
	char	*function_pop _((char *));
	char	*function_push _((char *));
	char	*function_pluck _((char *));
	char	*function_remove _((char *));
	char	*function_sort _((char *));
	char	*function_rsort _((char *));
	char	*function_isort _((char *));
	char	*function_irsort _((char *));
	char	*function_jot _((char *));
	char	*function_isdigit _((char *));
	char	*function_isalpha _((char *));
	char	*function_chr _((char *));
	char	*function_ascii _((char *));
	char	*function_idle _((char *));
	char	*function_uptime _((char *));

typedef struct
{
	char	*name;
	char	*(*func)();
}	BuiltInFunctions;

static BuiltInFunctions	built_in_functions[] =
{
	{ "LEFT",		function_left },
	{ "RIGHT",		function_right },
	{ "MID",		function_mid },
	{ "BEFORE",		function_before },
	{ "AFTER",		function_after },
	{ "RAND",		function_rand },
	{ "SRAND",		function_srand },
	{ "TIME",		function_time },
	{ "TDIFF",		function_tdiff },
	{ "STIME",		function_stime },
	{ "INDEX",		function_index },
	{ "RINDEX",		function_rindex },
	{ "MATCH",		function_match },
	{ "RMATCH",		function_rmatch },
	{ "USERHOST",		function_userhost },
	{ "STRIP",		function_strip },
	{ "ENCODE",		function_encode },
	{ "DECODE",		function_decode },
	{ "ISCHANNEL",		function_ischannel },
	{ "ISCHANOP",		function_ischanop },
	{ "WORD",		function_word },
	{ "WINNUM",		function_winnum },
	{ "WINNAM",		function_winnam },
	{ "CONNECT",		function_connect },
	{ "LISTEN",		function_listen },
	{ "TOUPPER",		function_toupper },
	{ "TOLOWER",		function_tolower },
	{ "MYCHANNELS",		function_channels },
	{ "MYSERVERS",		function_servers },
	{ "CURPOS",		function_curpos },
	{ "ONCHANNEL",		function_onchannel },
	{ "PID",		function_pid },
	{ "PPID",		function_ppid },
	{ "CHANUSERS",		function_chanusers },
	{ "STRFTIME",		function_strftime },
	{ "CLUSTER",            function_cluster },

	{ "GETKEY",             function_getkey },
	{ "UTIME",              function_utime },
	{ "TDIFF2",             function_tdiff2 },
	{ "REASON",		function_reason },
	{ "CLUSTERHOST",	function_clusterhost },
	{ "RANDREAD",		function_randread },
	{ "TIMESTAMP",		function_timestamp },

	{ "LENGTH",		function_length },
	{ "REVERSE",		function_reverse },
	{ "FORMAT",		function_format },
	{ "LFORMAT",		function_lformat },
	{ "CENTER",		function_center },
	{ "SANDR",		function_sandr },
	{ "SAR",		function_sandr },
	{ "TR",			function_tr },
	{ "NOTWORD",		function_notword },
	{ "STRCNT",		function_strcnt },
	{ "SHIFT",		function_shift },
	{ "UNSHIFT",		function_unshift },
	{ "POP",		function_pop },
	{ "PUSH",		function_push },
	{ "PLUCK",		function_pluck },
	{ "REMOVE",		function_remove },
	{ "SORT",		function_sort },
	{ "RSORT",		function_rsort },
	{ "ISORT",		function_isort },
	{ "IRSORT",		function_irsort },
	{ "RISORT",		function_irsort },
	{ "FOPEN",		function_fopen },
	{ "FCLOSE",		function_fclose },
	{ "FEOF",		function_feof },
	{ "FGETC",		function_fgetc },
	{ "FWRITE",		function_fwrite },
	{ "FWRITELN",		function_fwriteln },
	{ "FPUTC",		function_fputc },
	{ "FTELL",		function_ftell },
	{ "FSEEK",		function_fseek },
	{ "FREAD",		function_fread },
	{ "FREADLN",		function_freadln },
	{ "JOT",		function_jot },

	{ "ISDIGIT",		function_isdigit },
	{ "ISALPHA",		function_isalpha },
	{ "CHR",		function_chr },
	{ "ASCII",		function_ascii },
	{ "IDLE",		function_idle },
	{ "UPTIME",		function_uptime },
	{ (char *) 0,		NULL }
};

/* alias_illegals: characters that are illegal in alias names */
	char	alias_illegals[] = " #+-*/\\()={}[]<>!@$%^~`,?;:|'\"";

Alias	*alias_list[] =
{
	(Alias *) 0,
	(Alias *) 0
};

/* alias_string: the thing that gets replaced by the $"..." construct */
static	char	*alias_string = (char *) 0;

static	int	eval_args;

/* function_stack and function_stkptr - hold the return values from functions */
static	char	*function_stack[128] =
{ 
	(char *) 0
};
static	int	function_stkptr = 0;

extern	char	*MatchingBracket();

/*
 * find_alias: looks up name in in alias list.  Returns the Alias	entry if
 * found, or null if not found.   If unlink is set, the found entry is
 * removed from the list as well.  If match is null, only perfect matches
 * will return anything.  Otherwise, the number of matches will be returned. 
 */
static	Alias	*
find_alias(list, name, unlink, match)
	Alias	**list;
	char	*name;
	int	unlink;
	int	*match;
{
	Alias	*tmp,
		*last = (Alias *) 0;
	int	cmp,
		len;
	int	(*cmp_func)();

	if (match)
	{
		*match = 0;
		cmp_func = my_strnicmp;
	}
	else
		cmp_func = my_stricmp;
	if (name)
	{
		len = strlen(name);
		for (tmp = *list; tmp; tmp = tmp->next)
		{
			if ((cmp = cmp_func(name, tmp->name, len)) == 0)
			{
				if (unlink)
				{
					if (last)
						last->next = tmp->next;
					else
						*list = tmp->next;
				}
				if (match)
				{
					(*match)++;
					if (strlen(tmp->name) == len)
					{
						*match = 0;
						return (tmp);
					}
				}
				else
					return (tmp);
			}
			else if (cmp < 0)
				break;
			last = tmp;
		}
	}
	if (match && (*match == 1))
		return (last);
	else
		return ((Alias *) 0);
}

/*
 * insert_alias: adds the given alias to the alias list.  The alias list is
 * alphabetized by name 
 */
void	
insert_alias(list, alias)
	Alias	**list;
	Alias	*alias;
{
	Alias	*tmp,
		*last,
		*foo;

	last = (Alias *) 0;
	for (tmp = *list; tmp; tmp = tmp->next)
	{
		if (strcmp(alias->name, tmp->name) < 0)
			break;
		last = tmp;
	}
	if (last)
	{
		foo = last->next;
		last->next = alias;
		alias->next = foo;
	}
	else
	{
		alias->next = *list;
		*list = alias;
	}
}

/*
 * add_alias: given the alias name and "stuff" that makes up the alias,
 * add_alias() first checks to see if the alias name is already in use... if
 * so, the old alias is replaced with the new one.  If the alias is not
 * already in use, it is added. 
 */
void	
add_alias(type, name, stuff)
	int	type;
	char	*name,
		*stuff;
{
	Alias	*tmp;
	char	*ptr;

	upper(name);
	if (type == COMMAND_ALIAS)
		say("Alias	%s added", name);
	else
	{
		if (!strcmp(name, "FUNCTION_RETURN"))
		{
			if (function_stack[function_stkptr])
				new_free(&function_stack[function_stkptr]);
			malloc_strcpy(&function_stack[function_stkptr], stuff);
			return;
		}
		if ((ptr = sindex(name, alias_illegals)) != NULL)
		{
			yell("Assign names may not contain '%c'", *ptr);
			return;
		}
		say("Assign %s added", name);
	}
	if ((tmp = find_alias(&(alias_list[type]), name, 1, (int *) 0)) ==
			(Alias *) 0)
	{
		tmp = (Alias *) new_malloc(sizeof(Alias));
		if (tmp == (Alias *) 0)
		{
			yell("Couldn't allocate memory for new alias!");
			return;
		}
		tmp->name = (char *) 0;
		tmp->stuff = (char *) 0;
	}
	malloc_strcpy(&(tmp->name), name);
	malloc_strcpy(&(tmp->stuff), stuff);
	tmp->mark = 0;
	tmp->global = loading_global;
	insert_alias(&(alias_list[type]), tmp);
}

/* alias_arg: a special version of next_arg for aliases */
static	char	*
alias_arg(str, pos)
	char	**str;
	u_int	*pos;
{
	char	*ptr;

	if (!*str)
		return (char *) 0;
	*pos = 0;
	ptr = *str;
	while (' ' == *ptr)
	{
		ptr++;
		(*pos)++;
	}
	if (*ptr == '\0')
	{
		*str = empty_string;
		return ((char *) 0);
	}
	if ((*str = sindex(ptr, " ")) != NULL)
		*((*str)++) = '\0';
	else
		*str = empty_string;
	return (ptr);
}

/* word_count: returns the number of words in the given string */
extern	int	
word_count(str)
	char	*str;
{
	int	cnt = 0;
	char	*ptr;

	while (1)
	{
		if ((ptr = sindex(str, "^ ")) != NULL)
		{
			cnt++;
			if ((str = sindex(ptr, " ")) == (char *) 0)
				return (cnt);
		}
		else
			return (cnt);
	}
}

static	char	*
built_in_alias(c)
	char	c;
{
	BuiltIns	*tmp;
	char	*ret = (char *) 0;

	for(tmp = built_in;(tmp && tmp->name);tmp++)
		if (c == tmp->name)
		{
			malloc_strcpy(&ret, tmp->func());
			break;
		}
	return(ret);
}

/*
 * find_inline: This simply looks up the given str.  It first checks to see
 * if its a user variable and returns it if so.  If not, it checks to see if
 * it's an IRC variable and returns it if so.  If not, it checks to see if
 * its and environment variable and returns it if so.  If not, it returns
 * null.  It mallocs the returned string 
 */
static	char	*
find_inline(str)
	char	*str;
{
	Alias	*alias;
	char	*ret = NULL;
	char	*tmp;

	if ((alias = find_alias(&(alias_list[VAR_ALIAS]), str, 0, (int *) NULL))
			!= NULL)
	{
		malloc_strcpy(&ret, alias->stuff);
		return (ret);
	}
	if ((strlen(str) == 1) && (ret = built_in_alias(*str)))
		return(ret);
	if ((ret = make_string_var(str)) != NULL)
		return (ret);
#ifdef DAEMON_UID
	if (getuid() == DAEMON_UID)
	malloc_strcpy(&ret, (getuid() != DAEMON_UID) && (tmp = getenv(str)) ?
				tmp : empty_string);
#else
	malloc_strcpy(&ret, (tmp = getenv(str)) ? tmp : empty_string);
#endif /* DAEMON_UID */
	return (ret);
}

static	char	*
call_function(name, f_args, args, args_flag)
	char	*name,
		*f_args,
		*args;
	int	*args_flag;
{
	char	*tmp;
	char	*result = (char *) 0;
	char	*sub_buffer = (char *) 0;
	int	builtnum;
	char	*debug_copy = (char *) 0;

	tmp = expand_alias((char *) 0, f_args, args, args_flag, NULL);
	if (get_int_var(DEBUG_VAR) & DEBUG_FUNCTIONS)
		malloc_strcpy(&debug_copy, tmp);
	for (builtnum = 0; built_in_functions[builtnum].name != NULL &&
			my_stricmp(built_in_functions[builtnum].name, name);
			builtnum++)
		;
	if (built_in_functions[builtnum].name)
		result = built_in_functions[builtnum].func(tmp);
	else
	{
		sub_buffer = new_malloc(strlen(name)+strlen(tmp)+2);
		strcpy(sub_buffer, name);
		strcat(sub_buffer, " ");
		strcat(sub_buffer, tmp);
		function_stack[++function_stkptr] = (char *) 0;
		parse_command(sub_buffer, 0, empty_string);
		new_free(&sub_buffer);
		eval_args=1;
		result = function_stack[function_stkptr];
		function_stack[function_stkptr] = (char *) 0;
		if (!result)
			malloc_strcpy(&result, empty_string);
		function_stkptr--;
	}
	if (debug_copy)
	{
		yell("Function %s(%s) returned %s",
		    name, debug_copy, result);
		new_free(&debug_copy);
	}
	new_free(&tmp);
	return result;
}


/* Given a pointer to an operator, find the last operator in the string */
char	*
lastop(ptr)
	char	*ptr;
{
	while (ptr[1] && index("!=<>&^|#+/-*", ptr[1]))
		ptr++;
	return ptr;
}

#define	NU_EXPR	0
#define	NU_CONJ NU_EXPR
#define	NU_ASSN	1
#define	NU_COMP 2
#define	NU_ADD  3
#define	NU_MULT 4
#define	NU_UNIT 5
#define	NU_TERT 6
#define	NU_BITW 8

static	char	*
next_unit(str, args, arg_flag, stage)
	char	*str,
		*args;
	int	*arg_flag,
		stage;
{
	char	*ptr,
		*ptr2,
		*right;
	int	got_sloshed = 0;
	char	*lastc;
	char	tmp[40];
	char	*result1 = (char *) 0,
		*result2 = (char *) 0;
	long	value1 = 0,
		value2,
		value3;
	char	op;
	int	display;
	char	*ArrayIndex,
		*EndIndex;

	while (isspace(*str))
		++str;
	if (!*str)
	{
		malloc_strcpy(&result1, empty_string);
		return result1;
	}
	lastc = str+strlen(str)-1;
	while (isspace(*lastc))
		*lastc-- = '\0';
	if (stage == NU_UNIT && *lastc == ')' && *str == '(')
	{
		str++, *lastc-- = '\0';
		return next_unit(str, args, arg_flag, NU_EXPR);
	}
	if (!*str)
	{
		malloc_strcpy(&result1, empty_string);
		return result1;
	}
	for (ptr = str; *ptr; ptr++)
	{
		if (got_sloshed) /* Help! I'm drunk! */
		{
			got_sloshed = 0;
			continue;
		}
		switch(*ptr)
		{
		case '\\':
			got_sloshed = 1;
			continue;
		case '(':
			if (stage != NU_UNIT || ptr == str)
			{
				if (!(ptr2 = MatchingBracket(ptr+1, '(', ')')))
					ptr = ptr+strlen(ptr)-1;
				else
					ptr = ptr2;
				break;
			}
			*ptr++ = '\0';
			right = ptr;
			ptr = MatchingBracket(right, LEFT_PAREN, RIGHT_PAREN);
			if (ptr)
				*ptr++ = '\0';
			result1 = call_function(str, right, args, arg_flag);
			if (ptr && *ptr)
			{
				malloc_strcat(&result1, ptr);
				result2 = next_unit(result1, args, arg_flag,
						stage);
				new_free(&result1);
				result1 = result2;
			}
			return result1;
		case '[':
			if (stage != NU_UNIT)
			{
				if (!(ptr2 = MatchingBracket(ptr+1, '[', ']')))
					ptr = ptr+strlen(ptr)-1;
				else
					ptr = ptr2;
				break;
			}
			*ptr++ = '\0';
			right = ptr;
			ptr = MatchingBracket(right, LEFT_BRACKET, RIGHT_BRACKET);
			if (ptr)
				*ptr++ = '\0';
			result1 = expand_alias((char *) 0, right, args, arg_flag, NULL);
			if (*str)
			{
				result2 = new_malloc(strlen(str)+
						(result1?strlen(result1):0)+
						(ptr?strlen(ptr):0) + 2);
				strcpy(result2, str);
				strcat(result2, ".");
				strcat(result2, result1);
				new_free(&result1);
				if (ptr && *ptr)
				{
					strcat(result2, ptr);
					result1 = next_unit(result2, args,
						arg_flag, stage);
				}
				else
				{
					result1 = find_inline(result2);
					if (!result1)
						malloc_strcpy(&result1,
							empty_string);
				}
				new_free(&result2);
			}
			else if (ptr && *ptr)
			{
				malloc_strcat(&result1, ptr);
				result2 = next_unit(result1, args, arg_flag,
					stage);
				new_free(&result1);
				result1 = result2;
			}
			return result1;
		case '-':
		case '+':
                        if (*(ptr+1) == *(ptr))  /* index operator */
                        {
                                char *tptr;

                                *ptr++ = '\0';
				if (ptr == str + 1)	/* Its a prefix */
                                {
                                        tptr = str + 2;
                                }
				else			/* Its a postfix */
                                {
                                        tptr = str;
                                }
			    	result1 = find_inline(tptr);
                                if (!result1) 
                                        malloc_strcpy(&result1,"0");

                                {           /* This isnt supposed to be
                                                attached to the if, so
                                                dont "fix" it. */
                                        int r;
                                        r = atoi(result1);
                                        if (*ptr == '+')
                                                r++;
                                        else    r--;
                                        sprintf(tmp,"%d",r);
                                        display = window_display;
                                        window_display = 0;
                                        add_alias(VAR_ALIAS,tptr,tmp);
                                        window_display = display;
                                }
		/* A kludge?  Cheating?  Maybe.... */
				if (ptr == str + 1) 
				{
	                                *(ptr-1) = ' ';
					*ptr = ' ';
				} else            
				{
                                        if (*ptr == '+')
					        *(ptr-1) = '-';
                                        else
                                                *(ptr-1) = '+';
					*ptr = '1';
				}
                                ptr = str; 
                                new_free(&result1);
                                break;
                        }
			if (ptr == str) /* It's unary..... do nothing */
				break;
			if (stage != NU_ADD)
			{
				ptr = lastop(ptr);
				break;
			}
			op = *ptr;
			*ptr++ = '\0';
			result1 = next_unit(str, args, arg_flag, stage);
			result2 = next_unit(ptr, args, arg_flag, stage);
			value1 = atol(result1);
			value2 = atol(result2);
			new_free(&result1);
			new_free(&result2);
			if (op == '-')
				value3 = value1 - value2;
			else
				value3 = value1 + value2;
			sprintf(tmp, "%ld", value3);
			malloc_strcpy(&result1, tmp);
			return result1;
		case '/':
		case '*':
  		case '%':
			if (stage != NU_MULT)
			{
				ptr = lastop(ptr);
				break;
			}
			op = *ptr;
			*ptr++ = '\0';
			result1 = next_unit(str, args, arg_flag, stage);
			result2 = next_unit(ptr, args, arg_flag, stage);
			value1 = atol(result1);
			value2 = atol(result2);
			new_free(&result1);
			new_free(&result2);
			if (op == '/')
			{
				if (value2)
					value3 = value1 / value2;
				else
				{
					value3 = 0;
					say("Division by zero");
				}
			}
			else
                        if (op == '*')
				value3 = value1 * value2;
                        else
                        {
                                if (value2)
                                    value3 = value1 % value2;
                                else
                                {
                                        value3 = 0;
                                        say("Mod by zero");
                                }
                        }
			sprintf(tmp, "%ld", value3);
			malloc_strcpy(&result1, tmp);
			return result1;
		case '#':
			if (stage != NU_ADD || ptr[1] != '#')
			{
				ptr = lastop(ptr);
				break;
			}
			*ptr = '\0';
			ptr += 2;
			result1 = next_unit(str, args, arg_flag, stage);
			result2 = next_unit(ptr, args, arg_flag, stage);
			malloc_strcat(&result1, result2);
			new_free(&result2);
			return result1;
	/* Reworked - Jeremy Nelson, Feb 1994
	 * & or && should both be supported, each with different
	 * stages, same with || and ^^.  Also, they should be
	 * short-circuit as well.
	 */
		case '&':
			if (ptr[0] == ptr[1])
			{
				if (stage != NU_CONJ)
				{
					ptr = lastop(ptr);
					break;
				}
				*ptr = '\0';
				ptr += 2;
				result1 = next_unit(str, args, arg_flag, stage);
				value1 = atol(result1);
				if (value1)
				{
					result2 = next_unit(ptr, args, arg_flag, stage);
					value2 = atol(result2);
					value3 = value1 && value2;
				}
				else
					value3 = 0;
				new_free(&result1);
				new_free(&result2);
                                tmp[0] = '0' + (value3?1:0);
                                tmp[1] = '\0';
				malloc_strcpy(&result1, tmp);
 				return result1;
			}
			else
			{
				if (stage != NU_BITW)
				{
					ptr = lastop(ptr);
					break;
				}
				*ptr = '\0';
				ptr += 1;
				result1 = next_unit(str, args, arg_flag, stage);
				result2 = next_unit(ptr, args, arg_flag, stage);
				value1 = atol(result1);
				value2 = atol(result2);
				new_free(&result1);
				new_free(&result2);
				value3 = value1 & value2;
                                sprintf(tmp, "%ld",value3);
				malloc_strcpy(&result1, tmp);
 				return result1;
			}
		case '|':
			if (ptr[0] == ptr[1])
			{
				if (stage != NU_CONJ)
				{
					ptr = lastop(ptr);
					break;
				}
				*ptr = '\0';
				ptr += 2;
				result1 = next_unit(str, args, arg_flag, stage);
				value1 = atol(result1);
				if (!value1)
				{
					result2 = next_unit(ptr, args, arg_flag, stage);
					value2 = atol(result2);
					value3 = value1 || value2;
				}
				else	
					value3 = 1;
				new_free(&result1);
				new_free(&result2);
				tmp[0] = '0' + (value3 ? 1 : 0);
				tmp[1] = '\0';
				malloc_strcpy(&result1, tmp);
 				return result1;
			}
			else
			{
				if (stage != NU_BITW)
				{
					ptr = lastop(ptr);
					break;
				}
				*ptr = '\0';
				ptr += 1;
				result1 = next_unit(str, args, arg_flag, stage);
				result2 = next_unit(ptr, args, arg_flag, stage);
				value1 = atol(result1);
				value2 = atol(result2);
				new_free(&result1);
				new_free(&result2);
				value3 = value1 | value2;
                                sprintf(tmp, "%ld",value3);
				malloc_strcpy(&result1, tmp);
 				return result1;
			}
		case '^':
			if (ptr[0] == ptr[1])
			{
				if (stage != NU_CONJ)
				{
					ptr = lastop(ptr);
					break;
				}
				*ptr = '\0';
				ptr += 2;
				result1 = next_unit(str, args, arg_flag, stage);
				result2 = next_unit(ptr, args, arg_flag, stage);
				value1 = atol(result1);
				value2 = atol(result2);
				value1 = value1?1:0;
				value2 = value2?1:0;
				value3 = value1 ^ value2;
				new_free(&result1);
				new_free(&result2);
				tmp[0] = '0' + (value3 ? 1 : 0);
				tmp[1] = '\0';
				malloc_strcpy(&result1, tmp);
 				return result1;
			}
			else
			{
				if (stage != NU_BITW)
				{
					ptr = lastop(ptr);
					break;
				}
				*ptr = '\0';
				ptr += 1;
				result1 = next_unit(str, args, arg_flag, stage);
				result2 = next_unit(ptr, args, arg_flag, stage);
				value1 = atol(result1);
				value2 = atol(result2);
				new_free(&result1);
				new_free(&result2);
				value3 = value1 ^ value2;
                                sprintf(tmp, "%ld",value3);
				malloc_strcpy(&result1, tmp);
 				return result1;
			}
                case '?':
                        if (stage != NU_TERT)
                        {
				ptr = lastop(ptr);
				break;
			}
			*ptr++ = '\0';
			result1 = next_unit(str, args, arg_flag, stage);
			ptr2 = index(ptr, ':');
			*ptr2++ = '\0';
                        right = result1;
                        value1 = parse_number(&right);
                        if ((value1 == -1) && (*right == (char) 0))
                                value1 = 0;
                        if ( value1 == 0 )
                                while (isspace(*right))
                                        *(right++) = '\0';
                        if ( value1 || *right )
				result2 = next_unit(ptr, args, arg_flag, stage);
			else
				result2 = next_unit(ptr2, args, arg_flag, stage);
                        *(ptr2-1) = ':';
			new_free(&result1);
			return result2;
		case '=':
			if (ptr[1] != '=')
			{
				if (stage != NU_ASSN)
				{
					ptr = lastop(ptr);
					break;
				}
				*ptr++ = '\0';
				result1 = expand_alias((char *) 0, str,
					args, arg_flag, NULL);
				result2 = next_unit(ptr, args, arg_flag, stage);
				display = window_display;
				window_display = 0;
				lastc = result1 + strlen(result1) - 1;
				while (lastc > result1 && *lastc == ' ')
					*lastc-- = '\0';
				for (ptr = result1; *ptr == ' '; ptr++);
				while ((ArrayIndex = (char *) index(ptr, '['))
						!= NULL)
				{
				    *ArrayIndex++='.';
				    if ((EndIndex = MatchingBracket(ArrayIndex,
				        LEFT_BRACKET, RIGHT_BRACKET)) != NULL)
				    {
				        *EndIndex++='\0';
				        strcat(ptr, EndIndex);
				    }
				    else
				        break;
				}
				if (*ptr)
					add_alias(VAR_ALIAS, ptr, result2);
				else
					yell("Invalid assignment expression");
				window_display = display;
				new_free(&result1);
				return result2;
			}
			if (stage != NU_COMP)
			{
				ptr = lastop(ptr);
				break;
			}
			*ptr = '\0';
			ptr += 2;
			result1 = next_unit(str, args, arg_flag, stage);
			result2 = next_unit(ptr, args, arg_flag, stage);
			if (!my_stricmp(result1, result2))
				malloc_strcpy(&result1, "1");
			else
				malloc_strcpy(&result1, "0");
			new_free(&result2);
			return result1;
		case '>':
		case '<':
			if (stage != NU_COMP)
			{
				ptr = lastop(ptr);
				break;
			}
			op = *ptr;
			if (ptr[1] == '=')
				value3 = 1, *ptr++ = '\0';
			else
				value3 = 0;
			*ptr++ = '\0';
			result1 = next_unit(str, args, arg_flag, stage);
			result2 = next_unit(ptr, args, arg_flag, stage);
			if (isdigit(*result1) && isdigit(*result2))
			{
				value1 = atol(result1);
				value2 = atol(result2);
				value1 = (value1 == value2) ? 0 : ((value1 <
					value2) ? -1 : 1);
			}
			else
				value1 = my_stricmp(result1, result2);
			if (value1)
			{
				value2 = (value1 > 0) ? 1 : 0;
				if (op == '<')
					value2 = 1 - value2;
			}
			else
				value2 = value3;
			new_free(&result2);
			sprintf(tmp, "%ld", value2);
			malloc_strcpy(&result1, tmp);
			return result1;
		case '~':
			if (ptr == str)
			{
				if (stage != NU_BITW)
					break;
				result1 = next_unit(str+1, args, arg_flag,
					stage);
				if (isdigit(*result1))
				{
					value1 = atol(result1);
					value2 = ~value1;
				}
				else
					value2 = 0;
				sprintf(tmp, "%ld", value2);
				malloc_strcpy(&result1, tmp);
				return result1;
			}
                        else
                        {
                                ptr = lastop(ptr);
                                break;
                        }
		case '!':
			if (ptr == str)
			{
				if (stage != NU_UNIT)
					break;
				result1 = next_unit(str+1, args, arg_flag,
					stage);
				if (isdigit(*result1))
				{
					value1 = atol(result1);
					value2 = value1 ? 0 : 1;
				}
				else
				{
					value2 = ((*result1)?0:1);
				}
				sprintf(tmp, "%ld", value2);
				malloc_strcpy(&result1, tmp);
				return result1;
			}
			if (stage != NU_COMP || ptr[1] != '=')
			{
				ptr = lastop(ptr);
				break;
			}
			*ptr = '\0';
			ptr += 2;
			result1 = next_unit(str, args, arg_flag, stage);
			result2 = next_unit(ptr, args, arg_flag, stage);
			if (!my_stricmp(result1, result2))
				malloc_strcpy(&result1, "0");
			else
				malloc_strcpy(&result1, "1");
			new_free(&result2);
			return result1;
                case ',': 
			/*
			 * this utterly kludge code is needed (?) to get
			 * around bugs introduced from hop's patches to
			 * alias.c.  the $, variable stopped working
			 * because of this.  -mrg, july 94.
			 */
			if (ptr == str || (ptr > str && ptr[-1] == '$'))
				break;
                        if (stage != NU_EXPR)
                        {
				ptr = lastop(ptr);
				break;
			}
			*ptr++ = '\0';
			result1 = next_unit(str, args, arg_flag, stage);
			result2 = next_unit(ptr, args, arg_flag, stage);
			new_free(&result1);
			return result2;
		}
	}
	if (stage != NU_UNIT)
		return next_unit(str, args, arg_flag, stage+1);
	if (isdigit(*str) || *str == '+' || *str == '-')
		malloc_strcpy(&result1, str);
	else
	{
		if (*str == '#' || *str=='@')
			op = *str++;
		else
			op = '\0';
		result1 = find_inline(str);
		if (!result1)
			malloc_strcpy(&result1, empty_string);
		if (op)
		{
			if (op == '#')
				value1 = word_count(result1);
			else if (op == '@')
				value1 = strlen(result1);
			sprintf(tmp, "%ld", value1);
			malloc_strcpy(&result1, tmp);
		}
	}
	return result1;
}

/*
 * parse_inline:  This evaluates user-variable expression.  I'll talk more
 * about this at some future date. The ^ function and some fixes by
 * troy@cbme.unsw.EDU.AU (Troy Rollo) 
 */
char	*
parse_inline(str, args, args_flag)
	char	*str;
	char	*args;
	int	*args_flag;
{
	return next_unit(str, args, args_flag, NU_EXPR);
}

/*
 * arg_number: Returns the argument 'num' from 'str', or, if 'num' is
 * negative, returns from argument 'num' to the end of 'str'.  You might be
 * wondering what's going on down there... here goes.  First we copy 'str' to
 * malloced space.  Then, using next_arg(), we strip out each argument ,
 * putting them in arg_list, and putting their position in the original
 * string in arg_list_pos.  Anyway, once parsing is done, the arguments are
 * returned directly from the arg_list array... or in the case of negative
 * 'num', the arg_list_pos is used to return the postion of the rest of the
 * args in the original string... got it?  Anyway, the bad points of the
 * routine:  1) Always parses out everything, even if only one arg is used.
 * 2) The malloced stuff remains around until arg_number is called with a
 * different string. Even then, some malloced stuff remains around.  This can
 * be fixed. 
 */

#define	LAST_ARG 8000

extern	char	*arg_number(lower_lim, upper_lim, str)
int	lower_lim,
	upper_lim;
char	*str;
{
	char	*ptr,
		*arg,
		c;
	int	use_full = 0;
	unsigned int	pos,
		start_pos;
	static	char	*last_args = (char *) 0;
	static	char	*last_range = (char *) 0;
	static	char	**arg_list = (char **) 0;
	static	unsigned int	*arg_list_pos = (unsigned int *) 0;
	static	unsigned int	*arg_list_end_pos = (unsigned int *) 0;
	static	int	arg_list_size;

	if (eval_args)
	{
		int	arg_list_limit;

		eval_args = 0;
		new_free(&arg_list);
		new_free(&arg_list_pos);
		new_free(&arg_list_end_pos);
		arg_list_size = 0;
		arg_list_limit = 10;
		arg_list = (char **) new_malloc(sizeof(char *) *
			arg_list_limit);
		arg_list_pos = (unsigned int *) new_malloc(sizeof(unsigned int)
			* arg_list_limit);
		arg_list_end_pos = (unsigned int *) new_malloc(sizeof(unsigned
			int) * arg_list_limit);
		malloc_strcpy(&last_args, str);
		ptr = last_args;
		pos = 0;
		while ((arg = alias_arg(&ptr, &start_pos)) != NULL)
		{
			arg_list_pos[arg_list_size] = pos;
			pos += start_pos + strlen(arg);
			arg_list_end_pos[arg_list_size] = pos++;
			arg_list[arg_list_size++] = arg;
			if (arg_list_size == arg_list_limit)
			{
				arg_list_limit += 10;
				arg_list = (char **) new_realloc(arg_list,
					sizeof(char *) * arg_list_limit);
				arg_list_pos = (unsigned int *)
					new_realloc(arg_list_pos,
					sizeof(unsigned int) * arg_list_limit);
				arg_list_end_pos = (unsigned int *)
					new_realloc(arg_list_end_pos,
					sizeof(unsigned int) *
					arg_list_limit);
			}
		}
	}
	if (upper_lim == LAST_ARG && lower_lim == LAST_ARG)
		upper_lim = lower_lim = arg_list_size - 1;
	if (arg_list_size == 0)
		return (empty_string);
	if ((upper_lim >= arg_list_size) || (upper_lim < 0))
	{
		use_full = 1;
		upper_lim = arg_list_size - 1;
	}
	if (upper_lim < lower_lim)
                return empty_string;
	if (lower_lim >= arg_list_size)
		lower_lim = arg_list_size - 1;
	else if (lower_lim < 0)
		lower_lim = 0;
	if ((use_full == 0) && (lower_lim == upper_lim))
		return (arg_list[lower_lim]);
	c = *(str + arg_list_end_pos[upper_lim]);
	*(str + arg_list_end_pos[upper_lim]) = (char) 0;
	malloc_strcpy(&last_range, str + arg_list_pos[lower_lim]);
	*(str + arg_list_end_pos[upper_lim]) = c;
	return (last_range);
}

/*
 * parse_number: returns the next number found in a string and moves the
 * string pointer beyond that point	in the string.  Here's some examples: 
 *
 * "123harhar"  returns 123 and str as "harhar" 
 *
 * while: 
 *
 * "hoohar"     returns -1  and str as "hoohar" 
 */
extern	int	
parse_number(str)
	char	**str;
{
	int	ret;
	char	*ptr;

	ptr = *str;
	if (isdigit(*ptr))
	{
		ret = atoi(ptr);
		for (; isdigit(*ptr); ptr++);
		*str = ptr;
	}
	else
		ret = -1;
	return (ret);
}

void	
do_alias_string()
{
	malloc_strcpy(&alias_string, get_input());
	irc_io_loop = 0;
}

/*
 * expander_addition: This handles string width formatting for irc variables
 * when [] is specified.  
 */
static	void	
expander_addition(buff, add, length, quote_em)
	char	*buff,
		*add;
	int	length;
	char	*quote_em;
{
	char	format[40],
		*ptr;

	if (length)
	{
		sprintf(format, "%%%d.%ds", -length, (length < 0 ? -length :
			length));
		sprintf(buffer, format, add);
		add = buffer;
	}
	if (quote_em)
	{
		ptr = double_quote(add, quote_em);
		strmcat(buff, ptr, BIG_BUFFER_SIZE);
		new_free(&ptr);
	}
	else
                if (buff)
		        strmcat(buff, add, BIG_BUFFER_SIZE);
}

/* MatchingBracket returns the next unescaped bracket of the given type */
char	*
MatchingBracket(string, left, right)
	char	*string;
	char	left,
		right;
{
	int	bracket_count = 1;

	while (*string && bracket_count)
	{
		if (*string == left)
			bracket_count++;
		else if (*string == right)
		{
			if (!--bracket_count)
				return string;
		}
		else if (*string == '\\' && string[1])
			string++;
		string++;
	}
	return (char *) 0;
}


/*
 * alias_special_char: Here we determin what to do with the character after
 * the $ in a line of text. The special characters are described more fulling
 * in the help/ALIAS file.  But they are all handled here. Paremeters are the
 * name of the alias (if applicable) to prevent deadly recursion, a
 * destination buffer (of size BIG_BUFFER_SIZE) to which things are appended,
 * a ptr to the string (the first character of which is the special
 * character, the args to the alias, and a character indication what
 * characters in the string should be quoted with a backslash.  It returns a
 * pointer to the character right after the converted alias.

 The args_flag is set to 1 if any of the $n, $n-, $n-m, $-m, $*, or $() is used
 in the alias.  Otherwise it is left unchanged.
 */
/*ARGSUSED*/
static	char	*
alias_special_char(name, buffer, ptr, args, quote_em,args_flag)
	char	*name;
	char	*buffer;
	char	*ptr;
	char	*args;
	char	*quote_em;
	int	*args_flag;
{
	char	*tmp,
		c;
	int	upper,
	lower,
	length;

	length = 0;
	if ((c = *ptr) == LEFT_BRACKET)
	{
		ptr++;
		if ((tmp = (char *) index(ptr, RIGHT_BRACKET)) != NULL)
		{
			*(tmp++) = (char) 0;
			length = atoi(ptr);
			ptr = tmp;
			c = *ptr;
		}
		else
		{
			say("Missing %c", RIGHT_BRACKET);
			return (ptr);
		}
	}
	tmp = ptr+1;
	switch (c)
	{
	case LEFT_PAREN:
		{
			char	sub_buffer[BIG_BUFFER_SIZE+1];

			if ((ptr = MatchingBracket(tmp, LEFT_PAREN,
			    RIGHT_PAREN)) || (ptr = (char *) index(tmp,
			    RIGHT_PAREN)))
				*(ptr++) = (char) 0;
			tmp = expand_alias((char *) 0, tmp, args, args_flag,
				NULL);
			*sub_buffer = (char) 0;
			alias_special_char((char *) 0, sub_buffer, tmp,
				args, quote_em,args_flag);
			expander_addition(buffer, sub_buffer, length, quote_em);
			new_free(&tmp);
			*args_flag = 1;
		}
		return (ptr);
	case '!':
		if ((ptr = (char *) index(tmp, '!')) != NULL)
			*(ptr++) = (char) 0;
		if ((tmp = do_history(tmp, empty_string)) != NULL)
		{
			expander_addition(buffer, tmp, length, quote_em);
			new_free(&tmp);
		}
		return (ptr);
	case LEFT_BRACE:
		if ((ptr = (char *) index(tmp, RIGHT_BRACE)) != NULL)
			*(ptr++) = (char) 0;
		if ((tmp = parse_inline(tmp, args, args_flag)) != NULL)
		{
			expander_addition(buffer, tmp, length, quote_em);
			new_free(&tmp);
		}
		return (ptr);
	case DOUBLE_QUOTE:
		if ((ptr = (char *) index(tmp, DOUBLE_QUOTE)) != NULL)
			*(ptr++) = (char) 0;
		alias_string = (char *) 0;
		if (irc_io(tmp, do_alias_string, use_input, 1))
		{
			yell("Illegal recursive edit");
			break;
		}
		expander_addition(buffer, alias_string, length, quote_em);
		new_free(&alias_string);
		return (ptr);
	case '*':
		expander_addition(buffer, args, length, quote_em);
		*args_flag = 1;
		return (ptr + 1);
	default:
		if (isdigit(c) || (c == '-') || c == '~')
		{
			*args_flag = 1;
			if (*ptr == '~')
			{
				lower = upper = LAST_ARG;
				ptr++;
			}
			else
			{
				lower = parse_number(&ptr);
				if (*ptr == '-')
				{
					ptr++;
					upper = parse_number(&ptr);
				}
				else
					upper = lower;
			}
			expander_addition(buffer, arg_number(lower, upper,
				args), length, quote_em);
			return (ptr ? ptr : empty_string);
		}
		else
		{
			char	*rest,
				c = (char) 0;

		/*
		 * Why use ptr+1?  Cause try to maintain backward compatability
		 * can be a pain in the butt.  Basically, we don't want any of
		 * the illegal characters in the alias, except that things like
		 * $* and $, were around first, so they must remain legal.  So
		 * we skip the first char after the $.  Does this make sense?
		 */
			/* special case for $ */
			if (*ptr == '$')
			{
				rest = ptr+1;
				c = *rest;
				*rest = (char) 0;
			}
			else if ((rest = sindex(ptr+1, alias_illegals)) != NULL)
			{
				if (isalpha(*ptr) || *ptr == '_')
					while ((*rest == LEFT_BRACKET ||
					    *rest == LEFT_PAREN) &&
					    (tmp = MatchingBracket(rest+1,
					    *rest, (*rest == LEFT_BRACKET) ?
					    RIGHT_BRACKET: RIGHT_PAREN)))
						rest = tmp + 1;
				c = *rest;
				*rest = (char) 0;
			}
			if ((tmp = parse_inline(ptr, args, args_flag)) != NULL)
			{
				expander_addition(buffer, tmp, length,
					quote_em);
				new_free(&tmp);
			}
			if (rest)
				*rest = c;
			return(rest);
		}
	}
	return NULL;
}


/*
 * expand_alias: Expands inline variables in the given string and returns the
 * expanded string in a new string which is malloced by expand_alias(). 
 *
 * Also unescapes anything that was quoted with a backslash
 *
 * Behaviour is modified by the following:
 *	Anything between brackets (...) {...} is left unmodified.
 *	If more_text is supplied, the text is broken up at
 *		semi-colons and returned one at a time. The unprocessed
 *		portion is written back into more_text.
 *	Backslash escapes are unescaped.
 */

char	*
expand_alias(name, string, args,args_flag, more_text)
	char	*name,
		*string,
		*args;
	int	*args_flag;
	char	**more_text;
{
	char	buffer[BIG_BUFFER_SIZE + 1],
		*ptr,
		*stuff = (char *) 0,
		*free_stuff;
	char	*quote_em,
		*quote_str = (char *) 0;
	char	ch;
	int	quote_cnt = 0;
	int	is_quote = 0;
	void	(*str_cat)();

	if (*string == '@' && more_text)
	{
		str_cat = strmcat;
		*args_flag = 1; /* Stop the @ command from auto appending */
	}
	else
		str_cat = strmcat_ue;
	malloc_strcpy(&stuff, string);
	free_stuff = stuff;
	*buffer = (char) 0;
	eval_args = 1;
	ptr = stuff;
	if (more_text)
		*more_text = NULL;
	while (ptr && *ptr)
	{
		if (is_quote)
		{
			is_quote = 0;
			++ptr;
			continue;
		}
		switch(*ptr)
		{
		case '$':
	/*
	 * The test here ensures that if we are in the expression
	 * evaluation command, we don't expand $. In this case we
	 * are only coming here to do command separation at ';'s.
	 * If more_text is not defined, and the first character is
	 * '@', we have come here from [] in an expression.
	 */
			if (more_text && *string == '@')
			{
				ptr++;
				break;
			}
			*(ptr++) = (char) 0;
			(*str_cat)(buffer, stuff, BIG_BUFFER_SIZE);
			while (*ptr == '^')
			{
				ptr++;
				if (quote_str)
					quote_str = (char *)
						new_realloc(quote_str,
						sizeof(char) * (quote_cnt + 2));
				else
					quote_str = (char *)
						new_malloc(sizeof(char) *
						(quote_cnt + 2));
				quote_str[quote_cnt++] = *(ptr++);
				quote_str[quote_cnt] = (char) 0;
			}
			quote_em = quote_str;
			stuff = alias_special_char(name, buffer, ptr, args,
				quote_em, args_flag);
			if (stuff)
				new_free(&quote_str);
			quote_cnt = 0;
			ptr = stuff;
			break;
		case ';':
			if (!more_text)
			{
				ptr++;
				break;
			}
			*more_text = string + (ptr - free_stuff) +1;
			*ptr = '\0'; /* To terminate the loop */
			break;
		case LEFT_PAREN:
		case LEFT_BRACE:
			ch = *ptr;
			*ptr = '\0';
			(*str_cat)(buffer, stuff, BIG_BUFFER_SIZE);
			stuff = ptr;
			*args_flag = 1;
			if (!(ptr = MatchingBracket(stuff + 1, ch,
					(ch == LEFT_PAREN) ?
					RIGHT_PAREN : RIGHT_BRACE)))
			{
				yell("Unmatched %c", ch);
				ptr = stuff + strlen(stuff+1)+1;
			}
			else
				ptr++;
			*stuff = ch;
			ch = *ptr;
			*ptr = '\0';
			strmcat(buffer, stuff, BIG_BUFFER_SIZE);
			stuff = ptr;
			*ptr = ch;
			break;
		case '\\':
			is_quote = 1;
			ptr++;
			break;
		default:
			ptr++;
			break;
		}
	}
	if (stuff)
		(*str_cat)(buffer, stuff, BIG_BUFFER_SIZE);
	ptr = (char *) 0;
	new_free(&free_stuff);
	malloc_strcpy(&ptr, buffer);
	if (get_int_var(DEBUG_VAR) & DEBUG_EXPANSIONS)
		yell("Expanded [%s] to [%s]",
			string, ptr);
	return (ptr);
}

/*
 * get_alias: returns the alias matching 'name' as the function value. 'args'
 * are expanded as needed, etc.  If no matching alias is found, null is
 * returned, cnt is 0, and full_name is null.  If one matching alias is
 * found, it is retuned, with cnt set to 1 and full_name set to the full name
 * of the alias.  If more than 1 match are found, null is returned, cnt is
 * set to the number of matches, and fullname is null. NOTE: get_alias()
 * mallocs the space for the full_name, but returns the actual value of the
 * alias if found! 
 */
char	*
get_alias(type, name, cnt, full_name)
	int	type;
	char	*name,
		**full_name;
	int	*cnt;
{
	Alias	*tmp;

	*full_name = (char *) 0;
	if ((name == (char *) 0) || (*name == (char) 0))
	{
		*cnt = 0;
		return ((char *) 0);
	}
	if ((tmp = find_alias(&(alias_list[type]), name, 0, cnt)) != NULL)
	{
		if (*cnt < 2)
		{
			malloc_strcpy(full_name, tmp->name);
			return (tmp->stuff);
		}
	}
	return ((char *) 0);
}

/*
 * match_alias: this returns a list of alias names that match the given name.
 * This is used for command completion etc.  Note that the returned array is
 * malloced in this routine.  Returns null if no matches are found 
 */
char	**
match_alias(name, cnt, type)
	char	*name;
	int	*cnt;
	int	type;
{
	Alias	*tmp;
	char	**matches = (char **) 0;
	int	matches_size = 5;
	int	len;
	char	*last_match = (char *) 0;
	char	*dot;

	len = strlen(name);
	*cnt = 0;
	matches = (char	**) new_malloc(sizeof(char *) * matches_size);
	for (tmp = alias_list[type]; tmp; tmp = tmp->next)
	{
		if (strncmp(name, tmp->name, len) == 0)
		{
			if ((dot = (char *) index(tmp->name+len, '.')) != NULL)
			{
				if (type == COMMAND_ALIAS)
					continue;
				else
				{
					*dot = '\0';
					if (last_match && !strcmp(last_match,
							tmp->name))
					{
						*dot = '.';
						continue;
					}
				}
			}
			matches[*cnt] = (char *) 0;
			malloc_strcpy(&(matches[*cnt]), tmp->name);
			last_match = matches[*cnt];
			if (dot)
				*dot = '.';
			if (++(*cnt) == matches_size)
			{
				matches_size += 5;
				matches = (char	**) new_realloc(matches,
					sizeof(char *) * matches_size);
			}
		}
		else if (*cnt)
			break;
	}
	if (*cnt)
	{
		matches = (char	**) new_realloc(matches, sizeof(char *) *
			(*cnt + 1));
		matches[*cnt] = (char *) 0;
	}
	else
		new_free(&matches);
	return (matches);
}

/* delete_alias: The alias name is removed from the alias list. */
void
delete_alias(type, name)
	int	type;
	char	*name;
{
	Alias	*tmp;

	upper(name);
	if ((tmp = find_alias(&(alias_list[type]), name, 1, (int *) NULL))
			!= NULL)
	{
		new_free(&(tmp->name));
		new_free(&(tmp->stuff));
		new_free(&tmp);
		if (type == COMMAND_ALIAS)
			say("Alias	%s removed", name);
		else
			say("Assign %s removed", name);
	}
	else
		say("No such alias: %s", name);
}

/*
 * list_aliases: Lists all aliases matching 'name'.  If name is null, all
 * aliases are listed 
 */
void
list_aliases(type, name)
	int	type;
	char	*name;
{
	Alias	*tmp;
	int	len;
	int	DotLoc,
		LastDotLoc = 0;
	char	*LastStructName = NULL;
	char	*s;

	if (type == COMMAND_ALIAS)
		say("Aliases:");
	else
		say("Assigns:");
	if (name)
	{
		upper(name);
		len = strlen(name);
	}
	else
		len = 0;
	for (tmp = alias_list[type]; tmp; tmp = tmp->next)

	{
		if (!name || !strncmp(tmp->name, name, len))
		{
			s = index(tmp->name + len, '.');
			if (!s)
				say("\t%s\t%s", tmp->name, tmp->stuff);
			else
			{
				DotLoc = s - tmp->name;
				if (!LastStructName || (DotLoc != LastDotLoc) || strncmp(tmp->name, LastStructName, DotLoc))
				{
					say("\t%*.*s\t<Structure>", DotLoc, DotLoc, tmp->name);
					LastStructName = tmp->name;
					LastDotLoc = DotLoc;
				}
			}
		}
	}
}

/*
 * mark_alias: sets the mark field of the given alias to 'flag', and returns
 * the previous value of the mark.  If the name is not found, -1 is returned.
 * This is used to prevent recursive aliases by marking and unmarking
 * aliases, and not reusing an alias that has previously been marked.  I'll
 * explain later 
 */
int
mark_alias(name, flag)
	char	*name;
	int	flag;
{
	int	old_mark;
	Alias	*tmp;
	int	match;

	if ((tmp = find_alias(&(alias_list[COMMAND_ALIAS]), name, 0, &match))
			!= NULL)
	{
		if (match < 2)
		{
			old_mark = tmp->mark;
		/* New handling of recursion */
			if (flag)
			{
				int	i;
				/* Count recursion */

				tmp->mark = tmp->mark + flag;
				if ((i = get_int_var(MAX_RECURSIONS_VAR)) > 1)
				{
					if (tmp->mark > i)
					{
						tmp->mark = 0;
						return(1); /* MAX exceeded. */
					}
					else return(0);
				/* In recursion but it's ok */
				}
				else
				{
					if (tmp->mark > 1)
					{
						tmp->mark = 0;
						return(1);
				/* max of 1 here.. exceeded */
					}
					else return(0);
				/* In recursion but it's ok */
				}
			}
			else
		/* Not in recursion at all */
			{
				tmp->mark = 0;
				return(old_mark);
			/* This one gets ignored anyway */
			}
		}
	}
	return (-1);
}

/*
 * execute_alias: After an alias has been identified and expanded, it is sent
 * here for proper execution.  This routine mainly prevents recursive
 * aliasing.  The name is the full name of the alias, and the alias is
 * already expanded alias (both of these parameters are returned by
 * get_alias()) 
 */
void
execute_alias(alias_name, alias, args)
	char	*alias_name,
		*alias,
		*args;
{
	if (mark_alias(alias_name, 1))
		say("Maximum recursion count exceeded in: %s", alias_name);
	else
	{
		parse_line(alias_name, alias, args, 0,1);
		mark_alias(alias_name, 0);
	}
}

/*
 * save_aliases: This will write all of the aliases to the FILE pointer fp in
 * such a way that they can be read back in using LOAD or the -l switch 
 */
void
save_aliases(fp, do_all)
	FILE	*fp;
	int	do_all;
{
	Alias	*tmp;

	for (tmp = alias_list[VAR_ALIAS]; tmp; tmp = tmp->next)
		if (!tmp->global || do_all)
			fprintf(fp, "ASSIGN %s %s\n", tmp->name, tmp->stuff);
	for (tmp = alias_list[COMMAND_ALIAS]; tmp; tmp = tmp->next)
		if (!tmp->global || do_all)
			fprintf(fp, "ALIAS %s %s\n", tmp->name, tmp->stuff);
}

/* The Built-In Alias expando functions */
static	char	*
alias_line()
{
	return(get_input());
}

static	char	*
alias_buffer()
{
	return cut_buffer;
}

static	char	*
alias_time()
{
	return(update_clock(GET_TIME));
}

static	char	*
alias_dollar()
{
	return("$");
}

static	char	*
alias_detected()
{
	return last_notify_nick;
}

static	char	*
alias_nick()
{
	return(get_server_nickname(curr_scr_win->server));
}

static	char	*
alias_away()
{
	return server_list[curr_scr_win->server].away;
}

static	char	*
alias_sent_nick()
{
	return (sent_nick) ? sent_nick : empty_string;
}

static	char	*
alias_recv_nick()
{
	return (recv_nick) ? recv_nick : empty_string;
}

static	char	*
alias_msg_body()
{
	return (sent_body) ? sent_body : empty_string;
}

static	char	*
alias_joined_nick()
{
	return (joined_nick) ? joined_nick : empty_string;
}

static	char	*
alias_public_nick()
{
	return (public_nick) ? public_nick : empty_string;
}

static	char	*
alias_channel()
{
	char	*tmp;

	return (tmp = get_channel_by_refnum(0)) ? tmp : "0";
}

static	char	*
alias_server()
{
	return (parsing_server_index == -1) ?
		get_server_itsname(parsing_server_index) :
		(get_window_server(0) != -1) ?
			get_server_itsname(get_window_server(0)) : empty_string;
}

static	char	*
alias_query_nick()
{
	char	*tmp;

	return (tmp = query_nick()) ? tmp : empty_string;
}

static	char	*
alias_target()
{
	char	*tmp;

	return (tmp = get_target_by_refnum(0)) ? tmp : empty_string;
}

static	char	*
alias_invite()
{
	return (invite_channel) ? invite_channel : empty_string;
}

char	*
alias_cmdchar()
{
	static	char	thing[2];
	char	*cmdchars;

	if ((cmdchars = get_string_var(CMDCHARS_VAR)) == (char *) 0)
		cmdchars = DEFAULT_CMDCHARS;
	thing[0] = cmdchars[0];
	thing[1] = (char) 0;
	return(thing);
}

static	char	*
alias_oper()
{
	return get_server_operator(from_server) ?
		get_string_var(STATUS_OPER_VAR) : empty_string;
}

static	char	*
alias_chanop()
{
	char	*tmp;

	return ((tmp = get_channel_by_refnum(0)) && get_channel_oper(tmp,
			from_server)) ?
		"@" : empty_string;
}

static	char	*
alias_modes()
{
	char	*tmp;

	return (tmp = get_channel_by_refnum(0)) ?
		get_channel_mode(tmp, from_server) : empty_string;
}

/*
 * alias: the /ALIAS command.  Calls the correct alias function depending on
 * the args 
 */
void	
alias(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	char	*name,
		*rest;
	int	type;
	char	*ArrayIndex;
	char	*EndIndex;

	type = *command - 48;	/*
				 * A trick!  Yes, well, what the hell.  Note
                                 * the the command part of ALIAS is "0" and
                                 * the command part of ASSIGN is "1" in the
				 * command array list
				 */
	if ((name = next_arg(args, &rest)) != NULL)
	{
		while ((ArrayIndex = (char *) index(name, '[')) != NULL)
		{
			*ArrayIndex++ = '.';
			if ((EndIndex = MatchingBracket(ArrayIndex,
					LEFT_BRACKET, RIGHT_BRACKET)) != NULL)
			{
				*EndIndex++ = '\0';
				strcat(name, EndIndex);
			}
			else
				break;
		}
		if (*rest)
		{
			if (*rest == LEFT_BRACE)
			{
				char	*ptr = MatchingBracket(++rest,
						LEFT_BRACE, RIGHT_BRACE);
				if (!ptr)
				    say("Unmatched brace in ALIAS or ASSIGN");
				else if (ptr[1])
				{
					say("Junk after closing brace in ALIAS or ASSIGN");
				}
				else
				{
					*ptr = '\0';
					add_alias(type, name, rest);
				}
			}
			else
				add_alias(type, name, rest);
		}
		else
		{
			if (*name == '-')
			{
				if (*(name + 1))
					delete_alias(type, name + 1);
				else
					say("You must specify an alias to be removed");
			}
			else
				list_aliases(type, name);
		}
	}
	else
		list_aliases(type, (char *) 0);
}



char	*
function_left(input)
	char	*input;
{
	char	*result = (char *) 0;
	char	*count;
	int	cvalue;

	count = next_arg(input, &input);
	if (count)
		cvalue = atoi(count);
	else
		cvalue = 0;
	if (strlen(input) > cvalue)
		input[cvalue] = '\0';
	malloc_strcpy(&result, input);
	return result;
}

char	*
function_right(input)
	char	*input;
{
	char	*result = (char *) 0;
	char	*count;
	int	cvalue;

	count = next_arg(input, &input);
	if (count)
		cvalue = atoi(count);
	else
		cvalue = 0;
	if (strlen(input) > cvalue)
		input += strlen(input) - cvalue;
	malloc_strcpy(&result, input);
	return result;
}

char	*
function_mid(input)
	char	*input;
{
	char	*result = (char *) 0;
	char	*mid_index;
	int	ivalue;
	char	*count;
	int	cvalue;

	mid_index = next_arg(input, &input);
	if (mid_index)
		ivalue = atoi(mid_index);
	else
		ivalue = 0;
	count = next_arg(input, &input);
	if (count)
		cvalue = atoi(count);
	else
		cvalue = 0;
	if (strlen(input) > ivalue)
		input += ivalue;
	else
		*input = (char) 0;
	if (strlen(input) > cvalue)
		input[cvalue] = (char) 0;
	malloc_strcpy(&result, input);
	return result;
}

char	*
function_before(input)
	char	*input;
{
	char	*result = NULL;
	char	*find,
		*index;
	int	which,
		count;

	malloc_strcpy(&result, empty_string);

	find = next_arg(input, &input);
	if (find && *find)
	{
		which = atoi(find);	
		if (which)
		{
			find = next_arg(input, &input);
			if (!find || !*find)
			{
				return result;
			}
			if (which < 0)
			{
				count = countstr(input, find);
				which = count + which + 1;
				if (which < 1)
				{
					return result;
				}
			}

		}
		else
		{
			which = 1;
		}
		index = input;
		while (which > 0)
		{
			index = my_strstr(index , find);
			if (index)
			{
				if (which-- == 1)
				{
					*index = 0;
				}
				index += strlen(find);
			}
			else
			{
				return result;
			}
		}
		malloc_strcpy(&result, input);
	}
	return result;
}

char	*
function_after(input)
	char	*input;
{
	char	*result = NULL;
	char	*find;
	int	which,
		count;

	malloc_strcpy(&result, empty_string);

	find = next_arg(input, &input);
	if (find && *find)
	{
		which = atoi(find);
		if (which)
		{
			find = next_arg(input, &input);
			if (!find || !*find)
			{
				return result;
			}
			if (which < 0)
			{
				count = countstr(input, find);
				which = count + which + 1;
				if (which < 1)
				{
					return result;
				}
			}

		}
		else
		{
			which = 1;
		}
		while (which > 0)
		{
			input = my_strstr(input, find);
			if (input)
			{
				input += strlen(find);
				which--;
			}
			else
			{
				return result;
			}
		}
		malloc_strcpy(&result, input);
	}
	return result;
}

/* patch from Sarayan to make $rand() better */

#define RAND_A 16807L
#define RAND_M 2147483647L
#define RAND_Q 127773L
#define RAND_R 2836L

static	long	
randm(l)
	long	l;
{
	static	u_long	z = 0;
	long	t;

	if (!z)
		z = (u_long) getuid();
	if (!l)
	{
		t = RAND_A * (z % RAND_Q) - RAND_R * (z / RAND_Q);
		if (t > 0)
			z = t;
		else
			z = t + RAND_M;
		return (z >> 8) | ((z & 255) << 23);
	}
	else
	{
		if (l < 0)
			z = (u_long) getuid();
		else
			z = l;
		return 0;
	}
}

char	*
function_rand(input)
	char	*input;
{
	char	*result = (char *) 0;
	char	tmp[40];
	long	tempin;

	sprintf(tmp, "%ld", (tempin = atol(input)) ? randm(0L) % tempin : 0);
	malloc_strcpy(&result, tmp);
	return result;
}

char	*
function_srand(input)
	char	*input;
{
	char	*result = (char *) 0;

	if (input && *input)
		(void) randm(atol(input));
	else
		(void) randm((time_t) time(NULL));
	malloc_strcpy(&result, empty_string);
	return result;
}

/*ARGSUSED*/
char	*
function_time(input)
	char	*input;
{
	char	*result = (char *) 0;
	time_t	ltime;

	(void) time(&ltime);
	sprintf(buffer, "%ld", ltime);
	malloc_strcpy(&result, buffer);
	return result;
}

char	*
function_stime(input)
	char	*input;
{
	char	*result = (char *) 0;
	time_t	ltime;

	if (input && *input)
	{
		ltime = atol(input);
		malloc_strcpy(&result, ctime(&ltime));
		result[strlen(result) - 1] = (char) 0;
	}
	else
	{
		malloc_strcpy(&result, empty_string);
	}
	return result;
}

char	*
function_tdiff(input)
	char	*input;
{
	char	*result = (char *) 0;

	if (input && *input)
	{
		malloc_strcpy(&result, tdiff(atoi(input)));
	}
	else
	{
		malloc_strcpy(&result, empty_string);
	}
	return result;
}

char  *
function_tdiff2(input)
	char    *input;
{
	char    *result = (char *) 0;

	if (input && *input)
	{
		malloc_strcpy(&result, tdiff2(atoi(input)));
	}
	else
	{
		malloc_strcpy(&result, empty_string);
	}
	return result;
}

char	*
function_index(input)
	char	*input;
{
	char	*result = (char *) 0;
	char	*schars;
	char	*iloc;
	int	ival;

	if (input && *input)
	{
		schars = next_arg(input, &input);
		iloc = (schars) ? sindex(input, schars) : NULL;
		ival = (iloc) ? iloc - input : -1;
		sprintf(buffer, "%d", ival);
		malloc_strcpy(&result, buffer);
	}
	else
	{
		malloc_strcpy(&result, empty_string);
	}
	return result;
}

char	*
function_rindex(input)
	char	*input;
{
	char	*result = (char *) 0;
	char	*schars;
	char	*iloc, *liloc;
	int	ival;

	if (input && *input)
	{
		schars = next_arg(input, &input);
		iloc = NULL;
		if (schars)
		{
			liloc = sindex(input, schars);
			while (liloc)
				liloc = sindex((iloc = liloc) + 1, schars);
		}
		ival = (iloc) ? iloc-input : -1;
		sprintf(buffer, "%d", ival);
		malloc_strcpy(&result, buffer);
	}
	else
	{
		malloc_strcpy(&result, empty_string);
	}
	return result;
}

char	*
function_match(input)
	char	*input;
{
	char	*result = (char *) 0;
	char	*pattern;
	char	*word;
	int	current_match;
	int	best_match = 0;
	int	match = 0;
	int	match_index = 0;

	if (input && *input)
	{
		if ((pattern = next_arg(input, &input)) != NULL)
		{
			while ((word = next_arg(input, &input)) != NULL)
			{
				match_index++;
				if ((current_match = wild_match(pattern, word))
					> best_match)
				{
					match = match_index;
					best_match = current_match;
				}
			}
		}
		sprintf(buffer, "%d", match);
		malloc_strcpy(&result, buffer);
	}
	else
	{
		malloc_strcpy(&result, empty_string);
	}
	return result;
}

char	*
function_rmatch(input)
	char	*input;
{
	char	*result = (char *) 0;
	char	*pattern;
	char	*word;
	int	current_match;
	int	best_match = 0;
	int	match = 0;
	int	rmatch_index = 0;

	if (input && *input)
	{
		if ((pattern = next_arg(input, &input)) != NULL)
		{
			while ((word = next_arg(input, &input)) != NULL)
			{
				rmatch_index++;
				if ((current_match =
					wild_match(word, pattern)) > best_match)
				{
					match = rmatch_index;
					best_match = current_match;
				}
			}
		}
		sprintf(buffer, "%d", match);
		malloc_strcpy(&result, buffer);
	}
	else
	{
		malloc_strcpy(&result, empty_string);
	}
	return result;
}

/*ARGSUSED*/
char	*
function_userhost(input)
	char	*input;
{
	char	*result = (char *) 0;

	malloc_strcpy(&result, FromUserHost ? FromUserHost : empty_string);
	return result;
}

char	*
function_strip(input)
	char	*input;
{
	static	char	result[BIG_BUFFER_SIZE+1];
	char	*retval = (char *) 0;
	char	*chars;
	char	*cp, *dp;

	if (input && *input)
	{
		if ((chars = next_arg(input, &input)) && input)
		{
			for (cp = input, dp = result; *cp; cp++)
			{
				if (!index(chars, *cp))
					*dp++ = *cp;
			}
			*dp = '\0';
		}
		malloc_strcpy(&retval, result);
	}
	else
	{
		malloc_strcpy(&retval, empty_string);
	}
	return retval;
}

char	*
function_encode(input)
	unsigned char	*input;
{
	static unsigned char	result[BIG_BUFFER_SIZE+1];
	char	*retval = (char *) 0;
	unsigned char	*c;
	int	i = 0;

	for (c = input; *c; c++)
	{
		result[i++] = (*c >> 4) + 0x41;
		result[i++] = (*c & 0x0f) + 0x41;
	}
	result[i] = '\0';
	malloc_strcpy(&retval, result);
	return retval;
}


char	*
function_decode(input)
	unsigned char	*input;
{
	static unsigned	char	result[BIG_BUFFER_SIZE+1];
	char	*retval = (char *) 0;
	unsigned char	*c;
	unsigned char	d,e;
	int	i = 0;

	c = input;
	while((d = *c) && (e = *(c+1)))
	{
		result[i] = ((d - 0x41) << 4) | (e - 0x41);
		c += 2;
		i++;
	}
	result[i] = '\0';
	malloc_strcpy(&retval, result);
	return retval;
}

char	*
function_ischannel(input)
	char	*input;
{
	char	*result = (char *) 0;

	malloc_strcpy(&result, is_channel(input) ? "1" : "0");
	return result;
}

char	*
function_ischanop(input)
	char	*input;
{
	char	*result = (char *) 0;
	char	*nick;
	char	*channel = NULL;

	if (!(nick = next_arg(input, &channel)))
		malloc_strcpy(&result, "0");
	else
		malloc_strcpy(&result, is_chanop(channel, nick) ? "1" : "0");
	return result;
}


char	*
function_word(input)
	char	*input;
{
	char	*result = (char *) 0;
	char	*count;
	int	cvalue;
	char	*word;

	count = next_arg(input, &input);
	if (count)
		cvalue = atoi(count);
	else
		cvalue = 0;
	if (cvalue < 0)
		malloc_strcpy(&result, empty_string);
	else
	{
		for (word = next_arg(input, &input); word && cvalue--;
				word = next_arg(input, &input))
			;
		malloc_strcpy(&result, (word) ? word : empty_string);
	}
	return result;
}


char	*
function_winnum(input)
	char	*input;
{
	char	*result = (char *) 0;

	if (curr_scr_win)
		sprintf(buffer, "%d", curr_scr_win->refnum);
	else
		strcpy(buffer, "-1");
	malloc_strcpy(&result, buffer);
	return result;
}

char	*
function_winnam(input)
	char	*input;
{
	char	*result = (char *) 0;

	malloc_strcpy(&result, (curr_scr_win && curr_scr_win->name) ?
		curr_scr_win->name : empty_string);
	return result;
}

char	*
function_connect(input)
	char	*input;
{
	char	*result = (char *) 0;
	char	*host;

#ifdef DAEMON_UID
	if (getuid() == DAEMON_UID)
		put_it("You are not permitted to use CONNECT()");
	else
#endif
		if ((host = next_arg(input, &input)) != NULL)
			result = dcc_raw_connect(host, atoi(input));
	return result;
}


char	*
function_listen(input)
	char	*input;
{
	char	*result = (char *) 0;

#ifdef DAEMON_UID
	if (getuid() == DAEMON_UID)
		malloc_strcpy(&result, "0");
	else
#endif
		result = dcc_raw_listen(atoi(input));
	return result;
}

static	char	*
alias_version(input)
	char	*input;
{
	char *result = (char *) 0;
	malloc_strcpy(&result, internal_version);
	return result;
}

static	char	*
alias_currdir(input)
	char	*input;
{
	char	*result = (char *) 0;

        getcwd(buffer, BIG_BUFFER_SIZE+1);
	/* is this more portable? *shrug*. i chose BIG_BUFFER_SIZE+1
	   because it's more readable -cgw- */
	/* getcwd((char *)buffer, sizeof((char *)buffer)); */
	malloc_strcpy(&result, buffer);
	return result;
}

static	char	*
alias_current_numeric(input)
	char	*input;
{
	char	*result = (char *) 0,
		number[4];
	
	sprintf(number, "%03d", -current_numeric);
	malloc_strcpy(&result, number);
	return result;
}

static	char	*
alias_server_version(input)
	char	*input;
{
	char	*result = (char *) 0,
		*s;

	malloc_strcpy(&result,
		(s = server_list[curr_scr_win->server].version_string) ?
				s : empty_string);
	return result;
}

char	*
function_toupper(input)
	char	*input;
{
	char	*new = (char *) 0,
		*ptr;

	if (!input)
		return empty_string;
	malloc_strcpy(&new, input);
	for (ptr = new; *ptr; ptr++)
		*ptr = islower(*ptr) ? toupper(*ptr) : *ptr;
	return new;
}

char	*
function_tolower(input)
	char	*input;
{
	char	*new = (char *) 0,
		*ptr;

	if (!input)
		return empty_string;
	malloc_strcpy(&new, input);
	for (ptr = new; *ptr; ptr++)
		*ptr = (isupper(*ptr)) ? tolower(*ptr) : *ptr;
	return new;
}

char	*
function_curpos(input)
	char	*input;
{
	char	*new = (char *) 0,
		pos[4];

	sprintf(pos, "%d", current_screen->buffer_pos);
	malloc_strcpy(&new, pos);
	return new;
}

char	*
function_channels(input)
	char	*input;
{
	Window	*window;

	if (input)
		window = isdigit(*input) ? get_window_by_refnum(atoi(input))
					 : curr_scr_win;
	else
		window = curr_scr_win;

	return create_channel_list(window);
}

char	*
function_servers(input)
	char	*input;
{
	return create_server_list();
}

char	*
function_onchannel(input)
	char	*input;
{
	char	*result = (char *) 0;
	char	*nick;
	char	*channel = NULL;

	if (!(nick = next_arg(input, &channel)))
		malloc_strcpy(&result, "0");
	else
		malloc_strcpy(&result, is_on_channel(channel, nick) ? "1"
								    : "0");
	return result;
}

char	*
function_pid(input)
	char	*input;
{
	char	*result = (char *) 0;

	sprintf(buffer, "%d", (int) getpid());
	malloc_strcpy(&result, buffer);
	return result;
}

char	*
function_ppid(input)
	char	*input;
{
	char	*result = (char *) 0;

	sprintf(buffer, "%d", (int) getppid());
	malloc_strcpy(&result, buffer);
	return result;
}

char	*
function_chanusers(input)
	char	*input;
{
	ChannelList	*chan;
	NickList	*nicks;
	char	*result = (char *) 0;
	int	len = 0;
	int	notfirst = 0;

	chan = lookup_channel(input, from_server, 0);
	if ((ChannelList *) 0 == chan)
		return (char *) 0;

	*buffer = '\0';

	for (nicks = chan->nicks; nicks; nicks = nicks->next)
	{
		len += strlen(nicks->nick);
		len += 2;
		len += nicks->user ? strlen(nicks->user) : 9;
		len += nicks->host ? strlen(nicks->host) : 9;
		if (len > BIG_BUFFER_SIZE)
		{
			malloc_strcat(&result, buffer);
			*buffer = '\0';
			len = 0;
		}
		if (notfirst)
			strcat(buffer, " ");
		else
			notfirst = 1;
		strcat(buffer, nicks->nick);
		strcat(buffer, "!");
		strcat(buffer, nicks->user ? nicks->user : "<UNKNOWN>");
		strcat(buffer, "@");
		strcat(buffer, nicks->host ? nicks->host : "<UNKNOWN>");
	}
	malloc_strcat(&result, buffer);
	return result;
}

/*
 * strftime() patch from hari (markc@arbld.unimelb.edu.au)
 */
char    *
function_strftime(input)
        char    *input;
{
        char    result[128];
        time_t  ltime;
        char    *fmt = (char *) 0;

        ltime = atol(input);
        fmt = input;
        /* skip the time field */
        while (isdigit(*fmt))
                ++fmt;
        if (*fmt && *++fmt)
        {
                struct tm       *tm;

                tm = localtime(&ltime);
                if (strftime(result, 128, fmt, tm))
                {
                        char *  s = (char *) 0;

                        malloc_strcpy(&s, result);
                        return s;
                }
                else
                        return (char *) 0;
        }
        else
        {
                return (char *) 0;
        }
}

char *function_utime(input)
char *input;
{
	char    *result = (char *) 0;
	struct  timeval         tp;
	struct  timezone        tzp;
	if (gettimeofday(&tp,&tzp) == 0)
	{
		sprintf(buffer, "%ld %ld", tp.tv_sec, tp.tv_usec);
		malloc_strcpy(&result, buffer);
		return result;
	}
	else
		return (char *) 0;
}

char *function_cluster(input)
char    *input;
{
	char *temp;
	char *result = (char *) 0;
	temp = cluster(input);
	if (!temp)
		return (char *) 0;
	malloc_strcpy(&result, temp);
	return result;
}

char    *function_getkey(input)
char    *input;
{
	char *temp;
	char *result = (char *) 0;

	temp = channel_key(input);
	if (!temp)
		return (char *) 0;
	malloc_strcpy(&result, temp);
	return result;
}

char	*function_reason(input)
char	*input;
{
	char *temp;
	char *result = (char *) 0;

	temp = (char *)get_reason();
	if (!temp)
		return (char *) 0;
	malloc_strcpy(&result, temp);
	return result;
}

char	*
function_clusterhost(input)
	char	*input;
{
	char	*result = (char *) 0;

	malloc_strcpy(&result, FromUserHost ? cluster(FromUserHost) : empty_string);
	return result;
}

char	*
function_timestamp(input)
	char	*input;
{
	char	*result = (char *) 0;

	malloc_strcpy(&result, TimeStamp());
	return result;
}

char	*
function_length(input)
	char	*input;
{
	char	*result = (char *) 0;
	int	lval;

	lval = strlen(input);
	sprintf(buffer, "%d", lval);
	malloc_strcpy(&result, buffer);
	return result;
}

char	*
function_reverse(input)
	char	*input;
{
	char	*result = (char *) 0;
	int	i = 0, j, l;

	if (input) {
		j = l = strlen(input);
		result = new_malloc(l+1);
		result[j--] = '\000';
		while (i < l)
			result[j--] = input[i++];
	} else
		malloc_strcpy(&result, empty_string);

	return result;
}

char	*
function_format(input)
	char	*input;
{
	char	*result = (char *) 0;
	char	*width;
	int	w, l;

	if (input) {
		width = next_arg(input, &input);
		if (width)
			w = atoi(width);
		else
			w = 0;

		if (!input)
			input = empty_string;

		l = strlen(input);

		w = (w > l) ? w : l;

		result = new_malloc(w+1);
		memset(result, 32, w);

		strcpy(&result[w-l], input);
	} else
		malloc_strcpy(&result, empty_string);

	return result;
}

char	*
function_lformat(input)
	char	*input;
{
	char	*result = (char *) 0;
	char	*width;
	int	w, l;

	if (input) {
		width = next_arg(input, &input);
		if (width)
			w = atoi(width);
		else
			w = 0;

		if (!input)
			input = empty_string;

		l = strlen(input);

		w = (w > l) ? w : l;

		result = new_malloc(w+1);
		memset(result, 32, w);

		strcpy(result, input);
		result[l] = ' ';
		result[w] = '\000';
	} else
		malloc_strcpy(&result, empty_string);

	return result;
}

char	*
function_center(input)
	char	*input;
{
	char	*result = (char *) 0;
	char	*width;
	int	w, l;

	if (input) {
		width = next_arg(input, &input);
		if (width)
			w = atoi(width);
		else
			w = 0;

		if (!input)
			input = empty_string;

		l = strlen(input);

		w = (w > l) ? ((w-l)/2)+l : l;

		result = new_malloc(w+1);
		memset(result, 32, w);

		strcpy(&result[w-l], input);
	} else
		malloc_strcpy(&result, empty_string);

	return result;
}

/* Probably not the most efficient implementation, but it works. :) */
char	*
function_sandr(input)
	char	*input;
{
	char	*result = (char *) 0;
	Alias	*alias = NULL;
	char	*flags, *src, *rep, *words, *varname;
	int	srclen;
	int	wlen, wpos;
	int	gflag = 0,
		rflag = 0;
	char	tmp[2];

	tmp[1] = '\000';

	if (input)
	{
		flags = input;
		src = index(flags, '/');
		if (src)
		{
			*src++ = '\000';
			rep = index(src, '/');
			if (rep)
			{
				*rep++ = '\000';
				words = index(rep, '/');
				if (words)
				{
					*words++ = '\000';
					gflag = index(flags, 'g') ||
						index(flags, 'G');
					rflag = index(flags, 'r') ||
						index(flags, 'R');
					if (rflag)
					{
						varname = next_arg(words, NULL);
						alias = find_alias(&(alias_list[VAR_ALIAS]), varname, 0, (int *) NULL);
						if (!alias)
						{
							alias = (Alias *)new_malloc(sizeof(Alias));
							if (!alias)
							{
								malloc_strcpy(&result, empty_string);
								return result;
							}
							alias->name = (char *) 0;
							alias->stuff = (char *) 0;
							malloc_strcpy(&alias->name, upper(varname));
							malloc_strcpy(&alias->stuff, empty_string);
							insert_alias(&(alias_list[VAR_ALIAS]), alias);
						}
						words = alias->stuff;
					}
					if (!*src)
					{
						malloc_strcpy(&result, words);
						return result;
					}
					wlen = strlen(words);
					srclen = strlen(src);
					for (wpos = 0; wpos < wlen; wpos++)
					{
						if (!my_strnicmp(src, &words[wpos], srclen))
						{
							malloc_strcat(&result, rep);
							if (srclen)
							{
								wpos += srclen-1;
							}
							if (!gflag)
							{
								malloc_strcat(&result, &words[wpos + 1]);
								break;
							}
						}
						else
						{
							tmp[0] = words[wpos];
							malloc_strcat(&result, tmp); 
						}
					}
					if (rflag)
					{
						malloc_strcpy(&(alias->stuff),
							result);
					}
				}
				else
				{
					malloc_strcpy(&result, empty_string);
				}
			}
			else
			{
				malloc_strcpy(&result, empty_string);
			}
		}
		else
		{
			malloc_strcpy(&result, empty_string);
		}
	}
	else
	{
		malloc_strcpy(&result, empty_string);
	}
	return result;
}

char	*
function_tr(input)
	char	*input;
{
	char	*result = (char *) 0;
	char	*flags,
		*src,
		*dst,
		*target;
	int	srcl, dstl;
	int	i, j, k, l;
	int	m;

	if (input && *input)
	{
		flags = input;
		src = index(input, '/');
		if (src)
		{
			*(src++) = 0;
			dst = index(src, '/');
			if (dst)
			{
				*(dst++) = 0;
				target = index(dst, '/');
				if (target)
				{
					*(target++) = 0;
					srcl = strlen(src);
					dstl = strlen(dst);
					malloc_strcpy(&result, target);
					l = 0;
					for (i = 0; target[i]; i++)
					{
						m = 0;
						for (j = 0, k = 0; j < srcl; j++)
						{
							if (target[i] == src[j])
							{
								if (dst[k])
								{
									result[l++] = dst[k];
								}
								m = 1;
								break;
							}
							if (k < (dstl - 1))
							{
								k++;
							}
						}
						if (!m)
						{
							result[l++] = target[i];
						}
					}
					result[l] = 0;
				}
				else
				{
					malloc_strcpy(&result, empty_string);
				}
			}
			else
			{
				malloc_strcpy(&result, empty_string);
			}
		}
		else
		{
			malloc_strcpy(&result, empty_string);
		}
	}
	else
	{
		malloc_strcpy(&result, empty_string);
	}
	return result;
}

char	*
function_notword(input)
	char	*input;
{
	char	*result = (char *) 0;
	char	*ix, *aword;
	int	i, j;

	malloc_strcpy(&result, empty_string);
	if (input) {
		ix = next_arg(input, &input);
		if (ix) {
			i = atoi(ix);
			j = 1;
			while (input && *input) {
				aword = next_arg(input, &input);
				if (j++ != i) {
					if (result && *result)
						malloc_strcat(&result, " ");
					malloc_strcat(&result, aword);
				}
			}
		}
	}

	return result;
}

char	*
function_strcnt(input)
	char	*input;
{
	char	*result = (char *) 0;
	int	count = 0;

	if (input)
	{
		while (next_arg(input, &input))
		{
			count++;
		}
	}
	sprintf(buffer, "%d", count);
	malloc_strcpy(&result, buffer);
	return result;
}

char	*
function_shift(input)
	char	*input;
{
	char	*result = (char *) 0;
	Alias	*alias;
	char	*list, *space;

	if (input) {
		list = next_arg(input, &input);
		if (list) {
			if (*list == '$')
				list++;
			if ((alias = find_alias(&(alias_list[VAR_ALIAS]), list, 0, (int *) NULL))
					!= NULL)
			{
				space = index(alias->stuff, ' ');
				if (space)
				{
					*(space++) = 0;
					malloc_strcpy(&result, alias->stuff);
					malloc_strcpy(&alias->stuff, space);
				}
				else
				{
					malloc_strcpy(&result, alias->stuff);
					malloc_strcpy(&alias->stuff,
						empty_string);
				}
			} else
				malloc_strcpy(&result, empty_string);
		} else
			malloc_strcpy(&result, empty_string);
	} else
		malloc_strcpy(&result, empty_string);

	return result;
}

char	*
function_unshift(input)
	char	*input;
{
	char	*result = (char *) 0;
	char	*tmp = (char *) 0;
	Alias	*alias;
	char	*list;

	if (input) {
		list = next_arg(input, &input);
		if (list) {
			if (*list == '$')
				list++;
			if (input && *input)
				if ((alias = find_alias(&(alias_list[VAR_ALIAS]), list, 0, (int *) NULL))
						!= NULL)
				{
					malloc_strcpy(&tmp, input);
					if (*(alias->stuff))
					{
						malloc_strcat(&tmp, " ");
						malloc_strcat(&tmp,
							alias->stuff);
					}
					malloc_strcpy(&alias->stuff, tmp);
					new_free(&tmp);
					malloc_strcpy(&result, alias->stuff);
				} else {
					alias = (Alias *)new_malloc(sizeof(Alias));
					alias->name = (char *) 0;
					alias->stuff = (char *) 0;
					malloc_strcpy(&alias->name, upper(list));
					malloc_strcpy(&alias->stuff, input);
					insert_alias(&(alias_list[VAR_ALIAS]), alias);
					malloc_strcpy(&result, alias->stuff);
				}
			else
				malloc_strcpy(&result, empty_string);
		} else
			malloc_strcpy(&result, empty_string);
	} else
		malloc_strcpy(&result, empty_string);

	return result;
}

char	*
function_pop(input)
	char	*input;
{
	char	*result = (char *) 0;
	Alias	*alias;
	char	*list, *space;

	if (input) {
		list = next_arg(input, &input);
		if (list) {
			if (*list == '$')
				list++;
			if ((alias = find_alias(&(alias_list[VAR_ALIAS]), list, 0, (int *) NULL))
					!= NULL)
			{
				space = rindex(alias->stuff, ' ');
				if (space)
				{
					*(space++) = 0;
					malloc_strcpy(&result, space);
				}
				else
				{
					malloc_strcpy(&result, alias->stuff);
					malloc_strcpy(&alias->stuff,
						empty_string);
				}
			} else
				malloc_strcpy(&result, empty_string);
		} else
			malloc_strcpy(&result, empty_string);
	} else
		malloc_strcpy(&result, empty_string);

	return result;
}

char	*
function_push(input)
	char	*input;
{
	char	*result = (char *) 0;
	Alias	*alias;
	char	*list;

	if (input) {
		list = next_arg(input, &input);
		if (list) {
			if (*list == '$')
				list++;
			if (input && *input)
				if ((alias = find_alias(&(alias_list[VAR_ALIAS]), list, 0, (int *) NULL))
						!= NULL)
				{
					if (*(alias->stuff))
					{
						malloc_strcat(&alias->stuff,
							" ");
					}
					malloc_strcat(&alias->stuff, input);
					malloc_strcpy(&result, alias->stuff);
				} else {
					alias = (Alias *)new_malloc(sizeof(Alias));
					alias->name = (char *) 0;
					alias->stuff = (char *) 0;
					malloc_strcpy(&alias->name, upper(list));
					malloc_strcpy(&alias->stuff, input);
					insert_alias(&(alias_list[VAR_ALIAS]), alias);
					malloc_strcpy(&result, alias->stuff);
				}
			else
				malloc_strcpy(&result, empty_string);
		} else
			malloc_strcpy(&result, empty_string);
	} else
		malloc_strcpy(&result, empty_string);

	return result;
}

char	*
function_pluck(input)
	char	*input;
{
	char	*result = (char *) 0;
	Alias	*alias;
	char	*list, *aword, *tmp;
	char	*words = (char *) 0;

	if (input) {
		list = next_arg(input, &input);
		if (list) {
			if (*list == '$')
				list++;
			aword = next_arg(input, &input);
			if (aword && *aword) {
				if ((alias = find_alias(&(alias_list[VAR_ALIAS]), list, 0, (int *) NULL))
						!= NULL)
				{
					malloc_strcpy(&words, alias->stuff);
					list = words;
					while (words && *words) {
						tmp = next_arg(words, &words);
						if (tmp && !wild_match(aword, tmp)) {
							if (result && *result)
								malloc_strcat(&result, " ");
							malloc_strcat(&result, tmp);
						} else
							aword = empty_string;
					}
					new_free(&list);
				}
			}
		}
	}

	if (!result)
		malloc_strcpy(&result, empty_string);

	return result;
}

char	*
function_remove(input)
	char	*input;
{
	char	*result = (char *) 0;
	Alias	*alias;
	char	*list, *aword, *tmp;
	char	*words = (char *) 0;

	if (input) {
		list = next_arg(input, &input);
		if (list) {
			if (*list == '$')
				list++;
			aword = next_arg(input, &input);
			if (aword && *aword) {
				if ((alias = find_alias(&(alias_list[VAR_ALIAS]), list, 0, (int *) NULL))
						!= NULL)
				{
					malloc_strcpy(&words, alias->stuff);
					list = words;
					while (words && *words) {
						tmp = next_arg(words, &words);
						if (tmp && !wild_match(aword, tmp)) {
							if (result && *result)
								malloc_strcat(&result, " ");
							malloc_strcat(&result, tmp);
						}
					}
					new_free(&list);
				}
			}
		}
	}

	if (!result)
		malloc_strcpy(&result, empty_string);

	return result;
}

typedef struct sortword {
	struct sortword	*next;
	char		*word;
} SortWord;

char	*
function_sort(input)
	char	*input;
{
	char		*result = (char *) 0;
	char		*word;
	SortWord	*words = (SortWord *) 0, *tmp, *curr;

	if (input) {
		while (input && *input && (word = next_arg(input, &input))) {
			tmp = (SortWord *)new_malloc(sizeof(SortWord));
			tmp->word = (char *) 0;
			malloc_strcpy(&tmp->word, word);

			if (!words || (strcmp(words->word, tmp->word) > 0)) {
				tmp->next = words;
				words = tmp;
			} else {
				curr = words;
				while (curr->next && (strcmp(curr->next->word, tmp->word) < 0))
					curr = curr->next;
				tmp->next = curr->next;
				curr->next = tmp;
			}
		}
	}

	while (words) {
		if (result && *result)
			malloc_strcat(&result, " ");
		malloc_strcat(&result, words->word);
		tmp = words;
		words = words->next;
		new_free(&tmp);
	}

	if (!result)
		malloc_strcpy(&result, empty_string);

	return result;
}

char	*
function_rsort(input)
	char	*input;
{
	char		*result = (char *) 0;
	char		*word;
	SortWord	*words = (SortWord *) 0, *tmp, *curr;

	if (input) {
		while (input && *input && (word = next_arg(input, &input))) {
			tmp = (SortWord *)new_malloc(sizeof(SortWord));
			tmp->word = (char *) 0;
			malloc_strcpy(&tmp->word, word);

			if (!words || (strcmp(words->word, tmp->word) < 0)) {
				tmp->next = words;
				words = tmp;
			} else {
				curr = words;
				while (curr->next && (strcmp(curr->next->word, tmp->word) > 0))
					curr = curr->next;
				tmp->next = curr->next;
				curr->next = tmp;
			}
		}
	}

	while (words) {
		if (result && *result)
			malloc_strcat(&result, " ");
		malloc_strcat(&result, words->word);
		tmp = words;
		words = words->next;
		new_free(&tmp);
	}

	if (!result)
		malloc_strcpy(&result, empty_string);

	return result;
}
char	*
function_isort(input)
	char	*input;
{
	char		*result = (char *) 0;
	char		*word;
	SortWord	*words = (SortWord *) 0, *tmp, *curr;

	if (input) {
		while (input && *input && (word = next_arg(input, &input))) {
			tmp = (SortWord *)new_malloc(sizeof(SortWord));
			tmp->word = (char *) 0;
			malloc_strcpy(&tmp->word, word);

			if (!words || (my_stricmp(words->word, tmp->word) > 0)) {
				tmp->next = words;
				words = tmp;
			} else {
				curr = words;
				while (curr->next && (my_stricmp(curr->next->word, tmp->word) < 0))
					curr = curr->next;
				tmp->next = curr->next;
				curr->next = tmp;
			}
		}
	}

	while (words) {
		if (result && *result)
			malloc_strcat(&result, " ");
		malloc_strcat(&result, words->word);
		tmp = words;
		words = words->next;
		new_free(&tmp);
	}

	if (!result)
		malloc_strcpy(&result, empty_string);

	return result;
}

char	*
function_irsort(input)
	char	*input;
{
	char		*result = (char *) 0;
	char		*word;
	SortWord	*words = (SortWord *) 0, *tmp, *curr;

	if (input) {
		while (input && *input && (word = next_arg(input, &input))) {
			tmp = (SortWord *)new_malloc(sizeof(SortWord));
			tmp->word = (char *) 0;
			malloc_strcpy(&tmp->word, word);

			if (!words || (my_stricmp(words->word, tmp->word) < 0)) {
				tmp->next = words;
				words = tmp;
			} else {
				curr = words;
				while (curr->next && (my_stricmp(curr->next->word, tmp->word) > 0))
					curr = curr->next;
				tmp->next = curr->next;
				curr->next = tmp;
			}
		}
	}

	while (words) {
		if (result && *result)
			malloc_strcat(&result, " ");
		malloc_strcat(&result, words->word);
		tmp = words;
		words = words->next;
		new_free(&tmp);
	}

	if (!result)
		malloc_strcpy(&result, empty_string);

	return result;
}

char	*
function_jot(input)
	char	*input;
{
	char	*result = (char *) 0;
	char	*fromstr, *tostr, *intstr;
	int	from, to, interval;
	int	i;

	malloc_strcpy(&result, empty_string);

	fromstr = next_arg(input, &input);
	tostr = next_arg(input, &input);
	intstr = next_arg(input, &input);
	if (fromstr && *fromstr && tostr && *tostr && is_number(fromstr) &&
			is_number(tostr)) {
		from = atoi(fromstr);
		to = atoi(tostr);
		if (intstr && is_number(intstr)) {
			if (!(interval = atoi(intstr)))
				interval = 1;
		} else
			interval = 1;

		if (((from > to) && (interval > 0)) ||
				((to > from) && (interval < 0)))
			interval = -interval;
		if (from < to)
			for (i = from; i <= to; i += interval) {
				if (*result)
					malloc_strcat(&result, " ");
				sprintf(buffer, "%d", i);
				malloc_strcat(&result, buffer);
			}
		else
			for (i = from; i >= to; i += interval) {
				if (*result)
					malloc_strcat(&result, " ");
				sprintf(buffer, "%d", i);
				malloc_strcat(&result, buffer);
			}
	}
	return result;
}

char	*
function_isdigit(input)
	char	*input;
{
	char	*result = (char *) 0;

	if (input)
	{
		if (isdigit(*input))
		{
			malloc_strcpy(&result, "1");
		}
		else
		{
			malloc_strcpy(&result, "0");
		}
	}
	else
	{
		malloc_strcpy(&result, empty_string);
	}
	return result;
}

char	*
function_isalpha(input)
	char	*input;
{
	char	*result = (char *) 0;

	if (input)
	{
		if (isalpha(*input))
		{
			malloc_strcpy(&result, "1");
		}
		else
		{
			malloc_strcpy(&result, "0");
		}
	}
	else
	{
		malloc_strcpy(&result, empty_string);
	}
	return result;
}

char	*
function_chr(input)
	char	*input;
{
	char	*result = (char *) 0;
	char	tmp[2] = { 0, 0 };
	char	*item;

	malloc_strcpy(&result, empty_string);
	if (input && *input)
	{
		while (NULL != (item = next_arg(input, &input)))
		{
			tmp[0] = atoi(item);
			malloc_strcat(&result, (char *)tmp);
		}
	}
	return result;
}

char	*
function_ascii(input)
	char	*input;
{
	char	*result = (char *) 0;

	malloc_strcpy(&result, empty_string);
	if (input)
	{
		for (; *input; input++)
		{
			sprintf(buffer, "%d", *input);
			if (*result)
			{
				malloc_strcat(&result, " ");
			}
			malloc_strcat(&result, buffer);
		}
	}
	return result;
}

char	*
function_idle(input)
	char	*input;
{
	char	*result = (char *) 0;

	sprintf(buffer, "%ld", time(0) - idle_time);
	malloc_strcpy(&result, buffer);
	return result;
}

char	*
function_uptime(input)
	char	*input;
{
	char	*result = (char *) 0;

	sprintf(buffer, "%ld", time(0) - start_time);
	malloc_strcpy(&result, buffer);
	return result;
}

