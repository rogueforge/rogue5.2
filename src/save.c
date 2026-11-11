/*
 * save and restore routines
 *
 * @(#)save.c	4.15 (Berkeley) 5/10/82
 */

#include <curses.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include "rogue.h"

typedef struct stat STAT;

extern char version[], *strerror();
extern const char encstr[];
#include <errno.h>

extern char __executable_start[], _end[];
extern char rg_bss[], *const rg_edata, *const rg_ebss, *const rg_estr, *release;
extern int playit(), rg_restored();
extern char *mytty;

long save_stamp[3];			/* tell one build from another */
chtype save_scr[MAXLINES][MAXCOLS];	/* what was on the screen */
int save_lines, save_cols;		/* and how big it was */

#ifdef __SANITIZE_ADDRESS__
#define NOCHECK	__attribute__((no_sanitize_address))
#define read	rg_read
#else
#define NOCHECK
#endif

STAT sbuf;

/*
 * save_ok:
 *	A saved game is the game's variables from version to rg_edata
 *	and from rg_bss to rg_ebss.  See that they are where that takes
 *	them to be, by one variable of each kind.
 */
save_ok()
{
    return ((char *) &level >= version && (char *) &level < rg_edata
	&& (char *) &save_lines >= rg_bss && (char *) &save_lines < rg_ebss);
}

/*
 * save_game:
 *	Implement the "save game" command
 */
