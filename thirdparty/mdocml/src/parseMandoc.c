/*	$Id: main.c,v 1.269 2016/07/12 05:18:38 kristaps Exp $ */
/*
 * Copyright (c) 2008-2012 Kristaps Dzonsons <kristaps@bsd.lv>
 * Copyright (c) 2010-2012, 2014-2016 Ingo Schwarze <schwarze@openbsd.org>
 * Copyright (c) 2010 Joerg Sonnenberger <joerg@netbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHORS DISCLAIM ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include "config.h"

#include <sys/types.h>
#ifndef _WIN32
#include <sys/param.h>	/* MACHINE */
#include <sys/wait.h>
#endif

#include <assert.h>
#include <ctype.h>
#if HAVE_ERR
#include <err.h>
#endif
#include <errno.h>
#include <fcntl.h>
#ifndef _WIN32
#include <glob.h>
#endif
#if HAVE_SANDBOX_INIT
#include <sandbox.h>
#endif
#include <signal.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include "mandoc_aux.h"
#include "mandoc.h"
#include "roff.h"
#include "mdoc.h"
#include "man.h"
//#include "tag.h"
//#include "main.h"
#include "parseMandoc.h"
#include "manconf.h"
#include "mansearch.h"

#if !defined(__GNUC__) || (__GNUC__ < 2)
# if !defined(lint)
#  define __attribute__(x)
# endif
#endif /* !defined(__GNUC__) || (__GNUC__ < 2) */

enum	outmode {
	OUTMODE_DEF = 0,
	OUTMODE_FLN,
	OUTMODE_LST,
	OUTMODE_ALL,
	OUTMODE_INT,
	OUTMODE_ONE
};

enum	outt {
	OUTT_ASCII = 0,	/* -Tascii */
	OUTT_LOCALE,	/* -Tlocale */
	OUTT_UTF8,	/* -Tutf8 */
	OUTT_TREE,	/* -Ttree */
	OUTT_MAN,	/* -Tman */
	OUTT_HTML,	/* -Thtml */
	OUTT_LINT,	/* -Tlint */
	OUTT_PS,	/* -Tps */
	OUTT_PDF	/* -Tpdf */
};

struct	curparse {
	struct mparse	 *mp;
	enum mandoclevel  wlevel;	/* ignore messages below this */
	int		  wstop;	/* stop after a file with a warning */
	enum outt	  outtype;	/* which output to use */
	void		 *outdata;	/* data for output */
	struct manoutput *outopts;	/* output options */
};

static	int		  fs_lookup(const struct manpaths *,
				size_t ipath, const char *,
				const char *, const char *,
				struct manpage **, size_t *);
static	void		  fs_search(const struct mansearch *,
				const struct manpaths *, int, char**,
				struct manpage **, size_t *);
static	int		  koptions(int *, char *);
#if HAVE_SQLITE3
int			  mandocdb(int, char**);
#endif
static	int		  moptions(int *, char *);
static	void		  mmsg(enum mandocerr, enum mandoclevel,
				const char *, int, int, const char *);
static	void		  parse(struct curparse *, int, const char *);
static	void		  passthrough(const char *, int, int);
//static	pid_t		  spawn_pager(struct tag_files *);
static	int		  toptions(struct curparse *, char *);
static	void		  usage(enum argmode) __attribute__((noreturn));
static	int		  woptions(struct curparse *, char *);

static	const int sec_prios[] = {1, 4, 5, 8, 6, 3, 7, 2, 9};
static	char		  help_arg[] = "help";
static	char		 *help_argv[] = {help_arg, NULL};
static	enum mandoclevel  rc;

