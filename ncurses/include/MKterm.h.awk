# vile:awkmode
function declare_termtype(number,suffix) {
	printf "typedef struct termtype%s {	/* in-core form of terminfo data */\n", suffix;
	print  "    char  *term_names;		/* str_table offset of term names */"
	print  "    char  *str_table;		/* pointer to string table */"
	print  "    NCURSES_SBOOL  *Booleans;	/* array of boolean values */"
	printf "    %-5s *Numbers;		/* array of integer values */\n", number;
	print  "    char  **Strings;		/* array of string offsets */"
	print  ""
	print  "#if NCURSES_XNAMES"
	print  "    char  *ext_str_table;	/* pointer to extended string table */"
	print  "    char  **ext_Names;		/* corresponding names */"
	print  ""
	print  "    unsigned short num_Booleans;/* count total Booleans */";
	print  "    unsigned short num_Numbers;	/* count total Numbers */";
	print  "    unsigned short num_Strings;	/* count total Strings */";
	print  ""
	print  "    unsigned short ext_Booleans;/* count extensions to Booleans */";
	print  "    unsigned short ext_Numbers;	/* count extensions to Numbers */";
	print  "    unsigned short ext_Strings;	/* count extensions to Strings */";
	print  "#endif /* NCURSES_XNAMES */"
	print  ""
	printf "} TERMTYPE%s;\n", suffix;
}
BEGIN {
	lcurl = "{";
	rcurl = "}";
	print  "/****************************************************************************"
	print  " * Copyright 2018-2020,2021 Thomas E. Dickey                                *"
	print  " * Copyright 1998-2013,2017 Free Software Foundation, Inc.                  *"
	print  " *                                                                          *"
	print  " * Permission is hereby granted, free of charge, to any person obtaining a  *"
	print  " * copy of this software and associated documentation files (the            *"
	print  " * \"Software\"), to deal in the Software without restriction, including      *"
	print  " * without limitation the rights to use, copy, modify, merge, publish,      *"
	print  " * distribute, distribute with modifications, sublicense, and/or sell       *"
	print  " * copies of the Software, and to permit persons to whom the Software is    *"
	print  " * furnished to do so, subject to the following conditions:                 *"
	print  " *                                                                          *"
	print  " * The above copyright notice and this permission notice shall be included  *"
	print  " * in all copies or substantial portions of the Software.                   *"
	print  " *                                                                          *"
	print  " * THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS  *"
	print  " * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF               *"
	print  " * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.   *"
	print  " * IN NO EVENT SHALL THE ABOVE COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,   *"
	print  " * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR    *"
	print  " * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR    *"
	print  " * THE USE OR OTHER DEALINGS IN THE SOFTWARE.                               *"
	print  " *                                                                          *"
	print  " * Except as contained in this notice, the name(s) of the above copyright   *"
	print  " * holders shall not be used in advertising or otherwise to promote the     *"
	print  " * sale, use or other dealings in this Software without prior written       *"
	print  " * authorization.                                                           *"
	print  " ****************************************************************************/"
	print  ""
	print  "/****************************************************************************/"
	print  "/* Author: Zeyd M. Ben-Halim <zmbenhal@netcom.com> 1992,1995                */"
	print  "/*    and: Eric S. Raymond <esr@snark.thyrsus.com>                          */"
	print  "/*    and: Thomas E. Dickey                        1995-on                  */"
	print  "/****************************************************************************/"
	print  ""
	print  "/* $Id: MKterm.h.awk.in,v 1.82 2021/09/24 17:02:46 tom Exp $ */"
	print  ""
	print  "/*"
	print  "**	term.h -- Definition of struct term"
	print  "*/"
	print  ""
	print  "#ifndef NCURSES_TERM_H_incl"
	print  "#define NCURSES_TERM_H_incl 1"
	print  ""
	print  "#undef  NCURSES_VERSION"
	print  "#define NCURSES_VERSION \"6.3\""
	print  ""
	print  "#include <ncurses_dll.h>"
	print  ""
	print  "#ifdef __cplusplus"
	printf "extern \"C\" %s\n", lcurl;
	print  "#endif"
	print  ""
	print  "/* Make this file self-contained by providing defaults for the HAVE_TERMIO[S]_H"
	print  " * definition (based on the system for which this was configured)."
	print  " */"
	print  ""
	print  "#ifndef __NCURSES_H"
	print  ""
	print  "typedef struct screen  SCREEN;"
	print  ""
	print  "#if 1"
	print  "#undef  NCURSES_SP_FUNCS"
	print  "#define NCURSES_SP_FUNCS 20211021"
	print  "#undef  NCURSES_SP_NAME"
	print  "#define NCURSES_SP_NAME(name) name##_sp"
	print  ""
	print  "/* Define the sp-funcs helper function */"
	print  "#undef  NCURSES_SP_OUTC"
	print  "#define NCURSES_SP_OUTC NCURSES_SP_NAME(NCURSES_OUTC)"
	print  "typedef int (*NCURSES_SP_OUTC)(SCREEN*, int);"
	print  "#endif"
	print  ""
	print  "#endif /* __NCURSES_H */"
	print  ""
	print  "#undef  NCURSES_CONST"
	print  "#define NCURSES_CONST const"
	print  ""
	print  "#undef  NCURSES_SBOOL"
	print  "#define NCURSES_SBOOL char"
	print  ""
	print  "#undef  NCURSES_USE_DATABASE"
	print  "#define NCURSES_USE_DATABASE 1"
	print  ""
	print  "#undef  NCURSES_USE_TERMCAP"
	print  "#define NCURSES_USE_TERMCAP 0"
	print  ""
	print  "#undef  NCURSES_XNAMES"
	print  "#define NCURSES_XNAMES 1"
	print  ""
	print  "/* We will use these symbols to hide differences between"
	print  " * termios/termio/sgttyb interfaces."
	print  " */"
	print  "#undef  TTY"
	print  "#undef  SET_TTY"
	print  "#undef  GET_TTY"
	print  ""
	print  "/* Assume POSIX termio if we have the header and function */"
	print  "/* #if HAVE_TERMIOS_H && HAVE_TCGETATTR */"
	print  "#if 1 && 1"
	print  ""
	print  "#undef  TERMIOS"
	print  "#define TERMIOS 1"
	print  ""
	print  "#include <termios.h>"
	print  "#define TTY struct termios"
	print  ""
	print  "#else /* !HAVE_TERMIOS_H */"
	print  ""
	print  "/* #if HAVE_TERMIO_H */"
	print  "#if 0"
	print  ""
	print  "#undef  TERMIOS"
	print  "#define TERMIOS 1"
	print  ""
	print  "#include <termio.h>"
	print  "#define TTY struct termio"
	print  ""
	print  "#else /* !HAVE_TERMIO_H */"
	print  ""
	print  "#if (defined(_WIN32) || defined(_WIN64))"
	print  "#if 0"
	print  "#include <win32_curses.h>"
	print  "#define TTY struct winconmode"
	print  "#else"
	print  "#include <ncurses_mingw.h>"
	print  "#define TTY struct termios"
	print  "#endif"
	print  "#else"
	print  "#undef TERMIOS"
	print  "#include <sgtty.h>"
	print  "#include <sys/ioctl.h>"
	print  "#define TTY struct sgttyb"
	print  "#endif /* MINGW32 */"
	print  "#endif /* HAVE_TERMIO_H */"
	print  ""
	print  "#endif /* HAVE_TERMIOS_H */"
	print  ""
	print  "#ifdef TERMIOS"
	print  "#define GET_TTY(fd, buf) tcgetattr(fd, buf)"
	print  "#define SET_TTY(fd, buf) tcsetattr(fd, TCSADRAIN, buf)"
	print  "#elif 0 && (defined(_WIN32) || defined(_WIN64))"
	print  "#define GET_TTY(fd, buf) _nc_console_getmode(_nc_console_fd2handle(fd),buf)"
	print  "#define SET_TTY(fd, buf) _nc_console_setmode(_nc_console_fd2handle(fd),buf)"
	print  "#else"
	print  "#define GET_TTY(fd, buf) gtty(fd, buf)"
	print  "#define SET_TTY(fd, buf) stty(fd, buf)"
	print  "#endif"
	print  ""
	print  "#ifndef	GCC_NORETURN"
	print  "#define	GCC_NORETURN /* nothing */"
	print  "#endif"
	print  ""
	print  "#define NAMESIZE 256"
	print  ""
	print  "/* The cast works because TERMTYPE is the first data in TERMINAL */"
	print  "#define CUR ((TERMTYPE *)(cur_term))->"
	print  ""
}

