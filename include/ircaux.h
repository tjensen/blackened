/*
 * ircaux.h: header file for ircaux.c 
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 *
 * @(#)$Id: ircaux.h,v 1.4.2.2 2002/03/13 21:14:11 toast Exp $
 */

#ifndef _IRCAUX_H_
#define _IRCAUX_H_

#include <stdio.h>

extern	char	*check_nickname();
extern	char	*next_arg();
extern	char	*new_next_arg();
extern	char	*new_new_next_arg();
extern	char	*expand_twiddle();
extern	char	*upper();
extern	char	*sindex();
extern	char	*rfgets();
extern	char	*path_search();
extern	char	*double_quote();
extern	char	*new_malloc();
extern	char	*new_realloc();
extern	void	malloc_strcpy();
extern	char	*m_strdup();
extern	void	malloc_strcat();
extern	void	new_free();
extern	void	wait_new_free();
extern	FILE	*zcat();
extern	int	is_number();
extern	int	my_stricmp _((char *, char *));
extern	int	my_strnicmp _((char *, char *, int));
extern	char	*my_strstr _((char *, char *));
extern	int	scanstr _((char *, char *));
extern	int	countstr _((char *, char *));
extern	void	really_free();
extern	void	strmcpy();
extern	void	strmcat();
extern	char	*tdiff _((time_t));
extern	char	*tdiff2 _((time_t));

#ifdef NEED_STRLCPY
extern	int	strlcpy _((char *, char *, int));
#endif /* NEED_STRCPY */

#ifdef NEED_STRLCAT
extern	int	strlcat _((char *, char *, int));
#endif /* NEED_STRLCAT */

#ifdef NEED_STRPBRK
extern	char	*strpbrk _((char *, char *));
#endif /* NEED_STRPBRK */

#endif /* _IRCAUX_H_ */
