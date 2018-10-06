/*
 * fileio.c: File I/O functions and commands
 *
 * Written By Timothy Jensen
 *
 * Copyright(c) 1999 
 *
 * See the COPYRIGHT file, or do a HELP COPYRIGHT 
 */

#ifndef lint
static	char	rcsid[] = "@(#)$Id: fileio.c,v 1.9 2001/12/09 03:02:17 toast Exp $";
#endif

#include "irc.h"

#include <stdio.h>
#include "ircaux.h"
#include "config.h"
#include "fileio.h"

static FILE	*FileList[MAX_FILES];

void
init_files()
{
	int	i;

	for (i = 0; i < MAX_FILES; i++)
		FileList[i] = (FILE *) 0;
}

char	*
function_fopen(input)
	char	*input;
{
	char	tmp[16];
	char	*result = (char *) 0;
	char	*mode;
	int	i;

	for (i = 0; i < MAX_FILES; i++)
	{
		if (FileList[i] == (FILE *) 0)
		{
			break;
		}
	}

	if (input && *input && (i < MAX_FILES))
	{
		mode = next_arg(input, &input);
		if (mode && *mode && input && *input)
		{
			if ((FileList[i] = fopen(input, mode)) != NULL)
			{
				sprintf(tmp, "%d", i+1);
				malloc_strcpy(&result, tmp);
			}
			else
			{
				malloc_strcpy(&result, "0");
			}
		}
		else
		{
			malloc_strcpy(&result, "0");
		}
	}
	else
	{
		malloc_strcpy(&result, "0");
	}

	return result;
}

char	*
function_fclose(input)
	char	*input;
{
	char	tmp[16];
	char	*result = (char *) 0;
	char	*filenum;
	int	i, retval;

	if (input && *input)
	{
		filenum = next_arg(input, &input);
		if (filenum && *filenum && is_number(filenum) &&
			((i = atoi(filenum)) > 0) && (i <= MAX_FILES) &&
			(FileList[i-1] != (FILE *)0))
		{
			retval = fclose(FileList[i-1]);
			FileList[i-1] = (FILE *) 0;
			sprintf(tmp, "%d", retval);
			malloc_strcpy(&result, tmp);
		}
		else
		{
			malloc_strcpy(&result, "-1");
		}
	}
	else
	{
		malloc_strcpy(&result, "-1");
	}

	return result;
}

void
fclosecmd(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	char	*filenum;
	int	i;

	if (args && *args)
	{
		filenum = next_arg(args, &args);
		if (filenum && *filenum && is_number(filenum) &&
			((i = atoi(filenum)) > 0) && (i <= MAX_FILES) &&
			(FileList[i-1] != (FILE *)0))
		{
			(void)fclose(FileList[i-1]);
			FileList[i-1] = (FILE *) 0;
		}
		else
		{
			say("Invalid filenum: %s",
				filenum ? filenum : empty_string);
		}
	}
	else
	{
		say("No filenum specified.");
	}
}

char	*
function_fgetc(input)
	char	*input;
{
	char	tmp[16];
	char	*result = (char *) 0;
	char	*filenum;
	int	i,
		count = 1,
		retval;

	sprintf(tmp, "%d", EOF);
	malloc_strcpy(&result, tmp);
	if (input && *input)
	{
		filenum = next_arg(input, &input);
		if (filenum && *filenum && is_number(filenum) &&
			((i = atoi(filenum)) > 0) && (i <= MAX_FILES) &&
			(FileList[i - 1] != (FILE *)0))
		{
			if (input && *input)
			{
				count = atoi(input);
			}
			if (count <= 0)
			{
				count = 1;
			}
			malloc_strcpy(&result, empty_string);
			while (count-- > 0)
			{
				retval = fgetc(FileList[i - 1]);
				sprintf(tmp, "%d", retval);
				if (*result)
				{
					malloc_strcat(&result, " ");
				}
				malloc_strcat(&result, tmp);
			}
		}
	}


	return result;
}

char	*
function_feof(input)
	char	*input;
{
	char	tmp[16];
	char	*result = (char *) 0;
	char	*filenum;
	int	i, retval;

	if (input && *input)
	{
		filenum = next_arg(input, &input);
		if (filenum && *filenum && is_number(filenum) &&
			((i = atoi(filenum)) > 0) && (i <= MAX_FILES) &&
			(FileList[i-1] != (FILE *)0))
		{
			retval = feof(FileList[i-1]);
			sprintf(tmp, "%d", retval);
			malloc_strcpy(&result, tmp);
		}
		else
		{
			malloc_strcpy(&result, "0");
		}
	}
	else
	{
		malloc_strcpy(&result, "0");
	}

	return result;
}

