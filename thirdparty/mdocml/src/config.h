/*
#ifdef __cplusplus
#error "Do not use C++.  See the INSTALL file."
#endif
*/

#ifndef MANDOC_CONFIG_H
#define MANDOC_CONFIG_H
/*
#if defined(__linux__) || defined(__MINT__)
#define _GNU_SOURCE	// See test-*.c what needs this.
#endif
*/

#include <sys/types.h>

#define MAN_CONF_FILE "/etc/man.conf"
#define MANPATH_DEFAULT "/usr/share/man:/usr/X11R6/man:/usr/local/man"
#define HAVE_DIRENT_NAMLEN 0
#if defined ( _WIN32 )
#define HAVE_ERR 0
#else
#define HAVE_ERR 0
#endif
#define HAVE_FTS 1
#define HAVE_GETLINE 1
#define HAVE_GETSUBOPT 1
#define HAVE_ISBLANK 1
#define HAVE_MKDTEMP 1
//#define HAVE_MMAP 1
#define HAVE_MMAP 0
#define HAVE_PLEDGE 0
#define HAVE_PROGNAME 0
#define HAVE_REALLOCARRAY 0
#define HAVE_REWB_BSD 0
#define HAVE_REWB_SYSV 1
#define HAVE_SANDBOX_INIT 0
#define HAVE_STRCASESTR 1
#define HAVE_STRINGLIST 0
#define HAVE_STRLCAT 0
#define HAVE_STRLCPY 0
//#define HAVE_STRPTIME 1
#define HAVE_STRPTIME 0
#define HAVE_STRSEP 1
#define HAVE_STRTONUM 0
//#define HAVE_VASPRINTF 1
#define HAVE_VASPRINTF 0
#define HAVE_WCHAR 1
#define HAVE_SQLITE3 0
#define HAVE_SQLITE3_ERRSTR 1
#define HAVE_OHASH 0
#define HAVE_MANPATH 1

#define BINM_APROPOS "apropos"
#define BINM_MAKEWHATIS "makewhatis"
#define BINM_MAN "man"
#define BINM_SOELIM "soelim"
#define BINM_WHATIS "whatis"

#define STDIN_FILENO 0


#if defined ( _WIN32 )

#ifndef va_copy
#ifdef __va_copy
#define va_copy(DEST, SRC) __va_copy((DEST), (SRC))
#else
#define va_copy(DEST, SRC) memcpy((&DEST), (&SRC), sizeof(va_list))
#endif
#endif


#define snprintf _snprintf
#define vsnprintf _vsnprintf

#define strncasecmp _strnicmp
#define strcasecmp _stricmp

#define strtoll _strtoi64
#define strtoull _strtoui64

#define pclose _pclose
#define popen _popen

#define MAX_PATHSIZE   _MAX_PATH
#else 
#define MAX_PATHSIZE   PATH_MAX
#endif


extern 	const char *getprogname(void);
extern	void	  setprogname(const char *);
extern	void	 *reallocarray(void *, size_t, size_t);
extern	size_t	  strlcat(char *, const char *, size_t);
extern	size_t	  strlcpy(char *, const char *, size_t);
extern	long long strtonum(const char *, long long, long long, const char **);

#endif /* MANDOC_CONFIG_H */
