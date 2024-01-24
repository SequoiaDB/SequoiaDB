/* 
   +----------------------------------------------------------------------+
   | PHP Version 5                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2011 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Original design:  Shane Caraveo <shane@caraveo.com>                  |
   | Authors: Andi Gutmans <andi@zend.com>                                |
   |          Zeev Suraski <zeev@zend.com>                                |
   +----------------------------------------------------------------------+
*/

/* $Id: SAPI.c 309319 2011-03-16 23:54:14Z pajoye $ */

#include <ctype.h>
#include <sys/stat.h>

#include "php.h"
#include "SAPI.h"
#include "php_variables.h"
#include "php_ini.h"
#include "ext/standard/php_string.h"
#include "ext/standard/pageinfo.h"
#if (HAVE_PCRE || HAVE_BUNDLED_PCRE) && !defined(COMPILE_DL_PCRE)
#include "ext/pcre/php_pcre.h"
#endif
#ifdef ZTS
#include "TSRM.h"
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#include "rfc1867.h"

#ifdef PHP_WIN32
#define STRCASECMP stricmp
#else
#define STRCASECMP strcasecmp
#endif

#include "php_content_types.h"

#ifdef ZTS
SAPI_API int sapi_globals_id;
#else
sapi_globals_struct sapi_globals;
#endif

static void sapi_globals_ctor(sapi_globals_struct *sapi_globals TSRMLS_DC)
{
	memset(sapi_globals, 0, sizeof(*sapi_globals));
	zend_hash_init_ex(&sapi_globals->known_post_content_types, 5, NULL, NULL, 1, 0);
	php_setup_sapi_content_types(TSRMLS_C);
}

static void sapi_globals_dtor(sapi_globals_struct *sapi_globals TSRMLS_DC)
{
	zend_hash_destroy(&sapi_globals->known_post_content_types);
}

/* True globals (no need for thread safety) */
SAPI_API sapi_module_struct sapi_module;


SAPI_API void sapi_startup(sapi_module_struct *sf)
{
	sf->ini_entries = NULL;
	sapi_module = *sf;

#ifdef ZTS
	ts_allocate_id(&sapi_globals_id, sizeof(sapi_globals_struct), (ts_allocate_ctor) sapi_globals_ctor, (ts_allocate_dtor) sapi_globals_dtor);
# ifdef PHP_WIN32
	_configthreadlocale(_ENABLE_PER_THREAD_LOCALE);
# endif
#else
	sapi_globals_ctor(&sapi_globals);
#endif

	virtual_cwd_startup(); /* Could use shutdown to free the main cwd but it would just slow it down for CGI */

#ifdef PHP_WIN32
	tsrm_win32_startup();
#endif

	reentrancy_startup();
}

SAPI_API void sapi_shutdown(void)
{
#ifdef ZTS
	ts_free_id(sapi_globals_id);
#else
	sapi_globals_dtor(&sapi_globals);
#endif

	reentrancy_shutdown();

	virtual_cwd_shutdown();

#ifdef PHP_WIN32
	tsrm_win32_shutdown();
#endif
}


SAPI_API void sapi_free_header(sapi_header_struct *sapi_header)
{
	efree(sapi_header->header);
}


SAPI_API void sapi_handle_post(void *arg TSRMLS_DC)
{
	if (SG(request_info).post_entry && SG(request_info).content_type_dup) {
		SG(request_info).post_entry->post_handler(SG(request_info).content_type_dup, arg TSRMLS_CC);
		if (SG(request_info).post_data) {
			efree(SG(request_info).post_data);
			SG(request_info).post_data = NULL;
		}
		efree(SG(request_info).content_type_dup);
		SG(request_info).content_type_dup = NULL;
	}
}