char	*
function_fwrite(input)
	char	*input;
{
	char	*result = (char *) 0;
	char	*filenum;
	int	i;

	if (input && *input)
	{
		filenum = next_arg(input, &input);
		if (input && *input && filenum && *filenum &&
			is_number(filenum) && ((i = atoi(filenum)) > 0) &&
			(i <= MAX_FILES) && (FileList[i-1] != (FILE *)0))
		{
			fprintf(FileList[i-1], "%s", input);
			malloc_strcpy(&result, "0");
		}
		else
		{
			malloc_strcpy(&result, "-1");
		}
	}
	else
	{
		malloc_strcpy(&result, "-1");
	}

	return result;
}

void
fwritecmd(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	char	*filenum;
	int	i;

	if (args && *args)
	{
		filenum = next_arg(args, &args);
		if (filenum && *filenum && is_number(filenum) &&
			((i = atoi(filenum)) > 0) && (i <= MAX_FILES) &&
			(FileList[i-1] != (FILE *)0))
		{
			if (args && *args)
			{
				fprintf(FileList[i-1], "%s", args);
			}
		}
		else
		{
			say("Invalid filenum: %s",
				filenum ? filenum : empty_string);
		}
	}
	else
	{
		say("No filenum specified.");
	}
}

char	*
function_fwriteln(input)
	char	*input;
{
	char	*result = (char *) 0;
	char	*filenum;
	int	i;

	if (input && *input)
	{
		filenum = next_arg(input, &input);
		if (input && filenum && *filenum && is_number(filenum) &&
			((i = atoi(filenum)) > 0) && (i <= MAX_FILES) &&
			(FileList[i-1] != (FILE *)0))
		{
			fprintf(FileList[i-1], "%s\n", input);
			malloc_strcpy(&result, "0");
		}
		else
		{
			malloc_strcpy(&result, "-1");
		}
	}
	else
	{
		malloc_strcpy(&result, "-1");
	}

	return result;
}

void
fwritelncmd(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	char	*filenum;
	int	i;

	if (args && *args)
	{
		filenum = next_arg(args, &args);
		if (filenum && *filenum && is_number(filenum) &&
			((i = atoi(filenum)) > 0) && (i <= MAX_FILES) &&
			(FileList[i-1] != (FILE *)0))
		{
			if (args)
			{
				fprintf(FileList[i-1], "%s\n", args);
			}
		}
		else
		{
			say("Invalid filenum: %s",
				filenum ? filenum : empty_string);
		}
	}
	else
	{
		say("No filenum specified.");
	}
}

char	*
function_fputc(input)
	char	*input;
{
	char	tmp[16];
	char	*result = (char *) 0;
	char	*filenum, *charval;
	int	i, c, count = 0;

	if (input && *input)
	{
		filenum = next_arg(input, &input);
		if (filenum && *filenum && is_number(filenum) &&
			((i = atoi(filenum)) > 0) && (i <= MAX_FILES) &&
			(FileList[i-1] != (FILE *)0))
		{
			while (input && *input)
			{
				if ((charval = next_arg(input, &input)) &&
					charval && *charval &&
					is_number(charval))
				{
					c = atoi(charval);
					fprintf(FileList[i-1], "%c", c);
					count++;
				}
			}
			sprintf(tmp, "%d", count);
			malloc_strcpy(&result, tmp);
		}
		else
		{
			malloc_strcpy(&result, "0");
		}
	}
	else
	{
		malloc_strcpy(&result, "0");
	}

	return result;
}

void
fputccmd(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	char	*result = (char *) 0;
	char	*filenum, *charval;
	int	i, c;

	if (args && *args)
	{
		filenum = next_arg(args, &args);
		if (filenum && *filenum && is_number(filenum) &&
			((i = atoi(filenum)) > 0) && (i <= MAX_FILES) &&
			(FileList[i-1] != (FILE *)0))
		{
			while (args && *args)
			{
				if ((charval = next_arg(args, &args)) &&
					charval && *charval &&
					is_number(charval))
				{
					c = atoi(charval);
					fprintf(FileList[i-1], "%c", c);
				}
			}
		}
		else
		{
			say("Invalid filenum: %s",
				filenum ? filenum : empty_string);
		}
	}
	else
	{
		say("No filenum specified.");
	}
}

char	*
function_ftell(input)
	char	*input;
{
	char	tmp[16];
	char	*result = (char *) 0;
	char	*filenum;
	int	i;
	long	retval;

	if (input && *input)
	{
		filenum = next_arg(input, &input);
		if (filenum && *filenum && is_number(filenum) &&
			((i = atoi(filenum)) > 0) && (i <= MAX_FILES) &&
			(FileList[i-1] != (FILE *)0))
		{
			retval = ftell(FileList[i-1]);
			sprintf(tmp, "%d", retval);
			malloc_strcpy(&result, tmp);
		}
		else
		{
			malloc_strcpy(&result, "-1");
		}
	}
	else
	{
		malloc_strcpy(&result, "-1");
	}

	return result;
}

static	int
get_whence(value)
	char	*value;
{
	int	val;

	if (value && *value)
	{
		if (is_number(value))
		{
			val = atoi(value);
			if ((val == SEEK_SET) || (val == SEEK_CUR) ||
				(val == SEEK_END))
			{
				return val;
			}
		}
		else
		{
			if (!my_stricmp(value, "SEEK_SET"))
			{
				return SEEK_SET;
			}
			else if (!my_stricmp(value, "SEEK_CUR"))
			{
				return SEEK_CUR;
			}
			else if (!my_stricmp(value, "SEEK_END"))
			{
				return SEEK_END;
			}
		}
	}
	return -1;
}

