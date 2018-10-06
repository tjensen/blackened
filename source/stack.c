/*
 * stack.c - does the handling of stack functions
 *
 * written by matthew green
 *
 * copyright (C) 1993.
 */

#ifndef lint
static	char	rcsid[] = "@(#)$Id: stack.c,v 1.1.1.1.2.1 2002/03/07 22:29:52 toast Exp $";
#endif

#include "irc.h"

#include "stack.h"
#include "window.h"
#include "hook.h"
#include "ircaux.h"
#include "output.h"
#include "list.h"
#include "vars.h"
#include "format.h"

static	Stack	*on_stack = NULL;
static  Stack	*alias_stack = NULL;
static	Stack	*assign_stack = NULL;
static	Stack	*set_stack = NULL;
static	Stack	*format_stack = NULL;
static	Stack	*nformat_stack = NULL;

static	Stack	*alias_get _((char *, int));
static	Stack	*alias_stack_find _((char *, int));
static	void	alias_stack_add _((Stack *, int));

extern	void	insert_alias _((Alias **, Alias *));

static	void
do_stack_on(type, args)
	int	type;
	char	*args;
{
	char	foo[4];
	int	len, cnt, i, which = 0;
	Hook	*list;
	NumericList *nhook, *nptr, *ntmp;

	if (!on_stack && (type == STACK_POP || type == STACK_LIST))
	{
		say("ON stack is empty!");
		return;
	}
	if (!args || !*args)
	{
		say("Missing event type for STACK ON");
		return;
	}
	len = strlen(args);
	for (cnt = 0, i = 0; i < NUMBER_OF_LISTS; i++)
	{
		if (!my_strnicmp(args, hook_functions[i].name, len))
		{
			if (strlen(hook_functions[i].name) == len)
			{
				cnt = 1;
				which = i;
				break;
			}
			else
			{
				cnt++;
				which = i;
			}
		}
		else if (cnt)
			break;
	}
	if (!cnt)
	{
		if (is_number(args))
		{
			which = atoi(args);
			if (which < 1 || which > 999)
			{
				say("Numerics must be between 001 and 999");
				return;
			}
			which = -which;
		}
		else
		{
			say("No such ON function: %s", args);
			return;
		}
	}
	if (which < 0)
	{
		sprintf(foo, "%3.3u", -which);
		if ((nhook = (NumericList *) find_in_list(&numeric_list, foo, 0))
				!= NULL)
			list = nhook->list;
		else
			list = NULL;
	}
	else
		list = hook_functions[which].list;
	if (type == STACK_PUSH)
	{
		Stack	*new;

		if (list == NULL)
		{
			say("The ON %s list is empty", args);
			return;
		}
		new = (Stack *) new_malloc(sizeof(Stack));
		new->next = on_stack;
		on_stack = new;
		new->which = which;
		new->list = list;
		if (which < 0)
		{
			if (nhook == numeric_list)
			{
				numeric_list = nhook->next;
				new_free(&nhook->name);
				new_free(&nhook);
				return;
			}
			for (nptr = numeric_list; nptr;
					ntmp = nptr, nptr = nptr->next)
			{
				if (nptr == nhook)
				{
					ntmp->next = nptr->next;
					new_free(&nptr->name);
					new_free(&nptr);
					return;
				}
			}
		}
		else
			hook_functions[which].list = NULL;
		return;
	}
	else if (type == STACK_POP)
	{
		Stack	*p, *tmp;

		for (p = on_stack; p; tmp = p, p = p->next)
		{
			if (p->which == which)
			{
				if (p == on_stack)
					on_stack = p->next;
				else
					tmp->next = p->next;
				break;
			}
		}
		if (!p)
		{
			say("No %s on the stack", args);
			return;
		}
		if (which < 0 && nhook || hook_functions[which].list)
			remove_hook(which, NULL, 0, 0, 1);	/* free hooks */
		if (which < 0)
		{
			if ((nptr = (NumericList *) find_in_list(&numeric_list,
					foo, 0)) == NULL)
			{
				nptr = (NumericList *) new_malloc(sizeof(NumericList));
				nptr->name = NULL;
				nptr->list = p->list;
				malloc_strcpy(&nptr->name, foo);
				add_to_list(&numeric_list, nptr);
			}
			else
				add_to_list(&numeric_list->list, p->list);
		}
		else
			hook_functions[which].list = p->list;
		return;
	}
	else if (type == STACK_LIST)
	{
		int	slevel = 0;
		Stack	*osptr;

		for (osptr = on_stack; osptr; osptr = osptr->next)
			if (osptr->which == which)
			{
				Hook	*hptr;

				slevel++;
				say("Level %d stack", slevel);
				for (hptr = osptr->list; hptr; hptr = hptr->next)
					show_hook(hptr, args);
			}

		if (!slevel)
			say("The STACK ON %s list is empty", args);
		return;
	}
	say("Unknown STACK ON type ??");
}