static void sapi_read_post_data(TSRMLS_D)
{
	sapi_post_entry *post_entry;
	uint content_type_length = strlen(SG(request_info).content_type);
	char *content_type = estrndup(SG(request_info).content_type, content_type_length);
	char *p;
	char oldchar=0;
	void (*post_reader_func)(TSRMLS_D) = NULL;


	/* dedicated implementation for increased performance:
	 * - Make the content type lowercase
	 * - Trim descriptive data, stay with the content-type only
	 */
	for (p=content_type; p<content_type+content_type_length; p++) {
		switch (*p) {
			case ';':
			case ',':
			case ' ':
				content_type_length = p-content_type;
				oldchar = *p;
				*p = 0;
				break;
			default:
				*p = tolower(*p);
				break;
		}
	}

	/* now try to find an appropriate POST content handler */
	if (zend_hash_find(&SG(known_post_content_types), content_type,
			content_type_length+1, (void **) &post_entry) == SUCCESS) {
		/* found one, register it for use */
		SG(request_info).post_entry = post_entry;
		post_reader_func = post_entry->post_reader;
	} else {
		/* fallback */
		SG(request_info).post_entry = NULL;
		if (!sapi_module.default_post_reader) {
			/* no default reader ? */
			SG(request_info).content_type_dup = NULL;
			sapi_module.sapi_error(E_WARNING, "Unsupported content type:  '%s'", content_type);
			return;
		}
	}
	if (oldchar) {
		*(p-1) = oldchar;
	}

	SG(request_info).content_type_dup = content_type;

	if(post_reader_func) {
		post_reader_func(TSRMLS_C);
	}

	if(sapi_module.default_post_reader) {
		sapi_module.default_post_reader(TSRMLS_C);
	}
}


SAPI_API SAPI_POST_READER_FUNC(sapi_read_standard_form_data)
{
	int read_bytes;
	int allocated_bytes=SAPI_POST_BLOCK_SIZE+1;

	if ((SG(post_max_size) > 0) && (SG(request_info).content_length > SG(post_max_size))) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "POST Content-Length of %ld bytes exceeds the limit of %ld bytes",
					SG(request_info).content_length, SG(post_max_size));
		return;
	}
	SG(request_info).post_data = emalloc(allocated_bytes);

	for (;;) {
		read_bytes = sapi_module.read_post(SG(request_info).post_data+SG(read_post_bytes), SAPI_POST_BLOCK_SIZE TSRMLS_CC);
		if (read_bytes<=0) {
			break;
		}
		SG(read_post_bytes) += read_bytes;
		if ((SG(post_max_size) > 0) && (SG(read_post_bytes) > SG(post_max_size))) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Actual POST length does not match Content-Length, and exceeds %ld bytes", SG(post_max_size));
			break;
		}
		if (read_bytes < SAPI_POST_BLOCK_SIZE) {
			break;
		}
		if (SG(read_post_bytes)+SAPI_POST_BLOCK_SIZE >= allocated_bytes) {
			allocated_bytes = SG(read_post_bytes)+SAPI_POST_BLOCK_SIZE+1;
			SG(request_info).post_data = erealloc(SG(request_info).post_data, allocated_bytes);
		}
	}
	SG(request_info).post_data[SG(read_post_bytes)] = 0;  /* terminating NULL */
	SG(request_info).post_data_length = SG(read_post_bytes);
}


SAPI_API char *sapi_get_default_content_type(TSRMLS_D)
{
	char *mimetype, *charset, *content_type;

	mimetype = SG(default_mimetype) ? SG(default_mimetype) : SAPI_DEFAULT_MIMETYPE;
	charset = SG(default_charset) ? SG(default_charset) : SAPI_DEFAULT_CHARSET;

	if (strncasecmp(mimetype, "text/", 5) == 0 && *charset) {
		int len = strlen(mimetype) + sizeof("; charset=") + strlen(charset); /* sizeof() includes \0 */
		content_type = emalloc(len);
		snprintf(content_type, len, "%s; charset=%s", mimetype, charset);
	} else {
		content_type = estrdup(mimetype);
	}
	return content_type;
}


SAPI_API void sapi_get_default_content_type_header(sapi_header_struct *default_header TSRMLS_DC)
{
	char *default_content_type = sapi_get_default_content_type(TSRMLS_C);
	int default_content_type_len = strlen(default_content_type);

	default_header->header_len = (sizeof("Content-type: ")-1) + default_content_type_len;
	default_header->header = emalloc(default_header->header_len+1);
	memcpy(default_header->header, "Content-type: ", sizeof("Content-type: "));
	memcpy(default_header->header+sizeof("Content-type: ")-1, default_content_type, default_content_type_len);
	default_header->header[default_header->header_len] = 0;
	efree(default_content_type);
}

/*
 * Add charset on content-type header if the MIME type starts with
 * "text/", the default_charset directive is not empty and
 * there is not already a charset option in there.
 *
 * If "mimetype" is non-NULL, it should point to a pointer allocated
 * with emalloc().  If a charset is added, the string will be
 * re-allocated and the new length is returned.  If mimetype is
 * unchanged, 0 is returned.
 *
 */
