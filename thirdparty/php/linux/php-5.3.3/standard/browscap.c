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
   | Author: Zeev Suraski <zeev@zend.com>                                 |
   +----------------------------------------------------------------------+
 */

/* $Id: browscap.c 293036 2010-01-03 09:23:27Z sebastian $ */

#include "php.h"
#include "php_browscap.h"
#include "php_ini.h"
#include "php_string.h"
#include "ext/pcre/php_pcre.h"

#include "zend_ini_scanner.h"
#include "zend_globals.h"

static HashTable browser_hash;
static zval *current_section;
static char *current_section_name;

#define DEFAULT_SECTION_NAME "Default Browser Capability Settings"

/* OBJECTS_FIXME: This whole extension needs going through. The use of objects looks pretty broken here */

static void browscap_entry_dtor(zval **zvalue) /* {{{ */
{
	if (Z_TYPE_PP(zvalue) == IS_ARRAY) {
		zend_hash_destroy(Z_ARRVAL_PP(zvalue));
		free(Z_ARRVAL_PP(zvalue));
	} else if (Z_TYPE_PP(zvalue) == IS_STRING) {
		if (Z_STRVAL_PP(zvalue)) {
			free(Z_STRVAL_PP(zvalue));
		}
	}
	free(*zvalue);
}
/* }}} */

static void convert_browscap_pattern(zval *pattern) /* {{{ */
{
	int i, j=0;
	char *t;

	php_strtolower(Z_STRVAL_P(pattern), Z_STRLEN_P(pattern));

	t = (char *) safe_pemalloc(Z_STRLEN_P(pattern), 2, 5, 1);

	t[j++] = '�';
	t[j++] = '^';

	for (i=0; i<Z_STRLEN_P(pattern); i++, j++) {
		switch (Z_STRVAL_P(pattern)[i]) {
			case '?':
				t[j] = '.';
				break;
			case '*':
				t[j++] = '.';
				t[j] = '*';
				break;
			case '.':
				t[j++] = '\\';
				t[j] = '.';
				break;
			case '\\':
				t[j++] = '\\';
				t[j] = '\\';
				break;
			case '(':
				t[j++] = '\\';
				t[j] = '(';
				break;
			case ')':
				t[j++] = '\\';
				t[j] = ')';
				break;
			case '�':
				t[j++] = '\\';
				t[j] = '�';
				break;
			default:
				t[j] = Z_STRVAL_P(pattern)[i];
				break;
		}
	}

	t[j++] = '$';
	t[j++] = '�';

	t[j]=0;
	Z_STRVAL_P(pattern) = t;
	Z_STRLEN_P(pattern) = j;
}
/* }}} */

