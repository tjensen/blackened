#ifndef _FILEIO_H_
#define _FILEIO_H_

void	init_files();
char	*function_fopen(char *);
char	*function_fclose(char *);
void	fclosecmd(char *, char *, char *);
char	*function_feof(char *);
char	*function_fgetc(char *);
char	*function_fwrite(char *);
void	fwritecmd(char *, char *, char *);
char	*function_fwriteln(char *);
void	fwritelncmd(char *, char *, char *);
char	*function_fputc(char *);
void	fputccmd(char *, char *, char *);
char	*function_ftell(char *);
char	*function_fseek(char *);
void	fseekcmd(char *, char *, char *);
char	*function_fread(char *);
char	*function_freadln(char *);

#endif
