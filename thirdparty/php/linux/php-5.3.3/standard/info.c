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
   | Authors: Rasmus Lerdorf <rasmus@php.net>                             |
   |          Zeev Suraski <zeev@zend.com>                                |
   |          Colin Viebrock <colin@easydns.com>                          |
   +----------------------------------------------------------------------+
*/

/* $Id: info.c 299960 2010-05-30 07:46:45Z pajoye $ */

#include "php.h"
#include "php_ini.h"
#include "php_globals.h"
#include "ext/standard/head.h"
#include "ext/standard/html.h"
#include "info.h"
#include "credits.h"
#include "css.h"
#include "SAPI.h"
#include <time.h>
#include "php_main.h"
#include "zend_globals.h"		/* needs ELS */
#include "zend_extensions.h"
#include "zend_highlight.h"
#ifdef HAVE_SYS_UTSNAME_H
#include <sys/utsname.h>
#endif

#ifdef PHP_WIN32
typedef void (WINAPI *PGNSI)(LPSYSTEM_INFO);
typedef BOOL (WINAPI *PGPI)(DWORD, DWORD, DWORD, DWORD, PDWORD);

# include "winver.h"

#if _MSC_VER < 1300
# define OSVERSIONINFOEX php_win_OSVERSIONINFOEX
#endif

#endif

#if HAVE_MBSTRING
#include "ext/mbstring/mbstring.h"
ZEND_EXTERN_MODULE_GLOBALS(mbstring)
#endif

#if HAVE_ICONV
#include "ext/iconv/php_iconv.h"
ZEND_EXTERN_MODULE_GLOBALS(iconv)
#endif

#define SECTION(name)	if (!sapi_module.phpinfo_as_text) { \
							PUTS("<h2>" name "</h2>\n"); \
						} else { \
							php_info_print_table_start(); \
							php_info_print_table_header(1, name); \
							php_info_print_table_end(); \
						} \

PHPAPI extern char *php_ini_opened_path;
PHPAPI extern char *php_ini_scanned_path;
PHPAPI extern char *php_ini_scanned_files;
	
static int php_info_write_wrapper(const char *str, uint str_length)
{
	int new_len, written;
	char *elem_esc;

	TSRMLS_FETCH();

	elem_esc = php_escape_html_entities((unsigned char *)str, str_length, &new_len, 0, ENT_QUOTES, NULL TSRMLS_CC);

	written = php_body_write(elem_esc, new_len TSRMLS_CC);

	efree(elem_esc);

	return written;
}


PHPAPI void php_info_print_module(zend_module_entry *zend_module TSRMLS_DC) /* {{{ */
{
	if (zend_module->info_func || zend_module->version) {
		if (!sapi_module.phpinfo_as_text) {
			php_printf("<h2><a name=\"module_%s\">%s</a></h2>\n", zend_module->name, zend_module->name);
		} else {
			php_info_print_table_start();
			php_info_print_table_header(1, zend_module->name);
			php_info_print_table_end();
		}
		if (zend_module->info_func) {
			zend_module->info_func(zend_module TSRMLS_CC);
		} else {
			php_info_print_table_start();
			php_info_print_table_row(2, "Version", zend_module->version);
			php_info_print_table_end();
			DISPLAY_INI_ENTRIES();
		}
	} else {
		if (!sapi_module.phpinfo_as_text) {
			php_printf("<tr><td>%s</td></tr>\n", zend_module->name);
		} else {
			php_printf("%s\n", zend_module->name);
		}	
	}
}
/* }}} */

static int _display_module_info_func(zend_module_entry *module TSRMLS_DC) /* {{{ */
{
	if (module->info_func || module->version) {
		php_info_print_module(module TSRMLS_CC);
	}
	return ZEND_HASH_APPLY_KEEP;
}
/* }}} */

static int _display_module_info_def(zend_module_entry *module TSRMLS_DC) /* {{{ */
{
	if (!module->info_func && !module->version) {
		php_info_print_module(module TSRMLS_CC);
	}
	return ZEND_HASH_APPLY_KEEP;
}
/* }}} */

/* {{{ php_print_gpcse_array
 */
static void php_print_gpcse_array(char *name, uint name_length TSRMLS_DC)
{
	zval **data, **tmp, tmp2;
	char *string_key;
	uint string_len;
	ulong num_key;

	zend_is_auto_global(name, name_length TSRMLS_CC);

	if (zend_hash_find(&EG(symbol_table), name, name_length+1, (void **) &data)!=FAILURE
		&& (Z_TYPE_PP(data)==IS_ARRAY)) {
		zend_hash_internal_pointer_reset(Z_ARRVAL_PP(data));
		while (zend_hash_get_current_data(Z_ARRVAL_PP(data), (void **) &tmp) == SUCCESS) {
			if (!sapi_module.phpinfo_as_text) {
				PUTS("<tr>");
				PUTS("<td class=\"e\">");

			}

			PUTS(name);
			PUTS("[\"");
			
			switch (zend_hash_get_current_key_ex(Z_ARRVAL_PP(data), &string_key, &string_len, &num_key, 0, NULL)) {
				case HASH_KEY_IS_STRING:
					if (!sapi_module.phpinfo_as_text) {
						php_info_html_esc_write(string_key, string_len - 1 TSRMLS_CC);
					} else {
						PHPWRITE(string_key, string_len - 1);
					}	
					break;
				case HASH_KEY_IS_LONG:
					php_printf("%ld", num_key);
					break;
			}
			PUTS("\"]");
			if (!sapi_module.phpinfo_as_text) {
				PUTS("</td><td class=\"v\">");
			} else {
				PUTS(" => ");
			}
			if (Z_TYPE_PP(tmp) == IS_ARRAY) {
				if (!sapi_module.phpinfo_as_text) {
					PUTS("<pre>");
					zend_print_zval_r_ex((zend_write_func_t) php_info_write_wrapper, *tmp, 0 TSRMLS_CC);
					PUTS("</pre>");
				} else {
					zend_print_zval_r(*tmp, 0 TSRMLS_CC);
				}
			} else if (Z_TYPE_PP(tmp) != IS_STRING) {
				tmp2 = **tmp;
				zval_copy_ctor(&tmp2);
				convert_to_string(&tmp2);
				if (!sapi_module.phpinfo_as_text) {
					if (Z_STRLEN(tmp2) == 0) {
						PUTS("<i>no value</i>");
					} else {
						php_info_html_esc_write(Z_STRVAL(tmp2), Z_STRLEN(tmp2) TSRMLS_CC);
					} 
				} else {
					PHPWRITE(Z_STRVAL(tmp2), Z_STRLEN(tmp2));
				}	
				zval_dtor(&tmp2);
			} else {
				if (!sapi_module.phpinfo_as_text) {
					if (Z_STRLEN_PP(tmp) == 0) {
						PUTS("<i>no value</i>");
					} else {
						php_info_html_esc_write(Z_STRVAL_PP(tmp), Z_STRLEN_PP(tmp) TSRMLS_CC);
					}
				} else {
					PHPWRITE(Z_STRVAL_PP(tmp), Z_STRLEN_PP(tmp));
				}	
			}
			if (!sapi_module.phpinfo_as_text) {
				PUTS("</td></tr>\n");
			} else {
				PUTS("\n");
			}	
			zend_hash_move_forward(Z_ARRVAL_PP(data));
		}
	}
}
/* }}} */

