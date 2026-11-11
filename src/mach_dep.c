/*
 * Various installation dependent routines
 *
 * @(#)mach_dep.c	4.23 (Berkeley) 5/19/82
 */

/*
 * The various tuneable defines are:
 *
 *	SCOREFILE	Where/if the score file should live.
 *	MAXLOAD		What (if any) the maximum load average should be
 *			when people are playing.
 *	MAXUSERS	What (if any) the maximum user count should be
 *			when people are playing.  If defined, then
 *		UCOUNT		Should rogue define it's own routine to
 *				count users?
 *		UTMP		If so, where does the user list hide?
 *	CHECKTIME	How often/if rogue should check during the game
 *			for high load average.
 */

#include <curses.h>
#include "extern.h"
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef SCOREFILE
static char *lockfile = "/tmp/.roguelock";
#endif

#ifdef CHECKTIME
static int num_checks;		/* times we've gone over in checkout() */
#endif

/*
 * init_check:
 *	Check out too see if it is proper to play the game now
 */
char *mytty;
int maxusers;

init_check()
{
    if (!isatty(0))
    {
	printf("Must play from a tty.\n");
	exit(1);
    }
    mytty = ttyname(0);
#if defined(MAXLOAD) || defined(MAXUSERS)
    maxusers = strlen("012345");
    if (too_much())
    {
	printf("Sorry, %s, but the system is too loaded now.\n", whoami);
	printf("Try again later.  Meanwhile, why not enjoy a%s %s?\n",
	    vowelstr(fruit), fruit);
	if (author())
	    printf("However, since you're a good guy, it's up to you\n");
	else
	    exit(1);
    }
#endif
}

/*
 * open_score:
 *	Open up the score file for future use, and then
 *	setuid(getuid()) in case we are running setuid.
 */
open_score()
{
#ifdef SCOREFILE
    fd = open(SCOREFILE, 2);
#else
    fd = -1;
#endif
    setuid(getuid());
    setgid(getgid());
}

/*
 * setup:
 *	Get starting setup for all games
 */
setup()
{
    int  auto_save(), quit(), endit(), tstp();
#ifdef CHECKTIME
    int  checkout();
#endif

    /*
     * make sure that large terminals don't overflow the bounds
     * of the program
     */
    if (LINES > MAXLINES)
	LINES = MAXLINES;
    if (COLS > MAXCOLS)
	COLS = MAXCOLS;

    signal(SIGHUP, auto_save);
#ifndef DUMP
    signal(SIGILL, auto_save);
    signal(SIGTRAP, auto_save);
    signal(SIGIOT, auto_save);
#ifdef SIGEMT
    signal(SIGEMT, auto_save);
#endif
    signal(SIGFPE, auto_save);
    signal(SIGBUS, auto_save);
    signal(SIGSEGV, auto_save);
    signal(SIGSYS, auto_save);
    signal(SIGTERM, auto_save);
#endif

    signal(SIGINT, quit);
#ifndef DUMP
    signal(SIGQUIT, endit);
#endif
#ifdef CHECKTIME
    signal(SIGALRM, checkout);
    alarm(CHECKTIME * 60);
    num_checks = 0;
#endif
    crmode();				/* Cbreak mode */
    noecho();				/* Echo off */
}

/*
 * start_score:
 *	Start the scoring sequence
 */
start_score()
{
#ifdef CHECKTIME
    signal(SIGALRM, SIG_IGN);
#endif
}

/*
 * is_symlink:
 *	See if the file has a symbolic link
 */
is_symlink(sp)
char *sp;
{
#ifdef S_IFLNK
    struct stat sbuf2;

    if (lstat(sp, &sbuf2) < 0)
	return FALSE;
    else
	return ((sbuf2.st_mode & S_IFMT) != S_IFREG);
#else
    return FALSE;
#endif
}

#if defined(MAXLOAD) || defined(MAXUSERS)
/*
 * too_much:
 *	See if the system is being used too much for this game
 */
too_much()
{
#ifdef MAXLOAD
    double avec[3];
#else
    register int cnt;
#endif

#ifdef MAXLOAD
    loadav(avec);
    return (avec[1] > (MAXLOAD / 10.0));
#else
    if (mytty != NULL && mytty[8] == 'd' && ucount() >= maxusers)
	return TRUE;
    return FALSE;
#endif
}

/*
 * author:
 *	See if a user is an author of the program
 */
author()
{
#ifdef WIZARD
    if (wizard)
	return TRUE;
#endif
    switch (getuid())
    {
	case 80:
	    return TRUE;
	default:
	    return FALSE;
    }
}
#endif