SAPI_API size_t sapi_apply_default_charset(char **mimetype, size_t len TSRMLS_DC)
{
	char *charset, *newtype;
	size_t newlen;
	charset = SG(default_charset) ? SG(default_charset) : SAPI_DEFAULT_CHARSET;

	if (*mimetype != NULL) {
		if (*charset && strncmp(*mimetype, "text/", 5) == 0 && strstr(*mimetype, "charset=") == NULL) {
			newlen = len + (sizeof(";charset=")-1) + strlen(charset);
			newtype = emalloc(newlen + 1);
	 		PHP_STRLCPY(newtype, *mimetype, newlen + 1, len);
			strlcat(newtype, ";charset=", newlen + 1);
			strlcat(newtype, charset, newlen + 1);
			efree(*mimetype);
			*mimetype = newtype;
			return newlen;
		}
	}
	return 0;
}

SAPI_API void sapi_activate_headers_only(TSRMLS_D)
{
	if (SG(request_info).headers_read == 1)
		return;
	SG(request_info).headers_read = 1;
	zend_llist_init(&SG(sapi_headers).headers, sizeof(sapi_header_struct), 
			(void (*)(void *)) sapi_free_header, 0);
	SG(sapi_headers).send_default_content_type = 1;

	/* SG(sapi_headers).http_response_code = 200; */ 
	SG(sapi_headers).http_status_line = NULL;
	SG(sapi_headers).mimetype = NULL;
	SG(read_post_bytes) = 0;
	SG(request_info).post_data = NULL;
	SG(request_info).raw_post_data = NULL;
	SG(request_info).current_user = NULL;
	SG(request_info).current_user_length = 0;
	SG(request_info).no_headers = 0;
	SG(request_info).post_entry = NULL;
	SG(global_request_time) = 0;

	/*
	 * It's possible to override this general case in the activate() callback, 
	 * if necessary.
	 */
	if (SG(request_info).request_method && !strcmp(SG(request_info).request_method, "HEAD")) {
		SG(request_info).headers_only = 1;
	} else {
		SG(request_info).headers_only = 0;
	}
	if (SG(server_context)) {
		SG(request_info).cookie_data = sapi_module.read_cookies(TSRMLS_C);
		if (sapi_module.activate) {
			sapi_module.activate(TSRMLS_C);
		}
	}
	if (sapi_module.input_filter_init ) {
		sapi_module.input_filter_init(TSRMLS_C);
	}
}

/*
 * Called from php_request_startup() for every request.
 */

SAPI_API void sapi_activate(TSRMLS_D)
{
	zend_llist_init(&SG(sapi_headers).headers, sizeof(sapi_header_struct), (void (*)(void *)) sapi_free_header, 0);
	SG(sapi_headers).send_default_content_type = 1;

	/*
	SG(sapi_headers).http_response_code = 200;
	*/
	SG(sapi_headers).http_status_line = NULL;
	SG(sapi_headers).mimetype = NULL;
	SG(headers_sent) = 0;
	SG(read_post_bytes) = 0;
	SG(request_info).post_data = NULL;
	SG(request_info).raw_post_data = NULL;
	SG(request_info).current_user = NULL;
	SG(request_info).current_user_length = 0;
	SG(request_info).no_headers = 0;
	SG(request_info).post_entry = NULL;
	SG(request_info).proto_num = 1000; /* Default to HTTP 1.0 */
	SG(global_request_time) = 0;

	/* It's possible to override this general case in the activate() callback, if
	 * necessary.
	 */
	if (SG(request_info).request_method && !strcmp(SG(request_info).request_method, "HEAD")) {
		SG(request_info).headers_only = 1;
	} else {
		SG(request_info).headers_only = 0;
	}
	SG(rfc1867_uploaded_files) = NULL;

	/* handle request mehtod */
	if (SG(server_context)) {
		if ( SG(request_info).request_method) {
			if(!strcmp(SG(request_info).request_method, "POST")
			   && (SG(request_info).content_type)) {
				/* HTTP POST -> may contain form data to be read into variables
				   depending on content type given
				*/
				sapi_read_post_data(TSRMLS_C);
			} else {
				/* any other method with content payload will fill 
				   $HTTP_RAW_POST_DATA if enabled by always_populate_raw_post_data 
				   it is up to the webserver to decide whether to allow a method or not
				*/
				SG(request_info).content_type_dup = NULL;
				if(sapi_module.default_post_reader) {
					sapi_module.default_post_reader(TSRMLS_C);
				}
			}
		} else {
			SG(request_info).content_type_dup = NULL;
		}

		/* Cookies */
		SG(request_info).cookie_data = sapi_module.read_cookies(TSRMLS_C);
		if (sapi_module.activate) {
			sapi_module.activate(TSRMLS_C);
		}
	}
	if (sapi_module.input_filter_init ) {
		sapi_module.input_filter_init(TSRMLS_C);
	}
}


