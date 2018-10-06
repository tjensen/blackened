/*
 * log.h: header for log.c 
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 *
 * @(#)$Id: log.h,v 1.1.1.1 1999/07/16 21:21:14 toast Exp $
 */

#ifndef _LOG_H_
#define _LOG_H_

extern	FILE	*do_log();
extern	void	logger();
extern	void	set_log_file();
extern	void	add_to_log();

#endif /* _LOG_H_ */