/* {{{ php_info_print_style
 */
void php_info_print_style(TSRMLS_D)
{
	php_printf("<style type=\"text/css\">\n");
	php_info_print_css(TSRMLS_C);
	php_printf("</style>\n");
}
/* }}} */

/* {{{ php_info_html_esc_write
 */
PHPAPI void php_info_html_esc_write(char *string, int str_len TSRMLS_DC)
{
	int new_len;
	char *ret = php_escape_html_entities((unsigned char *)string, str_len, &new_len, 0, ENT_QUOTES, NULL TSRMLS_CC);

	PHPWRITE(ret, new_len);
	efree(ret);
}
/* }}} */

/* {{{ php_info_html_esc
 */
PHPAPI char *php_info_html_esc(char *string TSRMLS_DC)
{
	int new_len;
	return php_escape_html_entities((unsigned char *)string, strlen(string), &new_len, 0, ENT_QUOTES, NULL TSRMLS_CC);
}
/* }}} */


#ifdef PHP_WIN32
/* {{{  */
char* php_get_windows_name()
{
	OSVERSIONINFOEX osvi;
	SYSTEM_INFO si;
	PGNSI pGNSI;
	PGPI pGPI;
	BOOL bOsVersionInfoEx;
	DWORD dwType;
	char *major = NULL, *sub = NULL, *retval;

	ZeroMemory(&si, sizeof(SYSTEM_INFO));
	ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

	if (!(bOsVersionInfoEx = GetVersionEx ((OSVERSIONINFO *) &osvi))) {
		return NULL;
	}

	pGNSI = (PGNSI) GetProcAddress(GetModuleHandle("kernel32.dll"), "GetNativeSystemInfo");
	if(NULL != pGNSI) {
		pGNSI(&si);
	} else {
		GetSystemInfo(&si);
	}

	if (VER_PLATFORM_WIN32_NT==osvi.dwPlatformId && osvi.dwMajorVersion > 4 ) {
		if (osvi.dwMajorVersion == 6) {
			if( osvi.dwMinorVersion == 0 ) {
				if( osvi.wProductType == VER_NT_WORKSTATION ) {
					major = "Windows Vista";
				} else {
					major = "Windows Server 2008";
				}
			} else
			if ( osvi.dwMinorVersion == 2 ) {
				if( osvi.wProductType == VER_NT_WORKSTATION )  {
					major = "Windows 7";
				} else {
					major = "Windows Server 2008 R2";
				}
			} else {
				major = "Unknow Windows version";
			}

			pGPI = (PGPI) GetProcAddress(GetModuleHandle("kernel32.dll"), "GetProductInfo");
			pGPI(6, 0, 0, 0, &dwType);

			switch (dwType) {
				case PRODUCT_ULTIMATE:
					sub = "Ultimate Edition";
					break;
				case PRODUCT_HOME_PREMIUM:
					sub = "Home Premium Edition";
					break;
				case PRODUCT_HOME_BASIC:
					sub = "Home Basic Edition";
					break;
				case PRODUCT_ENTERPRISE:
					sub = "Enterprise Edition";
					break;
				case PRODUCT_BUSINESS:
					sub = "Business Edition";
					break;
				case PRODUCT_STARTER:
					sub = "Starter Edition";
					break;
				case PRODUCT_CLUSTER_SERVER:
					sub = "Cluster Server Edition";
					break;
				case PRODUCT_DATACENTER_SERVER:
					sub = "Datacenter Edition";
					break;
				case PRODUCT_DATACENTER_SERVER_CORE:
					sub = "Datacenter Edition (core installation)";
					break;
				case PRODUCT_ENTERPRISE_SERVER:
					sub = "Enterprise Edition";
					break;
				case PRODUCT_ENTERPRISE_SERVER_CORE:
					sub = "Enterprise Edition (core installation)";
					break;
				case PRODUCT_ENTERPRISE_SERVER_IA64:
					sub = "Enterprise Edition for Itanium-based Systems";
					break;
				case PRODUCT_SMALLBUSINESS_SERVER:
					sub = "Small Business Server";
					break;
				case PRODUCT_SMALLBUSINESS_SERVER_PREMIUM:
					sub = "Small Business Server Premium Edition";
					break;
				case PRODUCT_STANDARD_SERVER:
					sub = "Standard Edition";
					break;
				case PRODUCT_STANDARD_SERVER_CORE:
					sub = "Standard Edition (core installation)";
					break;
				case PRODUCT_WEB_SERVER:
					sub = "Web Server Edition";
					break;
			}
		}

		if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 2 )	{
			if (GetSystemMetrics(SM_SERVERR2))
				major = "Windows Server 2003 R2";
			else if (osvi.wSuiteMask == VER_SUITE_STORAGE_SERVER)
				major = "Windows Storage Server 2003";
			else if (osvi.wSuiteMask == VER_SUITE_WH_SERVER)
				major = "Windows Home Server";
			else if (osvi.wProductType == VER_NT_WORKSTATION &&
				si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64) {
				major = "Windows XP Professional x64 Edition";
			} else {
				major = "Windows Server 2003";
			}

			/* Test for the server type. */
			if ( osvi.wProductType != VER_NT_WORKSTATION ) {
				if ( si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_IA64 ) {
					if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
						sub = "Datacenter Edition for Itanium-based Systems";
					else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
						sub = "Enterprise Edition for Itanium-based Systems";
				}

				else if ( si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64 ) {
					if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
						sub = "Datacenter x64 Edition";
					else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
						sub = "Enterprise x64 Edition";
					else sub = "Standard x64 Edition";
				} else {
					if ( osvi.wSuiteMask & VER_SUITE_COMPUTE_SERVER )
						sub = "Compute Cluster Edition";
					else if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
						sub = "Datacenter Edition";
					else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
						sub = "Enterprise Edition";
					else if ( osvi.wSuiteMask & VER_SUITE_BLADE )
						sub = "Web Edition";
					else sub = "Standard Edition";
				}
			} 
		}

		if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1 )	{
			major = "Windows XP";
			if( osvi.wSuiteMask & VER_SUITE_PERSONAL )
				sub = "Home Edition";
			else sub = "Professional";
		}

		if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0 ) {
			major = "Windows 2000";

			if (osvi.wProductType == VER_NT_WORKSTATION ) {
				sub = "Professional";
			} else {
				if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
					sub = "Datacenter Server";
				else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
					sub = "Advanced Server";
				else sub = "Server";
			}
		}
	} else {
		return NULL;
	}

	spprintf(&retval, 0, "%s%s%s%s%s", major, sub?" ":"", sub?sub:"", osvi.szCSDVersion[0] != '\0'?" ":"", osvi.szCSDVersion);
	return retval;
}
/* }}}  */