static void sapi_send_headers_free(TSRMLS_D)
{
	if (SG(sapi_headers).http_status_line) {
		efree(SG(sapi_headers).http_status_line);
		SG(sapi_headers).http_status_line = NULL;
	}
}
	
SAPI_API void sapi_deactivate(TSRMLS_D)
{
	zend_llist_destroy(&SG(sapi_headers).headers);
	if (SG(request_info).post_data) {
		efree(SG(request_info).post_data);
	}  else 	if (SG(server_context)) {
		if(sapi_module.read_post) { 
			/* make sure we've consumed all request input data */
			char dummy[SAPI_POST_BLOCK_SIZE];
			int read_bytes;

			while((read_bytes = sapi_module.read_post(dummy, sizeof(dummy)-1 TSRMLS_CC)) > 0) {
				SG(read_post_bytes) += read_bytes;
			}
		}
	}
	if (SG(request_info).raw_post_data) {
		efree(SG(request_info).raw_post_data);
	} 
	if (SG(request_info).auth_user) {
		efree(SG(request_info).auth_user);
	}
	if (SG(request_info).auth_password) {
		efree(SG(request_info).auth_password);
	}
	if (SG(request_info).auth_digest) {
		efree(SG(request_info).auth_digest);
	}
	if (SG(request_info).content_type_dup) {
		efree(SG(request_info).content_type_dup);
	}
	if (SG(request_info).current_user) {
		efree(SG(request_info).current_user);
	}
	if (sapi_module.deactivate) {
		sapi_module.deactivate(TSRMLS_C);
	}
	if (SG(rfc1867_uploaded_files)) {
		destroy_uploaded_files_hash(TSRMLS_C);
	}
	if (SG(sapi_headers).mimetype) {
		efree(SG(sapi_headers).mimetype);
		SG(sapi_headers).mimetype = NULL;
	}
	sapi_send_headers_free(TSRMLS_C);
	SG(sapi_started) = 0;
	SG(headers_sent) = 0;
	SG(request_info).headers_read = 0;
	SG(global_request_time) = 0;
}


SAPI_API void sapi_initialize_empty_request(TSRMLS_D)
{
	SG(server_context) = NULL;
	SG(request_info).request_method = NULL;
	SG(request_info).auth_digest = SG(request_info).auth_user = SG(request_info).auth_password = NULL;
	SG(request_info).content_type_dup = NULL;
}


static int sapi_extract_response_code(const char *header_line)
{
	int code = 200;
	const char *ptr;

	for (ptr = header_line; *ptr; ptr++) {
		if (*ptr == ' ' && *(ptr + 1) != ' ') {
			code = atoi(ptr + 1);
			break;
		}
	}
	
	return code;
}


static void sapi_update_response_code(int ncode TSRMLS_DC)
{
	/* if the status code did not change, we do not want
	   to change the status line, and no need to change the code */
	if (SG(sapi_headers).http_response_code == ncode) {
		return;
	}

	if (SG(sapi_headers).http_status_line) {
		efree(SG(sapi_headers).http_status_line);
		SG(sapi_headers).http_status_line = NULL;
	}
	SG(sapi_headers).http_response_code = ncode;
}

static int sapi_find_matching_header(void *element1, void *element2)
{
	int len = strlen((char*)element2);
	return strncasecmp(((sapi_header_struct*)element1)->header, (char*)element2, len) == 0 && ((sapi_header_struct*)element1)->header[len] == ':';
}

SAPI_API int sapi_add_header_ex(char *header_line, uint header_line_len, zend_bool duplicate, zend_bool replace TSRMLS_DC)
{
	sapi_header_line ctr = {0};
	int r;
	
	ctr.line = header_line;
	ctr.line_len = header_line_len;

	r = sapi_header_op(replace ? SAPI_HEADER_REPLACE : SAPI_HEADER_ADD,
			&ctr TSRMLS_CC);

	if (!duplicate)
		efree(header_line);

	return r;
}