int
parse_mandoc(int argc, const char *argv[])
{
	struct manconf	 conf;
	struct curparse	 curp;
	struct mansearch search;
	const char	*progname;
	char		*defos;
	unsigned char	*uc;
	struct manpage	*res, *resp;
	char		*conf_file, *defpaths;
	const char	*sec;
	size_t		 i, sz;
	int		 prio, best_prio;
	enum outmode	 outmode;
	int		 fd;
	int		 options;
	int		 status, signum;
	int		 c;

    progname = argv[0];
	setprogname(progname);

	/* Search options. */

	memset(&conf, 0, sizeof(conf));
	conf_file = defpaths = NULL;

	memset(&search, 0, sizeof(struct mansearch));
	search.outkey = "Nd";

    search.argmode = ARG_FILE;

	/* Parser and formatter options. */

	memset(&curp, 0, sizeof(struct curparse));
	curp.outtype = OUTT_LOCALE;
	curp.wlevel  = MANDOCLEVEL_BADARG;
	curp.outopts = &conf.output;
	options = MPARSE_SO | MPARSE_UTF8 | MPARSE_LATIN1;
	defos = NULL;

	outmode = OUTMODE_DEF;

    // input from utf8
    options |=  MPARSE_UTF8;
    options &= ~MPARSE_LATIN1;
    // set output format
#if defined _WIN32
    curp.outtype = OUTT_LOCALE;
#else
    curp.outtype = OUTT_UTF8;
#endif
    
	if (outmode == OUTMODE_DEF) {
		switch (search.argmode) {
		case ARG_FILE:
			outmode = OUTMODE_ALL;
			break;
		case ARG_NAME:
			outmode = OUTMODE_ONE;
			break;
		default:
			outmode = OUTMODE_LST;
			break;
		}
	}

	if (argc > 0) {
		argc -= 5;
		argv += 5;

	}
	resp = NULL;

	/*
	 * Quirks for help(1)
	 * and for a man(1) section argument without -s.
	 */

	rc = MANDOCLEVEL_OK;

	mchars_alloc();
	curp.mp = mparse_alloc(options, curp.wlevel, mmsg, defos);
    mparse_reset(curp.mp);

	while (argc > 0) {
		fd = mparse_open(curp.mp, resp != NULL ? resp->file : *argv);
		if (fd != -1) {
			if (resp == NULL) {
				parse(&curp, fd, *argv);
			}
            // TODO: error may happen
		}
        break ;
	}

	if (curp.outdata != NULL) {
		switch (curp.outtype) {
		case OUTT_HTML:
			html_free(curp.outdata);
			break;
		case OUTT_UTF8:
		case OUTT_LOCALE:
		case OUTT_ASCII:
			ascii_free(curp.outdata);
			break;
		case OUTT_PDF:
		case OUTT_PS:
			pspdf_free(curp.outdata);
			break;
		default:
			break;
		}
	}
	mparse_free(curp.mp);
	mchars_free();

	free(defos);

	return (int)rc;
}

