/*
 * crypt.c: handles some encryption of messages stuff. 
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 */

#ifndef lint
static	char	rcsid[] = "@(#)$Id: crypt.c,v 1.1.1.1 1999/07/16 21:21:42 toast Exp $";
#endif

#include "irc.h"
#include "crypt.h"
#include "vars.h"
#include "ircaux.h"
#include "list.h"
#include "ctcp.h"
#include "output.h"

#define CRYPT_BUFFER_SIZE (IRCD_BUFFER_SIZE - 50)	/* Make this less than
							 * the trasmittable
							 * buffer */
/*
 * Crypt: the crypt list structure,  consists of the nickname, and the
 * encryption key 
 */
typedef struct	CryptStru
{
	struct	CryptStru *next;
	char	*nick;
	char	*key;
}	Crypt;

/* crypt_list: the list of nicknames and encryption keys */
static	Crypt	*crypt_list = (Crypt *) 0;

/*
 * add_to_crypt: adds the nickname and key pair to the crypt_list.  If the
 * nickname is already in the list, then the key is changed the the supplied
 * key. 
 */
static	void
add_to_crypt(nick, key)
	char	*nick;
	char	*key;
{
	Crypt	*new;

	if ((new = (Crypt *) remove_from_list(&crypt_list, nick)) != NULL)
	{
		new_free(&(new->nick));
		new_free(&(new->key));
		new_free(&new);
	}
	new = (Crypt *) new_malloc(sizeof(Crypt));
	new->nick = (char *) 0;
	new->key = (char *) 0;
	malloc_strcpy(&(new->nick), nick);
	malloc_strcpy(&(new->key), key);
	/* upper(new->nick); */
	add_to_list(&crypt_list, new);
}

/*
 * remove_crypt: removes the given nickname from the crypt_list, returning 0
 * if successful, and 1 if not (because the nickname wasn't in the list) 
 */
static	int
remove_crypt(nick)
	char	*nick;
{
	Crypt	*tmp;

	if ((tmp = (Crypt *) list_lookup(&crypt_list, nick, !USE_WILDCARDS,
			REMOVE_FROM_LIST)) != NULL)
	{
		new_free(&(tmp->nick));
		new_free(&(tmp->key));
		new_free(&tmp);
		return (0);
	}
	return (1);
}

/*
 * is_crypted: looks up nick in the crypt_list and returns the encryption key
 * if found in the list.  If not found in the crypt_list, null is returned. 
 */
char	*
is_crypted(nick)
	char	*nick;
{
	Crypt	*tmp;

	if (!crypt_list)
		return NULL;
	if ((tmp = (Crypt *) list_lookup(&crypt_list, nick,
			!USE_WILDCARDS, !REMOVE_FROM_LIST)) != NULL)
		return (tmp->key);
	return NULL;
}

/*
 * encrypt_cmd: the ENCRYPT command.  Adds the given nickname and key to the
 * encrypt list, or removes it, or list the list, you know. 
 */
/*ARGSUSED*/
void
encrypt_cmd(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	char	*nick,
	*key;

	if ((nick = next_arg(args, &args)) != NULL)
	{
		if ((key = next_arg(args, &args)) != NULL)
		{
			add_to_crypt(nick, key);
			say("%s added to the crypt with key %s", nick, key);
		}
		else
		{
			if (remove_crypt(nick))
				say("No such nickname in the crypt: %s", nick);
			else
				say("%s removed from the crypt", nick);
		}
	}
	else
	{
		if (crypt_list)
		{
			Crypt	*tmp;

			say("The crypt:");
			for (tmp = crypt_list; tmp; tmp = tmp->next)
				put_it("%s with key %s", tmp->nick, tmp->key);
		}
		else
			say("The crypt is empty");
	}
}

static	void
encrypt_str(str, len, key)
	char	*str,
		*key;
	int	len;
{
	int	key_len,
	key_pos,
	i;
	char	mix,
	tmp;

	key_len = strlen(key);
	key_pos = 0;
	/*    mix = key[key_len-1]; */
	mix = 0;
	for (i = 0; i < len; i++)
	{
		tmp = str[i];
		str[i] = mix ^ tmp ^ key[key_pos];
		mix ^= tmp;
		key_pos = (key_pos + 1) % key_len;
	}
	str[i] = (char) 0;
}

static	void
decrypt(str, len, key)
	char	*str,
		*key;
	int	len;
{
	int	key_len,
		key_pos,
		i;
	char	mix,
		tmp;

	key_len = strlen(key);
	key_pos = 0;
	/*    mix = key[key_len-1]; */
	mix = 0;
	for (i = 0; i < len; i++)
	{
		tmp = mix ^ str[i] ^ key[key_pos];
		str[i] = tmp;
		mix ^= tmp;
		key_pos = (key_pos + 1) % key_len;
	}
	str[i] = (char) 0;
}