/* {{{  */
void php_get_windows_cpu(char *buf, int bufsize)
{
	SYSTEM_INFO SysInfo;
	GetSystemInfo(&SysInfo);
	switch (SysInfo.wProcessorArchitecture) {
		case PROCESSOR_ARCHITECTURE_INTEL :
			snprintf(buf, bufsize, "i%d", SysInfo.dwProcessorType);
			break;
		case PROCESSOR_ARCHITECTURE_MIPS :
			snprintf(buf, bufsize, "MIPS R%d000", SysInfo.wProcessorLevel);
			break;
		case PROCESSOR_ARCHITECTURE_ALPHA :
			snprintf(buf, bufsize, "Alpha %d", SysInfo.wProcessorLevel);
			break;
		case PROCESSOR_ARCHITECTURE_PPC :
			snprintf(buf, bufsize, "PPC 6%02d", SysInfo.wProcessorLevel);
			break;
		case PROCESSOR_ARCHITECTURE_IA64 :
			snprintf(buf, bufsize,  "IA64");
			break;
#if defined(PROCESSOR_ARCHITECTURE_IA32_ON_WIN64)
		case PROCESSOR_ARCHITECTURE_IA32_ON_WIN64 :
			snprintf(buf, bufsize, "IA32");
			break;
#endif
#if defined(PROCESSOR_ARCHITECTURE_AMD64)
		case PROCESSOR_ARCHITECTURE_AMD64 :
			snprintf(buf, bufsize, "AMD64");
			break;
#endif
		case PROCESSOR_ARCHITECTURE_UNKNOWN :
		default:
			snprintf(buf, bufsize, "Unknown");
			break;
	}
}
/* }}}  */
#endif

/* {{{ php_get_uname
 */
PHPAPI char *php_get_uname(char mode)
{
	char *php_uname;
	char tmp_uname[256];
#ifdef PHP_WIN32
	DWORD dwBuild=0;
	DWORD dwVersion = GetVersion();
	DWORD dwWindowsMajorVersion =  (DWORD)(LOBYTE(LOWORD(dwVersion)));
	DWORD dwWindowsMinorVersion =  (DWORD)(HIBYTE(LOWORD(dwVersion)));
	DWORD dwSize = MAX_COMPUTERNAME_LENGTH + 1;
	char ComputerName[MAX_COMPUTERNAME_LENGTH + 1];
	
	GetComputerName(ComputerName, &dwSize);

	if (mode == 's') {
		if (dwVersion < 0x80000000) {
			php_uname = "Windows NT";
		} else {
			php_uname = "Windows 9x";
		}
	} else if (mode == 'r') {
		snprintf(tmp_uname, sizeof(tmp_uname), "%d.%d", dwWindowsMajorVersion, dwWindowsMinorVersion);
		php_uname = tmp_uname;
	} else if (mode == 'n') {
		php_uname = ComputerName;
	} else if (mode == 'v') {
		char *winver = php_get_windows_name();
		dwBuild = (DWORD)(HIWORD(dwVersion));
		if(winver == NULL) {
			snprintf(tmp_uname, sizeof(tmp_uname), "build %d", dwBuild);
		} else {
			snprintf(tmp_uname, sizeof(tmp_uname), "build %d (%s)", dwBuild, winver);
		}
		php_uname = tmp_uname;
		if(winver) {
			efree(winver);
		}
	} else if (mode == 'm') {
		php_get_windows_cpu(tmp_uname, sizeof(tmp_uname));
		php_uname = tmp_uname;
	} else { /* assume mode == 'a' */
		/* Get build numbers for Windows NT or Win95 */
		if (dwVersion < 0x80000000){
			char *winver = php_get_windows_name();
			char wincpu[20];

			php_get_windows_cpu(wincpu, sizeof(wincpu));
			dwBuild = (DWORD)(HIWORD(dwVersion));
			snprintf(tmp_uname, sizeof(tmp_uname), "%s %s %d.%d build %d (%s) %s",
					 "Windows NT", ComputerName,
					 dwWindowsMajorVersion, dwWindowsMinorVersion, dwBuild, winver?winver:"unknown", wincpu);
			if(winver) {
				efree(winver);
			}
		} else {
			snprintf(tmp_uname, sizeof(tmp_uname), "%s %s %d.%d",
					 "Windows 9x", ComputerName,
					 dwWindowsMajorVersion, dwWindowsMinorVersion);
		}
		php_uname = tmp_uname;
	}
#else
#ifdef HAVE_SYS_UTSNAME_H
	struct utsname buf;
	if (uname((struct utsname *)&buf) == -1) {
		php_uname = PHP_UNAME;
	} else {
#ifdef NETWARE
		if (mode == 's') {
			php_uname = buf.sysname;
		} else if (mode == 'r') {
			snprintf(tmp_uname, sizeof(tmp_uname), "%d.%d.%d", 
					 buf.netware_major, buf.netware_minor, buf.netware_revision);
			php_uname = tmp_uname;
		} else if (mode == 'n') {
			php_uname = buf.servername;
		} else if (mode == 'v') {
			snprintf(tmp_uname, sizeof(tmp_uname), "libc-%d.%d.%d #%d",
					 buf.libmajor, buf.libminor, buf.librevision, buf.libthreshold);
			php_uname = tmp_uname;
		} else if (mode == 'm') {
			php_uname = buf.machine;
		} else { /* assume mode == 'a' */
			snprintf(tmp_uname, sizeof(tmp_uname), "%s %s %d.%d.%d libc-%d.%d.%d #%d %s",
					 buf.sysname, buf.servername,
					 buf.netware_major, buf.netware_minor, buf.netware_revision,
					 buf.libmajor, buf.libminor, buf.librevision, buf.libthreshold,
					 buf.machine);
			php_uname = tmp_uname;
		}
#else
		if (mode == 's') {
			php_uname = buf.sysname;
		} else if (mode == 'r') {
			php_uname = buf.release;
		} else if (mode == 'n') {
			php_uname = buf.nodename;
		} else if (mode == 'v') {
			php_uname = buf.version;
		} else if (mode == 'm') {
			php_uname = buf.machine;
		} else { /* assume mode == 'a' */
			snprintf(tmp_uname, sizeof(tmp_uname), "%s %s %s %s %s",
					 buf.sysname, buf.nodename, buf.release, buf.version,
					 buf.machine);
			php_uname = tmp_uname;
		}
#endif /* NETWARE */
	}
#else
	php_uname = PHP_UNAME;
#endif
#endif
	return estrdup(php_uname);
}
/* }}} */