static void php_browscap_parser_cb(zval *arg1, zval *arg2, zval *arg3, int callback_type, void *arg TSRMLS_DC) /* {{{ */
{
	if (!arg1) {
		return;
	}

	switch (callback_type) {
		case ZEND_INI_PARSER_ENTRY:
			if (current_section && arg2) {
				zval *new_property;
				char *new_key;

				/* parent entry can not be same as current section -> causes infinite loop! */
				if (!strcasecmp(Z_STRVAL_P(arg1), "parent") && 
					current_section_name != NULL &&
					!strcasecmp(current_section_name, Z_STRVAL_P(arg2))
				) {
					zend_error(E_CORE_ERROR, "Invalid browscap ini file: 'Parent' value cannot be same as the section name: %s (in file %s)", current_section_name, INI_STR("browscap"));
					return;
				}

				new_property = (zval *) pemalloc(sizeof(zval), 1);
				INIT_PZVAL(new_property);
				Z_TYPE_P(new_property) = IS_STRING;

				/* Set proper value for true/false settings */
				if ((Z_STRLEN_P(arg2) == 2 && !strncasecmp(Z_STRVAL_P(arg2), "on", sizeof("on") - 1)) ||
					(Z_STRLEN_P(arg2) == 3 && !strncasecmp(Z_STRVAL_P(arg2), "yes", sizeof("yes") - 1)) ||
					(Z_STRLEN_P(arg2) == 4 && !strncasecmp(Z_STRVAL_P(arg2), "true", sizeof("true") - 1))
				) {
					Z_STRVAL_P(new_property) = zend_strndup("1", 1);
					Z_STRLEN_P(new_property) = 1;
				} else if (
					(Z_STRLEN_P(arg2) == 2 && !strncasecmp(Z_STRVAL_P(arg2), "no", sizeof("no") - 1)) ||
					(Z_STRLEN_P(arg2) == 3 && !strncasecmp(Z_STRVAL_P(arg2), "off", sizeof("off") - 1)) ||
					(Z_STRLEN_P(arg2) == 4 && !strncasecmp(Z_STRVAL_P(arg2), "none", sizeof("none") - 1)) ||
					(Z_STRLEN_P(arg2) == 5 && !strncasecmp(Z_STRVAL_P(arg2), "false", sizeof("false") - 1))
				) {
					Z_STRVAL_P(new_property) = zend_strndup("", 0);
					Z_STRLEN_P(new_property) = 0;
				} else { /* Other than true/false setting */
					Z_STRVAL_P(new_property) = zend_strndup(Z_STRVAL_P(arg2), Z_STRLEN_P(arg2));
					Z_STRLEN_P(new_property) = Z_STRLEN_P(arg2);
				}
				new_key = zend_strndup(Z_STRVAL_P(arg1), Z_STRLEN_P(arg1));
				zend_str_tolower(new_key, Z_STRLEN_P(arg1));
				zend_hash_update(Z_ARRVAL_P(current_section), new_key, Z_STRLEN_P(arg1) + 1, &new_property, sizeof(zval *), NULL);
				free(new_key);
			}
			break;
		case ZEND_INI_PARSER_SECTION: {
				zval *processed;
				zval *unprocessed;
				HashTable *section_properties;

				/*printf("'%s' (%d)\n",$1.value.str.val,$1.value.str.len + 1);*/
				current_section = (zval *) pemalloc(sizeof(zval), 1);
				INIT_PZVAL(current_section);
				processed = (zval *) pemalloc(sizeof(zval), 1);
				INIT_PZVAL(processed);
				unprocessed = (zval *) pemalloc(sizeof(zval), 1);
				INIT_PZVAL(unprocessed);

				section_properties = (HashTable *) pemalloc(sizeof(HashTable), 1);
				zend_hash_init(section_properties, 0, NULL, (dtor_func_t) browscap_entry_dtor, 1);
				Z_ARRVAL_P(current_section) = section_properties;
				Z_TYPE_P(current_section) = IS_ARRAY;
				if (current_section_name) {
					free(current_section_name);
				}
				current_section_name = zend_strndup(Z_STRVAL_P(arg1), Z_STRLEN_P(arg1));

				zend_hash_update(&browser_hash, Z_STRVAL_P(arg1), Z_STRLEN_P(arg1) + 1, (void *) &current_section, sizeof(zval *), NULL);

				Z_STRVAL_P(processed) = Z_STRVAL_P(arg1);
				Z_STRLEN_P(processed) = Z_STRLEN_P(arg1);
				Z_TYPE_P(processed) = IS_STRING;
				Z_STRVAL_P(unprocessed) = Z_STRVAL_P(arg1);
				Z_STRLEN_P(unprocessed) = Z_STRLEN_P(arg1);
				Z_TYPE_P(unprocessed) = IS_STRING;
				Z_STRVAL_P(unprocessed) = zend_strndup(Z_STRVAL_P(unprocessed), Z_STRLEN_P(unprocessed));

				convert_browscap_pattern(processed);
				zend_hash_update(section_properties, "browser_name_regex", sizeof("browser_name_regex"), (void *) &processed, sizeof(zval *), NULL);
				zend_hash_update(section_properties, "browser_name_pattern", sizeof("browser_name_pattern"), (void *) &unprocessed, sizeof(zval *), NULL);
			}
			break;
	}
}
/* }}} */