static void
usage(enum argmode argmode)
{

	switch (argmode) {
	case ARG_FILE:
		fputs("usage: mandoc [-acfhkl] [-I os=name] "
		    "[-K encoding] [-mformat] [-O option]\n"
		    "\t      [-T output] [-W level] [file ...]\n", stderr);
		break;
	case ARG_NAME:
		fputs("usage: man [-acfhklw] [-C file] [-I os=name] "
		    "[-K encoding] [-M path] [-m path]\n"
		    "\t   [-O option=value] [-S subsection] [-s section] "
		    "[-T output] [-W level]\n"
		    "\t   [section] name ...\n", stderr);
		break;
	case ARG_WORD:
		fputs("usage: whatis [-acfhklw] [-C file] "
		    "[-M path] [-m path] [-O outkey] [-S arch]\n"
		    "\t      [-s section] name ...\n", stderr);
		break;
	case ARG_EXPR:
		fputs("usage: apropos [-acfhklw] [-C file] "
		    "[-M path] [-m path] [-O outkey] [-S arch]\n"
		    "\t       [-s section] expression ...\n", stderr);
		break;
	}
	exit((int)MANDOCLEVEL_BADARG);
}
/*
static int
fs_lookup(const struct manpaths *paths, size_t ipath,
	const char *sec, const char *arch, const char *name,
	struct manpage **res, size_t *ressz)
{
	glob_t		 globinfo;
	struct manpage	*page;
	char		*file;
	int		 form, globres;

	form = FORM_SRC;
	mandoc_asprintf(&file, "%s/man%s/%s.%s",
	    paths->paths[ipath], sec, name, sec);
	if (access(file, R_OK) != -1)
		goto found;
	free(file);

	mandoc_asprintf(&file, "%s/cat%s/%s.0",
	    paths->paths[ipath], sec, name);
	if (access(file, R_OK) != -1) {
		form = FORM_CAT;
		goto found;
	}
	free(file);

	if (arch != NULL) {
		mandoc_asprintf(&file, "%s/man%s/%s/%s.%s",
		    paths->paths[ipath], sec, arch, name, sec);
		if (access(file, R_OK) != -1)
			goto found;
		free(file);
	}

	mandoc_asprintf(&file, "%s/man%s/%s.[01-9]*",
	    paths->paths[ipath], sec, name);
	globres = glob(file, 0, NULL, &globinfo);
	if (globres != 0 && globres != GLOB_NOMATCH)
		warn("%s: glob", file);
	free(file);
	if (globres == 0)
		file = mandoc_strdup(*globinfo.gl_pathv);
	globfree(&globinfo);
	if (globres != 0)
		return 0;

found:
#if HAVE_SQLITE3
	warnx("outdated mandoc.db lacks %s(%s) entry, run %s %s",
	    name, sec, BINM_MAKEWHATIS, paths->paths[ipath]);
#endif
	*res = mandoc_reallocarray(*res, ++*ressz, sizeof(struct manpage));
	page = *res + (*ressz - 1);
	page->file = file;
	page->names = NULL;
	page->output = NULL;
	page->ipath = ipath;
	page->bits = NAME_FILE & NAME_MASK;
	page->sec = (*sec >= '1' && *sec <= '9') ? *sec - '1' + 1 : 10;
	page->form = form;
	return 1;
}

static void
fs_search(const struct mansearch *cfg, const struct manpaths *paths,
	int argc, char **argv, struct manpage **res, size_t *ressz)
{
	const char *const sections[] =
	    {"1", "8", "6", "2", "3", "5", "7", "4", "9", "3p"};
	const size_t nsec = sizeof(sections)/sizeof(sections[0]);

	size_t		 ipath, isec, lastsz;

	assert(cfg->argmode == ARG_NAME);

	*res = NULL;
	*ressz = lastsz = 0;
	while (argc) {
		for (ipath = 0; ipath < paths->sz; ipath++) {
			if (cfg->sec != NULL) {
				if (fs_lookup(paths, ipath, cfg->sec,
				    cfg->arch, *argv, res, ressz) &&
				    cfg->firstmatch)
					return;
			} else for (isec = 0; isec < nsec; isec++)
				if (fs_lookup(paths, ipath, sections[isec],
				    cfg->arch, *argv, res, ressz) &&
				    cfg->firstmatch)
					return;
		}
		if (*ressz == lastsz)
			warnx("No entry for %s in the manual.", *argv);
		lastsz = *ressz;
		argv++;
		argc--;
	}
}
*/
static void
parse(struct curparse *curp, int fd, const char *file)
{
	enum mandoclevel  rctmp;
	struct roff_man	 *man;

	// Begin by parsing the file itself.

	assert(file);
	assert(fd >= 0);

	rctmp = mparse_readfd(curp->mp, fd, file);
	if (fd != STDIN_FILENO)
		close(fd);
	if (rc < rctmp)
		rc = rctmp;

	//
	// With -Wstop and warnings or errors of at least the requested
	// level, do not produce output.
	//

	if (rctmp != MANDOCLEVEL_OK && curp->wstop)
		return;

	// If unset, allocate output dev now (if applicable).

	if (curp->outdata == NULL) {
		switch (curp->outtype) {
		case OUTT_HTML:
			curp->outdata = html_alloc(curp->outopts);
			break;
		case OUTT_UTF8:
			curp->outdata = utf8_alloc(curp->outopts);
			break;
		case OUTT_LOCALE:
			curp->outdata = locale_alloc(curp->outopts);
			break;
		case OUTT_ASCII:
			curp->outdata = ascii_alloc(curp->outopts);
			break;
		case OUTT_PDF:
			curp->outdata = pdf_alloc(curp->outopts);
			break;
		case OUTT_PS:
			curp->outdata = ps_alloc(curp->outopts);
			break;
		default:
			break;
		}
	}

	mparse_result(curp->mp, &man, NULL);

	// Execute the out device, if it exists.

	if (man == NULL)
		return;
	if (man->macroset == MACROSET_MDOC) {
		mdoc_validate(man);
		switch (curp->outtype) {
		case OUTT_HTML:
			html_mdoc(curp->outdata, man);
			break;
		case OUTT_TREE:
			tree_mdoc(curp->outdata, man);
			break;
		case OUTT_MAN:
			man_mdoc(curp->outdata, man);
			break;
		case OUTT_PDF:
		case OUTT_ASCII:
		case OUTT_UTF8:
		case OUTT_LOCALE:
		case OUTT_PS:
			terminal_mdoc(curp->outdata, man);
			break;
		default:
			break;
		}
	}
	if (man->macroset == MACROSET_MAN) {
		man_validate(man);
		switch (curp->outtype) {
		case OUTT_HTML:
			html_man(curp->outdata, man);
			break;
		case OUTT_TREE:
			tree_man(curp->outdata, man);
			break;
		case OUTT_MAN:
			man_man(curp->outdata, man);
			break;
		case OUTT_PDF:
		case OUTT_ASCII:
		case OUTT_UTF8:
		case OUTT_LOCALE:
		case OUTT_PS:
			terminal_man(curp->outdata, man);
			break;
		default:
			break;
		}
	}
}