/* {{{ php_print_info_htmlhead
 */
PHPAPI void php_print_info_htmlhead(TSRMLS_D)
{

/*** none of this is needed now ***

	const char *charset = NULL;

	if (SG(default_charset)) {
		charset = SG(default_charset);
	}

#if HAVE_MBSTRING
	if (php_ob_handler_used("mb_output_handler" TSRMLS_CC)) {
		if (MBSTRG(current_http_output_encoding) == mbfl_no_encoding_pass) {
			charset = "US-ASCII";
		} else {
			charset = mbfl_no2preferred_mime_name(MBSTRG(current_http_output_encoding));
		}
	}
#endif   

#if HAVE_ICONV
	if (php_ob_handler_used("ob_iconv_handler" TSRMLS_CC)) {
		charset = ICONVG(output_encoding);
	}
#endif

	if (!charset || !charset[0]) {
		charset = "US-ASCII";
	}

*** none of that is needed now ***/


	PUTS("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"DTD/xhtml1-transitional.dtd\">\n");
	PUTS("<html>");
	PUTS("<head>\n");
	php_info_print_style(TSRMLS_C);
	PUTS("<title>phpinfo()</title>");
	PUTS("<meta name=\"ROBOTS\" content=\"NOINDEX,NOFOLLOW,NOARCHIVE\" />");
/*
	php_printf("<meta http-equiv=\"Content-Type\" content=\"text/html; charset=%s\" />\n", charset);
*/
	PUTS("</head>\n");
	PUTS("<body><div class=\"center\">\n");
}
/* }}} */

/* {{{ module_name_cmp */
static int module_name_cmp(const void *a, const void *b TSRMLS_DC)
{
	Bucket *f = *((Bucket **) a);
	Bucket *s = *((Bucket **) b);

	return strcasecmp(((zend_module_entry *)f->pData)->name,
				  ((zend_module_entry *)s->pData)->name);
}
/* }}} */

/* {{{ php_print_info
 */