static	char	*
do_crypt(str, key, flag)
	char	*str,
		*key;
	int	flag;
{
	int	in[2],
	out[2],
	c;
	char	buffer[CRYPT_BUFFER_SIZE + 1];
	char	*ptr = (char *) 0,
		*encrypt_program;

	encrypt_program = get_string_var(ENCRYPT_PROGRAM_VAR);
	if (encrypt_program)
	{
#ifdef DAEMON_UID
		if (DAEMON_UID == getuid())
		{
			say("ENCRYPT_PROGRAM not available from daemon mode");
			return (char *) 0;
		}
#endif
		in[0] = in[1] = -1;
		out[0] = out[1] = -1;
		if (access(encrypt_program, X_OK))
		{
			say("Unable to execute encryption program: %s", encrypt_program);
			return ((char *) 0);
		}
		if (!flag)
		{
			c = strlen(str);
			ptr = ctcp_unquote_it(str, &c);
		}
		else
			malloc_strcpy(&ptr, str);
		if (pipe(in) || pipe(out))
		{
			say("Unable to start encryption process: %s", strerror(errno));
			if (in[0] != -1)
			{
				close(in[0]);
				close(in[1]);
			}
			if (out[0] != -1)
			{
				close(out[0]);
				close(out[1]);
			}
		}
		switch (fork())
		{
		case -1:
			say("Unable to start encryption process: %s", strerror(errno));
			return ((char *) 0);
		case 0:
			MY_SIGNAL(SIGINT, SIG_IGN, 0);
			dup2(out[1], 1);
			dup2(in[0], 0);
			close(out[0]);
			close(in[1]);
			setuid(getuid());
			setgid(getgid());
			execl(encrypt_program, encrypt_program, key, NULL);
			exit(0);
		default:
			close(out[1]);
			close(in[0]);
			write(in[1], ptr, strlen(ptr));
			close(in[1]);
			c = read(out[0], buffer, CRYPT_BUFFER_SIZE);
			buffer[c] = (char) 0;
			close(out[0]);
			break;
		}
		new_free(&ptr);
		if (flag)
			ptr = ctcp_quote_it(buffer, strlen(buffer));
		else
			malloc_strcpy(&ptr, buffer);
	}
	else
	{
		c = strlen(str);
		if (flag)
		{
			encrypt_str(str, c, key);
			ptr = ctcp_quote_it(str, c);
		}
		else
		{
			ptr = ctcp_unquote_it(str, &c);
			decrypt(ptr, c, key);
		}
	}
	return (ptr);
}

/*
 * crypt_msg: Executes the encryption program on the given string with the
 * given key.  If flag is true, the string is encrypted and the returned
 * string is ready to be sent over irc.  If flag is false, the string is
 * decrypted and the returned string should be readable 
 */
char	*
crypt_msg(str, key, flag)
	char	*str,
		*key;
	int	flag;
{
	static	char	buffer[CRYPT_BUFFER_SIZE + 1];
	char	thing[6];
	char	*ptr,
		*rest;
	int	on = 1;

	if (flag)
	{
		sprintf(thing, "%cSED ", CTCP_DELIM_CHAR);
		*buffer = (char) 0;
		while ((rest = index(str, '\005')) != NULL)
		{
			*(rest++) = (char) 0;
			if (on && *str && (ptr = do_crypt(str, key, flag)))
			{
				strmcat(buffer, thing, CRYPT_BUFFER_SIZE);
				strmcat(buffer, ptr, CRYPT_BUFFER_SIZE);
				strmcat(buffer, CTCP_DELIM_STR, CRYPT_BUFFER_SIZE);
				new_free(&ptr);
			}
			else
				strmcat(buffer, str, CRYPT_BUFFER_SIZE);
			on = !on;
			str = rest;
		}
		if (on && (ptr = do_crypt(str, key, flag)))
		{
			strmcat(buffer, thing, CRYPT_BUFFER_SIZE);
			strmcat(buffer, ptr, CRYPT_BUFFER_SIZE);
			strmcat(buffer, CTCP_DELIM_STR, CRYPT_BUFFER_SIZE);
			new_free(&ptr);
		}
		else
			strmcat(buffer, str, CRYPT_BUFFER_SIZE);
	}
	else
	{
		if ((ptr = do_crypt(str, key, flag)) != NULL)
		{
			strmcpy(buffer, ptr, CRYPT_BUFFER_SIZE);
			new_free(&ptr);
		}
		else
			strmcat(buffer, str, CRYPT_BUFFER_SIZE);
	}
	return (buffer);
}

