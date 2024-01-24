/*
   +----------------------------------------------------------------------+
   | PHP Version 5                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2010 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Author: Pierre A. Joye <pierre@php.net>                              |
   +----------------------------------------------------------------------+
 */

/* $Id: link_win32.c 293036 2010-01-03 09:23:27Z sebastian $ */
#ifdef PHP_WIN32

#include "php.h"
#include "php_filestat.h"
#include "php_globals.h"

#include <WinBase.h>

#include <stdlib.h>

#include <string.h>
#if HAVE_PWD_H
#include "win32/pwd.h"
#endif

#if HAVE_GRP_H
#include "win32/grp.h"
#endif

#include <errno.h>
#include <ctype.h>

#include "safe_mode.h"
#include "php_link.h"
#include "php_string.h"

/*
TODO:
- Create php_readlink, php_link and php_symlink in win32/link.c
- Expose them (PHPAPI) so extensions developers can use them
- define link/readlink/symlink to their php_ equivalent and use them in ext/standart/link.c
- this file is then useless and we have a portable link API
*/

#ifndef VOLUME_NAME_NT
#define VOLUME_NAME_NT 0x2
#endif

#ifndef VOLUME_NAME_DOS
#define VOLUME_NAME_DOS 0x0
#endif

/* {{{ proto string readlink(string filename)
   Return the target of a symbolic link */
PHP_FUNCTION(readlink)
{
	HINSTANCE kernel32;
	char *link;
	int link_len;
	TCHAR Path[MAXPATHLEN];
	char path_resolved[MAXPATHLEN];
	HANDLE hFile;
	DWORD dwRet;

	typedef BOOL (WINAPI *gfpnh_func)(HANDLE, LPTSTR, DWORD, DWORD);
	gfpnh_func pGetFinalPathNameByHandle;

	kernel32 = LoadLibrary("kernel32.dll");

	if (kernel32) {
		pGetFinalPathNameByHandle = (gfpnh_func)GetProcAddress(kernel32, "GetFinalPathNameByHandleA");
		if (pGetFinalPathNameByHandle == NULL) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Can't call GetFinalPathNameByHandleA");
			RETURN_FALSE;
		}
	} else {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Can't call get a handle on kernel32.dll");
		RETURN_FALSE;
	}

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &link, &link_len) == FAILURE) {
		return;
	}

	if (OPENBASEDIR_CHECKPATH(link)) {
		RETURN_FALSE;
	}
	if (!expand_filepath(link, path_resolved TSRMLS_CC)) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "No such file or directory");
		RETURN_FALSE;
	}
	hFile = CreateFile(path_resolved,                  // file to open
				 GENERIC_READ,          // open for reading
				 FILE_SHARE_READ,       // share for reading
				 NULL,                  // default security
				 OPEN_EXISTING,         // existing file only
				 FILE_FLAG_BACKUP_SEMANTICS, // normal file
				 NULL);                 // no attr. template

	if( hFile == INVALID_HANDLE_VALUE) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Could not open file (error %d)", GetLastError());
			RETURN_FALSE;
	}

	dwRet = pGetFinalPathNameByHandle(hFile, Path, MAXPATHLEN, VOLUME_NAME_DOS);
	if(dwRet >= MAXPATHLEN) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Can't resolve the full path, the path exceeds the MAX_PATH_LEN (%d) limit", MAXPATHLEN);
		RETURN_FALSE;
	}

	CloseHandle(hFile);

	/* Append NULL to the end of the string */
	Path[dwRet] = '\0';

	if(dwRet > 4) {
		/* Skip first 4 characters if they are "\??\" */
		if(Path[0] == '\\' && Path[1] == '\\' && Path[2] == '?' && Path[3] ==  '\\') {
			RETURN_STRING(Path + 4, 1);
		}
	} else {
		RETURN_STRING(Path, 1);
	}
}
/* }}} */

/* {{{ proto int linkinfo(string filename)
   Returns the st_dev field of the UNIX C stat structure describing the link */
PHP_FUNCTION(linkinfo)
{
	char *link;
	int link_len;
	struct stat sb;
	int ret;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &link, &link_len) == FAILURE) {
		return;
	}

	ret = VCWD_STAT(link, &sb);
	if (ret == -1) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", strerror(errno));
		RETURN_LONG(-1L);
	}

	RETURN_LONG((long) sb.st_dev);
}
/* }}} */