#ifdef CHECKTIME
/*
 * checkout:
 *	Check each CHECKTIME seconds to see if the load is too high
 */
checkout()
{
    static char *msgs[] = {
	"The load is too high to be playing.  Please leave in %0.1f minutes",
	"Please save your game.  You have %0.1f minutes",
	"Last warning.  You have %0.1f minutes to leave",
    };
    int checktime;

    signal(SIGALRM, checkout);
    if (too_much())
    {
	if (author())
	{
	    num_checks = 1;
	    chmsg("The load is rather high, O exaulted one");
	}
	else if (num_checks++ == 3)
	    fatal("Sorry.  You took too long.  You are dead\n");
	checktime = (CHECKTIME * 60) / num_checks;
	alarm(checktime);
	chmsg(msgs[num_checks - 1], ((double) checktime / 60.0));
    }
    else
    {
	if (num_checks)
	{
	    num_checks = 0;
	    chmsg("The load has dropped back down.  You have a reprieve");
	}
	alarm(CHECKTIME * 60);
    }
}

/*
 * chmsg:
 *	checkout()'s version of msg.  If we are in the middle of a
 *	shell, do a printf instead of a msg to avoid the refresh.
 */
chmsg(fmt, arg)
char *fmt;
double arg;
{
    if (in_shell)
    {
	printf(fmt, arg);
	putchar('\n');
	fflush(stdout);
    }
    else
	msg(fmt, arg);
}
#endif

#ifdef MAXLOAD
/*
 * loadav:
 *	Get the load average from the system, which calls it
 *	getloadavg() now
 */
loadav(avg)
register double *avg;
{
    if (getloadavg(avg, 3) < 3)
	avg[0] = avg[1] = avg[2] = 0.0;
}
#endif

#ifdef UCOUNT
/*
 * ucount:
 *	Count number of users on the system
 */
#include <utmp.h>

struct utmp buf;

ucount()
{
    register struct utmp *up;
    register FILE *utmp;
    register int count;
    char c;

    up = &buf;
    count = 0;

    if ((utmp = fopen(UTMP, "r")) == NULL)
	return 0;

    while (fread(up, 1, sizeof (*up), utmp) > 0)
	if (buf.ut_name[0] != '\0' && buf.ut_line[3] == 'd'
	    && index("012345", (c = buf.ut_line[4])))
	    count++;
    fclose(utmp);
    return count;
}
#endif

/*
 * lock_sc:
 *	lock the score file.  If it takes too long, ask the user if
 *	they care to wait.  Return TRUE if the lock is successful.
 */
lock_sc()
{
#ifdef SCOREFILE
    register int cnt;
    static struct stat sbuf;
    time_t time();

over:
    if (creat(lockfile, 0000) > 0)
	return TRUE;
    for (cnt = 0; cnt < 5; cnt++)
    {
	sleep(1);
	if (creat(lockfile, 0000) > 0)
	    return TRUE;
    }
    if (stat(lockfile, &sbuf) < 0)
    {
	creat(lockfile, 0000);
	return TRUE;
    }
    if (time(NULL) - sbuf.st_mtime > 10)
    {
	if (unlink(lockfile) < 0)
	    return FALSE;
	goto over;
    }
    else
    {
	printf("The score file is very busy.  Do you want to wait longer\n");
	printf("for it to become free so your score can get posted?\n");
	printf("If so, type \"y\"\n");
	fgets(prbuf, MAXSTR, stdin);
	if (prbuf[0] == 'y')
	    for (;;)
	    {
		if (creat(lockfile, 0000) > 0)
		    return TRUE;
		if (stat(lockfile, &sbuf) < 0)
		{
		    creat(lockfile, 0000);
		    return TRUE;
		}
		if (time(NULL) - sbuf.st_mtime > 10)
		{
		    if (unlink(lockfile) < 0)
			return FALSE;
		}
		sleep(1);
	    }
	else
	    return FALSE;
    }
#endif
}

/*
 * unlock_sc:
 *	Unlock the score file
 */
unlock_sc()
{
#ifdef SCOREFILE
    unlink(lockfile);
#endif
}

/*
 * flush_type:
 *	Flush typeahead for traps, etc.
 */
flush_type()
{
    register int flag;

    flag = _tty.sg_flags;
    _tty.sg_flags |= RAW;
    stty(_tty_ch, &_tty);
    _tty.sg_flags = flag;
    stty(_tty_ch, &_tty);
}