PHPAPI void php_print_info(int flag TSRMLS_DC)
{
	char **env, *tmp1, *tmp2;
	char *php_uname;
	int expose_php = INI_INT("expose_php");

	if (!sapi_module.phpinfo_as_text) {
		php_print_info_htmlhead(TSRMLS_C);
	} else {
		PUTS("phpinfo()\n");
	}	

	if (flag & PHP_INFO_GENERAL) {
		char *zend_version = get_zend_version();
		char temp_api[10];
		char *logo_guid;

		php_uname = php_get_uname('a');
		
		if (!sapi_module.phpinfo_as_text) {
			php_info_print_box_start(1);
		}

		if (expose_php && !sapi_module.phpinfo_as_text) {
			PUTS("<a href=\"http://www.php.net/\"><img border=\"0\" src=\"");
			if (SG(request_info).request_uri) {
				char *elem_esc = php_info_html_esc(SG(request_info).request_uri TSRMLS_CC);
				PUTS(elem_esc);
				efree(elem_esc);
			}
			PUTS("?=");
			logo_guid = php_logo_guid();
			PUTS(logo_guid);
			efree(logo_guid);
			PUTS("\" alt=\"PHP Logo\" /></a>");
		}

		if (!sapi_module.phpinfo_as_text) {
			php_printf("<h1 class=\"p\">PHP Version %s</h1>\n", PHP_VERSION);
		} else {
			php_info_print_table_row(2, "PHP Version", PHP_VERSION);
		}	
		php_info_print_box_end();
		php_info_print_table_start();
		php_info_print_table_row(2, "System", php_uname );
		php_info_print_table_row(2, "Build Date", __DATE__ " " __TIME__ );
#ifdef COMPILER
		php_info_print_table_row(2, "Compiler", COMPILER);
#endif
#ifdef ARCHITECTURE
		php_info_print_table_row(2, "Architecture", ARCHITECTURE);
#endif
#ifdef CONFIGURE_COMMAND
		php_info_print_table_row(2, "Configure Command", CONFIGURE_COMMAND );
#endif

		if (sapi_module.pretty_name) {
			php_info_print_table_row(2, "Server API", sapi_module.pretty_name );
		}

#ifdef VIRTUAL_DIR
		php_info_print_table_row(2, "Virtual Directory Support", "enabled" );
#else
		php_info_print_table_row(2, "Virtual Directory Support", "disabled" );
#endif

		php_info_print_table_row(2, "Configuration File (php.ini) Path", PHP_CONFIG_FILE_PATH);
		php_info_print_table_row(2, "Loaded Configuration File", php_ini_opened_path ? php_ini_opened_path : "(none)");
		php_info_print_table_row(2, "Scan this dir for additional .ini files", php_ini_scanned_path ? php_ini_scanned_path : "(none)");
		php_info_print_table_row(2, "Additional .ini files parsed", php_ini_scanned_files ? php_ini_scanned_files : "(none)");

		snprintf(temp_api, sizeof(temp_api), "%d", PHP_API_VERSION);
		php_info_print_table_row(2, "PHP API", temp_api);

		snprintf(temp_api, sizeof(temp_api), "%d", ZEND_MODULE_API_NO);
		php_info_print_table_row(2, "PHP Extension", temp_api);

		snprintf(temp_api, sizeof(temp_api), "%d", ZEND_EXTENSION_API_NO);
		php_info_print_table_row(2, "Zend Extension", temp_api);

		php_info_print_table_row(2, "Zend Extension Build", ZEND_EXTENSION_BUILD_ID);
		php_info_print_table_row(2, "PHP Extension Build", ZEND_MODULE_BUILD_ID);

#if ZEND_DEBUG
		php_info_print_table_row(2, "Debug Build", "yes" );
#else
		php_info_print_table_row(2, "Debug Build", "no" );
#endif

#ifdef ZTS
		php_info_print_table_row(2, "Thread Safety", "enabled" );
#else
		php_info_print_table_row(2, "Thread Safety", "disabled" );
#endif

		php_info_print_table_row(2, "Zend Memory Manager", is_zend_mm(TSRMLS_C) ? "enabled" : "disabled" );

#ifdef ZEND_MULTIBYTE
		php_info_print_table_row(2, "Zend Multibyte Support", "enabled");
#else
		php_info_print_table_row(2, "Zend Multibyte Support", "disabled");
#endif

#if HAVE_IPV6
		php_info_print_table_row(2, "IPv6 Support", "enabled" );
#else
		php_info_print_table_row(2, "IPv6 Support", "disabled" );
#endif
		{
			HashTable *url_stream_wrappers_hash;
			char *stream_protocol, *stream_protocols_buf = NULL;
			int stream_protocol_len, stream_protocols_buf_len = 0;
			ulong num_key;

			if ((url_stream_wrappers_hash = php_stream_get_url_stream_wrappers_hash())) {
				HashPosition pos;
				for (zend_hash_internal_pointer_reset_ex(url_stream_wrappers_hash, &pos);
						zend_hash_get_current_key_ex(url_stream_wrappers_hash, &stream_protocol, (uint *)&stream_protocol_len, &num_key, 0, &pos) == HASH_KEY_IS_STRING;
						zend_hash_move_forward_ex(url_stream_wrappers_hash, &pos)) {
					stream_protocols_buf = erealloc(stream_protocols_buf, stream_protocols_buf_len + stream_protocol_len + 2 + 1);
					memcpy(stream_protocols_buf + stream_protocols_buf_len, stream_protocol, stream_protocol_len - 1);
					stream_protocols_buf[stream_protocols_buf_len + stream_protocol_len - 1] = ',';
					stream_protocols_buf[stream_protocols_buf_len + stream_protocol_len] = ' ';
					stream_protocols_buf_len += stream_protocol_len + 1;
				}
				if (stream_protocols_buf) {
					stream_protocols_buf[stream_protocols_buf_len - 2] = ' ';
					stream_protocols_buf[stream_protocols_buf_len] = 0;
					php_info_print_table_row(2, "Registered PHP Streams", stream_protocols_buf);
					efree(stream_protocols_buf);
				} else {
					/* Any chances we will ever hit this? */
					php_info_print_table_row(2, "Registered PHP Streams", "no streams registered");
				}
			} else {
				/* Any chances we will ever hit this? */
				php_info_print_table_row(2, "PHP Streams", "disabled"); /* ?? */
			}
		}

		{
			HashTable *stream_xport_hash;
			char *xport_name, *xport_buf = NULL;
			int xport_name_len, xport_buf_len = 0, xport_buf_size = 0;
			ulong num_key;

			if ((stream_xport_hash = php_stream_xport_get_hash())) {
				HashPosition pos;
				for(zend_hash_internal_pointer_reset_ex(stream_xport_hash, &pos);
					zend_hash_get_current_key_ex(stream_xport_hash, &xport_name, (uint *)&xport_name_len, &num_key, 0, &pos) == HASH_KEY_IS_STRING;
					zend_hash_move_forward_ex(stream_xport_hash, &pos)) {
					if (xport_buf_len + xport_name_len + 2 > xport_buf_size) {
						while (xport_buf_len + xport_name_len + 2 > xport_buf_size) {
							xport_buf_size += 256;
						}
						if (xport_buf) {
							xport_buf = erealloc(xport_buf, xport_buf_size);
						} else {
							xport_buf = emalloc(xport_buf_size);
						}
					}
					if (xport_buf_len > 0) {
						xport_buf[xport_buf_len++] = ',';
						xport_buf[xport_buf_len++] = ' ';
					}
					memcpy(xport_buf + xport_buf_len, xport_name, xport_name_len - 1);
					xport_buf_len += xport_name_len - 1;
					xport_buf[xport_buf_len] = '\0';
				}
				if (xport_buf) {
					php_info_print_table_row(2, "Registered Stream Socket Transports", xport_buf);
					efree(xport_buf);
				} else {
					/* Any chances we will ever hit this? */
					php_info_print_table_row(2, "Registered Stream Socket Transports", "no transports registered");
				}
			} else {
				/* Any chances we will ever hit this? */
				php_info_print_table_row(2, "Stream Socket Transports", "disabled"); /* ?? */
			}
		}

		{
			HashTable *stream_filter_hash;
			char *filter_name, *filter_buf = NULL;
			int filter_name_len, filter_buf_len = 0, filter_buf_size = 0;
			ulong num_key;

			if ((stream_filter_hash = php_get_stream_filters_hash())) {
				HashPosition pos;
				for(zend_hash_internal_pointer_reset_ex(stream_filter_hash, &pos);
					zend_hash_get_current_key_ex(stream_filter_hash, &filter_name, (uint *)&filter_name_len, &num_key, 0, &pos) == HASH_KEY_IS_STRING;
					zend_hash_move_forward_ex(stream_filter_hash, &pos)) {
					if (filter_buf_len + filter_name_len + 2 > filter_buf_size) {
						while (filter_buf_len + filter_name_len + 2 > filter_buf_size) {
							filter_buf_size += 256;
						}
						if (filter_buf) {
							filter_buf = erealloc(filter_buf, filter_buf_size);
						} else {
							filter_buf = emalloc(filter_buf_size);
						}
					}
					if (filter_buf_len > 0) {
						filter_buf[filter_buf_len++] = ',';
						filter_buf[filter_buf_len++] = ' ';
					}
					memcpy(filter_buf + filter_buf_len, filter_name, filter_name_len - 1);
					filter_buf_len += filter_name_len - 1;
					filter_buf[filter_buf_len] = '\0';
				}
				if (filter_buf) {
					php_info_print_table_row(2, "Registered Stream Filters", filter_buf);
					efree(filter_buf);
				} else {
					/* Any chances we will ever hit this? */
					php_info_print_table_row(2, "Registered Stream Filters", "no filters registered");
				}
			} else {
				/* Any chances we will ever hit this? */
				php_info_print_table_row(2, "Stream Filters", "disabled"); /* ?? */
			}
		}
		
		php_info_print_table_end();

		/* Zend Engine */
		php_info_print_box_start(0);
		if (expose_php && !sapi_module.phpinfo_as_text) {
			PUTS("<a href=\"http://www.zend.com/\"><img border=\"0\" src=\"");
			if (SG(request_info).request_uri) {
				char *elem_esc = php_info_html_esc(SG(request_info).request_uri TSRMLS_CC);
				PUTS(elem_esc);
				efree(elem_esc);
			}
			PUTS("?="ZEND_LOGO_GUID"\" alt=\"Zend logo\" /></a>\n");
		}
		PUTS("This program makes use of the Zend Scripting Language Engine:");
		PUTS(!sapi_module.phpinfo_as_text?"<br />":"\n");
		if (sapi_module.phpinfo_as_text) {
			PUTS(zend_version);
		} else {
			zend_html_puts(zend_version, strlen(zend_version) TSRMLS_CC);
		}
		php_info_print_box_end();
		efree(php_uname);
	}

	if ((flag & PHP_INFO_CREDITS) && expose_php && !sapi_module.phpinfo_as_text) {	
		php_info_print_hr();
		PUTS("<h1><a href=\"");
		if (SG(request_info).request_uri) {
			char *elem_esc = php_info_html_esc(SG(request_info).request_uri TSRMLS_CC);
			PUTS(elem_esc);
			efree(elem_esc);
		}
		PUTS("?=PHPB8B5F2A0-3C92-11d3-A3A9-4C7B08C10000\">");
		PUTS("PHP Credits");
		PUTS("</a></h1>\n");
	}

	zend_ini_sort_entries(TSRMLS_C);

	if (flag & PHP_INFO_CONFIGURATION) {
		php_info_print_hr();
		if (!sapi_module.phpinfo_as_text) {
			PUTS("<h1>Configuration</h1>\n");
		} else {
			SECTION("Configuration");
		}	
		if (!(flag & PHP_INFO_MODULES)) {
			SECTION("PHP Core");
			display_ini_entries(NULL);
		}
	}

	if (flag & PHP_INFO_MODULES) {
		HashTable sorted_registry;
		zend_module_entry tmp;

		zend_hash_init(&sorted_registry, zend_hash_num_elements(&module_registry), NULL, NULL, 1);
		zend_hash_copy(&sorted_registry, &module_registry, NULL, &tmp, sizeof(zend_module_entry));
		zend_hash_sort(&sorted_registry, zend_qsort, module_name_cmp, 0 TSRMLS_CC);

		zend_hash_apply(&sorted_registry, (apply_func_t) _display_module_info_func TSRMLS_CC);

		SECTION("Additional Modules");
		php_info_print_table_start();
		php_info_print_table_header(1, "Module Name");
		zend_hash_apply(&sorted_registry, (apply_func_t) _display_module_info_def TSRMLS_CC);
		php_info_print_table_end();

		zend_hash_destroy(&sorted_registry);
	}

	if (flag & PHP_INFO_ENVIRONMENT) {
		SECTION("Environment");
		php_info_print_table_start();
		php_info_print_table_header(2, "Variable", "Value");
		for (env=environ; env!=NULL && *env !=NULL; env++) {
			tmp1 = estrdup(*env);
			if (!(tmp2=strchr(tmp1,'='))) { /* malformed entry? */
				efree(tmp1);
				continue;
			}
			*tmp2 = 0;
			tmp2++;
			php_info_print_table_row(2, tmp1, tmp2);
			efree(tmp1);
		}
		php_info_print_table_end();
	}

	if (flag & PHP_INFO_VARIABLES) {
		zval **data;

		SECTION("PHP Variables");

		php_info_print_table_start();
		php_info_print_table_header(2, "Variable", "Value");
		if (zend_hash_find(&EG(symbol_table), "PHP_SELF", sizeof("PHP_SELF"), (void **) &data) != FAILURE) {
			php_info_print_table_row(2, "PHP_SELF", Z_STRVAL_PP(data));
		}
		if (zend_hash_find(&EG(symbol_table), "PHP_AUTH_TYPE", sizeof("PHP_AUTH_TYPE"), (void **) &data) != FAILURE) {
			php_info_print_table_row(2, "PHP_AUTH_TYPE", Z_STRVAL_PP(data));
		}
		if (zend_hash_find(&EG(symbol_table), "PHP_AUTH_USER", sizeof("PHP_AUTH_USER"), (void **) &data) != FAILURE) {
			php_info_print_table_row(2, "PHP_AUTH_USER", Z_STRVAL_PP(data));
		}
		if (zend_hash_find(&EG(symbol_table), "PHP_AUTH_PW", sizeof("PHP_AUTH_PW"), (void **) &data) != FAILURE) {
			php_info_print_table_row(2, "PHP_AUTH_PW", Z_STRVAL_PP(data));
		}
		php_print_gpcse_array("_REQUEST", sizeof("_REQUEST")-1 TSRMLS_CC);
		php_print_gpcse_array("_GET", sizeof("_GET")-1 TSRMLS_CC);
		php_print_gpcse_array("_POST", sizeof("_POST")-1 TSRMLS_CC);
		php_print_gpcse_array("_FILES", sizeof("_FILES")-1 TSRMLS_CC);
		php_print_gpcse_array("_COOKIE", sizeof("_COOKIE")-1 TSRMLS_CC);
		php_print_gpcse_array("_SERVER", sizeof("_SERVER")-1 TSRMLS_CC);
		php_print_gpcse_array("_ENV", sizeof("_ENV")-1 TSRMLS_CC);
		php_info_print_table_end();
	}

	if (flag & PHP_INFO_LICENSE) {
		if (!sapi_module.phpinfo_as_text) {
			SECTION("PHP License");
			php_info_print_box_start(0);
			PUTS("<p>\n");
			PUTS("This program is free software; you can redistribute it and/or modify ");
			PUTS("it under the terms of the PHP License as published by the PHP Group ");
			PUTS("and included in the distribution in the file:  LICENSE\n");
			PUTS("</p>\n");
			PUTS("<p>");
			PUTS("This program is distributed in the hope that it will be useful, ");
			PUTS("but WITHOUT ANY WARRANTY; without even the implied warranty of ");
			PUTS("MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n");
			PUTS("</p>\n");
			PUTS("<p>");
			PUTS("If you did not receive a copy of the PHP license, or have any questions about ");
			PUTS("PHP licensing, please contact license@php.net.\n");
			PUTS("</p>\n");
			php_info_print_box_end();
		} else {
			PUTS("\nPHP License\n");
			PUTS("This program is free software; you can redistribute it and/or modify\n");
			PUTS("it under the terms of the PHP License as published by the PHP Group\n");
			PUTS("and included in the distribution in the file:  LICENSE\n");
			PUTS("\n");
			PUTS("This program is distributed in the hope that it will be useful,\n");
			PUTS("but WITHOUT ANY WARRANTY; without even the implied warranty of\n");
			PUTS("MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n");
			PUTS("\n");
			PUTS("If you did not receive a copy of the PHP license, or have any\n");
			PUTS("questions about PHP licensing, please contact license@php.net.\n");
		}
	}
	if (!sapi_module.phpinfo_as_text) {
		PUTS("</div></body></html>");
	}	
}
/* }}} */