static	int
is_valid_whence(value)
	char	*value;
{
	int	val;

	if (value && *value)
	{
		if (is_number(value))
		{
			val = atoi(value);
			if ((val == SEEK_SET) || (val == SEEK_CUR) ||
				(val == SEEK_END))
			{
				return 1;
			}
		}
		else
		{
			if (!my_stricmp(value, "SEEK_SET") ||
				!my_stricmp(value, "SEEK_CUR") ||
				!my_stricmp(value, "SEEK_END"))
			{
				return 1;
			}
		}
	}
	return 0;
}

char	*
function_fseek(input)
	char	*input;
{
	char	tmp[16];
	char	*result = (char *) 0;
	char	*filenum, *offsetstr, *whencestr;
	long	offset;
	int	whence;
	int	i, retval;

	if (input && *input)
	{
		filenum = next_arg(input, &input);
		offsetstr = next_arg(input, &input);
		whencestr = next_arg(input, &input);
		if (filenum && *filenum && offsetstr && *offsetstr &&
			whencestr && *whencestr && is_number(filenum) &&
			is_number(offsetstr) && is_valid_whence(whencestr) &&
			((i = atoi(filenum)) > 0) && (i <= MAX_FILES) &&
			(FileList[i-1] != (FILE *)0))
		{
			offset = atol(offsetstr);
			whence = get_whence(whencestr);
			retval = fseek(FileList[i-1], offset, whence);
			sprintf(tmp, "%d", retval);
			malloc_strcpy(&result, tmp);
		}
		else
		{
			malloc_strcpy(&result, "-1");
		}
	}
	else
	{
		malloc_strcpy(&result, "-1");
	}

	return result;
}

void
fseekcmd(commands, args, subargs)
	char	*commands,
		*args,
		*subargs;
{
	char	*filenum, *offsetstr, *whencestr;
	long	offset;
	int	whence;
	int	i;

	if (args && *args)
	{
		filenum = next_arg(args, &args);
		offsetstr = next_arg(args, &args);
		whencestr = next_arg(args, &args);
		if (filenum && *filenum && is_number(filenum) &&
			((i = atoi(filenum)) > 0) && (i <= MAX_FILES) &&
			(FileList[i-1] != (FILE *)0))
		{
			if (offsetstr && *offsetstr && is_number(offsetstr))
			{
				if (whencestr && *whencestr &&
					is_valid_whence(whencestr))
				{
					offset = atol(offsetstr);
					whence = get_whence(whencestr);
					(void)fseek(FileList[i-1], offset,
						whence);
				}
				else
				{
					say("Invalid whence value: %s",
						whencestr ? whencestr :
						empty_string);
				}
			}
			else
			{
				say("Invalid offset: %s",
					offsetstr ? offsetstr : empty_string);
			}
		}
		else
		{
			say("Invalid filenum: %s",
				filenum ? filenum : empty_string);
		}
	}
	else
	{
		say("No filenum specified.");
	}
}

char	*
function_fread(input)
	char	*input;
{
	char	tmp[BIG_BUFFER_SIZE + 1];
	char	*result = (char *) 0;
	char	*filenum;
	int	i, j = 0;
	char	c;

	if (input && *input)
	{
		filenum = next_arg(input, &input);
		if (filenum && *filenum && is_number(filenum) &&
			((i = atoi(filenum)) > 0) && (i <= MAX_FILES) &&
			(FileList[i-1] != (FILE *)0))
		{
			bzero(tmp, BIG_BUFFER_SIZE + 1);
			while ((j < BIG_BUFFER_SIZE) &&
				((c = fgetc(FileList[i-1])) != EOF))
			{
				if (c == ' ' || c == '\t' || c == '\n')
				{
					if (*tmp)
					{
						break;
					}
					else
					{
						continue;
					}
				}
				tmp[j++] = c;
			}
			malloc_strcpy(&result, tmp);
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
function_freadln(input)
	char	*input;
{
	char	tmp[BIG_BUFFER_SIZE + 1];
	char	*result = (char *) 0;
	char	*filenum;
	int	i, j = 0;
	char	c;

	if (input && *input)
	{
		filenum = next_arg(input, &input);
		if (filenum && *filenum && is_number(filenum) &&
			((i = atoi(filenum)) > 0) && (i <= MAX_FILES) &&
			(FileList[i-1] != (FILE *)0))
		{
			bzero(tmp, BIG_BUFFER_SIZE + 1);
			while ((j < BIG_BUFFER_SIZE) &&
				((c = fgetc(FileList[i-1])) != EOF))
			{
				if (c == '\n')
				{
					break;
				}
				tmp[j++] = c;
			}
			malloc_strcpy(&result, tmp);
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