static	void
do_stack_alias(type, args, which)
	int	type;
	char	*args;
	int	which;
{
	char	*name;
	Stack	*aptr,
		**aptrptr;

	if (which == STACK_DO_ALIAS)
	{
		name = "ALIAS";
		aptrptr = &alias_stack;
	}
	else
	{
		name = "ASSIGN";
		aptrptr = &assign_stack;
	}
	if (!*aptrptr && (type == STACK_POP || type == STACK_LIST))
	{
		say("%s stack is empty!", name);
		return;
	}

	if (STACK_PUSH == type)
	{
		aptr = alias_get(args, which);
		if ((Stack *) 0 == aptr)
		{
			say("No such %s %s", name, args);
			return;
		}
		if (aptrptr)
			aptr->next = *aptrptr;
		*aptrptr = aptr;
		return;
	}
	if (STACK_POP == type)
	{
		aptr = alias_stack_find(args, which);
		if ((Stack *) 0 == aptr)
		{
			say("%s is not on the %s stack!", args, name);
			return;
		}
		alias_stack_add(aptr, which);
		return;
	}
	if (STACK_LIST == type)
	{
		int	slevel = 0;

		for (aptr = *aptrptr; aptr; aptr = aptr->next)
		{
			Alias	*tmp = aptr->list;
			if (!my_stricmp(tmp->name, args))
			{
				slevel++;
				say("Level %d stack", slevel);
				say("%s = %s", tmp->name, tmp->stuff);
			}
		}
		if (!slevel)
		{
			say("The STACK %s %s list is empty", name, args);
		}
		return;
	}
}

static	void
do_stack_set(type, args)
	int	type;
	char	*args;
{
	Stack	*aptr;

	if (!set_stack && (type == STACK_POP || type == STACK_LIST))
	{
		say("SET stack is empty!");
		return;
	}

	if (!args || !*args)
	{
		say("Missing variable name for STACK SET");
		return;
	}

	if (STACK_PUSH == type)
	{
		Stack		*new;
		IrcVariable	*list;
		int		which;

		list = (IrcVariable *) new_malloc(sizeof(IrcVariable));
		memset(list, 0, sizeof(IrcVariable));
		if (list)
		{
			which = variable_get(args, list);
			if (which >= 0)
			{
				new = (Stack *) new_malloc(sizeof(Stack));
				if (new)
				{
					new->next = set_stack;
					set_stack = new;
					new->which = which;
					new->list = list;
					return;
				}
			}
			else
			{
				say("No such SET %s", args);
				return;
			}
		}
		say("Whoa!  Out of memory?!");
	}
	else if (STACK_POP == type)
	{
		Stack	*p, *last = NULL;
		int	which;

		which = variable_get(args, NULL);
		if (which < 0)
		{
			say("No such SET %s", args);
			return;
		}

		for (p = set_stack; p; p = p->next)
		{
			if (p->which == which)
			{
				IrcVariable	*list = (IrcVariable *)p->list;

				if (last)
				{
					last->next = p->next;
				}
				else
				{
					set_stack = p->next;
				}
				variable_set(args, list);
				if (list->string)
				{
					new_free(&list->string);
				}
				new_free(&list);
				new_free(&p);
				return;
			}
			last = p;
		}
		say("%s is not on the SET stack!", args);
		return;
	}
	else if (STACK_LIST == type)
	{
		Stack	*ssptr;
		int	slevel = 0;
		int	which;

		which = variable_get(args, NULL);
		if (which < 0)
		{
			say("No such SET %s", args);
			return;
		}

		for (ssptr = set_stack; ssptr; ssptr = ssptr->next)
		{
			if (ssptr->which == which)
			{
				IrcVariable	*vptr;

				slevel++;
				say("Level %d stack", slevel);
				vptr = (IrcVariable *) ssptr->list;
				switch (vptr->type)
				{
				case BOOL_TYPE_VAR:
					say("%s is %s", vptr->name,
						vptr->integer ? "ON" : "OFF");
					break;
				case CHAR_TYPE_VAR:
					say("%s is %c", vptr->name,
						vptr->integer);
					break;
				case INT_TYPE_VAR:
					say("%s is %d", vptr->name,
						vptr->integer);
					break;
				case STR_TYPE_VAR:
					if (vptr->string)
					{
						say("%s is %s", vptr->name,
							vptr->string);
					}
					else
					{
						say("No value for %s",
							vptr->name);
					}
					break;
				}
			}
		}
		if (!slevel)
		{
			say("The STACK SET %s list is empty", args);
		}
		return;
	}
}