/* {{{ proto int symlink(string target, string link)
   Create a symbolic link */
PHP_FUNCTION(symlink)
{
	char *topath, *frompath;
	int topath_len, frompath_len;
	BOOLEAN ret;
	char source_p[MAXPATHLEN];
	char dest_p[MAXPATHLEN];
	char dirname[MAXPATHLEN];
	size_t len;
	DWORD attr;
	HINSTANCE kernel32;
	typedef BOOLEAN (WINAPI *csla_func)(LPCSTR, LPCSTR, DWORD);
	csla_func pCreateSymbolicLinkA;

	kernel32 = LoadLibrary("kernel32.dll");

	if (kernel32) {
		pCreateSymbolicLinkA = (csla_func)GetProcAddress(kernel32, "CreateSymbolicLinkA");
		if (pCreateSymbolicLinkA == NULL) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Can't call CreateSymbolicLinkA");
			RETURN_FALSE;
		}
	} else {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Can't call get a handle on kernel32.dll");
		RETURN_FALSE;
	}

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss", &topath, &topath_len, &frompath, &frompath_len) == FAILURE) {
		return;
	}
	
	if (!expand_filepath(frompath, source_p TSRMLS_CC)) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "No such file or directory");
		RETURN_FALSE;
	}

	memcpy(dirname, source_p, sizeof(source_p));
	len = php_dirname(dirname, strlen(dirname));

	if (!expand_filepath_ex(topath, dest_p, dirname, len TSRMLS_CC)) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "No such file or directory");
		RETURN_FALSE;
	}

	if (php_stream_locate_url_wrapper(source_p, NULL, STREAM_LOCATE_WRAPPERS_ONLY TSRMLS_CC) ||
		php_stream_locate_url_wrapper(dest_p, NULL, STREAM_LOCATE_WRAPPERS_ONLY TSRMLS_CC) ) 
	{
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to symlink to a URL");
		RETURN_FALSE;
	}

	if (OPENBASEDIR_CHECKPATH(dest_p)) {
		RETURN_FALSE;
	}

	if (OPENBASEDIR_CHECKPATH(source_p)) {
		RETURN_FALSE;
	}

	if ((attr = GetFileAttributes(topath)) == INVALID_FILE_ATTRIBUTES) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Could not fetch file information(error %d)", GetLastError());
			RETURN_FALSE;
	}

	/* For the source, an expanded path must be used (in ZTS an other thread could have changed the CWD).
	 * For the target the exact string given by the user must be used, relative or not, existing or not.
	 * The target is relative to the link itself, not to the CWD. */
	ret = pCreateSymbolicLinkA(source_p, topath, (attr & FILE_ATTRIBUTE_DIRECTORY ? 1 : 0));

	if (!ret) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Cannot create symlink, error code(%d)", GetLastError());
		RETURN_FALSE;
	}

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto int link(string target, string link)
   Create a hard link */
PHP_FUNCTION(link)
{
	char *topath, *frompath;
	int topath_len, frompath_len;
	int ret;
	char source_p[MAXPATHLEN];
	char dest_p[MAXPATHLEN];

	/*First argument to link function is the target and hence should go to frompath
	  Second argument to link function is the link itself and hence should go to topath */
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss", &frompath, &frompath_len, &topath, &topath_len) == FAILURE) {
		return;
	}

	if (!expand_filepath(frompath, source_p TSRMLS_CC) || !expand_filepath(topath, dest_p TSRMLS_CC)) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "No such file or directory");
		RETURN_FALSE;
	}

	if (php_stream_locate_url_wrapper(source_p, NULL, STREAM_LOCATE_WRAPPERS_ONLY TSRMLS_CC) ||
		php_stream_locate_url_wrapper(dest_p, NULL, STREAM_LOCATE_WRAPPERS_ONLY TSRMLS_CC) ) 
	{
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to link to a URL");
		RETURN_FALSE;
	}

	if (OPENBASEDIR_CHECKPATH(source_p)) {
		RETURN_FALSE;
	}

	if (OPENBASEDIR_CHECKPATH(dest_p)) {
		RETURN_FALSE;
	}

#ifndef ZTS
	ret = CreateHardLinkA(topath, frompath, NULL);
#else 
	ret = CreateHardLinkA(dest_p, source_p, NULL);	
#endif	

	if (ret == 0) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", strerror(errno));
		RETURN_FALSE;
	}

	RETURN_TRUE;
}
/* }}} */

#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