PHPAPI void php_info_print_table_start(void)
{
	if (!sapi_module.phpinfo_as_text) {
		php_printf("<table border=\"0\" cellpadding=\"3\" width=\"600\">\n");
	} else {
		php_printf("\n");
	}	
}

PHPAPI void php_info_print_table_end(void)
{
	if (!sapi_module.phpinfo_as_text) {
		php_printf("</table><br />\n");
	}

}

PHPAPI void php_info_print_box_start(int flag)
{
	php_info_print_table_start();
	if (flag) {
		if (!sapi_module.phpinfo_as_text) {
			php_printf("<tr class=\"h\"><td>\n");
		}
	} else {
		if (!sapi_module.phpinfo_as_text) {
			php_printf("<tr class=\"v\"><td>\n");
		} else {
			php_printf("\n");
		}	
	}
}

PHPAPI void php_info_print_box_end(void)
{
	if (!sapi_module.phpinfo_as_text) {
		php_printf("</td></tr>\n");
	}
	php_info_print_table_end();
}

PHPAPI void php_info_print_hr(void)
{
	if (!sapi_module.phpinfo_as_text) {
		php_printf("<hr />\n");
	} else {
		php_printf("\n\n _______________________________________________________________________\n\n");
	}
}

PHPAPI void php_info_print_table_colspan_header(int num_cols, char *header)
{
	int spaces;

	if (!sapi_module.phpinfo_as_text) {
		php_printf("<tr class=\"h\"><th colspan=\"%d\">%s</th></tr>\n", num_cols, header );
	} else {
		spaces = (74 - strlen(header));
		php_printf("%*s%s%*s\n", (int)(spaces/2), " ", header, (int)(spaces/2), " ");
	}	
}

