/*
 * Defines for things used in mach_dep.c
 *
 * @(#)extern.h	4.3 (Berkeley) 4/2/82
 */

/*
 * Don't change the constants, since they are used for sizes in many
 * places in the program.
 */

#include <stdarg.h>
#include <time.h>
#include <unistd.h>
#include <limits.h>

#define MAXSTR		80	/* maximum length of strings */
#define MAXPATH		PATH_MAX	/* maximum length of a file name */
#define MAXLINES	32	/* maximum number of screen lines used */
#define MAXCOLS		80	/* maximum number of screen columns used */
#define LINES		rg_lines	/* the size of the screen, as */
#define COLS		rg_cols		/* the game was started with */

extern int rg_lines, rg_cols;

#define RN		(((seed = seed*11109+13849) >> 16) & 0xffff)

/*
 * Now all the global variables
 */

extern bool	after, amulet, askme, between, door_stop, fight_flush,
		firstmove, in_shell, jump, noscore, p_know[], passgo,
		playing, r_know[], running, s_know[], save_msg,
		slow_invent, terse, weak_save, wizard, ws_know[];

extern char	_flags[], _level[], *a_names[], file_name[], fruit[],
		home[], huh[], outbuf[], *p_colors[], *p_guess[],
		prbuf[], *r_guess[], *r_stones[], *release, runch,
		*s_guess[], *s_names[], take, *w_names[], whoami[],
		*ws_guess[], *ws_made[], *ws_type[];

extern int	a_chances[], a_class[], count, dnum, food_left,
		fung_hit, fd, group, hungry_state, inpack, lastscore,
		level, max_level, mpos, no_command, no_food, no_move,
		ntraps, purse, quiet, total, turn_left;

extern long	seed;

extern WINDOW	*hw, *kw;

/*
 * Function types
 */

char	*charge_str(), *ctime(), *getenv(), *inv_name(),
	*killname(), *malloc(), *nothing(), *num(), *ring_num(),
	*strcat(), *strcpy(), *tr_name(), *vowelstr();

int	msg(char *fmt, ...), addmsg(char *fmt, ...);

int	auto_save(), doctor(), endit(), leave(), nohaste(), quit(),
	rollwand(), runners(), sight(), stomach(), swander(), tstp(),
	turn_see(), unconfuse(), unsee();

#ifdef CHECKTIME
int	checkout();
#endif