SAPI_API int sapi_header_op(sapi_header_op_enum op, void *arg TSRMLS_DC)
{
	int retval;
	sapi_header_struct sapi_header;
	char *colon_offset;
	long myuid = 0L;
	char *header_line;
	uint header_line_len;
	int http_response_code;
	
	if (SG(headers_sent) && !SG(request_info).no_headers) {
		char *output_start_filename = php_get_output_start_filename(TSRMLS_C);
		int output_start_lineno = php_get_output_start_lineno(TSRMLS_C);

		if (output_start_filename) {
			sapi_module.sapi_error(E_WARNING, "Cannot modify header information - headers already sent by (output started at %s:%d)",
				output_start_filename, output_start_lineno);
		} else {
			sapi_module.sapi_error(E_WARNING, "Cannot modify header information - headers already sent");
		}
		return FAILURE;
	}

	switch (op) {
		case SAPI_HEADER_SET_STATUS:
			sapi_update_response_code((int)(zend_intptr_t) arg TSRMLS_CC);
			return SUCCESS;

		case SAPI_HEADER_ADD:
		case SAPI_HEADER_REPLACE:
		case SAPI_HEADER_DELETE: {
				sapi_header_line *p = arg;

				if (!p->line || !p->line_len) {
					return FAILURE;
				}
				header_line = p->line;
				header_line_len = p->line_len;
				http_response_code = p->response_code;
				break;
			}

		case SAPI_HEADER_DELETE_ALL:
			if (sapi_module.header_handler) {
				sapi_module.header_handler(&sapi_header, op, &SG(sapi_headers) TSRMLS_CC);
			}
			zend_llist_clean(&SG(sapi_headers).headers);
			return SUCCESS;

		default:
			return FAILURE;
	}

	header_line = estrndup(header_line, header_line_len);

	/* cut of trailing spaces, linefeeds and carriage-returns */
	while(header_line_len && isspace(header_line[header_line_len-1])) 
		  header_line[--header_line_len]='\0';
	
	if (op == SAPI_HEADER_DELETE) {
		if (strchr(header_line, ':')) {
			efree(header_line);
			sapi_module.sapi_error(E_WARNING, "Header to delete may not contain colon.");
			return FAILURE;
		}
	} else {
		/* new line safety check */
		char *s = header_line, *e = header_line + header_line_len, *p;
		while (s < e && (p = memchr(s, '\n', (e - s)))) {
			if (*(p + 1) == ' ' || *(p + 1) == '\t') {
				s = p + 1;
				continue;
			}
			efree(header_line);
			sapi_module.sapi_error(E_WARNING, "Header may not contain more than a single header, new line detected.");
			return FAILURE;
		}
	}

	sapi_header.header = header_line;
	sapi_header.header_len = header_line_len;

	if (op == SAPI_HEADER_DELETE) {
		if (sapi_module.header_handler) {
			sapi_module.header_handler(&sapi_header, op, &SG(sapi_headers) TSRMLS_CC);
		}
		zend_llist_del_element(&SG(sapi_headers).headers, sapi_header.header, (int(*)(void*, void*))sapi_find_matching_header);
		sapi_free_header(&sapi_header);
		return SUCCESS;
	}

	/* Check the header for a few cases that we have special support for in SAPI */
	if (header_line_len>=5 
		&& !strncasecmp(header_line, "HTTP/", 5)) {
		/* filter out the response code */
		sapi_update_response_code(sapi_extract_response_code(header_line) TSRMLS_CC);
		/* sapi_update_response_code doesn't free the status line if the code didn't change */
		if (SG(sapi_headers).http_status_line) {
			efree(SG(sapi_headers).http_status_line);
		}
		SG(sapi_headers).http_status_line = header_line;
		return SUCCESS;
	} else {
		colon_offset = strchr(header_line, ':');
		if (colon_offset) {
			*colon_offset = 0;
			if (!STRCASECMP(header_line, "Content-Type")) {
				char *ptr = colon_offset+1, *mimetype = NULL, *newheader;
				size_t len = header_line_len - (ptr - header_line), newlen;
				while (*ptr == ' ') {
					ptr++;
					len--;
				}

				/* Disable possible output compression for images */
				if (!strncmp(ptr, "image/", sizeof("image/")-1)) {
					zend_alter_ini_entry("zlib.output_compression", sizeof("zlib.output_compression"), "0", sizeof("0") - 1, PHP_INI_USER, PHP_INI_STAGE_RUNTIME);
				}

				mimetype = estrdup(ptr);
				newlen = sapi_apply_default_charset(&mimetype, len TSRMLS_CC);
				if (!SG(sapi_headers).mimetype){
					SG(sapi_headers).mimetype = estrdup(mimetype);
				}

				if (newlen != 0) {
					newlen += sizeof("Content-type: ");
					newheader = emalloc(newlen);
					PHP_STRLCPY(newheader, "Content-type: ", newlen, sizeof("Content-type: ")-1);
					strlcat(newheader, mimetype, newlen);
					sapi_header.header = newheader;
					sapi_header.header_len = newlen - 1;
					efree(header_line);
				}
				efree(mimetype);
				SG(sapi_headers).send_default_content_type = 0;
			} else if (!STRCASECMP(header_line, "Location")) {
				if ((SG(sapi_headers).http_response_code < 300 ||
					SG(sapi_headers).http_response_code > 307) &&
					SG(sapi_headers).http_response_code != 201) {
					/* Return a Found Redirect if one is not already specified */
					if (http_response_code) { /* user specified redirect code */
						sapi_update_response_code(http_response_code TSRMLS_CC);
					} else if (SG(request_info).proto_num > 1000 && 
					   SG(request_info).request_method && 
					   strcmp(SG(request_info).request_method, "HEAD") &&
					   strcmp(SG(request_info).request_method, "GET")) {
						sapi_update_response_code(303 TSRMLS_CC);
					} else {
						sapi_update_response_code(302 TSRMLS_CC);
					}
				}
			} else if (!STRCASECMP(header_line, "WWW-Authenticate")) { /* HTTP Authentication */

				sapi_update_response_code(401 TSRMLS_CC); /* authentication-required */

				if(PG(safe_mode)) 
#if (HAVE_PCRE || HAVE_BUNDLED_PCRE) && !defined(COMPILE_DL_PCRE)
				{
					zval *repl_temp;
					char *ptr = colon_offset+1, *result, *newheader;
					int ptr_len=0, result_len = 0, newlen = 0;

					/* skip white space */
					while (isspace(*ptr)) {
						ptr++;
					}

					myuid = php_getuid();

					ptr_len = strlen(ptr);
					MAKE_STD_ZVAL(repl_temp);
					Z_TYPE_P(repl_temp) = IS_STRING;
					Z_STRLEN_P(repl_temp) = spprintf(&Z_STRVAL_P(repl_temp), 0, "realm=\"\\1-%ld\"", myuid);
					/* Modify quoted realm value */
					result = php_pcre_replace("/realm=\"(.*?)\"/i", 16,
											 ptr, ptr_len,
											 repl_temp,
											 0, &result_len, -1, NULL TSRMLS_CC);
					if(result_len==ptr_len) {
						efree(result);
						efree(Z_STRVAL_P(repl_temp));
						Z_STRLEN_P(repl_temp) = spprintf(&Z_STRVAL_P(repl_temp), 0, "realm=\\1-%ld\\2", myuid);
						/* modify unquoted realm value */
						result = php_pcre_replace("/realm=([^\\s]+)(.*)/i", 21, 
											 	ptr, ptr_len,
											 	repl_temp,
											 	0, &result_len, -1, NULL TSRMLS_CC);
						if(result_len==ptr_len) {
							char *lower_temp = estrdup(ptr);	
							char conv_temp[32];
							int conv_len;

							php_strtolower(lower_temp,strlen(lower_temp));
							/* If there is no realm string at all, append one */
							if(!strstr(lower_temp,"realm")) {
								efree(result);
								conv_len = slprintf(conv_temp, sizeof(conv_temp), " realm=\"%ld\"",myuid);
								result = emalloc(ptr_len+conv_len+1);
								result_len = ptr_len+conv_len;
								memcpy(result, ptr, ptr_len);	
								memcpy(result+ptr_len, conv_temp, conv_len);
								*(result+ptr_len+conv_len) = '\0';
							}
							efree(lower_temp);
						}
					}
					newlen = spprintf(&newheader, 0, "WWW-Authenticate: %s", result);
					efree(header_line);
					sapi_header.header = newheader;
					sapi_header.header_len = newlen;
					efree(result);
					efree(Z_STRVAL_P(repl_temp));
					efree(repl_temp);
				} 
#else
				{
					myuid = php_getuid();
					efree(header_line);
					sapi_header.header_len = spprintf(&sapi_header.header, 0, "WWW-Authenticate: Basic realm=\"%ld\"", myuid);
				}
#endif
			}
			if (sapi_header.header==header_line) {
				*colon_offset = ':';
			}
		}
	}
	if (http_response_code) {
		sapi_update_response_code(http_response_code TSRMLS_CC);
	}
	if (sapi_module.header_handler) {
		retval = sapi_module.header_handler(&sapi_header, op, &SG(sapi_headers) TSRMLS_CC);
	} else {
		retval = SAPI_HEADER_ADD;
	}
	if (retval & SAPI_HEADER_ADD) {
		/* in replace mode first remove the header if it already exists in the headers llist */
		if (op == SAPI_HEADER_REPLACE) {
			colon_offset = strchr(sapi_header.header, ':');
			if (colon_offset) {
				char sav;
				sav = *colon_offset;
				*colon_offset = 0;
				zend_llist_del_element(&SG(sapi_headers).headers, sapi_header.header, (int(*)(void*, void*))sapi_find_matching_header);
				*colon_offset = sav;
			}
		}

		zend_llist_add_element(&SG(sapi_headers).headers, (void *) &sapi_header);
	} else {
		sapi_free_header(&sapi_header);
	}
	return SUCCESS;
}