/* {{{ php_info_print_table_header
 */
PHPAPI void php_info_print_table_header(int num_cols, ...)
{
	int i;
	va_list row_elements;
	char *row_element;

	TSRMLS_FETCH();

	va_start(row_elements, num_cols);
	if (!sapi_module.phpinfo_as_text) {
		php_printf("<tr class=\"h\">");
	}	
	for (i=0; i<num_cols; i++) {
		row_element = va_arg(row_elements, char *);
		if (!row_element || !*row_element) {
			row_element = " ";
		}
		if (!sapi_module.phpinfo_as_text) {
			PUTS("<th>");
			PUTS(row_element);
			PUTS("</th>");
		} else {
			PUTS(row_element);
			if (i < num_cols-1) {
				PUTS(" => ");
			} else {
				PUTS("\n");
			}	
		}	
	}
	if (!sapi_module.phpinfo_as_text) {
		php_printf("</tr>\n");
	}	

	va_end(row_elements);
}
/* }}} */

/* {{{ php_info_print_table_row_internal
 */
static void php_info_print_table_row_internal(int num_cols, 
		const char *value_class, va_list row_elements)
{
	int i;
	char *row_element;
	char *elem_esc = NULL;
/*
	int elem_esc_len;
*/

	TSRMLS_FETCH();

	if (!sapi_module.phpinfo_as_text) {
		php_printf("<tr>");
	}	
	for (i=0; i<num_cols; i++) {
		if (!sapi_module.phpinfo_as_text) {
			php_printf("<td class=\"%s\">",
			   (i==0 ? "e" : value_class )
			);
		}	
		row_element = va_arg(row_elements, char *);
		if (!row_element || !*row_element) {
			if (!sapi_module.phpinfo_as_text) {
				PUTS( "<i>no value</i>" );
			} else {
				PUTS( " " );
			}
		} else {
			if (!sapi_module.phpinfo_as_text) {
				elem_esc = php_info_html_esc(row_element TSRMLS_CC);
				PUTS(elem_esc);
				efree(elem_esc);
			} else {
				PUTS(row_element);
				if (i < num_cols-1) {
					PUTS(" => ");
				}	
			}
		}
		if (!sapi_module.phpinfo_as_text) {
			php_printf(" </td>");
		} else if (i == (num_cols - 1)) {
			PUTS("\n");
		}
	}
	if (!sapi_module.phpinfo_as_text) {
		php_printf("</tr>\n");
	}
}
/* }}} */

/* {{{ php_info_print_table_row
 */
PHPAPI void php_info_print_table_row(int num_cols, ...)
{
	va_list row_elements;
	
	va_start(row_elements, num_cols);
	php_info_print_table_row_internal(num_cols, "v", row_elements);
	va_end(row_elements);
}
/* }}} */

/* {{{ php_info_print_table_row_ex
 */
PHPAPI void php_info_print_table_row_ex(int num_cols, const char *value_class, 
		...)
{
	va_list row_elements;
	
	va_start(row_elements, value_class);
	php_info_print_table_row_internal(num_cols, value_class, row_elements);
	va_end(row_elements);
}
/* }}} */

/* {{{ register_phpinfo_constants
 */
