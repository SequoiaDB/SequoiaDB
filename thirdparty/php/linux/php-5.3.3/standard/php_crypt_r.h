/* $Id: php_crypt_r.h 293036 2010-01-03 09:23:27Z sebastian $ */
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
   | Authors: Pierre Alain Joye  <pajoye@php.net                          |
   +----------------------------------------------------------------------+
 */

#ifndef _CRYPT_WIHN32_H_
#define _CRYPT_WIHN32_H_

#ifdef __cplusplus
extern "C"
{
#endif
#include "crypt_freesec.h"

#ifndef __const
#ifdef __GNUC__
#define __CONST __const
#else
#define __CONST
#endif
#else
#define __CONST __const
#endif

void php_init_crypt_r();
void php_shutdown_crypt_r();

extern void _crypt_extended_init_r(void);

/*PHPAPI char* crypt(const char *key, const char *salt);*/
PHPAPI char *php_crypt_r (const char *__key, const char *__salt, struct php_crypt_extended_data * __data);

#define MD5_HASH_MAX_LEN 120

extern char * php_md5_crypt_r(const char *pw, const char *salt, char *out);
extern char * php_crypt_blowfish_rn(__CONST char *key, __CONST char *setting,
	char *output, int size);
extern char * php_sha512_crypt_r (const char *key, const char *salt, char *buffer, int buflen);
extern char * php_sha256_crypt_r (const char *key, const char *salt, char *buffer, int buflen);

#ifdef __cplusplus
}
#endif

#endif /* _CRYPT_WIHN32_H_ */