SAPI_API int sapi_send_headers(TSRMLS_D)
{
	int retval;
	int ret = FAILURE;

	if (SG(headers_sent) || SG(request_info).no_headers) {
		return SUCCESS;
	}

	/* Success-oriented.  We set headers_sent to 1 here to avoid an infinite loop
	 * in case of an error situation.
	 */
	if (SG(sapi_headers).send_default_content_type && sapi_module.send_headers) {
		sapi_header_struct default_header;
		sapi_get_default_content_type_header(&default_header TSRMLS_CC);
		sapi_add_header_ex(default_header.header, default_header.header_len, 0, 0 TSRMLS_CC);
	}

	SG(headers_sent) = 1;

	if (sapi_module.send_headers) {
		retval = sapi_module.send_headers(&SG(sapi_headers) TSRMLS_CC);
	} else {
		retval = SAPI_HEADER_DO_SEND;
	}

	switch (retval) {
		case SAPI_HEADER_SENT_SUCCESSFULLY:
			ret = SUCCESS;
			break;
		case SAPI_HEADER_DO_SEND: {
				sapi_header_struct http_status_line;
				char buf[255];

				if (SG(sapi_headers).http_status_line) {
					http_status_line.header = SG(sapi_headers).http_status_line;
					http_status_line.header_len = strlen(SG(sapi_headers).http_status_line);
				} else {
					http_status_line.header = buf;
					http_status_line.header_len = slprintf(buf, sizeof(buf), "HTTP/1.0 %d X", SG(sapi_headers).http_response_code);
				}
				sapi_module.send_header(&http_status_line, SG(server_context) TSRMLS_CC);
			}
			zend_llist_apply_with_argument(&SG(sapi_headers).headers, (llist_apply_with_arg_func_t) sapi_module.send_header, SG(server_context) TSRMLS_CC);
			if(SG(sapi_headers).send_default_content_type) {
				sapi_header_struct default_header;

				sapi_get_default_content_type_header(&default_header TSRMLS_CC);
				sapi_module.send_header(&default_header, SG(server_context) TSRMLS_CC);
				sapi_free_header(&default_header);
			}
			sapi_module.send_header(NULL, SG(server_context) TSRMLS_CC);
			ret = SUCCESS;
			break;
		case SAPI_HEADER_SEND_FAILED:
			SG(headers_sent) = 0;
			ret = FAILURE;
			break;
	}

	sapi_send_headers_free(TSRMLS_C);

	return ret;
}