$2 == "%%-STOP-HERE-%%" {
	print  ""
	printf "#define BOOLWRITE %d\n", BoolCount
	printf "#define NUMWRITE  %d\n", NumberCount
	printf "#define STRWRITE  %d\n", StringCount
	print  ""
	print  "/* older synonyms for some capabilities */"
	print  "#define beehive_glitch	no_esc_ctlc"
	print  "#define teleray_glitch	dest_tabs_magic_smso"
	print  ""
	print  "/* HPUX-11 uses this name rather than the standard one */"
	print  "#ifndef micro_char_size"
	print  "#define micro_char_size micro_col_size"
	print  "#endif"
	print  ""
	print  "#ifdef __INTERNAL_CAPS_VISIBLE"
}

/^#/ { next; }

/^used_by/ { next ; }
/^userdef/ { next ; }

$1 == "acs_chars" {
	acsindex = StringCount;
}

$3 == "bool" {
	printf "#define %-30s CUR Booleans[%d]\n", $1, BoolCount++
}

$3 == "num" {
	printf "#define %-30s CUR Numbers[%d]\n", $1, NumberCount++
}

$3 == "str" {
	printf "#define %-30s CUR Strings[%d]\n", $1, StringCount++
}

END {
	print  "#endif /* __INTERNAL_CAPS_VISIBLE */"
	print  ""
	print  ""
	print  "/*"
	print  " * Predefined terminfo array sizes"
	print  " */"
	printf "#define BOOLCOUNT %d\n", BoolCount
	printf "#define NUMCOUNT  %d\n", NumberCount
	printf "#define STRCOUNT  %d\n", StringCount
	print  ""
	print  "/* used by code for comparing entries */"
	print  "#define acs_chars_index	", acsindex
	print  ""
	declare_termtype("short","");
	print  ""
	print  "/*"
	print  " * The only reason these structures are visible is for read-only use."
	print  " * Programs which modify the data are not, never were, portable across"
	print  " * curses implementations."
	print  " *"
	print  " * The first field in TERMINAL is used in macros."
	print  " * The remaining fields are private."
	print  " */"
	print  "#ifdef NCURSES_INTERNALS"
	print  ""
	print  "#undef TERMINAL"
	print  "#define TERMINAL struct term"
	print  "TERMINAL;"
	print  ""
	if (0) {
	declare_termtype("int","2");
	} else {
	print  "#undef TERMTYPE2"
	print  "#define TERMTYPE2 TERMTYPE"
	}
	print  "#else"
	print  ""
	print  "typedef struct term {		/* describe an actual terminal */"
	print  "    TERMTYPE	type;		/* terminal type description */"
	print  "} TERMINAL;"
	print  ""
	print  "#endif /* NCURSES_INTERNALS */"
	print  ""
	print  ""
	print  "#if 0 && !0"
	print  "extern NCURSES_EXPORT_VAR(TERMINAL *) cur_term;"
	print  "#elif 0"
	print  "NCURSES_WRAPPED_VAR(TERMINAL *, cur_term);"
	print  "#define cur_term   NCURSES_PUBLIC_VAR(cur_term())"
	print  "#else"
	print  "extern NCURSES_EXPORT_VAR(TERMINAL *) cur_term;"
	print  "#endif"
	print  ""
	print  "#if 0 || 0"
	print  "NCURSES_WRAPPED_VAR(NCURSES_CONST char * const *, boolnames);"
	print  "NCURSES_WRAPPED_VAR(NCURSES_CONST char * const *, boolcodes);"
	print  "NCURSES_WRAPPED_VAR(NCURSES_CONST char * const *, boolfnames);"
	print  "NCURSES_WRAPPED_VAR(NCURSES_CONST char * const *, numnames);"
	print  "NCURSES_WRAPPED_VAR(NCURSES_CONST char * const *, numcodes);"
	print  "NCURSES_WRAPPED_VAR(NCURSES_CONST char * const *, numfnames);"
	print  "NCURSES_WRAPPED_VAR(NCURSES_CONST char * const *, strnames);"
	print  "NCURSES_WRAPPED_VAR(NCURSES_CONST char * const *, strcodes);"
	print  "NCURSES_WRAPPED_VAR(NCURSES_CONST char * const *, strfnames);"
	print  ""
	print  "#define boolnames  NCURSES_PUBLIC_VAR(boolnames())"
	print  "#define boolcodes  NCURSES_PUBLIC_VAR(boolcodes())"
	print  "#define boolfnames NCURSES_PUBLIC_VAR(boolfnames())"
	print  "#define numnames   NCURSES_PUBLIC_VAR(numnames())"
	print  "#define numcodes   NCURSES_PUBLIC_VAR(numcodes())"
	print  "#define numfnames  NCURSES_PUBLIC_VAR(numfnames())"
	print  "#define strnames   NCURSES_PUBLIC_VAR(strnames())"
	print  "#define strcodes   NCURSES_PUBLIC_VAR(strcodes())"
	print  "#define strfnames  NCURSES_PUBLIC_VAR(strfnames())"
	print  ""
	print  "#else"
	print  ""
	print  "extern NCURSES_EXPORT_VAR(NCURSES_CONST char * const ) boolnames[];"
	print  "extern NCURSES_EXPORT_VAR(NCURSES_CONST char * const ) boolcodes[];"
	print  "extern NCURSES_EXPORT_VAR(NCURSES_CONST char * const ) boolfnames[];"
	print  "extern NCURSES_EXPORT_VAR(NCURSES_CONST char * const ) numnames[];"
	print  "extern NCURSES_EXPORT_VAR(NCURSES_CONST char * const ) numcodes[];"
	print  "extern NCURSES_EXPORT_VAR(NCURSES_CONST char * const ) numfnames[];"
	print  "extern NCURSES_EXPORT_VAR(NCURSES_CONST char * const ) strnames[];"
	print  "extern NCURSES_EXPORT_VAR(NCURSES_CONST char * const ) strcodes[];"
	print  "extern NCURSES_EXPORT_VAR(NCURSES_CONST char * const ) strfnames[];"
	print  ""
	print  "#endif"
	print  ""
	print  "/*"
	print  " * These entrypoints are used only by the ncurses utilities such as tic."
	print  " */"
	print  "#ifdef NCURSES_INTERNALS"
	print  ""
	print  "extern NCURSES_EXPORT(int) _nc_set_tty_mode (TTY *buf);"
	print  "extern NCURSES_EXPORT(int) _nc_read_entry2 (const char * const, char * const, TERMTYPE2 *const);"
	print  "extern NCURSES_EXPORT(int) _nc_read_file_entry (const char *const, TERMTYPE2 *);"
	print  "extern NCURSES_EXPORT(int) _nc_read_termtype (TERMTYPE2 *, char *, int);"
	print  "extern NCURSES_EXPORT(char *) _nc_first_name (const char *const);"
	print  "extern NCURSES_EXPORT(int) _nc_name_match (const char *const, const char *const, const char *const);"
	print  "extern NCURSES_EXPORT(char *) _nc_tiparm(int, const char *, ...);"
	print  ""
	print  "#endif /* NCURSES_INTERNALS */"
	print  ""
	print  ""
	print  "/*"
	print  " * These entrypoints are used by tack 1.07."
	print  " */"
	print  "extern NCURSES_EXPORT(const TERMTYPE *) _nc_fallback (const char *);"
	print  "extern NCURSES_EXPORT(int) _nc_read_entry (const char * const, char * const, TERMTYPE *const);"
	print  ""
	print  "/*"
	print  " * Normal entry points"
	print  " */"
	print  "extern NCURSES_EXPORT(TERMINAL *) set_curterm (TERMINAL *);"
	print  "extern NCURSES_EXPORT(int) del_curterm (TERMINAL *);"
	print  ""
	print  "/* miscellaneous entry points */"
	print  "extern NCURSES_EXPORT(int) restartterm (NCURSES_CONST char *, int, int *);"
	print  "extern NCURSES_EXPORT(int) setupterm (const char *,int,int *);"
	print  ""
	print  "/* terminfo entry points, also declared in curses.h */"
	print  "#if !defined(__NCURSES_H)"
	print  "extern NCURSES_EXPORT(char *) tigetstr (const char *);"
	print  "extern NCURSES_EXPORT_VAR(char) ttytype[];"
	print  "extern NCURSES_EXPORT(int) putp (const char *);"
	print  "extern NCURSES_EXPORT(int) tigetflag (const char *);"
	print  "extern NCURSES_EXPORT(int) tigetnum (const char *);"
	print  ""
	print  "#if 1 /* NCURSES_TPARM_VARARGS */"
	print  "extern NCURSES_EXPORT(char *) tparm (const char *, ...);	/* special */"
	print  "#else"
	print  "extern NCURSES_EXPORT(char *) tparm (const char *, long,long,long,long,long,long,long,long,long);	/* special */"
	print  "#endif"
	print  ""
	print  "extern NCURSES_EXPORT(char *) tiparm (const char *, ...);		/* special */"
	print  ""
	print  "#endif /* __NCURSES_H */"
	print  ""
	print  "/* termcap database emulation (XPG4 uses const only for 2nd param of tgetent) */"
	print  "#if !defined(NCURSES_TERMCAP_H_incl)"
	print  "extern NCURSES_EXPORT(char *) tgetstr (const char *, char **);"
	print  "extern NCURSES_EXPORT(char *) tgoto (const char *, int, int);"
	print  "extern NCURSES_EXPORT(int) tgetent (char *, const char *);"
	print  "extern NCURSES_EXPORT(int) tgetflag (const char *);"
	print  "extern NCURSES_EXPORT(int) tgetnum (const char *);"
	print  "extern NCURSES_EXPORT(int) tputs (const char *, int, int (*)(int));"
	print  "#endif /* NCURSES_TERMCAP_H_incl */"
	print  ""
	print  "/*"
	print  " * Include curses.h before term.h to enable these extensions."
	print  " */"
	print  "#if defined(NCURSES_SP_FUNCS) && (NCURSES_SP_FUNCS != 0)"
	print  ""
	print  "extern NCURSES_EXPORT(char *)  NCURSES_SP_NAME(tigetstr) (SCREEN*, const char *);"
	print  "extern NCURSES_EXPORT(int)     NCURSES_SP_NAME(putp) (SCREEN*, const char *);"
	print  "extern NCURSES_EXPORT(int)     NCURSES_SP_NAME(tigetflag) (SCREEN*, const char *);"
	print  "extern NCURSES_EXPORT(int)     NCURSES_SP_NAME(tigetnum) (SCREEN*, const char *);"
	print  ""
	print  "#if 1 /* NCURSES_TPARM_VARARGS */"
	print  "extern NCURSES_EXPORT(char *)  NCURSES_SP_NAME(tparm) (SCREEN*, const char *, ...);	/* special */"
	print  "#else"
	print  "extern NCURSES_EXPORT(char *)  NCURSES_SP_NAME(tparm) (SCREEN*, const char *, long,long,long,long,long,long,long,long,long);	/* special */"
	print  "#endif"
	print  ""
	print  "/* termcap database emulation (XPG4 uses const only for 2nd param of tgetent) */"
	print  "extern NCURSES_EXPORT(char *)  NCURSES_SP_NAME(tgetstr) (SCREEN*, const char *, char **);"
	print  "extern NCURSES_EXPORT(char *)  NCURSES_SP_NAME(tgoto) (SCREEN*, const char *, int, int);"
	print  "extern NCURSES_EXPORT(int)     NCURSES_SP_NAME(tgetent) (SCREEN*, char *, const char *);"
	print  "extern NCURSES_EXPORT(int)     NCURSES_SP_NAME(tgetflag) (SCREEN*, const char *);"
	print  "extern NCURSES_EXPORT(int)     NCURSES_SP_NAME(tgetnum) (SCREEN*, const char *);"
	print  "extern NCURSES_EXPORT(int)     NCURSES_SP_NAME(tputs) (SCREEN*, const char *, int, NCURSES_SP_OUTC);"
	print  ""
	print  "extern NCURSES_EXPORT(TERMINAL *) NCURSES_SP_NAME(set_curterm) (SCREEN*, TERMINAL *);"
	print  "extern NCURSES_EXPORT(int)     NCURSES_SP_NAME(del_curterm) (SCREEN*, TERMINAL *);"
	print  ""
	print  "extern NCURSES_EXPORT(int)     NCURSES_SP_NAME(restartterm) (SCREEN*, NCURSES_CONST char *, int, int *);"
	print  "#endif /* NCURSES_SP_FUNCS */"
	print  ""
	print  "/*"
	print  " * Debugging features."
	print  " */"
	print  "extern GCC_NORETURN NCURSES_EXPORT(void)    exit_terminfo(int);"
	print  ""
	print  "#ifdef __cplusplus"
	printf "%s\n", rcurl;
	print  "#endif"
	print  ""
	print  "#endif /* NCURSES_TERM_H_incl */"
}