void register_phpinfo_constants(INIT_FUNC_ARGS)
{
	REGISTER_LONG_CONSTANT("INFO_GENERAL", PHP_INFO_GENERAL, CONST_PERSISTENT|CONST_CS);
	REGISTER_LONG_CONSTANT("INFO_CREDITS", PHP_INFO_CREDITS, CONST_PERSISTENT|CONST_CS);
	REGISTER_LONG_CONSTANT("INFO_CONFIGURATION", PHP_INFO_CONFIGURATION, CONST_PERSISTENT|CONST_CS);
	REGISTER_LONG_CONSTANT("INFO_MODULES", PHP_INFO_MODULES, CONST_PERSISTENT|CONST_CS);
	REGISTER_LONG_CONSTANT("INFO_ENVIRONMENT", PHP_INFO_ENVIRONMENT, CONST_PERSISTENT|CONST_CS);
	REGISTER_LONG_CONSTANT("INFO_VARIABLES", PHP_INFO_VARIABLES, CONST_PERSISTENT|CONST_CS);
	REGISTER_LONG_CONSTANT("INFO_LICENSE", PHP_INFO_LICENSE, CONST_PERSISTENT|CONST_CS);
	REGISTER_LONG_CONSTANT("INFO_ALL", PHP_INFO_ALL, CONST_PERSISTENT|CONST_CS);
	REGISTER_LONG_CONSTANT("CREDITS_GROUP",	PHP_CREDITS_GROUP, CONST_PERSISTENT|CONST_CS);
	REGISTER_LONG_CONSTANT("CREDITS_GENERAL",	PHP_CREDITS_GENERAL, CONST_PERSISTENT|CONST_CS);
	REGISTER_LONG_CONSTANT("CREDITS_SAPI",	PHP_CREDITS_SAPI, CONST_PERSISTENT|CONST_CS);
	REGISTER_LONG_CONSTANT("CREDITS_MODULES",	PHP_CREDITS_MODULES, CONST_PERSISTENT|CONST_CS);
	REGISTER_LONG_CONSTANT("CREDITS_DOCS",	PHP_CREDITS_DOCS, CONST_PERSISTENT|CONST_CS);
	REGISTER_LONG_CONSTANT("CREDITS_FULLPAGE",	PHP_CREDITS_FULLPAGE, CONST_PERSISTENT|CONST_CS);
	REGISTER_LONG_CONSTANT("CREDITS_QA",	PHP_CREDITS_QA, CONST_PERSISTENT|CONST_CS);
	REGISTER_LONG_CONSTANT("CREDITS_ALL",	PHP_CREDITS_ALL, CONST_PERSISTENT|CONST_CS);
}
/* }}} */

/* {{{ proto void phpinfo([int what])
   Output a page of useful information about PHP and the current request */
PHP_FUNCTION(phpinfo)
{
	long flag = PHP_INFO_ALL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|l", &flag) == FAILURE) {
		return;
	}

	/* Andale!  Andale!  Yee-Hah! */
	php_start_ob_buffer(NULL, 4096, 0 TSRMLS_CC);
	php_print_info(flag TSRMLS_CC);
	php_end_ob_buffer(1, 0 TSRMLS_CC);

	RETURN_TRUE;
}

/* }}} */

/* {{{ proto string phpversion([string extension])
   Return the current PHP version */
PHP_FUNCTION(phpversion)
{
	zval **arg;
	const char *version;
	int argc = ZEND_NUM_ARGS();

	if (argc == 0) {
		RETURN_STRING(PHP_VERSION, 1);
	} else {
		if (zend_parse_parameters(argc TSRMLS_CC, "Z", &arg) == FAILURE) {
			return;
		}
			
		convert_to_string_ex(arg);
		version = zend_get_module_version(Z_STRVAL_PP(arg));
		
		if (version == NULL) {
			RETURN_FALSE;
		}
		RETURN_STRING(version, 1);
	}
}
/* }}} */

/* {{{ proto void phpcredits([int flag])
   Prints the list of people who've contributed to the PHP project */
PHP_FUNCTION(phpcredits)
{
	long flag = PHP_CREDITS_ALL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|l", &flag) == FAILURE) {
		return;
	}

	php_print_credits(flag TSRMLS_CC);
	RETURN_TRUE;
}
/* }}} */


/* {{{ php_logo_guid
 */
PHPAPI char *php_logo_guid(void)
{
	char *logo_guid;

	time_t the_time;
	struct tm *ta, tmbuf;

	the_time = time(NULL);
	ta = php_localtime_r(&the_time, &tmbuf);

	if (ta && (ta->tm_mon==3) && (ta->tm_mday==1)) {
		logo_guid = PHP_EGG_LOGO_GUID;
	} else {
		logo_guid = PHP_LOGO_GUID;
	}

	return estrdup(logo_guid);

}
/* }}} */

/* {{{ proto string php_logo_guid(void)
   Return the special ID used to request the PHP logo in phpinfo screens*/
PHP_FUNCTION(php_logo_guid)
{

	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}

	RETURN_STRING(php_logo_guid(), 0);
}
/* }}} */

/* {{{ proto string php_real_logo_guid(void)
   Return the special ID used to request the PHP logo in phpinfo screens*/
PHP_FUNCTION(php_real_logo_guid)
{

	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}

	RETURN_STRINGL(PHP_LOGO_GUID, sizeof(PHP_LOGO_GUID)-1, 1);
}
/* }}} */

/* {{{ proto string php_egg_logo_guid(void)
   Return the special ID used to request the PHP logo in phpinfo screens*/
PHP_FUNCTION(php_egg_logo_guid)
{
	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}

	RETURN_STRINGL(PHP_EGG_LOGO_GUID, sizeof(PHP_EGG_LOGO_GUID)-1, 1);
}
/* }}} */

/* {{{ proto string zend_logo_guid(void)
   Return the special ID used to request the Zend logo in phpinfo screens*/
PHP_FUNCTION(zend_logo_guid)
{
	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}

	RETURN_STRINGL(ZEND_LOGO_GUID, sizeof(ZEND_LOGO_GUID)-1, 1);
}
/* }}} */

/* {{{ proto string php_sapi_name(void)
   Return the current SAPI module name */
PHP_FUNCTION(php_sapi_name)
{
	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}

	if (sapi_module.name) {
		RETURN_STRING(sapi_module.name, 1);
	} else {
		RETURN_FALSE;
	}
}

/* }}} */

/* {{{ proto string php_uname(void)
   Return information about the system PHP was built on */
PHP_FUNCTION(php_uname)
{
	char *mode = "a";
	int modelen = sizeof("a")-1;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|s", &mode, &modelen) == FAILURE) {
		return;
	}
	RETURN_STRING(php_get_uname(*mode), 0);
}

/* }}} */

/* {{{ proto string php_ini_scanned_files(void)
   Return comma-separated string of .ini files parsed from the additional ini dir */
PHP_FUNCTION(php_ini_scanned_files)
{
	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}
	
	if (strlen(PHP_CONFIG_FILE_SCAN_DIR) && php_ini_scanned_files) {
		RETURN_STRING(php_ini_scanned_files, 1);
	} else {
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ proto string php_ini_loaded_file(void)
   Return the actual loaded ini filename */
PHP_FUNCTION(php_ini_loaded_file)
{
	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}
	
	if (php_ini_opened_path) {
		RETURN_STRING(php_ini_opened_path, 1);
	} else {
		RETURN_FALSE;
	}
}
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