save_game()
{
    register FILE *savef;
    register int c;
    char buf[MAXSTR];

    if (!save_ok())
    {
	msg("this rogue cannot save a game");
	return FALSE;
    }
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
	    c = readchar();
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
auto_save()
{
    register FILE *savef;
    register int i;

    for (i = 0; i < NSIG; i++)
	signal(i, SIG_IGN);
    if (!save_ok())
    {
	endwin();
	exit(1);
    }
    if (file_name[0] != '\0' && (savef = fopen(file_name, "w")) != NULL)
	save_file(savef);
    endwin();
    exit(1);
}

/*
 * save_file:
 *	Write the saved game on the file
 */
save_file(savef)
register FILE *savef;
{
    register int y, x;

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
    save_lines = LINES;
    save_cols = COLS;
    for (y = 0; y < LINES && y < MAXLINES; y++)
	for (x = 0; x < COLS && x < MAXCOLS; x++)
	    save_scr[y][x] = mvinch(y, x);
    save_stamp[0] = (long) playit - (long) version;
    save_stamp[1] = (long) rg_restored - (long) __executable_start;
    save_stamp[2] = (long) rg_estr - (long) release;
    encwrite(version, rg_edata - version, savef);
    encwrite(rg_bss, rg_ebss - rg_bss, savef);
    fclose(savef);
}

/*
 * use_up:
 *	Empty a saved game and remove it, so that it cannot be restored
 *	a second time under another name.  Return TRUE if it is gone.
 */
use_up(file, inf)
char *file;
int inf;
{
    register int outf;
    register char *sp;
    STAT sb1, sb2;
    char dir[MAXPATH];

    strncpy(dir, file, MAXPATH - 1);
    dir[MAXPATH - 1] = '\0';
    if ((sp = strrchr(dir, '/')) == NULL)
	strcpy(dir, ".");
    else if (sp == dir)
	dir[1] = '\0';
    else
	*sp = '\0';
    if (access(dir, W_OK) < 0 || fstat(inf, &sb1) < 0 || fchmod(inf, 0600) < 0)
	return FALSE;
    if ((outf = open(file, O_WRONLY|O_NONBLOCK)) < 0 || fstat(outf, &sb2) < 0
	|| sb2.st_dev != sb1.st_dev || sb2.st_ino != sb1.st_ino
	|| unlink(file) < 0)
    {
	if (outf >= 0)
	    close(outf);
	fchmod(inf, sb1.st_mode & 07777);
	return FALSE;
    }
    ftruncate(outf, 0L);
    close(outf);
    return TRUE;
}

/*
 * restore:
 *	Restore a saved game from a file with elaborate checks for file
 *	integrity from cheaters
 */
restore(file, envp)
register char *file;
char **envp;
{
    register int inf;
    register int (*savesig)();
    register bool syml;
    extern char **environ;
    char buf[MAXSTR], name[MAXSTR];
    STAT sbuf2;
    int ofd, owiz, y, x;
    sigset_t set, oset;
    long sstamp;
    char *otty;

    if (strcmp(file, "-r") == 0)
	file = strcpy(name, file_name);

#ifdef SIGTSTP
    /*
     * If a process can be suspended, this code wouldn't work
     */
    savesig = signal(SIGTSTP, SIG_IGN);
#endif

    if ((inf = open(file, 0)) < 0)
    {
	perror(file);
	return FALSE;
    }
    fstat(inf, &sbuf2);
    syml = is_symlink(file);

    fflush(stdout);
    encread(buf, strlen(version) + 1, inf);
    if (strcmp(buf, version) != 0)
    {
	printf("Sorry, saved game is out of date.\n");
	return FALSE;
    }

    if (sbuf2.st_size != (rg_edata - version) + (rg_ebss - rg_bss))
    {
	printf("Sorry, saved game is out of date.\n");
	return FALSE;
    }
    fflush(stdout);
    lseek(inf, 0L, 0);
    ofd = fd;
#ifdef WIZARD
    owiz = wizard;
#endif
    otty = mytty;
    sstamp = (long) rg_estr - (long) release;
    if (encread(version, (unsigned int) (rg_edata - version), inf)
	    != rg_edata - version
	|| encread(rg_bss, (unsigned int) (rg_ebss - rg_bss), inf)
	    != rg_ebss - rg_bss)
    {
	printf("Sorry, saved game is damaged.\n");
	return FALSE;
    }
    if (save_stamp[0] != (long) playit - (long) version
	|| save_stamp[1] != (long) rg_restored - (long) __executable_start
	|| save_stamp[2] != sstamp)
    {
	printf("Sorry, saved game is out of date.\n");
	return FALSE;
    }
    rg_rebase(__executable_start, _end, version, rg_edata, rg_bss, rg_ebss);
    if (!rg_restored())
    {
	printf("Sorry, saved game is damaged.\n");
	return FALSE;
    }
    fd = ofd;
    mytty = otty;
    /*
     * we do not close the file so that we will have a hold of the
     * inode for as long as possible
     */

#ifdef WIZARD
    if (!wizard)
#endif
	if (sbuf2.st_ino != sbuf.st_ino)
	{
	    printf("Sorry, saved game is not in the same file.\n");
	    return FALSE;
	}
	else if (sbuf2.st_ctime - sbuf.st_ctime > 15)
	{
	    printf("Sorry, file has been touched, so this score won't be recorded\n");
	    noscore = TRUE;
	}
#ifdef SIGTSTP
    signal(SIGTSTP, savesig);
#endif
    initscr();
    getmaxyx(stdscr, y, x);
    if (y < save_lines || x < save_cols)
    {
	endwin();
	printf("Sorry, this saved game needs a terminal with %d lines and %d columns.\n", save_lines, save_cols);
	return FALSE;
    }
    LINES = save_lines;
    COLS = save_cols;
    hw = newwin(LINES, COLS, 0, 0);
    kw = NULL;
    for (y = 0; y < LINES && y < MAXLINES; y++)
	for (x = 0; x < COLS && x < MAXCOLS; x++)
	    mvaddch(y, x, save_scr[y][x]);
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
	    endwin();
	    printf("Cannot restore from a linked file\n");
	    return FALSE;
	}
    sigemptyset(&set);
    sigaddset(&set, SIGHUP);
    sigaddset(&set, SIGTERM);
    sigaddset(&set, SIGINT);
    sigprocmask(SIG_BLOCK, &set, &oset);
    if (
#ifdef WIZARD
	!owiz &&
#endif
	!use_up(file, inf))
    {
	endwin();
	printf("Cannot unlink file\n");
	return FALSE;
    }

    environ = envp;
    strcpy(file_name, file);
    setup();
#ifdef CHECKTIME
    sigdelset(&oset, SIGALRM);
#endif
    sigprocmask(SIG_SETMASK, &oset, (sigset_t *) NULL);
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
NOCHECK
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
NOCHECK
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