static	void
do_stack_format(type, args)
	int	type;
	char	*args;
{
	Stack	*aptr;
	int	numeric;

	if (!format_stack && !nformat_stack &&
		(type == STACK_POP || type == STACK_LIST))
	{
		say("FORMAT stack is empty!");
		return;
	}

	if (!args || !*args)
	{
		say("Missing variable name for STACK FORMAT");
		return;
	}

	numeric = is_number(args);

	if (STACK_PUSH == type)
	{
		Stack		*new;
		int		which;

		if (numeric)
		{
			NumFormat	*list;

			list = (NumFormat *) new_malloc(sizeof(NumFormat));
			memset(list, 0, sizeof(NumFormat));
			which = format_nget(atoi(args), list);
			if (which >= 0)
			{
				new = (Stack *) new_malloc(sizeof(Stack));
				new->next = nformat_stack;
				nformat_stack = new;
				new->which = which;
				new->list = list;
				return;
			}
		}
		else
		{
			IrcFormat	*list;

			list = (IrcFormat *) new_malloc(sizeof(IrcFormat));
			memset(list, 0, sizeof(IrcFormat));
			which = format_get(args, list);
			if (which >= 0)
			{
				new = (Stack *) new_malloc(sizeof(Stack));
				new->next = format_stack;
				format_stack = new;
				new->which = which;
				new->list = list;
				return;
			}
		}
		say("No such FORMAT %s", args);
		return;

	}
	else if (STACK_POP == type)
	{
		Stack	*p, *last = NULL;
		int	which;

		if (numeric)
		{
			which = atoi(args);
			for (p = nformat_stack; p; p = p->next)
			{
				if (p->which = which)
				{
					NumFormat *list = (NumFormat *)p->list;

					if (last)
					{
						last->next = p->next;
					}
					else
					{
						nformat_stack = p->next;
					}
					format_nset(which, list);
					if (list->fmt)
					{
						new_free(&list->fmt);
					}
					new_free(&list);
					new_free(&p);
					return;
				}
				last = p;
			}
		}
		else
		{
			which = format_get(args, NULL);
			if (which < 0)
			{
				say("No such FORMAT %s", args);
				return;
			}

			for (p = format_stack; p; p = p->next)
			{
				if (p->which == which)
				{
					IrcFormat *list = (IrcFormat *)p->list;

					if (last)
					{
						last->next = p->next;
					}
					else
					{
						format_stack = p->next;
					}
					format_set(args, list);
					if (list->fmt)
					{
						new_free(&list->fmt);
					}
					new_free(&list);
					new_free(&p);
					return;
				}
				last = p;
			}
		}
		say("%s is not on the FORMAT stack!", args);
		return;
	}
	else if (STACK_LIST == type)
	{
		Stack	*fsptr;
		int	slevel = 0;
		int	which;

		if (numeric)
		{
			which = atoi(args);

			for (fsptr = nformat_stack; fsptr; fsptr = fsptr->next)
			{
				if (fsptr->which == which)
				{
					NumFormat	*fptr;

					slevel++;
					say("Level %d stack", slevel);
					fptr = (NumFormat *) fsptr->list;
					if (fptr->fmt)
					{
						say("%3.3d is %s", fptr->num,
							fptr->fmt);
					}
					else
					{
						say("No value for %3.3d",
							fptr->num);
					}
				}
			}
		}
		else
		{
			which = format_get(args, NULL);
			if (which < 0)
			{
				say("No such FORMAT %s", args);
				return;
			}

			for (fsptr = format_stack; fsptr; fsptr = fsptr->next)
			{
				if (fsptr->which == which)
				{
					IrcFormat	*fptr;

					slevel++;
					say("Level %d stack", slevel);
					fptr = (IrcFormat *) fsptr->list;
					if (fptr->fmt)
					{
						say("%s is %s", fptr->name,
							fptr->fmt);
					}
					else
					{
						say("No value for %s",
							fptr->name);
					}
				}
			}
		}
		if (!slevel)
		{
			say("The STACK FORMAT %s list is empty", args);
		}
		return;
	}
}

