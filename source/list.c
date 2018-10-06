/*
 * list.c: some generic linked list managing stuff 
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 */

#ifndef lint
static	char	rcsid[] = "@(#)$Id: list.c,v 1.1.1.1 1999/07/16 21:21:43 toast Exp $";
#endif

#include "irc.h"

#include "list.h"
#include "ircaux.h"

/*
 * These have now been made more general. You used to only be able to
 * order these lists by alphabetical order. You can now order them
 * arbitrarily. The functions are still called the same way if you
 * wish to use alphabetical order on the key string, and the old
 * function name now represents a stub function which calls the
 * new with the appropriate parameters.
 *
 * The new function name is the same in each case as the old function
 * name, with the addition of a new parameter, cmp_func, which is
 * used to perform comparisons.
 *
 */

int
add_list_strcmp(item1, item2)
	List	*item1, *item2;
{
	return my_stricmp(item1->name, item2->name);
}

int
list_strcmp(item1, str)
	List	*item1;
	char	*str;
{
	return my_stricmp(item1->name, str);
}

int
list_match(item1, str)
	List	*item1;
	char	*str;
{
	return wild_match(item1->name, str);
}

/*
 * add_to_list: This will add an element to a list.  The requirements for the
 * list are that the first element in each list structure be a pointer to the
 * next element in the list, and the second element in the list structure be
 * a pointer to a character (char *) which represents the sort key.  For
 * example 
 *
 * struct my_list{ struct my_list *next; char *name; <whatever else you want>}; 
 *
 * The parameters are:  "list" which is a pointer to the head of the list. "add"
 * which is a pre-allocated element to be added to the list.  
 */
void
add_to_list_ext(list, add, cmp_func)
	List	**list;
	List	*add;
	int	(*cmp_func)();
{
	List	*tmp,
		*last;

	if (!cmp_func)
		cmp_func = add_list_strcmp;
	last = (List *) 0;
	for (tmp = *list; tmp; tmp = tmp->next)
	{
		if (cmp_func(tmp, add) > 0)
			break;
		last = tmp;
	}
	if (last)
		last->next = add;
	else
		*list = add;
	add->next = tmp;
}

void
add_to_list(list, add)
	List	**list;
	List	*add;
{
	add_to_list_ext(list, add, NULL);
}


/*
 * find_in_list: This looks up the given name in the given list.  List and
 * name are as described above.  If wild is true, each name in the list is
 * used as a wild card expression to match name... otherwise, normal matching
 * is done 
 */
List	*
find_in_list_ext(list, name, wild, cmp_func)
	List	**list;
	char	*name;
	int	wild;
	int	(*cmp_func)();
{
	List	*tmp;
	int	best_match,
		current_match;

	if (!cmp_func)
		cmp_func = wild ? list_match : list_strcmp;
	best_match = 0;

	if (wild)
	{
		List	*match = (List *) 0;

		for (tmp = *list; tmp; tmp = tmp->next)
		{
			if ((current_match = cmp_func(tmp, name)) > best_match)
			{
				match = tmp;
				best_match = current_match;
			}
		}
		return (match);
	}
	else
	{
		for (tmp = *list; tmp; tmp = tmp->next)
			if (cmp_func(tmp, name) == 0)
				return (tmp);
	}
	return ((List *) 0);
}

List	*
find_in_list(list, name, wild)
	List	**list;
	char	*name;
	int	wild;
{
	return find_in_list_ext(list, name, wild, NULL);
}

/*
 * remove_from_list: this remove the given name from the given list (again as
 * described above).  If found, it is removed from the list and returned
 * (memory is not deallocated).  If not found, null is returned. 
 */
List	*
remove_from_list_ext(list, name, cmp_func)
	List	**list;
	char	*name;
	int	 (*cmp_func)();
{
	List	*tmp,
		*last;

	if (!cmp_func)
		cmp_func = list_strcmp;
	last = (List *) 0;
	for (tmp = *list; tmp; tmp = tmp->next)
	{
		if (cmp_func(tmp, name) == 0)
		{
			if (last)
				last->next = tmp->next;
			else
				*list = tmp->next;
			return (tmp);
		}
		last = tmp;
	}
	return ((List *) 0);
}

List	*
remove_from_list(list, name)
	List	**list;
	char	*name;
{
	return remove_from_list_ext(list, name, NULL);
}


/*
 * list_lookup: this routine just consolidates remove_from_list and
 * find_in_list.  I did this cause it fit better with some alread existing
 * code 
 */
List	*
list_lookup_ext(list, name, wild, delete, cmp_func)
	List	**list;
	char	*name;
	int	wild;
	int	delete;
	int	(*cmp_func)();
{
	List	*tmp;

	if (delete)
		tmp = remove_from_list_ext(list, name, cmp_func);
	else
		tmp = find_in_list_ext(list, name, wild, cmp_func);
	return (tmp);
}

List	*
list_lookup(list, name, wild, delete)
	List	**list;
	char	*name;
	int	wild;
	int	delete;
{
	return list_lookup_ext(list, name, wild, delete, NULL);
}
