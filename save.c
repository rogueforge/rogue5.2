/*
 * save and restore routines
 *
 * @(#)save.c	4.15 (Berkeley) 5/10/82
 */

#include <curses.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include "rogue.h"

typedef struct stat STAT;

extern char version[], encstr[];
#if !defined(_XOPEN_CURSES) && !defined(__NCURSES_H)
extern bool _endwin;
#endif

STAT sbuf;

/*
 * save_game:
 *	Implement the "save game" command
 */
int
save_game()
{
    register FILE *savef;
    register int c;
    char buf[MAXSTR];

    /*
     * get file name
     */
    mpos = 0;
over:
    if (file_name[0] != '\0')
    {
	for (;;)
	{
	    msg("save file (%s)? ", file_name);
	    c = getchar();
	    mpos = 0;
	    if (c == ESCAPE)
	    {
		msg("");
		return FALSE;
	    }
	    else if (c == 'n' || c == 'N' || c == 'y' || c == 'Y')
		break;
	    else
		msg("please answer Y or N");
	}
	if (c == 'y' || c == 'Y')
	{
	    strcpy(buf, file_name);
	    goto gotfile;
	}
    }

    do
    {
	mpos = 0;
	msg("file name: ");
	buf[0] = '\0';
	if (get_str(buf, stdscr) == QUIT)
	{
quit:
	    msg("");
	    return FALSE;
	}
	mpos = 0;
gotfile:
	/*
	 * test to see if the file exists
	 */
	if (stat(buf, &sbuf) >= 0)
	{
	    for (;;)
	    {
		msg("File exists.  Do you wish to overwrite it?");
		mpos = 0;
		if ((c = readchar()) == ESCAPE)
		    goto quit;
		if (c == 'y' || c == 'Y')
		    break;
		else if (c == 'n' || c == 'N')
		    goto over;
		else
		    msg("Please answer Y or N");
	    }
	    msg("file name: %s", buf);
	}
	strcpy(file_name, buf);
	if ((savef = fopen(file_name, "w")) == NULL)
	    msg(strerror(errno));	/* fake perror() */
    } while (savef == NULL);

    /*
     * write out encrpyted file (after a stat)
     * The fwrite is to force allocation of the buffer before the write
     */
    save_file(savef);
    return TRUE;
}

/*
 * auto_save:
 *	Automatically save a file.  This is used if a HUP signal is
 *	recieved
 */
void
auto_save(int sig)
{
    register FILE *savef;
    register int i;

    for (i = 0; i < NSIG; i++)
	signal(i, SIG_IGN);
    if (file_name[0] != '\0' && (savef = fopen(file_name, "w")) != NULL)
	save_file(savef);
    endwin();
    exit(1);
}

/*
 * save_file:
 *	Write the saved game on the file
 */
void
save_file(savef)
register FILE *savef;
{
    int slines = LINES;
    int scols  = COLS;

    /*
     * close any open score file
     */
    close(fd);
    move(LINES-1, 0);
    refresh();
    fstat(fileno(savef), &sbuf);
    /*
     * DO NOT DELETE.  This forces stdio to allocate the output buffer
     * so that malloc doesn't get confused on restart
     */
    fwrite("junk", 1, 5, savef);

    fseek(savef, 0L, 0);
#if !defined(_XOPEN_CURSES) && !defined(__NCURSES_H)
    _endwin = TRUE;
#endif
    encwrite(version,strlen(version)+1,savef);
    encwrite((char *)&sbuf.st_ino,sizeof(sbuf.st_ino),savef);
    encwrite((char *)&sbuf.st_dev,sizeof(sbuf.st_dev),savef);
    encwrite((char *)&sbuf.st_ctime,sizeof(sbuf.st_ctime),savef);
    encwrite((char *)&sbuf.st_mtime,sizeof(sbuf.st_mtime),savef);
    encwrite((char *)&slines,sizeof(slines),savef);
    encwrite((char *)&scols,sizeof(scols),savef);

    rs_save_file(savef);

    fclose(savef);
}

/*
 * restore:
 *	Restore a saved game from a file with elaborate checks for file
 *	integrity from cheaters
 */