/*
 * alias_get: this returns a point to an `Stack' structure that
 * has be extracted from the current aliases, and removed from that
 * list.
 */
static	Stack	*
alias_get(args, which)
	char	*args;
	int	which;
{
	Alias	*tmp;

	if (which == STACK_DO_ALIAS)
	{
		tmp = alias_list[0];
	}
	else
	{
		tmp = alias_list[1];
	}

	while (tmp)
	{
		if (!my_stricmp(args, tmp->name))
		{
			Alias	*new;
			Stack	*asptr;

			new = (Alias *) new_malloc(sizeof(Alias));
			asptr = (Stack *) new_malloc(sizeof(Stack));

			memset(new, 0, sizeof(Alias));
			malloc_strcpy(&new->name, tmp->name);
			malloc_strcpy(&new->stuff, tmp->stuff);
			new->mark = tmp->mark;
			new->global = tmp->global;
			new->next = NULL;

			asptr->list = new;
			asptr->which = which;
			asptr->next = NULL;

			return asptr;
		}
		tmp = tmp->next;
	}

	return (Stack *) 0;
}

/*
 * alias_stack_find: this returns the pointer to the struct with the
 * most recent alias for `args' in the stack.
 */
static	Stack	*
alias_stack_find(args, which)
	char	*args;
	int	which;
{
	Stack	*tmp, *prev = NULL;

	if (which == STACK_DO_ALIAS)
	{
		tmp = alias_stack;
	}
	else
	{
		tmp = assign_stack;
	}

	while (tmp)
	{
		if (!my_stricmp(args, ((Alias *)(tmp->list))->name))
		{
			if (prev)
			{
				prev->next = tmp->next;
			}
			else
			{
				if (which == STACK_DO_ALIAS)
				{
					alias_stack = tmp->next;
				}
				else
				{
					assign_stack = tmp->next;
				}
			}
			tmp->next = NULL;
			return tmp;
		}
		prev = tmp;
		tmp = tmp->next;
	}

	return (Stack *) 0;
}

/*
 * alias_stack_add: this adds `aptr' to the alias/assign stack.
 */
static	void
alias_stack_add(aptr, which)
	Stack *aptr;
	int which;
{
	Alias	*find, **prev;
	Alias	*tmp = (Alias *) aptr->list;
	int	cmp;

	if (which == STACK_DO_ALIAS)
	{
		prev = &alias_list[0];
		find = alias_list[0];
	}
	else
	{
		prev = &alias_list[1];
		find = alias_list[1];
	}

	while (1)
	{
		if (!find || ((cmp = my_stricmp(tmp->name, find->name)) <= 0))
		{
			if (!find || (cmp < 0))
			{
				tmp->next = *prev;
				*prev = tmp;
			}
			else if (cmp == 0)
			{
				new_free(&find->stuff);
				find->stuff = tmp->stuff;
				find->mark = tmp->mark;
				find->global = tmp->global;
				new_free(&tmp);
			}
			new_free(&aptr);
			return;
		}

		prev = &find->next;
		find = find->next;
	}

	return;
}

extern	void
stackcmd(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	char	*arg;
	int	len, type;

	if ((arg = next_arg(args, &args)) != NULL)
	{
		len = strlen(arg);
		if (!my_strnicmp(arg, "PUSH", len))
			type = STACK_PUSH;
		else if (!my_strnicmp(arg, "POP", len))
			type = STACK_POP;
		else if (!my_strnicmp(arg, "LIST", len))
			type = STACK_LIST;
		else
		{
			say("%s is unknown stack type", arg);
			return;
		}
	}
	else
	{
		say("Need operation for STACK");
		return;
	}
	if ((arg = next_arg(args, &args)) != NULL)
	{
		len = strlen(arg);
		if (!my_strnicmp(arg, "ON", len))
			do_stack_on(type, args);
		else if (!my_strnicmp(arg, "ALIAS", len))
			do_stack_alias(type, args, STACK_DO_ALIAS);
		else if (!my_strnicmp(arg, "ASSIGN", len))
			do_stack_alias(type, args, STACK_DO_ASSIGN);
		else if (!my_strnicmp(arg, "SET", len))
			do_stack_set(type, args);
		else if (!my_strnicmp(arg, "FORMAT", len))
			do_stack_format(type, args);
		else
		{
			say("%s is not a valid STACK type");
			return;
		}
	}
	else
	{
		say("Need stack type for STACK");
		return;
	}
}