PHP_MINIT_FUNCTION(browscap) /* {{{ */
{
	char *browscap = INI_STR("browscap");

	if (browscap && browscap[0]) {
		zend_file_handle fh;
		memset(&fh, 0, sizeof(fh));

		if (zend_hash_init_ex(&browser_hash, 0, NULL, (dtor_func_t) browscap_entry_dtor, 1, 0) == FAILURE) {
			return FAILURE;
		}

		fh.handle.fp = VCWD_FOPEN(browscap, "r");
		fh.opened_path = NULL;
		fh.free_filename = 0;
		if (!fh.handle.fp) {
			zend_error(E_CORE_WARNING, "Cannot open '%s' for reading", browscap);
			return FAILURE;
		}
		fh.filename = browscap;
		Z_TYPE(fh) = ZEND_HANDLE_FP;
		current_section_name = NULL;
		zend_parse_ini_file(&fh, 1, ZEND_INI_SCANNER_RAW, (zend_ini_parser_cb_t) php_browscap_parser_cb, &browser_hash TSRMLS_CC);
		if (current_section_name) {
			free(current_section_name);
			current_section_name = NULL;
		}
	}

	return SUCCESS;
}
/* }}} */

PHP_MSHUTDOWN_FUNCTION(browscap) /* {{{ */
{
	char *browscap = INI_STR("browscap");
	if (browscap && browscap[0]) {
		zend_hash_destroy(&browser_hash);
	}
	return SUCCESS;
}
/* }}} */

static int browser_reg_compare(zval **browser TSRMLS_DC, int num_args, va_list args, zend_hash_key *key) /* {{{ */
{
	zval **browser_regex, **previous_match;
	pcre *re;
	int re_options;
	pcre_extra *re_extra;
	char *lookup_browser_name = va_arg(args, char *);
	int lookup_browser_length = va_arg(args, int);
	zval **found_browser_entry = va_arg(args, zval **);

	/* See if we have an exact match, if so, we're done... */
	if (*found_browser_entry) {
		if (zend_hash_find(Z_ARRVAL_PP(found_browser_entry), "browser_name_pattern", sizeof("browser_name_pattern"), (void**) &previous_match) == FAILURE) {
			return 0;
		}
		else if (!strcasecmp(Z_STRVAL_PP(previous_match), lookup_browser_name)) {
			return 0;
		}
	}

	if (zend_hash_find(Z_ARRVAL_PP(browser), "browser_name_regex", sizeof("browser_name_regex"), (void **) &browser_regex) == FAILURE) {
		return 0;
	}

	re = pcre_get_compiled_regex(Z_STRVAL_PP(browser_regex), &re_extra, &re_options TSRMLS_CC);
	if (re == NULL) {
		return 0;
	}

	if (pcre_exec(re, re_extra, lookup_browser_name, lookup_browser_length, 0, re_options, NULL, 0) == 0) {
		/* If we've found a possible browser, we need to do a comparison of the
		   number of characters changed in the user agent being checked versus
		   the previous match found and the current match. */
		if (*found_browser_entry) {
			int i, prev_len = 0, curr_len = 0, ua_len;
			zval **current_match;

			if (zend_hash_find(Z_ARRVAL_PP(browser), "browser_name_pattern", sizeof("browser_name_pattern"), (void**) &current_match) == FAILURE) {
				return 0;
			}

			ua_len = lookup_browser_length;

			for (i = 0; i < Z_STRLEN_PP(previous_match); i++) {
				switch (Z_STRVAL_PP(previous_match)[i]) {
					case '?':
					case '*':
						/* do nothing, ignore these characters in the count */
					break;

					default:
						++prev_len;
				}
			}

			for (i = 0; i < Z_STRLEN_PP(current_match); i++) {
				switch (Z_STRVAL_PP(current_match)[i]) {
					case '?':
					case '*':
						/* do nothing, ignore these characters in the count */
					break;

					default:
						++curr_len;
				}
			}

			/* Pick which browser pattern replaces the least amount of
			   characters when compared to the original user agent string... */
			if (ua_len - prev_len > ua_len - curr_len) {
				*found_browser_entry = *browser;
			}
		}
		else {
			*found_browser_entry = *browser;
		}
	}

	return 0;
}
/* }}} */