SAPI_API int sapi_register_post_entries(sapi_post_entry *post_entries TSRMLS_DC)
{
	sapi_post_entry *p=post_entries;

	while (p->content_type) {
		if (sapi_register_post_entry(p TSRMLS_CC) == FAILURE) {
			return FAILURE;
		}
		p++;
	}
	return SUCCESS;
}


SAPI_API int sapi_register_post_entry(sapi_post_entry *post_entry TSRMLS_DC)
{
	if (SG(sapi_started) && EG(in_execution)) {
		return FAILURE;
	}
	return zend_hash_add(&SG(known_post_content_types),
			post_entry->content_type, post_entry->content_type_len+1,
			(void *) post_entry, sizeof(sapi_post_entry), NULL);
}

SAPI_API void sapi_unregister_post_entry(sapi_post_entry *post_entry TSRMLS_DC)
{
	if (SG(sapi_started) && EG(in_execution)) {
		return;
	}
	zend_hash_del(&SG(known_post_content_types), post_entry->content_type,
			post_entry->content_type_len+1);
}


SAPI_API int sapi_register_default_post_reader(void (*default_post_reader)(TSRMLS_D))
{
	TSRMLS_FETCH();
	if (SG(sapi_started) && EG(in_execution)) {
		return FAILURE;
	}
	sapi_module.default_post_reader = default_post_reader;
	return SUCCESS;
}


