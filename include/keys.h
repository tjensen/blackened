/*
 * keys.h: header for keys.c 
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 *
 * This file is automatically created by keys.h.proto.
 *
 * @(#)$Id: keys.h,v 1.2.2.1 2002/03/08 19:32:21 toast Exp $
 */

#ifndef __keys_h
#define __keys_h

enum KEY_TYPES {
	AUTO_MSG,
	BACKSPACE,
	BACKWARD_CHARACTER,
	BACKWARD_HISTORY,
	BACKWARD_WORD,
	BEGINNING_OF_LINE,
	CLEAR_SCREEN,
	COMMAND_COMPLETION,
	DELETE_CHARACTER,
	DELETE_NEXT_WORD,
	DELETE_PREVIOUS_WORD,
	END_OF_LINE,
	ENTER_DIGRAPH,
	ENTER_MENU,
	ERASE_LINE,
	ERASE_TO_BEG_OF_LINE,
	ERASE_TO_END_OF_LINE,
	FORWARD_CHARACTER,
	FORWARD_HISTORY,
	FORWARD_WORD,
	GROW_WINDOW,
	HIDE_WINDOW,
	KILL_WINDOW,
	LIST_WINDOWS,
	META1_CHARACTER,
	META2_CHARACTER,
	META3_CHARACTER,
	META4_CHARACTER,
	NEW_WINDOW,
	NEXT_WINDOW,
	NOTHING,
	PAGE_DOWN,
	PAGE_UP,
	PARSE_COMMAND,
	PREVIOUS_WINDOW,
	QUIT_IRC,
	QUOTE_CHARACTER,
	REFRESH_INPUTLINE,
	REFRESH_SCREEN,
	SCROLL_BACKWARD,
	SCROLL_END,
	SCROLL_FORWARD,
	SCROLL_START,
	SELF_INSERT,
	SEND_LINE,
	SHRINK_WINDOW,
	STOP_IRC,
	SWAP_LAST_WINDOW,
	SWAP_NEXT_WINDOW,
	SWAP_PREVIOUS_WINDOW,
	SWITCH_CHANNELS,
	TOGGLE_INSERT_MODE,
	TOGGLE_STOP_SCREEN,
	TRANSPOSE_CHARACTERS,
	TYPE_TEXT,
	UNSTOP_ALL_WINDOWS,
	YANK_FROM_CUTBUFFER,
	NUMBER_OF_FUNCTIONS
};

/* KeyMap: the structure of the irc keymaps */
typedef struct
{
	int	index;
	char	changed;
	int	global;
	char	*stuff;
}	KeyMap;

/* KeyMapNames: the structure of the keymap to realname array */
typedef struct
{
	char	*name;
	void	(*func) ();
}	KeyMapNames;

extern	KeyMap	keys[],
		meta1_keys[],
		meta2_keys[],
		meta3_keys[],
		meta4_keys[];
extern	KeyMapNames key_names[];

extern	void	(* get_send_line())();
extern	void	save_bindings();
extern	void	init_bindings();
extern	void	change_send_line();
extern	void	bindcmd _((char *, char *, char *));
extern	void	rbindcmd _((char *, char *, char *));
extern	void	parsekeycmd _((char *, char *, char *));
extern	void	type _((char *, char *, char *));

#endif /* __keys_h */
