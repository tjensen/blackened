/*
 * Here we define how our menus are held
 *
 * @(#)$Id: menu.h,v 1.1.1.1 1999/07/16 21:21:14 toast Exp $
 */

#ifndef _MENU_H_
#define _MENU_H_

#define IRCII_MENU_H

#define	SMF_ERASE	0x0001
#define	SMF_NOCURSOR	0x0002
#define	SMF_CURSONLY	0x0004
#define	SMF_CALCONLY	0x0008

struct	MenuOptionTag
{
	char	*Name;
	char	*Arguments;
	void	(*Func)();
};

typedef	struct	MenuOptionTag	MenuOption;

struct	MenuTag
{
	struct	MenuTag	*next;
	char	*Name;
	int	TotalOptions;
	MenuOption	**Options;
};

typedef struct MenuTag Menu;

/* Below are our known menu functions */
extern	void	menu_previous();	/* Go to previous menu */
extern	void	menu_submenu();		/* Invoke a submenu */
extern	void	menu_exit();		/* Exit the menu */
extern	void	menu_channels();	/* List of channels menu */
extern	void	menu_command();		/* Invoke an IRCII command */
extern	void	menu_key();
extern	void	load_menu();
extern	int	ShowMenu();
extern	int	ShowMenuByWindow();

#endif /* _MENU_H_ */
