/*
 * file.c contains file open/read/write routines to be used to open
 * files with the access permissions of the real UID instead of the
 * effective UID. This allows IRCII to be run setuid->root. If it
 * has effective UID == root, it will then use privileged ports to
 * connect to servers, allowing the servers, some day, to take
 * advantage of this information to ensure that the user names are
 * who they claim to be.
 *
 * It can also be run setuid->something else, with ircserv being
 * setuid->root and only runable by the given GID.
 *
 * Copyright (c) 1991 Troy Rollo
 *
 * See HELP IRCII COPYRIGHT for details.
 */

#ifndef lint
static	char	rcsid[] = "@(#)$Id: file.c,v 1.1.1.1 1999/07/16 21:21:42 toast Exp $";
#endif

#include "irc.h"

#include <sys/stat.h>

#ifdef PRIV_PORT
# undef	PRIV_PORT
#endif /* PRIV_PORT */

int
directory_writeable(file)
	char	*file;
{
	char	dir[BIG_BUFFER_SIZE+1];
	char	*ptr;

	strmcpy(dir, file, BIG_BUFFER_SIZE);
	if (ptr = rindex(dir, '/'))
	{
		if (ptr == dir
#ifdef APOLLO
		    || (ptr == dir+1 && *dir == '/')
#endif /*APOLLO*/
		    )
			ptr++;
		*ptr = '\0';
	}
	else
		strcpy(dir, ".");
	return (!access(dir, W_OK|X_OK));
}

int
ruid_open(filename, flags, mode)
	char	*filename;
	int	flags;
	int	mode;
{
	int	access_flags;
	int	fd;

	switch(flags&(O_RDONLY|O_WRONLY|O_RDWR))
	{
	case O_RDWR:
		access_flags = R_OK|W_OK;
		break;
	case O_RDONLY:
		access_flags = R_OK;
		break;
	case O_WRONLY:
		access_flags = W_OK;
		break;
	}
	if (!access(filename, access_flags))
		return open(filename, flags, mode);
	else if ((flags&O_CREAT) == O_CREAT && directory_writeable(filename))
	{
		fd = open(filename, flags, mode);
		chown(filename, getuid(), getgid());
		return fd;
	}
	else
		return -1;
}

FILE	*
ruid_fopen(filename, mode)
	char	*filename;
	char	*mode;
{
	int	access_flags;
	FILE	*fp;
	char	*tm;

	access_flags = 0;
	for (tm = mode; *tm != '\0'; tm++)
	{
		switch (*tm)
		{
		case '+':
			access_flags |= W_OK|R_OK;
			break;
		case 'r':
			access_flags |= R_OK;
			break;
		case 'w':
		case 'a':
			access_flags |= W_OK;
			break;
		case 't':	/* Text and binary - harmless */
		case 'b':
			break;
		default:	 /* Calls are guilty unless proven innocent */
			return NULL; /* :P to all those who think otherwise! */
		}
	}
	if (!access(filename, access_flags))
		return fopen(filename, mode);
	else if ((access_flags&W_OK) == W_OK && directory_writeable(filename))
	{
		fp = fopen(filename, mode);
		chown(filename, getuid(), getgid());
		return fp;
	}
	else
		return NULL;
}

int
ruid_unlink(filename)
	char	*filename;
{
	if (!access(filename, W_OK) && directory_writeable(filename))
		unlink(filename);
}

int
ruid_system(command)
	char	*command;
{
	int	pid;

	switch (pid = fork())
	{
	case 0:
		setuid(getuid());
		setgid(getgid());
		system(command);
		_exit(0);
		break;
	case -1:
		return -1;
	default:
		while(wait(0) != pid);
		return 0;
	}
}

int
ruid_stat(path, buf)
	char	*path;
	struct	stat *buf;
{
	if (!access(path, 0))
		return -1;
	else
		return stat(path, buf);
}