int
restore(file, envp)
register char *file;
char **envp;
{
    register int inf;
    register bool syml;
    extern char **environ;
    char buf[MAXSTR];
    STAT sbuf2;
    int slines, scols;

    if (strcmp(file, "-r") == 0)
	file = file_name;

#ifdef SIGTSTP
    /*
     * If a process can be suspended, this code wouldn't work
     */
# ifdef SIG_HOLD
    signal(SIGTSTP, SIG_HOLD);
# else
    signal(SIGTSTP, SIG_IGN);
# endif
#endif

    if ((inf = open(file, 0)) < 0)
    {
	perror(file);
	return FALSE;
    }

    fflush(stdout);
    encread(buf, strlen(version) + 1, inf);
    if (strcmp(buf, version) != 0)
    {
        printf("Sorry, saved game is out of date.\n");
        return FALSE;
    }

    fstat(inf, &sbuf2);
    syml = issymlink(file);
    if (
#ifdef WIZARD
	!wizard &&
#endif
	unlink(file) < 0)
    {
	printf("Cannot unlink file\n");
	return FALSE;
    }

    fflush(stdout);

    encread((char *)&sbuf.st_ino,sizeof(sbuf.st_ino), inf);
    encread((char *)&sbuf.st_dev,sizeof(sbuf.st_dev), inf);
    encread((char *)&sbuf.st_ctime,sizeof(sbuf.st_ctime), inf);
    encread((char *)&sbuf.st_mtime,sizeof(sbuf.st_mtime), inf);
    encread((char *)&slines,sizeof(slines),inf);
    encread((char *)&scols,sizeof(scols),inf);

    /*
     * we do not close the file so that we will have a hold of the
     * inode for as long as possible
     */

#ifdef WIZARD
    if (!wizard)
#endif
	if (sbuf2.st_ino != sbuf.st_ino || sbuf2.st_dev != sbuf.st_dev)
	{
	    printf("Sorry, saved game is not in the same file.\n");
	    return FALSE;
	}
	else if (sbuf2.st_ctime - sbuf.st_ctime > 15)
	{
	    printf("Sorry, file has been touched, so this score won't be recorded\n");
	    noscore = TRUE;
	}

	initscr();

	if (slines > LINES) 
    { 
        printf("Sorry, original game was played on a screen with %d lines.\n",slines); 
        printf("Current screen only has %d lines. Unable to restore game\n",LINES); 
        return(FALSE); 
    } 
    
    if (scols > COLS) 
    { 
        printf("Sorry, original game was played on a screen with %d columns.\n",scols); 
        printf("Current screen only has %d columns. Unable to restore game\n",COLS); 
        return(FALSE); 
    }

	hw = newwin(LINES, COLS, 0, 0);
    
	mpos = 0;
    mvprintw(0, 0, "%s: %s", file, ctime(&sbuf2.st_mtime));

    /*
     * defeat multiple restarting from the same place
     */
#ifdef WIZARD
    if (!wizard)
#endif
	if (sbuf2.st_nlink != 1 || syml)
	{
	    printf("Cannot restore from a linked file\n");
	    return FALSE;
	}

    if (rs_restore_file(inf) == FALSE)
    {
        printf("Cannot restore file\n");
        return(FALSE);
    }

#ifdef SIGTSTP
    signal(SIGTSTP, tstp);
#endif

    environ = envp;
#if !defined(_XOPEN_CURSES) && !defined(__NCURSES_H)
    if (!My_term && isatty(2))
    {
	register char	*sp;

	_tty_ch = 2;
	gettmode();
	if ((sp = getenv("TERM")) == NULL)
	    sp = Def_term;
	setterm(sp);
    }
    else
	setterm(Def_term);
#endif
    strcpy(file_name, file);
    setup();
    clearok(curscr, TRUE);
    srand(getpid());
    msg("file name: %s", file);
    playit();
    /*NOTREACHED*/
}

/*
 * encwrite:
 *	Perform an encrypted write
 */
void
encwrite(start, size, outf)
register char *start;
unsigned int size;
register FILE *outf;
{
    register char *ep;

    ep = encstr;

    while (size--)
    {
	putc(*start++ ^ *ep++, outf);
	if (*ep == '\0')
	    ep = encstr;
    }
}

/*
 * encread:
 *	Perform an encrypted read
 */
int
encread(start, size, inf)
register char *start;
unsigned int size;
register int inf;
{
    register char *ep;
    register int read_size;

    if ((read_size = read(inf, start, size)) == -1 || read_size == 0)
	return read_size;

    ep = encstr;

    while (size--)
    {
	*start++ ^= *ep++;
	if (*ep == '\0')
	    ep = encstr;
    }
    return read_size;
}