/* {{{ proto mixed get_browser([string browser_name [, bool return_array]])
   Get information about the capabilities of a browser. If browser_name is omitted or null, HTTP_USER_AGENT is used. Returns an object by default; if return_array is true, returns an array. */
PHP_FUNCTION(get_browser)
{
	char *agent_name = NULL;
	int agent_name_len = 0;
	zend_bool return_array = 0;
	zval **agent, **z_agent_name, **http_user_agent;
	zval *found_browser_entry, *tmp_copy;
	char *lookup_browser_name;
	char *browscap = INI_STR("browscap");

	if (!browscap || !browscap[0]) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "browscap ini directive not set");
		RETURN_FALSE;
	}

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|s!b", &agent_name, &agent_name_len, &return_array) == FAILURE) {
		return;
	}

	if (agent_name == NULL) {
		zend_is_auto_global("_SERVER", sizeof("_SERVER") - 1 TSRMLS_CC);
		if (!PG(http_globals)[TRACK_VARS_SERVER] ||
			zend_hash_find(HASH_OF(PG(http_globals)[TRACK_VARS_SERVER]), "HTTP_USER_AGENT", sizeof("HTTP_USER_AGENT"), (void **) &http_user_agent) == FAILURE
		) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "HTTP_USER_AGENT variable is not set, cannot determine user agent name");
			RETURN_FALSE;
		}
		agent_name = Z_STRVAL_PP(http_user_agent);
		agent_name_len = Z_STRLEN_PP(http_user_agent);
	}

	lookup_browser_name = estrndup(agent_name, agent_name_len);
	php_strtolower(lookup_browser_name, agent_name_len);

	if (zend_hash_find(&browser_hash, lookup_browser_name, agent_name_len + 1, (void **) &agent) == FAILURE) {
		found_browser_entry = NULL;
		zend_hash_apply_with_arguments(&browser_hash TSRMLS_CC, (apply_func_args_t) browser_reg_compare, 3, lookup_browser_name, agent_name_len, &found_browser_entry);

		if (found_browser_entry) {
			agent = &found_browser_entry;
		} else if (zend_hash_find(&browser_hash, DEFAULT_SECTION_NAME, sizeof(DEFAULT_SECTION_NAME), (void **) &agent) == FAILURE) {
			efree(lookup_browser_name);
			RETURN_FALSE;
		}
	}

	if (return_array) {
		array_init(return_value);
		zend_hash_copy(Z_ARRVAL_P(return_value), Z_ARRVAL_PP(agent), (copy_ctor_func_t) zval_add_ref, (void *) &tmp_copy, sizeof(zval *));
	}
	else {
		object_init(return_value);
		zend_hash_copy(Z_OBJPROP_P(return_value), Z_ARRVAL_PP(agent), (copy_ctor_func_t) zval_add_ref, (void *) &tmp_copy, sizeof(zval *));
	}

	while (zend_hash_find(Z_ARRVAL_PP(agent), "parent", sizeof("parent"), (void **) &z_agent_name) == SUCCESS) {
		if (zend_hash_find(&browser_hash, Z_STRVAL_PP(z_agent_name), Z_STRLEN_PP(z_agent_name) + 1, (void **)&agent) == FAILURE) {
			break;
		}

		if (return_array) {
			zend_hash_merge(Z_ARRVAL_P(return_value), Z_ARRVAL_PP(agent), (copy_ctor_func_t) zval_add_ref, (void *) &tmp_copy, sizeof(zval *), 0);
		}
		else {
			zend_hash_merge(Z_OBJPROP_P(return_value), Z_ARRVAL_PP(agent), (copy_ctor_func_t) zval_add_ref, (void *) &tmp_copy, sizeof(zval *), 0);
		}
	}

	efree(lookup_browser_name);
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