SAPI_API int sapi_register_treat_data(void (*treat_data)(int arg, char *str, zval *destArray TSRMLS_DC))
{
	TSRMLS_FETCH();
	if (SG(sapi_started) && EG(in_execution)) {
		return FAILURE;
	}
	sapi_module.treat_data = treat_data;
	return SUCCESS;
}

SAPI_API int sapi_register_input_filter(unsigned int (*input_filter)(int arg, char *var, char **val, unsigned int val_len, unsigned int *new_val_len TSRMLS_DC), unsigned int (*input_filter_init)(TSRMLS_D))
{
	TSRMLS_FETCH();
	if (SG(sapi_started) && EG(in_execution)) {
		return FAILURE;
	}
	sapi_module.input_filter = input_filter;
	sapi_module.input_filter_init = input_filter_init;
	return SUCCESS;
}

SAPI_API int sapi_flush(TSRMLS_D)
{
	if (sapi_module.flush) {
		sapi_module.flush(SG(server_context));
		return SUCCESS;
	} else {
		return FAILURE;
	}
}

SAPI_API struct stat *sapi_get_stat(TSRMLS_D)
{
	if (sapi_module.get_stat) {
		return sapi_module.get_stat(TSRMLS_C);
	} else {
		if (!SG(request_info).path_translated || (VCWD_STAT(SG(request_info).path_translated, &SG(global_stat)) == -1)) {
			return NULL;
		}
		return &SG(global_stat);
	}
}

SAPI_API char *sapi_getenv(char *name, size_t name_len TSRMLS_DC)
{
	if (sapi_module.getenv) { 
		char *value, *tmp = sapi_module.getenv(name, name_len TSRMLS_CC);
		if (tmp) {
			value = estrdup(tmp);
		} else {
			return NULL;
		}
		sapi_module.input_filter(PARSE_ENV, name, &value, strlen(value), NULL TSRMLS_CC);
		return value;
	}
	return NULL;
}

SAPI_API int sapi_get_fd(int *fd TSRMLS_DC)
{
	if (sapi_module.get_fd) {
		return sapi_module.get_fd(fd TSRMLS_CC);
	} else {
		return FAILURE;
	}
}

SAPI_API int sapi_force_http_10(TSRMLS_D)
{
	if (sapi_module.force_http_10) {
		return sapi_module.force_http_10(TSRMLS_C);
	} else {
		return FAILURE;
	}
}


SAPI_API int sapi_get_target_uid(uid_t *obj TSRMLS_DC)
{
	if (sapi_module.get_target_uid) {
		return sapi_module.get_target_uid(obj TSRMLS_CC);
	} else {
		return FAILURE;
	}
}

SAPI_API int sapi_get_target_gid(gid_t *obj TSRMLS_DC)
{
	if (sapi_module.get_target_gid) {
		return sapi_module.get_target_gid(obj TSRMLS_CC);
	} else {
		return FAILURE;
	}
}

SAPI_API time_t sapi_get_request_time(TSRMLS_D)
{
	if(SG(global_request_time)) return SG(global_request_time);

	if (sapi_module.get_request_time && SG(server_context)) {
		SG(global_request_time) = sapi_module.get_request_time(TSRMLS_C);
	} else {
		SG(global_request_time) = time(0);
	}
	return SG(global_request_time);
}

SAPI_API void sapi_terminate_process(TSRMLS_D) {
	if (sapi_module.terminate_process) {
		sapi_module.terminate_process(TSRMLS_C);
	}
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
