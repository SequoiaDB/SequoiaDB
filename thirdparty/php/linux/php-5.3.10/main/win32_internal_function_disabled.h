/*
   +----------------------------------------------------------------------+
   | PHP Version 5                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2012 The PHP Group                                |
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

/* $Id: win32_internal_function_disabled.h 321634 2012-01-01 13:15:04Z felipe $ */

/* 5 means the min version is 5 (XP/2000), 6 (2k8/vista), etc. */

/*
Windows Server 2008     6.0
Windows Vista           6.0
Windows Server 2003 R2  5.2
Windows Server 2003     5.2
Windows XP              5.1
Windows 2000            5.0
*/
static const char *function_name_5[] = {"link", NULL};
const int function_name_cnt_5 = 1;
static const char *function_name_6[] = {"readlink", "symlink", NULL};
const int function_name_cnt_6 = 2;
