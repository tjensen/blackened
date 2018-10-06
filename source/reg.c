/*
 * The original was spagetti. I have replaced Michael's code with some of
 * my own which is a thousand times more readable and can also handle '%',
 * which substitutes anything except a space. This should enable people
 * to position things better based on argument. I have also added '?', which
 * substitutes to any single character. And of course it still handles '*'.
 * this should be more efficient than the previous version too.
 *
 * Thus this whole file becomes:
 *
 * Written By Troy Rollo
 *
 * Copyright(c) 1992
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 */

#if 0
static	char	rcsid[] = "@(#)$Id: reg.c,v 1.1.1.1 1999/07/16 21:21:43 toast Exp $";
#endif

#include "irc.h"
#include "ircaux.h"
#include "output.h"

static	int	total_explicit;

/*
 * The following #define is here because we *know* its behaviour.
 * The behaviour of toupper tends to be undefined when it's given
 * a non lower case letter.
 * All the systems supported by IRCII should be ASCII
 */
#define	mkupper(c)	(((c) >= 'a' && (c) <= 'z') ? ((c) - 'a' + 'A') : c)

#ifdef __STDC__
int match(char *pattern, char *string)
#else
int	match(pattern, string)
char	*pattern,
	*string;
#endif
{
	char	type;

#if 0
	if ((!pattern || !string) && !x_debug)
	{
		yell("match: pattern or string is NULL!");
		return 0;
	}
#endif

	while (*string && *pattern && *pattern != '*' && *pattern != '%')
	{
		if (*pattern == '\\' && *(pattern + 1))
		{
			if (!*++pattern || !(mkupper(*pattern) ==
					mkupper(*string)))
				return 0;
			else
				pattern++, string++, total_explicit++;
		}


		if (*pattern == '?')
			pattern++, string++;
		else if (mkupper(*pattern) == mkupper(*string))
			pattern++, string++, total_explicit++;
		else
			break;
	}
	if (*pattern == '*' || *pattern == '%')
	{
		type = (*pattern++);
		while (*string)
		{
			if (match(pattern, string))
				return 1;
			else if (type == '*' || *string != ' ')
				string++;
			else
				break;
		}
	}
	if (!*string && !*pattern)
		return 1;
	return 0;
}

/*
 * This version of wild_match returns 1 + the  count  of characters
 * explicitly matched if a match occurs. That way we can look for
 * the best match in a list
 */
/* \\[ and \\] handling done by Jeremy Nelson
 * EPIC will not use the new pattern matcher currently used by 
 * ircii because i am not convinced that it is 1) better * and 
 * 2) i think the \\[ \\] stuff is important.
 */
#ifdef __STDC__
int wild_match (char *pattern, char *str)
#else
int	wild_match(pattern, str)
char	*pattern,
	*str;
#endif
{
	char *ptr;
	char *ptr2 = pattern;
	int nest = 0;
	char my_buff[2048];
	char *arg;
	int best_total = 0;

	total_explicit = 0;

	/* Is there a \[ in the pattern to be expanded? */
	/* This stuff here just reduces the \[ \] set into a series of
	 * one-simpler patterns and then recurses */
	if ((ptr2 = strstr(pattern, "\\[")))
	{
		/* we will have to null this out, but not until weve used it */
		char *placeholder = ptr2;
		ptr = ptr2;

		/* yes. whats the character after it? (first time
		   through is a trivial case) */
		do
		{
			switch (ptr[1]) 
			{
					/* step over it and add to nest */
				case '[' :  ptr2 = ptr + 2 ;
					    nest++;
					    break;
					/* step over it and remove nest */
				case ']' :  ptr2 = ptr + 2;
					    nest--;
					    break;
			}
		}
		/* Repeat while there are more backslashes to look at and
		 * we have are still in nested \[ \] sets
		 */
		while ((nest) && (ptr = index(ptr2, '\\')));

		/* right now, we know ptr points to a \] or to null */
		/* remember that && short circuits and that ptr will 
		   not be set to null if (nest) is zero... */
		if (ptr)
		{
			/* null out and step over the original \[ */
			*placeholder = '\0';
			placeholder += 2;

			/* null out and step over the matching \] */
			*ptr = '\0';
			ptr +=2;

			/* grab words ("" sets or space words) one at a time
			 * and attempt to match all of them.  The best value
			 * matched is the one used.
			 */
			while ((arg = new_next_arg(placeholder, &placeholder)))
			{
				int tmpval;
				strcpy(my_buff, pattern);
				strcat(my_buff, arg);
				strcat(my_buff, ptr);

				/* the total_explicit we return is whichever
				 * pattern has the highest total_explicit */
				if ((tmpval = wild_match(my_buff, str)))
				{
					if (tmpval > best_total)
						best_total = tmpval;
				}
			}
			return best_total; /* end of expansion section */
		}
		/* Possibly an unmatched \[ \] set */
		else
		{
			total_explicit = 0;
			if (match(pattern, str))
				return total_explicit + 1;
			else
			{
#if 0
				yell("Unmatched \\[ !");
#endif
				return 0;
			}
		}
	}
	/* trivial case (no expansion) when weve expanded all the way out */
	else if (match(pattern, str))
		return total_explicit+1;
	else
		return 0;
}