static void
passthrough(const char *file, int fd, int synopsis_only)
{
	const char	 synb[] = "S\bSY\bYN\bNO\bOP\bPS\bSI\bIS\bS";
	const char	 synr[] = "SYNOPSIS";

	FILE		*stream;
	const char	*syscall;
	char		*line, *cp;
	size_t		 linesz;
	int		 print;

	line = NULL;
	linesz = 0;

	if ((stream = fdopen(fd, "r")) == NULL) {
		close(fd);
		syscall = "fdopen";
		goto fail;
	}

	print = 0;
	while (getline(&line, &linesz, stream) != -1) {
		cp = line;
		if (synopsis_only) {
			if (print) {
				if ( ! isspace((unsigned char)*cp))
					goto done;
				while (isspace((unsigned char)*cp))
					cp++;
			} else {
				if (strcmp(cp, synb) == 0 ||
				    strcmp(cp, synr) == 0)
					print = 1;
				continue;
			}
		}
		if (fputs(cp, stdout)) {
			fclose(stream);
			syscall = "fputs";
			goto fail;
		}
	}

	if (ferror(stream)) {
		fclose(stream);
		syscall = "getline";
		goto fail;
	}

done:
	free(line);
	fclose(stream);
	return;

fail:
	free(line);
	warn("%s: SYSERR: %s", file, syscall);
	if (rc < MANDOCLEVEL_SYSERR)
		rc = MANDOCLEVEL_SYSERR;
}

static int
koptions(int *options, char *arg)
{

	if ( ! strcmp(arg, "utf-8")) {
		*options |=  MPARSE_UTF8;
		*options &= ~MPARSE_LATIN1;
	} else if ( ! strcmp(arg, "iso-8859-1")) {
		*options |=  MPARSE_LATIN1;
		*options &= ~MPARSE_UTF8;
	} else if ( ! strcmp(arg, "us-ascii")) {
		*options &= ~(MPARSE_UTF8 | MPARSE_LATIN1);
	} else {
		warnx("-K %s: Bad argument", arg);
		return 0;
	}
	return 1;
}

static int
moptions(int *options, char *arg)
{

	if (arg == NULL)
        ;
	else if (0 == strcmp(arg, "doc"))
		*options |= MPARSE_MDOC;
	else if (0 == strcmp(arg, "andoc"))
        ;
	else if (0 == strcmp(arg, "an"))
		*options |= MPARSE_MAN;
	else {
		warnx("-m %s: Bad argument", arg);
		return 0;
	}

	return 1;
}

static int
toptions(struct curparse *curp, char *arg)
{

	if (0 == strcmp(arg, "ascii"))
		curp->outtype = OUTT_ASCII;
	else if (0 == strcmp(arg, "lint")) {
		curp->outtype = OUTT_LINT;
		curp->wlevel  = MANDOCLEVEL_WARNING;
	} else if (0 == strcmp(arg, "tree"))
		curp->outtype = OUTT_TREE;
	else if (0 == strcmp(arg, "man"))
		curp->outtype = OUTT_MAN;
	else if (0 == strcmp(arg, "html"))
		curp->outtype = OUTT_HTML;
	else if (0 == strcmp(arg, "utf8"))
		curp->outtype = OUTT_UTF8;
	else if (0 == strcmp(arg, "locale"))
		curp->outtype = OUTT_LOCALE;
	else if (0 == strcmp(arg, "xhtml"))
		curp->outtype = OUTT_HTML;
	else if (0 == strcmp(arg, "ps"))
		curp->outtype = OUTT_PS;
	else if (0 == strcmp(arg, "pdf"))
		curp->outtype = OUTT_PDF;
	else {
		warnx("-T %s: Bad argument", arg);
		return 0;
	}

	return 1;
}

static int
woptions(struct curparse *curp, char *arg)
{
	char		*v, *o;
	const char	*toks[7];

	toks[0] = "stop";
	toks[1] = "all";
	toks[2] = "warning";
	toks[3] = "error";
	toks[4] = "unsupp";
	toks[5] = "fatal";
	toks[6] = NULL;

	while (*arg) {
		o = arg;
		switch (getsubopt(&arg, UNCONST(toks), &v)) {
		case 0:
			curp->wstop = 1;
			break;
		case 1:
		case 2:
			curp->wlevel = MANDOCLEVEL_WARNING;
			break;
		case 3:
			curp->wlevel = MANDOCLEVEL_ERROR;
			break;
		case 4:
			curp->wlevel = MANDOCLEVEL_UNSUPP;
			break;
		case 5:
			curp->wlevel = MANDOCLEVEL_BADARG;
			break;
		default:
			warnx("-W %s: Bad argument", o);
			return 0;
		}
	}

	return 1;
}

static void
mmsg(enum mandocerr t, enum mandoclevel lvl,
		const char *file, int line, int col, const char *msg)
{
	const char	*mparse_msg;

	fprintf(stderr, "%s: %s:", getprogname(), file);

	if (line)
		fprintf(stderr, "%d:%d:", line, col + 1);

	fprintf(stderr, " %s", mparse_strlevel(lvl));

	if (NULL != (mparse_msg = mparse_strerror(t)))
		fprintf(stderr, ": %s", mparse_msg);

	if (msg)
		fprintf(stderr, ": %s", msg);

	fputc('\n', stderr);
}


